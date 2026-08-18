// /gamemode 命令 GameTest：切换玩家游戏模式。
//
// 覆盖 wiki 命令章节核心行为：
//   - /gamemode <mode>：切换命令源自身游戏模式（Ref: wiki commands/gamemode.txt）
//   - /gamemode <mode> <target>：切换指定玩家游戏模式
//
// 设计要点：
//   1. GameModeCommand 此前经 GameModeManager→PlayerManager 查 ServerPlayerData，
//      SimulatedPlayer 不进 PlayerManager 故 /gamemode 对其失效。已补实体旁路：
//      GameModeManager 失败时经 ServerPlayerEntityManager 解析实体直接 Player::setGameMode。
//   2. 判定模式切换生效用 Player.getGameMode()（继承自 @minecraft/server Player），
//      读 Player::m_gameMode 实体字段（实体旁路写入处），返回 GameMode 字符串枚举。
//   3. spawnSimulatedPlayer 第3参 gameMode 在 Cubium 绑定中为数字（0=Survival,1=Creative），
//      与官方字符串枚举不一致，故用 `0 as any`/`1 as any` 绕过类型（同 CaveSpiderTests 范式）。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// /gamemode <mode> 切换命令源自身（走 _setGameModeSelf 分支）。
// Ref: wiki commands/gamemode.txt（gamemode <mode> 改自己）
function gamemodeChangesSelfToCreative(test: Test): void {
    // 以生存模式生成（第3参 0），便于验证切到创造。
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op", 0 as any);
    test.assert(player.getGameMode() === "survival", "spawn should be survival");

    player.chat("/gamemode creative");

    // 命令同步执行，立即断言模式已切换。
    test.assert(player.getGameMode() === "creative", `expected creative, got ${player.getGameMode()}`);
    test.succeed();
}

// /gamemode <mode> 从创造切回生存（验证双向切换）。
function gamemodeChangesSelfToSurvival(test: Test): void {
    // 默认创造模式生成（不传第3参）。
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.assert(player.getGameMode() === "creative", "spawn should be creative");

    player.chat("/gamemode survival");

    test.assert(player.getGameMode() === "survival", `expected survival, got ${player.getGameMode()}`);
    test.succeed();
}

// /gamemode adventure 切到冒险模式。
function gamemodeSetsAdventure(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    player.chat("/gamemode adventure");

    test.assert(player.getGameMode() === "adventure", `expected adventure, got ${player.getGameMode()}`);
    test.succeed();
}

// /gamemode <mode> <playerName> 切换指定玩家（走 _setGameModeOthers 分支）。
// Ref: wiki commands/gamemode.txt（gamemode <mode> <player> 改指定玩家）
function gamemodeChangesBySelector(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "target", 0 as any);
    // 用显式名字 "target" 作为 target 选择器。
    player.chat("/gamemode creative target");

    test.assert(player.getGameMode() === "creative", `expected creative, got ${player.getGameMode()}`);
    test.succeed();
}

export function registerGameModeTests(): void {
    GameTest.register("CommandTests", "gamemode_changes_self_to_creative", gamemodeChangesSelfToCreative)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamemode_changes_self_to_survival", gamemodeChangesSelfToSurvival)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamemode_sets_adventure", gamemodeSetsAdventure)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "gamemode_changes_by_selector", gamemodeChangesBySelector)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
