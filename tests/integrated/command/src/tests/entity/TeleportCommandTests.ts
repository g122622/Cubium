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

/** 取结构空气腔内所有玩家实体的 location 列表（区域限定避免选到同批并行测试的玩家）。 */
function getPlayerLocations(test: Test): { x: number; y: number; z: number }[] {
    return test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(AREA_FROM),
        volume: AREA_VOLUME,
    }).map(p => p.location);
}

/** 统计 location 列表中接近目标坐标（误差 <1.5 格水平）的玩家数。 */
function countPlayersNear(locations: { x: number; y: number; z: number }[], target: { x: number; y: number; z: number }): number {
    return locations.filter(loc => Math.abs(loc.x - target.x) < 1.5 && Math.abs(loc.z - target.z) < 1.5).length;
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
    // 玩家名须全服独一：PlayerResolver::findPlayerIdByUsername 的 SimulatedPlayer 回退分支
    // （PlayerResolver.cpp:197-202）遍历全服实体按 username 匹配返回第一个命中。若其他并行
    // 测试也 spawn 了同名玩家（如 SpreadPlayersTests 的 "mover"），/tp <名字> 会命中错误实体，
    // 本测试玩家不被传送 → loc=null 超时失败。用 "tpMover" 避开冲突。
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "tpMover");
    const targetRel = { x: 4, y: 4, z: 4 };
    // 用显式名字 "tpMover" 作为 targets。
    player.chat(`/tp tpMover ${worldCoords(test, targetRel)}`);

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

// /tp @a[distance=..N] <x> <y> <z> 批量传送多玩家到坐标（走 _teleportTargetToPosition 多目标分支）。
// spawn 2 个 SimulatedPlayer 在结构内不同位置，@a[distance=..20]（以命令源为中心）选中两者，
// tp 到同一目标坐标，断言两个玩家都被传送（验证 resolvePlayerIds 多结果 + teleportPlayers 循环）。
// distance=..20 区域限定避免选中 73 格外同批并行测试的 SimulatedPlayer（污染防护）。
// 走 TeleportCommand::_teleportTargetToPosition（targetsArg=players() 多目标）。
// Ref: wiki teleport.txt（tp <targets> <location> 批量传送多玩家）
function tpAllPlayersToPosition(test: Test): void {
    // 两玩家在空气腔 y=2 站立层（下方 y=1 stone 地板支撑），不同位置。
    const playerA = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 2 }, "moverA");
    test.spawnSimulatedPlayer({ x: 6, y: 2, z: 6 }, "moverB");
    const targetRel = { x: 4, y: 3, z: 4 };

    // 等 moverB 生成稳定后执行（@a 解析需两玩家都已注册到 ServerPlayerEntityManager）。
    test.runAtTickTime(5, () => {
        // @a[distance=..20] 以 playerA 位置为中心，选中结构内两玩家（间距约 5.6 格 < 20）。
        playerA.chat(`/tp @a[distance=..20] ${worldCoords(test, targetRel)}`);
    });

    pollUntilSucceed(test, () => {
        const locs = getPlayerLocations(test);
        if (locs.length < 2) return false;
        const target = test.worldLocation(targetRel);
        // 两个玩家都应被传到目标附近。
        return countPlayersNear(locs, target) >= 2;
    }, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => {
            const locs = getPlayerLocations(test);
            const target = test.worldLocation(targetRel);
            test.assert(false,
                `tp @a did not move both players to target ${target.x},${target.z}; ` +
                `players=${locs.length}, near=${countPlayersNear(locs, target)}, locs=${JSON.stringify(locs)}`);
        },
    });
}

// /tp @a[distance=..N] <destinationPlayer> 批量传送多玩家到目标玩家位置（走 _teleportTargetToEntity 多目标分支）。
// spawn 2 个 mover + 1 个 dest 玩家，@a[distance=..20] 选中全部 3 玩家，传到 dest 玩家位置。
// dest 自身被选中传到自己位置（无位移），两 mover 被传到 dest 位置，3 玩家最终聚集在 dest 位置。
// 走 TeleportCommand::_teleportTargetToEntity（targetsArg 多目标 + destinationArg 单目标）。
// 注：不读 dest 句柄的 location——SimulatedPlayer 句柄在传送后 .location 可能失效（句柄生命周期问题），
// 改用 dest 的预期世界坐标（spawn 位置不变，dest 传到自己位置无位移）。
// Ref: wiki teleport.txt（tp <targets> <destination> 批量传送到目标玩家）
function tpAllPlayersToDestinationPlayer(test: Test): void {
    const playerA = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 2 }, "moverA");
    test.spawnSimulatedPlayer({ x: 6, y: 2, z: 6 }, "moverB");
    // dest 玩家在 (4,2,4)，作为传送目的地（传送后仍在该位）。
    test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "dest");
    // dest 预期世界坐标（spawn 位置，传送后不变）。
    const destRel = { x: 4, y: 2, z: 4 };

    test.runAtTickTime(5, () => {
        // @a[distance=..20] 选中结构内全部 3 玩家，传到 dest 名字玩家位置。
        // dest 自身被选中传到自己位置（无位移），两 mover 被传到 dest 位置。
        playerA.chat(`/tp @a[distance=..20] dest`);
    });

    pollUntilSucceed(test, () => {
        const locs = getPlayerLocations(test);
        if (locs.length < 3) return false;
        const destWorld = test.worldLocation(destRel);
        // 3 玩家都应聚集在 dest 位置附近（两 mover 被传到 dest + dest 自身原地）。
        return countPlayersNear(locs, destWorld) >= 3;
    }, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => {
            const locs = getPlayerLocations(test);
            const destWorld = test.worldLocation(destRel);
            test.assert(false,
                `tp @a dest did not move both movers to dest ${destWorld.x},${destWorld.z}; ` +
                `players=${locs.length}, near=${countPlayersNear(locs, destWorld)}, locs=${JSON.stringify(locs)}`);
        },
    });
}

// /tp <x> <y> <z> 在越界坐标（Y 超出 ±20,000,000 可生成高度边界）应被守卫拦截不传送玩家
// （对齐 Java TeleportCommand.performTeleport 首行守卫 TeleportCommand.java:254-256：
// !Level.isInSpawnableBounds(BlockPos.containing(pos)) → 抛 INVALID_POSITION）。
//
// 守卫位置：teleportPlayers 函数入口（所有 /tp 传送路径的唯一汇聚点），复用 world::isInSpawnableBounds
// （同 SummonCommand 边界守卫范式）。修复前 teleportPlayers 无任何边界校验，越界坐标直接
// setPosition/requestTeleport 可能崩溃或产生越界实体。
//
// 测试用绝对坐标 Y=21,000,000（刚越 isInSpawnableBounds 上界 20,000,000）触发守卫：
// /tp 0 21000000 0 → 边界守卫 sendError(commands.teleport.invalidPosition) 返回 0，玩家不移动，
// location 仍在初始 PLAYER_POS(5,1,5) 附近（未被传送）。
// 用绝对坐标而非 worldCoords，因越界坐标本就在结构外。
//
// 对照见 tpAllowedWithinBounds（合法坐标玩家正常传送，证明守卫不误拦合法坐标）。
// Ref: wiki teleport.txt（tp 对越界坐标拒绝传送）
// Ref: TeleportCommand.cpp（边界校验守卫 isInSpawnableBounds）、TeleportCommand.java:254-256（vanilla 守卫）
function tpRejectedForOutOfBoundsY(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // Y=21000000 越上界触发边界守卫。X/Z=0 在世界边界内合法，Y 是唯一越界变量，确保是边界守卫拦截。
    player.chat("/tp 0 21000000 0");

    // 轮询断言玩家未被传送（仍停在初始位置附近）。修复前玩家可能被传走致 location 偏离初始位置。
    // 初始位置 PLAYER_POS(5,1,5) 经 worldLocation 转世界坐标，玩家 location 应始终接近该坐标。
    const initialWorld = test.worldLocation(PLAYER_POS);
    pollUntilSucceed(test, () => {
        const loc = getPlayerLocation(test);
        if (loc === null) return false;
        // 玩家未被传送：location 仍在初始位置附近（误差 < 1.5 格水平）。
        return Math.abs(loc.x - initialWorld.x) < 1.5 && Math.abs(loc.z - initialWorld.z) < 1.5;
    }, {
        startTick: 5,
        maxTick: 80,
        onTimeout: () => {
            const loc = getPlayerLocation(test);
            test.assert(false,
                `tp should be rejected for out-of-bounds Y but player moved ` +
                `(initial=${initialWorld.x},${initialWorld.z}, loc=${JSON.stringify(loc)})`);
        },
    });
}

// /tp <x> <y> <z> 在合法坐标内正常传送玩家（对照 tpRejectedForOutOfBoundsY）。
//
// 守卫 isInSpawnableBounds 对合法坐标（结构内 Y∈[1,5]、X/Z∈[1,7]）放行。
// /tp <targetRel> → 守卫通过 → 传送玩家到 targetRel。pollUntilSucceed 断言玩家 location 接近 targetRel。
// 证明边界守卫只拦越界不误拦合法坐标。
// Ref: wiki teleport.txt（tp 合法坐标正常传送）
function tpAllowedWithinBounds(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const targetRel = { x: 2, y: 2, z: 2 };
    player.chat(`/tp ${worldCoords(test, targetRel)}`);

    pollUntilSucceed(test, () => {
        const loc = getPlayerLocation(test);
        if (loc === null) return false;
        const target = test.worldLocation(targetRel);
        return Math.abs(loc.x - target.x) < 1.5 && Math.abs(loc.z - target.z) < 1.5;
    }, {
        maxTick: 60,
        onTimeout: () => {
            const loc = getPlayerLocation(test);
            const target = test.worldLocation(targetRel);
            test.assert(false,
                `tp should succeed within bounds but player not moved ` +
                `(target=${target.x},${target.z}, loc=${JSON.stringify(loc)})`);
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

    GameTest.register("CommandTests", "tp_all_players_to_position", tpAllPlayersToPosition)
        .structureName("gametests:cmd_arena")
        .maxTicks(120);

    GameTest.register("CommandTests", "tp_all_players_to_destination_player", tpAllPlayersToDestinationPlayer)
        .structureName("gametests:cmd_arena")
        .maxTicks(120);

    // 边界校验测试：守卫不修改世界级状态（不改难度/gamerule），可用默认 batch 并行，无需独占。
    GameTest.register("CommandTests", "tp_rejected_for_out_of_bounds_y", tpRejectedForOutOfBoundsY)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);

    GameTest.register("CommandTests", "tp_allowed_within_bounds", tpAllowedWithinBounds)
        .structureName("gametests:cmd_arena")
        .maxTicks(100);
}
