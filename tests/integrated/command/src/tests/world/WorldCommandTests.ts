// 世界操作类命令 GameTest：/setblock /fill 等。
//
// 覆盖 wiki 命令章节核心行为：
//   - /setblock：在指定坐标放置/替换方块，支持 destroy/keep/replace 模式（Ref: wiki setblock.txt）
//   - /fill：用指定方块填充立方区域，支持 hollow/outline/keep/replace 模式（Ref: wiki fill.txt）
//
// 设计要点：
//   1. 命令目标坐标是世界绝对坐标，需用 test.worldLocation(rel) 把 helper 相对坐标转世界坐标拼命令串。
//   2. SimulatedPlayer 默认 Creative（permLevel=2），可执行管理命令；chat("/cmd") 走 ServerCommandSource。
//   3. chat 命令同步执行（SimulatedPlayer::chat 立即派发到 CommandDispatcher），方块变更当 tick 生效，
//      用 runAtTickTime 留 5 tick 给命令执行 + 方块更新传播，再 assertBlockPresent。
//   4. cmd_arena 9×7×9：y=0 stone 地板，y=6 stone 封顶，y=1..5 内部 7×7 空气腔（x,z∈[1,7]）。
//      玩家站 (3,1,3) 地板上方空气；命令操作区用 (2,2,2)..(4,4,4) 的 3×3×3 立方体，中心 (3,3,3)
//      位于区域内部（非外壳），用于 hollow/outline 内部判定测试。
//
// 注：chat 执行命令仅 Cubium 端有效（基岩 BDS chat 是发消息语义非命令），命令类测试 Cubium one-sided。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// cmd_arena 内部空气腔坐标：x,z∈[1,7]，y∈[1,5]。
// 命令填充区 (2,2,2)..(4,4,4) 的 3×3×3 立方体，中心 (3,3,3) 是内部格。
// 玩家站 (5,1,5) 地板上方空气，在填充区外，避免 fill 把玩家所在格也填掉。
const FILL_FROM = { x: 2, y: 2, z: 2 };
const FILL_TO = { x: 4, y: 4, z: 4 };
const PLAYER_POS = { x: 5, y: 1, z: 5 };

/**
 * 把 helper 相对坐标格式化为命令用的世界绝对坐标字符串 "x y z"。
 * setblock/fill 命令接收世界绝对坐标（非相对 ~），故需 worldLocation 转换。
 */
function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// /setblock 在空气格放石头，默认 replace 模式（替换已有方块）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_setblock.txt（默认 replace）
function setblockPlacesBlock(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 目标格 (6,2,6) 是空气，setblock replace 放 stone。
    player.chat(`/setblock ${worldCoords(test, { x: 6, y: 2, z: 6 })} minecraft:stone`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:stone", { x: 6, y: 2, z: 6 }, true);
        test.succeed();
    });
}

// /setblock keep 模式：目标格已有方块时保留原方块不变（不覆盖）。
// 先在 (6,2,6) 放 oak_planks，再用 keep 模式 setblock stone → 应保留 planks。
// Ref: wiki setblock.txt（keep：仅当目标为空气时放置）
function setblockKeepModePreservesExisting(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 先放 oak_planks 作为已有方块。
    test.setBlockType("minecraft:oak_planks", { x: 6, y: 2, z: 6 });
    // keep 模式 setblock stone：已有 planks → 保留 planks，不覆盖。
    player.chat(`/setblock ${worldCoords(test, { x: 6, y: 2, z: 6 })} minecraft:stone keep`);

    test.runAtTickTime(5, () => {
        // 应仍是 oak_planks（keep 保留已有方块）。
        test.assertBlockPresent("minecraft:oak_planks", { x: 6, y: 2, z: 6 }, true);
        test.succeed();
    });
}

// /setblock destroy 模式：目标格已有方块时先破坏（掉落）再放置。
// 先放 glass（可被破坏掉落），destroy 模式 setblock stone → glass 被破坏，变 stone。
// Ref: wiki setblock.txt（destroy：破坏原方块并掉落，再放置）
function setblockDestroyModeBreaksExisting(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.setBlockType("minecraft:glass", { x: 6, y: 2, z: 6 });
    player.chat(`/setblock ${worldCoords(test, { x: 6, y: 2, z: 6 })} minecraft:stone destroy`);

    test.runAtTickTime(5, () => {
        // destroy 破坏 glass 后放 stone。
        test.assertBlockPresent("minecraft:stone", { x: 6, y: 2, z: 6 }, true);
        test.succeed();
    });
}

// /fill 用 stone 填充 3×3×3 立方体，默认 replace 模式（替换区域内所有方块）。
// 验证立方体角点与中心均已填充 stone。
// Ref: wiki fill.txt（默认 replace：替换区域内所有方块为目标方块）
function fillPlacesArea(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const from = worldCoords(test, FILL_FROM);
    const to = worldCoords(test, FILL_TO);
    player.chat(`/fill ${from} ${to} minecraft:stone`);

    test.runAtTickTime(5, () => {
        // 验证角点与中心（replace 全填充）。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 4, y: 4, z: 4 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 3, y: 3, z: 3 }, true);
        test.succeed();
    });
}

// /fill keep 模式：仅替换区域内的空气格，已有方块保留。
// 先在区域内放一个 oak_planks，fill keep stone → planks 保留，其余 air 变 stone。
// Ref: wiki fill.txt（keep：仅替换空气格）
function fillKeepModePreservesExisting(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 区域内放一个 planks 作为已有方块。
    test.setBlockType("minecraft:oak_planks", { x: 3, y: 3, z: 3 });
    const from = worldCoords(test, FILL_FROM);
    const to = worldCoords(test, FILL_TO);
    player.chat(`/fill ${from} ${to} minecraft:stone keep`);

    test.runAtTickTime(5, () => {
        // planks 保留（keep 不覆盖已有方块）。
        test.assertBlockPresent("minecraft:oak_planks", { x: 3, y: 3, z: 3 }, true);
        // 外壳角点 air 变 stone。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        test.succeed();
    });
}

// /fill hollow 模式：填充外壳为目标方块，内部清空为空气。
// 验证外壳是 stone、内部中心是 air。
// Ref: wiki fill.txt（hollow：外壳填目标方块，内部清空）
function fillHollowModeClearsInterior(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const from = worldCoords(test, FILL_FROM);
    const to = worldCoords(test, FILL_TO);
    player.chat(`/fill ${from} ${to} minecraft:stone hollow`);

    test.runAtTickTime(5, () => {
        // 外壳角点 (2,2,2) 是 stone。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        // 内部中心 (3,3,3) 应是 air（hollow 清空内部）。
        test.assertBlockPresent("minecraft:air", { x: 3, y: 3, z: 3 }, true);
        test.succeed();
    });
}

// /fill outline 模式：仅填充外壳，内部保持不变（不清空）。
// 先在内部放 oak_planks，fill outline stone → 外壳 stone，内部 planks 保留。
// Ref: wiki fill.txt（outline：仅替换外壳方块，内部不变）
function fillOutlineModePreservesInterior(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 内部放 planks。
    test.setBlockType("minecraft:oak_planks", { x: 3, y: 3, z: 3 });
    const from = worldCoords(test, FILL_FROM);
    const to = worldCoords(test, FILL_TO);
    player.chat(`/fill ${from} ${to} minecraft:stone outline`);

    test.runAtTickTime(5, () => {
        // 外壳角点 (2,2,2) stone。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        // 内部 planks 保留。
        test.assertBlockPresent("minecraft:oak_planks", { x: 3, y: 3, z: 3 }, true);
        test.succeed();
    });
}

export function registerWorldCommandTests(): void {
    GameTest.register("CommandTests", "setblock_places_block", setblockPlacesBlock)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "setblock_keep_mode_preserves_existing", setblockKeepModePreservesExisting)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "setblock_destroy_mode_breaks_existing", setblockDestroyModeBreaksExisting)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "fill_places_area", fillPlacesArea)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "fill_keep_mode_preserves_existing", fillKeepModePreservesExisting)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "fill_hollow_mode_clears_interior", fillHollowModeClearsInterior)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);

    GameTest.register("CommandTests", "fill_outline_mode_preserves_interior", fillOutlineModePreservesInterior)
        .structureName("gametests:cmd_arena")
        .maxTicks(50);
}
