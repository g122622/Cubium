// /clone 命令 GameTest：克隆方块区域。
//
// 覆盖 wiki 命令章节核心行为：
//   - /clone <begin> <end> <destination>：克隆立方区域方块到目标位置（Ref: wiki clone.txt）
//   - replace/force/move 模式（基础 replace 已由 cloneBlocksCommand 覆盖，此处补 cmd_arena 场景）
//
// 设计要点：
//   1. clone 三组坐标均为世界绝对坐标（begin/end 源区域，destination 目标起始角）。
//   2. 源区域需先 setBlockType 摆方块，clone 后在 destination 验证方块复制到位。
//   3. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。
//      源区域 (2,2,2)..(4,2,4) 3×1×3 摆 stone；目标起始 (5,2,2)（clone 后复制到 (5,2,2)..(7,2,4)）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 1, y: 1, z: 1 };

function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

/** 在源区域 3×1×3 摆 stone。 */
function placeSourceBlocks(test: Test): void {
    for (let x = 2; x <= 4; x++) {
        for (let z = 2; z <= 4; z++) {
            test.setBlockType("minecraft:stone", { x, y: 2, z });
        }
    }
}

// /clone 把源区域 3×1×3 stone 复制到目标位置。
// Ref: wiki clone.txt（clone <begin> <end> <destination> 复制方块区域）
function cloneCopiesBlocks(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    placeSourceBlocks(test);
    // 源 (2,2,2)..(4,2,4)，目标起始 (5,2,2)。
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 5, y: 2, z: 2 })}`);

    test.runAtTickTime(5, () => {
        // 目标区域 (5,2,2)..(7,2,4) 应有 stone（复制源 3×1×3）。
        test.assertBlockPresent("minecraft:stone", { x: 5, y: 2, z: 2 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 6, y: 2, z: 3 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 7, y: 2, z: 4 }, true);
        // 源区域仍保留（clone 不移动源）。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        test.succeed();
    });
}

// /clone 源区域含混合方块，验证每种方块都正确复制（非仅 stone）。
function clonePreservesBlockTypes(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 源区域放三种方块。
    test.setBlockType("minecraft:oak_planks", { x: 2, y: 2, z: 2 });
    test.setBlockType("minecraft:cobblestone", { x: 3, y: 2, z: 3 });
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 4 });
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 5, y: 3, z: 5 })}`);

    test.runAtTickTime(5, () => {
        // 目标区域对应位置应有同种方块。
        test.assertBlockPresent("minecraft:oak_planks", { x: 5, y: 3, z: 5 }, true);
        test.assertBlockPresent("minecraft:cobblestone", { x: 6, y: 3, z: 6 }, true);
        test.assertBlockPresent("minecraft:glass", { x: 7, y: 3, z: 7 }, true);
        test.succeed();
    });
}

// /clone 目标区域已有方块时，默认 replace 模式覆盖。
// 目标区先放 oak_planks，clone stone 源 → 目标被 stone 覆盖。
function cloneReplaceModeOverwrites(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    placeSourceBlocks(test); // 源 stone
    // 目标区先放 planks。
    test.setBlockType("minecraft:oak_planks", { x: 5, y: 2, z: 2 });
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 5, y: 2, z: 2 })}`);

    test.runAtTickTime(5, () => {
        // 目标 (5,2,2) 被 stone 覆盖（replace 默认）。
        test.assertBlockPresent("minecraft:stone", { x: 5, y: 2, z: 2 }, true);
        test.succeed();
    });
}

export function registerCloneCommandTests(): void {
    GameTest.register("CommandTests", "clone_copies_blocks", cloneCopiesBlocks)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "clone_preserves_block_types", clonePreservesBlockTypes)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "clone_replace_mode_overwrites", cloneReplaceModeOverwrites)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);
}
