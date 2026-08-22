// /trigger 命令 GameTest：触发器记分板目标。
//
// 覆盖 wiki 命令章节核心行为（对齐 MC 1.21.11 TriggerCommand，Ref: Java
// net/minecraft/server/commands/TriggerCommand.java）：
//   - /trigger <objective>：对该 trigger 准则目标自增 1（玩家自己的分数）
//   - /trigger <objective> add <value>：自增指定值
//   - /trigger <objective> set <value>：设置为指定值
//   - 用后锁定：trigger 一旦使用，该玩家的分数被锁定，再次 /trigger 失败（需 /scoreboard players enable 重新启用）
//   - 未 enable 不可用：玩家须先被 /scoreboard players enable prime（创建分数并解锁）才能 /trigger
//
// 设计要点：
//   1. trigger 命令权限=0（所有玩家可用，含 OP）。SimulatedPlayer permLevel=4 亦可执行。
//   2. trigger 准则目标须先 /scoreboard objectives add <name> trigger 创建。
//   3. 玩家须先 /scoreboard players enable <player> <obj> prime（getOrCreateScore + setLocked(false)）。
//      trigger 命令用 source.name()（=player->username()="op"）查 entityHasObjective，故 enable 的 target
//      必须用 player.name（@s 不生效，ScoreboardCommand target 是裸字符串非选择器，见
//      [[scoreboard-script-binding-and-command-tests]]）。
//   4. trigger 命令 source.name() 与 enable 的 player.name 必须一致（都是 username "op"），否则
//      entityHasObjective 查不到分数致 "not primed" 失败。
//   5. 断言用 world.scoreboard.getObjective(name).getScore(participant) 读分数（复用 ScoreboardTests 范式）。
//   6. 记分板 objective 是世界级单例，跨测试持久化。用唯一 objective 名 + runOnFinish remove 防污染。
//   7. 锁定测试：enable 后 /trigger（分数=1，锁定），再 /trigger 应失败（checkValidTrigger 检查 isLocked
//      返 nullptr，分数不变仍=1）。
//   8. 未 enable 测试：创建 trigger 目标后不 enable，直接 /trigger 应失败（entityHasObjective=false，
//      分数 undefined）。
//   9. cmd_arena 9×7×9：玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_触发.txt（触发命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// Cubium 扩展的 Scoreboard/Objective 接口（world.scoreboard 经 as 断言，官方类型无此属性）。
interface CubiumObjective {
    getScore(participant: string): number | undefined;
    getParticipants(): string[];
}
interface CubiumScoreboard {
    getObjective(name: string): CubiumObjective | undefined;
    getObjectives(): CubiumObjective[];
}

function getScoreboard(): CubiumScoreboard {
    return (world as unknown as { scoreboard: CubiumScoreboard }).scoreboard;
}

// 读玩家分数（player.name 做 participant）。
function getScore(objName: string, playerName: string): number | undefined {
    const obj = getScoreboard().getObjective(objName);
    if (obj === undefined) {
        return undefined;
    }
    return obj.getScore(playerName);
}

// /trigger <objective> 自增 1：enable 后 /trigger foo，断言分数=1。
// Ref: Java TriggerCommand（trigger 简单调用，incrementScore）
function triggerAddsOne(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "trig_one";

    player.chat(`/scoreboard objectives add ${objName} trigger`);
    player.chat(`/scoreboard players enable ${player.name} ${objName}`);
    player.chat(`/trigger ${objName}`);

    const score = getScore(objName, player.name);
    test.assert(score === 1, `score should be 1 after /trigger, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /trigger <objective> add <value> 自增指定值：enable 后 add 5，断言分数=5。
// Ref: Java TriggerCommand（trigger add，addScore）
function triggerAddValue(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "trig_add";

    player.chat(`/scoreboard objectives add ${objName} trigger`);
    player.chat(`/scoreboard players enable ${player.name} ${objName}`);
    player.chat(`/trigger ${objName} add 5`);

    const score = getScore(objName, player.name);
    test.assert(score === 5, `score should be 5 after add 5, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /trigger <objective> set <value> 设置指定值：enable 后 set 10，断言分数=10。
// Ref: Java TriggerCommand（trigger set，setScorePoints）
function triggerSetValue(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "trig_set";

    player.chat(`/scoreboard objectives add ${objName} trigger`);
    player.chat(`/scoreboard players enable ${player.name} ${objName}`);
    player.chat(`/trigger ${objName} set 10`);

    const score = getScore(objName, player.name);
    test.assert(score === 10, `score should be 10 after set 10, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /trigger 用后锁定：enable 后 /trigger（分数=1，锁定），再 /trigger 应失败（分数仍=1）。
// 验证 lockTriggerScore(setLocked(true)) + checkValidTrigger(isLocked 检查)。
// Ref: Java TriggerCommand（用后 setLocked，再次触发 checkValidTrigger 拒绝）
function triggerLockedAfterUse(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "trig_lock";

    player.chat(`/scoreboard objectives add ${objName} trigger`);
    player.chat(`/scoreboard players enable ${player.name} ${objName}`);
    player.chat(`/trigger ${objName}`);

    let score = getScore(objName, player.name);
    test.assert(score === 1, `score should be 1 after first /trigger, got ${score}`);

    // 再次 /trigger 应因锁定失败，分数不变仍=1。
    player.chat(`/trigger ${objName}`);
    score = getScore(objName, player.name);
    test.assert(score === 1, `locked trigger should not change score, still 1, got ${score}`);

    // 重新 enable 后可再次触发。
    player.chat(`/scoreboard players enable ${player.name} ${objName}`);
    player.chat(`/trigger ${objName}`);
    score = getScore(objName, player.name);
    test.assert(score === 2, `score should be 2 after re-enable + /trigger, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /trigger 未 enable 不可用：创建 trigger 目标后不 enable，直接 /trigger 应失败（分数 undefined）。
// 验证 entityHasObjective 检查（未 prime 的玩家查不到分数）。
// Ref: Java TriggerCommand（checkValidTrigger，未 prime 返 nullptr）
function triggerRequiresEnable(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "trig_noenable";

    player.chat(`/scoreboard objectives add ${objName} trigger`);
    // 不 enable，直接 /trigger 应失败。
    player.chat(`/trigger ${objName}`);

    const score = getScore(objName, player.name);
    test.assert(score === undefined, `unprimed trigger should not create score, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /trigger 非 trigger 准则目标应失败：创建 dummy 目标，enable 后 /trigger 应失败（criteria 非 trigger）。
// 验证 checkValidTrigger 的 criteria.getName() != trigger 检查。
// Ref: Java TriggerCommand（checkValidTrigger，非 trigger 准则拒绝）
function triggerRejectsNonTriggerCriteria(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "trig_dummy";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    // dummy 目标即使 set 分数，/trigger 也应失败（非 trigger 准则）。
    player.chat(`/scoreboard players set ${player.name} ${objName} 5`);
    player.chat(`/trigger ${objName}`);

    // 分数应仍为 5（trigger 未生效），非 6。
    const score = getScore(objName, player.name);
    test.assert(score === 5, `dummy criteria trigger should not modify score, still 5, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

export function registerTriggerTests(): void {
    // 记分板 objective 世界级单例，用唯一名 + runOnFinish remove 防污染。
    GameTest.register("CommandTests", "trigger_adds_one", triggerAddsOne)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "trigger_add_value", triggerAddValue)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "trigger_set_value", triggerSetValue)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "trigger_locked_after_use", triggerLockedAfterUse)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "trigger_requires_enable", triggerRequiresEnable)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "trigger_rejects_non_trigger_criteria", triggerRejectsNonTriggerCriteria)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
