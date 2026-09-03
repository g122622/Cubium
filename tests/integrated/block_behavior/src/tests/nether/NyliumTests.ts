// 菌岩（crimson_nylium/warped_nylium）行为 GameTest：验证骨粉生成下界植被 + 光照退化
// （对齐 wiki 绯红菌岩/诡异菌岩#骨粉 + #用途 退化章节）。
//
// wiki world_绯红菌岩.txt#骨粉（:48-65）：
//   "类似于草方块，玩家可以对绯红菌岩使用骨粉。被使用的绯红菌岩、其附近的绯红菌岩和诡异菌岩
//    上方会生成绯红菌索和两种下界菌（多数是绯红菌）。"
//   生成细节：在菌岩上方一格水平偏移各2格内随机选取9次方块，按固定权重生成：
//     绯红菌索 87/99, 绯红菌 11/99, 诡异菌 1/99
//   如果目标位置为空气且可放置所选中的方块，则成功生成。
//
// wiki world_诡异菌岩.txt#骨粉：
//   "玩家可以对诡异菌岩使用骨粉。会生成两种菌索、两种下界菌（多数是诡异菌）和下界苗，
//    还有 1/8 的概率生成缠怨藤。"
//   生成细节：
//     WARPED_FOREST_VEGETATION_BONEMEAL：诡异菌索 85/100, 绯红菌索 1/100, 诡异菌 13/100, 绯红菌 1/100
//     NETHER_SPROUTS_BONEMEAL：在同样散布范围内放置下界苗
//     TWISTING_VINES_BONEMEAL：1/8 概率，在散布范围内放置缠怨藤
//
// wiki 退化（world_绯红菌岩.txt#用途 / world_诡异菌岩.txt#用途）：
//   "类似于草方块和菌丝体，当上方放有下表面遮挡形状完整的固体方块时菌岩会退化为下界岩
//    （基于随机刻）。但不同的是，当上方有水或熔岩时菌岩不会退化。"
//   JE 当雪覆盖在菌岩上时，菌岩也会退化。
//
// ============================ Cubium 实现链路 ============================
// NyliumBlock（nether/NyliumBlock.cpp）：
//   - randomTick（:46-55）：若 !_isDarkEnough → setBlockState(pos, NETHERRACK) 退化为下界岩。
//   - canGrow（:57-65）：菌岩上方须为空气才能用骨粉。
//   - canUseBonemeal（:67-76）：恒 true（菌岩骨粉 100% 有效）。
//   - grow（:78-113）：
//     绯红菌岩 → _placeNetherVegetation(Crimson)：9次散布，绯红菌索87/绯红菌11/诡异菌1
//     诡异菌岩 → _placeNetherVegetation(Warped) + _placeNetherSprouts + 1/8 _placeTwistingVines
//   - _placeNetherVegetation（:115-174）：spreadWidth=3, spreadHeight=1 → 9次散布尝试，
//     偏移 nextInt(3)-nextInt(3) 三角分布 [-2,2]，目标须为 air 且下方须为菌岩。
//   - _isDarkEnough（:269-278）：getLightBlockInto(上方方块) < MAX_LIGHT_LEVEL(15) → 足够暗。
//     上方空气 opacity=0 → lightBlockInto=max(1,0)=1 < 15 → 暗不退化；
//     上方完整方块 opacity=15 且 facesHaveOcclusion → lightBlockInto=16 >= 15 → 退化。
//
// BoneMealItem::onItemUse（src/common/item/items/special/BoneMealItem.cpp:61-123）：
//   - dynamic_cast<IGrowable> 取 NyliumBlock（实现 IGrowable）
//   - canGrow(上方须 air) → canUseBonemeal(恒 true) → grow(散布下界植被)
//   - grow 是同步 setBlockState（flags=3），useItemOnBlock 返回后即可读。
//
// SimulatedPlayer::useItemOnBlock（src/server/test/simulated/SimulatedPlayer.cpp:314-418）：
//   - Block.use 前置：NyliumBlock 未 override onBlockActivated，基类返 Pass → 放行
//   - Item.useOn fallback：BoneMealItem::onItemUse → grow → 返回 Success → useItemOnBlock 返 true
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃坑）============================
// glass_pit：y=0 glass 底，y=1..3 air 空腔，y=4 glass 顶。helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 结构内容从 origin+(0,1,0) 放置（placeOrigin），helper worldBlockPosition(rel)=origin+rel。
// 故相对 y=N 对应结构内 y=N-1。相对 y=0 是 glass 底层（结构内 y=0）。
//
// 散布命中概率分析：
//   _placeNetherVegetation 散布 origin 附近 [-2,2] 偏移，9 次尝试。
//   每次尝试检查 currentPos 须为 air 且 below 须为菌岩。
//   若仅单格菌岩，9 次中仅 dx=0,dz=0 命中（概率约 1/9/次），9 次至少命中1次概率≈65%——不够稳健。
//   解决：铺 3×3 菌岩区域（(2,1,2)~(4,1,2) 等9格），散布范围内 below 命中菌岩的概率大幅提升。
//   3×3 区域中心 (3,1,2)，散布 currentPos 落在 (1..5, 2, 0..4)，below 落在 (1..5, 1, 0..4)。
//   其中 below 为菌岩的区域是 (2..4, 1, 2..4) 共9格，散布命中概率≈9/25≈36%/次，9 次至少命中1次概率≈99%。
//
// 测试1 crimson_nylium_bonemeal_grows_vegetation（绯红菌岩骨粉生成植被）：
//   y=1 层铺 3×3 crimson_nylium（中心 (3,1,2)），上方 y=2 层为 air。
//   SimulatedPlayer 持骨粉对中心菌岩 (3,1,2) useItemOnBlock(Up) → grow 散布下界植被。
//   判定：5×5 air 层（y=2）内至少1格变 crimson_roots/crimson_fungus/warped_fungus。
//   绯红菌岩骨粉权重：绯红菌索 87/99 ≈ 87.9%，绯红菌 11/99 ≈ 11.1%，诡异菌 1/99 ≈ 1.0%。
//   9次散布每次独立加权选择，配合 3×3 菌岩区域，至少1次命中概率≈99%。
//
// 测试2 warped_nylium_bonemeal_grows_vegetation（诡异菌岩骨粉生成植被，含下界苗）：
//   y=1 层铺 3×3 warped_nylium（中心 (3,1,2)），上方 y=2 层为 air。
//   SimulatedPlayer 持骨粉对中心菌岩 (3,1,2) useItemOnBlock(Up) → grow 散布下界植被 + 下界苗 + 1/8 缠怨藤。
//   判定：5×5 air 层（y=2）内至少1格变骨粉结果方块。
//
// 测试3 nylium_decays_when_covered（菌岩被完整方块覆盖时退化为下界岩）：
//   (3,1,2) 放 crimson_nylium，正上方 (3,2,2) 放 stone（opacity=15 完整方块下表面遮挡）。
//   _isDarkEnough 判 lightBlockInto=16 >= 15 → 不够暗 → randomTick 退化成 netherrack。
//   调高 randomTickSpeed 后等待，断言 (3,1,2) 变 netherrack。
//
// ============================ 排除项（不写测试）============================
// - 缠怨藤生成（1/8 概率）：随机性强，单次 useItemOnBlock 命中概率仅 12.5%，验证价值低，跳过。
// - 雪覆盖退化（JE 雪覆盖菌岩退化）：需精确控制雪层 state，且 Cubium SnowBlock 雪层体系差异，跳过。
// - 水覆盖不退化：需放置水方块，且与退化逻辑交互复杂，跳过。
// - 末影人搬起/放下菌岩：需末影人 AI + mobGriefing，随机性强，跳过。
//
// ============================ 跨服务端对比 ============================
// - crimson_nylium/warped_nylium/netherrack typeId 两端一致（1.16 加入，1.21.11 已含）。
// - 骨粉生成下界植被行为两端一致（wiki 明文权重一致）。
// - 菌岩退化行为两端一致（wiki 明文"上方遮挡退化"一致）。
// - useItemOnBlock + getBlock typeId 判定为两端通用 API，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_绯红菌岩.txt#骨粉（:48-65 9次散布，权重87/11/1）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_诡异菌岩.txt#骨粉（权重85/1/13/1 + 下界苗 + 1/8缠怨藤）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_绯红菌岩.txt#用途（上方遮挡退化为下界岩）
// Ref: NyliumBlock.cpp:46-55（randomTick 退化）、:78-113（grow 散布植被）、:115-174（_placeNetherVegetation）
// Ref: NyliumBlock.cpp:269-278（_isDarkEnough 光照判定）
// Ref: BoneMealItem.cpp:61-123（onItemUse → canGrow/canUseBonemeal/grow 链路）
// Ref: BoneMealTests.ts（骨粉 useItemOnBlock 测试范式）、NetherRootsTests.ts（glass_pit 菌岩支撑测试范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 3×3 菌岩区域中心（y=1 层 air 空腔，glass_pit 内部）。
const NYLIUM_CENTER = { x: 3, y: 1, z: 2 };
// 中心菌岩正上方（y=2 层 air，骨粉散布起点）。
const ABOVE_CENTER = { x: 3, y: 2, z: 2 };

// 调高 randomTickSpeed 使菌岩格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。退化测试依赖 randomTick 触发退化逻辑。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 绯红菌岩骨粉可能生成的所有方块 typeId 集合（含跨菌岩的诡异菌）。
const CRIMSON_NYLIUM_BONEMEAL_RESULTS = new Set([
    "minecraft:crimson_roots",
    "minecraft:crimson_fungus",
    "minecraft:warped_fungus",
]);

// 诡异菌岩骨粉可能生成的所有方块 typeId 集合（含下界苗 + 跨菌岩的绯红系列）。
const WARPED_NYLIUM_BONEMEAL_RESULTS = new Set([
    "minecraft:warped_roots",
    "minecraft:warped_fungus",
    "minecraft:crimson_roots",
    "minecraft:crimson_fungus",
    "minecraft:nether_sprouts",
    "minecraft:twisting_vines",
    "minecraft:twisting_vines_plant",
]);

// 在 y=1 层铺设 3×3 菌岩区域（中心 (3,1,2)，覆盖 (2,1,2)~(4,1,2) 等九格）。
// 3×3 区域扩大 below 为菌岩的面积，使散布命中率从单格 ≈65% 提升至 ≈99%。
function placeNyliumArea(test: Test, nyliumType: string): void {
    for (let dx = -1; dx <= 1; ++dx) {
        for (let dz = -1; dz <= 1; ++dz) {
            test.setBlockType(nyliumType, { x: 3 + dx, y: 1, z: 2 + dz });
        }
    }
}

// 统计 5×5 air 层（y=2）内已生成的骨粉结果方块数。
// 散布偏移范围 [-2,2]，glass_pit 内部 5×5 air 空腔，实际有效散布格在 (1,2,1)~(5,2,5) 范围。
// 为稳健起见扫描整个 5×5 air 层（x,z∈[1,5], y=2）。
function countBonemealResults(
    test: Test,
    validTypes: Set<string>,
): number {
    let count = 0;
    for (let x = 1; x <= 5; ++x) {
        for (let z = 1; z <= 5; ++z) {
            const typeId = getTypeId(test, { x, y: 2, z });
            if (validTypes.has(typeId)) {
                ++count;
            }
        }
    }
    return count;
}

// 对绯红菌岩使用骨粉生成下界植被。
// 布局：y=1 层铺 3×3 crimson_nylium（中心 (3,1,2)），上方 y=2 层为 air（canGrow 检查通过）。
// SimulatedPlayer 持骨粉对中心菌岩 (3,1,2) useItemOnBlock(Up) → grow 散布下界植被。
function crimsonNyliumBonemealGrowsVegetation(test: Test): void {
    placeNyliumArea(test, "minecraft:crimson_nylium");
    test.assert(
        getTypeId(test, NYLIUM_CENTER) === "minecraft:crimson_nylium",
        `crimson_nylium should be at ${JSON.stringify(NYLIUM_CENTER)}, got ${getTypeId(test, NYLIUM_CENTER)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对绯红菌岩 useItemOnBlock 骨粉 → BoneMealItem::onItemUse → grow 散布下界植被。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        NYLIUM_CENTER,
        Direction.Up,
    );
    test.assert(used, `useItemOnBlock should return true when bonemealing crimson_nylium, got used=${used}`);

    // 判定：5×5 air 层（y=2）内至少1格变骨粉结果方块（绯红菌索/绯红菌/诡异菌）。
    pollUntilSucceed(
        test,
        () => countBonemealResults(test, CRIMSON_NYLIUM_BONEMEAL_RESULTS) >= 1,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                const count = countBonemealResults(test, CRIMSON_NYLIUM_BONEMEAL_RESULTS);
                test.assert(
                    false,
                    `crimson_nylium bonemeal: expected >=1 vegetation in 5x5 air layer, got ${count} ` +
                        `(nylium=${getTypeId(test, NYLIUM_CENTER)} above=${getTypeId(test, ABOVE_CENTER)}; ` +
                        `if 0, canGrow may falsely fail or grow may not spread vegetation)`,
                );
            },
        },
    );
}

// 对诡异菌岩使用骨粉生成下界植被（含下界苗）。
// 布局：y=1 层铺 3×3 warped_nylium（中心 (3,1,2)），上方 y=2 层为 air。
// SimulatedPlayer 持骨粉对中心菌岩 (3,1,2) useItemOnBlock(Up) → grow 散布下界植被 + 下界苗 + 1/8 缠怨藤。
function warpedNyliumBonemealGrowsVegetation(test: Test): void {
    placeNyliumArea(test, "minecraft:warped_nylium");
    test.assert(
        getTypeId(test, NYLIUM_CENTER) === "minecraft:warped_nylium",
        `warped_nylium should be at ${JSON.stringify(NYLIUM_CENTER)}, got ${getTypeId(test, NYLIUM_CENTER)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        NYLIUM_CENTER,
        Direction.Up,
    );
    test.assert(used, `useItemOnBlock should return true when bonemealing warped_nylium, got used=${used}`);

    // 判定：5×5 air 层（y=2）内至少1格变骨粉结果方块（诡异菌索/诡异菌/绯红菌索/绯红菌/下界苗/缠怨藤）。
    pollUntilSucceed(
        test,
        () => countBonemealResults(test, WARPED_NYLIUM_BONEMEAL_RESULTS) >= 1,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                const count = countBonemealResults(test, WARPED_NYLIUM_BONEMEAL_RESULTS);
                test.assert(
                    false,
                    `warped_nylium bonemeal: expected >=1 vegetation in 5x5 air layer, got ${count} ` +
                        `(nylium=${getTypeId(test, NYLIUM_CENTER)} above=${getTypeId(test, ABOVE_CENTER)}; ` +
                        `if 0, canGrow may falsely fail or grow may not spread vegetation)`,
                );
            },
        },
    );
}

// 菌岩被完整方块覆盖时退化为下界岩（wiki 退化：上方遮挡退化）。
// 布局：(3,1,2) 放 crimson_nylium，正上方 (3,2,2) 放 stone（opacity=15 完整方块下表面遮挡）。
// _isDarkEnough 判 lightBlockInto=16 >= 15 → 不够暗 → randomTick 退化成 netherrack。
function nyliumDecaysWhenCovered(test: Test): void {
    test.setBlockType("minecraft:crimson_nylium", NYLIUM_CENTER);
    test.setBlockType("minecraft:stone", ABOVE_CENTER);

    // 调高 randomTickSpeed 使菌岩格在数 tick 内被随机刻确定性命中。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);

    pollUntilSucceed(
        test,
        () => getTypeId(test, NYLIUM_CENTER) === "minecraft:netherrack",
        {
            startTick: 40,
            interval: 20,
            maxTick: 140,
            onTimeout: () => {
                const type = getTypeId(test, NYLIUM_CENTER);
                test.assert(
                    false,
                    `nylium decay when covered: expected crimson_nylium->netherrack, got ${type} ` +
                        `(above=${getTypeId(test, ABOVE_CENTER)} should be stone; ` +
                        `if still crimson_nylium, _isDarkEnough lightBlockInto>=15 check may not trigger decay)`,
                );
            },
        },
    );
}

export function registerNyliumTests(): void {
    GameTest.register("BlockBehaviorTests", "crimson_nylium_bonemeal_grows_vegetation", crimsonNyliumBonemealGrowsVegetation)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "warped_nylium_bonemeal_grows_vegetation", warpedNyliumBonemealGrowsVegetation)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "nylium_decays_when_covered", nyliumDecaysWhenCovered)
        .structureName("gametests:glass_pit")
        .maxTicks(180);
}
