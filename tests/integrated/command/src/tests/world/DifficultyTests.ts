// /difficulty 命令 GameTest：设置/查询世界难度。
//
// 覆盖 wiki 命令章节核心行为：
//   - /difficulty <peaceful|easy|normal|hard>：设置世界难度（Ref: wiki commands/difficulty.txt）
//   - /difficulty：查询当前难度
//
// 设计要点：
//   1. DifficultyCommand 经 server->setDifficulty(difficulty) 修改世界难度（非占位），与玩家无关，
//      对 SimulatedPlayer 完全生效。此前脚本侧无难度读取绑定，/difficulty 查询消息经
//      source.sendMessage 不经脚本可读通道，无法端到端断言。本次补全 Dimension.getDifficulty()
//      （IWorld::difficulty 经 ServerWorld override，映射 "peaceful"/"easy"/"normal"/"hard"）
//      脚本读取绑定，解锁端到端测试。
//   2. 难度是世界级单例状态（MinecraftServer::m_difficulty），GameTest 共享单一 ServerWorld，
//      跨测试/跨批次持久化不自动重置。测试临时改难度后须 runOnFinish 恢复 Normal（默认值），
//      避免污染同次全量跑里依赖难度的其他测试（如 DespawnTests 依赖 peaceful 清怪物、
//      蜜蜂中毒 Normal 10s/Hard 18s）。
//   3. 改难度的多个测试须独占 batch 串行执行（同 [[gametest-world-state-gamerule-difficulty-batch-isolation]]
//      范式）：同 batch 并行执行共享世界难度会互相覆盖。各测试用独占 batch 名（difficulty_*_solo），
//      前缀非 day/night → 走 day 环境，由测试内 /difficulty 自行设。
//   4. peaceful 效果对照（difficulty_peaceful_clears_mobs）：spawn zombie 后 /difficulty peaceful，
//      pollUntil 断言 zombie 被 DespawnManager 清除（MonsterEntity::isDespawnPeaceful 返 true，
//      见 [[gametest-world-state-gamerule-difficulty-batch-isolation]]），验证难度不仅改标志还真实
//      影响世界行为。runOnFinish 恢复 normal。
//   5. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_difficulty.txt（难度命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { asDim } from "../../utils/script/cubiumExtensions.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 默认难度（与 MinecraftServer.hpp m_difficulty=Normal 一致），runOnFinish 恢复用。
const DEFAULT_DIFFICULTY = "normal";

// /difficulty hard 设置难度为 hard，断言 getDifficulty()=="hard"。
// runOnFinish 恢复 normal。
// Ref: wiki commands/difficulty.txt（difficulty hard）
function difficultySetHard(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/difficulty hard");

    const val = dim.getDifficulty();
    test.assert(val === "hard", `expected difficulty="hard", got "${val}"`);

    test.runOnFinish(() => {
        player.chat(`/difficulty ${DEFAULT_DIFFICULTY}`);
    });
    test.succeed();
}

// /difficulty peaceful 设置难度为 peaceful，断言 getDifficulty()=="peaceful"。
// runOnFinish 恢复 normal（peaceful 会清全维度怪物，必须恢复）。
// Ref: wiki commands/difficulty.txt（difficulty peaceful）
function difficultySetPeaceful(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/difficulty peaceful");

    const val = dim.getDifficulty();
    test.assert(val === "peaceful", `expected difficulty="peaceful", got "${val}"`);

    test.runOnFinish(() => {
        player.chat(`/difficulty ${DEFAULT_DIFFICULTY}`);
    });
    test.succeed();
}

// /difficulty easy 设置难度为 easy，断言 getDifficulty()=="easy"。
// runOnFinish 恢复 normal。
// Ref: wiki commands/difficulty.txt（difficulty easy）
function difficultySetEasy(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/difficulty easy");

    const val = dim.getDifficulty();
    test.assert(val === "easy", `expected difficulty="easy", got "${val}"`);

    test.runOnFinish(() => {
        player.chat(`/difficulty ${DEFAULT_DIFFICULTY}`);
    });
    test.succeed();
}

// 难度往返：normal→hard→easy→normal，每步断言 getDifficulty 反映最新值。
// 验证 setDifficulty 重复设置覆盖旧值（非仅首次生效），各难度名都能正确映射。
// Ref: wiki commands/difficulty.txt（多次切换难度）
function difficultyToggleBack(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/difficulty hard");
    test.assert(dim.getDifficulty() === "hard", "after set hard should be hard");

    player.chat("/difficulty easy");
    test.assert(dim.getDifficulty() === "easy", "after set easy should be easy");

    player.chat("/difficulty normal");
    test.assert(dim.getDifficulty() === "normal", "after set normal should be normal");

    // 已恢复 normal，无需 runOnFinish。
    test.succeed();
}

// peaceful 难度清除怪物对照：spawn zombie 后 /difficulty peaceful，pollUntil 断言 zombie 消失。
// 验证难度不仅改标志还真实触发 DespawnManager 清除（MonsterEntity::isDespawnPeaceful=true）。
// 独占 batch + runOnFinish 恢复 normal（peaceful 清全维度怪物必须恢复，否则污染依赖怪物的测试）。
// Ref: wiki commands/difficulty.txt（peaceful 难度无怪物生成且清除现有怪物）
function difficultyPeacefulClearsMobs(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // cmd_arena 内 spawn zombie（生存空间，相对坐标 3,2,3）。
    test.spawn("minecraft:zombie", { x: 3, y: 2, z: 3 });

    // 设 peaceful 难度。
    player.chat("/difficulty peaceful");

    // 轮询断言 zombie 被 DespawnManager 清除。
    // DespawnManager 每 tick 扫描，peaceful 难度下 MonsterEntity 立即标记清除。
    // maxTick 200 留余量（DespawnManager tick 调度 + 清除延迟）。
    pollUntilSucceed(
        test,
        () => {
            const zombies = test.getDimension().getEntities({
                type: "minecraft:zombie",
                location: test.worldLocation({ x: 0, y: 0, z: 0 }),
                volume: { x: 9, y: 7, z: 9 },
            });
            return zombies.length === 0;
        },
        {
            maxTick: 200,
            onTimeout: () => {
                const zombies = test.getDimension().getEntities({
                    type: "minecraft:zombie",
                    location: test.worldLocation({ x: 0, y: 0, z: 0 }),
                    volume: { x: 9, y: 7, z: 9 },
                });
                test.assert(false, `peaceful did not clear zombie, count=${zombies.length}`);
            },
        },
    );

    test.runOnFinish(() => {
        player.chat(`/difficulty ${DEFAULT_DIFFICULTY}`);
    });
}

export function registerDifficultyTests(): void {
    // 难度是世界级单例状态，各测试独占 batch 串行避免并行互相覆盖。
    GameTest.register("CommandTests", "difficulty_set_hard", difficultySetHard)
        .structureName("gametests:cmd_arena")
        .batch("difficulty_hard_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "difficulty_set_peaceful", difficultySetPeaceful)
        .structureName("gametests:cmd_arena")
        .batch("difficulty_peaceful_set_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "difficulty_set_easy", difficultySetEasy)
        .structureName("gametests:cmd_arena")
        .batch("difficulty_easy_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "difficulty_toggle_back", difficultyToggleBack)
        .structureName("gametests:cmd_arena")
        .batch("difficulty_toggle_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "difficulty_peaceful_clears_mobs", difficultyPeacefulClearsMobs)
        .structureName("gametests:cmd_arena")
        .batch("difficulty_clear_solo")
        .maxTicks(220);
}
