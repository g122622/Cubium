// 锻造台（smithing_table）物品放置链路、state 无属性验证与破坏行为 GameTest。
//
// wiki block_锻造台.txt#破坏/用途/数据值：
//   - 破坏（:37）：锻造台被破坏后掉落自身（基线掉落行为）。挖掘工具为斧（涉工具判定，本组不测）。
//   - 用途（:39）：右键打开锻造 GUI（升级装备/加纹饰），涉 GUI，SimulatedPlayer 无 GUI 放物品 API，
//     不可测交互，留 TODO。
//   - 更改村民职业：锻造台是工具匠村民工作站（涉村民 AI，非方块放置行为，本组不测）。
//   - 数据值/方块状态：锻造台无 BlockState 属性（朝向无关完整方块，对齐 vanilla 无 facing）。
//   - 锻造台无 BlockEntity（vanilla 无，一致）。
//   - 锻造台不产生比较器信号（vanilla 无 getAnalogOutputSignal override）。
//   - JE/BE 差异：仅音效差异，不影响放置/掉落测试。
//
// C++ 链路：SmithingTableBlock（blocks/functional/SmithingTableBlock.cpp）：
//   - 无 BlockState 属性（构造函数 StateContainer::Builder 不 .add 任何 property），m_shape=fullBlock()。
//     对齐 vanilla SmithingTableBlock.java extends CraftingTableBlock（CraftingTableBlock extends Block）
//     无 createBlockStateDefinition override（无 facing 属性）。
//   - getStateForPlacement（:68-71）：return defaultState()——对齐 vanilla（vanilla 无 override，用基类
//     默认 defaultBlockState()）。锻造台是朝向无关方块，放置永远用默认状态。
//   - getShape：返 fullBlock（对齐 vanilla 默认完整方块）。
//   - onBlockActivated（:79-100）：openContainer(ContainerType::Smithing) + INTERACT_WITH_SMITHING_TABLE
//     统计（涉 GUI，SimulatedPlayer 不可测交互）。
//   - 无 canSurvive/updatePostPlacement/hasBlockEntity/rotate/mirror。
//   - 物品注册：BlockItemRegistry.cpp:346 registerSimpleBlock(VanillaBlocks::SMITHING_TABLE,
//     "smithing_table")，方块注册 BuildingBlocks.cpp:335 registerBlock<SmithingTableBlock>
//     (Material::WOOD, hardness2.5, resistance2.5, flammable, ignitedByLava)。物品与方块均已注册，
//     useItemOnBlock 放置链路可用。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 smithing_table 物品
//   点击 stone 顶面 → onBlockActivated（stone 非锻造台，targetBlock 走基类 Pass）→ fallback Item.useOn →
//   BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext → getStateForPlacement defaultState() →
//   setBlockState 放锻造台。创造模式不消耗物品。
//
// 测试覆盖（3 个场景，覆盖 wiki 物品放置 + state 无属性 + 破坏核心确定行为）：
//   1. 物品放置链路：useItemOnBlock smithing_table 放置 → typeId=smithing_table（验证物品注册
//      补全后放置可用）。
//   2. state 无 facing 属性验证：放置后 getState("facing") 返回 undefined（锻造台无 facing 属性，对齐
//      vanilla 无 facing 设计，区别于朝向类方块）。
//   3. 破坏不崩溃：放锻造台 → setBlockType air 破坏 → 位置变 air（无 BlockEntity，链路安全）。
//
// 关键约束：
// 1. 场景 1 用 useItemOnBlock 放置：手持 smithing_table 点击 (3,1,1) stone 顶面 Up →
//    placementPos=(3,2,1)（stone 不可替换 → 相邻位置上方 air）→ getStateForPlacement defaultState() →
//    setBlockState 放置。断言 typeId=smithing_table。
// 2. 场景 2 验证无 facing 属性：getState("facing" as any) 对无 facing 属性的方块返回 undefined（属性
//    不存在）。这区别于朝向类方块（furnace/glazed/stonecutter/loom 返方向名字符串）。锻造台对齐 vanilla
//    无 facing，getState("facing") 必为 undefined，证明方块状态容器无 facing 属性。
// 3. 场景 3 放锻造台后 setBlockType air 破坏：纯功能方块无 BlockEntity，基类 onBlockRemoved 空操作，
//    位置变 air。断言位置变 air（破坏成功，链路不崩溃）。破坏掉落物非确定（项目范式不验证掉落物实体，
//    见 GrindstoneTests/BarrelTests），仅测变 air。
// 4. 锻造台碰撞箱为 fullBlock，有碰撞但 isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近真实
//    放置；玩家不能站在 placementPos（碰撞检查 BlockItem.cpp:262-274），场景 1 玩家位置 (1,2,1) 远离
//    (3,2,1)，不冲突。
// 5. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「锻造 GUI 交互」：涉 GUI + 装备/纹饰物品放入，SimulatedPlayer 无 GUI 放物品 API，不可测。
//   TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 smithing_table_opens_gui。
// 不测「破坏工具（斧）」：涉工具判定 + 徒手/错工具破坏掉落差异，SimulatedPlayer 持工具破坏 API 受限且
//   破坏掉落物非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧工具破坏掉落测试范式完善后补
//   smithing_table_requires_axe_to_drop。
// 不测「更改村民职业」：涉村民 AI 寻路工作站，非方块放置行为，跳过。
// 不测「比较器信号」：锻造台不产生比较器信号（vanilla 无 override），无可测行为。
//
// 跨服务端：smithing_table 方块名两端一致。无 BlockState 属性两端一致（vanilla 无 facing）。
//   物品放置链路 + 破坏行为两端与 vanilla 一致。useItemOnBlock 放置是通用链路两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_锻造台.txt#破坏（破坏掉落自身）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_锻造台.txt#用途（锻造 GUI，涉 GUI 不可测留 TODO）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_锻造台.txt#数据值（无 BlockState 属性，朝向无关）
// Ref: SmithingTableBlock.cpp:68-71（getStateForPlacement defaultState，对齐 vanilla 无 facing）
// Ref: SmithingTableBlock.java（extends CraftingTableBlock，无 FACING 属性，无 getStateForPlacement override）
// Ref: BuildingBlocks.cpp:335（SMITHING_TABLE 方块注册，Material::WOOD hardness2.5）
// Ref: BlockItemRegistry.cpp:346（smithing_table 物品注册）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）
// Ref: FurnaceTests.ts（useItemOnBlock 物品放置链路范式）
// Ref: CartographyTableTests.ts（朝向无关功能方块同构范式，本组复用）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/3：锻造台 (3,2,1)，下方 (3,1,1) stone 支撑。

const SMITHING_TYPE = "minecraft:smithing_table";

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 方块 facing state。锻造台无 facing 属性，返回 undefined（属性不存在）。
// 朝向类方块（furnace 等）返回方向名字符串；锻造台返回 undefined 证明无 facing 属性。
function getFacing(test: Test, x: number, y: number, z: number): string | undefined {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return undefined;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : undefined;
}

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放锻造台位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：物品放置链路——useItemOnBlock smithing_table 放置 → typeId=smithing_table。
//
// 布局：(3,1,1) stone（被点击方块）。玩家 (1,2,1) 默认 yaw=0。手持 smithing_table useItemOnBlock
//   点击 (3,1,1) 顶面 Up → placementPos=(3,2,1)（stone 不可替换 → 相邻位置上方 air）→
//   getStateForPlacement defaultState() → setBlockState 放锻造台。
//
// 判定：(3,2,1) typeId === "minecraft:smithing_table"（物品放置链路可用，物品已注册）。
//
// 此场景验证锻造台物品放置链路：物品已注册（useItemOnBlock 成功放置）。锻造台无朝向，放置永远默认状态，
// 无需验证 facing。
function smithingTableItemPlacement(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "placer");
    const smithingItem = new ItemStack(SMITHING_TYPE, 1);

    // 手持 smithing_table 点击 (3,1,1) 顶面 Up → 锻造台落 (3,2,1)。getStateForPlacement defaultState()。
    const used = player.useItemOnBlock(
        smithingItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing smithing table");

    // 判定：锻造台 (3,2,1) 已放置。
    test.assert(getBlockTypeId(test, 3, 2, 1) === SMITHING_TYPE, `smithing table should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：state 无 facing 属性验证——放置后 getState("facing") 返回 undefined。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 锻造台（useItemOnBlock 放置，defaultState）。
//
// 判定：getFacing(3,2,1) === undefined（锻造台无 facing 属性，对齐 vanilla 无 facing 设计）。
//
// 此场景验证 wiki「锻造台无 BlockState 属性（朝向无关）」：getState("facing") 对无 facing 属性的方块返回
//   undefined。这区别于朝向类方块（furnace/glazed/stonecutter/loom 返方向名字符串）。锻造台对齐 vanilla
//   无 facing，getState("facing") 必为 undefined，证明方块状态容器无 facing 属性。
function smithingTableHasNoFacingState(test: Test): void {
    placeStoneSupport(test);
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "placer");
    const smithingItem = new ItemStack(SMITHING_TYPE, 1);
    player.useItemOnBlock(
        smithingItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(getBlockTypeId(test, 3, 2, 1) === SMITHING_TYPE, `smithing table should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===undefined（锻造台无 facing 属性，对齐 vanilla 无 facing）。
    test.assert(getFacing(test, 3, 2, 1) === undefined, `facing should be undefined (smithing table has no facing property), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：破坏不崩溃——放锻造台 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 锻造台（setBlockType 放置）。
// setBlockType("minecraft:air", (3,2,1)) 破坏锻造台 → 纯功能方块无 BlockEntity → 基类
//   Block::onBlockRemoved（Block.cpp:516-523）空操作 → 位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（锻造台已破坏，链路不崩溃）。
//
// 此场景验证锻造台破坏链路安全性：放锻造台后破坏，基类 onBlockRemoved 空操作不崩溃，位置正确变 air。
//   破坏掉落物（wiki :37 破坏掉落自身）非确定（项目范式不验证掉落物实体，见 GrindstoneTests/BarrelTests），
//   故仅测变 air，不验物品实体。TODO: 待脚本侧破坏掉落物测试范式完善后补 smithing_table_drops_itself。
function smithingTableBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType(SMITHING_TYPE, { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === SMITHING_TYPE, `smithing table should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏锻造台 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言锻造台 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `smithing table pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerSmithingTableTests(): void {
    GameTest.register("BlockBehaviorTests", "smithing_table_item_placement", smithingTableItemPlacement)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "smithing_table_has_no_facing_state", smithingTableHasNoFacingState)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "smithing_table_breaks_when_removed", smithingTableBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
