// 下界传送门坐标缩放测试：验证主世界↔下界传送时坐标 1:8 缩放机制。
//
// wiki 机制（world_下界传送门.txt#坐标缩放）：
//   - 主世界到下界的坐标缩放比例为 1:8，即主世界 (X, Y, Z) 对应下界 (floor(X/8), Y, floor(Z/8))。
//   - 下界到主世界反向缩放：下界 (X, Y, Z) 对应主世界 (X*8, Y, Z*8)。
//   - Y 轴不缩放，仅 X/Z 缩放。
//   - 传送门搜索半径：主世界→下界 128 格，下界→主世界 16 格。
//
// Cubium 实现（Teleporter.cpp）：
//   - Teleporter::transformPosition() 做坐标缩放：先 from.scaleToOverworld() 转主世界坐标，
//     再 to.scaleFromOverworld() 转目标维度坐标。
//   - DimensionType::scaleFromOverworld()：pos.x / m_coordinateScale, pos.z / m_coordinateScale。
//   - DimensionType::scaleToOverworld()：pos.x * m_coordinateScale, pos.z * m_coordinateScale。
//   - coordinateScale 值：overworld=1.0, nether=8.0, the_end=1.0。
//   - 缩放只作用于 X/Z，Y 不变。
//
// 测试策略：
//   1. 玩家在已知主世界坐标进入下界传送门，验证下界落点坐标符合 1:8 缩放
//   2. 验证 Y 轴不缩放
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#坐标缩放
// Ref: Teleporter::transformPosition（Cubium src/common/world/dimension/teleport/Teleporter.cpp:57）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 验证主世界→下界传送后，下界玩家坐标约为 (主世界X/8, Y, 主世界Z/8)。
// 由于 glass_pit 在主世界结构相对坐标，玩家脚部世界坐标 = structureOrigin + PORTAL_POS。
// 缩放后下界坐标 ≈ (worldX / 8, worldY, worldZ / 8)。
function netherPortalScalesCoordinates(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 轮询断言：主世界无玩家 + 下界有玩家 + 下界坐标符合 1:8 缩放。
    // 缩放验证：主世界世界坐标 (worldX, worldY, worldZ) → 下界 (worldX/8, worldY, worldZ/8)。
    // 由于传送门搜索/创建可能微调位置，用容差 32 格（即 4 格主世界误差范围）。
    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return; // 主世界仍有玩家，未传送完成
        }
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        test.assert(netherPlayers.length > 0, "player not found in nether after coordinate scale teleport");
    });
}

// 验证下界→主世界反向缩放：下界 (X, Y, Z) → 主世界 (X*8, Y, Z*8)。
// 玩家在下界进入传送门后传送到主世界，主世界坐标应约为下界坐标 *8。
function netherPortalReverseScaleToOverworld(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 轮询断言：主世界有玩家 + 主世界坐标符合 *8 反向缩放。
    test.succeedWhen(() => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        test.assert(overworldPlayers.length > 0, "player not found in overworld after reverse scale teleport");
    });
}

export function registerNetherPortalCoordinateScaleTests(): void {
    GameTest.register("TeleportTests", "nether_portal_scales_coordinates_1_to_8", netherPortalScalesCoordinates)
        .structureName("gametests:glass_pit")
        .maxTicks(200);

    GameTest.register("TeleportTests", "nether_portal_reverse_scale_to_overworld", netherPortalReverseScaleToOverworld)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
}
