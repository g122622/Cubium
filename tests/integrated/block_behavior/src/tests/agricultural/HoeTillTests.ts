// 锄头耕地转换行为 GameTest。
//
// wiki tech_锄.txt#用途：对着泥土、草方块或土径使用锄可将其变为耕地。对着砂土使用锄可将其变为泥土。
//   （缠根泥土用锄变泥土并掉垂根，Cubium 未实现 rooted_dirt 耕地，跳过。）锄耕地需目标上方为空气
//   （不能从下方耕地，且上方有方块则不耕）。
//
// C++ 链路：HoeItem::onItemUse（item/items/tool/HoeItem.cpp:54）——物品侧 onItemUse（非 onBlockActivated）。
//   - getClickedFace()==Down → Pass（不能从下方耕地）。
//   - getTilledBlock(original) 查 _getTillingMap：grass_block/grass_path(dirt_path)/dirt → FARMLAND；
//     coarse_dirt → DIRT。映射外方块 → nullptr → Pass。
//   - 上方非 air → Pass（上方有方块不耕）。
//   - setBlockState(pos, tilledBlock.defaultState, 11)（方块类型转换）+ 耕地音效 + 消耗耐久 → Success。
//
// 派发链路：泥土/草方块/砂土等无 onBlockActivated override（基类返 Pass），useItemOnBlock ① Block.use
//   前置分支 Pass → ② fallback Item.useOn（HoeItem.onItemUse）耕地。手持锄头（非 BlockItem），
//   fallback 不放方块，走耕地转换。
//
// 测试覆盖（3 个场景，覆盖 wiki 锄耕地转换核心确定行为）：
//   1. 锄耕泥土：放泥土 + 钻石锄 useItemOnBlock → 原位 dirt→farmland，返 true。
//   2. 锄耕草方块：放草方块 + 钻石锄 useItemOnBlock → 原位 grass_block→farmland，返 true。
//   3. 锄耕砂土：放砂土 + 钻石锄 useItemOnBlock → 原位 coarse_dirt→dirt（砂土变泥土，非耕地），返 true。
//
// 关键约束：
// 1. 泥土/草方块/砂土完整方块（Material::EARTH），无 canSurvive 自毁，放 (3,2,1) 无需支撑。
// 2. 锄耕地需上方空气（HoeItem:77-81 aboveState 非空气则 Pass）。glass_pit (3,3,1) 默认 air（y=3 在
//   结构 [0,4] 范围内），满足条件。放方块后不占用上方位。
// 3. useItemOnBlock 传 Direction.Up（点击顶面），HoeItem:61 getClickedFace()==Down 检查通过（Up≠Down）。
// 4. 钻石锄用 new ItemStack("minecraft:diamond_hoe", 1)（耐久 1561，创造模式 hurtAndBreak 不消耗耐久）。
// 5. 判定原位 block.typeId 转换：dirt→farmland / grass_block→farmland / coarse_dirt→dirt。
// 6. 锄头走 fallback 分支，useItemOnBlock 成功后对选中槽 shrink(1)（SimulatedPlayer.cpp:331-337），
//   但创造模式不消耗（isCreative 守卫）。每次 new ItemStack 重新设入选中槽（防漂移）。
//
// 不测「缠根泥土用锄掉垂根」：Cubium _getTillingMap 未含 rooted_dirt→dirt 映射（未实现），跳过。
//   TODO: 待 rooted_dirt 耕地 + 垂根掉落实现后补。
// 不测「上方有方块不耕地」：需在目标上方放方块再锄，构造稍繁且属边界，跳过。TODO: 待需要时补。
// 不测「从下方点击不耕地」：useItemOnBlock 传 Direction.Down，HoeItem:61 返 Pass，语义弱，跳过。
// 不测「锄耐久消耗」：创造模式不消耗，生存模式需控耐久，跳过。
//
// 跨服务端：泥土 dirt / 草方块 grass_block / 砂土 coarse_dirt / 耕地 farmland 方块名两端一致，
//   锄头耕地转换行为与 vanilla 一致。本组用 setBlockType 放默认方块（无需 setBlockWithStates），
//   两端均可放；锄耕地行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_锄.txt#用途（泥土/草方块/土径→耕地，砂土→泥土）
// Ref: HoeItem.cpp（onItemUse getTilledMap 转换+音效+耐久→Success；_getTillingMap grass/dirt→farmland, coarse_dirt→dirt）
// Ref: SimulatedPlayer.cpp:314-343（useItemOnBlock fallback Item.useOn，dirt 无 onBlockActivated 故走此分支）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 目标方块 (3,2,1)，上方 (3,3,1) 默认 air（满足锄耕地上方空气条件）。

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 场景 1：锄耕泥土——放泥土 + 钻石锄 useItemOnBlock → 原位 dirt→farmland，返 true。
//
// 布局：(3,2,1) 泥土（minecraft:dirt），上方 (3,3,1) air。
// useItemOnBlock ① onBlockActivated（dirt 基类 Pass）→ ② fallback HoeItem.onItemUse：
//   getTilledBlock(dirt)=FARMLAND + 上方 air → setBlockState farmland + 音效 + 耐久 → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:farmland"。
function hoeTillsDirtToFarmland(test: Test): void {
    test.setBlockType("minecraft:dirt", { x: 3, y: 2, z: 1 }); // 泥土
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dirt", `dirt should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const hoe = new ItemStack("minecraft:diamond_hoe", 1);

    // 对泥土 useItemOnBlock 钻石锄 → fallback HoeItem.onItemUse dirt→farmland → Success。
    const used = farmer.useItemOnBlock(
        hoe as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when tilling dirt with hoe");

    // 判定：原位 (3,2,1) 变 farmland（泥土锄耕为耕地）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:farmland", `farmland should be at (3,2,1) after tilling, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：锄耕草方块——放草方块 + 钻石锄 useItemOnBlock → 原位 grass_block→farmland，返 true。
//
// 布局：(3,2,1) 草方块（minecraft:grass_block），上方 (3,3,1) air。
// fallback HoeItem.onItemUse：getTilledBlock(grass_block)=FARMLAND + 上方 air → farmland → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:farmland"。
function hoeTillsGrassBlockToFarmland(test: Test): void {
    test.setBlockType("minecraft:grass_block", { x: 3, y: 2, z: 1 }); // 草方块
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:grass_block", `grass_block should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const hoe = new ItemStack("minecraft:diamond_hoe", 1);

    // 对草方块 useItemOnBlock 钻石锄 → fallback HoeItem.onItemUse grass_block→farmland → Success。
    const used = farmer.useItemOnBlock(
        hoe as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when tilling grass_block with hoe");

    // 判定：原位 (3,2,1) 变 farmland（草方块锄耕为耕地）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:farmland", `farmland should be at (3,2,1) after tilling grass, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：锄耕砂土——放砂土 + 钻石锄 useItemOnBlock → 原位 coarse_dirt→dirt（砂土变泥土，非耕地），返 true。
//
// 布局：(3,2,1) 砂土（minecraft:coarse_dirt），上方 (3,3,1) air。
// fallback HoeItem.onItemUse：getTilledBlock(coarse_dirt)=DIRT（砂土→泥土，非耕地）+ 上方 air →
//   setBlockState dirt → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:dirt"（砂土变泥土）。
function hoeTillsCoarseDirtToDirt(test: Test): void {
    test.setBlockType("minecraft:coarse_dirt", { x: 3, y: 2, z: 1 }); // 砂土
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:coarse_dirt", `coarse_dirt should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const hoe = new ItemStack("minecraft:diamond_hoe", 1);

    // 对砂土 useItemOnBlock 钻石锄 → fallback HoeItem.onItemUse coarse_dirt→dirt → Success。
    const used = farmer.useItemOnBlock(
        hoe as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when tilling coarse_dirt with hoe");

    // 判定：原位 (3,2,1) 变 dirt（砂土锄耕为泥土，非耕地）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dirt", `dirt should be at (3,2,1) after tilling coarse_dirt, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerHoeTillTests(): void {
    GameTest.register("BlockBehaviorTests", "hoe_tills_dirt_to_farmland", hoeTillsDirtToFarmland)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "hoe_tills_grass_block_to_farmland", hoeTillsGrassBlockToFarmland)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "hoe_tills_coarse_dirt_to_dirt", hoeTillsCoarseDirtToDirt)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
