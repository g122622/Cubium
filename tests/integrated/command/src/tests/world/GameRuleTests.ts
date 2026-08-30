// /gamerule 命令 GameTest：设置/查询游戏规则。
//
// 覆盖 wiki 命令章节核心行为：
//   - /gamerule <规则> <值>：设置规则值（Ref: wiki commands/gamerule.txt）
//   - /gamerule <规则>：查询规则当前值
//
// 设计要点：
//   1. GameRuleCommand 经 world->getGameRules().setFromString(ruleName, valueStr, nullptr)
//      作用于 ServerWorld 全局规则表（非占位），与玩家无关，对 SimulatedPlayer 完全生效。
//      此前脚本侧无 gamerule 读取绑定，/gamerule <规则> 查询消息经 source.sendMessage
//      不经脚本可读通道，无法端到端断言。本次补全 Dimension.getGameRule(name)（经新增
//      GameRules::getValueAsString 按名取值）脚本读取绑定，解锁端到端测试。
//   2. getGameRule 返回字符串：布尔规则 "true"/"false"，整数规则十进制串，规则不存在返空串。
//   3. 规则名是 camelCase（"mobGriefing"/"doMobSpawning"/"doDaylightCycle"/"randomTickSpeed"），
//      非 kebab-case，对齐 GameRuleKey::getName 注册名。
//   4. 世界级状态污染防护：GameTest 共享单一 ServerWorld，gamerule 跨测试/跨批次持久化。
//      测试临时改 gamerule 后须 runOnFinish 恢复默认值，避免污染同次全量跑里依赖该规则的
//      其他包测试（如 mob_behavior 的 spawner 测试依赖 doMobSpawning=true）。
//   5. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_gamerule.txt（游戏规则）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { asDim } from "../../utils/script/cubiumExtensions.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 默认值常量（与 GameRules.cpp 注册默认值一致）。
const DO_MOB_SPAWNING_DEFAULT = "true";
const RANDOM_TICK_SPEED_DEFAULT = "3";

// /gamerule doMobSpawning false 设置布尔规则为 false，断言 getGameRule 返 "false"。
// runOnFinish 恢复 "true"，防污染 mob_behavior spawner 类测试。
// Ref: wiki commands/gamerule.txt（gamerule <规则> <值>）
function gameruleSetBoolean(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/gamerule doMobSpawning false");

    const val = dim.getGameRule("doMobSpawning");
    test.assert(val === "false", `expected doMobSpawning="false", got "${val}"`);

    test.runOnFinish(() => {
        player.chat(`/gamerule doMobSpawning ${DO_MOB_SPAWNING_DEFAULT}`);
    });
    test.succeed();
}

// /gamerule randomTickSpeed 100 设置整数规则为 100，断言 getGameRule 返 "100"。
// runOnFinish 恢复 "3"。
// Ref: wiki commands/gamerule.txt（整数规则）
function gameruleSetInteger(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/gamerule randomTickSpeed 100");

    const val = dim.getGameRule("randomTickSpeed");
    test.assert(val === "100", `expected randomTickSpeed="100", got "${val}"`);

    test.runOnFinish(() => {
        player.chat(`/gamerule randomTickSpeed ${RANDOM_TICK_SPEED_DEFAULT}`);
    });
    test.succeed();
}

// 未显式设置的规则读默认值：doDaylightCycle 默认 "true"（不修改，仅读取断言）。
// 验证 getValueAsString 回退注册表默认值路径（规则已注册但实例未显式设值）。
function gameruleReadsDefault(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    const val = dim.getGameRule("doDaylightCycle");
    test.assert(val === "true", `expected doDaylightCycle default="true", got "${val}"`);
    test.succeed();
}

// 布尔规则往返：set false 再 set true，断言每次 getGameRule 反映最新值。
// 验证 setFromString 重复设置覆盖旧值（非仅首次生效）。
function gameruleToggleBack(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/gamerule doMobSpawning false");
    test.assert(dim.getGameRule("doMobSpawning") === "false", "after set false should be false");

    player.chat("/gamerule doMobSpawning true");
    test.assert(dim.getGameRule("doMobSpawning") === "true", "after set true should be true");

    // 未改默认值，无需 runOnFinish 恢复（已是 true）。
    test.succeed();
}

// 不存在的规则名返空串（getValueAsString 规则不存在返回空）。
function gameruleUnknownReturnsEmpty(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    const val = dim.getGameRule("nonexistentRule123");
    test.assert(val === "", `expected empty for unknown rule, got "${val}"`);
    test.succeed();
}

export function registerGameRuleTests(): void {
    // 改世界级 gamerule 的测试用独占 batch 串行（同批 name 隔离 default 批并行 tick）：
    // amethyst/mushroom/grass 等测试先 /gamerule randomTickSpeed 1000 再轮询蔓延，
    // 若本组 gamerule_set_integer 与其并行，runOnFinish 恢复 "3" 会把它们的 1000 覆盖
    // 致蔓延超时（全量跑 amethyst_bud_grows_to_next_stage / mushroom_spreads_in_dark 假失败）。
    GameTest.register("CommandTests", "gamerule_set_boolean", gameruleSetBoolean)
        .batch("gamerule")
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamerule_set_integer", gameruleSetInteger)
        .batch("gamerule")
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamerule_reads_default", gameruleReadsDefault)
        .batch("gamerule")
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamerule_toggle_back", gameruleToggleBack)
        .batch("gamerule")
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamerule_unknown_returns_empty", gameruleUnknownReturnsEmpty)
        .batch("gamerule")
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
