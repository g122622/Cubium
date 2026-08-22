// /fill 命令 GameTest：填充区域方块。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/fill.txt）：
//   - /fill <from> <to> <block>：默认 replace 模式填充整块区域
//   - /fill <from> <to> <block> replace <filter>：仅替换匹配 filter 的方块
//   - /fill <from> <to> <block> hollow：空心填充（外壳填充、内部填空气）
//   - /fill <from> <to> <block> outline：轮廓填充（仅外壳、内部保持不变）
//   - /fill <from> <to> <block> keep：仅替换空气
//   - /fill <from> <to> <block> destroy：破坏原有方块掉落后填充
//
// 设计要点：
//   1. FillCommand::executeFill 遍历 from..to 立方体逐格 setBlockState 同步填充。命令经 chat 队列下一
//      tick 执行，断言用 runAtTickTime(5/10) 延迟读回（对齐 CloneCommandTests 范式）。
//   2. 坐标：命令用世界绝对坐标（worldCoords 转换），断言用结构相对坐标。cmd_arena 9×7×9 内部空气腔
//      x,z∈[1,7]，y∈[1,5]。填充区域取 3×1×3 或 3×3×3 子区域。
//   3. hollow 模式：外壳（6 面边界）填 fillState，内部填 air。测试 3×3×3 区域 fill stone hollow，
//      断言 8 角（外壳）为 stone、中心 (内部) 为 air。
//   4. outline 模式：仅外壳填 fillState，内部保持原样（不填 air）。测试先在内部摆 dirt，fill stone
//      outline，断言外壳 stone、内部 dirt 保留（区别 hollow 内部变 air）。
//   5. keep 模式：仅替换空气。测试区域先部分摆 stone（非空气），fill dirt keep，断言 stone 保留、
//      空气格变 dirt。
//   6. replace filter 模式：仅替换匹配 filter 方块。测试区域混摆 stone+dirt，fill glass replace stone，
//      断言 stone 变 glass、dirt 保留。
//   7. destroy 模式：破坏旧方块掉落 item 后填充。测试先摆 stone，fill dirt destroy，断言变 dirt 且
//      区域出现 item 实体（掉落物）。
//   8. SimulatedPlayer::chat permLevel 固定=4（≥2 满足 fill 权限）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_fill.txt（填充方块命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 1, y: 1, z: 1 };

// 结构相对坐标 → 世界绝对坐标字符串（命令用）。
function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// /fill <from> <to> <block> 默认 replace 填充整块 3×1×3 区域 stone。
// 走 FillCommand::_fill（FillMode::Replace，filterState=nullptr 全填）。
// Ref: wiki commands/fill.txt（fill <from> <to> <block>）
function fillReplacesRegion(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const from = { x: 2, y: 2, z: 2 };
    const to = { x: 4, y: 2, z: 4 };

    player.chat(`/fill ${worldCoords(test, from)} ${worldCoords(test, to)} minecraft:stone`);

    test.runAtTickTime(5, () => {
        // 3×1×3 区域全部应为 stone。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 3, y: 2, z: 3 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 4, y: 2, z: 4 }, true);
        test.succeed();
    });
}

// /fill <from> <to> <block> replace <filter> 仅替换匹配 filter 方块：区域混摆 stone+dirt，
// /fill glass replace stone，断言 stone 变 glass、dirt 保留。
// 走 FillCommand::_fillReplace filter 分支（FillMode::Replace filterState=stone，仅 blockId 匹配才填）。
// Ref: wiki commands/fill.txt（fill <from> <to> <block> replace <filter>）
function fillReplaceFilterSelective(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 摆 stone 和 dirt 各一。
    test.setBlockType("minecraft:stone", { x: 2, y: 2, z: 2 });
    test.setBlockType("minecraft:dirt", { x: 3, y: 2, z: 3 });

    player.chat(`/fill ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 3, y: 2, z: 3 })} minecraft:glass replace minecraft:stone`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:glass", { x: 2, y: 2, z: 2 }, true);
        // dirt 不匹配 filter，保留。
        test.assertBlockPresent("minecraft:dirt", { x: 3, y: 2, z: 3 }, true);
        test.succeed();
    });
}

// /fill <from> <to> <block> hollow 空心填充：3×3×3 区域 fill stone hollow，外壳（边界）stone、
// 内部中心 air。走 FillCommand::_fillHollow（FillMode::Hollow，isShell 填 fillState，内部填 air）。
// Ref: wiki commands/fill.txt（fill <from> <to> <block> hollow）
function fillHollowShellAndAirInterior(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const from = { x: 2, y: 2, z: 2 };
    const to = { x: 4, y: 2, z: 4 }; // 3×1×3，hollow 外壳填满（y 单层全是外壳），内部无。
    // 改用 3×3×3 才有内部：y∈[2,4]。
    const from3 = { x: 2, y: 2, z: 2 };
    const to3 = { x: 4, y: 4, z: 4 };

    player.chat(`/fill ${worldCoords(test, from3)} ${worldCoords(test, to3)} minecraft:stone hollow`);

    test.runAtTickTime(5, () => {
        // 外壳角应为 stone。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        test.assertBlockPresent("minecraft:stone", { x: 4, y: 4, z: 4 }, true);
        // 内部中心 (3,3,3) 应为 air（hollow 内部填 air）。
        test.assertBlockPresent("minecraft:stone", { x: 3, y: 3, z: 3 }, false);
        test.succeed();
    });
}

// /fill <from> <to> <block> outline 轮廓填充：仅外壳填 fillState，内部保持原样。先在内部中心摆 dirt，
// /fill stone outline，断言外壳 stone、内部 dirt 保留（区别 hollow 内部变 air）。
// 走 FillCommand::_fillOutline（FillMode::Outline，isShell 填 fillState，内部不动）。
// Ref: wiki commands/fill.txt（fill <from> <to> <block> outline）
function fillOutlineKeepsInterior(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 内部中心先摆 dirt。
    test.setBlockType("minecraft:dirt", { x: 3, y: 3, z: 3 });

    player.chat(`/fill ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 4, y: 4, z: 4 })} minecraft:stone outline`);

    test.runAtTickTime(5, () => {
        // 外壳角 stone。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        // 内部 dirt 保留（outline 不动内部）。
        test.assertBlockPresent("minecraft:dirt", { x: 3, y: 3, z: 3 }, true);
        test.succeed();
    });
}

// /fill <from> <to> <block> keep 仅替换空气：区域部分摆 stone（非空气），/fill dirt keep，
// 断言 stone 保留、空气格变 dirt。走 FillCommand::_fillKeep（FillMode::Keep，仅 air 才填）。
// Ref: wiki commands/fill.txt（fill <from> <to> <block> keep）
function fillKeepOnlyAir(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // (2,2,2) 摆 stone 占位，(3,2,3) 留空气。
    test.setBlockType("minecraft:stone", { x: 2, y: 2, z: 2 });

    player.chat(`/fill ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 3, y: 2, z: 3 })} minecraft:dirt keep`);

    test.runAtTickTime(5, () => {
        // stone 占位格保留（keep 跳过非空气）。
        test.assertBlockPresent("minecraft:stone", { x: 2, y: 2, z: 2 }, true);
        // 空气格变 dirt。
        test.assertBlockPresent("minecraft:dirt", { x: 3, y: 2, z: 3 }, true);
        test.succeed();
    });
}

// /fill <from> <to> <block> destroy 破坏旧方块掉落后填充：先摆 stone（掉落 cobblestone），
// /fill dirt destroy，断言变 dirt 且区域出现 item 实体（掉落物）。
// 走 FillCommand::_fillDestroy（FillMode::Destroy，generateDrops+spawnDrops 后 setBlockState）。
// Ref: wiki commands/fill.txt（fill <from> <to> <block> destroy）
function fillDestroyDropsAndFills(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.setBlockType("minecraft:stone", { x: 2, y: 2, z: 2 });
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });

    player.chat(`/fill ${worldCoords(test, { x: 2, y: 2, z: 2 })} ${worldCoords(test, { x: 3, y: 2, z: 3 })} minecraft:dirt destroy`);

    test.runAtTickTime(10, () => {
        test.assertBlockPresent("minecraft:dirt", { x: 2, y: 2, z: 2 }, true);
        test.assertBlockPresent("minecraft:dirt", { x: 3, y: 2, z: 3 }, true);
        // stone 破坏掉落 cobblestone item 实体。
        const center = test.worldLocation({ x: 2, y: 2, z: 2 });
        const items = test.getDimension().getEntities({
            type: "item",
            location: center,
            volume: { x: 4, y: 4, z: 4 },
        });
        test.assert(items.length > 0, `expected dropped items after fill destroy, got ${items.length}`);
        test.succeed();
    });
}

export function registerFillTests(): void {
    GameTest.register("CommandTests", "fill_replaces_region", fillReplacesRegion)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "fill_replace_filter_selective", fillReplaceFilterSelective)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "fill_hollow_shell_and_air_interior", fillHollowShellAndAirInterior)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "fill_outline_keeps_interior", fillOutlineKeepsInterior)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "fill_keep_only_air", fillKeepOnlyAir)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "fill_destroy_drops_and_fills", fillDestroyDropsAndFills)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
