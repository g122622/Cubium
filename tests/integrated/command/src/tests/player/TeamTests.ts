// /team 命令 GameTest：记分板队伍管理。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/team.txt）：
//   - /team add <team> [displayName]：创建队伍
//   - /team remove <team>：删除队伍
//   - /team join <team> <members>：加入队伍（members 是选择器，@s/@a 生效）
//   - /team leave <members>：离开队伍
//   - /team empty <team>：清空队伍成员
//   - /team modify <team> <property> <value>：修改队伍属性（color/friendlyFire/seeFriendlyInvisibles/
//     nametagVisibility/deathMessageVisibility/collisionRule）
//   - /team list [team]：列出队伍（命令侧反馈，JS 侧用 getTeams 断言）
//
// 设计要点：
//   1. TeamCommand 已实现 7 个子命令，核心 Scoreboard/ScorePlayerTeam 真实存储（非 stub）。脚本侧此前无
//      Team 绑定，GameTest JS 无法读队伍断言。本批测试伴随新增的 Scoreboard.getTeam/getTeams +
//      Team JS 类绑定（id/color/friendlyFire/seeFriendlyInvisibles/nametagVisibility/
//      deathMessageVisibility/collisionRule/getMembers/hasMember，见 [[team-script-binding-and-command-tests]]）。
//   2. 断言用 world.scoreboard.getTeam(name) 读 Team 对象，team.id/color/friendlyFire/... 读属性，
//      team.getMembers()/hasMember(name) 读成员。getTeam 不存在返 undefined，getTeams 返 Team[]。
//   3. 关键差异（对比 ScoreboardCommand）：TeamCommand 的 add/remove/empty/modify 的 <team> 参数是裸
//      StringArgumentType::string()（队名是字面名称，符合原版语义）；但 join/leave 的 <members> 是
//      EntityArgumentType::entities()（**选择器**），@s/@a **生效**（区别于 ScoreboardCommand target
//      裸 string 致 @s 不生效，见 [[scoreboard-script-binding-and-command-tests]]）。
//   4. 记分板是世界级单例（ServerScoreboard 全局唯一），跨测试持久化不自动重置。故每个测试用唯一
//      team 名（含测试函数名后缀）避免互斥，独占 batch 串行 + runOnFinish remove team 清理，防污染
//      后续依赖空记分板的测试。
//   5. TeamCommand 需 permLevel≥2（hasPermission(2)）。SimulatedPlayer permLevel 固定=4（对齐 vanilla
//      单人 OP 等级），满足。Survival 模式亦可执行（permLevel 与游戏模式解耦，见
//      [[simulated-player-permlevel-decoupled-from-gamemode]]）。
//   6. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_队伍.txt（队伍命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// Cubium 扩展的 Team/Scoreboard 接口（world.scoreboard 经 as 断言，官方类型无此属性）。
interface CubiumTeam {
    id: string;
    color: string;
    friendlyFire: boolean;
    seeFriendlyInvisibles: boolean;
    nametagVisibility: string;
    deathMessageVisibility: string;
    collisionRule: string;
    getMembers(): string[];
    hasMember(name: string): boolean;
}

interface CubiumScoreboard {
    getTeam(name: string): CubiumTeam | undefined;
    getTeams(): CubiumTeam[];
}

// 将官方 world 断言为含 scoreboard 属性的类型。
function getScoreboard(): CubiumScoreboard {
    return (world as unknown as { scoreboard: CubiumScoreboard }).scoreboard;
}

// /team add <name> 创建队伍，断言 getTeam(name) 非 undefined 且 id==name，默认属性对齐 vanilla
// （color=white、friendlyFire=true、seeFriendlyInvisibles=true、nametag/deathMessage=always、collision=always）。
// Ref: wiki commands/team.txt（add）
function teamAddCreatesTeam(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_add";

    player.chat(`/team add ${teamName}`);

    const team = getScoreboard().getTeam(teamName);
    test.assert(team !== undefined, `team ${teamName} should exist after add`);
    test.assert(team!.id === teamName, `team id should be ${teamName}, got ${team!.id}`);
    // 默认属性对齐 vanilla Team 构造（ScorePlayerTeam.hpp:155-160）。
    test.assert(team!.color === "white", `default color should be white, got ${team!.color}`);
    test.assert(team!.friendlyFire === true, "default friendlyFire should be true");
    test.assert(team!.seeFriendlyInvisibles === true, "default seeFriendlyInvisibles should be true");
    test.assert(team!.nametagVisibility === "always", `default nametagVisibility should be always, got ${team!.nametagVisibility}`);
    test.assert(team!.deathMessageVisibility === "always", `default deathMessageVisibility should be always`);
    test.assert(team!.collisionRule === "always", `default collisionRule should be always`);

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team remove <name> 删除队伍：add 后 remove，断言 getTeam 返 undefined。
// Ref: wiki commands/team.txt（remove）
function teamRemoveDeletesTeam(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_rm";

    player.chat(`/team add ${teamName}`);
    test.assert(getScoreboard().getTeam(teamName) !== undefined, "precondition: team exists");

    player.chat(`/team remove ${teamName}`);
    test.assert(
        getScoreboard().getTeam(teamName) === undefined,
        `team ${teamName} should not exist after remove`,
    );

    // 已 remove，无需 runOnFinish。
    test.succeed();
}

// /team join <team> @s 加入队伍：add 后 join @s，断言 hasMember(player.name) 为 true。
// 验证 join 的 members 选择器 @s 对 SimulatedPlayer 生效（区别 ScoreboardCommand target 裸 string）。
// Ref: wiki commands/team.txt（join）
function teamJoinAddsMember(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_join";

    player.chat(`/team add ${teamName}`);
    player.chat(`/team join ${teamName} @s`);

    const team = getScoreboard().getTeam(teamName);
    test.assert(team !== undefined, "team should exist");
    test.assert(
        team!.hasMember(player.name),
        `player ${player.name} should be a member of team ${teamName}, members=${JSON.stringify(team!.getMembers())}`,
    );
    test.assert(
        team!.getMembers().includes(player.name),
        `getMembers should include ${player.name}, got ${JSON.stringify(team!.getMembers())}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team leave @s 离开队伍：join 后 leave @s，断言 hasMember 返 false。
// Ref: wiki commands/team.txt（leave）
function teamLeaveRemovesMember(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_leave";

    player.chat(`/team add ${teamName}`);
    player.chat(`/team join ${teamName} @s`);
    test.assert(getScoreboard().getTeam(teamName)!.hasMember(player.name), "precondition: player is member");

    player.chat(`/team leave @s`);
    test.assert(
        !getScoreboard().getTeam(teamName)!.hasMember(player.name),
        `player ${player.name} should not be a member after leave`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team empty <team> 清空成员：join 一名成员后 empty，断言成员集合为空。
// 用 @s 选择器加入 SimulatedPlayer（@a 在单玩家测试环境等价 @s，但 @s 更直接）。
// Ref: wiki commands/team.txt（empty）
function teamEmptyClearsMembers(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_empty";

    player.chat(`/team add ${teamName}`);
    player.chat(`/team join ${teamName} @s`);

    const team = getScoreboard().getTeam(teamName);
    test.assert(team !== undefined && team!.hasMember(player.name), "precondition: player is member");

    player.chat(`/team empty ${teamName}`);
    test.assert(
        getScoreboard().getTeam(teamName)!.getMembers().length === 0,
        `team ${teamName} should have 0 members after empty, got ${JSON.stringify(getScoreboard().getTeam(teamName)!.getMembers())}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team modify <team> color <color> 修改颜色：add 后 modify color red，断言 team.color=="red"。
// Ref: wiki commands/team.txt（modify color）
function teamModifyColor(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_color";

    player.chat(`/team add ${teamName}`);
    player.chat(`/team modify ${teamName} color red`);

    test.assert(
        getScoreboard().getTeam(teamName)!.color === "red",
        `team color should be red, got ${getScoreboard().getTeam(teamName)!.color}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team modify <team> friendlyFire <bool> 修改友军伤害：默认 true，modify false 后断言为 false。
// Ref: wiki commands/team.txt（modify friendlyFire）
function teamModifyFriendlyFire(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_ff";

    player.chat(`/team add ${teamName}`);
    test.assert(getScoreboard().getTeam(teamName)!.friendlyFire === true, "default friendlyFire should be true");

    player.chat(`/team modify ${teamName} friendlyFire false`);
    test.assert(
        getScoreboard().getTeam(teamName)!.friendlyFire === false,
        `friendlyFire should be false after modify, got ${getScoreboard().getTeam(teamName)!.friendlyFire}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team modify <team> seeFriendlyInvisibles <bool>：默认 true，modify false 后断言为 false。
// Ref: wiki commands/team.txt（modify seeFriendlyInvisibles）
function teamModifySeeFriendlyInvisibles(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_invis";

    player.chat(`/team add ${teamName}`);
    test.assert(
        getScoreboard().getTeam(teamName)!.seeFriendlyInvisibles === true,
        "default seeFriendlyInvisibles should be true",
    );

    player.chat(`/team modify ${teamName} seeFriendlyInvisibles false`);
    test.assert(
        getScoreboard().getTeam(teamName)!.seeFriendlyInvisibles === false,
        `seeFriendlyInvisibles should be false after modify`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team modify <team> nametagVisibility <value>：modify hideForOtherTeams 后断言对应字符串。
// Ref: wiki commands/team.txt（modify nametagVisibility）
function teamModifyNametagVisibility(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_nametag";

    player.chat(`/team add ${teamName}`);
    player.chat(`/team modify ${teamName} nametagVisibility hideForOtherTeams`);

    test.assert(
        getScoreboard().getTeam(teamName)!.nametagVisibility === "hideForOtherTeams",
        `nametagVisibility should be hideForOtherTeams, got ${getScoreboard().getTeam(teamName)!.nametagVisibility}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team modify <team> collisionRule <value>：modify pushOwnTeam 后断言对应字符串。
// Ref: wiki commands/team.txt（modify collisionRule）
function teamModifyCollisionRule(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName = "team_collide";

    player.chat(`/team add ${teamName}`);
    player.chat(`/team modify ${teamName} collisionRule pushOwnTeam`);

    test.assert(
        getScoreboard().getTeam(teamName)!.collisionRule === "pushOwnTeam",
        `collisionRule should be pushOwnTeam, got ${getScoreboard().getTeam(teamName)!.collisionRule}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName}`);
    });
    test.succeed();
}

// /team list 列出所有队伍：add 两个队伍后，getTeams 应含两者 id。
// 对齐 wiki /team list 语义（命令侧反馈 + JS 侧 getTeams 断言集合）。
// Ref: wiki commands/team.txt（list）
function teamListShowsAllTeams(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const teamName1 = "team_list1";
    const teamName2 = "team_list2";

    player.chat(`/team add ${teamName1}`);
    player.chat(`/team add ${teamName2}`);

    const ids = getScoreboard().getTeams().map((t) => t.id);
    test.assert(
        ids.includes(teamName1) && ids.includes(teamName2),
        `getTeams should include both, got ${JSON.stringify(ids)}`,
    );

    test.runOnFinish(() => {
        player.chat(`/team remove ${teamName1}`);
        player.chat(`/team remove ${teamName2}`);
    });
    test.succeed();
}

export function registerTeamTests(): void {
    // 队伍是世界级单例，独占 batch 串行 + 唯一 team 名 + runOnFinish 清理防跨测试污染。
    GameTest.register("CommandTests", "team_add_creates_team", teamAddCreatesTeam)
        .structureName("gametests:cmd_arena")
        .batch("team_add_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_remove_deletes_team", teamRemoveDeletesTeam)
        .structureName("gametests:cmd_arena")
        .batch("team_remove_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_join_adds_member", teamJoinAddsMember)
        .structureName("gametests:cmd_arena")
        .batch("team_join_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_leave_removes_member", teamLeaveRemovesMember)
        .structureName("gametests:cmd_arena")
        .batch("team_leave_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_empty_clears_members", teamEmptyClearsMembers)
        .structureName("gametests:cmd_arena")
        .batch("team_empty_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_modify_color", teamModifyColor)
        .structureName("gametests:cmd_arena")
        .batch("team_color_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_modify_friendly_fire", teamModifyFriendlyFire)
        .structureName("gametests:cmd_arena")
        .batch("team_ff_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_modify_see_friendly_invisibles", teamModifySeeFriendlyInvisibles)
        .structureName("gametests:cmd_arena")
        .batch("team_invis_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_modify_nametag_visibility", teamModifyNametagVisibility)
        .structureName("gametests:cmd_arena")
        .batch("team_nametag_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_modify_collision_rule", teamModifyCollisionRule)
        .structureName("gametests:cmd_arena")
        .batch("team_collide_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "team_list_shows_all_teams", teamListShowsAllTeams)
        .structureName("gametests:cmd_arena")
        .batch("team_list_solo")
        .maxTicks(60);
}
