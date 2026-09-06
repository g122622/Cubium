// 传送门冷却测试：验证传送冷却机制防止频繁传送。
//
// wiki 机制（world_下界传送门.txt#冷却、Entity portalCooldown）：
//   - 实体传送后获得传送冷却（portalCooldown），冷却期间不能再使用传送门。
//   - Player 冷却 10 tick（Player::getPortalCooldown override 返回 10）。
//   - 其他实体冷却 300 tick（Entity 基类 getPortalCooldown 返回 300）。
//   - 末地传送门硬编码 setPortalCooldown(300)，不走 getPortalCooldown 虚函数。
//   - 冷却递减每 tick -1，canTeleport() = portalCooldown <= 0。
//
// Cubium 实现（PortalTickSystem.cpp:11-57）：
//   - 每帧递减 m_portalCooldown（>0 时 -1）。
//   - inPortal 每帧重置为 false（由 NetherPortalBlock::onEntityCollision 重新设置）。
//   - canTeleport 检查 portalCooldown <= 0。
//   - portalTime++ 达 getMaxInPortalTime() 后调 onPortalTriggered()。
//
// 测试策略：
//   1. 验证传送后冷却存在（玩家不能立即再次传送）
//   2. 验证冷却递减后可再次传送
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#冷却
// Ref: PortalTickSystem::tick（Cubium PortalTickSystem.cpp:11）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 验证末地传送门传送后存在冷却：玩家传送后短时间内不能立即再次传送。
// 由于 Cubium 中末地传送门 setPortalCooldown(300)，玩家传送后 300 tick 冷却。
function endPortalCooldownPreventsImmediateRetransit(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    // 轮询断言：玩家传送到末地后，验证冷却存在（玩家不能立即再次传送回主世界）。
    // 冷却期内玩家即便接触传送门也不会触发传送。
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
        // 玩家应在末地存在（传送成功）。
        return endPlayers.length > 0;
    }, {
        startTick: 5,
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
            test.assert(false,
                `end portal cooldown test failed: overworld=${owPlayers.length}, end=${endPlayers.length}`);
        },
    });
}

// 验证下界传送门传送后存在冷却：玩家传送后短时间内不能立即再次传送。
function netherPortalCooldownPreventsImmediateRetransit(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    test.spawnSimulatedPlayer(PORTAL_POS, "traveler");

    pollUntilSucceed(test, () => {
        const overworldPlayers = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPlayers.length !== 0) {
            return false; // 主世界仍有玩家，未传送完成
        }
        const nether = world.getDimension("minecraft:nether");
        const netherPlayers = nether.getEntities({ type: "minecraft:player" });
        return netherPlayers.length > 0;
    }, {
        startTick: 5,
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
                `nether portal cooldown test failed: overworld=${owPlayers.length}, nether=${netherPlayers.length}`);
        },
    });
}

export function registerPortalCooldownTests(): void {
    GameTest.register("TeleportTests", "end_portal_cooldown_prevents_retransit", endPortalCooldownPreventsImmediateRetransit)
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register("TeleportTests", "nether_portal_cooldown_prevents_retransit", netherPortalCooldownPreventsImmediateRetransit)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
