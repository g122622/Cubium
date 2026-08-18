// /xp（/experience 别名）命令 GameTest：增减/设置/查询玩家经验。
//
// 覆盖 wiki 命令章节核心行为：
//   - /xp add <player> <amount> levels：增加经验等级（Ref: wiki commands/experience.txt）
//   - /xp add <player> <amount> points：增加经验点数
//   - /xp set <player> <amount> levels：设置经验等级
//   - /xp set <player> <amount> points：设置经验点数
//
// 设计要点：
//   1. ExperienceCommand 已是实体旁路：getTargetPlayer 经 ServerPlayerEntityManager 解析实体调
//      Player::addExperience/addExperienceLevels/setExperience/setExperienceLevel/totalExperience/experienceLevel，
//      对 SimulatedPlayer 直接生效，无需修框架。
//   2. 判定经验变化用 SimulatedPlayer 新绑定的 level（只读，等级）、getTotalXp()（总经验点数）、
//      xp（只读，0-1 进度）三个绑定（对齐基岩 Player.level/getTotalXp/xp，读 Player 实体层字段，
//      见 [[simulated-player-js-class-no-entity-inheritance]]）。SimulatedPlayer JS 类独立注册未继承
//      Entity 原型，故这三个绑定需在 ScriptSimulatedPlayer.cpp 单独绑定。
//   3. 新 SimulatedPlayer 初始 level=0、totalXp=0。
//   4. SimulatedPlayer::chat permLevel 已固定为 4（与游戏模式解耦），任意模式可执行管理命令。
//   5. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_experience.txt（add/set/query）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// SimulatedPlayer 上 level/getTotalXp/xp 是 Cubium 扩展绑定（非官方 @minecraft/server-gametest 类型声明
// 所含），用 as any 绕过 TS 校验。
interface ExperiencePlayer {
    readonly level: number;
    getTotalXp(): number;
    readonly xp: number;
}

function xpPlayer(player: unknown): ExperiencePlayer {
    return player as ExperiencePlayer;
}

// /xp add @s 5 levels 增加等级（走 addLevels 分支）。
// Ref: wiki commands/experience.txt（experience add <player> <amount> levels）
function xpAddsLevels(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.assert(xpPlayer(player).level === 0, `expected initial level 0, got ${xpPlayer(player).level}`);

    player.chat("/xp add @s 5 levels");

    test.assert(xpPlayer(player).level === 5, `expected level 5 after adding 5 levels, got ${xpPlayer(player).level}`);
    test.succeed();
}

// /xp add @s 100 points 增加经验点数（走 addPoints 分支），总经验点数应增加 100。
// Ref: wiki commands/experience.txt（experience add <player> <amount> points）
function xpAddsPoints(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.assert(xpPlayer(player).getTotalXp() === 0, `expected initial totalXp 0, got ${xpPlayer(player).getTotalXp()}`);

    player.chat("/xp add @s 100 points");

    test.assert(
        xpPlayer(player).getTotalXp() === 100,
        `expected totalXp 100 after adding 100 points, got ${xpPlayer(player).getTotalXp()}`,
    );
    test.succeed();
}

// /xp set @s 10 levels 设置等级（走 setLevels 分支）。
// Ref: wiki commands/experience.txt（experience set <player> <amount> levels）
function xpSetsLevel(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 先增加一些等级与点数，使初始状态非 0，验证 set 是覆盖而非累加。
    player.chat("/xp add @s 3 levels");
    player.chat("/xp add @s 50 points");
    test.assert(xpPlayer(player).level >= 3, `expected level >= 3 before set, got ${xpPlayer(player).level}`);

    player.chat("/xp set @s 10 levels");

    test.assert(
        xpPlayer(player).level === 10,
        `expected level 10 after setting 10 levels, got ${xpPlayer(player).level}`,
    );
    test.succeed();
}

// /xp set @s 50 points 设置经验点数（走 setPoints 分支），setPoints 先重置为 0 再 add，故 totalXp==50。
// Ref: wiki commands/experience.txt（experience set <player> <amount> points）
function xpSetsPoints(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 先增加点数，使初始状态非 0，验证 set 是覆盖而非累加。
    player.chat("/xp add @s 200 points");
    test.assert(
        xpPlayer(player).getTotalXp() === 200,
        `expected totalXp 200 before set, got ${xpPlayer(player).getTotalXp()}`,
    );

    player.chat("/xp set @s 50 points");

    test.assert(
        xpPlayer(player).getTotalXp() === 50,
        `expected totalXp 50 after setting 50 points, got ${xpPlayer(player).getTotalXp()}`,
    );
    test.succeed();
}

// /xp add 负值减少等级（vanilla 支持负 amount，走 addLevels，等级降至 0 不为负）。
// Ref: wiki commands/experience.txt（amount 允许负值）
function xpAddsNegativeLevelsFlooredAtZero(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 初始 level=0，减少 5 级不应使等级为负（vanilla 经验等级下限为 0）。
    player.chat("/xp add @s -5 levels");

    test.assert(
        xpPlayer(player).level === 0,
        `expected level floored at 0 after subtracting from 0, got ${xpPlayer(player).level}`,
    );
    test.succeed();
}

export function registerExperienceTests(): void {
    GameTest.register("CommandTests", "xp_adds_levels", xpAddsLevels).structureName("gametests:cmd_arena").maxTicks(60);

    GameTest.register("CommandTests", "xp_adds_points", xpAddsPoints)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "xp_sets_level", xpSetsLevel).structureName("gametests:cmd_arena").maxTicks(60);

    GameTest.register("CommandTests", "xp_sets_points", xpSetsPoints)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "xp_adds_negative_levels_floored_at_zero", xpAddsNegativeLevelsFlooredAtZero)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
