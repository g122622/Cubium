// 下界传送门坐标缩放测试：验证主世界→下界传送时坐标 1:8 缩放机制（实际断言坐标值）。
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
// 测试策略（实际断言坐标值，非仅维度切换）：
//   1. nether_portal_scales_coordinates_1_to_8：主世界→下界，断言下界玩家 X/Z ≈ 主世界坐标 / 8
//   2. nether_portal_y_axis_not_scaled：Y 轴不缩放，断言下界 Y ≈ 主世界 Y（不除以 8）
//
// 注：下界→主世界反向缩放测试因传送门冷却（300 tick）导致单次测试内无法完成往返传送，故不包含。
//   如需测试反向缩放，需在脚本侧重置传送门冷却或大幅延长 maxTicks（> 300 + 缓冲）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#坐标缩放
// Ref: Teleporter::transformPosition（Cubium src/common/world/dimension/teleport/Teleporter.cpp:57）
// Ref: DimensionType::scaleFromOverworld/scaleToOverworld（Cubium DimensionType.cpp:172-208）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 坐标缩放容差。
// createPortal 在目标位置创建传送门（createNetherPortal 在 blockPos 处创建 2×3 门），
// findPortal 返回最近传送门位置。传送门搜索/创建可能使最终落点偏离纯数学缩放值数格。
// 主世界→下界：缩放后下界坐标 = worldX/8，门创建位置偏差叠加 1:8 缩放，故容差取较大值。
const SCALE_TOLERANCE = 8;

// 验证主世界→下界传送后，下界玩家坐标符合 1:8 缩放。
//
// 链路：NetherPortalBlock::onEntityCollision → setInPortal(true) → PortalTickSystem::tick
//   → onPortalTriggered → ServerPlayer::changeDimension(NETHER)
//   → transformPosition(currentPos, OVERWORLD, NETHER) = currentPos/8（X/Z）
//   → findPortal（半径 128）找不到 → createPortal 创建新门
//   → _performDimensionTransfer 迁移到下界。
//
// 断言：下界玩家坐标 X/Z ≈ 主世界世界坐标 / 8。
function netherPortalScalesCoordinates1To8(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
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
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        test.assert(netherPlayers.length > 0, "player not found in nether after coordinate scale teleport");

        // 玩家在主世界 PORTAL_POS 的世界坐标 = structureOrigin + PORTAL_POS。
        // 传送前 currentPos = (worldX, worldY, worldZ)，缩放后下界 targetPos ≈ (worldX/8, worldY, worldZ/8)。
        const worldOrigin = test.worldLocation({ x: 0, y: 0, z: 0 });
        const expectedX = (worldOrigin.x + PORTAL_POS.x) / 8;
        const expectedZ = (worldOrigin.z + PORTAL_POS.z) / 8;
        const p = netherPlayers[0];
        test.assert(
            Math.abs(p.location.x - expectedX) < SCALE_TOLERANCE,
            `nether X ${p.location.x} not near scaled ${expectedX} (tol ${SCALE_TOLERANCE})`,
        );
        test.assert(
            Math.abs(p.location.z - expectedZ) < SCALE_TOLERANCE,
            `nether Z ${p.location.z} not near scaled ${expectedZ} (tol ${SCALE_TOLERANCE})`,
        );
    });
}

// 验证 Y 轴不缩放：主世界→下界传送后，下界 Y 坐标 ≈ 主世界 Y（不除以 8）。
//
// Cubium 实现（DimensionType::scaleFromOverworld，DimensionType.cpp:179）：
//   return Vector3d(pos.x / m_coordinateScale, pos.y, pos.z / m_coordinateScale);
// Y 直接保留，不参与缩放。
//
// 断言：下界玩家 Y 坐标 ≈ 主世界世界坐标 Y（不除以 8）。
// 若 Y 被错误地除以 8，则 expectedY 与 actualY 差距远超 SCALE_TOLERANCE，测试会正确失败。
function netherPortalYAxisNotScaled(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
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
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        test.assert(netherPlayers.length > 0, "player not found in nether after y-axis scale test");

        // Y 不缩放：下界 Y ≈ 主世界世界坐标 Y（不应被除以 8）。
        const worldOrigin = test.worldLocation({ x: 0, y: 0, z: 0 });
        const expectedY = worldOrigin.y + PORTAL_POS.y;
        const p = netherPlayers[0];
        test.assert(
            Math.abs(p.location.y - expectedY) < SCALE_TOLERANCE,
            `nether Y ${p.location.y} not near unscaled ${expectedY} (tol ${SCALE_TOLERANCE})`,
        );
    });
}

export function registerNetherPortalCoordinateScaleTests(): void {
    GameTest.register("TeleportTests", "nether_portal_scales_coordinates_1_to_8", netherPortalScalesCoordinates1To8)
        .structureName("gametests:glass_pit")
        .maxTicks(200);

    GameTest.register("TeleportTests", "nether_portal_y_axis_not_scaled", netherPortalYAxisNotScaled)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
}
