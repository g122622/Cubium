// 铜氧化集成测试：验证 IOxidizableBlock::tryOxidize 的 randomTick 氧化进阶、邻居抑制、最高级不氧化
// （对齐 wiki 铜块氧化章节与 Cubium 氧化算法实现）。
//
// wiki other_铜块.txt#氧化（:80-82）："铜块受氧化机制影响，能够在不同变种之间转化。"
// wiki other_铜块.txt 历史（:239/:274）："铜块的锈蚀现在基于随机刻，而不是先前的计划刻。"
// 氧化链：copper_block(Unaffected) → exposed_copper(Exposed) → weathered_copper(Weathered)
//         → oxidized_copper(Oxidized)（CopperBlocks.cpp:304-317 注册并设置氧化链）。
//
// Cubium 实现（IOxidizableBlock.cpp:37-134 tryOxidize，由 WeatheringCopperBlock::randomTick 调用）：
//   - 已是最高氧化等级（Oxidized）→ 直接返回 false 不处理（:40-42）。
//   - 外层门限概率 OXIDATION_GATE_CHANCE=0.05688889（约5.69%），未过门限直接返回（:51-54）。
//   - 扫描曼哈顿距离4内邻居（:63-114）：存在更低氧化等级邻居 → hasLowerAgeNeighbor=true 立即取消（:95-119）。
//   - 最终概率 f1 = ((k+1)/(k+j+1))^2 * chanceModifier（k=更高等级邻居数, j=同等级邻居数）。
//     孤立方块 j=k=0 → f=1 → f1=1.0（Exposed/Weathered，modifier=1.0）。Unaffected modifier=0.75（:79）。
//   - 通过概率则 withPropertiesOf 保留属性替换为下一等级方块（:127-130）。
// getOxidationChanceModifier：Unaffected=0.75，Exposed/Weathered=1.0（IOxidizableBlock.hpp:77-80）。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 铜氧化由随机刻驱动（同作物光照门槛、草蔓延测试，见记忆 randomtick-threshold-test-via-gamerule-speedup）。
// 默认 randomTickSpeed=3 时单格每 tick 被选中概率仅 3/4096≈0.073%，氧化无从验证。测试开头用
// SimulatedPlayer.chat("/gamerule randomTickSpeed 1000") 调高使铜方块格每 tick 命中概率≈24.4%。
//
// 孤立 Exposed 铜块氧化：单次 randomTick 命中后门限5.69% × 内层100% = 5.69%/命中。speed=1000 时
// 每 tick 命中概率24.4%，氧化期望≈1.39%/tick，300 tick 内氧化概率≈98.5%（1-(1-0.0139)^300）。
// 配 maxTick 320 + 轮询，确定性足够（概率事件转高概率）。若 flaky 可进一步调高 speed。
//
// 邻居抑制（测试2）与最高级不氧化（测试3）是确定性逻辑：邻居抑制命中即取消（无概率），最高级
// randomTick 直接 return。调高 randomTickSpeed 后多次命中都维持不变，断言"不变"确定性成立。
//
// MinecraftStructurePlacer 为测试结构区域加 forced chunk ticket，chunk 常驻，randomTick 覆盖铜方块格。
//
// chat 返回值不 assert：Cubium chat 返回 int，基岩 BDS chat 返回 void（发消息语义不执行命令），两端
// chat 语义不同，用 chat 执行 /gamerule 的测试基岩侧 one-sided（同 WeatherSkyDarkeningTests）。
// 测试1（孤立方块氧化）证明 randomTickSpeed 调高生效，反证测试2/3的"不变"是抑制/最高级逻辑而非没命中。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒）============================
// light_box 内部 x,z∈[1,5] y∈[1,5] air。铜方块无 isValidPosition/canSurvive/updatePostPlacement 重写
// （WeatheringCopperBlock 仅 randomTick），放置稳定，无支撑/光照依赖，可强放任意位置。
// 用 light_box 而非 glass_pit：light_box 封顶隔绝天空光 + 石墙隔离，避免外部世界生成铜方块/植被污染
// 邻居扫描（tryOxidize 扫描曼哈顿距离4，需保证测试铜方块4格内无非测试铜方块）。
//
// 测试1 exposed_copper_oxidizes_to_weathered（孤立方块氧化进阶）：
//   (3,1,3) 孤立 exposed_copper（周围4格内无其他铜方块），调高 randomTickSpeed 后等待，断言变
//   weathered_copper（Exposed→Weathered 进阶）。验证 randomTick 驱动氧化 + 概率门限通过后进阶。
//   轮询用 interval=1 每 tick 查（非默认 20），因 speed=1000 下 weathered 中间态窗口短，稀疏检查点
//   偶发跳过致假失败（详见测试函数注释）。
//
// 测试2 lower_grade_neighbor_prevents_oxidation（更低等级邻居抑制氧化）：
//   (3,1,3) exposed_copper + (4,1,3) copper_block（Unaffected，更低等级）。exposed randomTick 时
//   tryOxidize 扫描到更低等级邻居 copper_block → hasLowerAgeNeighbor=true 立即取消，exposed 不氧化。
//   调高 randomTickSpeed 后等待较短时间，断言 (3,1,3) 仍 exposed_copper（抑制生效）。
//   配对测试1：测试1孤立方块氧化成功证明 randomTickSpeed 调高生效，反证测试2的 exposed 不变是邻居
//   抑制逻辑而非没命中。
//
//   竞态说明：邻居 copper_block（Unaffected）自身也会氧化（5.69%门限 × 0.75 modifier，比 exposed 更慢）。
//   若 copper_block 先氧化为 exposed，则两侧同级，exposed 失去更低等级邻居抑制可能在后续命中氧化。
//   为规避此竞态，maxTick 设较短（100）：speed=1000 时 100 tick 内 exposed 命中 randomTick 约24次（每次
//   被更低等级邻居抑制，断言成立），而 copper_block 在 100 tick 内氧化概率仅约6%（1-(1-0.244×0.0569×
//   0.75)^60），远低于 exposed 命中次数，竞态概率低。若仍 flaky 可进一步缩短 maxTick 或调低 speed。
//   注：涂蜡铜（waxed_copper_block）虽不氧化但非 IOxidizableBlock（WaxedCopperBlock: public Block 不继承
//   IOxidizableBlock），tryOxidize 扫描 dynamic_cast 返回 nullptr 跳过，不能作更低等级邻居，故只能用
//   未涂蜡 copper_block。
//
// 测试3 oxidized_copper_does_not_oxidize（最高级不氧化）：
//   (3,1,3) oxidized_copper（最高氧化等级），randomTick 时 tryOxidize 直接返回 false（:40-42）。
//   调高 randomTickSpeed 后等待，断言 (3,1,3) 仍 oxidized_copper（不进阶，无下一级方块）。
//   注意：oxidized_copper 构造时 m_ticksRandomly=false（WeatheringCopperBlock.cpp:39-41 仅非 Oxidized
//   才设 ticksRandomly），故 oxidized 不响应 randomTick。断言"不变"对未响应 randomTick 与响应但
//   返回 false 两种实现都成立（行为等价：最高级稳定不变）。
//
// ============================ 排除项（不写测试）============================
// - 雷击除锈（闪电击中减轻锈蚀）：需召唤闪电实体 + 命中精确位置，闪电机制本身复杂且 Cubium 闪电实体
//   实现状态未核实，按准则不为未核实行为写测试，跳过。TODO: 待核实 Cubium 闪电除锈实现后补充。
// - 涂蜡方块不氧化：CopperWaxTests.ts 已覆盖涂蜡/除蜡，涂蜡方块不响应氧化（WaxedCopperBlock 非
//   IOxidizableBlock），但与涂蜡测试重叠，跳过。
// - Unaffected→Exposed 氧化（copper_block 起始）：Unaffected modifier=0.75 氧化更慢，且 exposed 起始
//   的测试1已覆盖氧化进阶核心机制，copper_block 起始仅 modifier 差异，价值有限，跳过。
// - 邻居减慢（同等级/更高等级邻居降低概率）：概率精细计算，确定性难验证，核心"邻居抑制"已由测试2
//   覆盖，跳过。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat 在 Cubium 侧可用。基岩 BDS SimulatedPlayer.chat 是
//   发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧无法跑（one-sided，同
//   WeatherSkyDarkeningTests.ts）。
// - 铜方块 typeId（exposed_copper/weathered_copper/oxidized_copper/copper_block）两端一致，氧化进阶
//   与邻居抑制行为两端一致（randomTick 驱动氧化是 vanilla 行为）。
// - 测试1（概率氧化）基岩侧即使能跑也受基岩 randomTick 概率 + 5.69%门限影响可能 flaky，但本组基岩侧
//   因 chat 语义不可执行命令已 one-sided，无需对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_铜块.txt#氧化（:80-82 铜块受氧化机制影响）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_铜块.txt 历史（:239/:274 氧化基于随机刻）
// Ref: IOxidizableBlock.cpp:37-134（tryOxidize 门限5.69% + 邻居扫描 + 概率公式 + 进阶替换）
// Ref: IOxidizableBlock.hpp:77-80（getOxidationChanceModifier Unaffected=0.75 其余=1.0）
// Ref: WeatheringCopperBlock.cpp:33-47（构造仅非 Oxidized 设 ticksRandomly，randomTick 调 tryOxidize）
// Ref: CopperBlocks.cpp:304-317（copper_block/exposed/weathered/oxidized 注册 + 氧化链设置）
// Ref: CropLightThresholdTests.ts / GrassSpreadTests.ts（randomTickSpeed 调高确定性方案范式）
// Ref: WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 测试铜方块位置（light_box 内部）。
const COPPER = { x: 3, y: 1, z: 3 };
// 测试2 中作为更低等级邻居的 copper_block（Unaffected）位置（与 COPPER 水平相邻1格，曼哈顿距离1<4）。
const NEIGHBOR = { x: 4, y: 1, z: 3 };

// 调高 randomTickSpeed 使铜方块格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。孤立方块氧化期望≈1.39%/tick，300 tick 内氧化概率≈98.5%。
// light_box 石墙隔离 + 内部仅铜方块/stone/air，规避外部铜方块污染邻居扫描。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId，缺失返回 undefined。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string | undefined {
    const block = test.getBlock(pos);
    return block?.typeId;
}

// 调高 randomTickSpeed（SimulatedPlayer 创造模式权限2 执行 /gamerule）。不 assert chat 返回值。
function raiseRandomTickSpeed(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);
}

// 孤立 exposed_copper 在随机刻下氧化进阶为 weathered_copper（wiki 氧化：铜块受氧化机制影响，基于随机刻）。
// (3,1,3) 孤立 exposed_copper（周围4格内无其他铜方块），调高 randomTickSpeed 后等待，断言变 weathered_copper。
//
// 轮询密度选 interval=1（每 tick 查）而非默认 20，原因：speed=1000 下 exposed→weathered 与 weathered→oxidized
// 两级氧化都很快（每次期望约 82 tick）。weathered 中间态窗口约 82 tick，但若用 interval=20 的稀疏检查点
// [40,60,80,...]，全量并行环境下服务器 tick 调度抖动偶发使 exposed→weathered 与 weathered→oxidized 都落在
// 同一检查点间隔内（两个检查点之间），导致检查点恰好跳过 weathered 中间态直接看到 oxidized_copper 假失败。
// randomTick 机制保证 weathered 状态至少持续 1 个完整 tick（每格每 tick 至多一次 randomTick：tick N 做
// exposed→weathered 后，weathered→oxidized 最早在 tick N+1 的下一个 randomTick），故每 tick 查必能抓到
// weathered 中间态，彻底消除 timing 依赖。预注册 [40..360] 每 tick 一个 runAtTickTime 检查点（约 321 个），
// 开销可接受。
function exposedCopperOxidizesToWeathered(test: Test): void {
    test.setBlockType("minecraft:exposed_copper", COPPER);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 氧化进阶：exposed_copper → weathered_copper。每 tick 查以抓 weathered 中间态。
            return getTypeId(test, COPPER) === "minecraft:weathered_copper";
        },
        {
            startTick: 40,
            interval: 1,
            maxTick: 360,
            onTimeout: () => {
                const type = getTypeId(test, COPPER);
                test.assert(
                    false,
                    `exposed_copper oxidation: expected weathered_copper, got ${type} ` +
                        `(if still exposed_copper, randomTickSpeed may not be raised or ` +
                        `oxidation gate 5.69% chance too low — consider higher speed or longer maxTicks; ` +
                        `if oxidized_copper, weathered intermediate was skipped — interval=1 per-tick poll ` +
                        `should prevent this, investigate randomTick multi-step-per-tick)`,
                );
            },
        },
    );
}

// 更低氧化等级的邻居抑制氧化（wiki 氧化算法：存在更低等级邻居则取消氧化）。
// (3,1,3) exposed_copper + (4,1,3) copper_block（Unaffected 更低等级）。exposed randomTick 时扫描到更低等级
// 邻居 → 立即取消氧化。调高 randomTickSpeed 后等待较短时间，断言 (3,1,3) 仍 exposed_copper（抑制生效）。
// 竞态与 maxTick 取值见文件头测试2设计说明。
function lowerGradeNeighborPreventsOxidation(test: Test): void {
    test.setBlockType("minecraft:exposed_copper", COPPER);
    test.setBlockType("minecraft:copper_block", NEIGHBOR);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 邻居抑制生效：exposed_copper 不氧化（更低等级邻居 copper_block 抑制）。
            return getTypeId(test, COPPER) === "minecraft:exposed_copper";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 100,
            onTimeout: () => {
                const type = getTypeId(test, COPPER);
                const neighborType = getTypeId(test, NEIGHBOR);
                test.assert(
                    false,
                    `lower-grade neighbor prevents oxidation: expected exposed_copper intact, got ${type} ` +
                        `(neighbor=${neighborType}; if exposed_copper changed, ` +
                        `neighbor inhibition may not trigger — tryOxidize hasLowerAgeNeighbor logic issue, ` +
                        `or neighbor copper_block oxidized to exposed first removing the lower-grade neighbor)`,
                );
            },
        },
    );
}

// 最高氧化等级 oxidized_copper 不再氧化（wiki 氧化：已是最高级则无法继续氧化）。
// (3,1,3) oxidized_copper（最高等级），randomTick 时 tryOxidize 直接返回 false。调高 randomTickSpeed 后
// 等待，断言 (3,1,3) 仍 oxidized_copper（不进阶）。oxidized 构造时 ticksRandomly=false 不响应 randomTick，
// 断言"不变"对两种实现都成立。
function oxidizedCopperDoesNotOxidize(test: Test): void {
    test.setBlockType("minecraft:oxidized_copper", COPPER);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 最高级不氧化：oxidized_copper 不进阶（无下一级方块，且不响应/不通过 randomTick）。
            return getTypeId(test, COPPER) === "minecraft:oxidized_copper";
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 120,
            onTimeout: () => {
                const type = getTypeId(test, COPPER);
                test.assert(
                    false,
                    `oxidized_copper should not oxidize further: expected oxidized_copper, got ${type} ` +
                        `(if changed, oxidized_copper may incorrectly respond to randomTick or have a next oxidation block)`,
                );
            },
        },
    );
}

export function registerCopperOxidationTests(): void {
    GameTest.register("BlockBehaviorTests", "exposed_copper_oxidizes_to_weathered", exposedCopperOxidizesToWeathered)
        .structureName("gametests:light_box")
        .maxTicks(420);
    GameTest.register(
        "BlockBehaviorTests",
        "lower_grade_neighbor_prevents_oxidation",
        lowerGradeNeighborPreventsOxidation,
    )
        .structureName("gametests:light_box")
        .maxTicks(180);
    GameTest.register("BlockBehaviorTests", "oxidized_copper_does_not_oxidize", oxidizedCopperDoesNotOxidize)
        .structureName("gametests:light_box")
        .maxTicks(200);
}
