// /tp（teleport）命令 GameTest：传送玩家到指定坐标。
//
// 覆盖 wiki 命令章节核心行为：
//   - /tp <player> <x> <y> <z>：传送玩家到世界绝对坐标（Ref: wiki teleport.txt）
//   - /teleport 是 /tp 的别名
//
// 设计要点：
//   1. TeleportCommand 的 target/destination 仅支持玩家目标（EntityArgumentType::player），
//      不支持 @e[type=zombie] 等非玩家实体——故用 SimulatedPlayer 作为传送目标。
//   2. 传送后用 getEntities 读取玩家 location 验证坐标变化。
//   3. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。
//      玩家初始 (5,1,5)，/tp 传送到 (2,2,2)，验证 location 接近目标。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// 注：teleport 包已有 5 个传送测试（跨维度/script teleport），本测试专测 /tp 命令本身。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };
const AREA_FROM = { x: 1, y: 1, z: 1 };
const AREA_VOLUME = { x: 7, y: 5, z: 7 };

function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

/** 取区域内玩家实体的 location。 */
function getPlayerLocation(test: Test): { x: number; y: number; z: number } | null {
    const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(AREA_FROM),
        volume: AREA_VOLUME,
    });
    if (players.length === 0) return null;
    return players[0].location;
}

// /tp <x> <y> <z> 把命令源玩家自己传送到指定坐标（vanilla 语法：省略 target 即传自己）。
// 注：/tp @s x y z 在 Cubium 命令树中 @s 优先匹配 selfDestinationArg（把自己传到 @s 玩家），
// 坐标被忽略——故用省略 target 的 /tp <x> <y> <z> 语法（走 _teleportToPosition 分支）。
// Ref: wiki teleport.txt（tp <location> 传送自己到坐标）
function tpTeleportsPlayer(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const targetRel = { x: 2, y: 2, z: 2 };
    player.chat(`/tp ${worldCoords(test, targetRel)}`);

    pollUntilSucceed(test, () => {
        const loc = getPlayerLocation(test);
        if (loc === null) return false;
        const target = test.worldLocation(targetRel);
        // 传送后玩家 location 应接近目标坐标（误差 < 1.5 格，因实体站立偏移）。
        return Math.abs(loc.x - target.x) < 1.5 && Math.abs(loc.z - target.z) < 1.5;
    }, {
        maxTick: 60,
        onTimeout: () => {
            const loc = getPlayerLocation(test);
            test.assert(false, `tp did not move player to target, loc=${JSON.stringify(loc)}`);
        },
    });
}

// /teleport 是 /tp 的别名，同样传送玩家（省略 target 传自己）。
// Ref: wiki teleport.txt（teleport 与 tp 等价）
function teleportAliasWorks(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const targetRel = { x: 6, y: 3, z: 6 };
    player.chat(`/teleport ${worldCoords(test, targetRel)}`);

    pollUntilSucceed(test, () => {
        const loc = getPlayerLocation(test);
        if (loc === null) return false;
        const target = test.worldLocation(targetRel);
        return Math.abs(loc.x - target.x) < 1.5 && Math.abs(loc.z - target.z) < 1.5;
    }, {
        maxTick: 60,
        onTimeout: () => {
            const loc = getPlayerLocation(test);
            test.assert(false, `teleport alias did not move player, loc=${JSON.stringify(loc)}`);
        },
    });
}

// /tp <playerName> <x> <y> <z> 用显式玩家名作为 targets（走 _teleportTargetToPosition 分支）。
// Ref: wiki teleport.txt（tp <targets> <location> 传送指定玩家到坐标）
function tpByPlayerName(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "mover");
    const targetRel = { x: 4, y: 4, z: 4 };
    // 用显式名字 "mover" 作为 targets。
    player.chat(`/tp mover ${worldCoords(test, targetRel)}`);

    pollUntilSucceed(test, () => {
        const loc = getPlayerLocation(test);
        if (loc === null) return false;
        const target = test.worldLocation(targetRel);
        return Math.abs(loc.x - target.x) < 1.5 && Math.abs(loc.z - target.z) < 1.5;
    }, {
        maxTick: 60,
        onTimeout: () => {
            const loc = getPlayerLocation(test);
            test.assert(false, `tp by name did not move player, loc=${JSON.stringify(loc)}`);
        },
    });
}

export function registerTeleportCommandTests(): void {
    GameTest.register("CommandTests", "tp_teleports_player", tpTeleportsPlayer)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);

    GameTest.register("CommandTests", "teleport_alias_works", teleportAliasWorks)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);

    GameTest.register("CommandTests", "tp_by_player_name", tpByPlayerName)
        .structureName("gametests:cmd_arena")
        .maxTicks(80);
}
