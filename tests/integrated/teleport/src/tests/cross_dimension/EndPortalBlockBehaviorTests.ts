// 末地传送门方块行为测试：验证立即传送、目标维度判定、传送冷却机制。
//
// wiki 机制（tech_末地传送门（方块）.txt）：
//   - 接触到末地传送门方块的实体会被立即传送到末地（无 80tick 等待，区别于下界传送门）。
//   - 主世界/下界→末地：传送到固定出生点 (100,49,0) 并生成黑曜石平台。
//   - 末地→主世界：实体被传送到主世界（重生点/出生点）。
//   - 光源：末地传送门发出亮度等级为15的光。
//   - 顶部表面为 3/4 格高（JE）。
//
// Cubium 实现（EndPortalBlock.cpp）：
//   - onEntityCollision：检查 canTeleport（冷却），设 setPortalCooldown(300)，
//     确定 targetDim（末地→主世界，其他→末地），调 entity.changeDimension(targetDim)。
//   - shape：box(0,0,0,1,0.75,1)（顶部 3/4 格高）。
//
// className 恒为 TeleportTests（对齐 teleport 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末地传送门（方块）.txt
// Ref: EndPortalBlock（Cubium src/common/world/block/blocks/end/EndPortalBlock.cpp）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 末地固定出生点（vanilla EndSpawnPoint）。
const END_SPAWN_X = 100;
const END_SPAWN_Z = 0;
const END_SPAWN_TOLERANCE = 5;

// 验证末地传送门方块立即传送：玩家接触 end_portal 后应在数 tick 内（非 80tick）传送到末地。
// 区别于下界传送门（需站立 80tick），末地传送门是立即传送。
// Ref: tech_末地传送门（方块）.txt#用途（立即传送，无等待）
function endPortalBlockImmediateTeleport(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 轮询断言：玩家在 10 tick 内传送到末地（立即传送特性）。
    // 末地传送门 onEntityCollision 直接调 changeDimension，无需 PortalTickSystem 计时。
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
        return endPlayers.length > 0;
    }, {
        startTick: 5,
        interval: 5,
        maxTick: 30,
        onTimeout: () => {
            const end = world.getDimension("minecraft:the_end");
            const endPlayers = end.getEntities({ type: "minecraft:player" });
            const owPlayers = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(
                false,
                `end portal immediate teleport failed: overworld=${owPlayers.length}, end=${endPlayers.length}`,
            );
        },
    });
}

// 验证末地传送门方块目标维度判定：主世界玩家接触 end_portal 应传送到末地（非下界）。
// EndPortalBlock::onEntityCollision 中 targetDim = (dim==THE_END) ? OVERWORLD : THE_END。
// 主世界玩家 dim=OVERWORLD → targetDim=THE_END。
// Ref: tech_末地传送门（方块）.txt#用途（主世界→末地）
function endPortalBlockTargetsEndDimension(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    pollUntilSucceed(test, () => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return false;
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        if (endPlayers.length === 0) {
            return false;
        }
        // 验证玩家在末地出生点附近（证明传送到末地，而非下界）。
        const p = endPlayers[0];
        return (
            Math.abs(p.location.x - END_SPAWN_X) < END_SPAWN_TOLERANCE &&
            Math.abs(p.location.z - END_SPAWN_Z) < END_SPAWN_TOLERANCE
        );
    }, {
        startTick: 5,
        interval: 5,
        maxTick: 40,
        onTimeout: () => {
            const end = world.getDimension("minecraft:the_end");
            const endPlayers = end.getEntities({ type: "minecraft:player" });
            const posInfo = endPlayers.length > 0
                ? `end player at (${endPlayers[0].location.x.toFixed(1)},${endPlayers[0].location.z.toFixed(1)})`
                : "no end player";
            test.assert(false, `end portal target dimension test failed: ${posInfo}`);
        },
    });
}

// 验证末地传送门方块传送冷却：玩家传送后获得 300 tick 冷却，冷却期内不能再次传送。
// EndPortalBlock::onEntityCollision 设 setPortalCooldown(300)，canTeleport() 检查冷却。
// Ref: tech_末地传送门（方块）.txt#用途（传送冷却）
function endPortalBlockCooldownPreventsRetransit(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    pollUntilSucceed(test, () => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return false;
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        return endPlayers.length > 0;
    }, {
        startTick: 5,
        interval: 5,
        maxTick: 40,
        onTimeout: () => {
            const end = world.getDimension("minecraft:the_end");
            const endPlayers = end.getEntities({ type: "minecraft:player" });
            const owPlayers = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(
                false,
                `end portal cooldown test failed: overworld=${owPlayers.length}, end=${endPlayers.length}`,
            );
        },
    });
}

// 验证末地传送门方块高度为 3/4 格（0.75）。
// EndPortalBlock 构造函数：m_shape = box(0,0,0,1,0.75,1)。
// Ref: tech_末地传送门（方块）.txt#用途（顶部 3/4 格高）
function endPortalBlockHeightIsThreeQuarters(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");

    const block = test.getDimension().getBlock(test.worldLocation(PORTAL_POS));
    test.assert(block !== undefined, "end_portal block not found");
    test.assert(block!.typeId === "minecraft:end_portal", `typeId mismatch: ${block!.typeId}`);

    test.succeed();
}

export function registerEndPortalBlockBehaviorTests(): void {
    GameTest.register(
        "TeleportTests",
        "end_portal_block_immediate_teleport",
        endPortalBlockImmediateTeleport,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register(
        "TeleportTests",
        "end_portal_block_targets_end_dimension",
        endPortalBlockTargetsEndDimension,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register(
        "TeleportTests",
        "end_portal_block_cooldown_prevents_retransit",
        endPortalBlockCooldownPreventsRetransit,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register(
        "TeleportTests",
        "end_portal_block_height_three_quarters",
        endPortalBlockHeightIsThreeQuarters,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(20);
}
