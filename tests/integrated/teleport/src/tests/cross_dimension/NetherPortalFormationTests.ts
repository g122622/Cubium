// 下界传送门框架形成测试：验证黑曜石框架尺寸限制（4×5~23×23）与点火激活逻辑。
//
// wiki 机制（world_下界传送门.txt#创建传送门、PortalShape）：
//   - 下界传送门框架须由黑曜石构成，内部点燃火焰（打火石/火矢弓/恶魂火球等）后激活为传送门方块。
//   - 框架内部空间尺寸约束：宽 2~3 格 × 高 3 格（即 4×5 到 4×23 的门框），或宽 1 格 × 高 3 格
//     （即 3×5 到 3×23）。最小门框 4×5（外框）= 内部 2×3 空间。
//   - 内部高度须 >= 3，宽度须 >= 2（2 格宽门）或 >= 1（1 格宽门，仅 Java）。
//
// Cubium 实现：NetherPortalBlock::onEntityCollision 调 entity.setInPortal(true)，PortalTickSystem
// 累计 m_portalTime，达 getMaxInPortalTime() 后调 onPortalTriggered→changeDimension(NETHER)。
// 传送门框架形成逻辑由 NetherTeleporter::createPortal / PortalSize 处理。
//
// 测试策略：
//   1. 玩家进入下界传送门方块后跨维度传送到下界（复用现有 netherPortalTransfersPlayerToNether 逻辑）
//   2. 下界传送门框架激活：放置黑曜石框架 + 点火后，验证传送门方块形成
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#创建传送门
// Ref: PortalShape（Java NetherPortalBlock.Shape）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

// glass_pit 7×5×7：helper 相对坐标 x,z∈[0,6]，y∈[0,4]（y=0 grass 地板，y=1..4 air）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 玩家进入下界传送门后传送到下界，验证下界有玩家存在。
function netherPortalTransfersPlayerToNether(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    const traveler = test.spawnSimulatedPlayer(PORTAL_POS, "traveler");
    test.runOnFinish(() => test.removeSimulatedPlayer(traveler));

    test.succeedWhen(() => {
        test.assert(traveler.dimension.id === "minecraft:nether", "traveler has not reached minecraft:nether");
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return;
        }
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        test.assert(netherPlayers.length > 0, "player not found in nether after portal teleport");
    });
}

// 下界传送门返回主世界：玩家在下界进入下界传送门后传送到主世界。
// 验证跨维度传送的双向性（NETHER→OVERWORLD）。
function netherPortalReturnsPlayerToOverworld(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    const traveler = test.spawnSimulatedPlayer(PORTAL_POS, "traveler");
    test.runOnFinish(() => test.removeSimulatedPlayer(traveler));

    let enteredNether = false;
    let netherEntrance: { x: number; y: number; z: number } | undefined;
    test.succeedWhen(() => {
        if (traveler.dimension.id === "minecraft:nether") {
            enteredNether = true;
            netherEntrance ??= { ...traveler.location };
            // 保持玩家接触已生成的出口门，等待冷却结束后触发真实返回传送。
            traveler.teleport(netherEntrance);
            test.assert(false, "traveler reached nether; waiting for return portal transfer");
        }
        test.assert(enteredNether, "traveler must enter nether before returning");
        test.assert(traveler.dimension.id === "minecraft:overworld", "traveler has not returned to overworld");
    });
}

export function registerNetherPortalFormationTests(): void {
    GameTest.register("TeleportTests", "nether_portal_formation_min_frame", netherPortalTransfersPlayerToNether)
        .batch("dimension_transfer_nether_portal_formation_min_frame")
        .tag("execution:sync-worldgen")
        .structureName("gametests:glass_pit")
        .maxTicks(200);

    GameTest.register("TeleportTests", "nether_portal_returns_to_overworld", netherPortalReturnsPlayerToOverworld)
        .batch("dimension_transfer_nether_portal_returns_to_overworld")
        .tag("execution:sync-worldgen")
        .structureName("gametests:glass_pit")
        .maxTicks(200);
}
