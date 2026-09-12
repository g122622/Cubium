// 传送门方块破坏后无掉落物测试：验证 end_portal / nether_portal / end_gateway 破坏后不生成掉落物。
//
// wiki 机制（tech_末地传送门（方块）.txt、tech_下界传送门（方块）.txt）：
//   - 末地传送门方块、末地折跃门方块、下界传送门方块均不可作为物品拾取，破坏后不掉落任何物品。
//   - Java EndPortalBlock.getCloneItemStack 返回 ItemStack.EMPTY（无法作为物品拾取）。
//
// Cubium 实现：
//   - 传送门方块在 NetherBlocks.cpp 注册处使用 .noLootTable()：
//     nether_portal（239-240 行）、end_portal（689-690 行）、end_gateway（699-700 行）。
//   - .noLootTable() 将 m_lootTableId 清空，Block::getLootTable 返回 nullptr。
//   - BlockDropHandler::generateDrops 检测到 lootTable == nullptr 时走 getDefaultDrops 分支，
//     getDefaultDrops 返回空 vector，故不生成任何掉落物实体。
//
// 测试链路：
//   test.destroyBlock(pos, true) → GameTestHelper::destroyBlock(dropResources=true)
//   → BlockDropHandler::generateDrops（获取 lootTable，nullptr → getDefaultDrops 返回空）
//   → drops 为空 → 不调 spawnDrops → 区域内无 minecraft:item 实体。
//
// 对照测试（stone_drops_cobblestone_when_broken）：
//   破坏石头方块验证掉落圆石，排除"掉落链路根本没实现"的假阳性。
//   石头方块的掉落表 blocks/cobblestone 在破坏后会生成圆石掉落物实体。
//
// 测试结构（glass_pit 7×5×7）：
//   - 内部空间 X∈[1,5], Z∈[1,5], Y∈[1,3]（5×5×3）。
//   - Y=0 实心玻璃底板，Y=4 玻璃顶。
//
// className 恒为 TeleportTests（对齐 teleport 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末地传送门（方块）.txt
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_下界传送门（方块）.txt
// Ref: NetherBlocks.cpp（nether_portal/end_portal/end_gateway 注册 .noLootTable()）
// Ref: BlockDropHandler.cpp#generateDrops（lootTable==nullptr → getDefaultDrops 返回空）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部空间范围（结构相对坐标）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 放置/破坏传送门方块的中心位置。
const PORTAL_POS = { x: 3, y: 1, z: 3 };

// 统计 glass_pit 内部区域内的 minecraft:item 实体数量。
function countItemEntities(test: Test): number {
    const items = test.getDimension().getEntities({
        type: "minecraft:item",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
    return items.length;
}

// 验证破坏 end_portal 方块后不生成掉落物。
// end_portal 注册处 .noLootTable()，generateDrops 走 getDefaultDrops 返回空。
// Ref: NetherBlocks.cpp:689-690#end_portal 注册 .noLootTable()
function endPortalBlockDropsNothing(test: Test): void {
    test.setBlockWithStates("minecraft:end_portal", PORTAL_POS, "");
    test.destroyBlock(PORTAL_POS, true);

    // 轮询断言：破坏后区域内无 minecraft:item 实体。
    pollUntilSucceed(test, () => countItemEntities(test) === 0, {
        startTick: 2,
        interval: 4,
        maxTick: 30,
        onTimeout: () => {
            const count = countItemEntities(test);
            test.assert(
                false,
                `end_portal should drop nothing, but found ${count} item entities`,
            );
        },
    });
}

// 验证破坏 nether_portal 方块后不生成掉落物。
// nether_portal 注册处 .noLootTable()，generateDrops 走 getDefaultDrops 返回空。
// Ref: NetherBlocks.cpp:239-240#nether_portal 注册 .noLootTable()
function netherPortalBlockDropsNothing(test: Test): void {
    test.setBlockWithStates("minecraft:nether_portal", PORTAL_POS, "axis=x");
    test.destroyBlock(PORTAL_POS, true);

    pollUntilSucceed(test, () => countItemEntities(test) === 0, {
        startTick: 2,
        interval: 4,
        maxTick: 30,
        onTimeout: () => {
            const count = countItemEntities(test);
            test.assert(
                false,
                `nether_portal should drop nothing, but found ${count} item entities`,
            );
        },
    });
}

// 验证破坏 end_gateway 方块后不生成掉落物。
// end_gateway 注册处 .noLootTable()，generateDrops 走 getDefaultDrops 返回空。
// Ref: NetherBlocks.cpp:699-700#end_gateway 注册 .noLootTable()
function endGatewayBlockDropsNothing(test: Test): void {
    test.setBlockWithStates("minecraft:end_gateway", PORTAL_POS, "");
    test.destroyBlock(PORTAL_POS, true);

    pollUntilSucceed(test, () => countItemEntities(test) === 0, {
        startTick: 2,
        interval: 4,
        maxTick: 30,
        onTimeout: () => {
            const count = countItemEntities(test);
            test.assert(
                false,
                `end_gateway should drop nothing, but found ${count} item entities`,
            );
        },
    });
}

// 对照测试：破坏石头方块应生成掉落物（圆石）。
// 排除"掉落链路根本没实现"的假阳性：若 destroyBlock(dropResources=true) 从不生成掉落物，
// 则传送门方块"无掉落"的断言毫无意义（恒为真）。石头有掉落表 blocks/cobblestone，破坏后应掉落圆石。
function stoneDropsCobblestoneWhenBroken(test: Test): void {
    test.setBlockType("minecraft:stone", PORTAL_POS);
    test.destroyBlock(PORTAL_POS, true);

    pollUntilSucceed(test, () => countItemEntities(test) > 0, {
        startTick: 2,
        interval: 4,
        maxTick: 30,
        onTimeout: () => {
            const count = countItemEntities(test);
            test.assert(
                false,
                `stone should drop cobblestone, but found ${count} item entities`,
            );
        },
    });
}

export function registerPortalBlockNoDropTests(): void {
    GameTest.register("TeleportTests", "end_portal_block_drops_nothing", endPortalBlockDropsNothing)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "nether_portal_block_drops_nothing", netherPortalBlockDropsNothing)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "end_gateway_block_drops_nothing", endGatewayBlockDropsNothing)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "stone_drops_cobblestone_when_broken", stoneDropsCobblestoneWhenBroken)
        .structureName("gametests:glass_pit")
        .maxTicks(40);
}
