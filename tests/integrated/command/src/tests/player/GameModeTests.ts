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

// /gamemode <mode> @a[distance=..N] 批量切换多玩家游戏模式（走 _setGameModeOthers 多目标分支）。
// spawn 2 个 SimulatedPlayer（默认创造），/gamemode @a[distance=..20] survival 批量切生存，
// 断言两玩家都切换到 survival。
// 验证 PlayerResolver 选择器修复后 @a[distance=..N] 能选中多个 SimulatedPlayer（修复前 applyFilters
// 对 SimulatedPlayer 误删，@a 选不到任何 SimulatedPlayer，批量切换不执行）。
// distance=..20 以 playerA 为中心，选中结构内两玩家（间距约 5.6 格 < 20），区域限定避免选中同批
// 并行测试的 SimulatedPlayer（污染防护）。
// 走 GameModeCommand::_setGameModeOthers（resolvePlayerIds 多结果 + setGameModeOnPlayer 循环）。
// Ref: wiki commands/gamemode.txt（gamemode <mode> <targets> 批量改多玩家）
function gamemodeChangesAllPlayersBySelector(test: Test): void {
    // 两玩家默认创造模式生成，不同位置（空气腔 y=2 站立层）。
    const playerA = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 2 }, "moverA");
    const playerB = test.spawnSimulatedPlayer({ x: 6, y: 2, z: 6 }, "moverB");
    test.assert(playerA.getGameMode() === "creative", "moverA spawn should be creative");
    test.assert(playerB.getGameMode() === "creative", "moverB spawn should be creative");

    // 等 playerB 生成稳定后执行（@a 解析需两玩家都已注册到 ServerPlayerEntityManager）。
    test.runAtTickTime(5, () => {
        // /gamemode <mode> <targets>：mode survival 在前，@a[distance=..20] 在后。
        // @a[distance=..20] 以 playerA 位置为中心，选中结构内两玩家，批量切生存。
        playerA.chat("/gamemode survival @a[distance=..20]");
    });

    // 命令同步执行，但 @a 解析经 runAtTickTime 延迟，用 runAtTickTime 延迟断言两玩家都已切换。
    test.runAtTickTime(10, () => {
        const modeA = playerA.getGameMode();
        const modeB = playerB.getGameMode();
        test.assert(modeA === "survival", `moverA expected survival, got ${modeA}`);
        test.assert(modeB === "survival", `moverB expected survival, got ${modeB}`);
        test.succeed();
    });
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

    GameTest.register("CommandTests", "gamemode_changes_all_players_by_selector", gamemodeChangesAllPlayersBySelector)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
