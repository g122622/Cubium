// 切石机（stonecutter）放置朝向、state 读写与破坏行为 GameTest。
//
// wiki tech_切石机.txt#破坏/用途/方块状态：
//   - 破坏（:47）：切石机被破坏后掉落自身（基线掉落行为）。挖掘工具为镐，需木镐及以上否则不掉落
//     （涉工具等级判定，复杂，本组不测）。
//   - 用途/切石（:50）：右键打开切石 GUI，1 原料→成品切石配方（涉 GUI，SimulatedPlayer 无 GUI 放物品
//     API，不可测交互，留 TODO）。
//   - 更改村民职业（:332）：切石机是石匠村民工作站（涉村民 AI，非方块放置行为，本组不测）。
//   - 方块状态（:387）：HORIZONTAL_FACING（水平朝向，facing=opposite(玩家朝向)）。
//   - 切石机无 BlockEntity（JE 无，BE 历史 MATTIS 旧切石机是不同方块，不测）。
//   - 切石机不产生比较器信号（vanilla 无 getAnalogOutputSignal override）。
//
// C++ 链路：StonecutterBlock（blocks/functional/StonecutterBlock.cpp）单 state：
//   - HORIZONTAL_FACING（C++ 属性名 "facing"，DirectionProperty::createHorizontal，默认 North，
//     构造函数 :69 setDefaultState facing=North）。
//   - getStateForPlacement（:77-82）：facing=opposite(context.horizontalDirection())——对齐 vanilla
//     StonecutterBlock.java:44-47 getHorizontalDirection().getOpposite()（刀片朝向背离玩家）。
//   - horizontalDirection 由 playerYaw 计算（BlockItemUseContext.cpp:111-125）：
//     yaw∈[315,360)∪[0,45)→South，[45,135)→West，[135,225)→North，[225,315)→East。
//   - rotate（:84-89）/mirror（:91-100）用 Directions::rotateDirection/mirrorToRotation（结构旋转/镜像，
//     非 GameTest 范畴，本组不测）。
//   - getShape（:102-106）：固定 m_shape=box(0,0,0,16,9,16)（高 9 像素底座，对齐 vanilla
//     column(16,0,9)）。切石机碰撞箱为低底座（非 fullBlock），但 isValidPosition 基类 true 无支撑要求。
//   - onBlockActivated（:108-129）：openContainer(ContainerType::Stonecutter) + INTERACT_WITH_STONECUTTER
//     统计（涉 GUI，SimulatedPlayer 不可测交互）。
//   - 无 canSurvive/updatePostPlacement/hasBlockEntity（纯功能方块无支撑要求/无容器）。
//   - 物品注册：BlockItemRegistry.cpp 功能方块段补 registerSimpleBlock(VanillaBlocks::STONECUTTER,
//     "stonecutter")。此前方块类已实现但方块注册+物品注册完全缺失（与砂轮/带釉陶瓦物品同类坑），
//     本期补全：BuildingBlocks.hpp 加 STONECUTTER 常量 + BuildingBlocks.cpp 加 registerBlock
//     <StonecutterBlock>(Material::ROCK, hardness3.5, resistance3.5, requiresTool, Pickaxe) +
//     BlockItemRegistry.cpp 加 registerSimpleBlock。功能方块均注册在 BuildingBlocks（无 FunctionalBlocks
//     子注册表），仿 grindstone 范式。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 stonecutter 物品点击
//   stone 顶面 → onBlockActivated（stone 非切石机，targetBlock 走基类 Pass）→ fallback Item.useOn →
//   BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext（playerYaw 来自 SimulatedPlayer yaw(),
//   :325）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)) → setBlockState 放切石机。
//   创造模式不消耗物品。
//
// 朝向控制（复用 FurnaceTests/GlazedTerracottaTests opposite 范式）：
//   切石机 facing=opposite(horizontalDirection)（仅 yaw），与熔炉/带釉陶瓦同语义。故 4 朝向坐标配方与
//   FurnaceTests FACING_CASES 完全一致（玩家位置/lookAt 目标/yaw/facing 映射），仅替换被测方块为
//   stonecutter。lookAtLocation(blockPos) 瞬时设 yaw=atan2(-dx,dz)（0→South,90→West,180→North,
//   270→East，SimulatedPlayer.cpp:99）。
//   4 朝向映射（facing=opposite(玩家朝向)）：
//     - yaw=0(South)→facing=North；yaw=90(West)→facing=East；
//     - yaw=180(North)→facing=South；yaw=270(East)→facing=West。
//
// 测试覆盖（4 个场景，覆盖 wiki 放置朝向 + 物品放置链路 + state 读写 + 破坏核心确定行为）：
//   1. facing=opposite(玩家朝向) 放置（4 朝向）：玩家 lookAtLocation 控制朝向 → useItemOnBlock
//      stonecutter → 断言 facing=opposite(朝向)。4 朝向逐一验证 South→North/West→East/North→South/
//      East→West 映射（验证 getStateForPlacement opposite 对齐 vanilla）。
//   2. 物品放置链路：useItemOnBlock stonecutter 放置 → typeId=stonecutter + facing 默认 North
//      （默认 yaw=0→South→facing=North）。验证物品注册补全后放置可用。
//   3. facing state 读写：setBlockWithStates 预置 facing=east → getState 可读。
//   4. 破坏不崩溃：放切石机 → setBlockType air 破坏 → 位置变 air（无 BlockEntity，链路安全）。
//
// 关键约束：
// 1. 场景 1 朝向控制：每朝向独立 spawn 玩家（不与 (3,1,1)/(3,2,1) 重叠），lookAtLocation 传入 [0,6] 内
//    目标坐标产生目标 yaw，再 useItemOnBlock stonecutter 点击 (3,1,1) stone Up → 落 (3,2,1)。4 朝向
//    玩家位置/lookAt 目标/yaw/facing 映射见场景 1 注释（同 FurnaceTests FACING_CASES）。
// 2. 场景 2 用 useItemOnBlock 放置：手持 stonecutter 点击 (3,1,1) stone 顶面 Up →
//    placementPos=(3,2,1)（stone 不可替换 → 相邻位置上方 air）→ getStateForPlacement facing=North
//    （默认 yaw=0→South→opposite=North）→ setBlockState 放置。断言 typeId + facing=North。
// 3. 场景 3 用 setBlockWithStates 预置 facing=east（绕过物品放置，直接写 state）。getState 读 "facing"
//    （C++ 内部属性名）。验证 facing state 可读写。
// 4. 场景 4 放切石机后 setBlockType air 破坏：纯功能方块无 BlockEntity，基类 onBlockRemoved 空操作，
//    位置变 air。断言位置变 air（破坏成功，链路不崩溃）。破坏掉落物非确定（项目范式不验证掉落物实体，
//    见 GrindstoneTests/BarrelTests），仅测变 air。
// 5. 读 facing 用 getState("facing" as any)（DirectionProperty 序列化为方向名字符串 "north"/"south"/
//    "east"/"west"，与 Java 命名一致，小写）。
// 6. 切石机碰撞箱为低底座（9 像素高），但 isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近
//    真实放置；玩家不能站在 placementPos（碰撞检查 BlockItem.cpp:262-274），但低底座碰撞箱使玩家脚可能
//    落在 (3,2,1)——场景 1/2 玩家位置 (1,2,1) 等远离 (3,2,1)，不冲突。
// 7. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「切石 GUI 交互」：涉 GUI + 切石配方 + 物品放入，SimulatedPlayer 无 GUI 放物品 API，不可测。
//   TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 stonecutter_opens_gui。
// 不测「破坏工具等级」：wiki 需木镐及以上否则不掉落，涉工具等级判定 + 徒手/错工具破坏掉落差异，
//   SimulatedPlayer 持工具破坏 API 受限且破坏掉落物非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧
//   工具等级破坏掉落测试范式完善后补 stonecutter_requires_pickaxe_to_drop。
// 不测「更改村民职业」：涉村民 AI 寻路工作站，非方块放置行为，跳过。
// 不测「比较器信号」：切石机不产生比较器信号（vanilla 无 override），无可测行为。
// 不测「rotate/mirror」：结构旋转/镜像非 GameTest 放置范畴，跳过。
//
// 跨服务端：stonecutter 方块名两端一致。facing state 名两端一致（C++ 内部名 "facing"，Java 命名；BE 用
//   cardinal_direction 是 BE 偏差，Cubium 对齐 JE 用 facing）。朝向放置（facing=opposite(玩家朝向)）+
//   state 读写 + 破坏行为两端与 vanilla 一致。lookAtLocation 是 Cubium 专有朝向控制（基岩 GameTest 无
//   此 API），但 facing=opposite(朝向) 放置行为本身两端可对比（基岩用真实玩家朝向放置），非 one-sided。
//   setBlockWithStates 预置 state 是 Cubium 专有写入，但 state 行为本身两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_切石机.txt#破坏（破坏掉落自身）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_切石机.txt#用途（切石 GUI，涉 GUI 不可测留 TODO）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_切石机.txt#方块状态（HORIZONTAL_FACING facing=opposite(朝向)）
// Ref: StonecutterBlock.cpp:77-82（getStateForPlacement facing=opposite(horizontalDirection)，对齐 vanilla）
// Ref: StonecutterBlock.java:44-47（getHorizontalDirection().getOpposite()，刀片朝向背离玩家）
// Ref: BlockItemUseContext.cpp:111-125（horizontalDirection 由 playerYaw 计算：0=South/90=West/180=North/270=East）
// Ref: SimulatedPlayer.cpp:90-106（lookAtLocation yaw=atan2(-dx,dz)，瞬时 setRotation）
// Ref: BuildingBlocks.cpp（STONECUTTER 方块注册，本期补全，仿 grindstone 范式）
// Ref: BlockItemRegistry.cpp（stonecutter 物品注册，本期补全）
// Ref: FurnaceTests.ts:175-180（facing=opposite(horizontalDirection) 4 朝向坐标配方，本组复用）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）
// Ref: BigDripleafTests.ts（setBlockWithStates 预置 state + getState 读 C++ 内部属性名范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/4：切石机 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 3：切石机 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

const STONECUTTER_TYPE = "minecraft:stonecutter";

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BigDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 切石机 facing state（方向名字符串 "north"/"south"/"east"/"west"）。返回 null 表示失败或非切石机。
// 注意：HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（DirectionProperty::createHorizontal("facing")），
// getState 按内部名匹配，返回方向名字符串（Java 命名，小写）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 朝向放置映射表：玩家朝向 → 切石机 facing（facing=opposite(玩家朝向)，同 FurnaceTests/GlazedTerracotta 语义）。
// horizontalDirection（BlockItemUseContext.cpp:117-124）：yaw∈[315,360)∪[0,45)→South，
// [45,135)→West，[135,225)→North，[225,315)→East。切石机 facing=opposite(horizontalDirection)。
// lookAtLocation yaw=atan2(-dx,dz)（SimulatedPlayer.cpp:99）：0→South,90→West,180→North,270→East。
interface FacingCase {
    name: string; // 玩家朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（结构相对，不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（产生目标 yaw，坐标在 [0,6] 内）
    expectedFacing: string; // 切石机 facing=opposite(玩家朝向)
}

// 4 朝向逐一推算（玩家位置 + lookAt 目标 → yaw → horizontalDirection → facing=opposite）：
//   South（yaw∈[315,360)∪[0,45)→facing=North）：玩家 (1,2,1)，lookAt (3,2,5)，
//     dx=2,dz=4→atan2(-2,4)≈-26.6°→+360=333°∈[315,360)→South→facing=North。
//   West（yaw∈[45,135)→facing=East）：玩家 (1,2,1)，lookAt (0,2,1)，
//     dx=-1,dz=0→atan2(1,0)=90°∈[45,135)→West→facing=East。
//   North（yaw∈[135,225)→facing=South）：玩家 (1,2,5)，lookAt (3,2,1)，
//     dx=2,dz=-4→atan2(-2,-4)≈153°∈[135,225)→North→facing=South。
//   East（yaw∈[225,315)→facing=West）：玩家 (1,2,1)，lookAt (6,2,1)，
//     dx=5,dz=0→atan2(-5,0)=-90°→+360=270°∈[225,315)→East→facing=West。
// 玩家位置均不与 (3,1,1)/(3,2,1) 重叠；lookAt 目标均在 [0,6] 内不越界。
// 坐标配方同 FurnaceTests FACING_CASES（切石机与熔炉/带釉陶瓦 facing 语义一致：opposite(horizontalDirection)，
// 仅 yaw，lookAt.y=playerPos.y）。
const FACING_CASES: FacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 2, z: 5 }, expectedFacing: "north" },
    { name: "west", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 0, y: 2, z: 1 }, expectedFacing: "east" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 2, z: 1 }, expectedFacing: "south" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 2, z: 1 }, expectedFacing: "west" },
];

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放切石机位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：facing=opposite(玩家朝向) 放置——4 朝向逐一验证（验证 getStateForPlacement opposite 对齐 vanilla）。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持
//   stonecutter useItemOnBlock 点击 (3,1,1) stone 顶面 Up → placementPos=(3,2,1)（stone 不可替换 →
//   相邻位置上方 air）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)) → setBlockState
//   放切石机 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家朝向)）。
//
// 此场景验证 wiki「切石机方块状态 facing」+ getStateForPlacement facing=opposite(horizontalDirection)：
//   玩家朝 South→切石机 facing=North，朝 West→facing=East，朝 North→facing=South，朝 East→facing=West。
//   用 lookAtLocation 控制 yaw（复用 FurnaceTests opposite 范式坐标）。每朝向用新 player 避免 yaw 残留；
//   每次清理 (3,2,1) 避免切石机残留阻断放置。
function stonecutterFacingOppositePlayerFacing(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        // 每朝向独立 spawn 玩家（避免 yaw 残留）。
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 切石机 facing=opposite。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向切石机残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 stonecutter 点击 (3,1,1) stone 顶面 Up → 切石机落 (3,2,1)。
        // getStateForPlacement facing=opposite(horizontalDirection(yaw))。
        const stonecutterItem = new ItemStack(STONECUTTER_TYPE, 1);
        const used = player.useItemOnBlock(
            stonecutterItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing stonecutter facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言切石机 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === STONECUTTER_TYPE, `stonecutter should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `stonecutter facing should be ${c.expectedFacing} (opposite of player facing ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：物品放置链路——useItemOnBlock stonecutter 放置 → typeId + facing 默认 North。
//
// 布局：(3,1,1) stone（被点击方块）。玩家 (1,2,1) 默认 yaw=0（朝 South，spawn 未设 yaw）。手持
//   stonecutter useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) → getStateForPlacement
//   facing=opposite(South)=North → setBlockState 放切石机。
//
// 判定：(3,2,1) typeId === "minecraft:stonecutter" 且 facing==="north"（默认 yaw=0→South→opposite=North）。
//   验证物品注册补全后放置链路可用（修复前物品未注册，useItemOnBlock 无法放置）。
//
// 此场景验证切石机物品放置链路 + getStateForPlacement 朝向（默认朝向 North）：物品已注册
//   （useItemOnBlock 成功放置）+ facing=North（默认 yaw=0→South→opposite=North）。场景 1 已详测 4 朝向，
//   此场景聚焦物品放置链路可用性（回归验证物品注册补全）。
function stonecutterItemPlacement(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "placer");
    const stonecutterItem = new ItemStack(STONECUTTER_TYPE, 1);

    // 手持 stonecutter 点击 (3,1,1) 顶面 Up → 切石机落 (3,2,1)。
    // getStateForPlacement facing=opposite(South)=North（默认 yaw=0→South）。
    const used = player.useItemOnBlock(
        stonecutterItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing stonecutter");

    // 判定：切石机 (3,2,1) 已放置，facing=north（默认 yaw=0→South→opposite=North）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === STONECUTTER_TYPE, `stonecutter should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "north", `facing should be north after placement (default yaw=0→South→opposite=North), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：facing state 读写——预置 facing=east → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 切石机（setBlockWithStates 预置 facing=east，绕过物品放置直接写 state）。
//
// 判定：getState("facing")==="east"（验证 facing state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「切石机 HORIZONTAL_FACING state」可读写：setBlockWithStates 预置 facing=east 后
//   getState 可读。与场景 1/2（物品放置 facing=opposite(朝向)）互补，验证 state 直接写入路径。
function stonecutterStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=east（从默认 state 出发逐属性应用）。
    (test as TestWithStates).setBlockWithStates(STONECUTTER_TYPE, { x: 3, y: 2, z: 1 }, "facing=east");
    test.assert(getBlockTypeId(test, 3, 2, 1) === STONECUTTER_TYPE, `stonecutter should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===east（state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "east", `facing should be east after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：破坏不崩溃——放切石机 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 切石机（setBlockType 放置）。
// setBlockType("minecraft:air", (3,2,1)) 破坏切石机 → 纯功能方块无 BlockEntity → 基类
//   Block::onBlockRemoved（Block.cpp:516-523）空操作 → 位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（切石机已破坏，链路不崩溃）。
//
// 此场景验证切石机破坏链路安全性：放切石机后破坏，基类 onBlockRemoved 空操作不崩溃，位置正确变 air。
//   破坏掉落物（wiki :47 破坏掉落自身）非确定（项目范式不验证掉落物实体，见 GrindstoneTests/BarrelTests），
//   故仅测变 air，不验物品实体。TODO: 待脚本侧破坏掉落物测试范式完善后补 stonecutter_drops_itself。
function stonecutterBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType(STONECUTTER_TYPE, { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === STONECUTTER_TYPE, `stonecutter should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏切石机 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言切石机 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `stonecutter pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerStonecutterTests(): void {
    GameTest.register("BlockBehaviorTests", "stonecutter_facing_opposite_player_facing", stonecutterFacingOppositePlayerFacing)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "stonecutter_item_placement", stonecutterItemPlacement)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "stonecutter_state_readable", stonecutterStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "stonecutter_breaks_when_removed", stonecutterBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
