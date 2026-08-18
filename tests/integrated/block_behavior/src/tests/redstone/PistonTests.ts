// 活塞（piston）朝向放置、视线决定朝向、state 读写与破坏行为 GameTest。
//
// wiki tech_活塞.txt#放置/红石/破坏/数据值：
//   - 放置朝向（:94）：活塞在被放置时永远朝向玩家（活塞头朝玩家，facing=opposite(玩家视线最近方向)，
//     六向含 Up/Down）。与发射器放置朝向一致（同为 getNearestLookingDirection().getOpposite()）。
//   - 红石触发（:96,114-124）：被激活后伸出，推动前方方块最多 12 格；取消激活收回活塞头。激活方式：除活塞
//     朝向外 5 方向的电源/充能方块/指向活塞的红石器件。实际推动方块依赖 PistonStructureHelper +
//     MovingPistonBlock + PistonBlockEntity tick 调度（progress 0→1 两 tick 落地），GameTest 红石 tick 调度
//     时序不可靠，本组不测实际推动（仅测放置/state/破坏确定行为）。
//   - 破坏（:88）：活塞破坏后掉落自身；前方活塞头立即被破坏。破坏掉落物非确定（项目范式不验证掉落物
//     实体，仅测变 air）。
//   - 数据值/方块状态：FACING（6 向 Direction 含 Up/Down）+ EXTENDED（bool）。
//   - JE/BE 差异：半连接性 JE only（:120）；启动延迟 JE 动态（0/1 tick）/ BE 固定 2 tick（:134-138）。
//     仅影响推动行为与启动时序，不影响放置/state 读写/破坏测试。
//   - 破坏工具（:84）：镐。
//
// C++ 链路：PistonBlock（redstone/PistonBlock.cpp）两个 state：
//   - FACING（C++ 属性名 "facing"，DirectionProperty 6 向含 Up/Down，Properties.hpp，默认 North，
//     构造函数 :75-77 setDefaultState facing=North,extended=false）。
//   - EXTENDED（C++ 属性名 "extended"，BooleanProperty，默认 false）。
//   - getStateForPlacement（本提交新增）：facing=opposite(getNearestLookingDirection())，
//     extended=false。getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]（无条件取视线最近方向，含俯仰）。
//     此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（FACING 恒 North），
//     与预期分歧。修复后重写修正。
//   - neighborChanged（:101-113）：红石触发链路，调 _checkForMove（shouldBeExtended 检测充能 → extend/retract）。
//   - extend（:269-289）/retract（:291-327）：推动逻辑，调 _doMove 创建 MovingPiston + PistonBlockEntity。
//     extend 会把活塞位置改成 moving_piston（:414）再 setBlockState extended=true（:286），依赖 BlockEntity
//     tick 调度落地，GameTest 不可靠（不测）。
//   - 物品注册：BlockItemRegistry.cpp:1191 registerSimpleBlock(VanillaBlocks::PISTON, "piston")。
//     方块注册 RedstoneBlocks.cpp:310。物品与方块均已注册，useItemOnBlock 放置链路可用。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 piston 物品点击 stone →
//   onBlockActivated（stone 非活塞，targetBlock 走基类 Pass）→ fallback Item.useOn → BlockItem::onItemUse
//   → tryPlace → 构造 BlockItemUseContext → getStateForPlacement facing=opposite(getNearestLookingDirection),
//   extended=false → setBlockState 放活塞。创造模式不消耗物品。
//
// 朝向控制（复用 BarrelTests/DispenserTests 的含 pitch lookAtLocation 范式，活塞与 barrel/dispenser 同属
//   含 pitch 类）：
//   getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch=0 时 [0]=玩家水平朝向，
//   facing=opposite(水平朝向)。lookAt.y=playerPos.y+1 使 pitch≈0（眼高≈playerPos.y+1.62，lookAt.y+0.5
//   接近眼高使 dy≈0）。playerPos.y=2→lookAt.y=3（dy=-0.12，pitch≈1.3°），水平距离≥5 放大后 [0]=水平朝向。
//
// 测试覆盖（4 个场景，覆盖 wiki 朝向放置 + 视线决定朝向 + state 读写 + 破坏核心确定行为）：
//   1. 水平 4 朝向放置：pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 barrel/dispenser 范式坐标）。
//   2. 水平视线点击顶面（区分新旧实现）：玩家近水平朝东（yaw=270，pitch≈1.4°）点击 stone 顶面 Up →
//      旧实现 facing=North（基类 defaultState 恒 North），新实现 facing=West（opposite(East)，视线 East）。
//      断言 facing=West 验证修复（getStateForPlacement override 而非基类 defaultState）。
//   3. facing/extended state 读写：setBlockWithStates 预置 facing=down,extended=false → getState 可读
//      （6 向 facing 含 Up/Down + extended bool 双 state）。
//   4. 活塞破坏不崩溃：放活塞 → setBlockType air 破坏 → 位置变 air（基类 onBlockRemoved 链路安全）。
//
// 关键约束：
// 1. 场景 1 水平 4 朝向复用 BarrelTests/DispenserTests 的 4 朝向映射与坐标配方（活塞与 barrel/dispenser
//    同属含 pitch 类，lookAt.y=playerPos.y+1 使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）。每朝向独立
//    spawn 玩家避免 yaw 残留；每次清理 (3,2,1) 避免活塞残留阻断放置。facing=opposite(水平朝向)。修复前
//    Cubium 未重写 getStateForPlacement（基类 defaultState，FACING 恒 North），4 朝向放置 facing 全为 north，
//    断言失败；修复后修正。
// 2. 场景 2 区分新旧实现：玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使
//    pitch≈1.4° 近水平视线），useItemOnBlock 点击 (3,1,1) stone 顶面 Up。旧实现（基类 defaultState）
//    facing=North（恒定）；新实现 getNearestLookingDirection[0]=East（近水平朝东）→facing=opposite(East)=West。
//    断言 facing=West。此场景视线（East）与点击面（Up）不一致，是与旧 Cubium 分歧的边缘场景，
//    修复后修正。
// 3. 场景 3 setBlockWithStates 预置 facing=down,extended=false（6 向 facing 含 Up/Down，验证非水平方向
//    state）→ getState 双 state 均可读。
// 4. 场景 4 放活塞后 setBlockType air 破坏：PistonBlock 无 onBlockRemoved override（基类
//    Block::onBlockRemoved 空操作），位置变 air。断言变 air（链路不崩溃）。破坏掉落物非确定，仅测变 air。
// 5. 读 facing 用 getState("facing" as any)（DirectionProperty 6 向，返方向名 "north"/"south"/"east"/
//    "west"/"up"/"down"）。读 extended 用 getState("extended" as any)（BooleanProperty 返 bool）。
// 6. 活塞 fullBlock 碰撞箱，isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近真实放置；
//    玩家不能站在 placementPos（碰撞检查 BlockItem.cpp:262-274），场景 1 玩家位置均远离 (3,2,1)。
// 7. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「红石触发实际推动方块」：extend→_doMove→MovingPiston+PistonBlockEntity tick 调度落地（progress
//   0→1 两 tick），依赖 BlockEntity tick 调度时序，GameTest 不可靠。且 extend 会把活塞位置改成 moving_piston
//   再 setBlockState extended=true，中间态非确定。TODO: 待红石 tick 调度测试范式完善后补
//   piston_extends_and_pushes_block_on_redstone。
// 不测「红石 EXTENDED 翻转」：extend 同步 setBlockState(pos, extended=true, 67) 理论可测，但 extend 前置
//   _doMove 会把 pos 改成 moving_piston（:414）+ 创建 PistonBlockEntity，GameTest 读 pos 可能读到
//   moving_piston 而非 piston(extended=true)，时序不可靠。TODO: 待验证 extend 后 pos 稳定为 piston(extended=true)
//   后补 piston_extended_toggles_when_powered。
// 不测「破坏掉落自身」：破坏掉落物非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧破坏掉落物测试
//   范式完善后补 piston_drops_itself。
// 不测「粘性活塞拉回方块」：依赖 retract→_doMove→BlockEntity 落地，时序不可靠。粘性活塞与普通活塞共用
//   PistonBlock 类（m_sticky 区分），朝向/state/破坏行为相同，粘性特性属不可测项。见 StickyPistonTests。
// 不测「半连接性」：JE only，BE 无，两端不一致不测。
//
// 跨服务端：piston 方块名两端一致。facing/extended state 名两端一致（C++ 内部名 "facing"/"extended"）。
//   朝向放置（facing=opposite(getNearestLookingDirection)）+ state 读写 + 破坏行为两端一致。
//   修复前 Cubium 用基类 defaultState（FACING 恒 North，与预期分歧），修复后修正。lookAtLocation
//   是 Cubium 专有朝向控制，但 facing=opposite(视线) 放置行为两端可对比（基岩用真实玩家视线放置），非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_活塞.txt#放置（永远朝向玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_活塞.txt#红石（被激活伸出推动方块）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_活塞.txt#数据值（FACING 6 向 + EXTENDED bool）
// Ref: PistonBlock.cpp（getStateForPlacement facing=opposite(getNearestLookingDirection) 修复 / neighborChanged 红石触发 / extend 推动逻辑）
// Ref: BlockItemUseContext.cpp（getNearestLookingDirection 单数方法补全 + orderedByNearest）
// Ref: BarrelBlock.cpp:85-94 / DispenserBlock.cpp（同类含 pitch facing 修复模板，活塞照搬）
// Ref: RedstoneBlocks.cpp:310（PISTON 方块注册）
// Ref: BlockItemRegistry.cpp:1191（piston 物品注册）
// Ref: BarrelTests.ts / DispenserTests.ts（含 pitch lookAtLocation 朝向控制范式 + 水平 4 朝向坐标配方，活塞复用）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/4：活塞 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 3：活塞 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BarrelTests/DispenserTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 活塞 facing state（方向名字符串 "north"/"south"/"east"/"west"/"up"/"down"）。
// 返回 null 表示失败或非活塞。FACING() 的 C++ 属性名为 "facing"（DirectionProperty 6 向）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 活塞 extended state（bool）。返回 null 表示失败或非活塞。
// EXTENDED() 的 C++ 属性名为 "extended"（BooleanProperty::create("extended")）。
function getExtended(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("extended" as any);
    return typeof value === "boolean" ? value : null;
}

// 水平 4 朝向放置映射表（pitch≈0，复用 BarrelTests/DispenserTests 的 4 朝向映射与坐标配方，活塞同属含 pitch 类）。
// getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch≈0 时 [0]=玩家水平朝向，facing=opposite(水平朝向)。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0。playerPos.y=2→眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5
//   放大 horizDist 使 pitch 微小，保证 orderedByNearest[0]=水平朝向（非 Down/Up）。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 活塞 facing=opposite(玩家水平朝向)
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

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放活塞位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 piston useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement facing=opposite(
//   getNearestLookingDirection)（pitch≈0 时 [0]=水平朝向）, extended=false → setBlockState 放活塞 (3,2,1)。
//   断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「活塞永远朝向玩家」+ getStateForPlacement facing=opposite(getNearestLookingDirection)：
//   水平 4 朝向映射与 barrel/dispenser 一致。修复前 Cubium 未重写 getStateForPlacement（基类 defaultState，
//   FACING 恒 North），4 朝向放置 facing 全为 north，断言失败；修复后修正。每朝向用新 player 避免
//   yaw 残留；每次清理 (3,2,1) 避免活塞残留阻断放置。
function pistonFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向活塞残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 piston 点击 (3,1,1) stone 顶面 Up → 活塞落 (3,2,1)。
        // getStateForPlacement facing=opposite(getNearestLookingDirection)，pitch≈0 时 [0]=水平朝向。
        const pistonItem = new ItemStack("minecraft:piston", 1);
        const used = player.useItemOnBlock(
            pistonItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing piston facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言活塞 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:piston", `piston should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `piston facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：水平视线点击顶面（区分新旧实现）——玩家水平朝东点击 stone 顶面 Up → facing=West（非 North）。
//
// 布局：(3,1,1) stone。玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使 pitch≈1.4°
//   近水平视线）。手持 piston useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement facing=opposite(getNearestLookingDirection)（[0]=East 水平朝东）→ facing=West。
//
// 判定：(3,2,1) facing === "west"（非 "north"）。
//
// 此场景是与旧 Cubium 分歧的边缘场景，验证修复生效：玩家视线（East，水平）与点击面（Up，
//   顶面）不一致。旧实现（基类 defaultState）facing=North（恒定，无视视线）；新实现
//   facing=opposite(getNearestLookingDirection[0]=East)=West。断言 facing=West 验证 getStateForPlacement
//   override 而非基类 defaultState。修复前此场景 facing=North，断言 facing=West 失败；修复后修正。
function pistonFacingUsesLookingDirectionNotDefaultState(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_east");
    // 朝东近水平视线：lookAt (6,3,1)，dx=5,dz=0→yaw=atan2(-5,0)=-90°→270°(East)，lookAt.y=3→dy=-0.12→pitch≈1.4°。
    player.lookAtLocation({ x: 6, y: 3, z: 1 });

    // 手持 piston 点击 (3,1,1) 顶面 Up → 活塞落 (3,2,1)。
    // getNearestLookingDirection[0]=East（水平朝东）→facing=opposite(East)=West（非基类 defaultState 的 North）。
    const pistonItem = new ItemStack("minecraft:piston", 1);
    const used = player.useItemOnBlock(
        pistonItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing piston");

    // 断言 facing=west（视线 East 的反方向，非基类 defaultState 的 North）。验证 getStateForPlacement override。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:piston", `piston should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    const facing = getFacing(test, 3, 2, 1);
    test.assert(facing === "west", `piston facing should be west (opposite of player looking east), not north (default state), got ${facing}`);

    test.succeed();
}

// 场景 3：facing（6 向含 Up/Down）/extended state 读写——预置 facing=down,extended=false → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 活塞（setBlockWithStates 预置 facing=down,extended=false，绕过物品
//   放置直接写 state）。
//
// 判定：getState("facing")==="down" 且 getState("extended")===false（验证 6 向 facing（含 Up/Down）+ extended
//   双 state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「活塞 FACING（6 向含 Up/Down）/ EXTENDED 双 state」可读写：setBlockWithStates 预置
//   facing=down（6 向中非水平方向，验证 6 向 state 类型）,extended=false 后 getState 双 state 均可读。
function pistonStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=down,extended=false（6 向 facing 含 Up/Down，验证非水平方向 state）。
    (test as TestWithStates).setBlockWithStates("minecraft:piston", { x: 3, y: 2, z: 1 }, "facing=down,extended=false");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:piston", `piston should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===down 且 extended===false（6 向 facing + extended 双 state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "down", `facing should be down after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);
    test.assert(getExtended(test, 3, 2, 1) === false, `extended should be false after setBlockWithStates, got ${getExtended(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：活塞破坏不崩溃——放活塞 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 活塞（setBlockType 放置，extended=false 未伸出无活塞头）。
// setBlockType("minecraft:air", (3,2,1)) 破坏活塞 → PistonBlock 无 onBlockRemoved override（基类
//   Block::onBlockRemoved 空操作），位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（活塞已破坏，链路不崩溃）。
//
// 此场景验证活塞破坏链路安全性：放活塞（extended=false 未伸出）后破坏，基类 onBlockRemoved 不崩溃，
//   位置正确变 air。破坏掉落物（wiki :88 破坏掉落自身）非确定（项目范式不验证掉落物实体），故仅测变 air。
function pistonBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType("minecraft:piston", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:piston", `piston should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏活塞 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言活塞 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `piston pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerPistonTests(): void {
    GameTest.register("BlockBehaviorTests", "piston_facing_opposite_player_looking", pistonFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "piston_facing_uses_looking_direction_not_default_state", pistonFacingUsesLookingDirectionNotDefaultState)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "piston_state_readable", pistonStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "piston_breaks_when_removed", pistonBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
