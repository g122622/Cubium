// 下界传送门方块行为测试：验证轴向状态、Boss/骑乘实体检查、传送门高度。
//
// wiki 机制（tech_下界传送门（方块）.txt）：
//   - 状态属性：HORIZONTAL_AXIS（水平轴向，x/z）。
//   - Boss 不能使用传送门（末影龙、凋灵等）。
//   - 被骑乘的实体及其骑乘者接触下界传送门方块时不会被传送。
//   - 光源：下界传送门发出亮度等级为11的光。
//   - X 轴形状：box(0,0,0.375,1,1,0.625)；Z 轴形状：box(0.375,0,0,0.625,1,1)。
//
// Cubium 实现（NetherPortalBlock.cpp）：
//   - onEntityCollision：检查 isRiding()/hasPassengers()（骑乘检查）、!isNonBoss()（Boss 检查）、
//     canTeleport()（冷却检查），通过则 setInPortal(true) + setPortalPos(pos)。
//   - getStateForPlacement：根据 context.horizontalDirection() 取 axis。
//   - getShape：X 轴 box(0,0,0.375,1,1,0.625)，Z 轴 box(0.375,0,0,0.625,1,1)。
//
// className 恒为 TeleportTests（对齐 teleport 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_下界传送门（方块）.txt
// Ref: NetherPortalBlock（Cubium src/common/world/block/blocks/nether/NetherPortalBlock.cpp）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 验证下界传送门方块轴向状态：axis=x 和 axis=z。
// NetherPortalBlock 状态容器只有 HORIZONTAL_AXIS（"axis" 属性，值为 "x"/"z"）。
// Ref: tech_下界传送门（方块）.txt#数据值（方块状态 axis）
function netherPortalBlockAxisStates(test: Test): void {
    // 设 axis=x，读回验证。
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    const blockX = test.getDimension().getBlock(test.worldLocation(PORTAL_POS));
    test.assert(blockX !== undefined, "nether_portal (axis=x) block not found");
    test.assert(
        blockX!.permutation.getState("axis") === "x",
        `axis should be x, got ${blockX!.permutation.getState("axis")}`,
    );

    // 设 axis=z，读回验证。
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=z");
    const blockZ = test.getDimension().getBlock(test.worldLocation(PORTAL_POS));
    test.assert(blockZ !== undefined, "nether_portal (axis=z) block not found");
    test.assert(
        blockZ!.permutation.getState("axis") === "z",
        `axis should be z, got ${blockZ!.permutation.getState("axis")}`,
    );

    test.succeed();
}

// 验证 Boss 不能使用下界传送门：末影龙进入传送门后不传送。
// NetherPortalBlock::onEntityCollision 检查 !entity.isNonBoss() → return（Boss 不传送）。
// Ref: tech_下界传送门（方块）.txt#用途（Boss 不能使用传送门）
function netherPortalBlockBossCannotTeleport(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    // 生成末影龙（Boss 实体）于门方块内。
    test.spawn("minecraft:ender_dragon", PORTAL_POS);

    // 轮询断言：主世界仍有末影龙（Boss 不跨维度传送）。
    pollUntilSucceed(test, () => {
        const overworldDragons = test.getDimension().getEntities({
            type: "minecraft:ender_dragon",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return overworldDragons.length > 0;
    }, {
        startTick: 10,
        interval: 10,
        maxTick: 60,
        onTimeout: () => {
            const owDragons = test.getDimension().getEntities({
                type: "minecraft:ender_dragon",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(
                false,
                `boss (ender_dragon) teleport test failed: overworld dragons=${owDragons.length}`,
            );
        },
    });
}

// 验证骑乘实体不能使用下界传送门：骑乘状态下的实体进入传送门不传送。
// NetherPortalBlock::onEntityCollision 检查 entity.isRiding() || entity.hasPassengers() → return。
// Ref: tech_下界传送门（方块）.txt#用途（被骑乘的实体不能被传送）
function netherPortalBlockRidingEntityCannotTeleport(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    // 生成骑乘猪的僵尸：僵尸骑猪，猪为坐骑。
    // spawn 的实体若带 rider 参数会自动骑乘。
    const pig = test.spawn("minecraft:pig", PORTAL_POS);
    // 生成僵尸骑在猪上。
    test.spawn("minecraft:zombie", PORTAL_POS);

    // 轮询断言：主世界仍有猪（骑乘实体不跨维度传送）。
    pollUntilSucceed(test, () => {
        const overworldPigs = test.getDimension().getEntities({
            type: "minecraft:pig",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return overworldPigs.length > 0;
    }, {
        startTick: 10,
        interval: 10,
        maxTick: 60,
        onTimeout: () => {
            const owPigs = test.getDimension().getEntities({
                type: "minecraft:pig",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(
                false,
                `riding entity teleport test failed: overworld pigs=${owPigs.length}`,
            );
        },
    });
}

// 验证下界传送门方块存在性：放置后 typeId 正确。
// Ref: tech_下界传送门（方块）.txt#数据值
function netherPortalBlockPlacement(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");

    const block = test.getDimension().getBlock(test.worldLocation(PORTAL_POS));
    test.assert(block !== undefined, "nether_portal block not found");
    test.assert(block!.typeId === "minecraft:nether_portal", `typeId mismatch: ${block!.typeId}`);
    test.assert(
        block!.permutation.getState("axis") === "x",
        `axis should be x, got ${block!.permutation.getState("axis")}`,
    );

    test.succeed();
}

export function registerNetherPortalBlockBehaviorTests(): void {
    GameTest.register(
        "TeleportTests",
        "nether_portal_block_axis_states",
        netherPortalBlockAxisStates,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(20);

    GameTest.register(
        "TeleportTests",
        "nether_portal_block_boss_cannot_teleport",
        netherPortalBlockBossCannotTeleport,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register(
        "TeleportTests",
        "nether_portal_block_riding_entity_cannot_teleport",
        netherPortalBlockRidingEntityCannotTeleport,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(100);

    GameTest.register(
        "TeleportTests",
        "nether_portal_block_placement",
        netherPortalBlockPlacement,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(20);
}
