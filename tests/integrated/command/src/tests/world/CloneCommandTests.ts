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

// /clone masked 模式：源区域含 air 格时，不把 air 复制到目标（保留目标已有方块）。
// 源区域 (2,2,2)..(4,2,4) 仅 (3,2,3) 摆 stone，其余 air。目标区域 (5,2,2)..(7,2,4) 全摆 oak_planks。
// clone masked 后：目标 (6,2,3)（对应源 (3,2,3) stone）被 stone 覆盖，目标其余格保留 planks（源 air 不覆盖）。
// 走 CloneCommand FilterMode::Masked（shouldCopy = !state->isAir()，air 格跳过不复制）。
// Ref: wiki clone.txt（masked 模式仅复制非空气方块）
function cloneMaskedPreservesTargetBlocks(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 源区域仅中心一格 stone，其余 air。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });
    // 目标区域全摆 planks。
    for (let x = 5; x <= 7; x++) {
        for (let z = 2; z <= 4; z++) {
            test.setBlockType("minecraft:oak_planks", { x, y: 2, z });
        }
    }
    // clone masked：源 (2,2,2)..(4,2,4) → 目标起始 (5,2,2)。
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 5, y: 2, z: 2 })} masked`);

    test.runAtTickTime(5, () => {
        // 源中心 (3,2,3) stone → 目标 (6,2,3) 被 stone 覆盖。
        test.assertBlockPresent("minecraft:stone", { x: 6, y: 2, z: 3 }, true);
        // 源 air 格不覆盖目标：目标 (5,2,2)（对应源 (2,2,2) air）保留 planks。
        test.assertBlockPresent("minecraft:oak_planks", { x: 5, y: 2, z: 2 }, true);
        // 目标 (7,2,4)（对应源 (4,2,4) air）也保留 planks。
        test.assertBlockPresent("minecraft:oak_planks", { x: 7, y: 2, z: 4 }, true);
        test.succeed();
    });
}

// /clone move 模式：复制后清空源区域（源方块变 air）。
// 源区域 (2,2,2)..(4,2,4) 摆 stone，clone move 到目标起始 (5,3,2)（不重叠）。
// move 后：目标有 stone，源区域全变 air。
// 走 CloneCommand CloneMode::Move（先收集 sourcePositions，复制后把源位置 setBlockState(air)）。
// 注：MC Java /clone move 不调用 spawnAfterBreak，仅清空源区域（CloneCommand.cpp:296 注释）。
// Ref: wiki clone.txt（move 模式复制后清除源区域）
function cloneMoveClearsSourceRegion(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    placeSourceBlocks(test); // 源 (2,2,2)..(4,2,4) stone
    // clone move 到目标起始 (5,3,2)（y=3 不与源 y=2 重叠）。
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 5, y: 3, z: 2 })} replace move`);

    test.runAtTickTime(5, () => {
        // 目标区域 (5,3,2)..(7,3,4) 有 stone（复制成功）。
        test.assertBlockPresent("minecraft:stone", { x: 5, y: 3, z: 2 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 6, y: 3, z: 3 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 7, y: 3, z: 4 }, true);
        // 源区域已清空（move 清空源）。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, false);
        test.assertBlockPresent("minecraft:stone", { x: 3, y: 2, z: 3 }, false);
        test.assertBlockPresent("minecraft:stone", { x: 4, y: 2, z: 4 }, false);
        test.succeed();
    });
}

// /clone force 模式：允许源区域与目标区域重叠（normal 模式会拒绝重叠报错）。
// 源 (2,2,2)..(4,2,4)，目标起始 (3,2,3)（与源重叠）。clone force 应执行成功，目标区域有 stone。
// 走 CloneCommand CloneMode::Force（跳过 boxesOverlap 检查，CloneCommand.cpp:155 仅 Normal 检查重叠）。
// Ref: wiki clone.txt（force 模式强制复制即使重叠）
function cloneForceAllowsOverlap(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    placeSourceBlocks(test); // 源 (2,2,2)..(4,2,4) stone
    // 目标起始 (3,2,3)：目标区域 (3,2,3)..(5,2,5) 与源 (2,2,2)..(4,2,4) 重叠。
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 3, y: 2, z: 3 })} replace force`);

    test.runAtTickTime(5, () => {
        // force 允许重叠，复制成功。目标区域 (3,2,3)..(5,2,5) 应有 stone。
        // (5,2,5) 是目标区域角，源 stone 复制过来。
        test.assertBlockPresent("minecraft:stone", { x: 5, y: 2, z: 5 }, true);
        test.succeed();
    });
}

// /clone normal 模式拒绝重叠（反例）：源目标重叠时命令失败，不复制。
// 源 (2,2,2)..(4,2,4)，目标起始 (3,2,3)（重叠）。clone normal（默认）应报 overlap 返 0。
// 验证：目标区域新增的角 (5,2,5) 无 stone（命令未执行复制）。
// 走 CloneCommand CloneMode::Normal（boxesOverlap 命中 → sendError overlap → return 0）。
// Ref: wiki clone.txt（normal 模式重叠时失败）
function cloneNormalRejectsOverlap(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    placeSourceBlocks(test); // 源 (2,2,2)..(4,2,4) stone
    // 目标起始 (3,2,3) 与源重叠。默认 normal 模式（不写模式词）。
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 3, y: 2, z: 3 })}`);

    test.runAtTickTime(5, () => {
        // normal 拒绝重叠，命令失败未复制。目标区域角 (5,2,5) 无 stone（初始 air）。
        test.assertBlockPresent("minecraft:stone", { x: 5, y: 2, z: 5 }, false);
        test.succeed();
    });
}

// /clone filtered 模式：仅复制匹配 filter 方块类型的方块（其余跳过）。
// 源区域 (2,2,2)..(4,2,4) 混合：中心 (3,2,3) glass，其余 stone。clone filtered glass 到目标 (5,3,2)。
// filtered 后：目标仅 glass 格被复制（stone 格跳过）。
// 走 CloneCommand FilterMode::Filtered（state->blockId()==filterState->blockId() 才复制）。
// Ref: wiki clone.txt（filtered 模式仅复制指定方块）
function cloneFilteredOnlyCopiesMatching(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 源区域：中心 glass，其余 stone。
    placeSourceBlocks(test); // 全 stone
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 3 }); // 中心改 glass
    // clone filtered minecraft:glass 到目标起始 (5,3,2)。
    player.chat(`/clone ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 2, z: 4 })} ${worldCoords(test, { x: 5, y: 3, z: 2 })} filtered minecraft:glass`);

    test.runAtTickTime(5, () => {
        // 源中心 (3,2,3) glass → 目标 (6,3,3) 被 glass 复制。
        test.assertBlockPresent("minecraft:glass", { x: 6, y: 3, z: 3 }, true);
        // 源 stone 格不被复制：目标 (5,3,2)（对应源 (2,2,2) stone）无 stone（filtered 跳过 stone）。
        test.assertBlockPresent("minecraft:stone", { x: 5, y: 3, z: 2 }, false);
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

    GameTest.register("CommandTests", "clone_masked_preserves_target_blocks", cloneMaskedPreservesTargetBlocks)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "clone_move_clears_source_region", cloneMoveClearsSourceRegion)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "clone_force_allows_overlap", cloneForceAllowsOverlap)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "clone_normal_rejects_overlap", cloneNormalRejectsOverlap)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "clone_filtered_only_copies_matching", cloneFilteredOnlyCopiesMatching)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);
}
