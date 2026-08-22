// /execute in <dimension> run tp @s <x> <y> <z> 跨维度传送测试。
//
// 覆盖 vanilla `execute in <dim> run tp @s <x> <y> <z>` 语义（对齐 MC Java 1.21.11
// TeleportCommand.java:188-229 teleportToPos + Entity.java:3212-3228 teleportTo）：
// execute in 切换命令源维度上下文（ExecuteCommand::_executeIn → source.withDimension），
// tp 用 source 所在维度作为目标 Level（vanilla teleportToPos 取 source.getLevel()），
// performTeleport 调 entity.teleportTo(targetLevel, x, y, z, ...)，目标维度与实体当前维度不同
// 时走 teleportCrossDimension（在目标 Level 创建实体 + 移除旧实体）。
//
// 修复前 Cubium TeleportCommand::teleportPlayers 不读 source.dimensionId()，仅同维度
// setPosition/requestTeleport。execute in 把 source.world() 切到目标维度后，getPlayerEntity
// 在目标维度 EntityManager 查不到仍留源维度的 SimulatedPlayer → 返 nullptr → 传送不执行 → 命令静默失败。
// 本次补全：teleportPlayers 按 source.dimensionId() 与实体当前维度比较，跨维度走新增的
// Entity::teleportToDimension（ServerPlayer override 复用 changeDimension 迁移逻辑 + 自定义坐标）。
//
// 设计要点：
//   1. /execute in minecraft:the_end run tp @s 0 64 0：SimulatedPlayer chat 执行，@s 解析为自身
//      （chat 传 this 作命令源 player，permLevel=4 可执行管理命令）。execute in 切源维度到末地，
//      tp @s 0 64 0 走跨维度路径 teleportToDimension(THE_END, (0,64,0))。
//   2. 断言主世界 chamber 无玩家 + 末地有玩家。位置断言用 x/z 容差（末地 (0,64,0) 附近地形未知，
//      玩家悬空会下落只影响 y；x/z 由 tp 坐标精确设定不受下落影响）。
//   3. 区别于末地传送门固定 (100,49,0)：tp 坐标传 (0,64,0)，x≈0 证明走 tp 命令坐标而非传送门出生点。
//   4. 下界用例验证另一维度（minecraft:the_nether）同样可跨维度传送。
//   5. 同维度对照：execute in minecraft:overworld run tp @s <坐标> 不跨维度，走同维度路径，
//      玩家留在主世界目标坐标附近，验证同维度路径不回归。
//
// className 恒为 TeleportTests（对齐 teleport 包约定）。
// Ref: ExecuteCommand.cpp _executeIn（维度切换）；TeleportCommand.cpp teleportPlayers（跨维度补全）
// Ref: ServerPlayer.cpp teleportToDimension/_performDimensionTransfer（跨维度迁移）
// Ref: TeleportCommand.java:188-229（vanilla teleportToPos）、Entity.java:3212-3228（vanilla teleportTo）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 末地出生平台坐标（vanilla EndSpawnPoint，changeDimension 内 getEndSpawnPosition 返回）。
// 用于区别：末地传送门传送落 (100,49,0)；tp 0 64 0 落 x≈0。x 坐标差异证明走 tp 命令坐标。
const END_PORTAL_SPAWN_X = 100;

// execute in minecraft:the_end run tp @s 0 64 0：跨维度传送到末地 (0,64,0)。
//
// SimulatedPlayer chat 执行 /execute in minecraft:the_end run tp @s 0 64 0：
// - execute in the_end：_executeIn 验证末地维度存在 → source.withDimension(THE_END)（末地 scale=1 无坐标缩放）
//   → modifiedSource 维度=THE_END，playerId/entity 不变。
// - run tp @s 0 64 0：嵌套执行，tp @s 0 64 0 走 _teleportTargetToPosition（@s=targets，0 64 0=坐标）。
// - resolvePlayerIds(source, @s) Self 分支返 source.playerId()（SimulatedPlayer playerId）。
// - teleportPlayers(source, {playerId}, (0,64,0))：targetDim=source.dimensionId()=THE_END；
//   getPlayerDimension(playerId)=OVERWORLD（SimulatedPlayer spawn 时 playerJoinDimension(OVERWORLD)）；
//   currentDim != targetDim → 跨维度路径：getPlayerEntity(playerId, *overworldWorld) 取实体 →
//   entity->teleportToDimension(THE_END, (0,64,0), rot) → _performDimensionTransfer 迁移 EntityManager
//   到末地 + setPosition(0,64,0)。
//
// 断言：主世界 chamber 无玩家（已迁移出）+ 末地有玩家且 x≈0（区别末地传送门 (100,49,0)，证明走 tp 坐标）。
// 修复前 teleportPlayers 不读维度，getPlayerEntity(末地世界) 查不到主世界实体返 nullptr，传送不执行，
// 玩家留在主世界 chamber → 主世界 players≠0，测试超时失败。
function executeInTeleportsPlayerToEnd(test: Test): void {
    // Creative 模式 SimulatedPlayer（permLevel 恒=4 可执行管理命令，与游戏模式解耦，Creative 防意外坠落死亡干扰）。
    const player = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "traveler");

    // tick 5 等 spawn 注册稳定后执行跨维度 tp 命令（chat 命令队列有 tick 延迟，tick5 执行 tick6~7 生效）。
    test.runAtTickTime(5, () => {
        player.chat("/execute in minecraft:the_end run tp @s 0 64 0");
    });

    // 轮询断言：主世界 chamber 无玩家 + 末地有玩家且 x≈0。
    // x/z 容差 8：末地 (0,64,0) 附近悬空下落只影响 y，x/z 由 tp 精确设定。x≈0 区别于末地传送门 (100,49,0)。
    pollUntilSucceed(test, () => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return false; // 主世界仍有玩家，未传送完成
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        if (endPlayers.length === 0) {
            return false; // 末地无玩家，传送未完成
        }
        const p = endPlayers[0];
        // x≈0 证明走 tp 命令坐标 (0,64,0)（区别末地传送门固定出生点 x=100）。
        return Math.abs(p.location.x) < 8 && Math.abs(p.location.z) < 8;
    }, {
        startTick: 15,
        interval: 5,
        maxTick: 80,
        onTimeout: () => {
            const end = world.getDimension("minecraft:the_end");
            const endPlayers = end.getEntities({ type: "minecraft:player" });
            const owPlayers = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const posInfo = endPlayers.length > 0
            ? `end player at (${endPlayers[0].location.x.toFixed(1)},${endPlayers[0].location.y.toFixed(1)},${endPlayers[0].location.z.toFixed(1)})`
            : "no end player";
            test.assert(false,
                `execute in the_end run tp @s 0 64 0 failed: overworld=${owPlayers.length}, ${posInfo} (expected end player near x=0, not portal spawn x=${END_PORTAL_SPAWN_X})`);
        },
    });
}

// execute in minecraft:the_nether run tp @s 0 64 0：跨维度传送到下界 (0,64,0)。
//
// 同末地用例，验证另一维度 the_nether 同样可跨维度传送（teleportToDimension 对下界同样生效）。
// 下界 scale=8，但 execute in 的坐标缩放在 _executeIn 内由 transformPosition 处理（主世界→下界 ÷8），
// 然而此处 tp @s 0 64 0 的坐标是绝对坐标 0 64 0（非相对源位置），execute in 缩放的是 source.position
// （源位置），不影响 tp 显式坐标。故下界落点 x≈0、z≈0。
function executeInTeleportsPlayerToNether(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "traveler");

    test.runAtTickTime(5, () => {
        player.chat("/execute in minecraft:the_nether run tp @s 0 64 0");
    });

    pollUntilSucceed(test, () => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return false;
        }
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        return netherPlayers.length > 0;
    }, {
        startTick: 15,
        interval: 5,
        maxTick: 80,
        onTimeout: () => {
            const nether = world.getDimension("minecraft:nether");
            const netherPlayers = nether.getEntities({ type: "minecraft:player" });
            const owPlayers = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(false,
                `execute in the_nether run tp @s 0 64 0 failed: overworld=${owPlayers.length}, nether=${netherPlayers.length}`);
        },
    });
}

// execute in minecraft:overworld run tp @s <坐标>：同维度传送（对照，验证同维度路径不回归）。
//
// execute in overworld 不改维度（源已在主世界），tp @s 0 64 0 走同维度路径（currentDim==targetDim）：
// SimulatedPlayer 不在 PlayerManager → requestTeleport 返 0 → 回退 getPlayerEntity + setPosition(0,64,0)。
// 玩家留在主世界，断言主世界 chamber 有玩家（未跨维度跑掉）+ 位置在 (0,64,0) 附近。
//
// 注：(0,64,0) 在主世界结构 glass_pit 外（glass_pit 相对 y∈[0,4]，worldLocation 偏移后 (0,64,0) 在结构上方
// 高空）。玩家会被传到 (0,64,0) 悬空下落。为稳定断言位置，改用结构内坐标 (3,2,3) 附近的世界绝对坐标，
// 即用 test.worldLocation 把结构相对 (3,2,3) 转世界坐标作为 tp 目标，断言玩家在该世界坐标附近。
function executeInSameDimensionTeleport(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "traveler");

    // 目标：结构相对 (5,2,5) → 世界绝对坐标。execute in overworld（同维度）+ tp @s 到该坐标。
    const targetWorld = test.worldLocation({ x: 5, y: 2, z: 5 });
    test.runAtTickTime(5, () => {
        player.chat(`/execute in minecraft:overworld run tp @s ${targetWorld.x} ${targetWorld.y} ${targetWorld.z}`);
    });

    // 轮询断言：主世界 chamber 有玩家且在目标坐标附近（同维度传送位置精确，未跨维度跑掉）。
    pollUntilSucceed(test, () => {
        const players = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation({ x: 4, y: 0, z: 4 }),
            volume: { x: 3, y: 5, z: 3 },
        });
        if (players.length === 0) {
            return false;
        }
        const p = players[0];
        return Math.abs(p.location.x - targetWorld.x) < 1.5 && Math.abs(p.location.z - targetWorld.z) < 1.5;
    }, {
        startTick: 15,
        interval: 5,
        maxTick: 70,
        onTimeout: () => {
            const players = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation({ x: 4, y: 0, z: 4 }),
                volume: { x: 3, y: 5, z: 3 },
            });
            const posInfo = players.length > 0
                ? `player at (${players[0].location.x.toFixed(1)},${players[0].location.y.toFixed(1)},${players[0].location.z.toFixed(1)})`
                : "no player near target";
            test.assert(false,
                `execute in overworld run tp same-dimension failed: ${posInfo} (expected near ${targetWorld.x},${targetWorld.y},${targetWorld.z})`);
        },
    });
}

export function registerExecuteInTeleportTests(): void {
    GameTest.register("TeleportTests", "execute_in_teleports_player_to_end", executeInTeleportsPlayerToEnd)
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register("TeleportTests", "execute_in_teleports_player_to_nether", executeInTeleportsPlayerToNether)
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register("TeleportTests", "execute_in_same_dimension_teleport", executeInSameDimensionTeleport)
        .structureName("gametests:glass_pit")
        .maxTicks(90);
}
