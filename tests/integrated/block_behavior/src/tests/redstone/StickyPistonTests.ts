// 粘性活塞（sticky_piston）朝向放置（与活塞同类共享）、state 读写与破坏行为 GameTest。
//
// wiki tech_活塞.txt#放置/红石/破坏/数据值（粘性活塞与普通活塞共享 wiki 条目）：
//   - 放置朝向（:94）：粘性活塞在被放置时永远朝向玩家（活塞头朝玩家，facing=opposite(玩家视线最近方向)，
//     六向含 Up/Down）。与普通活塞放置朝向完全一致（粘性活塞与普通活塞共用 PistonBlock 类，m_sticky 区分）。
//   - 红石触发（:96）：被激活伸出推动方块；取消激活收回活塞头并拉回前方方块（粘性特性）。实际推动/拉回
//     依赖 PistonStructureHelper + MovingPistonBlock + PistonBlockEntity tick 调度，GameTest 不可靠，本组不测。
//   - 破坏（:88）：粘性活塞破坏后掉落自身。破坏掉落物非确定（项目范式不验证掉落物实体，仅测变 air）。
//   - 数据值/方块状态：FACING（6 向 Direction 含 Up/Down）+ EXTENDED（bool），与普通活塞完全一致（同类共享）。
//   - JE/BE 差异：半连接性 JE only（:120）。仅影响推动行为，不影响放置/state/破坏测试。
//
// C++ 链路：粘性活塞与普通活塞共用 PistonBlock 类（redstone/PistonBlock.cpp），构造参数 sticky=true 区分：
//   - 构造函数（:55-78）：与普通活塞完全相同的状态容器（FACING 6 向 + EXTENDED bool），默认 facing=North,
//     extended=false。仅 m_sticky=true。
//   - getStateForPlacement（本提交新增）：与普通活塞完全相同的实现（facing=opposite(
//     getNearestLookingDirection()), extended=false）。粘性活塞共用本类，继承自动获得正确朝向。修复前
//     PistonBlock 未重写该方法（基类 defaultState，FACING 恒 North），粘性活塞同样 FACING 恒 North；
//     修复后粘性活塞自动对齐。
//   - neighborChanged/extend/retract/_doMove 等全部共用，retract（:299-316）粘性活塞额外尝试拉回前方方块
//     （依赖 BlockEntity 落地，不测）。
//   - 物品注册：BlockItemRegistry.cpp:1192 registerSimpleBlock(VanillaBlocks::STICKY_PISTON, "sticky_piston")。
//     方块注册 RedstoneBlocks.cpp:317（sticky=true）。物品与方块均已注册，useItemOnBlock 放置链路可用。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 sticky_piston 物品点击
//   stone → onBlockActivated（stone 非活塞，targetBlock 走基类 Pass）→ fallback Item.useOn →
//   BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext → getStateForPlacement（共用 PistonBlock，
//   facing=opposite(getNearestLookingDirection)）→ setBlockState 放粘性活塞。创造模式不消耗物品。
//
// 朝向控制（复用 BarrelTests/DispenserTests/PistonTests 的含 pitch lookAtLocation 范式，粘性活塞同属含
//   pitch 类）：getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch=0 时 [0]=玩家水平朝向，
//   facing=opposite(水平朝向)。lookAt.y=playerPos.y+1 使 pitch≈0。
//
// 测试覆盖（3 个场景，覆盖 wiki 朝向放置 + state 读写 + 破坏核心确定行为，验证同类共享修复）：
//   1. 水平 4 朝向放置：pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 barrel/dispenser/piston 范式坐标）。验证粘性活塞
//      共用 PistonBlock 的 getStateForPlacement 修复生效。
//   2. facing/extended state 读写：setBlockWithStates 预置 facing=down,extended=false → getState 可读
//      （6 向 facing 含 Up/Down + extended bool 双 state，与普通活塞同构）。
//   3. 粘性活塞破坏不崩溃：放粘性活塞 → setBlockType air 破坏 → 位置变 air（基类 onBlockRemoved 链路安全）。
//
// 关键约束：
// 1. 场景 1 水平 4 朝向复用 BarrelTests/DispenserTests/PistonTests 的 4 朝向映射与坐标配方（粘性活塞共用
//    PistonBlock 同属含 pitch 类，lookAt.y=playerPos.y+1 使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）。
//    每朝向独立 spawn 玩家避免 yaw 残留；每次清理 (3,2,1) 避免粘性活塞残留阻断放置。facing=opposite(水平朝向)。
//    修复前 Cubium 粘性活塞共用缺陷（PistonBlock 未重写 getStateForPlacement，基类 defaultState，FACING 恒
//    North），4 朝向放置 facing 全为 north，断言失败；修复 PistonBlock 后粘性活塞自动对齐。
// 2. 场景 2 setBlockWithStates 预置 facing=down,extended=false（6 向 facing 含 Up/Down，验证非水平方向 state）
//    → getState 双 state 均可读（与普通活塞 PistonTests 场景 3 同构）。
// 3. 场景 3 放粘性活塞后 setBlockType air 破坏：PistonBlock 无 onBlockRemoved override（继承基类
//    Block::onBlockRemoved 空操作），位置变 air。断言变 air（链路不崩溃）。破坏掉落物非确定，仅测变 air。
// 4. 读 facing 用 getState("facing" as any)（DirectionProperty 6 向，返方向名 "north"/"south"/"east"/
//    "west"/"up"/"down"）。读 extended 用 getState("extended" as any)（BooleanProperty 返 bool）。
// 5. 粘性活塞 fullBlock 碰撞箱，isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近真实放置；
//    玩家不能站在 placementPos（碰撞检查 BlockItem.cpp:262-274），场景 1 玩家位置均远离 (3,2,1)。
// 6. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「红石触发实际推动/拉回方块」：粘性活塞 retract 拉回前方方块依赖 _doMove→MovingPiston+
//   PistonBlockEntity tick 调度落地，GameTest 不可靠。本组仅测放置/state/破坏确定行为（与 PistonTests
//   一致）。TODO: 待红石 tick 调度测试范式完善后补 sticky_piston_retracts_and_pulls_block_on_redstone。
// 不测「破坏掉落自身」：破坏掉落物非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧破坏掉落物测试
//   范式完善后补 sticky_piston_drops_itself。
// 不测「粘性特性区分普通活塞」：粘性特性（拉回方块）属不可测项（依赖推动落地）。粘性活塞与普通活塞共用
//   PistonBlock 类，朝向/state/破坏行为相同，本组验证同类共享修复即可。
// 不测「半连接性」：JE only，BE 无，两端不一致不测。
//
// 跨服务端：sticky_piston 方块名两端一致。facing/extended state 名两端一致（C++ 内部名 "facing"/"extended"）。
//   朝向放置（共用 PistonBlock facing=opposite(getNearestLookingDirection)）+ state 读写 + 破坏行为两端一致。
//   修复前 Cubium 粘性活塞共用缺陷（基类 defaultState，FACING 恒 North），修复后对齐。
//   lookAtLocation 是 Cubium 专有朝向控制，但 facing=opposite(视线) 放置行为两端可对比（基岩用真实玩家视线
//   放置），非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_活塞.txt#放置（永远朝向玩家，粘性与普通共享）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_活塞.txt#数据值（FACING 6 向 + EXTENDED bool，同类共享）
// Ref: PistonBlock.cpp（粘性活塞共用 PistonBlock，getStateForPlacement facing=opposite(getNearestLookingDirection) 修复 / retract 拉回逻辑）
// Ref: RedstoneBlocks.cpp:317（STICKY_PISTON 方块注册，sticky=true）
// Ref: BlockItemRegistry.cpp:1192（sticky_piston 物品注册）
// Ref: BarrelTests.ts / DispenserTests.ts / PistonTests.ts（含 pitch lookAtLocation 朝向控制范式 + 水平 4 朝向坐标配方，粘性活塞复用）
// Ref: PistonTests.ts（同源活塞测试，粘性活塞共用 PistonBlock 同构，本组精简为 3 场景）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/3：粘性活塞 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 2：粘性活塞 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 PistonTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 粘性活塞 facing state（方向名字符串 "north"/"south"/"east"/"west"/"up"/"down"）。
// 返回 null 表示失败或非粘性活塞。FACING() 的 C++ 属性名为 "facing"（DirectionProperty 6 向，共用 PistonBlock）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 粘性活塞 extended state（bool）。返回 null 表示失败或非粘性活塞。
// EXTENDED() 的 C++ 属性名为 "extended"（BooleanProperty，共用 PistonBlock）。
function getExtended(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("extended" as any);
    return typeof value === "boolean" ? value : null;
}

// 水平 4 朝向放置映射表（pitch≈0，复用 BarrelTests/DispenserTests/PistonTests 的 4 朝向映射与坐标配方）。
// getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch≈0 时 [0]=玩家水平朝向，facing=opposite(水平朝向)。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0。playerPos.y=2→眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5
//   放大 horizDist 使 pitch 微小，保证 orderedByNearest[0]=水平朝向（非 Down/Up）。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 粘性活塞 facing=opposite(玩家水平朝向)
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

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放粘性活塞位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 sticky_piston useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement（共用 PistonBlock）
//   facing=opposite(getNearestLookingDirection)（pitch≈0 时 [0]=水平朝向）, extended=false → setBlockState
//   放粘性活塞 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「粘性活塞永远朝向玩家」+ 共用 PistonBlock getStateForPlacement 修复生效：水平 4 朝向映射
//   与 barrel/dispenser/piston 一致。修复前 Cubium 粘性活塞共用缺陷（PistonBlock 未重写 getStateForPlacement，
//   基类 defaultState，FACING 恒 North），4 朝向放置 facing 全为 north，断言失败；修复 PistonBlock 后粘性活塞
//   自动对齐。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免粘性活塞残留阻断放置。
function stickyPistonFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向粘性活塞残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 sticky_piston 点击 (3,1,1) stone 顶面 Up → 粘性活塞落 (3,2,1)。
        // getStateForPlacement（共用 PistonBlock）facing=opposite(getNearestLookingDirection)，pitch≈0 时 [0]=水平朝向。
        const stickyPistonItem = new ItemStack("minecraft:sticky_piston", 1);
        const used = player.useItemOnBlock(
            stickyPistonItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing sticky piston facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言粘性活塞 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:sticky_piston", `sticky piston should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `sticky piston facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：facing（6 向含 Up/Down）/extended state 读写——预置 facing=down,extended=false → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 粘性活塞（setBlockWithStates 预置 facing=down,extended=false，绕过物品
//   放置直接写 state）。
//
// 判定：getState("facing")==="down" 且 getState("extended")===false（验证 6 向 facing（含 Up/Down）+ extended
//   双 state 经 setBlockWithStates 写入后可读，与普通活塞同构）。
//
// 此场景验证 wiki「粘性活塞 FACING（6 向含 Up/Down）/ EXTENDED 双 state」可读写（与普通活塞同构）：
//   setBlockWithStates 预置 facing=down（6 向中非水平方向，验证 6 向 state 类型）,extended=false 后
//   getState 双 state 均可读。
function stickyPistonStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=down,extended=false（6 向 facing 含 Up/Down，验证非水平方向 state）。
    (test as TestWithStates).setBlockWithStates("minecraft:sticky_piston", { x: 3, y: 2, z: 1 }, "facing=down,extended=false");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:sticky_piston", `sticky piston should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===down 且 extended===false（6 向 facing + extended 双 state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "down", `facing should be down after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);
    test.assert(getExtended(test, 3, 2, 1) === false, `extended should be false after setBlockWithStates, got ${getExtended(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：粘性活塞破坏不崩溃——放粘性活塞 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 粘性活塞（setBlockType 放置，extended=false 未伸出无活塞头）。
// setBlockType("minecraft:air", (3,2,1)) 破坏粘性活塞 → PistonBlock 无 onBlockRemoved override（继承基类
//   Block::onBlockRemoved 空操作），位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（粘性活塞已破坏，链路不崩溃）。
//
// 此场景验证粘性活塞破坏链路安全性：放粘性活塞（extended=false 未伸出）后破坏，基类 onBlockRemoved
//   不崩溃，位置正确变 air。破坏掉落物（wiki :88 破坏掉落自身）非确定（项目范式不验证掉落物实体），故仅测变 air。
function stickyPistonBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType("minecraft:sticky_piston", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:sticky_piston", `sticky piston should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏粘性活塞 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言粘性活塞 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `sticky piston pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerStickyPistonTests(): void {
    GameTest.register("BlockBehaviorTests", "sticky_piston_facing_opposite_player_looking", stickyPistonFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "sticky_piston_state_readable", stickyPistonStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "sticky_piston_breaks_when_removed", stickyPistonBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
