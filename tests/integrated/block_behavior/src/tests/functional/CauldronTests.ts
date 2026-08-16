// 炼药锅流体装填行为 GameTest。
//
// wiki tech_炼药锅.txt#装料：炼药锅可用水桶/岩浆桶/水瓶等装填流体。
//   - 空炼药锅 + 水桶右键 → 水炼药锅（water_cauldron，水位 level=3，满）。
//   - 空炼药锅 + 岩浆桶右键 → 岩浆炼药锅（lava_cauldron）。
//   - 空炼药锅 + 水瓶右键 → 水炼药锅（水位 level=1，一瓶）。
//   - 非流体容器物品（如石头）右键炼药锅 → 不触发装填（onBlockActivated 返 Pass）。
//   装填后炼药锅方块类型从 cauldron 变为 water_cauldron/lava_cauldron（不同方块，非同方块 state）。
//
// C++ 链路：CauldronBlock（CauldronBlock.cpp）空炼药锅无 level state（方块类型 cauldron）。
//   - onBlockActivated（CauldronBlock.cpp:143-170）：取手持物，依次 _handleBucketInteraction →
//     _handleBottleInteraction，任一非 Pass 即返回；都 Pass 则整体 Pass。
//   - _handleBucketInteraction（:237-278）：水桶（Items::WATER_BUCKET）→ 替换为 water_cauldron
//     defaultState.with(LEVEL_1_3, 3)（满水位 3）+ 倒水音效 + FLUID_PLACE 事件；非创造模式水桶变空桶。
//     岩浆桶（Items::LAVA_BUCKET）→ 替换为 lava_cauldron。
//   - 装填是方块类型替换（setBlockState 写新方块 state），非同方块 state 变化。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。炼药锅 onBlockActivated 处理桶/瓶交互返回 Success，短路不 fallback。
//   useItemOnBlock 调 onBlockActivated 前把 stack 设到主手选中槽，使 onBlockActivated 的
//   player.getHeldItem(hand) 读到水桶/岩浆桶。
//
// 测试覆盖（4 个场景，覆盖 wiki 水桶/岩浆桶装填+玻璃瓶取水+非容器不误触发核心行为）：
//   1. 水桶装水：空炼药锅 + 水桶 useItemOnBlock → water_cauldron level=3。
//   2. 岩浆桶装岩浆：空炼药锅 + 岩浆桶 useItemOnBlock → lava_cauldron。
//   3. 非容器不误触发：空炼药锅 + 石头 useItemOnBlock → 仍 cauldron（桶/瓶交互都 Pass，方块不变）。
//   4. 玻璃瓶取水：water_cauldron level=3 + 玻璃瓶 useItemOnBlock → level 3→2（one-sided，setBlockWithStates
//      放满水炼药锅）。
//
// 关键约束：
// 1. 炼药锅需放在固体方块上方（isValidPosition 检查 belowState.isSolid）——(3,1,1) 放 stone 支撑，
//    (3,2,1) 放空炼药锅（minecraft:cauldron）。
// 2. 装填是方块类型替换：判定用 getBlock 检查方块 typeId 是否变为 water_cauldron/lava_cauldron，
//    water_cauldron 再读 level state===3。
// 3. SimulatedPlayer 默认创造模式：水桶/岩浆桶不消耗（创造跳过空桶替换），但仍装水/岩浆（Success 返回）。
// 4. 场景 3 用石头（非桶非瓶）→ onBlockActivated 桶/瓶交互都 Pass → 整体 Pass → fallback Item.useOn
//    （石头无 onItemUse 返 Pass）→ useItemOnBlock 返回 false，炼药锅不变。
//
// 不测「水瓶装水（level=1）」：水瓶交互走 _handleBottleInteraction，涉及 PotionItem/玻璃瓶替换链路，
//   复杂度高于桶，跳过。TODO: 待瓶交互链路验证后补 cauldron_fills_with_water_bottle。
// 不测「降水/滴石填充」：handlePrecipitation 概率性（雨5%/雪10%），receiveStalactiteDrip 需滴石，
//   非确定/复杂，跳过。
// 不测「空桶取水（water_cauldron level3→0 + 水桶）」：与玻璃瓶取水同类（level 递减），本组测玻璃瓶
//   已覆盖「取水 level 递减」行为点，跳过。
//
// 跨服务端：炼药锅 cauldron/water_cauldron/lava_cauldron 方块名两端一致，桶装填行为与 vanilla 一致。
//   注意：基岩 BDS 无 setBlockWithStates，但本测试用 setBlockType 放空炼药锅（默认 state，无需设 level），
//   两端均可放；装填后判定方块类型两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_炼药锅.txt#装料（水桶/岩浆桶装填）
// Ref: CauldronBlock.cpp（onBlockActivated 桶/瓶交互；_handleBucketInteraction 水桶→water_cauldron level=3/岩浆桶→lava_cauldron）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 炼药锅 (3,2,1)，下方 (3,1,1) stone 支撑（炼药锅需 solid 上方放置）。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性，非官方 type.id）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 water_cauldron level state（number 1-3，Java 口径 state 名 level）。返回 null 表示读取失败或非水炼药锅。
function getWaterCauldronLevel(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("level" as any);
    return typeof value === "number" ? value : null;
}

// 放支撑 + 空炼药锅：(3,1,1) stone 支撑，(3,2,1) 空炼药锅（minecraft:cauldron 默认 state）。
function placeCauldron(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:cauldron", { x: 3, y: 2, z: 1 }); // 空炼药锅
}

// 场景 1：水桶装水——空炼药锅 + 水桶 useItemOnBlock → water_cauldron level=3。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空炼药锅。
// onBlockActivated 取手持水桶 → _handleBucketInteraction WATER_BUCKET → 替换为 water_cauldron
// defaultState.with(LEVEL_1_3, 3) → 返回 Success。
//
// 判定：getBlock typeId === "minecraft:water_cauldron" 且 level === 3。
function cauldronFillsWithWaterBucket(test: Test): void {
    placeCauldron(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const waterBucket = new ItemStack("minecraft:water_bucket", 1);

    // 对空炼药锅 useItemOnBlock 水桶 → onBlockActivated 水桶交互 → water_cauldron level=3。
    const used = farmer.useItemOnBlock(
        waterBucket as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when filling cauldron with water bucket");

    // 判定：方块类型变为 water_cauldron，level=3（满）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:water_cauldron", `cauldron should become water_cauldron, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getWaterCauldronLevel(test, 3, 2, 1) === 3, `water_cauldron level should be 3 (full), got ${getWaterCauldronLevel(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：岩浆桶装岩浆——空炼药锅 + 岩浆桶 useItemOnBlock → lava_cauldron。
//
// 布局：同场景 1。onBlockActivated 取手持岩浆桶 → _handleBucketInteraction LAVA_BUCKET →
// 替换为 lava_cauldron defaultState → 返回 Success。
//
// 判定：getBlock typeId === "minecraft:lava_cauldron"。
function cauldronFillsWithLavaBucket(test: Test): void {
    placeCauldron(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const lavaBucket = new ItemStack("minecraft:lava_bucket", 1);

    const used = farmer.useItemOnBlock(
        lavaBucket as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when filling cauldron with lava bucket");

    // 判定：方块类型变为 lava_cauldron。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:lava_cauldron", `cauldron should become lava_cauldron, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：非容器不误触发——空炼药锅 + 木棍 useItemOnBlock → 仍 cauldron（桶/瓶交互都 Pass）。
//
// 布局：同场景 1。onBlockActivated 取手持木棍 → _handleBucketInteraction（木棍非桶 Pass）→
// _handleBottleInteraction（木棍非瓶 Pass）→ 整体 Pass → fallback Item.useOn（木棍无 onItemUse 行为，
// 默认返 Pass）→ useItemOnBlock 返回 false，炼药锅不变。
//
// 注意：不能用石头等 BlockItem 测「非容器不误触发」——BlockItem::onItemUse 会尝试在点击方块旁放置
// 该方块（face=Up → 炼药锅上方放石头），返回 Success，与「容器交互」无关。木棍是普通 Item（非
// BlockItem），onItemUse 默认 Pass，不触发放置，才能干净验证「非容器物品不触发炼药锅装填」。
//
// 判定：useItemOnBlock 返回 false（未触发装填），方块仍 cauldron。
function cauldronIgnoresNonFluidItem(test: Test): void {
    placeCauldron(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对空炼药锅 useItemOnBlock 木棍 → 桶/瓶交互都 Pass，木棍无 onItemUse → 不装填。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    // 木棍非流体容器，不应触发装填（useItemOnBlock 返回 false）。
    test.assert(!used, `useItemOnBlock should return false for non-fluid item (stick), got ${used}`);

    // 判定：炼药锅仍为空 cauldron（未装填）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should remain empty cauldron for non-fluid item, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：玻璃瓶取水——water_cauldron level=3 + 玻璃瓶 useItemOnBlock → level 3→2。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) water_cauldron level=3（setBlockWithStates 直接放满水炼药锅）。
// water_cauldron 是 LayeredCauldronBlock（非 CauldronBlock 空炼药锅），其 onBlockActivated：
//   玻璃瓶 → _handleBottleInteraction → GLASS_BOTTLE 分支 → lowerFillLevel（level-1）+ 水瓶 → Success。
// lowerFillLevel：level>1 → with(LEVEL_1_3, level-1)（level 3→2）；level==1 → 替换为空 cauldron。
//
// 判定：useItemOnBlock 返 true（Success），level === 2（玻璃瓶取走一格水）。
// one-sided：setBlockWithStates 放满水炼药锅是 Cubium 专有 API（基岩 BDS 无），仅 Cubium 跑。
function cauldronDrainedByGlassBottle(test: Test): void {
    // setBlockWithStates 放满水炼药锅（water_cauldron level=3）。one-sided：Cubium 专有 API。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    (test as unknown as {
        setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
    }).setBlockWithStates("minecraft:water_cauldron", { x: 3, y: 2, z: 1 }, "level=3");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:water_cauldron", `block should be water_cauldron before, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getWaterCauldronLevel(test, 3, 2, 1) === 3, `water_cauldron level should be 3 before, got ${getWaterCauldronLevel(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const glassBottle = new ItemStack("minecraft:glass_bottle", 1);

    // 对满水炼药锅 useItemOnBlock 玻璃瓶 → LayeredCauldronBlock.onBlockActivated → _handleBottleInteraction
    // → GLASS_BOTTLE 分支 lowerFillLevel(level 3→2) + 水瓶 → Success。
    const used = farmer.useItemOnBlock(
        glassBottle as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when draining water_cauldron with glass bottle");

    // 判定：level === 2（玻璃瓶取走一格水，water_cauldron 仍存在，水位降为 2）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:water_cauldron", `block should remain water_cauldron after one drain, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getWaterCauldronLevel(test, 3, 2, 1) === 2, `water_cauldron level should be 2 after glass bottle drain, got ${getWaterCauldronLevel(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCauldronTests(): void {
    GameTest.register("BlockBehaviorTests", "cauldron_fills_with_water_bucket", cauldronFillsWithWaterBucket)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_fills_with_lava_bucket", cauldronFillsWithLavaBucket)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_ignores_non_fluid_item", cauldronIgnoresNonFluidItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_drained_by_glass_bottle", cauldronDrainedByGlassBottle)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
