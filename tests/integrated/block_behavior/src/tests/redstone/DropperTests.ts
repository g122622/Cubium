// 投掷器（dropper）朝向放置（继承发射器）、红石触发 TRIGGERED 与破坏行为 GameTest。
//
// wiki tech_投掷器.txt#放置/用途/红石元件/数据值：
//   - 放置朝向（:37）：投掷器可朝向任何一个方向，包括上方和下方。投掷器在被放置时面向玩家（发射口朝玩家，
//     facing=opposite(玩家视线最近方向)）。与发射器放置朝向完全一致（投掷器继承 DispenserBlock）。
//   - 用途（:44）：投掷器有 9 个物品槽位，对投掷器按使用键可打开界面（涉 GUI，SimulatedPlayer 无 GUI
//     放物品 API，不可测交互）。
//   - 破坏（:28,32）：挖掘工具为镐，破坏后掉落自身 + 内容物（破坏掉落物非确定，项目范式不验证掉落物
//     实体，仅测变 air）。
//   - 红石元件（:69-73）：被激活后产生 1 个计划刻，延时 2 红石刻（4 游戏刻）后投掷一个物品。上升沿触发，
//     不连续发射。随机选槽。投掷行为涉 BlockEntity + 物品实体，依赖 world.entityRegistry 与 tick 调度，
//     GameTest 红石 tick 调度时序不可靠，本组不测实际投掷物品实体（仅测 TRIGGERED 翻转）。
//   - 数据值/方块状态：FACING（6 向 Direction 含 Up/Down）+ TRIGGERED（bool），与发射器完全一致（继承）。
//   - JE/BE 差异：半连接性 JE only（:63）；投掷器向容器传递物品时不发声（:77）。仅影响投掷行为与音效，
//     不影响放置/红石触发 TRIGGERED/破坏测试。
//
// C++ 链路：DropperBlock（redstone/DropperBlock.cpp）继承 DispenserBlock：
//   - 构造函数（:45-49）：noexcept，仅调基类 DispenserBlock(properties)，复用 FACING+TRIGGERED 状态容器
//     与默认状态（facing=North,triggered=false）。对齐 vanilla DropperBlock extends DispenserBlock。
//   - getStateForPlacement：DropperBlock 不重写该方法（vanilla 亦不重写），继承 DispenserBlock 的
//     getStateForPlacement（本提交修复的 facing=opposite(getNearestLookingDirection)）。修复前
//     DispenserBlock 未重写该方法（基类 defaultState，FACING 恒 North），投掷器继承缺陷同样 FACING 恒 North；
//     修复后投掷器自动获得正确朝向。
//   - neighborChanged/tick/updatePostPlacement/hasBlockEntity 等全部继承自 DispenserBlock，红石触发链路可用。
//   - dispense/tryDispense（:51-172）：重写，投掷逻辑（向容器输出或生成物品实体，速度 0.1）。涉 BlockEntity
//     + 物品实体，本组不测。
//   - createBlockEntity（:174-177）：返 DropperBlockEntity（类型 BlockEntityType::Dropper），与发射器
//     DispenserBlockEntity 区分。
//   - 物品注册：BlockItemRegistry.cpp:1194 registerSimpleBlock(VanillaBlocks::DROPPER, "dropper")。
//     方块注册 RedstoneBlocks.cpp:337。物品与方块均已注册，useItemOnBlock 放置链路可用。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 dropper 物品点击 stone →
//   onBlockActivated（stone 非投掷器，targetBlock 走基类 Pass）→ fallback Item.useOn → BlockItem::onItemUse
//   → tryPlace → 构造 BlockItemUseContext → getStateForPlacement（继承 DispenserBlock，facing=opposite(
//   getNearestLookingDirection)）→ setBlockState 放投掷器（hasBlockEntity 触发 DropperBlockEntity 创建）。
//   创造模式不消耗物品。
//
// 朝向控制（复用 BarrelTests/DispenserTests 的含 pitch lookAtLocation 范式，投掷器继承发射器同属含 pitch 类）：
//   getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch=0 时 [0]=玩家水平朝向，
//   facing=opposite(水平朝向)。lookAt.y=playerPos.y+1 使 pitch≈0（眼高≈playerPos.y+1.62，lookAt.y+0.5
//   接近眼高使 dy≈0）。playerPos.y=2→lookAt.y=3（dy=-0.12，pitch≈1.3°），水平距离≥5 放大后 [0]=水平朝向。
//
// 测试覆盖（3 个场景，覆盖 wiki 朝向放置 + 红石触发 TRIGGERED + 破坏核心确定行为，验证继承修复）：
//   1. 水平 4 朝向放置：pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)（4 朝向
//      South→North/West→East/North→South/East→West，复用 barrel/dispenser 范式坐标）。验证投掷器继承
//      DispenserBlock 的 getStateForPlacement 修复生效。
//   2. 红石触发 TRIGGERED 翻转：放投掷器（默认 triggered=false）相邻放红石块 → triggered 翻转为 true
//      （继承 DispenserBlock neighborChanged 红石触发链路）。
//   3. 投掷器破坏不崩溃：放投掷器（DropperBlockEntity 创建）→ setBlockType air 破坏 → 位置变 air
//      （基类 onBlockRemoved 链路安全）。
//
// 关键约束：
// 1. 场景 1 水平 4 朝向复用 BarrelTests/DispenserTests 的 4 朝向映射与坐标配方（投掷器继承发射器同属含
//    pitch 类，lookAt.y=playerPos.y+1 使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）。每朝向独立 spawn
//    玩家避免 yaw 残留；每次清理 (3,2,1) 避免投掷器残留阻断放置。facing=opposite(水平朝向)。修复前 Cubium
//    投掷器继承缺陷（基类 defaultState，FACING 恒 North），4 朝向放置 facing 全为 north，断言失败；修复后对齐 vanilla。
// 2. 场景 2 红石触发 TRIGGERED：放投掷器（默认 triggered=false）+ (4,2,1) 放红石块（水平相邻，全向充能 15）。
//    放红石块走 setBlockState flags=3 → 邻居投掷器 neighborChanged（继承 DispenserBlock）→ isPowered=true
//    != isCurrentlyTriggered(false) → withTriggered(true) 写回 + scheduleBlockTick(4)。仅测 TRIGGERED 翻转
//    （不测 tick→dispense，因投掷依赖 entityRegistry + tick 调度时序，GameTest 不可靠）。pollUntilSucceed
//    轮询 triggered===true。
// 3. 场景 3 放投掷器后 setBlockType air 破坏：DropperBlock 无 onBlockRemoved override（继承基类
//    Block::onBlockRemoved 空操作），DropperBlockEntity 随方块移除清理。位置变 air。断言变 air（链路不崩溃）。
//    破坏掉落物非确定，仅测变 air。
// 4. 读 facing 用 getState("facing" as any)（DirectionProperty 6 向，返方向名 "north"/"south"/"east"/
//    "west"/"up"/"down"）。读 triggered 用 getState("triggered" as any)（BooleanProperty 返 bool）。
// 5. 投掷器 fullBlock 碰撞箱，isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近真实放置；
//    玩家不能站在 placementPos（碰撞检查 BlockItem.cpp:262-274），场景 1 玩家位置均远离 (3,2,1)。
// 6. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「GUI 交互（9 槽放物品）」：onBlockActivated 未实现（继承基类 Pass），SimulatedPlayer 无 GUI 放物品 API，
//   不可测。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 dropper_opens_gui。
// 不测「红石触发实际投掷物品实体」：tick→dispense→tryDispense→spawnItemEntity 依赖 world.entityRegistry
//   与红石 tick 调度时序（scheduleBlockTick(4)），GameTest 红石 tick 调度时序不可靠且投掷物实体非确定。
//   本组仅测 TRIGGERED 翻转（neighborChanged 同步触发，确定可测）。TODO: 待红石 tick 调度测试范式
//   完善后补 dropper_drops_item_on_redstone。
// 不测「向容器输出物品」：投掷器面向容器时尝试放入（tryDispense :99-140），依赖 BlockEntity 容器交互 +
//   红石 tick 调度，链路复杂且非确定。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补
//   dropper_inserts_item_into_adjacent_container。
// 不测「破坏掉落自身 + 内容物」：SimulatedPlayer 无 GUI 放物品 API，无法构造有内容物的投掷器；破坏掉落物
//   非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补
//   dropper_drops_contents_when_broken。
// 不测「比较器信号」：未实现 hasAnalogOutputSignal/getAnalogOutputSignal，无信号可测。
// 不测「半连接性」：JE only，BE 无，两端不一致不测。
//
// 跨服务端：dropper 方块名两端一致。facing/triggered state 名两端一致（C++ 内部名 "facing"/"triggered"）。
//   朝向放置（继承 DispenserBlock facing=opposite(getNearestLookingDirection)）+ 红石触发 TRIGGERED + 破坏
//   行为两端与 vanilla 一致。修复前 Cubium 投掷器继承缺陷（基类 defaultState，FACING 恒 North），修复后对齐。
//   lookAtLocation 是 Cubium 专有朝向控制，但 facing=opposite(视线) 放置行为两端可对比（基岩用真实玩家视线
//   放置），非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_投掷器.txt#放置（可朝任何方向，面向玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_投掷器.txt#红石元件（被激活产生计划刻，4tick 后投掷）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_投掷器.txt#数据值（FACING 6 向 + TRIGGERED bool，继承发射器）
// Ref: DropperBlock.cpp（继承 DispenserBlock，复用 getStateForPlacement / neighborChanged 红石触发 / dispense 投掷）
// Ref: DispenserBlock.cpp（getStateForPlacement facing=opposite(getNearestLookingDirection) 修复，投掷器继承）
// Ref: DropperBlock.java（extends DispenserBlock，无 getStateForPlacement override，靠继承复用）
// Ref: RedstoneBlocks.cpp:337（DROPPER 方块注册）
// Ref: BlockItemRegistry.cpp:1194（dropper 物品注册）
// Ref: BarrelTests.ts（含 pitch lookAtLocation 朝向控制范式 + 水平 4 朝向坐标配方，投掷器复用）
// Ref: DispenserTests.ts（同源发射器测试，投掷器继承同构，本组精简为 3 场景）
// Ref: CopperBulbTests.ts（红石块电源触发邻居 state 翻转范式，TRIGGERED 测试复用）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/3：投掷器 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 2：投掷器 (3,2,1)，红石块电源 (4,2,1) 水平相邻。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 投掷器 facing state（方向名字符串 "north"/"south"/"east"/"west"/"up"/"down"）。
// 返回 null 表示失败或非投掷器。FACING() 的 C++ 属性名为 "facing"（DirectionProperty 6 向，继承自 DispenserBlock）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 投掷器 triggered state（bool）。返回 null 表示失败或非投掷器。
// TRIGGERED() 的 C++ 属性名为 "triggered"（BooleanProperty，继承自 DispenserBlock）。
function getTriggered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("triggered" as any);
    return typeof value === "boolean" ? value : null;
}

// 水平 4 朝向放置映射表（pitch≈0，复用 BarrelTests/DispenserTests 的 4 朝向映射与坐标配方，投掷器继承发射器）。
// getNearestLookingDirection=orderedByNearest(yaw,pitch)[0]，pitch≈0 时 [0]=玩家水平朝向，facing=opposite(水平朝向)。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1（非 =playerPos.y）：玩家眼高≈playerPos.y+1.62，lookAt.y+0.5 须接近眼高
//   使 dy≈0→pitch≈0。playerPos.y=2→眼高3.62→lookAt.y=3（dy=3.5-3.62=-0.12，pitch≈1.3°）。水平距离≥5
//   放大 horizDist 使 pitch 微小，保证 orderedByNearest[0]=水平朝向（非 Down/Up）。
interface FacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的视线方向）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 投掷器 facing=opposite(玩家水平朝向)
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

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放投掷器位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：水平 4 朝向放置——pitch≈0，lookAtLocation 控制水平 yaw → facing=opposite(水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设水平朝向（lookAt.y=playerPos.y+1
//   使 pitch≈1.3°，水平距离≥5 放大后 [0]=水平朝向）→ 清理 (3,2,1) → 手持 dropper useItemOnBlock 点击
//   (3,1,1) stone 顶面 Up → placementPos=(3,2,1) → getStateForPlacement（继承 DispenserBlock）
//   facing=opposite(getNearestLookingDirection)（pitch≈0 时 [0]=水平朝向）→ setBlockState 放投掷器 (3,2,1)。
//   断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家水平朝向)）。
//
// 此场景验证 wiki「投掷器面向玩家」+ 继承 DispenserBlock getStateForPlacement 修复生效：水平 4 朝向映射
//   与 barrel/dispenser 一致。修复前 Cubium 投掷器继承缺陷（DispenserBlock 未重写 getStateForPlacement，
//   基类 defaultState，FACING 恒 North），4 朝向放置 facing 全为 north，断言失败；修复 DispenserBlock 后
//   投掷器继承自动对齐。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免投掷器残留阻断放置。
function dropperFacingOppositePlayerLooking(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设水平朝向（lookAt.y=playerPos.y+1 → dy≈-0.12 → pitch≈1.3°，[0]=水平朝向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向投掷器残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 dropper 点击 (3,1,1) stone 顶面 Up → 投掷器落 (3,2,1)。
        // getStateForPlacement（继承 DispenserBlock）facing=opposite(getNearestLookingDirection)，pitch≈0 时 [0]=水平朝向。
        const dropperItem = new ItemStack("minecraft:dropper", 1);
        const used = player.useItemOnBlock(
            dropperItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing dropper facing ${c.expectedFacing} (player looking ${c.name})`);

        // 断言投掷器 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dropper", `dropper should be placed at (3,2,1) for looking ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `dropper facing should be ${c.expectedFacing} (opposite of player looking ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：红石触发 TRIGGERED 翻转——放投掷器（默认 triggered=false）相邻放红石块 → triggered 翻转为 true。
//
// 布局：(3,2,1) 放投掷器（默认 triggered=false，未充能），(4,2,1) 放红石块（水平相邻，全向充能 15）。
// 放红石块走 setBlockState flags=3 → 邻居投掷器 neighborChanged（继承 DispenserBlock）→ isPowered=红石块
// weakPower 15=true != isCurrentlyTriggered(false) → withTriggered(true) 写回 + scheduleBlockTick(4)。
//
// 判定：pollUntilSucceed 轮询 triggered===true（neighborChanged 同步触发 TRIGGERED 翻转，留余量防时序）。
//
// 此场景验证 wiki「投掷器被激活产生计划刻」红石触发链路的前置（TRIGGERED 翻转，继承 DispenserBlock）。
//   不测 tick→dispense 实际投掷（依赖 entityRegistry + tick 调度时序，GameTest 不可靠，见文件头 TODO）。
function dropperTriggeredTogglesWhenPowered(test: Test): void {
    // (3,2,1) 放投掷器（默认 triggered=false，未充能）。
    test.setBlockType("minecraft:dropper", { x: 3, y: 2, z: 1 });

    // (4,2,1) 放红石块（水平相邻投掷器，getWeakPower 全向 15）。放红石块 flags=3 → 邻居投掷器
    // neighborChanged（继承 DispenserBlock）→ isPowered=true != isCurrentlyTriggered=false → withTriggered(true) 写回。
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
                test.assert(false, `dropper triggered: should be true when powered, got ${getTriggered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：投掷器破坏不崩溃——放投掷器（DropperBlockEntity 创建）→ setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 投掷器（setBlockType 放置，DropperBlockEntity 创建）。
// setBlockType("minecraft:air", (3,2,1)) 破坏投掷器 → DropperBlock 无 onBlockRemoved override（继承基类
//   Block::onBlockRemoved 空操作），DropperBlockEntity 随方块移除清理 → 位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（投掷器已破坏，BlockEntity 链路不崩溃）。
//
// 此场景验证投掷器容器破坏链路安全性：放投掷器（DropperBlockEntity 创建）后破坏，基类 onBlockRemoved
//   不崩溃，位置正确变 air。空投掷器无内容物可掉落（SimulatedPlayer 无 GUI 放物品 API，无法构造有内容物
//   的投掷器，故仅测空破坏不崩溃，见文件头 TODO）。
function dropperBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType("minecraft:dropper", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dropper", `dropper should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏投掷器 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言投掷器 (3,2,1) 已破坏变 air（BlockEntity 链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `dropper pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerDropperTests(): void {
    GameTest.register("BlockBehaviorTests", "dropper_facing_opposite_player_looking", dropperFacingOppositePlayerLooking)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "dropper_triggered_toggles_when_powered", dropperTriggeredTogglesWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "dropper_breaks_when_removed", dropperBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
