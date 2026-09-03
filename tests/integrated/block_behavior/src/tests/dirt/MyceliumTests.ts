// 菌丝体（mycelium）行为 GameTest：验证 SpreadableSnowyDirtBlock 的蔓延/退化（对齐 wiki 菌丝体章节）。
//
// wiki tech_菌丝体.txt#扩散（:48-56）：
//   "与草方块相同，菌丝体的扩散取决于随机刻。
//    要使泥土接受附近菌丝体的扩散，必须达到下列要求：
//    1. 以菌丝体下方的方块为扩散中心，被菌丝体扩散的泥土必须在该中心3×5×3的范围内。
//    2. 菌丝体的上方必须至少有9级的亮度。
//    3. 泥土上方不能被任何下表面遮挡形状完整的方块覆盖，但可以是一层高的雪。
//    4. 泥土上方不能是水或含水方块。"
// wiki tech_菌丝体.txt#转变（:59-62）：
//   "如果菌丝体直接被液体源方块或任何下表面遮挡形状完整的方块覆盖（一层高的雪除外），
//    菌丝体将会在随机时间后变为泥土。"
//   对着菌丝体使用锹可将其转变为土径，但菌丝体无法被锄犁成耕地，与灰化土相同。
//
// Cubium 实现（SpreadableSnowyDirtBlock.cpp:66-111 randomTick）：
//   - isSnowyConditions（:113-144）判退化：上方方块经 LightEngineUtils::getLightBlockInto 计算阻挡，
//     >=MAX_LIGHT_LEVEL(15) 即满阻挡 → isSnowyConditions 返回 false → 退化成泥土。
//   - 满足 isSnowyConditions 时取菌丝体上方光照 max(skyLight,blockLight)>=GRASS_SPREAD_LIGHT_THRESHOLD(9)
//     （:75-79）才尝试向4个随机位置（3×5×3 范围）的泥土蔓延（:83-108）。
//   - 蔓延目标须为 DIRT 且 isSnowyAndNotUnderwater（:146-165，再判 isSnowyConditions + 上方无流体）。
//
// Cubium 实现（HoeItem.cpp:177-202 _getTillingMap）：
//   - 耕地映射表仅含 GRASS_BLOCK/GRASS_PATH/DIRT→FARMLAND, COARSE_DIRT→DIRT。
//   - MYCELIUM 不在映射表中 → getTilledBlock 返回 nullptr → onItemUse 返 Pass（无法犁成耕地）。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 菌丝体蔓延由随机刻驱动（同草方块/蘑菇测试，见记忆 randomtick-threshold-test-via-gamerule-speedup）。
// 默认 randomTickSpeed=3 时菌丝体格每 tick 被选中概率仅 3/4096≈0.073%。测试开头用
// SimulatedPlayer.chat("/gamerule randomTickSpeed 1000") 调高使菌丝体格每 tick 命中概率≈24.4%，
// 120 tick 内至少命中一次概率≈100%。
//
// 蔓延期望：每次 randomTick 命中后向4个随机位置尝试蔓延，3×5×3=45 格范围内单格命中概率 4/45≈8.9%/次，
// 命中后周围泥土被蔓延概率→1。speed=1000 时菌丝体格每 tick 命中概率≈24.4%，多次命中后周围泥土被蔓延概率→1。
//
// 被遮挡退化为泥土：菌丝体正上方放 stone（opacity=15 完整方块），isSnowyConditions 判满阻挡返回 false →
//   randomTick 退化成 dirt（确定性逻辑，无概率）。
//
// 无法被锄犁成耕地：MYCELIUM 不在 _getTillingMap 中，锄耕返回 Pass（useItemOnBlock 返 false）。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒，skyLight=0）============================
// light_box 内部 x,z∈[1,5] y∈[1,5] air，y=0/y=6 stone。封顶隔绝天空光，blockLight 是唯一光源。
// (3,1,3) 菌丝体（setBlockType 强放存活，light_box 地板 stone，菌丝体在 stone 上非真实支撑但强放绕过
//   isValidPosition；SnowyDirtBlock::updatePostPlacement 只同步 SNOWY 状态无自毁，放置稳定）。
// 周围4格泥土 (2,1,3)/(4,1,3)/(3,1,2)/(3,1,4)（蔓延目标，3×5×3 范围内）。
//
// 测试1 mycelium_spreads_to_dirt_in_light（光照≥9 蔓延）：
//   菌丝体正上方 (3,2,3) 放 glowstone(15) 提供方块光，opacity=0 不触发退化，菌丝体上方光照=15≥9。
//   泥土上方距 glowstone 1格水平，光照衰减1→14≥9 且无遮挡。调高 randomTickSpeed 后等待足够 tick，
//   断言周围4格中至少1格变 mycelium（蔓延成功）。
//
// 测试2 mycelium_decays_when_covered（被遮挡退化为泥土）：
//   菌丝体正上方 (3,2,3) 放 stone（opacity=15 完整方块下表面遮挡），isSnowyConditions 判满阻挡返回
//   false → randomTick 退化成 dirt。调高 randomTickSpeed 后等待，断言 (3,1,3) 变 dirt（退化）。
//
// 测试3 mycelium_cannot_be_tilled_by_hoe（无法被锄犁成耕地）：
//   放菌丝体 (3,2,1) + 钻石锄 useItemOnBlock。MYCELIUM 不在 _getTillingMap 中，锄耕返 Pass
//   （useItemOnBlock 返 false）。判定：原位 (3,2,1) 仍为 mycelium（未变 farmland）+ useItemOnBlock 返 false。
//
// ============================ 排除项（不写测试）============================
// - 覆雪纹理同步（JE 被雪/雪块/细雪覆盖时纹理变覆雪）：纹理渲染不可由 GameTest 断言，且 SNOWY 状态
//   同步已由 PodzolTests 覆盖（podzol 与 mycelium 同走 SnowyDirtBlock::updatePostPlacement），跳过。
// - 末影人搬起/放下菌丝体：需末影人 AI + mobGriefing，随机性强且基岩行为不一致，跳过。
// - 大型云杉长成将下方菌丝体转灰化土：需树木生长特性，超本组范围，跳过。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat、blockLight/skyLight 在 Cubium 侧可用。基岩 BDS
//   SimulatedPlayer.chat 是发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧无法跑
//   （one-sided，同 WeatherSkyDarkeningTests.ts）。blockLight/skyLight Cubium 专有，黑暗/亮环境守卫仅
//   Cubium 侧判定。
// - 菌丝体/泥土 typeId 两端一致，蔓延（泥土→菌丝体）与退化（菌丝体→泥土）行为两端一致。
// - 菌丝体无法被锄犁成耕地：两端行为一致（MYCELIUM 不在锄耕地映射表中）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_菌丝体.txt#扩散（:48-56 3×5×3范围4次尝试，菌丝体上方光照≥9）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_菌丝体.txt#转变（:59-62 被遮挡退化成泥土）
// Ref: SpreadableSnowyDirtBlock.cpp:66-111（randomTick 蔓延/退化）、:113-144（isSnowyConditions 退化判定）
// Ref: HoeItem.cpp:177-202（_getTillingMap 耕地映射表，MYCELIUM 不在其中）
// Ref: GrassSpreadTests.ts（草方块蔓延测试范式，与本组同源）、HoeTillTests.ts（锄耕地测试范式）
// Ref: WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 菌丝体位置（light_box 运行时 air 区域 y=1 中心，下方 y=0 是 stone 地板）。
const MYCELIUM = { x: 3, y: 1, z: 3 };
// 蔓延目标（y=1 层 air，菌丝体周围4格泥土）。
const DIRT_NEIGHBORS = [
    { x: 2, y: 1, z: 3 },
    { x: 4, y: 1, z: 3 },
    { x: 3, y: 1, z: 2 },
    { x: 3, y: 1, z: 4 },
];
// 菌丝体正上方位置（放光源或遮挡方块）。
const ABOVE = { x: 3, y: 2, z: 3 };

// 调高 randomTickSpeed 使菌丝体格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。light_box 石墙隔离 + 内部仅 mycelium/dirt/glowstone/stone/air，
// randomTick 副作用可控。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId，缺失返回 undefined。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string | undefined {
    const block = test.getBlock(pos);
    return block?.typeId;
}

// 统计周围4格泥土中有多少已变 mycelium（蔓延成功数）。
function countSpreadMycelium(test: Test): number {
    let count = 0;
    for (const pos of DIRT_NEIGHBORS) {
        if (getTypeId(test, pos) === "minecraft:mycelium") {
            ++count;
        }
    }
    return count;
}

// 布置菌丝体 + 周围4格泥土（light_box 地板为 stone，强放绕过支撑检查）。
function placeMyceliumAndDirt(test: Test): void {
    test.setBlockType("minecraft:mycelium", MYCELIUM);
    for (const pos of DIRT_NEIGHBORS) {
        test.setBlockType("minecraft:dirt", pos);
    }
}

// 调高 randomTickSpeed（SimulatedPlayer 创造模式权限2 执行 /gamerule）。不 assert chat 返回值。
function raiseRandomTickSpeed(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);
}

// 光照≥9 时菌丝体向周围泥土蔓延（wiki 扩散：菌丝体上方光照≥9 才蔓延）。
// 菌丝体正上方放 glowstone(15) 提供方块光，opacity=0 不触发退化，菌丝体上方光照=15≥9。
// 调高 randomTickSpeed 后等待，断言周围4格中至少1格变 mycelium（蔓延成功）。
function myceliumSpreadsToDirtInLight(test: Test): void {
    placeMyceliumAndDirt(test);
    test.setBlockType("minecraft:glowstone", ABOVE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 蔓延成功：周围至少1格泥土变 mycelium。
            return countSpreadMycelium(test) >= 1;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                const count = countSpreadMycelium(test);
                test.assert(
                    false,
                    `mycelium spread in light: expected >=1 neighbor dirt->mycelium, got ${count}/4 ` +
                        `(mycelium=${getTypeId(test, MYCELIUM)} above=${getTypeId(test, ABOVE)}; ` +
                        `if 0, randomTickSpeed may not be raised or GRASS_SPREAD_LIGHT_THRESHOLD>=9 check missing)`,
                );
            },
        },
    );
}

// 被下表面遮挡形状完整的方块覆盖时菌丝体退化为泥土（wiki 转变：被遮挡退化）。
// 菌丝体正上方放 stone（opacity=15 完整方块），isSnowyConditions 判满阻挡返回 false → randomTick 退化。
// 调高 randomTickSpeed 后等待，断言 (3,1,3) 变 dirt（退化）。
function myceliumDecaysWhenCovered(test: Test): void {
    placeMyceliumAndDirt(test);
    test.setBlockType("minecraft:stone", ABOVE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 退化成功：菌丝体格变 dirt。
            return getTypeId(test, MYCELIUM) === "minecraft:dirt";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 120,
            onTimeout: () => {
                const type = getTypeId(test, MYCELIUM);
                test.assert(
                    false,
                    `mycelium decay when covered: expected mycelium->dirt, got ${type} ` +
                        `(above=${getTypeId(test, ABOVE)} should be stone; ` +
                        `if still mycelium, isSnowyConditions 满阻挡判定 may not trigger decay)`,
                );
            },
        },
    );
}

// 菌丝体无法被锄犁成耕地（wiki 转变：菌丝体无法被锄犁成耕地，与灰化土相同）。
// 布局：(3,2,1) 菌丝体，上方 (3,3,1) air。
// useItemOnBlock ① onBlockActivated（mycelium 基类 Pass）→ ② fallback HoeItem.onItemUse：
//   getTilledBlock(mycelium)=nullptr（MYCELIUM 不在 _getTillingMap 中）→ 返 Pass。
//
// 判定：useItemOnBlock 返 false（Pass），原位 (3,2,1) 仍为 mycelium（未变 farmland）。
function myceliumCannotBeTilledByHoe(test: Test): void {
    test.setBlockType("minecraft:mycelium", { x: 3, y: 2, z: 1 });
    test.assert(
        getTypeId(test, { x: 3, y: 2, z: 1 }) === "minecraft:mycelium",
        `mycelium should be at (3,2,1) before, got ${getTypeId(test, { x: 3, y: 2, z: 1 })}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const hoe = new ItemStack("minecraft:diamond_hoe", 1);

    // 对菌丝体 useItemOnBlock 钻石锄 → fallback HoeItem.onItemUse getTilledBlock(mycelium)=nullptr → Pass。
    const used = farmer.useItemOnBlock(
        hoe as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false when tilling mycelium with hoe (Pass), got used=${used}`);

    // 判定：原位 (3,2,1) 仍为 mycelium（未变 farmland）。
    test.assert(
        getTypeId(test, { x: 3, y: 2, z: 1 }) === "minecraft:mycelium",
        `mycelium should remain at (3,2,1) after hoe (cannot be tilled), got ${getTypeId(test, { x: 3, y: 2, z: 1 })}`,
    );

    test.succeed();
}

export function registerMyceliumTests(): void {
    GameTest.register("BlockBehaviorTests", "mycelium_spreads_to_dirt_in_light", myceliumSpreadsToDirtInLight)
        .structureName("gametests:light_box")
        .maxTicks(280);
    GameTest.register("BlockBehaviorTests", "mycelium_decays_when_covered", myceliumDecaysWhenCovered)
        .structureName("gametests:light_box")
        .maxTicks(200);
    GameTest.register("BlockBehaviorTests", "mycelium_cannot_be_tilled_by_hoe", myceliumCannotBeTilledByHoe)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
