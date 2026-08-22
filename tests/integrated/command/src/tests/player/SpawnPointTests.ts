// /spawnpoint 命令 GameTest：设置玩家重生点。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/spawnpoint.txt）：
//   - /spawnpoint：设置自己的重生点到当前位置（_setSelfSpawnPoint，source.player()）
//   - /spawnpoint <player>：设置指定玩家重生点到其当前位置（_setPlayerSpawnPoint）
//   - /spawnpoint <player> <pos>：设置指定玩家重生点到指定位置（_setPlayerSpawnPointAtPosition）
//
// 设计要点：
//   1. SpawnPointCommand 调 player->setSpawnPoint(dimensionId, spawnPos, false) 写 Player::m_spawnPoint
//      （optional<GlobalPos>）。脚本侧此前无 Player.getSpawnPoint 绑定，无法读重生点断言。本批测试
//      伴随新增 Player.getSpawnPoint readonlyProperty（Cubium 扩展，官方基岩 API 无）：返回
//      {x,y,z,dimensionId} 对象，无重生点返 undefined。
//   2. SimulatedPlayer::chat 传 this 作为命令源 player → ServerCommandSource 构造 dynamic_cast<ServerPlayer*>
//      成功 → source.player() 返回 SimulatedPlayer，_setSelfSpawnPoint（/spawnpoint 无参）可用。
//   3. /spawnpoint <pos> 用 Vec3ArgumentType，命令传世界绝对坐标（worldCoords 转换）。setSpawnPoint 内部
//      floor 取整，故断言用 floor(worldLocation)。
//   4. 默认重生点：SimulatedPlayer 构造时 m_spawnPoint=nullopt（无重生点），getSpawnPoint 返 undefined。
//   5. 重生点是玩家级状态（Player::m_spawnPoint），每个 SimulatedPlayer 独立实例，无跨测试污染。
//   6. SimulatedPlayer::chat permLevel 固定=4（≥2 满足 spawnpoint 权限）。
//   7. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_重生点.txt（设置重生点命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 重生点快照（Cubium 扩展 Player.getSpawnPoint 返回）。
interface SpawnPoint {
    x: number;
    y: number;
    z: number;
    dimensionId: number;
}

// 读玩家重生点（Cubium 扩展属性，TS 无类型用 as any）。无重生点返 undefined。
function getSpawnPoint(player: unknown): SpawnPoint | undefined {
    return (player as any).getSpawnPoint as SpawnPoint | undefined;
}

// 结构相对坐标 → 世界绝对坐标字符串（命令用）。
function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// 默认重生点为 undefined（SimulatedPlayer 构造 m_spawnPoint=nullopt）。
// 对照组：验证未执行 /spawnpoint 前 getSpawnPoint 返 undefined。
function spawnpointDefaultUndefined(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    test.assert(getSpawnPoint(player) === undefined, "default spawn point should be undefined");
    test.succeed();
}

// /spawnpoint 无参设置自己重生点到当前位置（_setSelfSpawnPoint，source.player()）。
// 玩家在 PLAYER_POS=(5,1,5) 相对坐标，worldLocation 转世界坐标 floor 后应等于 getSpawnPoint。
// Ref: wiki commands/spawnpoint.txt（spawnpoint 设当前位置）
function spawnpointSelfSetsCurrentPosition(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/spawnpoint");

    const sp = getSpawnPoint(player);
    test.assert(sp !== undefined, "spawn point should be set after /spawnpoint");
    const expected = test.worldLocation(PLAYER_POS);
    test.assert(sp!.x === Math.floor(expected.x), `x should be ${Math.floor(expected.x)}, got ${sp!.x}`);
    test.assert(sp!.y === Math.floor(expected.y), `y should be ${Math.floor(expected.y)}, got ${sp!.y}`);
    test.assert(sp!.z === Math.floor(expected.z), `z should be ${Math.floor(expected.z)}, got ${sp!.z}`);
    test.succeed();
}

// /spawnpoint @s 设置指定玩家重生点到其当前位置（_setPlayerSpawnPoint，经 playerEntityManager）。
// 与无参区别：走 _setPlayerSpawnPoint 分支（带 player 选择器），用玩家 position 而非 source.position。
// Ref: wiki commands/spawnpoint.txt（spawnpoint <player>）
function spawnpointPlayerSelectorSetsCurrent(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/spawnpoint @s");

    const sp = getSpawnPoint(player);
    test.assert(sp !== undefined, "spawn point should be set after /spawnpoint @s");
    const expected = test.worldLocation(PLAYER_POS);
    test.assert(sp!.x === Math.floor(expected.x), `x should be ${Math.floor(expected.x)}, got ${sp!.x}`);
    test.assert(sp!.y === Math.floor(expected.y), `y should be ${Math.floor(expected.y)}, got ${sp!.y}`);
    test.assert(sp!.z === Math.floor(expected.z), `z should be ${Math.floor(expected.z)}, got ${sp!.z}`);
    test.succeed();
}

// /spawnpoint @s <pos> 设置指定位置（_setPlayerSpawnPointAtPosition，Vec3ArgumentType）。
// 传一个不同于玩家当前位置的坐标，断言 getSpawnPoint 等于该坐标 floor。
// Ref: wiki commands/spawnpoint.txt（spawnpoint <player> <pos>）
function spawnpointSetsSpecifiedPosition(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const target = { x: 3, y: 2, z: 3 }; // 结构相对坐标，不同于 PLAYER_POS

    player.chat(`/spawnpoint @s ${worldCoords(test, target)}`);

    const sp = getSpawnPoint(player);
    test.assert(sp !== undefined, "spawn point should be set after /spawnpoint @s <pos>");
    const expected = test.worldLocation(target);
    test.assert(sp!.x === Math.floor(expected.x), `x should be ${Math.floor(expected.x)}, got ${sp!.x}`);
    test.assert(sp!.y === Math.floor(expected.y), `y should be ${Math.floor(expected.y)}, got ${sp!.y}`);
    test.assert(sp!.z === Math.floor(expected.z), `z should be ${Math.floor(expected.z)}, got ${sp!.z}`);
    test.succeed();
}

// /spawnpoint @s <pos> 可覆盖之前设置的重生点：先设 (3,2,3)，再设 (4,2,4)，断言最终为后者。
// 验证 setSpawnPoint 覆盖语义（m_spawnPoint 赋值替换非累加）。
// Ref: wiki commands/spawnpoint.txt（spawnpoint 覆盖语义）
function spawnpointOverwritesPrevious(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const first = { x: 3, y: 2, z: 3 };
    const second = { x: 4, y: 2, z: 4 };

    player.chat(`/spawnpoint @s ${worldCoords(test, first)}`);
    player.chat(`/spawnpoint @s ${worldCoords(test, second)}`);

    const sp = getSpawnPoint(player);
    test.assert(sp !== undefined, "spawn point should be set");
    const expected = test.worldLocation(second);
    test.assert(sp!.x === Math.floor(expected.x), `x should be second pos ${Math.floor(expected.x)}, got ${sp!.x}`);
    test.assert(sp!.y === Math.floor(expected.y), `y should be second pos ${Math.floor(expected.y)}, got ${sp!.y}`);
    test.assert(sp!.z === Math.floor(expected.z), `z should be second pos ${Math.floor(expected.z)}, got ${sp!.z}`);
    test.succeed();
}

export function registerSpawnPointTests(): void {
    GameTest.register("CommandTests", "spawnpoint_default_undefined", spawnpointDefaultUndefined)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "spawnpoint_self_sets_current_position", spawnpointSelfSetsCurrentPosition)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "spawnpoint_player_selector_sets_current", spawnpointPlayerSelectorSetsCurrent)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "spawnpoint_sets_specified_position", spawnpointSetsSpecifiedPosition)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "spawnpoint_overwrites_previous", spawnpointOverwritesPrevious)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
