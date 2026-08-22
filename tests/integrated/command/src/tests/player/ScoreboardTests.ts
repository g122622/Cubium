// /scoreboard 命令 GameTest：记分板目标与分数管理。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/scoreboard.txt）：
//   - /scoreboard objectives add <name> <criteria>：创建目标
//   - /scoreboard objectives remove <name>：删除目标
//   - /scoreboard players set <target> <objective> <score>：设置分数
//   - /scoreboard players add/remove <target> <objective> <score>：增减分数
//   - /scoreboard players reset <target>：重置分数
//
// 设计要点：
//   1. ScoreboardCommand 已实现 10 个子命令（objectives add/remove/list、players set/add/remove/
//      reset/get/enable/list），核心 Scoreboard 系统真实存储（非 stub）。但脚本侧此前无 Scoreboard
//      绑定，GameTest JS 无法读分数断言。本批测试伴随新增的 world.scoreboard 脚本绑定
//      （MinecraftModuleFactory.cpp Scoreboard/Objective JS 类 + ScriptWorldAccessor::getScoreboard
//      回调，见 [[scoreboard-script-binding-and-command-tests]]）。
//   2. 断言用 world.scoreboard.getObjective(name).getScore(participant) 读分数（返 number|undefined），
//      getObjective(name) 返 Objective|undefined，getObjectives() 返 Objective[]，
//      Objective.getParticipants() 返 string[]（holder 名），Objective.id 读目标名。
//   3. 关键约束：ScoreboardCommand 的 target 参数是裸 StringArgumentType::string()（非 vanilla 的
//      ScoreHolderArgument 选择器），故 **@s/@a/* 不生效**——会当字面字符串 "@s" 写入记分板。
//      必须用 SimulatedPlayer 的 username（player.name）做 target。这是已知对齐缺陷（vanilla 支持
//      选择器），待 ScoreHolderArgument 改造，见 [[scoreboard-script-binding-and-command-tests]]。
//   4. 记分板是世界级单例（ServerScoreboard 全局唯一），跨测试持久化不自动重置。故每个测试用
//      唯一 objective 名（含测试函数名后缀）避免互斥，独占 batch 串行 + runOnFinish remove objective
//      清理，防污染后续依赖空记分板的测试。
//   5. criteria 用 "dummy"（最通用判据，手动设值，ScoreCriteriaRegistry 已注册）。
//   6. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_记分板.txt（记分板命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// Cubium 扩展的 Scoreboard/Objective 接口（world.scoreboard 经 as 断言，官方类型无此属性）。
interface CubiumObjective {
    id: string;
    getScore(participant: string): number | undefined;
    getParticipants(): string[];
}

interface CubiumScoreboard {
    getObjective(name: string): CubiumObjective | undefined;
    getObjectives(): CubiumObjective[];
}

// 将官方 world 断言为含 scoreboard 属性的类型。
function getScoreboard(): CubiumScoreboard {
    return (world as unknown as { scoreboard: CubiumScoreboard }).scoreboard;
}

// /scoreboard objectives add kills dummy 创建目标，
// 断言 world.scoreboard.getObjective("kills") 非 undefined 且 id=="kills"。
// runOnFinish remove 清理。
// Ref: wiki commands/scoreboard.txt（objectives add）
function objectivesAddCreatesObjective(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_add";

    player.chat(`/scoreboard objectives add ${objName} dummy`);

    const obj = getScoreboard().getObjective(objName);
    test.assert(obj !== undefined, `objective ${objName} should exist after add`);
    test.assert(obj!.id === objName, `objective id should be ${objName}, got ${obj!.id}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /scoreboard objectives remove 删除目标：add 后 remove，断言 getObjective 返 undefined。
// Ref: wiki commands/scoreboard.txt（objectives remove）
function objectivesRemoveDeletesObjective(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_rm";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    test.assert(getScoreboard().getObjective(objName) !== undefined, "precondition: objective exists");

    player.chat(`/scoreboard objectives remove ${objName}`);
    test.assert(
        getScoreboard().getObjective(objName) === undefined,
        `objective ${objName} should not exist after remove`,
    );

    // 已 remove，无需 runOnFinish。
    test.succeed();
}

// /scoreboard players set <name> <obj> <score> 设置分数，断言 getScore 读回该值。
// 用 player.name 做 target（@s 不生效，见文件头说明）。
// Ref: wiki commands/scoreboard.txt（players set）
function playersSetWritesScore(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_set";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    player.chat(`/scoreboard players set ${player.name} ${objName} 10`);

    const obj = getScoreboard().getObjective(objName);
    test.assert(obj !== undefined, "objective should exist");
    const score = obj!.getScore(player.name);
    test.assert(score === 10, `expected score 10, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /scoreboard players add 累加分数：set 5 后 add 3，断言 getScore==8。
// Ref: wiki commands/scoreboard.txt（players add）
function playersAddAccumulatesScore(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_add2";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    player.chat(`/scoreboard players set ${player.name} ${objName} 5`);
    player.chat(`/scoreboard players add ${player.name} ${objName} 3`);

    const obj = getScoreboard().getObjective(objName);
    test.assert(obj !== undefined, "objective should exist");
    const score = obj!.getScore(player.name);
    test.assert(score === 8, `expected score 8 (5+3), got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /scoreboard players remove 递减分数：set 10 后 remove 4，断言 getScore==6。
// Ref: wiki commands/scoreboard.txt（players remove）
function playersRemoveDecrementsScore(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_rem2";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    player.chat(`/scoreboard players set ${player.name} ${objName} 10`);
    player.chat(`/scoreboard players remove ${player.name} ${objName} 4`);

    const obj = getScoreboard().getObjective(objName);
    test.assert(obj !== undefined, "objective should exist");
    const score = obj!.getScore(player.name);
    test.assert(score === 6, `expected score 6 (10-4), got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// /scoreboard players reset 重置分数：set 后 reset，断言 getScore 返 undefined（无分数）。
// 注：Cubium reset 只支持重置全部（无 [objective] 可选参数），但对单目标重置等价清空该 holder 在
// 所有目标上的分数——此处单目标场景验证清空生效。
// Ref: wiki commands/scoreboard.txt（players reset）
function playersResetClearsScore(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_rst";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    player.chat(`/scoreboard players set ${player.name} ${objName} 7`);
    test.assert(
        getScoreboard().getObjective(objName)!.getScore(player.name) === 7,
        "precondition: score should be 7",
    );

    player.chat(`/scoreboard players reset ${player.name}`);
    const score = getScoreboard().getObjective(objName)!.getScore(player.name);
    test.assert(score === undefined, `expected score undefined after reset, got ${score}`);

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// Objective.getParticipants 列出所有有分数的 holder：两个不同 holder set 分数后，
// getParticipants 应包含两个名字。
// Ref: wiki commands/scoreboard.txt（players list 语义 + 基岩 Objective.getParticipants）
function objectiveGetParticipantsListsHolders(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName = "kills_part";

    player.chat(`/scoreboard objectives add ${objName} dummy`);
    player.chat(`/scoreboard players set alice ${objName} 3`);
    player.chat(`/scoreboard players set bob ${objName} 5`);

    const obj = getScoreboard().getObjective(objName);
    test.assert(obj !== undefined, "objective should exist");
    const participants = obj!.getParticipants();
    test.assert(
        participants.includes("alice") && participants.includes("bob"),
        `participants should include alice and bob, got ${JSON.stringify(participants)}`,
    );

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName}`);
    });
    test.succeed();
}

// world.scoreboard.getObjectives 列出所有目标：add 两个目标后，getObjectives 应含两者 id。
// Ref: wiki commands/scoreboard.txt（objectives list 语义 + 基岩 Scoreboard.getObjectives）
function getObjectivesListsAll(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const objName1 = "kills_list1";
    const objName2 = "kills_list2";

    player.chat(`/scoreboard objectives add ${objName1} dummy`);
    player.chat(`/scoreboard objectives add ${objName2} dummy`);

    const objectives = getScoreboard().getObjectives();
    const ids = objectives.map((o) => o.id);
    test.assert(
        ids.includes(objName1) && ids.includes(objName2),
        `getObjectives should include both, got ${JSON.stringify(ids)}`,
    );

    test.runOnFinish(() => {
        player.chat(`/scoreboard objectives remove ${objName1}`);
        player.chat(`/scoreboard objectives remove ${objName2}`);
    });
    test.succeed();
}

export function registerScoreboardTests(): void {
    // 记分板是世界级单例，独占 batch 串行 + 唯一 objective 名 + runOnFinish 清理防跨测试污染。
    GameTest.register("CommandTests", "scoreboard_objectives_add_creates_objective", objectivesAddCreatesObjective)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_add_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_objectives_remove_deletes_objective", objectivesRemoveDeletesObjective)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_remove_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_players_set_writes_score", playersSetWritesScore)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_set_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_players_add_accumulates_score", playersAddAccumulatesScore)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_add2_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_players_remove_decrements_score", playersRemoveDecrementsScore)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_rem2_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_players_reset_clears_score", playersResetClearsScore)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_rst_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_objective_get_participants_lists_holders", objectiveGetParticipantsListsHolders)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_part_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "scoreboard_get_objectives_lists_all", getObjectivesListsAll)
        .structureName("gametests:cmd_arena")
        .batch("scoreboard_list_solo")
        .maxTicks(60);
}
