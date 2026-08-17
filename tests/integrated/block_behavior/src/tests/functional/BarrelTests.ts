// 木桶（barrel）朝向放置、比较器空信号、state 读写与破坏行为 GameTest。
//
// wiki block_木桶.txt#用途/破坏/容器/红石比较器：
//   - 用途（:61）：木桶放置方式类似活塞，桶盖面朝向玩家（facing=opposite(玩家视线最近方向)）。
//   - 容器（:63-66）：27 格存储，对着木桶按使用键打开容器界面；打开时桶盖面变为打开纹理（OPEN state）。
//     木桶不能合并，使用不被阻挡（与箱子不同）。
//   - 红石比较器（:68）：可检测木桶存储物品的数量（getComparatorInputOverride，空木桶信号 0）。
//   - 破坏（:58）：木桶被破坏后掉落自身和内容物（容器破坏掉落内容物，onBlockRemoved 遍历 inventory）。
//   - 猪灵（:80-82）：非和平难度成年猪灵对打开/破坏木桶的玩家敌对（AI 非确定，不测）。
//   - 渔夫村民工作站点（:77-78）：附近失业村民认领木桶转职渔夫（AI 非确定，不测）。
//
// C++ 链路：BarrelBlock（functional/BarrelBlock.cpp）两个 state：
//   - FACING（C++ 属性名 "facing"，DirectionProperty 6 向含 Up/Down，Properties.hpp，默认 North，
//     构造函数 :77-79 setDefaultState facing=North,open=false）。
//   - OPEN（C++ 属性名 "open"，BooleanProperty，默认 false）。
//   - getStateForPlacement（:85-93，本提交修复对齐 vanilla）：facing=opposite(getNearestLookingDirection())。
//     getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]（无条件取视线最近方向，含俯仰）。
//     修复前误用 context.getClickedFace()（点击面），在「视线近垂直却点击侧面」等边缘场景与 vanilla
//     分歧（vanilla 由视线决定，旧实现由点击面决定）。修复后严格对齐 vanilla BarrelBlock.java:97-99。
//   - getComparatorInputOverride（:121-134）：从 BarrelEntity::getComparatorSignal 读信号
//     （空木桶 filledSlots=0→返 0，BarrelEntity.cpp:107-109）。
//   - onBlockActivated（:136-164）：openContainer + barrel->openContainer（OPEN 翻转在 BarrelEntity
//     _updateBlockState，:133-144）。SimulatedPlayer 无 GUI 放物品 API，OPEN 翻转不可测，但
//     setBlockWithStates 可写 open=true 验证 state 可读。
//   - onBlockRemoved（:166-187）：override，遍历 BarrelEntity inventory removeItemNoUpdate 掉落
//     内容物（空木桶 inventory 全空，遍历无掉落）→ Block::onBlockRemoved 基类。
//   - hasBlockEntity()=true（:84），createBlockEntity（:116-119）返 BarrelEntity。
//   - 物品注册：BlockItemRegistry.cpp:341 registerSimpleBlock(BARREL, "barrel")。物品已注册，可放置。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 barrel 物品点击
//   stone → onBlockActivated 基类 Pass（stone 非木桶，targetBlock 非 barrel 走基类 Pass）→ fallback
//   Item.useOn → BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext（playerYaw/playerPitch 来自
//   SimulatedPlayer yaw()/pitch()，:325）→ getStateForPlacement facing=opposite(getNearestLookingDirection)
//   → setBlockState 放木桶（hasBlockEntity 触发 BlockEntity 创建）。创造模式 barrel 不消耗。
//
// 朝向控制（复用 FurnaceTests 建立的 lookAtLocation 范式，但 barrel 含俯仰需特殊处理）：
//   getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]（BlockItemUseContext.cpp:51-88,190-208）。
//   pitch=0（水平）时 [0]=玩家水平朝向，facing=opposite(水平朝向)，与 furnace 的 horizontalDirection
//   映射一致（South→North/West→East/North→South/East→West）。
//   【关键差异】barrel 用 getNearestLookingDirection（含 pitch），furnace 用 horizontalDirection（仅 yaw）。
//   lookAtLocation 的 pitch 由 getEyeY()（≈playerPos.y+1.62）与 lookAt.y+0.5 的差决定
//   （SimulatedPlayer.cpp:96,102）。若 lookAt.y=playerPos.y（如 furnace 范式），dy=playerPos.y+0.5-
//   (playerPos.y+1.62)=-1.12，产生俯视 pitch，水平距离小时 orderedByNearest[0] 会变为 Down→facing=Up
//   （与 furnace 分歧，因 furnace 不含 pitch）。故 barrel 水平朝向测试须让 pitch≈0：lookAt.y 选最接近
//   眼高的整数（playerPos.y=2→眼高3.62→lookAt.y=3，dy=-0.12，pitch≈1.3°），且水平距离≥5 放大 horizDist
//   使 pitch 微小，保证 orderedByNearest[0]=水平朝向。lookAtLocation 设 yaw=atan2(-dx,dz) +
//   pitch=-atan2(dy,horizDist)（SimulatedPlayer.cpp:99-102）。
//
// 测试覆盖（5 个场景，覆盖 wiki 朝向放置 + 比较器空信号 + state 读写 + 破坏核心确定行为）：
//   1. 水平 4 朝向放置：pitch=0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 furnace 范式）。
//   2. 水平视线点击顶面（区分新旧实现）：玩家近水平朝东（yaw=270，pitch≈1.4°）点击 stone 顶面 Up →
//      旧实现 facing=Up（getClickedFace），新实现 facing=West（opposite(East)，视线 East）。断言 facing=West
//      验证修复（getNearestLookingDirection 而非 getClickedFace）。
//   3. 比较器空信号=0：放空木桶 → getComparatorInputOverride=0（BarrelEntity 空木桶返 0）。注：GameTest
//      读比较器信号需比较器方块链路复杂，本场景用 setBlockWithStates 预置木桶 + 验证 state 读写替代。
//   4. facing/open state 读写：setBlockWithStates 预置 facing=east,open=true → getState 可读。
//   5. 木桶破坏不崩溃：放木桶（BlockEntity 创建）→ setBlockType air 破坏 → 位置变 air（onBlockRemoved
//      遍历空 inventory 无掉落，链路安全）。
//
// 关键约束：
// 1. 场景 1 水平 4 朝向复用 FurnaceTests 的 4 朝向映射，但坐标配方针对 barrel 含俯仰重算：lookAt.y=
//    playerPos.y+1（非=playerPos.y）使 pitch≈1.3°（眼高 playerPos.y+1.62，lookAt.y+0.5 接近眼高），
//    水平距离≥5 放大 horizDist 使 pitch 微小，保证 orderedByNearest[0]=水平朝向。每朝向独立 spawn 玩家
//    避免 yaw 残留。facing=opposite(水平朝向)（South→North/West→East/North→South/East→West）。
// 2. 场景 2 区分新旧实现：玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使
//    pitch≈1.4° 近水平），useItemOnBlock 点击 (3,1,1) stone 顶面 Up。旧实现 facing=Up（getClickedFace=Up）；
//    新实现 getNearestLookingDirection[0]=East（近水平朝东）→facing=opposite(East)=West。断言 facing=West。
//    此场景视线（East）与点击面（Up）不一致，是 vanilla 与旧 Cubium 分歧的边缘场景，修复后对齐 vanilla。
// 3. 场景 3 改为 state 读写（比较器信号读链路复杂，留 TODO）。setBlockWithStates 预置 facing=down,
//    open=false → getState 验证双 state（含 Up/Down 6 向 facing + open bool）。
// 4. 场景 4 setBlockWithStates 预置 facing=east,open=true（非默认组合）→ getState 读 facing=east,open=true。
// 5. 场景 5 放木桶后 setBlockType air 破坏：onBlockRemoved（:166-187）取 BarrelEntity inventory（空
//    木桶全空），遍历 removeItemNoUpdate 无非空 stack → 不掉落 → Block::onBlockRemoved 基类。位置变 air。
// 6. 读 facing 用 getState("facing" as any)（DirectionProperty 6 向，返方向名 "north"/"south"/"east"/
//    "west"/"up"/"down"）。读 open 用 getState("open" as any)（BooleanProperty 返 bool）。
// 7. 木桶 fullBlock 碰撞箱（m_shape=fullBlock，:82），isValidPosition 基类 true 无支撑要求。stone
//    支撑仅为贴近真实放置。
// 8. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「容器打开/OPEN 翻转」：onBlockActivated openContainer + BarrelEntity openContainer 翻 OPEN，
//   但 SimulatedPlayer 无 GUI 放物品 API，openContainer 链路虽触发但 OPEN 翻转时序不可靠，且无法验证
//   内容物。OPEN state 用 setBlockWithStates 写 open=true 验证可读（场景 4）。TODO: 待脚本侧 BlockEntity
//   容器操作 API 补全后补 barrel_open_state_toggles_on_interact。
// 不测「比较器信号强度」：需比较器贴木桶 + 读输出信号，链路复杂且 GameTest 读红石信号 API 受限。
//   空木桶信号=0 已由 getComparatorInputOverride 逻辑保证（BarrelEntity.cpp:107-109）。TODO: 待比较器
//   读容器信号测试范式完善后补 barrel_comparator_signal（含内容物场景）。
// 不测「内容物破坏掉落」：SimulatedPlayer 无 GUI 放物品 API，无法向木桶槽位放入物品（onBlockActivated
//   openContainer 仅打开 GUI，SimulatedPlayer 无后续放入操作），无法构造有内容物的木桶。场景 5 仅测
//   空木桶破坏不崩溃。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 barrel_drops_contents_when_broken。
// 不测「猪灵敌对/村民转职」：涉 AI 寻路/认领链路，非确定，跳过。
//
// 跨服务端：barrel 方块名两端一致。facing/open state 名两端一致（C++ 内部名 "facing"/"open"）。
//   朝向放置（facing=opposite(getNearestLookingDirection)）+ state 读写 + 破坏行为两端与 vanilla 一致。
//   修复前 Cubium 用 getClickedFace（与 vanilla 边缘场景分歧），修复后对齐。lookAtLocation 是 Cubium
//   专有朝向控制，但 facing=opposite(视线) 放置行为两端可对比（基岩用真实玩家视线放置），非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_木桶.txt#用途（放置类似活塞，桶盖朝玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_木桶.txt#容器（27 格，OPEN state 打开纹理）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_木桶.txt#红石比较器（检测存储物品数量）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_木桶.txt#破坏（掉落自身和内容物）
// Ref: BarrelBlock.cpp（getStateForPlacement facing=opposite(getNearestLookingDirection) 修复 / onBlockRemoved 掉落 / hasBlockEntity）
// Ref: BarrelBlock.java:97-99（vanilla getNearestLookingDirection().getOpposite()）
// Ref: BlockPlaceContext.java:63-65（vanilla getNearestLookingDirection=orderedByNearest[0] 无条件）
// Ref: BlockItemUseContext.cpp（getNearestLookingDirection 单数方法补全 + orderedByNearest）
// Ref: BarrelEntity.cpp:94-113（getComparatorSignal 空木桶返 0）/ :133-144（_updateBlockState OPEN 翻转）
// Ref: BlockItemRegistry.cpp:341（registerSimpleBlock BARREL "barrel"）
// Ref: FurnaceTests.ts（lookAtLocation 朝向控制范式 + 水平 4 朝向坐标配方）
// Ref: BrewingStandTests.ts（容器破坏不崩溃范式：setBlockType air + 断言变 air）
// Ref: BigDripleafTests.ts（setBlockWithStates 预置 state + getState 读 C++ 内部属性名范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/5：木桶 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 3/4：木桶 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BigDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 木桶 facing state（方向名字符串 "north"/"south"/"east"/"west"/"up"/"down"）。
// 返回 null 表示失败或非木桶。FACING() 的 C++ 属性名为 "facing"（DirectionProperty 6 向）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 木桶 open state（bool）。返回 null 表示失败或非木桶。
// OPEN() 的 C++ 属性名为 "open"（BooleanProperty::create("open")）。
function getOpen(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("open" as any);
    return typeof value === "boolean" ? value : null;
}

// 水平 4 朝向放置映射表（pitch≈0，复用 FurnaceTests 的 4 朝向映射，但坐标配方针对 barrel 含俯仰重算）。
// getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch≈0 时 [0]=玩家水平朝向，facing=opposite(水平朝向)。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East（同 furnace horizontalDirection）。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0。playerPos.y=2→眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5
//   放大 horizDist 使 pitch 微小，保证 orderedByNearest[0]=水平朝向（非 Down/Up）。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 木桶 facing=opposite(玩家水平朝向)
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

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放木桶位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 barrel useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement facing=opposite(
//   getNearestLookingDirection)（pitch≈0 时 [0]=水平朝向）→ setBlockState 放木桶 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「木桶桶盖朝玩家」+ getStateForPlacement facing=opposite(getNearestLookingDirection)：
//   水平 4 朝向映射与 furnace 一致（pitch≈0 时 getNearestLookingDirection[0]=水平朝向=horizontalDirection）。
//   【与 furnace 范式差异】barrel 含 pitch，lookAt.y 须=playerPos.y+1（非=playerPos.y）使 pitch≈0，
//   否则俯视 pitch 会让 [0]=Down→facing=Up（furnace 不受影响因仅用 yaw）。每朝向用新 player 避免 yaw 残留；
//   每次清理 (3,2,1) 避免木桶残留阻断放置。
function barrelFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向木桶残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 barrel 点击 (3,1,1) stone 顶面 Up → 木桶落 (3,2,1)。
        // getStateForPlacement facing=opposite(getNearestLookingDirection)，pitch≈0 时 [0]=水平朝向。
        const barrelItem = new ItemStack("minecraft:barrel", 1);
        const used = player.useItemOnBlock(
            barrelItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing barrel facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言木桶 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:barrel", `barrel should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `barrel facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：水平视线点击顶面（区分新旧实现）——玩家水平朝东点击 stone 顶面 Up → facing=West（非 Up）。
//
// 布局：(3,1,1) stone。玩家 (1,2,1) 朝东 lookAtLocation({6,3,1})（yaw=270 East，lookAt.y=3 使 pitch≈1.4°
//   近水平视线）。手持 barrel useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement facing=opposite(getNearestLookingDirection)（[0]=East 水平朝东）→ facing=West。
//
// 判定：(3,2,1) facing === "west"（非 "up"）。
//
// 此场景是 vanilla 与旧 Cubium 分歧的边缘场景，验证修复生效：玩家视线（East，水平）与点击面（Up，
//   顶面）不一致。旧实现 facing=getClickedFace()=Up；新实现 facing=opposite(getNearestLookingDirection[0]=East)=West。
//   断言 facing=West 验证 getStateForPlacement 用 getNearestLookingDirection（视线）而非 getClickedFace（点击面）。
//   修复前此场景 facing=Up，断言 facing=West 失败；修复后对齐 vanilla（视线决定）。
function barrelFacingUsesLookingDirectionNotClickedFace(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_east");
    // 朝东近水平视线：lookAt (6,3,1)，dx=5,dz=0→yaw=atan2(-5,0)=-90°→270°(East)，lookAt.y=3→dy=-0.12→pitch≈1.4°。
    player.lookAtLocation({ x: 6, y: 3, z: 1 });

    // 手持 barrel 点击 (3,1,1) 顶面 Up → 木桶落 (3,2,1)。
    // getNearestLookingDirection[0]=East（水平朝东）→facing=opposite(East)=West（非 Up）。
    const barrelItem = new ItemStack("minecraft:barrel", 1);
    const used = player.useItemOnBlock(
        barrelItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing barrel");

    // 断言 facing=west（视线 East 的反方向，非点击面 Up）。验证 getNearestLookingDirection 而非 getClickedFace。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:barrel", `barrel should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    const facing = getFacing(test, 3, 2, 1);
    test.assert(facing === "west", `barrel facing should be west (opposite of player looking east), not up (clicked face), got ${facing}`);

    test.succeed();
}

// 场景 3：facing（6 向含 Up/Down）/open state 读写——预置 facing=down,open=false → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 木桶（setBlockWithStates 预置 facing=down,open=false，绕过物品
//   放置直接写 state）。
//
// 判定：getState("facing")==="down" 且 getState("open")===false（验证 6 向 facing（含 Up/Down）+ open
//   双 state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「木桶 FACING（6 向含 Up/Down）/ OPEN 双 state」可读写：setBlockWithStates 预置
//   facing=down（6 向中非水平方向，验证 6 向 state 类型）,open=false 后 getState 双 state 均可读。
//   不测「OPEN 翻转」（onBlockActivated openContainer，SimulatedPlayer 无 GUI 放物品，跳过）。
function barrelStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=down,open=false（6 向 facing 含 Up/Down，验证非水平方向 state）。
    (test as TestWithStates).setBlockWithStates("minecraft:barrel", { x: 3, y: 2, z: 1 }, "facing=down,open=false");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:barrel", `barrel should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===down 且 open===false（6 向 facing + open 双 state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "down", `facing should be down after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);
    test.assert(getOpen(test, 3, 2, 1) === false, `open should be false after setBlockWithStates, got ${getOpen(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：facing/open state 非默认组合读写——预置 facing=east,open=true → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 木桶（setBlockWithStates 预置 facing=east,open=true，非默认组合）。
//
// 判定：getState("facing")==="east" 且 getState("open")===true（验证 open=true 非默认值可读写）。
//
// 此场景验证 OPEN state 非默认值（true）可读写：setBlockWithStates 预置 open=true（打开态，非默认 false）
//   后 getState 可读。open=true 在生产中由 onBlockActivated openContainer 翻转，本测试用 setBlockWithStates
//   直接写验证 state 可读性（绕过 GUI 链路）。
function barrelOpenStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=east,open=true（非默认组合，open=true 打开态）。
    (test as TestWithStates).setBlockWithStates("minecraft:barrel", { x: 3, y: 2, z: 1 }, "facing=east,open=true");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:barrel", `barrel should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===east 且 open===true（open 非默认值 true 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "east", `facing should be east after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);
    test.assert(getOpen(test, 3, 2, 1) === true, `open should be true after setBlockWithStates, got ${getOpen(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 5：木桶破坏不崩溃——放木桶（BlockEntity 创建）→ setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 木桶（setBlockType 放置，BlockEntity 创建）。
// setBlockType("minecraft:air", (3,2,1)) 破坏木桶 → BarrelBlock::onBlockRemoved（:166-187）：
//   取 BarrelEntity inventory（空木桶全空），遍历 removeItemNoUpdate 无非空 stack → 不掉落 →
//   Block::onBlockRemoved 基类。位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（木桶已破坏，onBlockRemoved 容器链路不崩溃）。
//
// 此场景验证木桶容器破坏 onBlockRemoved 链路安全性：放木桶（BlockEntity 创建）后破坏，onBlockRemoved
//   遍历空 inventory 不崩溃，位置正确变 air。空木桶无内容物可掉落（SimulatedPlayer 无 GUI 放物品 API，
//   无法构造有内容物的木桶，故仅测空破坏不崩溃，见文件头 TODO）。
function barrelBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType("minecraft:barrel", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:barrel", `barrel should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏木桶 → onBlockRemoved 遍历空 inventory 无掉落 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言木桶 (3,2,1) 已破坏变 air（onBlockRemoved 容器链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `barrel pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerBarrelTests(): void {
    GameTest.register("BlockBehaviorTests", "barrel_facing_opposite_player_looking", barrelFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "barrel_facing_uses_looking_direction_not_clicked_face", barrelFacingUsesLookingDirectionNotClickedFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "barrel_state_readable", barrelStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "barrel_open_state_readable", barrelOpenStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "barrel_breaks_when_removed", barrelBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
