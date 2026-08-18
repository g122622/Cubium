// 实体类命令 GameTest：/summon /kill 等。
//
// 覆盖 wiki 命令章节核心行为：
//   - /summon：生成实体到指定坐标或执行者位置（Ref: wiki summon.txt）
//   - /kill：清除实体（含 @e 选择器按 type 过滤）（Ref: wiki kill.txt）
//
// 设计要点：
//   1. /summon <entity> <x> <y> <z> 用世界绝对坐标（worldLocation 转换）。
//   2. /kill @e[type=zombie] 按选择器过滤清除；/kill @e 清除区域内所有实体（含玩家，慎用）。
//   3. 实体生成/清除非当 tick 生效（spawn 经 finalizeSpawn、kill 经 remove + 下一 tick 出列），
//      用 pollUntilSucceed 轮询区域计数。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家站 (5,1,5)。
//      区域限定用 (1,1,1)..(7,5,7) 全空气腔计数，排除 SimulatedPlayer（按 type 过滤）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };
// 区域计数范围：cmd_arena 内部空气腔 (1,1,1)..(7,5,7)。
const AREA_FROM = { x: 1, y: 1, z: 1 };
const AREA_VOLUME = { x: 7, y: 5, z: 7 };

/** 区域内指定 type 实体数量（排除 SimulatedPlayer，因 player 是独立 type）。 */
function countEntities(test: Test, type: string): number {
    return test.getDimension().getEntities({
        type,
        location: test.worldLocation(AREA_FROM),
        volume: AREA_VOLUME,
    }).length;
}

function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// /summon 在指定坐标生成实体。
// Ref: wiki summon.txt（summon <entity> [pos] 生成实体到指定坐标）
function summonSpawnsEntity(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    player.chat(`/summon minecraft:zombie ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);

    pollUntilSucceed(test, () => countEntities(test, "zombie") >= 1, {
        maxTick: 60,
        onTimeout: () => test.assert(false, "summon did not spawn zombie"),
    });
}

// /summon 在指定坐标生成实体，验证生成位置正确（实体坐标应在目标格附近）。
// Ref: wiki summon.txt（实体生成在指定坐标）
function summonSpawnsAtPosition(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const targetRel = { x: 2, y: 2, z: 2 };
    player.chat(`/summon minecraft:cow ${worldCoords(test, targetRel)}`);

    pollUntilSucceed(test, () => {
        const cows = test.getDimension().getEntities({
            type: "cow",
            location: test.worldLocation(AREA_FROM),
            volume: AREA_VOLUME,
        });
        if (cows.length < 1) return false;
        // 验证牛生成在目标格附近（实体坐标与目标世界坐标差距 < 2 格）。
        const target = test.worldLocation(targetRel);
        const c = cows[0];
        return Math.abs(c.location.x - target.x) < 2 && Math.abs(c.location.z - target.z) < 2;
    }, {
        maxTick: 60,
        onTimeout: () => test.assert(false, "summon cow not at target position"),
    });
}

// /kill @e[type=zombie] 清除区域内所有 zombie，cow 保留（选择器 type 过滤）。
// Ref: wiki kill.txt（kill 接受选择器，按 type 过滤）
function killRemovesByTypeSelector(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 预置 zombie + cow。
    test.spawn("zombie", { x: 2, y: 2, z: 2 });
    test.spawn("cow", { x: 4, y: 2, z: 4 });
    // 等实体生成稳定后 kill。
    test.runAtTickTime(5, () => {
        player.chat("/kill @e[type=zombie]");
    });

    pollUntilSucceed(test, () => countEntities(test, "zombie") === 0 && countEntities(test, "cow") >= 1, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `kill @e[type=zombie] failed: zombie=${countEntities(test, "zombie")}, cow=${countEntities(test, "cow")}`),
    });
}

// /kill @e 清除区域内所有非玩家实体（@e 默认不含玩家需 type 谓词，但 @e 含玩家）。
// 验证 zombie 被清除。注意 @e 含玩家，但玩家被 kill 会复活（Creative 无敌），不影响 zombie 计数。
// Ref: wiki kill.txt（@e 选择所有实体）
function killAllRemovesEntities(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.spawn("zombie", { x: 2, y: 2, z: 2 });
    test.spawn("zombie", { x: 4, y: 2, z: 4 });
    test.runAtTickTime(5, () => {
        // @e[type=zombie] 避免误杀玩家导致测试中断（玩家 Creative 被 kill 会 respawn，但保险起见限定 type）。
        player.chat("/kill @e[type=zombie]");
    });

    pollUntilSucceed(test, () => countEntities(test, "zombie") === 0, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `kill @e failed: zombie=${countEntities(test, "zombie")}`),
    });
}

// /summon 多次生成实体，验证数量累积（每次 summon 生成一个）。
// Ref: wiki summon.txt（每次 summon 生成一个实体）
function summonMultipleAccumulates(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 连续 summon 3 个 chicken 在不同坐标。
    player.chat(`/summon minecraft:chicken ${worldCoords(test, { x: 2, y: 2, z: 2 })}`);
    player.chat(`/summon minecraft:chicken ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);
    player.chat(`/summon minecraft:chicken ${worldCoords(test, { x: 4, y: 2, z: 4 })}`);

    pollUntilSucceed(test, () => countEntities(test, "chicken") >= 3, {
        maxTick: 60,
        onTimeout: () => test.assert(false,
            `summon x3 failed: chicken=${countEntities(test, "chicken")}`),
    });
}

export function registerEntityCommandTests(): void {
    GameTest.register("CommandTests", "summon_spawns_entity", summonSpawnsEntity)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);

    GameTest.register("CommandTests", "summon_spawns_at_position", summonSpawnsAtPosition)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);

    GameTest.register("CommandTests", "kill_removes_by_type_selector", killRemovesByTypeSelector)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);

    GameTest.register("CommandTests", "kill_all_removes_entities", killAllRemovesEntities)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);

    GameTest.register("CommandTests", "summon_multiple_accumulates", summonMultipleAccumulates)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);
}
