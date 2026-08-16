// 动态发光方块等级测试：覆盖需特定 state 才发光的方块（对齐 wiki 发光方块表）。
//
// 与 BlockLightEmissionTests/ExtraEmissionTests 同设计：light_box 内部地板 (3,1,3) 放发光方块，断言 blockLight。
// 区别：本组方块需用 setBlockWithStates 设特定 state（lit=true / charges=N / candles=N）才发光。
//
// setBlockWithStates 是 Cubium 专有方法（基岩 BDS 无），states 字符串 "key=value,..."，按 Java 对齐的
// state 名（lit/charges/candles）。state 名取自 BlockStateProperties（LIT/CHARGES/CANDLES）的小写形式。
//
// 覆盖的发光方块（核查确认已实现且与 wiki 一致）：
//   15: campfire（默认 lit=true，无需设 state）/ respawn_anchor(charges=4)
//   13: furnace(lit=true)
//   12: candle(candles=4,lit=true)
//   10: soul_campfire（默认 lit=true）
//   9:  redstone_ore(lit=true)
//   1:  brewing_stand（恒发光1，无需 state）
//
// 未纳入（附着/环境依赖复杂，跳过）：
//   - cave_vines(berries=true 需朝下附着)、glow_lichen(需多面 state)、sea_pickle(需水中)、
//     nether_portal(需 axis+obsidian 框架)、fire(需附着面且可能燃尽)、soul_fire。
//   TODO: 这些带附着/环境依赖的发光方块测试待后续补充。
//
// 已知偏差/未实现（不写测试）：
//   - copper_bulb：Cubium lit 恒=15 不分氧化度（wiki 15/12/8/4），偏差。
//   - conduit/sculk_sensor/sculk_catalyst：注册但未实现发光，跳过。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const PLACE = { x: 3, y: 1, z: 3 };

/**
 * 放置发光方块并断言 blockLight 等于 expected。
 * @param test GameTest Test 对象
 * @param placeFn 放置回调（调 setBlockType 或 setBlockWithStates）
 * @param expected 期望发光等级 (0-15)
 * @param label 超时错误标签
 */
function assertEmission(
    test: Test,
    placeFn: () => void,
    expected: number,
    label: string,
): void {
    placeFn();
    pollUntilSucceed(test, () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === expected, {
        startTick: 5,
        interval: 4,
        maxTick: 100,
        onTimeout: () => {
            const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
            test.assert(
                false,
                `${label}: expected blockLight=${expected} at source, got ${actual} (state not set or not propagated?)`,
            );
        },
    });
}

// 营火默认 lit=true 发光15：CampfireBlock 默认 LIT=true（无 waterlogged 时），setBlockType 直接放即发光。
// 下方 stone 地板提供支撑（营火需下方支撑）。验证营火默认点燃状态发光15（对齐 wiki）。
function campfireEmitsFifteen(test: Test): void {
    assertEmission(test, () => test.setBlockType("minecraft:campfire", PLACE), 15, "campfire");
}

// 灵魂营火默认 lit=true 发光10：SoulCampfireBlock 继承 CampfireBlock，构造传 lightValue=10，默认 lit=true。
function soulCampfireEmitsTen(test: Test): void {
    assertEmission(test, () => test.setBlockType("minecraft:soul_campfire", PLACE), 10, "soul_campfire");
}

// 熔炉 lit=true 发光13：furnace 默认 lit=false 不发光，setBlockWithStates 设 lit=true 点燃，发光13。
// AbstractFurnaceBlock::getLightLevel 返回 LIT?13:0。验证动态发光（state 驱动发光等级切换）。
function furnaceLitEmitsThirteen(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:furnace", PLACE, "lit=true"),
        13,
        "furnace lit=true",
    );
}

// 红石矿石 lit=true 发光9：redstone_ore 默认 lit=false，setBlockWithStates 设 lit=true 点亮，发光9。
// RedstoneOreBlock::getLightLevel 返回 LIT?9:0。
function redstoneOreLitEmitsNine(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:redstone_ore", PLACE, "lit=true"),
        9,
        "redstone_ore lit=true",
    );
}

// 重生锚 charges=4 发光15：respawn_anchor 默认 charges=0 不发光，setBlockWithStates 设 charges=4，发光15。
// RespawnAnchorBlock::getLightLevel 返回 floor(charges*3.75)：0→0,1→3,2→7,3→11,4→15。
// 本测试验证满能量(4)发光15。
function respawnAnchorChargesFourEmitsFifteen(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:respawn_anchor", PLACE, "charges=4"),
        15,
        "respawn_anchor charges=4",
    );
}

// 蜡烛 candles=4 lit=true 发光12：candle 默认 candles=1 lit=false，setBlockWithStates 设 4个+点燃，发光12。
// CandleBlock::getLightLevel 返回 lit?3*count:0：4个点燃=12。
function candleFourLitEmitsTwelve(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:candle", PLACE, "candles=4,lit=true"),
        12,
        "candle candles=4 lit=true",
    );
}

// 酿造台恒发光1：BrewingStandBlock::getLightLevel 恒返回1（无需 state）。下方 stone 支撑。
function brewingStandEmitsOne(test: Test): void {
    assertEmission(test, () => test.setBlockType("minecraft:brewing_stand", PLACE), 1, "brewing_stand");
}

export function registerDynamicEmissionTests(): void {
    GameTest.register("LightingTests", "light_campfire_emits_15", campfireEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_soul_campfire_emits_10", soulCampfireEmitsTen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_furnace_lit_emits_13", furnaceLitEmitsThirteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_redstone_ore_lit_emits_9", redstoneOreLitEmitsNine)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_respawn_anchor_charges_4_emits_15", respawnAnchorChargesFourEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_candle_four_lit_emits_12", candleFourLitEmitsTwelve)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_brewing_stand_emits_1", brewingStandEmitsOne)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
