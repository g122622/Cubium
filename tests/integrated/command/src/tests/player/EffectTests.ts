// /effect 命令 GameTest：给予/清除玩家状态效果。
//
// 覆盖 wiki 命令章节核心行为：
//   - /effect give <player> <effect> [seconds] [amplifier]：给予状态效果（Ref: wiki commands/effect.txt）
//   - /effect clear <player>：清除所有效果
//   - /effect clear <player> <effect>：清除指定效果
//
// 设计要点：
//   1. EffectCommand 此前经 PlayerManager.getPlayer 写 ServerPlayerData.effects（网络数据层），
//      对不进 PlayerManager 的 SimulatedPlayer 返 nullptr 跳过失效；且即便真实玩家，ServerPlayerData.effects
//      与脚本 Entity.getEffect 读的 LivingEntity::effectManager（实体层）层错配，致 /effect 成功但脚本读不到。
//      已统一改走实体层：经 ServerPlayerEntityManager 解析实体调 LivingEntity::addEffect/hasEffect/removeEffect，
//      与脚本 getEffect 对齐（同 GameModeCommand/TeleportCommand 旁路模式）。
//   2. 判定效果生效用 Entity.getEffect(effectType)（继承自 @minecraft/server Entity，已绑定），
//      返回 { typeId, amplifier, duration } 普通对象，无效果返回 undefined。读 LivingEntity::effectManager。
//   3. SimulatedPlayer::chat permLevel 已固定为 4（与游戏模式解耦），任意模式可执行管理命令。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_effect.txt（give/clear）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// /effect give @s <effect> 给予自身状态效果（走 _giveEffect 分支）。
// Ref: wiki commands/effect.txt（effect give <player> <effect> 给予效果）
function effectGivesSpeedToSelf(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 给予速度效果（默认 30 秒、amplifier 0）。
    player.chat("/effect give @s speed");

    // 命令同步执行，立即断言效果已添加。
    const speed = (player as any).getEffect("speed");
    test.assert(speed !== undefined, `expected speed effect, got undefined`);
    test.succeed();
}

// /effect give @s <effect> <seconds> <amplifier> 指定等级（amplifier 1 = 速度 II）。
// Ref: wiki commands/effect.txt（amplifier 参数：0=I, 1=II）
function effectGivesAmplifier(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 给予速度 II（amplifier=1）。
    player.chat("/effect give @s speed 30 1");

    const speed = (player as any).getEffect("speed");
    test.assert(speed !== undefined, `expected speed effect, got undefined`);
    // amplifier 0 对应等级 I，1 对应 II。getEffect 返回的 amplifier 是原始值（0-based）。
    test.assert(speed.amplifier === 1, `expected amplifier 1, got ${speed?.amplifier}`);
    test.succeed();
}

// /effect clear @s 清除所有效果（走 _clearAllEffects 分支）。
// Ref: wiki commands/effect.txt（effect clear <player> 清除全部）
function effectClearRemovesAll(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 先给予两种效果。
    player.chat("/effect give @s speed");
    player.chat("/effect give @s regeneration");
    test.assert((player as any).getEffect("speed") !== undefined, "speed should be present before clear");
    test.assert((player as any).getEffect("regeneration") !== undefined, "regeneration should be present before clear");

    // 清除所有效果。
    player.chat("/effect clear @s");

    test.assert((player as any).getEffect("speed") === undefined, "speed should be cleared");
    test.assert((player as any).getEffect("regeneration") === undefined, "regeneration should be cleared");
    test.succeed();
}

// /effect clear @s <effect> 仅清除指定效果，保留其他效果（走 _clearSpecificEffect 分支）。
// Ref: wiki commands/effect.txt（effect clear <player> <effect> 清除指定）
function effectClearSpecificKeepsOthers(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 先给予两种效果。
    player.chat("/effect give @s speed");
    player.chat("/effect give @s regeneration");

    // 仅清除 speed，regeneration 应保留。
    player.chat("/effect clear @s speed");

    test.assert((player as any).getEffect("speed") === undefined, "speed should be cleared");
    test.assert((player as any).getEffect("regeneration") !== undefined, "regeneration should be preserved");
    test.succeed();
}

// /effect give @a[distance=..N] <effect> 批量给多玩家状态效果（走 _giveEffect 多目标分支）。
// spawn 2 个 SimulatedPlayer，/effect give @a[distance=..20] speed 批量给速度，断言两玩家都有 speed。
// 验证 PlayerResolver 选择器修复后 @a[distance=..N] 能批量选中多个 SimulatedPlayer 并给予效果
// （修复前 applyFilters 对 SimulatedPlayer 误删，@a 选不到任何 SimulatedPlayer，批量给予不执行）。
// distance=..20 以 playerA 为中心，选中结构内两玩家（间距约 5.6 格 < 20），区域限定避免选中同批
// 并行测试的 SimulatedPlayer（污染防护）。
// 走 EffectCommand::_giveEffect（resolvePlayerIds 多结果 + LivingEntity::addEffect 循环）。
// Ref: wiki commands/effect.txt（effect give <targets> <effect> 批量给多玩家）
function effectGivesToAllPlayersBySelector(test: Test): void {
    // 两玩家在空气腔 y=2 站立层（下方 y=1 stone 地板支撑），不同位置。
    const playerA = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 2 }, "moverA");
    const playerB = test.spawnSimulatedPlayer({ x: 6, y: 2, z: 6 }, "moverB");

    // 等 playerB 生成稳定后执行（@a 解析需两玩家都已注册到 ServerPlayerEntityManager）。
    test.runAtTickTime(5, () => {
        // @a[distance=..20] 以 playerA 位置为中心，选中结构内两玩家，批量给速度。
        playerA.chat("/effect give @a[distance=..20] speed");
    });

    // 命令同步执行，但 @a 解析经 runAtTickTime 延迟，用 runAtTickTime 延迟断言两玩家都有 speed。
    test.runAtTickTime(10, () => {
        const speedA = (playerA as any).getEffect("speed");
        const speedB = (playerB as any).getEffect("speed");
        test.assert(speedA !== undefined, `moverA expected speed effect, got undefined`);
        test.assert(speedB !== undefined, `moverB expected speed effect, got undefined`);
        test.succeed();
    });
}

export function registerEffectTests(): void {
    GameTest.register("CommandTests", "effect_gives_speed_to_self", effectGivesSpeedToSelf)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "effect_gives_amplifier", effectGivesAmplifier)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "effect_clear_removes_all", effectClearRemovesAll)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "effect_clear_specific_keeps_others", effectClearSpecificKeepsOthers)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "effect_gives_to_all_players_by_selector", effectGivesToAllPlayersBySelector)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
