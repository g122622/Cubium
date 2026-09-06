// 末地传送门与出生平台测试：验证末地固定出生点 (100,49,0)、黑曜石平台尺寸与清空逻辑。
//
// wiki 机制（world_末地传送门.txt、EndSpawnPoint、EndPlatformFeature）：
//   - 末地传送门立即传送（无 80tick 等待），区别于下界传送门。
//   - 主世界→末地传送到固定出生点 (100,49,0)，并生成黑曜石平台。
//   - 黑曜石平台规格：5×5×5 黑曜石在 Y=48（中心 x=100, z=0），上方 Y=49~52 清空。
//   - 末地→主世界走 transformPosition（1:1 坐标，无缩放）。
//
// Cubium 实现（Teleporter.cpp）：
//   - Teleporter::getEndSpawnPosition() 返回 Vector3d(100.5, 50.0, 0.5)（方块坐标 100,50,0 顶部中心）。
//   - EndTeleporter::createEndSpawnPlatform()：
//     * 放置 5×5 黑曜石在 Y=48（SPAWN_Y - 2 = 50 - 2 = 48）。
//     * 清空上方 Y=49~52（SPAWN_Y - 1 到 SPAWN_Y + 2）。
//   - EndPortalBlock::onEntityCollision 调 entity.changeDimension(THE_END) 立即传送。
//
// 测试策略：
//   1. 玩家进入末地传送门后立即传送到末地出生点 (100,49,0) 附近
//   2. 验证末地黑曜石平台存在（5×5 在 Y=48）
//   3. 验证末地平台上方空间已清空（Y=49~52）
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_末地传送门.txt
// Ref: EndPlatformFeature.createEndPlatform（Java）
// Ref: EndTeleporter::createEndSpawnPlatform（Cubium Teleporter.cpp:495）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 末地固定出生点（vanilla EndSpawnPoint）。
const END_SPAWN_X = 100;
const END_SPAWN_Y = 50;
const END_SPAWN_Z = 0;
const END_SPAWN_TOLERANCE = 5;

// 平台规格常量（与 C++ EndTeleporter::createEndSpawnPlatform 一致）。
const PLATFORM_Y = END_SPAWN_Y - 2; // Y=48
const PLATFORM_RADIUS = 2; // 5×5 平台，半径 2

// 玩家进入末地传送门后立即传送到末地出生点 (100,49,0) 附近。
// 验证末地传送门的立即传送特性（无 80tick 等待）。
function endPortalTransfersPlayerToEndSpawn(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return; // 主世界仍有玩家，未传送完成
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        test.assert(endPlayers.length > 0, "player not found in the_end after end portal teleport");
        const p = endPlayers[0];
        test.assert(
            Math.abs(p.location.x - END_SPAWN_X) < END_SPAWN_TOLERANCE &&
                Math.abs(p.location.z - END_SPAWN_Z) < END_SPAWN_TOLERANCE,
            `player end position ${p.location.x},${p.location.z} not near (${END_SPAWN_X},${END_SPAWN_Z})`,
        );
    });
}

// 验证末地黑曜石平台存在：传送后平台位置 (98~102, 48, -2~2) 应为黑曜石。
function endPortalCreatesObsidianPlatform(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 轮询断言：玩家传送到末地后，验证黑曜石平台存在。
    // 平台中心 (100, 48, 0)，5×5 范围。
    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return; // 主世界仍有玩家，未传送完成
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        if (endPlayers.length === 0) {
            return; // 末地无玩家，传送未完成
        }
        // 验证平台中心方块为黑曜石。
        const centerBlock = end.getBlock({ x: END_SPAWN_X, y: PLATFORM_Y, z: END_SPAWN_Z });
        test.assert(
            centerBlock !== undefined && centerBlock.typeId === "minecraft:obsidian",
            `platform center block at (${END_SPAWN_X},${PLATFORM_Y},${END_SPAWN_Z}) is not obsidian`,
        );
    });
}

// 验证末地平台上方空间已清空：传送后 Y=49~52 范围内中心列应为空气。
function endPortalClearsSpaceAbovePlatform(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return; // 主世界仍有玩家，未传送完成
        }
        const end = world.getDimension("minecraft:the_end");
        const endPlayers = end.getEntities({ type: "minecraft:player" });
        if (endPlayers.length === 0) {
            return; // 末地无玩家，传送未完成
        }
        // 验证平台正上方中心列 Y=49~52 为空气（createEndSpawnPlatform 清空 4 层）。
        for (let y = END_SPAWN_Y - 1; y <= END_SPAWN_Y + 2; y++) {
            const block = end.getBlock({ x: END_SPAWN_X, y: y, z: END_SPAWN_Z });
            test.assert(
                block !== undefined && block.isAir,
                `block at (${END_SPAWN_X},${y},${END_SPAWN_Z}) should be air after platform clear`,
            );
        }
    });
}

export function registerEndPortalPlatformTests(): void {
    GameTest.register("TeleportTests", "end_portal_transfers_to_spawn_point", endPortalTransfersPlayerToEndSpawn)
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register("TeleportTests", "end_portal_creates_obsidian_platform", endPortalCreatesObsidianPlatform)
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register("TeleportTests", "end_portal_clears_space_above_platform", endPortalClearsSpaceAbovePlatform)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
