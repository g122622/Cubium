// 红石比较器右键切换比较/减法模式 + 放置朝向行为 GameTest。
//
// wiki mechanism_红石比较器.txt#（使用键切换）：比较器前端火把状态可由使用键切换——
//   - 火把熄灭 → 比较模式（compare）：比较主输入与侧输入，主输入≥侧输入则输出主输入，否则输出 0。
//   - 火把亮起 → 减法模式（subtract）：输出 = 主输入 − 侧输入（最小 0）。
//   右键在两种模式间切换。默认放置为比较模式（mode=compare）。
// wiki tech_红石中继器.txt#放置（比较器与中继器同属二极管，共享放置朝向规则）：比较器水平放置，
//   facing = opposite(玩家水平视线方向)（输入端朝玩家、输出端背离玩家，水平四向 South/West/North/
//   East）。朝向仅由玩家 yaw 决定，不含 pitch。
//
// C++ 链路：RedstoneComparatorBlock（redstone/RedstoneComparatorBlock.cpp）继承 RedstoneDiodeBlock，state：
//   HORIZONTAL_FACING（"facing"，默认 North）+ POWERED + MODE（"mode"，ComparatorMode 枚举，默认 Compare）。
//   - onBlockActivated（:304）：!mayBuild → Pass；否则 newMode = (cur==Compare)?Subtract:Compare
//     （compare↔subtract 翻转）→ withMode setBlockState 写回 + 点击音效（subtract 音高 0.55，compare 0.5）
//     + updateState 立即触发状态检查 → return Success。不检查手持物（空手/任意物品右键都切模式）。
//   - MODE state 名 "mode"，值 "compare"/"subtract"（EnumProperty<ComparatorMode>，见 :58-84）。
//   - getStateForPlacement（基类 RedstoneDiodeBlock 重写）：facing = opposite(
//     context.horizontalDirection())（水平四向）。比较器继承本方法自动获得正确朝向。此前基类未重写该
//     方法，落回 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒 North），与预期
//     按水平视线决定朝向的行为不一致。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。比较器 onBlockActivated 创造模式 mayBuild=true → 切模式返 Success 短路。
//   用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，仅占位 useItemOnBlock 的
//   ItemStack 形参）。放置朝向走 useItemOnBlock 手持 comparator 物品点击 stone 顶面 Up → BlockItem::tryPlace
//   → getStateForPlacement（基类 facing=opposite(horizontalDirection)）→ setBlockState。
//
// 测试覆盖（4 个场景，覆盖 wiki 右键切换模式 + 翻转回比较模式 + 放置朝向核心确定行为）：
//   1. 右键切到减法模式：放比较器（mode=compare）+ stick useItemOnBlock → mode=subtract，返 true。
//   2. 再次右键切回比较模式：已 mode=subtract + stick useItemOnBlock → mode=compare，返 true。
//   3. 水平 4 朝向放置：pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 barrel/RepeaterTests 4 朝向坐标配方，水平类仅 yaw）。
//   4. 水平视线决定朝向（区分新旧实现）：玩家近水平朝东点击 stone 顶面 Up → facing=West（非 North），
//      断言 facing=West 验证修复（getStateForPlacement override 而非基类 defaultState）。
//
// 关键约束：
// 1. 比较器 noCollision().notSolid()，Diode 无 canSurvive 自毁逻辑（同中继器），悬空放置不自毁。
//    仍放 (3,1,1) stone 支撑 + (3,2,1) 比较器，贴近真实放置且与同目录红石测试惯例一致。
// 2. setBlockType 放比较器带默认 state（facing=North, powered=false, mode=compare）。
//    右键切模式不依赖 facing，默认 North 即可。朝向测试用 useItemOnBlock 放置（走 getStateForPlacement）。
// 3. 读 mode state 用 getState("mode" as any) 绕过 BlockStateSuperset 白名单。mode 是字符串枚举
//    （"compare"/"subtract"），非数字。读 facing 用 getState("facing" as any)（HORIZONTAL_FACING，返小写
//    方向字符串 north/south/east/west）。
// 4. 创造模式 SimulatedPlayer mayBuild=true，onBlockActivated 走切模式分支（非 Pass）。
// 5. 切模式后 onBlockActivated 调 updateState（无信号输入时输出稳定为 0，不影响 mode state 断言）。
// 6. 切模式不消耗手持物（onBlockActivated 无 shrink），stick 可重复使用。
// 7. 场景 3/4 朝向控制（复用 RepeaterTests/BarrelTests 含 pitch lookAtLocation 范式，比较器为水平类
//    facing=opposite(horizontalDirection)，horizontalDirection 仅 yaw 决定，但 lookAt.y=playerPos.y+1 使
//    pitch≈0 与现有范式一致）。每朝向独立 spawn 玩家避免 yaw 残留；每次清理 (3,2,1) 避免比较器残留
//    阻断放置。facing=opposite(玩家水平朝向)。此前 Cubium 基类未重写 getStateForPlacement（基类
//    defaultState，HORIZONTAL_FACING 恒 North），4 朝向放置 facing 全为 north，断言失败；重写后修正。
// 8. 比较器 getShape 是低矮碰撞箱（同中继器），isValidPosition 基类 true 无支撑要求。stone 支撑仅为
//    贴近真实放置；玩家不能站在 placementPos（碰撞检查），场景 3 玩家位置均远离 (3,2,1)。
//
// 不测「比较/减法模式信号计算」：涉红石传导 + 容器/物品框信号源，复杂且本组聚焦右键切模式 state
//   变化，跳过。TODO: 待比较器信号源测试完善后补 compare/subtract 输出差异测试。
//
// 跨服务端：比较器 comparator 方块名两端一致，mode/facing state 名两端一致（mode compare/subtract），
//   右键切换模式 + 放置朝向（facing=opposite(horizontalDirection)）行为两端一致。基岩无 setBlockWithStates，
//   本测试用 setBlockType 放默认 state（mode=compare），两端均可放；右键切 mode 行为两端可对比；朝向
//   测试用 useItemOnBlock 放置（lookAtLocation 是 Cubium 专有朝向控制，但 facing=opposite(水平视线)
//   放置行为两端可对比，非 one-sided）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_红石比较器.txt#（使用键切换前火把：比较↔减法模式）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石中继器.txt#放置（facing=opposite(水平视线)，二极管共享）
// Ref: RedstoneComparatorBlock.cpp（onBlockActivated newMode 翻转+音效+updateState→Success；MODE_PROP state）
// Ref: RedstoneDiodeBlock.cpp（getStateForPlacement facing=opposite(horizontalDirection) 修复，比较器继承）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支）
// Ref: RepeaterTests.ts（同基类 RedstoneDiodeBlock 朝向测试范式 + 水平 4 朝向坐标配方，比较器复用）
// Ref: BarrelTests.ts（含 pitch lookAtLocation 朝向控制范式）
// Ref: DispenserTests.ts / PistonTests.ts（同类缺 getStateForPlacement override 坑模式 + 朝向测试范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 比较器 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取比较器 mode state（"compare" | "subtract"）。返回 null 表示读取失败或非比较器。
// mode state 名 "mode"，值 "compare"/"subtract"（EnumProperty<ComparatorMode>，见 RedstoneComparatorBlock.cpp:58-84）。
function getComparatorMode(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("mode" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 比较器 facing state（方向名字符串 "north"/"south"/"east"/"west"）。
// 返回 null 表示失败或非比较器。HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（水平四向，无 up/down）。
function getComparatorFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 水平 4 朝向放置映射表（复用 RepeaterTests/BarrelTests 的 4 朝向映射与坐标配方，比较器为水平类
// facing=opposite(horizontalDirection)，horizontalDirection 仅 yaw 决定）。
// horizontalDirection=orderedByNearest(yaw,pitch)[0] 的水平分量，pitch≈0 时 = 玩家水平朝向，
// facing=opposite(水平朝向)。lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0（虽比较器 facing 仅由 yaw 决定，但 pitch≈0 与现有范式一致且稳妥）。playerPos.y=2
//   →眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5 放大 horizDist 使 pitch 微小。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 比较器 facing=opposite(玩家水平朝向)
}

// 4 朝向推算（playerPos.y=2→眼高3.62，lookAt.y=3→dy=-0.12→pitch≈1.3°，[0]=水平朝向，facing=opposite）：
//   South（yaw[315,360)∪[0,45)→facing=North）：玩家(1,2,1)，lookAt(3,3,6)，dx=2,dz=5→atan2(-2,5)≈-21.8°→338°→South→North。
//   West（yaw[45,135)→facing=East）：玩家(5,2,1)，lookAt(0,3,1)，dx=-5,dz=0→atan2(5,0)=90°→West→East。
//   North（yaw[135,225)→facing=South）：玩家(1,2,5)，lookAt(3,3,0)，dx=2,dz=-5→atan2(-2,-5)≈158°→North→South。
//   East（yaw[225,315)→facing=West）：玩家(1,2,1)，lookAt(6,3,1)，dx=5,dz=0→atan2(-5,0)=-90°→270°→East→West。
// lookAt 目标 y=3=playerPos.y+1 → dy=-0.12 → pitch≈1.3°（horizDist≥5 放大后），[0]=水平朝向。
// 玩家位置均不与 (3,1,1)/(3,2,1) 重叠；lookAt 目标均在 [0,6] 内不越界。
const FACING_CASES: FacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 3, z: 6 }, expectedFacing: "north" },
    { name: "west", playerPos: { x: 5, y: 2, z: 1 }, lookAt: { x: 0, y: 3, z: 1 }, expectedFacing: "east" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 3, z: 0 }, expectedFacing: "south" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 3, z: 1 }, expectedFacing: "west" },
];

// 放支撑 + 比较器：(3,1,1) stone 支撑，(3,2,1) 比较器（minecraft:comparator 默认 mode=compare, facing=North）。
function placeComparator(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:comparator", { x: 3, y: 2, z: 1 }); // 比较器 mode=compare
}

// 放置朝向测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放比较器位（air）。
function placeStoneSupportForFacing(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：右键切到减法模式——放比较器（mode=compare）+ stick useItemOnBlock → mode=subtract，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 比较器 mode=compare。
// onBlockActivated：mayBuild=true → newMode=(compare==Compare)?Subtract=Subtract → withMode setBlockState
//   + 音效（0.55）+ updateState → return Success。
//
// 判定：useItemOnBlock 返 true（Success），mode === "subtract"（切到减法模式）。
function comparatorModeTogglesToSubtractOnUse(test: Test): void {
    placeComparator(test);
    test.assert(getComparatorMode(test, 3, 2, 1) === "compare", `comparator mode should be compare before, got ${getComparatorMode(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对比较器 useItemOnBlock stick → onBlockActivated newMode=Subtract → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when toggling comparator to subtract mode");

    // 判定：mode === "subtract"（切到减法模式）。
    test.assert(getComparatorMode(test, 3, 2, 1) === "subtract", `comparator mode should be subtract after toggle, got ${getComparatorMode(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：再次右键切回比较模式——已 mode=subtract + stick useItemOnBlock → mode=compare，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 比较器（先切换使 mode=subtract）。
// onBlockActivated：newMode=(subtract==Compare)?...:Compare=Compare → withMode setBlockState + 音效（0.5）
//   + updateState → return Success。
//
// 判定：useItemOnBlock 返 true（Success），mode === "compare"（切回比较模式）。
function comparatorModeTogglesBackToCompareOnUse(test: Test): void {
    placeComparator(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 第一次右键：切到减法模式（mode compare→subtract）。
    const firstUsed = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstUsed, "first toggle should succeed");
    test.assert(getComparatorMode(test, 3, 2, 1) === "subtract", `comparator mode should be subtract after first toggle, got ${getComparatorMode(test, 3, 2, 1)}`);

    // 第二次右键：切回比较模式（mode subtract→compare）。
    const secondUsed = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(secondUsed, "useItemOnBlock should return true when toggling comparator back to compare mode");

    // 判定：mode === "compare"（切回比较模式）。
    test.assert(getComparatorMode(test, 3, 2, 1) === "compare", `comparator mode should be compare after second toggle, got ${getComparatorMode(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 comparator useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement（基类 RedstoneDiodeBlock）
//   facing=opposite(horizontalDirection)（pitch≈0 时 [0]=水平朝向），保留 mode=compare 等默认 → setBlockState
//   放比较器 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「比较器 facing=opposite(玩家水平视线)」+ 基类 RedstoneDiodeBlock getStateForPlacement
//   修复生效：水平 4 朝向映射与 barrel/dispenser/piston/中继器一致。此前 Cubium 基类未重写
//   getStateForPlacement（基类 defaultState，HORIZONTAL_FACING 恒 North），4 朝向放置 facing 全为 north，
//   断言失败；重写后修正。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免比较器残留阻断放置。
function comparatorFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupportForFacing(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向比较器残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 comparator 点击 (3,1,1) stone 顶面 Up → 比较器落 (3,2,1)。
        // getStateForPlacement（基类）facing=opposite(horizontalDirection)，pitch≈0 时 [0]=水平朝向。
        const comparatorItem = new ItemStack("minecraft:comparator", 1);
        const used = player.useItemOnBlock(
            comparatorItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing comparator facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言比较器 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:comparator", `comparator should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getComparatorFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `comparator facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 4：水平视线决定朝向（区分新旧实现）——玩家水平朝东点击 stone 顶面 Up → facing=West（非 North）。
//
// 布局：(3,1,1) stone。玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使 pitch≈1.4°
//   近水平视线）。手持 comparator useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement facing=opposite(horizontalDirection)（[0]=East 水平朝东）→ facing=West。
//
// 判定：(3,2,1) facing === "west"（非 "north"）。
//
// 此场景是按水平视线决定朝向与旧基类 defaultState 行为分歧的边缘场景，验证修复生效：旧实现（基类
//   defaultState）facing=North（恒定，无视视线）；新实现 facing=opposite(horizontalDirection[0]=East)=West。
//   断言 facing=West 验证 getStateForPlacement override 而非基类 defaultState。此前此场景 facing=North，
//   断言 facing=West 失败；重写后修正。
function comparatorFacingUsesLookingDirectionNotDefaultState(test: Test): void {
    placeStoneSupportForFacing(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_east");
    // 朝东近水平视线：lookAt (6,3,1)，dx=5,dz=0→yaw=atan2(-5,0)=-90°→270°(East)，lookAt.y=3→dy=-0.12→pitch≈1.4°。
    player.lookAtLocation({ x: 6, y: 3, z: 1 });

    // 手持 comparator 点击 (3,1,1) 顶面 Up → 比较器落 (3,2,1)。
    // horizontalDirection[0]=East（水平朝东）→facing=opposite(East)=West（非基类 defaultState 的 North）。
    const comparatorItem = new ItemStack("minecraft:comparator", 1);
    const used = player.useItemOnBlock(
        comparatorItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing comparator");

    // 断言 facing=west（视线 East 的反方向，非基类 defaultState 的 North）。验证 getStateForPlacement override。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:comparator", `comparator should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    const facing = getComparatorFacing(test, 3, 2, 1);
    test.assert(facing === "west", `comparator facing should be west (opposite of player looking east), not north (default state), got ${facing}`);

    test.succeed();
}

export function registerComparatorTests(): void {
    GameTest.register("BlockBehaviorTests", "comparator_mode_toggles_to_subtract_on_use", comparatorModeTogglesToSubtractOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "comparator_mode_toggles_back_to_compare_on_use", comparatorModeTogglesBackToCompareOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "comparator_facing_opposite_player_looking", comparatorFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "comparator_facing_uses_looking_direction_not_default_state", comparatorFacingUsesLookingDirectionNotDefaultState)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
