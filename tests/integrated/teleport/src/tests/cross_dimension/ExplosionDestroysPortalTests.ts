// 爆炸破坏下界传送门方块与黑曜石框架爆炸抗性测试。
//
// wiki 机制（tech_爆炸.txt、tech_TNT.txt、world_下界传送门.txt#创建传送门）：
//   - 爆炸抗性（Blast Resistance）决定方块能否被爆炸破坏。
//   - 下界传送门方块爆炸抗性 = 0（任何爆炸都能破坏并关闭传送门）。
//   - 黑曜石爆炸抗性 = 1200（TNT 爆炸半径 4，远不足以摧毁黑曜石）。
//   - TNT 引信 80 tick（4 秒），爆炸半径 4，爆炸后破坏半径内的可破坏方块。
//   - 爆炸破坏方块时，逐个调用 block.onBlockExploded，然后 setBlockState(pos, air, 3) 移除。
//
// Cubium 实现：
//   - NetherPortalBlock 注册于 NetherBlocks.cpp，使用
//     BlockProperties(Material::PORTAL).noCollision().hardness(0.0f).lightLevel(11).noLootTable()，
//     未设置 resistance，默认 m_resistance = 0.0f，故爆炸抗性为 0。
//   - ObsidianBlock 注册于 BaseBlocks.cpp:298，使用
//     BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f)，爆炸抗性 1200。
//   - Block::getExplosionResistance(state) 返回 state.resistance()。
//   - BlockState::resistance() 返回 m_resistance（= m_owner->resistance()）。
//   - Explosion::_calculateAffectedBlocks（Explosion.cpp:367-464）射线步进 0.3 格，
//     强度衰减 = (resistance + 0.3) * 0.3。下界传送门 resistance=0 → 衰减 0.09，极易被破坏；
//     黑曜石 resistance=1200 → 衰减 360.09，TNT 初始强度约 4*(0.7+0.6r) ≤ 4.4，远不足穿透。
//   - Explosion::_destroyBlocks（Explosion.cpp:713-821）遍历 m_affectedBlocks，
//     block.onBlockExploded 后 setBlockState(pos, air, 3) 移除方块。
//
// 测试结构（glass_pit 7×5×7）：
//   - 内部空间 X∈[1,5], Z∈[1,5], Y∈[1,3]（5×5×3）。
//   - Y=0 实心玻璃底板（可替换为黑曜石底框）。
//   - Y=4 玻璃顶（可替换为黑曜石顶框）。
//
// 测试覆盖（3 个核心行为点）：
//   1. tnt_explosion_destroys_nether_portal：TNT 爆炸破坏下界传送门方块（爆炸抗性 0）。
//   2. tnt_explosion_preserves_obsidian_frame：TNT 爆炸无法摧毁黑曜石框架（爆炸抗性 1200）。
//   3. obsidian_resists_tnt_explosion：黑曜石直接承受 TNT 爆炸后仍存在。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_爆炸.txt
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_TNT.txt
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt
// Ref: Explosion.cpp#_calculateAffectedBlocks + _destroyBlocks
// Ref: NetherBlocks.cpp#NetherPortalBlock 注册（resistance 未设置 → 0）
// Ref: BaseBlocks.cpp:298#OBSIDIAN 注册（resistance=1200）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, GameMode, Direction } from "@minecraft/server";

// TNT 引信 80 tick，外加爆炸执行缓冲。测试 maxTicks 设 120 留足爆炸完成时间。
const TNT_FUSE_TICKS = 80;
const EXPLOSION_BUFFER_TICKS = 20;

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 放置黑曜石方块（框架方块）。
function placeObsidian(test: Test, x: number, y: number, z: number): void {
    test.setBlockType("minecraft:obsidian", { x, y, z });
}

// 清空指定位置为空气（确保内部为空气）。
function clearToAir(test: Test, x: number, y: number, z: number): void {
    test.setBlockType("minecraft:air", { x, y, z });
}

// 构建 X 轴向下界传送门框架（内部宽 2、高 3）。
// 布局：glass_pit 内部放置最小 X 轴向框架。
//   - 内部左下角 (3,1,3)，内部宽 2（X=3,4）、高 3（Y=1,2,3）。
//   - 底框 Y=0（X=2..5）、顶框 Y=4（X=2..5）。
//   - 左框 X=2、右框 X=5（Y=1..3）。
function buildMinXAxisPortalFrame(test: Test): void {
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 2;
    const internalH = 3;

    // 底框（Y=0，X=2..5）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        placeObsidian(test, innerX + dx - 1, innerY - 1, innerZ);
    }
    // 顶框（Y=4，X=2..5）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        placeObsidian(test, innerX + dx - 1, innerY + internalH, innerZ);
    }
    // 左框（X=2，Y=1..3）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX - 1, innerY + dy, innerZ);
    }
    // 右框（X=5，Y=1..3）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX + internalW, innerY + dy, innerZ);
    }
    // 清空内部（确保是空气）。
    for (let dx = 0; dx < internalW; dx++) {
        for (let dy = 0; dy < internalH; dy++) {
            clearToAir(test, innerX + dx, innerY + dy, innerZ);
        }
    }
}

// 检查内部区域是否全部为 nether_portal 方块且轴向正确。
function isPortalFormed(
    test: Test,
    innerX: number,
    innerY: number,
    innerZ: number,
    internalW: number,
    internalH: number,
    axis: string,
): boolean {
    for (let d = 0; d < internalW; d++) {
        for (let dy = 0; dy < internalH; dy++) {
            let x = innerX;
            let z = innerZ;
            if (axis === "x") {
                x = innerX + d;
            } else {
                z = innerZ + d;
            }
            const pos = { x, y: innerY + dy, z };
            if (getBlockTypeId(test, pos.x, pos.y, pos.z) !== "minecraft:nether_portal") {
                return false;
            }
        }
    }
    return true;
}

// 场景 1：TNT 爆炸破坏下界传送门方块（爆炸抗性 0）。
//
// wiki 机制（tech_爆炸.txt、world_下界传送门.txt）：
//   - 下界传送门方块爆炸抗性 = 0，任何爆炸都能破坏并关闭传送门。
//   - TNT 爆炸半径 4，引信 80 tick。
//
// 布局：glass_pit 内部放置最小 X 轴向框架（内部 2×3），点燃后形成 nether_portal 方块。
//   - 在传送门旁放置 TNT 并用打火石点燃。
//   - 等待 TNT 引信 80 tick 后爆炸。
//
// 判定：爆炸后传送门方块被破坏（变为空气），传送门关闭。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_爆炸.txt
// Ref: Explosion.cpp#_destroyBlocks
function tntExplosionDestroysNetherPortal(test: Test): void {
    // 构建最小 X 轴向框架（内部 2×3，X=3,4 / Y=1..3 / Z=3）。
    buildMinXAxisPortalFrame(test);

    // 在框架内部点燃火焰激活传送门。
    // 火方块放置在内部 (3,1,3)（底框上方相邻空气）。
    test.setBlockType("minecraft:fire", { x: 3, y: 1, z: 3 });

    // 验证传送门已形成（内部 2×3 区域全部为 nether_portal，axis=x）。
    test.assert(
        isPortalFormed(test, 3, 1, 3, 2, 3, "x"),
        "nether portal should be formed after ignition",
    );

    // 在传送门旁放置 TNT（与传送门方块相邻，确保爆炸波及传送门）。
    // 传送门方块在 X=3,4 / Y=1..3 / Z=3。TNT 放在 (3,1,2)（Z=2，紧邻 Z=3 传送门）。
    test.setBlockType("minecraft:tnt", { x: 3, y: 1, z: 2 });

    // spawn SimulatedPlayer（生存模式），手持打火石点燃 TNT。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    // 对 TNT useItemOnBlock 打火石 → onBlockActivated prime+setBlockState(null)+Success。
    const used = player.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 2 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when igniting TNT with flint and steel");

    // 等待 TNT 引信 80 tick + 爆炸执行缓冲后，验证传送门方块被破坏。
    const explosionTick = TNT_FUSE_TICKS + EXPLOSION_BUFFER_TICKS;
    test.runAtTickTime(explosionTick, () => {
        // 验证传送门方块已被爆炸破坏（变为空气或非传送门方块）。
        const portalType = "minecraft:nether_portal";
        for (let dx = 0; dx < 2; dx++) {
            for (let dy = 0; dy < 3; dy++) {
                const typeId = getBlockTypeId(test, 3 + dx, 1 + dy, 3);
                test.assert(
                    typeId !== portalType,
                    `nether_portal block at (${3 + dx},${1 + dy},3) should be destroyed by TNT explosion, got ${typeId}`,
                );
            }
        }
    });

    test.succeed();
}

// 场景 2：TNT 爆炸无法摧毁黑曜石框架（爆炸抗性 1200）。
//
// wiki 机制（tech_爆炸.txt、tech_黑曜石.txt）：
//   - 黑曜石爆炸抗性 = 1200，TNT 爆炸（半径 4，初始强度约 4）远不足以摧毁。
//   - TNT 爆炸半径 4，引信 80 tick。
//
// 布局：glass_pit 内部放置最小 X 轴向框架（内部 2×3），点燃后形成 nether_portal 方块。
//   - 在传送门旁放置 TNT 并用打火石点燃。
//   - 等待 TNT 引信 80 tick 后爆炸。
//
// 判定：爆炸后黑曜石框架仍存在（爆炸抗性 1200，TNT 无法摧毁）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_爆炸.txt
// Ref: BaseBlocks.cpp:298#OBSIDIAN（resistance=1200）
function tntExplosionPreservesObsidianFrame(test: Test): void {
    // 构建最小 X 轴向框架（内部 2×3，X=3,4 / Y=1..3 / Z=3）。
    buildMinXAxisPortalFrame(test);

    // 在框架内部点燃火焰激活传送门。
    test.setBlockType("minecraft:fire", { x: 3, y: 1, z: 3 });

    // 验证传送门已形成。
    test.assert(
        isPortalFormed(test, 3, 1, 3, 2, 3, "x"),
        "nether portal should be formed after ignition",
    );

    // 在传送门旁放置 TNT。
    test.setBlockType("minecraft:tnt", { x: 3, y: 1, z: 2 });

    // spawn SimulatedPlayer（生存模式），手持打火石点燃 TNT。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    const used = player.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 2 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when igniting TNT with flint and steel");

    // 等待 TNT 引信 80 tick + 爆炸执行缓冲后，验证黑曜石框架仍存在。
    const explosionTick = TNT_FUSE_TICKS + EXPLOSION_BUFFER_TICKS;
    test.runAtTickTime(explosionTick, () => {
        const obsidianType = "minecraft:obsidian";
        // 底框 Y=0，X=2..5。
        for (let dx = 0; dx < 4; dx++) {
            const typeId = getBlockTypeId(test, 2 + dx, 0, 3);
            test.assert(
                typeId === obsidianType,
                `obsidian frame at (${2 + dx},0,3) should survive TNT explosion, got ${typeId}`,
            );
        }
        // 顶框 Y=4，X=2..5。
        for (let dx = 0; dx < 4; dx++) {
            const typeId = getBlockTypeId(test, 2 + dx, 4, 3);
            test.assert(
                typeId === obsidianType,
                `obsidian frame at (${2 + dx},4,3) should survive TNT explosion, got ${typeId}`,
            );
        }
        // 左框 X=2，Y=1..3。
        for (let dy = 0; dy < 3; dy++) {
            const typeId = getBlockTypeId(test, 2, 1 + dy, 3);
            test.assert(
                typeId === obsidianType,
                `obsidian frame at (2,${1 + dy},3) should survive TNT explosion, got ${typeId}`,
            );
        }
        // 右框 X=5，Y=1..3。
        for (let dy = 0; dy < 3; dy++) {
            const typeId = getBlockTypeId(test, 5, 1 + dy, 3);
            test.assert(
                typeId === obsidianType,
                `obsidian frame at (5,${1 + dy},3) should survive TNT explosion, got ${typeId}`,
            );
        }
    });

    test.succeed();
}

// 场景 3：黑曜石直接承受 TNT 爆炸后仍存在。
//
// wiki 机制（tech_黑曜石.txt、tech_爆炸.txt）：
//   - 黑曜石爆炸抗性 = 1200，TNT 爆炸无法摧毁。
//   - 在黑曜石旁引爆 TNT，黑曜石仍存在。
//
// 布局：glass_pit 内部放置一块黑曜石 (3,1,3)。
//   - 在黑曜石旁放置 TNT 并用打火石点燃。
//   - 等待 TNT 引信 80 tick 后爆炸。
//
// 判定：爆炸后黑曜石仍存在（爆炸抗性 1200，TNT 无法摧毁）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_黑曜石.txt
// Ref: BaseBlocks.cpp:298#OBSIDIAN（resistance=1200）
function obsidianResistsTntExplosion(test: Test): void {
    // 放置一块黑曜石 (3,1,3)。
    placeObsidian(test, 3, 1, 3);

    // 在黑曜石旁放置 TNT（与黑曜石相邻，确保爆炸波及黑曜石）。
    // TNT 放在 (3,1,2)（Z=2，紧邻 Z=3 黑曜石）。
    test.setBlockType("minecraft:tnt", { x: 3, y: 1, z: 2 });

    // spawn SimulatedPlayer（生存模式），手持打火石点燃 TNT。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    const used = player.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 2 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when igniting TNT with flint and steel");

    // 等待 TNT 引信 80 tick + 爆炸执行缓冲后，验证黑曜石仍存在。
    const explosionTick = TNT_FUSE_TICKS + EXPLOSION_BUFFER_TICKS;
    test.runAtTickTime(explosionTick, () => {
        const typeId = getBlockTypeId(test, 3, 1, 3);
        test.assert(
            typeId === "minecraft:obsidian",
            `obsidian at (3,1,3) should survive TNT explosion, got ${typeId}`,
        );
    });

    test.succeed();
}

export function registerExplosionDestroysPortalTests(): void {
    GameTest.register(
        "TeleportTests",
        "tnt_explosion_destroys_nether_portal",
        tntExplosionDestroysNetherPortal,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(120);

    GameTest.register(
        "TeleportTests",
        "tnt_explosion_preserves_obsidian_frame",
        tntExplosionPreservesObsidianFrame,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(120);

    GameTest.register(
        "TeleportTests",
        "obsidian_resists_tnt_explosion",
        obsidianResistsTntExplosion,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
