// 发射器（dispenser）朝向放置、视线决定朝向、红石触发 TRIGGERED、state 读写与破坏行为 GameTest。
//
// wiki tech_发射器.txt#放置/用途/红石元件/数据值：
//   - 放置朝向（:44）：发射器的发射面可朝向任何方向，包括上方和下方。玩家放置发射器时，发射器会朝向
//     该玩家的方向（发射口朝玩家，facing=opposite(玩家视线最近方向)）。1.13 17w47a 明确记载
//     「发射器被放置时朝向玩家朝向的反方向」。
//   - 用途（:49）：发射器有 9 个物品槽位，对发射器按使用键可打开界面（涉 GUI，SimulatedPlayer 无 GUI
//     放物品 API，不可测交互）。
//   - 破坏（:35,39）：挖掘工具为镐，破坏后掉落自身 + 内容物（破坏掉落物非确定，项目范式不验证掉落物
//     实体，仅测变 air）。
//   - 红石元件（:73）：被激活后产生 1 个计划刻，延时 2 红石刻（4 游戏刻）后发射一个物品。上升沿触发，
//     不连续发射（:75）。随机选槽（:77）。发射行为涉 BlockEntity + 投射物实体，依赖 world.entityRegistry
//     与 tick 调度，GameTest setBlockType 不走放置上下文且红石 tick 调度时序不可靠，本组不测实际发射
//     物品实体（仅测 TRIGGERED 翻转）。
//   - 数据值/方块状态：FACING（6 向 Direction 含 Up/Down）+ TRIGGERED（bool）。
//   - JE/BE 差异：半连接性 JE only（:68）；发射弹射物/刷怪蛋音效差异（:191-192）。仅影响发射行为与音效，
//     不影响放置/红石触发 TRIGGERED/state 读写测试。
//
// C++ 链路：DispenserBlock（redstone/DispenserBlock.cpp）两个 state：
//   - FACING（C++ 属性名 "facing"，DirectionProperty 6 向含 Up/Down，Properties.hpp，默认 North，
//     构造函数 :77-80 setDefaultState facing=North,triggered=false）。
//   - TRIGGERED（C++ 属性名 "triggered"，BooleanProperty，默认 false）。
//   - getStateForPlacement（本提交新增）：facing=opposite(getNearestLookingDirection())。
//     getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]（无条件取视线最近方向，含俯仰）。
//     此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（FACING 恒 North），
//     与预期分歧。修复后重写修正。
//   - neighborChanged（:103-128）：红石触发链路。RedstonePower::isPowered 检测充能；若 shouldTrigger !=
//     isCurrentlyTriggered，被激活时 scheduleBlockTick(pos, *this, 4, TickPriority::High)（4 tick 延时，
//     TRIGGER_DURATION=4），并 withTriggered 写回 state（flags=2）。
//   - tick（:146-152）：调 dispense（涉 BlockEntity + 投射物，本组不测）。
//   - hasBlockEntity()=true（:95），createBlockEntity（:311-314）返 DispenserBlockEntity。
//   - 物品注册：BlockItemRegistry.cpp:1193 registerSimpleBlock(VanillaBlocks::DISPENSER, "dispenser")。
//     方块注册 RedstoneBlocks.cpp:333。物品与方块均已注册，useItemOnBlock 放置链路可用。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 dispenser 物品点击
//   stone → onBlockActivated（stone 非发射器，targetBlock 走基类 Pass）→ fallback Item.useOn →
//   BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext（playerYaw/playerPitch 来自 SimulatedPlayer
//   yaw()/pitch()）→ getStateForPlacement facing=opposite(getNearestLookingDirection) → setBlockState
//   放发射器（hasBlockEntity 触发 BlockEntity 创建）。创造模式不消耗物品。
//
// 朝向控制（复用 BarrelTests 建立的含 pitch lookAtLocation 范式，发射器与 barrel 同属含 pitch 类）：
//   getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]（BlockItemUseContext.cpp:51-88,190-208）。
//   pitch=0（水平）时 [0]=玩家水平朝向，facing=opposite(水平朝向)。
//   【关键】发射器用 getNearestLookingDirection（含 pitch），水平朝向测试须让 pitch≈0：
//   lookAt.y=playerPos.y+1（非=playerPos.y），玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高使 dy≈0→pitch≈0。
//   playerPos.y=2→眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°），水平距离≥5 放大 horizDist 使 pitch
//   微小，保证 orderedByNearest[0]=水平朝向（非 Down/Up）。lookAtLocation 设 yaw=atan2(-dx,dz) +
//   pitch=-atan2(dy,horizDist)（SimulatedPlayer.cpp:99-102）。
//
// 测试覆盖（5 个场景，覆盖 wiki 朝向放置 + 视线决定朝向 + 红石触发 TRIGGERED + state 读写 + 破坏核心
//   确定行为）：
//   1. 水平 4 朝向放置：pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 barrel 范式坐标）。
//   2. 水平视线点击顶面（区分新旧实现）：玩家近水平朝东（yaw=270，pitch≈1.4°）点击 stone 顶面 Up →
//      旧实现 facing=North（基类 defaultState 恒 North），新实现 facing=West（opposite(East)，视线 East）。
//      断言 facing=West 验证修复（getStateForPlacement override 而非基类 defaultState）。
//   3. 红石触发 TRIGGERED 翻转：放发射器（默认 triggered=false）相邻放红石块 → triggered 翻转为 true
//      （neighborChanged isPowered=true → withTriggered(true) 写回）。
//   4. facing/triggered state 读写：setBlockWithStates 预置 facing=down,triggered=false → getState 可读
//      （6 向 facing 含 Up/Down + triggered bool 双 state）。
//   5. 发射器破坏不崩溃：放发射器（BlockEntity 创建）→ setBlockType air 破坏 → 位置变 air
//      （onBlockRemoved 基类链路安全）。
//
// 关键约束：
// 1. 场景 1 水平 4 朝向复用 BarrelTests 的 4 朝向映射与坐标配方（发射器与 barrel 同属含 pitch 类，
//    lookAt.y=playerPos.y+1 使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）。每朝向独立 spawn 玩家
//    避免 yaw 残留；每次清理 (3,2,1) 避免发射器残留阻断放置。facing=opposite(水平朝向)。
// 2. 场景 2 区分新旧实现：玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使
//    pitch≈1.4° 近水平视线），useItemOnBlock 点击 (3,1,1) stone 顶面 Up。旧实现（基类 defaultState）
//    facing=North（恒定）；新实现 getNearestLookingDirection[0]=East（近水平朝东）→facing=opposite(East)=West。
//    断言 facing=West。此场景视线（East）与点击面（Up）不一致，是与旧 Cubium 分歧的边缘场景，
//    修复后修正。
// 3. 场景 3 红石触发 TRIGGERED：放发射器（默认 triggered=false）+ (4,2,1) 放红石块（水平相邻，全向
//    充能 15）。放红石块走 setBlockState flags=3 → 邻居发射器 neighborChanged → isPowered=true !=
//    isCurrentlyTriggered(false) → withTriggered(true) 写回 + scheduleBlockTick(4)。仅测 TRIGGERED 翻转
//    （不测 tick→dispense，因发射依赖 entityRegistry + tick 调度时序，GameTest 不可靠）。pollUntilSucceed
//    轮询 triggered===true。
// 4. 场景 4 setBlockWithStates 预置 facing=down,triggered=false（6 向 facing 含 Up/Down，验证非水平方向
//    state）→ getState 双 state 均可读。
// 5. 场景 5 放发射器后 setBlockType air 破坏：DispenserBlock 无 onBlockRemoved override（基类
//    Block::onBlockRemoved 空操作），DispenserBlockEntity 随方块移除清理。位置变 air。断言变 air（链路
//    不崩溃）。破坏掉落物非确定，仅测变 air。
// 6. 读 facing 用 getState("facing" as any)（DirectionProperty 6 向，返方向名 "north"/"south"/"east"/
//    "west"/"up"/"down"）。读 triggered 用 getState("triggered" as any)（BooleanProperty 返 bool）。
// 7. 发射器 fullBlock 碰撞箱，isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近真实放置；
//    玩家不能站在 placementPos（碰撞检查 BlockItem.cpp:262-274），场景 1 玩家位置均远离 (3,2,1)。
// 8. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「GUI 交互（9 槽放物品）」：onBlockActivated 未实现（基类 Pass），SimulatedPlayer 无 GUI 放物品 API，
//   不可测。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 dispenser_opens_gui。
// 不测「红石触发实际发射物品实体」：tick→dispense→tryDispense→spawnItemEntity 依赖 world.entityRegistry
//   与红石 tick 调度时序（scheduleBlockTick(4)），GameTest 红石 tick 调度时序不可靠且发射物实体非确定。
//   本组仅测 TRIGGERED 翻转（neighborChanged 同步触发，确定可测）。TODO: 待红石 tick 调度测试范式
//   完善后补 dispenser_dispenses_item_on_redstone。
// 不测「破坏掉落自身 + 内容物」：SimulatedPlayer 无 GUI 放物品 API，无法构造有内容物的发射器；破坏掉落物
//   非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补
//   dispenser_drops_contents_when_broken。
// 不测「比较器信号」：未实现 hasAnalogOutputSignal/getAnalogOutputSignal，无信号可测。
// 不测「半连接性」：JE only，BE 无，两端不一致不测。
//
// 跨服务端：dispenser 方块名两端一致。facing/triggered state 名两端一致（C++ 内部名 "facing"/"triggered"）。
//   朝向放置（facing=opposite(getNearestLookingDirection)）+ 红石触发 TRIGGERED + state 读写 + 破坏行为
//   两端一致。修复前 Cubium 用基类 defaultState（FACING 恒 North，与预期分歧），
//   修复后修正。lookAtLocation 是 Cubium 专有朝向控制，但 facing=opposite(视线) 放置行为两端可对比
//   （基岩用真实玩家视线放置），非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_发射器.txt#放置（发射面可朝任何方向，朝向玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_发射器.txt#红石元件（被激活产生计划刻，4tick 后发射）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_发射器.txt#数据值（FACING 6 向 + TRIGGERED bool）
// Ref: DispenserBlock.cpp（getStateForPlacement facing=opposite(getNearestLookingDirection) 修复 / neighborChanged 红石触发 TRIGGERED / hasBlockEntity）
// Ref: BlockItemUseContext.cpp（getNearestLookingDirection 单数方法补全 + orderedByNearest）
// Ref: BarrelBlock.cpp:85-94（同类含 pitch facing 修复模板，发射器照搬）
// Ref: RedstoneBlocks.cpp:333（DISPENSER 方块注册）
// Ref: BlockItemRegistry.cpp:1193（dispenser 物品注册）
// Ref: BarrelTests.ts（含 pitch lookAtLocation 朝向控制范式 + 水平 4 朝向坐标配方，发射器复用）
// Ref: CopperBulbTests.ts（红石块电源触发邻居 state 翻转范式，TRIGGERED 测试复用）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/5：发射器 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 3：发射器 (3,2,1)，红石块电源 (4,2,1) 水平相邻。
// 场景 4：发射器 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BarrelTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 发射器 facing state（方向名字符串 "north"/"south"/"east"/"west"/"up"/"down"）。
// 返回 null 表示失败或非发射器。FACING() 的 C++ 属性名为 "facing"（DirectionProperty 6 向）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 发射器 triggered state（bool）。返回 null 表示失败或非发射器。
// TRIGGERED() 的 C++ 属性名为 "triggered"（BooleanProperty::create("triggered")）。
function getTriggered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("triggered" as any);
    return typeof value === "boolean" ? value : null;
}

// 水平 4 朝向放置映射表（pitch≈0，复用 BarrelTests 的 4 朝向映射与坐标配方，发射器与 barrel 同属含 pitch 类）。
// getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch≈0 时 [0]=玩家水平朝向，facing=opposite(水平朝向)。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East（同 barrel）。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0。playerPos.y=2→眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5
//   放大 horizDist 使 pitch 微小，保证 orderedByNearest[0]=水平朝向（非 Down/Up）。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 发射器 facing=opposite(玩家水平朝向)
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

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放发射器位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 dispenser useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement facing=opposite(
//   getNearestLookingDirection)（pitch≈0 时 [0]=水平朝向）→ setBlockState 放发射器 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「发射器朝向玩家」+ getStateForPlacement facing=opposite(getNearestLookingDirection)：
//   水平 4 朝向映射与 barrel 一致（pitch≈0 时 getNearestLookingDirection[0]=水平朝向）。修复前 Cubium
//   未重写 getStateForPlacement（基类 defaultState，FACING 恒 North），4 朝向放置 facing 全为 north，
//   断言失败；修复后修正。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免发射器残留阻断放置。
function dispenserFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向发射器残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 dispenser 点击 (3,1,1) stone 顶面 Up → 发射器落 (3,2,1)。
        // getStateForPlacement facing=opposite(getNearestLookingDirection)，pitch≈0 时 [0]=水平朝向。
        const dispenserItem = new ItemStack("minecraft:dispenser", 1);
        const used = player.useItemOnBlock(
            dispenserItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing dispenser facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言发射器 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dispenser", `dispenser should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `dispenser facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：水平视线点击顶面（区分新旧实现）——玩家水平朝东点击 stone 顶面 Up → facing=West（非 North）。
//
// 布局：(3,1,1) stone。玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使 pitch≈1.4°
//   近水平视线）。手持 dispenser useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement facing=opposite(getNearestLookingDirection)（[0]=East 水平朝东）→ facing=West。
//
// 判定：(3,2,1) facing === "west"（非 "north"）。
//
// 此场景是与旧 Cubium 分歧的边缘场景，验证修复生效：玩家视线（East，水平）与点击面（Up，
//   顶面）不一致。旧实现（基类 defaultState）facing=North（恒定，无视视线）；新实现
//   facing=opposite(getNearestLookingDirection[0]=East)=West。断言 facing=West 验证 getStateForPlacement
//   override 而非基类 defaultState。修复前此场景 facing=North，断言 facing=West 失败；修复后修正。
function dispenserFacingUsesLookingDirectionNotDefaultState(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_east");
    // 朝东近水平视线：lookAt (6,3,1)，dx=5,dz=0→yaw=atan2(-5,0)=-90°→270°(East)，lookAt.y=3→dy=-0.12→pitch≈1.4°。
    player.lookAtLocation({ x: 6, y: 3, z: 1 });

    // 手持 dispenser 点击 (3,1,1) 顶面 Up → 发射器落 (3,2,1)。
    // getNearestLookingDirection[0]=East（水平朝东）→facing=opposite(East)=West（非基类 defaultState 的 North）。
    const dispenserItem = new ItemStack("minecraft:dispenser", 1);
    const used = player.useItemOnBlock(
        dispenserItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing dispenser");

    // 断言 facing=west（视线 East 的反方向，非基类 defaultState 的 North）。验证 getStateForPlacement override。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dispenser", `dispenser should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    const facing = getFacing(test, 3, 2, 1);
    test.assert(facing === "west", `dispenser facing should be west (opposite of player looking east), not north (default state), got ${facing}`);

    test.succeed();
}

// 场景 3：红石触发 TRIGGERED 翻转——放发射器（默认 triggered=false）相邻放红石块 → triggered 翻转为 true。
//
// 布局：(3,2,1) 放发射器（默认 triggered=false，未充能），(4,2,1) 放红石块（水平相邻，全向充能 15）。
// 放红石块走 setBlockState flags=3 → 邻居发射器 neighborChanged → isPowered(红石块 weakPower 15)=true !=
// isCurrentlyTriggered(false) → withTriggered(true) 写回 + scheduleBlockTick(4)。
//
// 判定：pollUntilSucceed 轮询 triggered===true（neighborChanged 同步触发 TRIGGERED 翻转，留余量防时序）。
//
// 此场景验证 wiki「发射器被激活产生计划刻」红石触发链路的前置（TRIGGERED 翻转）。不测 tick→dispense
//   实际发射（依赖 entityRegistry + tick 调度时序，GameTest 不可靠，见文件头 TODO）。
function dispenserTriggeredTogglesWhenPowered(test: Test): void {
    // (3,2,1) 放发射器（默认 triggered=false，未充能）。
    test.setBlockType("minecraft:dispenser", { x: 3, y: 2, z: 1 });

    // (4,2,1) 放红石块（水平相邻发射器，getWeakPower 全向 15）。放红石块 flags=3 → 邻居发射器
    // neighborChanged → isPowered=true != isCurrentlyTriggered=false → withTriggered(true) 写回。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 triggered === true（neighborChanged 同步翻转 TRIGGERED，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getTriggered(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `dispenser triggered: should be true when powered, got ${getTriggered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 4：facing（6 向含 Up/Down）/triggered state 读写——预置 facing=down,triggered=false → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 发射器（setBlockWithStates 预置 facing=down,triggered=false，绕过物品
//   放置直接写 state）。
//
// 判定：getState("facing")==="down" 且 getState("triggered")===false（验证 6 向 facing（含 Up/Down）+ triggered
//   双 state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「发射器 FACING（6 向含 Up/Down）/ TRIGGERED 双 state」可读写：setBlockWithStates 预置
//   facing=down（6 向中非水平方向，验证 6 向 state 类型）,triggered=false 后 getState 双 state 均可读。
function dispenserStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=down,triggered=false（6 向 facing 含 Up/Down，验证非水平方向 state）。
    (test as TestWithStates).setBlockWithStates("minecraft:dispenser", { x: 3, y: 2, z: 1 }, "facing=down,triggered=false");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dispenser", `dispenser should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===down 且 triggered===false（6 向 facing + triggered 双 state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "down", `facing should be down after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);
    test.assert(getTriggered(test, 3, 2, 1) === false, `triggered should be false after setBlockWithStates, got ${getTriggered(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 5：发射器破坏不崩溃——放发射器（BlockEntity 创建）→ setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 发射器（setBlockType 放置，BlockEntity 创建）。
// setBlockType("minecraft:air", (3,2,1)) 破坏发射器 → DispenserBlock 无 onBlockRemoved override（基类
//   Block::onBlockRemoved 空操作），DispenserBlockEntity 随方块移除清理 → 位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（发射器已破坏，BlockEntity 链路不崩溃）。
//
// 此场景验证发射器容器破坏链路安全性：放发射器（BlockEntity 创建）后破坏，基类 onBlockRemoved 不崩溃，
//   位置正确变 air。空发射器无内容物可掉落（SimulatedPlayer 无 GUI 放物品 API，无法构造有内容物的发射器，
//   故仅测空破坏不崩溃，见文件头 TODO）。
function dispenserBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType("minecraft:dispenser", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dispenser", `dispenser should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏发射器 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言发射器 (3,2,1) 已破坏变 air（BlockEntity 链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `dispenser pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerDispenserTests(): void {
    GameTest.register("BlockBehaviorTests", "dispenser_facing_opposite_player_looking", dispenserFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "dispenser_facing_uses_looking_direction_not_default_state", dispenserFacingUsesLookingDirectionNotDefaultState)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "dispenser_triggered_toggles_when_powered", dispenserTriggeredTogglesWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "dispenser_state_readable", dispenserStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "dispenser_breaks_when_removed", dispenserBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
