// 下界传送门传送测试：验证玩家进入下界传送门方块后跨维度传送到下界。
//
// wiki 机制（world_下界传送门.txt#传送）：玩家站在下界传送门方块内累计一定时间后传送到对应维度
// （主世界↔下界，坐标 1:8 缩放）。Cubium 实现：NetherPortalBlock::onEntityCollision（:173）调
// entity.setInPortal(true)（:199），PortalTickSystem 累计 m_portalTime，达 getMaxInPortalTime()
// 后调 onPortalTriggered→changeDimension(NETHER)。changeDimension 内迁移 EntityManager 到下界。
//
// 注：getMaxInPortalTime 基类返回 0（Entity.hpp:1152，ServerPlayer 未 override），故玩家进门约 1-2
// tick 即传送（对齐创造模式语义，vanilla 生存 80tick）。maxTicks 给 200 覆盖两种情况。
//
// 放置技巧（复用 NetherPortalEmissionTests）：nether_portal 的 isValidPosition 要求相邻有 obsidian/
// portal，纯空气环境 false。setBlockWithStates（flags=3）只调 onBlockAdded 不调新方块自身
// updatePostPlacement（含 isValidPosition），故 portal 在无框架的 glass_pit 内能存活。放置后不动周围方块。
//
// 跨服务端：基岩 BDS GameTest SimulatedPlayer 跨维度传送行为未文档化，且涉及网络层 chunk 加载，
// GameTest 单线程模拟可能不触发真实传送。本用例 Cubium one-sided，基岩侧预期失败/超时。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#传送

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

// glass_pit 7×5×7：helper 相对坐标 x,z∈[0,6]，y∈[0,4]（y=0 grass 地板，y=1..4 air）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 }; // 脚部格：玩家站 grass(y=0) 上方 y=1，门在脚部格触发碰撞

function netherPortalTransfersPlayerToNether(test: Test): void {
    // 强放 nether_portal（无黑曜石框架），axis=x。放置后不动周围方块避免触发邻居更新链致 portal 失效。
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");

    // spawn SimulatedPlayer 于门方块内（脚部 y=1 站 grass 上方），默认创造模式。
    // 实体 AABB 覆盖门格 → onEntityCollision 触发 setInPortal → PortalTickSystem 计时 → changeDimension。
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 主断言：主世界 chamber 内无玩家（玩家已迁移到下界 EntityManager）。
    // 辅助断言：下界有玩家（查整个下界，GameTest 场景下界通常无其他玩家）。
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
        test.assert(netherPlayers.length > 0, "player not found in nether after portal teleport");
    });
}

export function registerNetherPortalTests(): void {
    GameTest.register("TeleportTests", "nether_portal_transfers_player_to_nether", netherPortalTransfersPlayerToNether)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
}
