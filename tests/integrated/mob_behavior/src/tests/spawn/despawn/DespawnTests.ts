// DespawnManager（怪物消失）行为 GameTest。
//
// 覆盖 wiki tech_怪物消失.txt 核心清除机制：和平难度立即清除敌对生物、非和平难度持久存活。
//
// C++ 链路：DespawnManager::tick（DespawnManager.cpp:46-101）由 ServerDimension::tick 每 tick 调用，
//   遍历所有 MobEntity 调 shouldDespawn：
//   - 和平清除分支（:114）：difficulty==Peaceful && mob.isDespawnPeaceful() → return true（立即 remove）。
//     此分支在持久化短路（:119 isNoDespawnRequired||preventDespawn）之前，故 test.spawn 的持久化怪物
//     （enablePersistence）在和平仍被清——因 MonsterEntity::isDespawnPeaceful()=true（MonsterEntity.hpp:140）。
//   - 持久化短路（:119）：命名/桶装/骑乘等持久化生物永不消失（return false）。
//   - >128 格立即清除（:148）：closestPlayerDistSq > despawnDistance²(128²) && canDespawn → remove。
//   - >32 格随机清除（:153）：距玩家>32 且 idleTime>600 时 1/800 概率 remove。
//
// 关键事实：
//   1. test.spawn 的 Mob 自动 enablePersistence（GameTestHelper.cpp:789），但和平清除优先于持久化短路，
//      故和平难度下 test.spawn 的僵尸仍被清（验证 isDespawnPeaceful 门控）。
//   2. /difficulty peaceful 命令已实现（DifficultyCommand.cpp），创造玩家 permLevel=2 可执行。
//   3. remove() 后实体下一 tick 从 m_entities 移除（非立即），pollUntilSucceed 轮询捕获。
//   4. 僵尸是亡灵白天露天燃烧，hard 测试用 night batch 避免燃烧死亡干扰（区分"被 DespawnManager 清"
//      与"烧死"）。
//
// 判定手段：getEntities 区域计数。和平清除后 zombie==0；hard 存活 zombie>=1。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_怪物消失.txt（和平清除、距离清除）
// Ref: DespawnManager.cpp:103-158（shouldDespawn 和平清除优先于持久化短路）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 和平难度下敌对生物立即消失（wiki tech_怪物消失.txt：和平难度下所有敌对生物立即清除）。
//
// spawn zombie（test.spawn 设 persistence，但和平清除优先）+ 创造玩家 chat("/difficulty peaceful") →
// DespawnManager::tick 和平清除分支 isDespawnPeaceful()=true → remove。pollUntilSucceed 断言 zombie==0。
//
// 时序：/difficulty peaceful 生效后下一 DespawnManager::tick（每 tick）即清除，remove 后下一 tick 出列表。
// 命令执行 + DespawnManager tick + remove 移除约 2-5 tick，maxTick=200 留充裕余量。
//
// 注：chat 执行命令仅 Cubium 端有效（基岩 BDS chat 是发消息语义），本测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_怪物消失.txt（和平难度立即清除敌对生物）
function monsterDespawnsOnPeaceful(test: Test): void {
    const zombieType = "zombie";

    // 僵尸 spawn 于 (3,2,3)，创造玩家 (4,2,3) 旁（用于执行命令；玩家存在不影响和平清除）。
    test.spawn(zombieType, { x: 3, y: 2, z: 3 });
    const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "op");

    // 创造玩家 permLevel=2 执行 /difficulty peaceful。
    player.chat("/difficulty peaceful");

    // 轮询：和平清除后 zombie==0。
    pollUntilSucceed(test, () => {
        const zombies = test.getDimension().getEntities({
            type: zombieType,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return zombies.length === 0;
    }, {
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `zombie did not despawn on peaceful (zombie=${test.getDimension().getEntities({
                type: zombieType, location: test.worldLocation(PIT_FROM), volume: PIT_VOLUME,
            }).length})`),
    });
}

// 困难难度下怪物持续存活（对照：非和平难度 DespawnManager 不因难度清除）。
//
// spawn zombie + 创造玩家 chat("/difficulty hard")（默认 Normal，显式 hard）→ DespawnManager 无和平清除，
// 僵尸在玩家旁（<32 格）idleTime 重置，不触发距离清除，持续存活。pollUntilSucceed 断言 zombie>=1。
//
// 时序：命令生效后僵尸持续存活，等 100 tick（覆盖若干 DespawnManager tick）后断言仍存在。
// night batch 避免亡灵白天燃烧死亡（区分"存活"与"烧死后消失"）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_怪物消失.txt（非和平难度不因难度清除）
function monsterSurvivesOnHard(test: Test): void {
    const zombieType = "zombie";

    test.spawn(zombieType, { x: 3, y: 2, z: 3 });
    const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "op");
    player.chat("/difficulty hard");

    // 等 100 tick 后断言僵尸仍存活（DespawnManager 未清除）。
    test.runAtTickTime(100, () => {
        const zombies = test.getDimension().getEntities({
            type: zombieType,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        test.assert(zombies.length >= 1,
            `zombie despawned on hard difficulty (zombie=${zombies.length})`);
        test.succeed();
    });
}

export function registerDespawnTests(): void {
    GameTest.register("MobBehaviorTests", "monster_despawns_on_peaceful", monsterDespawnsOnPeaceful)
        .structureName("gametests:glass_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "monster_survives_on_hard", monsterSurvivesOnHard)
        .batch("night")
        .structureName("gametests:glass_pit")
        .maxTicks(150);
}
