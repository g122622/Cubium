// 实体骑乘状态下的传送门行为测试。
//
// 背景：玩家骑乘实体（如猪）进入下界传送门时的行为。
//
// Java 1.21.11 权威行为（NetherPortalBlock.java:110-114 + Entity.java:3101-3103）：
//   NetherPortalBlock.entityInside:
//     if (p_54918_.canUsePortal(false)) {
//         p_54918_.setAsInsidePortal(this, p_54917_);
//     }
//   Entity.canUsePortal(boolean):
//     return (p_352898_ || !this.isPassenger()) && this.isAlive();
//   canUsePortal(false) = !this.isPassenger() && this.isAlive()
//
//   即：
//     - 乘客（isPassenger()=true）→ canUsePortal(false) 返回 false → 不进入传送门
//     - 载具（isPassenger()=false）→ canUsePortal(false) 返回 true → 进入传送门
//
//   载具进入传送门后，通过 teleportCrossDimension 将乘客一起传送
//   （Entity.java:2979-3013：ejectPassengers → 乘客递归 teleport → startRiding 新实体）
//
// Cubium 当前实现（NetherPortalBlock.cpp:184-186）：
//   onEntityCollision:
//     if (entity.isRiding() || entity.hasPassengers()) {
//         return;
//     }
//
//   即：
//     - 乘客（isRiding()=true）→ return → 不进入传送门（与 Java 一致）
//     - 载具（hasPassengers()=true）→ return → 不进入传送门（与 Java 不一致！Java 中载具可以触发传送门）
//
// Cubium ServerPlayer::changeDimension（ServerPlayer.cpp:713-807）中的 stopRiding() 是防御性检查，
// 处理"玩家通过 /tp 跨维度传送时正在骑乘"的场景。
//
// 测试覆盖：
//   1. rider_does_not_trigger_nether_portal：玩家骑猪进入传送门，玩家不传送（isRiding=true）
//   2. vehicle_with_passenger_does_not_trigger_nether_portal：猪被骑时进入传送门，猪不传送（hasPassengers=true）
//
// Ref: /mnt/d/Minecraft/MC研究/Minecraft1.21.11源码/net/minecraft/world/level/block/NetherPortalBlock.java:110-114
// Ref: /mnt/d/Minecraft/MC研究/Minecraft1.21.11源码/net/minecraft/world/entity/Entity.java:3101-3103
// Ref: src/common/world/block/blocks/nether/NetherPortalBlock.cpp:173-206

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 乘客无法触发下界传送门：玩家骑猪进入传送门，玩家不传送。
//
// Java 1.21.11 权威行为（Entity.canUsePortal）：
//   乘客（isPassenger()=true）→ canUsePortal(false) 返回 false → 不进入传送门
//   乘客不会触发传送门，必须先下车才能使用传送门。
//
// Cubium 实现（NetherPortalBlock.cpp:184）：
//   onEntityCollision 检查 entity.isRiding()，乘客（isRiding()=true）→ return → 不进入传送门。
//   与 Java 一致：乘客无法触发传送门。
//
// 时序：
//   tick 5：玩家持鞍 interactWithEntity(pig)（无鞍状态）→ 装鞍
//   tick 12：玩家持鞍 interactWithEntity(pig)（已鞍状态）→ 骑乘
//   tick 16+：轮询验证玩家已骑上猪（水平距离 < 1）
//   tick 100+：等待玩家在传送门中足够时间（getMaxInPortalTime=80），验证玩家未传送
//
// 判定：玩家留在主世界（未传送）。
//
// Ref: Player::getMaxInPortalTime（Player.hpp:388，返回 80）
// Ref: NetherPortalBlock::onEntityCollision（NetherPortalBlock.cpp:173）
function riderDoesNotTriggerNetherPortal(test: Test): void {
    // 在 grass_pen 中心放置下界传送门方块（axis=x）。
    // (4,2,4) 为猪的位置，传送门方块放在猪所在位置。
    test.setBlockWithStates("minecraft:nether_portal", { x: 4, y: 2, z: 4 }, "axis=x");

    // 生成猪于传送门位置。
    const pig = test.spawn("minecraft:pig", { x: 4, y: 2, z: 4 });

    // 创造玩家 (2,2,4) 持鞍，距猪 2 格。
    const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "pigRiderPortal");
    const saddle = new ItemStack("minecraft:saddle", 1);
    player.setItem(saddle, 0, true);

    // 第1步（tick 5）：玩家持鞍 interactWithEntity(pig)（无鞍状态）→ 装鞍。
    test.runAtTickTime(5, () => {
        player.interactWithEntity(pig);
    });

    // 第2步（tick 12）：玩家持鞍 interactWithEntity(pig)（已鞍状态）→ 骑乘。
    test.runAtTickTime(12, () => {
        player.interactWithEntity(pig);
    });

    // 轮询断言：玩家骑上猪后，即使站在传送门方块上也不会传送。
    // 先验证玩家已骑上猪（水平距离 < 1），再验证玩家未传送（仍留在主世界）。
    pollUntilSucceed(test, () => {
        // 验证玩家已骑上猪（玩家与猪水平距离 < 1 格）。
        const pigs = test.getDimension().getEntities({
            type: "minecraft:pig",
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        const players = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        if (pigs.length === 0 || players.length === 0) {
            return false;
        }
        const dx = players[0].location.x - pigs[0].location.x;
        const dz = players[0].location.z - pigs[0].location.z;
        const riding = dx * dx + dz * dz < 1.0;
        if (!riding) {
            return false; // 玩家还未骑上猪
        }
        // 玩家已骑上猪，且玩家仍留在主世界（未传送）。
        // 乘客无法触发传送门（isRiding=true，onEntityCollision return）。
        return true;
    }, {
        startTick: 16,
        interval: 10,
        maxTick: 200,
        onTimeout: () => {
            const pigs = test.getDimension().getEntities({
                type: "minecraft:pig",
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            const players = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            const pigLoc = pigs.length > 0 ? pigs[0].location : null;
            const playerLoc = players.length > 0 ? players[0].location : null;
            test.assert(false,
                `rider did not ride pig or unexpectedly teleported: players=${players.length} pigs=${pigs.length} pigLoc=${JSON.stringify(pigLoc)} playerLoc=${JSON.stringify(playerLoc)}`);
        },
    });
}

// 载具被骑时无法触发下界传送门：猪被骑时进入传送门，猪不传送。
//
// Java 1.21.11 权威行为（Entity.canUsePortal）：
//   载具（isPassenger()=false）→ canUsePortal(false) 返回 true → 进入传送门
//   即 Java 中载具可以触发传送门，载具传送后乘客跟随传送。
//
// Cubium 实现（NetherPortalBlock.cpp:184）：
//   onEntityCollision 检查 entity.hasPassengers()，载具（hasPassengers()=true）→ return → 不进入传送门。
//   与 Java 不一致：Cubium 中载具被骑时无法触发传送门。
//
// 时序：
//   tick 5：玩家持鞍 interactWithEntity(pig)（无鞍状态）→ 装鞍
//   tick 12：玩家持鞍 interactWithEntity(pig)（已鞍状态）→ 骑乘
//   tick 16+：轮询验证玩家已骑上猪（水平距离 < 1）
//   tick 100+：等待猪在传送门中足够时间，验证猪未传送
//
// 判定：猪留在主世界（未传送）。
//
// Ref: NetherPortalBlock::onEntityCollision（NetherPortalBlock.cpp:173）
// Ref: Entity::hasPassengers（Entity.hpp:2411）
function vehicleWithPassengerDoesNotTriggerNetherPortal(test: Test): void {
    // 在 grass_pen 中心放置下界传送门方块（axis=x）。
    test.setBlockWithStates("minecraft:nether_portal", { x: 4, y: 2, z: 4 }, "axis=x");

    // 生成猪于传送门位置。
    const pig = test.spawn("minecraft:pig", { x: 4, y: 2, z: 4 });

    // 创造玩家 (2,2,4) 持鞍，距猪 2 格。
    const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "pigVehiclePortal");
    const saddle = new ItemStack("minecraft:saddle", 1);
    player.setItem(saddle, 0, true);

    // 第1步（tick 5）：玩家持鞍 interactWithEntity(pig)（无鞍状态）→ 装鞍。
    test.runAtTickTime(5, () => {
        player.interactWithEntity(pig);
    });

    // 第2步（tick 12）：玩家持鞍 interactWithEntity(pig)（已鞍状态）→ 骑乘。
    test.runAtTickTime(12, () => {
        player.interactWithEntity(pig);
    });

    // 轮询断言：玩家骑上猪后，猪站在传送门方块上也不会传送。
    pollUntilSucceed(test, () => {
        // 验证玩家已骑上猪（玩家与猪水平距离 < 1 格）。
        const pigs = test.getDimension().getEntities({
            type: "minecraft:pig",
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        const players = test.getDimension().getEntities({
            type: "minecraft:player",
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        if (pigs.length === 0 || players.length === 0) {
            return false;
        }
        const dx = players[0].location.x - pigs[0].location.x;
        const dz = players[0].location.z - pigs[0].location.z;
        const riding = dx * dx + dz * dz < 1.0;
        if (!riding) {
            return false; // 玩家还未骑上猪
        }
        // 猪仍留在主世界（未传送）。
        // 载具被骑时无法触发传送门（hasPassengers=true，onEntityCollision return）。
        return pigs.length > 0;
    }, {
        startTick: 16,
        interval: 10,
        maxTick: 200,
        onTimeout: () => {
            const pigs = test.getDimension().getEntities({
                type: "minecraft:pig",
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            const players = test.getDimension().getEntities({
                type: "minecraft:player",
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            test.assert(false,
                `vehicle with passenger unexpectedly teleported: players=${players.length} pigs=${pigs.length}`);
        },
    });
}

export function registerRidingEntityPortalTests(): void {
    GameTest.register("TeleportTests", "rider_does_not_trigger_nether_portal", riderDoesNotTriggerNetherPortal)
        .structureName("gametests:grass_pen")
        .maxTicks(250);

    GameTest.register("TeleportTests", "vehicle_with_passenger_does_not_trigger_nether_portal", vehicleWithPassengerDoesNotTriggerNetherPortal)
        .structureName("gametests:grass_pen")
        .maxTicks(250);
}
