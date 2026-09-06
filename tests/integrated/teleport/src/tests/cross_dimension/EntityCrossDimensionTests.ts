// 非玩家实体跨维度传送测试：验证非玩家实体（如猪、牛等）能否通过传送门跨维度传送。
//
// wiki 机制（world_下界传送门.txt#传送、Entity changeDimension）：
//   - vanilla 中非玩家实体（如矿车、船、动物）可以通过下界传送门跨维度传送（由 PortalForcer 处理）。
//   - 实体进入传送门后，传送冷却递减，达阈值后触发 onPortalTriggered。
//   - Entity 基类 getMaxInPortalTime() 返回 0（Cubium Entity.hpp:1187），
//     意味着非玩家实体进入传送门后立即传送（portalTime++ 达 0 即触发）。
//   - Entity 基类 changeDimension(targetDim) 默认返回 false（Cubium Entity.hpp:1214-1218），
//     即非玩家实体默认不跨维度传送。
//
// Cubium 实现（Entity.cpp、PortalTickSystem.cpp）：
//   - Entity::onPortalTriggered() 基类实现：重置传送门状态 + triggerPortalCooldown()，返回 false。
//   - Entity::changeDimension(targetDim) 基类实现：默认返回 false（不传送）。
//   - 仅 ServerPlayer override onPortalTriggered/changeDimension 实现真实跨维度传送。
//   - 非玩家实体进入下界传送门后，onEntityCollision → setInPortal(true) → PortalTickSystem
//     → onPortalTriggered（基类返回 false，不传送）。
//
// 测试策略：
//   1. 验证非玩家实体（猪）进入下界传送门后不会跨维度传送（Entity 基类 changeDimension 返回 false）
//   2. 验证非玩家实体（猪）进入末地传送门后不会跨维度传送（同上）
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#传送
// Ref: Entity::changeDimension（Cubium Entity.hpp:1214）
// Ref: Entity::onPortalTriggered（Cubium Entity.cpp:861）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 验证非玩家实体（猪）进入下界传送门后不会跨维度传送到下界。
// Entity 基类 changeDimension 返回 false，猪不会被传送到下界。
function nonPlayerEntityDoesNotCrossNetherPortal(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    // 生成猪于门方块内，onEntityCollision 触发 setInPortal。
    test.spawn("minecraft:pig", PORTAL_POS);

    // 轮询断言：等待若干 tick 后，验证主世界仍有猪（未跨维度传送）。
    pollUntilSucceed(test, () => {
        const overworldPigs = test.getDimension().getEntities({
            type: "minecraft:pig",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        // 猪应仍在主世界（Entity 基类 changeDimension 返回 false，不跨维度传送）。
        if (overworldPigs.length === 0) {
            return false; // 主世界无猪，可能已传送或还在加载
        }
        // 验证下界无猪（猪未被跨维度传送）。
        const nether = world.getDimension("minecraft:nether");
        const netherPigs = nether.getEntities({ type: "minecraft:pig" });
        return netherPigs.length === 0;
    }, {
        startTick: 20,
        interval: 10,
        maxTick: 100,
        onTimeout: () => {
            const nether = world.getDimension("minecraft:nether");
            const netherPigs = nether.getEntities({ type: "minecraft:pig" });
            const owPigs = test.getDimension().getEntities({
                type: "minecraft:pig",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(false,
                `pig cross-dimension via nether portal: overworld=${owPigs.length}, nether=${netherPigs.length}`);
        },
    });
}

// 验证非玩家实体（猪）进入末地传送门后不会跨维度传送到末地。
// Entity 基类 changeDimension 返回 false，猪不会被传送到末地。
function nonPlayerEntityDoesNotCrossEndPortal(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.spawn("minecraft:pig", PORTAL_POS);

    pollUntilSucceed(test, () => {
        const overworldPigs = test.getDimension().getEntities({
            type: "minecraft:pig",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (overworldPigs.length === 0) {
            return false; // 主世界无猪，可能已传送或还在加载
        }
        // 验证末地无猪（猪未被跨维度传送）。
        const end = world.getDimension("minecraft:the_end");
        const endPigs = end.getEntities({ type: "minecraft:pig" });
        return endPigs.length === 0;
    }, {
        startTick: 20,
        interval: 10,
        maxTick: 100,
        onTimeout: () => {
            const end = world.getDimension("minecraft:the_end");
            const endPigs = end.getEntities({ type: "minecraft:pig" });
            const owPigs = test.getDimension().getEntities({
                type: "minecraft:pig",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(false,
                `pig cross-dimension via end portal: overworld=${owPigs.length}, end=${endPigs.length}`);
        },
    });
}

export function registerEntityCrossDimensionTests(): void {
    GameTest.register("TeleportTests", "non_player_entity_does_not_cross_nether_portal", nonPlayerEntityDoesNotCrossNetherPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(120);

    GameTest.register("TeleportTests", "non_player_entity_does_not_cross_end_portal", nonPlayerEntityDoesNotCrossEndPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
