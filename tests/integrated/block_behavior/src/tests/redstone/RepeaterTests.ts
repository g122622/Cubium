// 红石中继器右键切换延迟档位 + 放置朝向行为 GameTest。
//
// wiki tech_红石中继器.txt#延迟信号：红石中继器共有 4 个档位，延迟分别为 2/4/6/8 游戏刻，默认 1 档
//   （2gt）。可对中继器按下使用键改变档位：每次点击 +1 档（延迟 +2gt），最大 4 档（8gt），再点击
//   重置为 1 档。循环顺序 1→2→3→4→1。
// wiki tech_红石中继器.txt#放置：中继器水平放置，facing = opposite(玩家水平视线方向)（输入端朝玩家、
//   输出端背离玩家，水平四向 South/West/North/East）。朝向仅由玩家 yaw 决定，不含 pitch。
//
// C++ 链路：RedstoneRepeaterBlock（redstone/RedstoneRepeaterBlock.cpp）继承 RedstoneDiodeBlock，state：
//   HORIZONTAL_FACING（"facing"，默认 North）+ POWERED + DELAY_1_4（"delay"，默认 1）+ LOCKED。
//   - onBlockActivated（:144）：!mayBuild → Pass；isLockedState → Pass（锁定时不调档）；否则
//     newDelay = (currentDelay % MAX_DELAY) + 1（1→2→3→4→1）→ withDelay setBlockState 写回 +
//     点击音效 → return Success。不检查手持物（空手/任意物品右键都切档）。
//   - DELAY state 名 "delay"（Properties.hpp DELAY_1_4 = IntegerProperty("delay",1,4)）。
//   - getStateForPlacement（基类 RedstoneDiodeBlock 重写）：facing = opposite(
//     context.horizontalDirection())（水平四向）。中继器继承本方法自动获得正确朝向。此前基类未重写该
//     方法，落回 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒 North），与预期
//     按水平视线决定朝向的行为不一致。
//   - 锁定（LOCKED=true）时右键返 Pass 不切档——本组不构造锁定场景（需侧面二极管信号，复杂）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。中继器 onBlockActivated 创造模式 mayBuild=true + 未锁定 → 切档返 Success 短路。
//   用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，仅占位 useItemOnBlock 的
//   ItemStack 形参）。放置朝向走 useItemOnBlock 手持 repeater 物品点击 stone 顶面 Up → BlockItem::tryPlace
//   → getStateForPlacement（基类 facing=opposite(horizontalDirection)）→ setBlockState。
//
// 测试覆盖（4 个场景，覆盖 wiki 右键切档 + 循环重置 + 放置朝向核心确定行为）：
//   1. 右键切档递增：放中继器（delay=1）+ stick useItemOnBlock → delay=2，返 true。
//   2. 循环重置到 1 档：连续 4 次 useItemOnBlock（delay 1→2→3→4→1）→ 第 4 次后 delay=1（4 档重置）。
//   3. 水平 4 朝向放置：pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 barrel 4 朝向坐标配方，水平类仅 yaw）。
//   4. 水平视线决定朝向（区分新旧实现）：玩家近水平朝东点击 stone 顶面 Up → facing=West（非 North），
//      断言 facing=West 验证修复（getStateForPlacement override 而非基类 defaultState）。
//
// 关键约束：
// 1. 中继器 noCollision().notSolid()，Diode 无 canSurvive 自毁逻辑（onBlockAdded 仅通知邻居，
//    updatePostPlacement 仅调度 powered 更新），悬空放置不自毁。但仍放 (3,1,1) stone 支撑 + (3,2,1)
//    中继器，贴近真实放置且与同目录红石测试惯例一致。
// 2. setBlockType 放中继器带默认 state（facing=North, powered=false, delay=1, locked=false）。
//    右键切档不依赖 facing，默认 North 即可。朝向测试用 useItemOnBlock 放置（走 getStateForPlacement）。
// 3. 读 delay state 用 getState("delay" as any) 绕过 BlockStateSuperset 白名单。读 facing 用
//    getState("facing" as any)（HORIZONTAL_FACING，返小写方向字符串 north/south/east/west）。
// 4. 创造模式 SimulatedPlayer mayBuild=true，onBlockActivated 走切档分支（非 Pass）。locked=false（
//    无侧面信号），不触发 isLockedState 守卫。
// 5. 切档不消耗手持物（onBlockActivated 无 shrink），stick 可重复使用；循环场景每次仍 new ItemStack
//    重新设入选中槽（与重生锚充能范式一致，避免选中槽状态漂移）。
// 6. 切档后 setBlockState(flags=3) 触发邻居更新 + 计划刻，但无信号输入 shouldPower=false=isPowered，
//    不调度亮灭，delay state 稳定可断言。
// 7. 场景 3/4 朝向控制（复用 BarrelTests 含 pitch lookAtLocation 范式，中继器为水平类 facing=
//    opposite(horizontalDirection)，horizontalDirection 仅 yaw 决定，但 lookAt.y=playerPos.y+1 使
//    pitch≈0 与现有范式一致）。每朝向独立 spawn 玩家避免 yaw 残留；每次清理 (3,2,1) 避免中继器残留
//    阻断放置。facing=opposite(玩家水平朝向)。此前 Cubium 基类未重写 getStateForPlacement（基类
//    defaultState，HORIZONTAL_FACING 恒 North），4 朝向放置 facing 全为 north，断言失败；重写后修正。
// 8. 中继器 fullBlock 碰撞箱（Diode getShape 是低矮碰撞箱），isValidPosition 基类 true 无支撑要求。
//    stone 支撑仅为贴近真实放置；玩家不能站在 placementPos（碰撞检查），场景 3 玩家位置均远离 (3,2,1)。
//
// 不测「锁定时右键不切档」：构造锁定需侧面二极管信号链路，复杂且涉红石传导，跳过。TODO: 待锁定
//   链路测试完善后补。
// 不测「延迟时序（2/4/6/8gt）」：涉计划刻 + 红石传导链路，非本组聚焦（右键切档 state 变化），跳过。
//
// 跨服务端：中继器 repeater 方块名两端一致，delay/facing state 名两端一致，右键切档 1→2→3→4→1 循环
//   + 放置朝向（facing=opposite(horizontalDirection)）行为两端一致。基岩无 setBlockWithStates，切档
//   测试用 setBlockType 放默认 state（delay=1）两端均可放；朝向测试用 useItemOnBlock 放置
//   （lookAtLocation 是 Cubium 专有朝向控制，但 facing=opposite(水平视线) 放置行为两端可对比，非 one-sided）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石中继器.txt#延迟信号（4 档 2/4/6/8gt，右键 +1，4 档重置 1 档）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石中继器.txt#放置（facing=opposite(水平视线)）
// Ref: RedstoneRepeaterBlock.cpp（onBlockActivated newDelay=(cur%4)+1 切档+音效→Success；DELAY_1_4 state）
// Ref: RedstoneDiodeBlock.cpp（getStateForPlacement facing=opposite(horizontalDirection) 修复，中继器继承）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支）
// Ref: BarrelTests.ts（含 pitch lookAtLocation 朝向控制范式 + 水平 4 朝向坐标配方，中继器复用水平部分）
// Ref: DispenserTests.ts / PistonTests.ts（同类缺 getStateForPlacement override 坑模式 + 朝向测试范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 中继器 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取中继器 delay state（number 1-4）。返回 null 表示读取失败或非中继器。
// delay state 名 "delay"（Java 命名，见 Properties.hpp DELAY_1_4 = IntegerProperty("delay",1,4)）。
function getRepeaterDelay(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("delay" as any);
    return typeof value === "number" ? value : null;
}

// 读取 (x,y,z) 中继器 facing state（方向名字符串 "north"/"south"/"east"/"west"）。
// 返回 null 表示失败或非中继器。HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（水平四向，无 up/down）。
function getRepeaterFacing(test: Test, x: number, y: number, z: number): string | null {
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

// 水平 4 朝向放置映射表（复用 BarrelTests/DispenserTests/PistonTests 的 4 朝向映射与坐标配方，中继器
// 为水平类 facing=opposite(horizontalDirection)，horizontalDirection 仅 yaw 决定）。
// horizontalDirection=orderedByNearest(yaw,pitch)[0] 的水平分量，pitch≈0 时 = 玩家水平朝向，
// facing=opposite(水平朝向)。lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0（虽中继器 facing 仅由 yaw 决定，但 pitch≈0 与现有范式一致且稳妥）。playerPos.y=2
//   →眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5 放大 horizDist 使 pitch 微小。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 中继器 facing=opposite(玩家水平朝向)
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

// 放支撑 + 中继器：(3,1,1) stone 支撑，(3,2,1) 中继器（minecraft:repeater 默认 delay=1, facing=North）。
function placeRepeater(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:repeater", { x: 3, y: 2, z: 1 }); // 中继器 delay=1
}

// 场景 1：右键切档递增——放中继器（delay=1）+ stick useItemOnBlock → delay=2，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 中继器 delay=1。
// onBlockActivated：mayBuild=true + locked=false → newDelay=(1%4)+1=2 → withDelay(2) setBlockState → Success。
//
// 判定：useItemOnBlock 返 true（Success），delay === 2（切到 2 档）。
function repeaterDelayCyclesUpOnUse(test: Test): void {
    placeRepeater(test);
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 1, `repeater delay should be 1 before, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对中继器 useItemOnBlock stick → onBlockActivated newDelay=(1%4)+1=2 → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when cycling repeater delay up");

    // 判定：delay === 2（切到 2 档）。
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 2, `repeater delay should be 2 after one click, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：循环重置到 1 档——连续 4 次 useItemOnBlock（delay 1→2→3→4→1）→ 第 4 次后 delay=1。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 中继器 delay=1（已放）。
// 每次右键 newDelay=(cur%4)+1：1→2→3→4→1。第 4 次从 4 档重置回 1 档（4 档后循环回起点）。
//
// 判定：4 次点击后 delay === 1（4 档循环重置回 1 档）。
function repeaterDelayResetsAfterMaxOnUse(test: Test): void {
    placeRepeater(test);
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 1, `repeater delay should be 1 before, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // 连续 4 次右键切档：delay 1→2→3→4→1。每次 new ItemStack 重新设入选中槽（防选中槽漂移）。
    const expectedSequence = [2, 3, 4, 1]; // 1→2→3→4→1
    for (let i = 0; i < 4; ++i) {
        const stick = new ItemStack("minecraft:stick", 1);
        const used = farmer.useItemOnBlock(
            stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on click #${i + 1} (delay cycle)`);
        const expected = expectedSequence[i];
        test.assert(getRepeaterDelay(test, 3, 2, 1) === expected, `repeater delay should be ${expected} after click #${i + 1}, got ${getRepeaterDelay(test, 3, 2, 1)}`);
    }

    // 判定：delay === 1（4 档循环重置回 1 档）。
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 1, `repeater delay should reset to 1 after 4 clicks, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    test.succeed();
}

// 放置朝向测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放中继器位（air）。
function placeStoneSupportForFacing(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 3：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 repeater useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement（基类 RedstoneDiodeBlock）
//   facing=opposite(horizontalDirection)（pitch≈0 时 [0]=水平朝向），保留 delay=1 等默认 → setBlockState
//   放中继器 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「中继器 facing=opposite(玩家水平视线)」+ 基类 RedstoneDiodeBlock getStateForPlacement
//   修复生效：水平 4 朝向映射与 barrel/dispenser/piston 一致。此前 Cubium 基类未重写
//   getStateForPlacement（基类 defaultState，HORIZONTAL_FACING 恒 North），4 朝向放置 facing 全为 north，
//   断言失败；重写后修正。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免中继器残留阻断放置。
function repeaterFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupportForFacing(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向中继器残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 repeater 点击 (3,1,1) stone 顶面 Up → 中继器落 (3,2,1)。
        // getStateForPlacement（基类）facing=opposite(horizontalDirection)，pitch≈0 时 [0]=水平朝向。
        const repeaterItem = new ItemStack("minecraft:repeater", 1);
        const used = player.useItemOnBlock(
            repeaterItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing repeater facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言中继器 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:repeater", `repeater should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getRepeaterFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `repeater facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 4：水平视线决定朝向（区分新旧实现）——玩家水平朝东点击 stone 顶面 Up → facing=West（非 North）。
//
// 布局：(3,1,1) stone。玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使 pitch≈1.4°
//   近水平视线）。手持 repeater useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement facing=opposite(horizontalDirection)（[0]=East 水平朝东）→ facing=West。
//
// 判定：(3,2,1) facing === "west"（非 "north"）。
//
// 此场景是按水平视线决定朝向与旧基类 defaultState 行为分歧的边缘场景，验证修复生效：旧实现（基类
//   defaultState）facing=North（恒定，无视视线）；新实现 facing=opposite(horizontalDirection[0]=East)=West。
//   断言 facing=West 验证 getStateForPlacement override 而非基类 defaultState。此前此场景 facing=North，
//   断言 facing=West 失败；重写后修正。
function repeaterFacingUsesLookingDirectionNotDefaultState(test: Test): void {
    placeStoneSupportForFacing(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_east");
    // 朝东近水平视线：lookAt (6,3,1)，dx=5,dz=0→yaw=atan2(-5,0)=-90°→270°(East)，lookAt.y=3→dy=-0.12→pitch≈1.4°。
    player.lookAtLocation({ x: 6, y: 3, z: 1 });

    // 手持 repeater 点击 (3,1,1) 顶面 Up → 中继器落 (3,2,1)。
    // horizontalDirection[0]=East（水平朝东）→facing=opposite(East)=West（非基类 defaultState 的 North）。
    const repeaterItem = new ItemStack("minecraft:repeater", 1);
    const used = player.useItemOnBlock(
        repeaterItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing repeater");

    // 断言 facing=west（视线 East 的反方向，非基类 defaultState 的 North）。验证 getStateForPlacement override。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:repeater", `repeater should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    const facing = getRepeaterFacing(test, 3, 2, 1);
    test.assert(facing === "west", `repeater facing should be west (opposite of player looking east), not north (default state), got ${facing}`);

    test.succeed();
}

export function registerRepeaterTests(): void {
    GameTest.register("BlockBehaviorTests", "repeater_delay_cycles_up_on_use", repeaterDelayCyclesUpOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "repeater_delay_resets_after_max_on_use", repeaterDelayResetsAfterMaxOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "repeater_facing_opposite_player_looking", repeaterFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "repeater_facing_uses_looking_direction_not_default_state", repeaterFacingUsesLookingDirectionNotDefaultState)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
