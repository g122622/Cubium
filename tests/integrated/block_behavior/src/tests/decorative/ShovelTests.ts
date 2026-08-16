// 铲子熄灭营火/压土径行为 GameTest。
//
// wiki tech_锹.txt#用途：
//   - 行283：对点燃的营火使用锹可使其熄灭（lit→false）。
//   - 行281：对着上方为空气的草方块、泥土、砂土、菌丝体、灰化土或缠根泥土的侧面或顶部使用锹可使其
//     变为土径（dirt_path），消耗1耐久。注：Cubium _getPathMap 当前仅映射 grass_block→dirt_path，
//     泥土/砂土等未映射（实现不完整，见 ShovelItem.cpp TODO），故本组只测草方块压土径。
//
// C++ 链路：ShovelItem::onItemUse（item/items/tool/ShovelItem.cpp:57）——物品侧 onItemUse。
//   - 交互顺序 1.熄灭营火/灵魂营火（isLit→with(LIT,false)+音效+耐久→Success）2.压土径。
//   - 压土径：getClickedFace==Down → Pass；getPathBlock(original) 查 _getPathMap（grass_block→dirt_path）
//     → 上方非 air → Pass；否则 setBlockState dirt_path + flatten 音效 + 耐久 → Success。
//
// 派发链路：
//   - 营火：营火 onBlockActivated（烹饪，铲子非食物 findMatchingRecipe 无匹配→Pass）→ fallback
//     ShovelItem.onItemUse 熄灭营火。手持铲子（非 BlockItem）。
//   - 草方块：草方块 onBlockActivated 基类 Pass → fallback ShovelItem.onItemUse 压土径。
//
// 测试覆盖（2 个场景，覆盖 wiki 铲子熄灭营火 + 压草方块土径核心确定行为）：
//   1. 铲子熄灭营火：放点燃营火 + 钻石铲 useItemOnBlock → lit=false，返 true。
//   2. 铲子压草方块成土径：放草方块 + 钻石铲 useItemOnBlock → 原位 grass_block→dirt_path，返 true。
//
// 关键约束：
// 1. 营火默认 lit=true（CampfireBlock setDefaultState LIT=true），glass_pit 无水不触发 waterlogged 熄灭。
//    放 (3,2,1) campfire（minecraft:campfire 默认 lit=true, signal_fire=false, facing=North）。
// 2. 草方块完整方块，放 (3,2,1) grass_block。压土径需上方空气（ShovelItem:124-129），(3,3,1) 默认 air。
// 3. 钻石铲用 new ItemStack("minecraft:diamond_shovel", 1)（耐久 1561，创造 hurtAndBreak 不消耗）。
// 4. useItemOnBlock 传 Direction.Up（点击顶面），ShovelItem 压土径 getClickedFace==Down 检查通过。
// 5. 营火场景读 lit state 用 getState("lit" as any)；草方块场景读 block.typeId 转换。
// 6. 营火有 BlockEntity（CampfireBlockEntity），铲子熄灭只改 lit state 不影响实体；营火 onBlockActivated
//    对铲子（非食物）返 Pass → fallback 铲子熄灭。
//
// 不测「铲子压泥土/砂土成土径」：Cubium _getPathMap 仅 grass_block→dirt_path，dirt/coarse_dirt 未映射
//   （实现不完整，见 ShovelItem.cpp TODO）。不为未实现行为写测试。TODO: 待映射补全后补泥土/砂土压土径。
// 不测「铲子熄灭灵魂营火」：灵魂营火 soul_campfire 同逻辑，本组测普通营火已覆盖熄灭行为点，跳过。
// 不测「从下方点击不压土径」：useItemOnBlock 传 Direction.Down，ShovelItem:114 返 Pass，语义弱，跳过。
// 不测「上方有方块不压土径」：边界场景，跳过。TODO: 待需要时补。
// 不测「铲耐久消耗」：创造模式不消耗，跳过。
//
// 跨服务端：营火 campfire / 草方块 grass_block / 土径 dirt_path 方块名两端一致，铲子熄灭营火 +
//   压草方块土径行为与 vanilla 一致。本组用 setBlockType 放默认方块（营火 lit=true、草方块默认），
//   两端均可放；铲子行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_锹.txt#用途（行283 铲熄灭营火；行281 压草方块等→土径）
// Ref: ShovelItem.cpp（onItemUse 1.熄灭营火 isLit→LIT=false 2.压土径 getPathBlock→dirt_path）
// Ref: CampfireBlock.cpp（setDefaultState LIT=true；onBlockActivated 非食物→Pass 让 fallback 铲熄灭）
// Ref: SimulatedPlayer.cpp:314-343（useItemOnBlock fallback Item.useOn，营火/草方块经此分支到铲子）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 营火/草方块 (3,2,1)。

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取营火 lit state（boolean）。返回 null 表示读取失败或非营火。
function getCampfireLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：铲子熄灭营火——放点燃营火 + 钻石铲 useItemOnBlock → lit=false，返 true。
//
// 布局：(3,2,1) 营火（minecraft:campfire 默认 lit=true）。
// useItemOnBlock ① onBlockActivated（营火烹饪，铲子非食物 findMatchingRecipe 无匹配→Pass）→ ② fallback
//   ShovelItem.onItemUse：营火 + isLit → with(LIT,false) setBlockState + 熄灭音效 + 耐久 → Success。
//
// 判定：useItemOnBlock 返 true（Success），lit === false（营火被铲子熄灭）。
function shovelExtinguishesCampfire(test: Test): void {
    test.setBlockType("minecraft:campfire", { x: 3, y: 2, z: 1 }); // 营火 lit=true
    test.assert(getCampfireLit(test, 3, 2, 1) === true, `campfire lit should be true before, got ${getCampfireLit(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const shovel = new ItemStack("minecraft:diamond_shovel", 1);

    // 对营火 useItemOnBlock 钻石铲 → onBlockActivated Pass → fallback ShovelItem 熄灭营火 → Success。
    const used = farmer.useItemOnBlock(
        shovel as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when extinguishing campfire with shovel");

    // 判定：lit === false（点燃的营火被铲子熄灭）。
    test.assert(getCampfireLit(test, 3, 2, 1) === false, `campfire lit should be false after shovel extinguish, got ${getCampfireLit(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：铲子压草方块成土径——放草方块 + 钻石铲 useItemOnBlock → 原位 grass_block→dirt_path，返 true。
//
// 布局：(3,2,1) 草方块（minecraft:grass_block），上方 (3,3,1) air。
// useItemOnBlock ① onBlockActivated（草方块基类 Pass）→ ② fallback ShovelItem.onItemUse：
//   getPathBlock(grass_block)=GRASS_PATH(dirt_path) + 上方 air → setBlockState dirt_path + flatten 音效 +
//   耐久 → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:dirt_path"。
function shovelFlattensGrassBlockToPath(test: Test): void {
    test.setBlockType("minecraft:grass_block", { x: 3, y: 2, z: 1 }); // 草方块
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:grass_block", `grass_block should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const shovel = new ItemStack("minecraft:diamond_shovel", 1);

    // 对草方块 useItemOnBlock 钻石铲 → onBlockActivated Pass → fallback ShovelItem 压土径 → Success。
    const used = farmer.useItemOnBlock(
        shovel as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when flattening grass_block with shovel");

    // 判定：原位 (3,2,1) 变 dirt_path（草方块被铲子压成土径）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dirt_path", `dirt_path should be at (3,2,1) after flattening, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerShovelTests(): void {
    GameTest.register("BlockBehaviorTests", "shovel_extinguishes_campfire", shovelExtinguishesCampfire)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "shovel_flattens_grass_block_to_path", shovelFlattensGrassBlockToPath)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
