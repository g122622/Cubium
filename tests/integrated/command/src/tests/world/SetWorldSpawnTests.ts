// /setworldspawn 命令 GameTest：设置世界出生点。
//
// 覆盖 wiki 命令章节核心行为（对齐 MC 1.21.11 SetWorldSpawnCommand，Ref: Java
// net/minecraft/server/commands/SetWorldSpawnCommand.java）：
//   - /setworldspawn：pos=BlockPos.containing(source.position())（玩家位置 floor 成整数 BlockPos），
//     rotation=ZERO_ROTATION（yaw=0, pitch=0）。vanilla 无参不用玩家朝向。
//   - /setworldspawn <pos>：pos=BlockPosArgument.getSpawnablePos（整数 BlockPos，无 centerCorrect 偏移），
//     rotation=ZERO_ROTATION（yaw=0, pitch=0）。
//   - /setworldspawn <pos> <rotation>：pos 同上，rotation=RotationArgument（接 yaw pitch 两值），
//     yaw 存 ServerWorld::m_spawnAngle（wrapDegrees 归一化 [-180,180]），pitch 暂丢弃（TODO 未建模）。
//
// 对齐修复背景（本批测试发现的偏差，已修 SetWorldSpawnCommand.cpp）：
//   1. pos 曾用 Vec3ArgumentType（centerCorrect=true 给绝对整数加 0.5 偏移到方块中心），致出生点偏 0.5，
//      vanilla 用 BlockPosArgument（整数 floor 无偏移）。已改 BlockPosArgumentType。
//   2. 无参分支 rotation 曾用 source.rotation().y（玩家当前朝向），vanilla 用 ZERO_ROTATION(0,0)。
//      已改为 0.0。
//   3. <rotation> 曾用 FloatArgumentType（单 angle），vanilla 用 RotationArgument（yaw pitch 两值）。
//      已改 RotationArgumentType，命令语法 /setworldspawn <pos> <yaw> <pitch>。
//
// 设计要点：
//   1. 世界出生点是世界级单例（ServerWorld::m_worldSpawnPoint 全局唯一），跨测试持久化不自动重置。故每个
//      测试独占 batch 串行 + runOnFinish 恢复初始出生点（测试开始时读 getDefaultSpawn 保存，结束时设回），
//      防污染后续依赖默认出生点的测试。
//   2. 默认出生点是 ServerWorld 启动时 climate search 算出的随机位置（不可预知），故不测"默认值"，
//      只测"set 后读回等于设置值"。
//   3. pos 存整数 BlockPos（.0），断言 expected 用 Math.floor(worldLocation(rel))（结构相对坐标 + 整数原点
//      本就是整数，floor 防御性）。worldCoords 传命令也用 floor 整数。
//   4. 无参分支 pos=floor(玩家实体位置)。玩家 spawn 在 PLAYER_POS 方块，实体在方块中心(+0.5)，floor 后
//      等于方块坐标=worldLocation(PLAYER_POS) 的 floor。故 expected=floor(worldLocation(PLAYER_POS))。
//   5. SimulatedPlayer::chat permLevel 固定=4（≥2 满足 setworldspawn 权限）。
//   6. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_世界出生点.txt（设置世界出生点命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 世界出生点快照（Cubium 扩展 world.getDefaultSpawn 返回）。
interface WorldSpawn {
    x: number;
    y: number;
    z: number;
    angle: number;
}

// 读世界出生点（Cubium 扩展属性，TS 无类型用 as unknown as）。
function getDefaultSpawn(): WorldSpawn | undefined {
    return (world as unknown as { getDefaultSpawn: WorldSpawn }).getDefaultSpawn;
}

// 结构相对坐标 → 世界绝对坐标字符串（命令用，整数 floor）。pos 存整数 BlockPos，故传 floor 整数。
function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// 结构相对坐标 → 世界绝对坐标（整数 floor），用于断言 expected（pos 存整数 BlockPos）。
function worldFloor(test: Test, rel: { x: number; y: number; z: number }): { x: number; y: number; z: number } {
    const w = test.worldLocation(rel);
    return { x: Math.floor(w.x), y: Math.floor(w.y), z: Math.floor(w.z) };
}

// /setworldspawn 无参设置当前位置为世界出生点。
// 对齐 vanilla: pos=BlockPos.containing(source.position())（玩家位置 floor 整数），rotation=ZERO_ROTATION(0,0)。
// 玩家在 PLAYER_POS=(5,1,5) 方块，实体在方块中心(+0.5)，floor 后=方块坐标=worldFloor(PLAYER_POS)。
// 故 getDefaultSpawn 应等于 worldFloor(PLAYER_POS)，angle=0。
// Ref: Java SetWorldSpawnCommand.java line 23（无参 setSpawn）
function setworldspawnSelfSetsCurrent(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = getDefaultSpawn();

    player.chat("/setworldspawn");

    const sp = getDefaultSpawn();
    test.assert(sp !== undefined, "world spawn should be defined after /setworldspawn");
    const expected = worldFloor(test, PLAYER_POS);
    test.assert(sp!.x === expected.x, `x should be ${expected.x}, got ${sp!.x}`);
    test.assert(sp!.y === expected.y, `y should be ${expected.y}, got ${sp!.y}`);
    test.assert(sp!.z === expected.z, `z should be ${expected.z}, got ${sp!.z}`);
    test.assert(sp!.angle === 0, `angle should be 0 (ZERO_ROTATION), got ${sp!.angle}`);

    test.runOnFinish(() => {
        // 恢复初始世界出生点（世界级单例，防污染后续测试）。
        if (initial !== undefined) {
            player.chat(`/setworldspawn ${Math.floor(initial.x)} ${Math.floor(initial.y)} ${Math.floor(initial.z)}`);
        }
    });
    test.succeed();
}

// /setworldspawn <pos> 设置指定位置（整数 BlockPos），rotation=ZERO_ROTATION(0,0)。
// 传一个不同于玩家当前位置的坐标，断言 getDefaultSpawn 等于该整数坐标，angle=0。
// Ref: Java SetWorldSpawnCommand.java line 28（<pos> setSpawn，ZERO_ROTATION）
function setworldspawnSetsSpecifiedPosition(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = getDefaultSpawn();
    const target = { x: 3, y: 2, z: 3 };

    player.chat(`/setworldspawn ${worldCoords(test, target)}`);

    const sp = getDefaultSpawn();
    test.assert(sp !== undefined, "world spawn should be defined after /setworldspawn <pos>");
    const expected = worldFloor(test, target);
    test.assert(sp!.x === expected.x, `x should be ${expected.x}, got ${sp!.x}`);
    test.assert(sp!.y === expected.y, `y should be ${expected.y}, got ${sp!.y}`);
    test.assert(sp!.z === expected.z, `z should be ${expected.z}, got ${sp!.z}`);
    test.assert(sp!.angle === 0, `angle should be 0 (ZERO_ROTATION), got ${sp!.angle}`);

    test.runOnFinish(() => {
        if (initial !== undefined) {
            player.chat(`/setworldspawn ${Math.floor(initial.x)} ${Math.floor(initial.y)} ${Math.floor(initial.z)}`);
        }
    });
    test.succeed();
}

// /setworldspawn <pos> <yaw> <pitch> 设置指定位置和朝向。
// 对齐 vanilla: rotation=RotationArgument（接 yaw pitch）。传 yaw=90 pitch=0，断言 angle=90。
// Ref: Java SetWorldSpawnCommand.java line 33（<pos> <rotation> setSpawn）
function setworldspawnSetsRotation(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = getDefaultSpawn();
    const target = { x: 3, y: 2, z: 3 };

    player.chat(`/setworldspawn ${worldCoords(test, target)} 90 0`);

    const sp = getDefaultSpawn();
    test.assert(sp !== undefined, "world spawn should be defined after /setworldspawn <pos> <rotation>");
    test.assert(sp!.angle === 90, `angle should be 90, got ${sp!.angle}`);

    test.runOnFinish(() => {
        if (initial !== undefined) {
            player.chat(`/setworldspawn ${Math.floor(initial.x)} ${Math.floor(initial.y)} ${Math.floor(initial.z)}`);
        }
    });
    test.succeed();
}

// /setworldspawn <pos> <yaw> <pitch> yaw 归一化：传 270 应归一化为 -90（wrapDegrees [-180,180]）。
// 验证 math::wrapDegrees 归一化语义（270-360=-90）。
// Ref: Java SetWorldSpawnCommand（Cubium 显式 wrapDegrees 保证存储规范化）
function setworldspawnRotationNormalized(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const initial = getDefaultSpawn();
    const target = { x: 3, y: 2, z: 3 };

    player.chat(`/setworldspawn ${worldCoords(test, target)} 270 0`);

    const sp = getDefaultSpawn();
    test.assert(sp !== undefined, "world spawn should be defined");
    test.assert(sp!.angle === -90, `angle 270 should normalize to -90, got ${sp!.angle}`);

    test.runOnFinish(() => {
        if (initial !== undefined) {
            player.chat(`/setworldspawn ${Math.floor(initial.x)} ${Math.floor(initial.y)} ${Math.floor(initial.z)}`);
        }
    });
    test.succeed();
}

export function registerSetWorldSpawnTests(): void {
    // 世界出生点是世界级单例，独占 batch 串行 + runOnFinish 恢复初始值防污染。
    GameTest.register("CommandTests", "setworldspawn_self_sets_current", setworldspawnSelfSetsCurrent)
        .structureName("gametests:cmd_arena")
        .batch("worldspawn_self_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "setworldspawn_sets_specified_position", setworldspawnSetsSpecifiedPosition)
        .structureName("gametests:cmd_arena")
        .batch("worldspawn_pos_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "setworldspawn_sets_rotation", setworldspawnSetsRotation)
        .structureName("gametests:cmd_arena")
        .batch("worldspawn_rot_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "setworldspawn_rotation_normalized", setworldspawnRotationNormalized)
        .structureName("gametests:cmd_arena")
        .batch("worldspawn_norm_solo")
        .maxTicks(60);
}
