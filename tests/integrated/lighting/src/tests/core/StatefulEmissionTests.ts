// 带 state/附着/数量梯度的发光方块等级测试：覆盖 BlockLightEmissionTests（静态）/ExtraEmissionTests（紫水晶）/
// DynamicEmissionTests（单 state 点亮）未纳入的、需多面 state / 朝下附着 / 水中 waterlogged / 数量梯度 才发光的方块。
//
// 设计：light_box 内部地板 (3,1,3)（下方 (3,0,3)=stone 地板，四周与上方空气）放发光方块，
// 等光照传播稳定后断言该格 blockLight 等于发光等级。light_box 封顶实心隔绝天空光，blockLight 是唯一光源。
//
// setBlockWithStates 是 Cubium 专有（基岩 BDS 无），states 字符串 "key=value,..."，按 Java 对齐的 C++ 属性名
// （lit/berries/waterlogged/pickles/candles/charges/down）。setBlockWithStates 经 ServerWorld::setBlockState(flags=3)
// 写入，不调 isValidPosition/getStateForPlacement，故即使附着力严格的方块也能强制放置（但仍须满足 onBlockAdded/
// tick 不自毁——见各方块注释）。
//
// 本组覆盖（核查确认已实现且与 wiki 一致）：
//   6:  sculk_catalyst（静态 lightLevel(6)，注册处 SculkBlocks.cpp:85 已设，纠正注释误标「未实现」）
//   7:  redstone_torch（默认 lit=true，setBlockType 即发光，无支撑检查）/ glow_lichen（down=true 附地板，任一面 true→7）
//   3:  candle_cake（lit=true→3）
//   14: cave_vines / cave_vines_plant（berries=true→14，需顶部 stone 支撑，朝下生长植物）
//   6/9/12/15: sea_pickle（waterlogged=true + pickles=1/2/3/4，水中发光模型，陆地为 0）
//   3/6/9: candle 梯度补充（candles=1/2/3 lit=true，补 DynamicEmissionTests 仅测 candles=4=12 的缺口）
//   3/7/11: respawn_anchor 梯度补充（charges=1/2/3，补 DynamicEmissionTests 仅测 charges=4=15 的缺口）
//
// 已知偏差/未实现（不写测试）：
//   - copper_bulb：Cubium lit 恒=15 不分氧化度（wiki 15/12/8/4），偏差，跳过。
//   - sculk_sensor/sculk_shrieker：Cubium 未实现发光（实际=0），跳过。
//   - end_portal_frame：Cubium 静态 lightLevel(1) 恒发光，无眼也发光，与 wiki「有眼才发光」触发条件不符，偏差，跳过。
//   - redstone_lamp：实现一致(15/0)且 state 可写，但 onBlockAdded/neighborChanged 会按红石信号重算 lit，
//     light_box 内无红石源时 setBlockWithStates 设 lit=true 可能被改回 false，时序不稳定，跳过。
//   - fire/soul_fire：fire 放置时存活（下方 stone 满足 isValidPosition）但会随 tick 老化熄灭（stone 不可燃无火源），
//     非确定时序；soul_fire 需下方灵魂沙/灵魂土支撑（light_box 默认无），构造复杂。均跳过。
//   - nether_portal：需黑曜石传送门框架相邻（isValidPosition 强制），light_box 内单独放置会被 updatePostPlacement
//     改成空气，构造复杂，跳过。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块
// Ref: DynamicEmissionTests.ts（单 state 点亮范式：furnace/redstone_ore/respawn_anchor/candle/brewing_stand/campfire）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

// light_box 内部地板中心格（y=0 stone 地板之上，下方有支撑可放置需附着的方块）。
const PLACE = { x: 3, y: 1, z: 3 };

/**
 * 放置发光方块并断言 blockLight 等于 expected。
 * @param test GameTest Test 对象
 * @param placeFn 放置回调（调 setBlockType 或 setBlockWithStates，或先放支撑再放发光方块）
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

// sculk_catalyst 静态发光6：注册处 SculkBlocks.cpp:85 .lightLevel(6) 已设，无 state 依赖，setBlockType 直接放即发光。
// 此前 DynamicEmissionTests/ExtraEmissionTests 顶部注释误标「sculk_catalyst 注册但未实现发光」，实际已实现且与 wiki 6 一致。
// 本测试纠正该注释遗漏。SculkCatalystBlock 无 isValidPosition 检查（继承基类恒 true），放置存活。
function sculkCatalystEmitsSix(test: Test): void {
    assertEmission(test, () => test.setBlockType("minecraft:sculk_catalyst", PLACE), 6, "sculk_catalyst");
}

// 红石火把默认 lit=true 发光7：RedstoneTorchBlock 构造默认 LIT=true（RedstoneTorchBlock.cpp:67），setBlockType 直接放即发光。
// RedstoneTorchBlock 无 isValidPosition 检查（继承基类恒 true，区别于 RedstoneWallTorchBlock 需墙面支撑）。
// getLightLevel 返回 isLit?7:0。注意：redstone_torch 是立式火把（地上），redstone_wall_torch 需墙面（本测试不用 wall 变体）。
function redstoneTorchEmitsSeven(test: Test): void {
    assertEmission(test, () => test.setBlockType("minecraft:redstone_torch", PLACE), 7, "redstone_torch");
}

// 蛋糕蜡烛 lit=true 发光3：CandleCakeBlock 默认 lit=false，setBlockWithStates 设 lit=true 点燃，发光3。
// CandleCakeBlock::getLightLevel 返回 lit?3:0。isValidPosition 查下方 isSolid（(3,0,3)=stone 满足），放置存活。
function candleCakeLitEmitsThree(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:candle_cake", PLACE, "lit=true"),
        3,
        "candle_cake lit=true",
    );
}

// 发光地衣 down=true 发光7：GlowLichenBlock 是 MultifaceBlock 子类，六个布尔面 state（north/south/east/west/up/down）。
// down=true 表示地衣附在下方方块顶面（(3,1,3) 的 down 面贴 (3,0,3)=stone 顶面，附着有效）。
// getLightLevel 任一面 true→7。setBlockWithStates 设 down=true 强制放置（不经 isValidPosition，但 down 面贴 stone 顶面
// 本就是有效附着，符合 vanilla 语义）。注意：up=true 会附在空气上（无效），须用 down=true 附地板。
function glowLichenDownEmitsSeven(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:glow_lichen", PLACE, "down=true"),
        7,
        "glow_lichen down=true",
    );
}

// 洞穴藤蔓（头）berries=true 发光14：CaveVinesBlock 是 GrowingPlantHeadBlock 子类，生长方向 Down（朝下生长），
// 故支撑方向 Up（需顶部附着）。placeFn 先在 (3,2,3) 放 stone 作顶部支撑，再在 (3,1,3) 放 cave_vines（berries=true）。
// GrowingPlantBlock::isValidPosition 查 opposite(growthDirection)=Up 方向 (3,2,3) 须同类藤蔓或 solid 面——放 stone 满足。
// getLightLevel 返回 berries?14:0。setBlockWithStates 设 berries=true，顶部 stone 支撑使其存活。
function caveVinesBerriesEmitsFourteen(test: Test): void {
    assertEmission(
        test,
        () => {
            // 先放顶部支撑（cave_vines 朝下生长，需上方 solid 面）。
            test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });
            // 再放 cave_vines（berries=true 发光14），下方 (3,1,3) 空气（藤蔓悬空，靠顶部支撑）。
            test.setBlockWithStates("minecraft:cave_vines", PLACE, "berries=true");
        },
        14,
        "cave_vines berries=true",
    );
}

// 洞穴藤蔓（身）berries=true 发光14：CaveVinesPlantBlock 是 GrowingPlantBodyBlock 子类，同 CaveVines 顶部附着。
// 与 cave_vines（头）区别：cave_vines 是藤蔓顶端（可生长），cave_vines_plant 是身段。两者发光逻辑相同（berries?14:0）。
// 同 cave_vines 需顶部 stone 支撑。
function caveVinesPlantBerriesEmitsFourteen(test: Test): void {
    assertEmission(
        test,
        () => {
            test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });
            test.setBlockWithStates("minecraft:cave_vines_plant", PLACE, "berries=true");
        },
        14,
        "cave_vines_plant berries=true",
    );
}

// 海泡菜水中发光梯度：sea_pickle waterlogged=true 时按 pickles 数量 1/2/3/4 对应 6/9/12/15，waterlogged=false 时 0。
// SeaPickleBlock::getLightLevel 返回 !waterlogged?0 : 3+count*3。isValidPosition 查下方 isSolid（stone 满足）。
// light_box 内是空气，但 waterlogged=true 是 state 强制写入（不要求周围真有水，方块仍存活），符合「海泡菜陆地可存在但水中才发光」语义。
// 测 pickles=1（6）+ pickles=4（15）两级验证梯度两端（中间 2/3 由线性公式保证，测两端足够）。
function seaPickleWaterloggedOneEmitsSix(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:sea_pickle", PLACE, "pickles=1,waterlogged=true"),
        6,
        "sea_pickle pickles=1 waterlogged=true",
    );
}

function seaPickleWaterloggedFourEmitsFifteen(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:sea_pickle", PLACE, "pickles=4,waterlogged=true"),
        15,
        "sea_pickle pickles=4 waterlogged=true",
    );
}

// 海泡菜陆地不发光：sea_pickle waterlogged=false 时 getLightLevel 返回 0（陆地无光）。
// 验证 waterlogged 门控发光（陆地海泡菜 blockLight=0，区别于水中发光）。
function seaPickleDryEmitsZero(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:sea_pickle", PLACE, "pickles=4,waterlogged=false"),
        0,
        "sea_pickle pickles=4 waterlogged=false (dry, no light)",
    );
}

// 蜡烛数量梯度补充：candle lit=true 时按 candles 数量 1/2/3/4 对应 3/6/9/12。
// DynamicEmissionTests 仅测 candles=4=12，本组补 candles=1/2/3（3/6/9）验证线性公式 3*count。
// CandleBlock::getLightLevel 返回 lit?3*count:0。isValidPosition 查下方 isSolid（stone 满足）。
// 测 candles=1（3）+ candles=3（9）两级验证梯度（candles=2=6 由线性公式保证）。
function candleOneLitEmitsThree(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:candle", PLACE, "candles=1,lit=true"),
        3,
        "candle candles=1 lit=true",
    );
}

function candleThreeLitEmitsNine(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:candle", PLACE, "candles=3,lit=true"),
        9,
        "candle candles=3 lit=true",
    );
}

// 重生锚能量梯度补充：respawn_anchor 按 charges 0/1/2/3/4 对应 0/3/7/11/15。
// DynamicEmissionTests 仅测 charges=4=15，本组补 charges=1/2/3（3/7/11）验证 floor(charges*3.75) 公式。
// RespawnAnchorBlock::getLightLevel 返回 floor(charges*3.75)：1→3, 2→7, 3→11。
// 测 charges=1（3）+ charges=3（11）两级验证梯度（charges=2=7 由公式保证）。
function respawnAnchorChargesOneEmitsThree(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:respawn_anchor", PLACE, "charges=1"),
        3,
        "respawn_anchor charges=1",
    );
}

function respawnAnchorChargesThreeEmitsEleven(test: Test): void {
    assertEmission(
        test,
        () => test.setBlockWithStates("minecraft:respawn_anchor", PLACE, "charges=3"),
        11,
        "respawn_anchor charges=3",
    );
}

export function registerStatefulEmissionTests(): void {
    GameTest.register("LightingTests", "light_sculk_catalyst_emits_6", sculkCatalystEmitsSix)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_redstone_torch_emits_7", redstoneTorchEmitsSeven)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_candle_cake_lit_emits_3", candleCakeLitEmitsThree)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_glow_lichen_down_emits_7", glowLichenDownEmitsSeven)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_cave_vines_berries_emits_14", caveVinesBerriesEmitsFourteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_cave_vines_plant_berries_emits_14", caveVinesPlantBerriesEmitsFourteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_sea_pickle_waterlogged_1_emits_6", seaPickleWaterloggedOneEmitsSix)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_sea_pickle_waterlogged_4_emits_15", seaPickleWaterloggedFourEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_sea_pickle_dry_emits_0", seaPickleDryEmitsZero)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_candle_one_lit_emits_3", candleOneLitEmitsThree)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_candle_three_lit_emits_9", candleThreeLitEmitsNine)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_respawn_anchor_charges_1_emits_3", respawnAnchorChargesOneEmitsThree)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_respawn_anchor_charges_3_emits_11", respawnAnchorChargesThreeEmitsEleven)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
