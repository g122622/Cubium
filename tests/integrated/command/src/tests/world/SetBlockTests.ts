// /setblock 命令 GameTest：在指定位置放置/替换方块。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/setblock.txt）：
//   - /setblock <pos> <block>：默认 replace 模式放置方块
//   - /setblock <pos> <block> replace：替换目标位置方块（与默认同）
//   - /setblock <pos> <block> keep：仅当目标位置为空气时放置
//   - /setblock <pos> <block> destroy：破坏原有方块并掉落物品后放置
//
// 设计要点：
//   1. SetBlockCommand::executeSetBlock 走 source.world()->setBlockState 同步放置。命令经 chat 队列
//      下一 tick 执行，故断言用 runAtTickTime(5) 延迟读回（对齐 CloneCommandTests 范式）。
//   2. 坐标：命令用世界绝对坐标（test.worldLocation(rel) 转换），断言用结构相对坐标
//      （test.assertBlockPresent/setBlockType）。cmd_arena 9×7×9 内部空气腔 x,z∈[1,7]，y∈[1,5]。
//   3. keep 模式：目标非空气时 executeSetBlock 检测 existingBlock非air → sendError 返 0 不放置。测试先
//      setBlockType 摆 stone 占位，再 /setblock keep dirt，断言仍是 stone（未替换）。
//   4. destroy 模式：先掉落旧方块物品（BlockDropHandler::generateDrops+spawnDrops）再放置新方块。测试
//      先摆 stone（掉落 cobblestone），/setblock destroy dirt，断言位置变 dirt 且附近生成 item 实体
//      （cobblestone 掉落物）。掉落物判定用 getEntities({type:"item"}) 区域查询。
//   5. replace 模式：目标已有方块时直接覆盖。测试先摆 stone，/setblock replace dirt，断言变 dirt。
//   6. SimulatedPlayer::chat permLevel 固定=4（≥2 满足 setblock 权限）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_setblock.txt（设置方块命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 1, y: 1, z: 1 };

// 结构相对坐标 → 世界绝对坐标字符串（命令用）。
function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

// /setblock <pos> <block> 默认 replace 模式放置方块：在空气格放 stone，断言出现。
// 走 SetBlockCommand::_setBlockState（默认 replace，onlyIfAir=false doDrop=false）。
// Ref: wiki commands/setblock.txt（setblock <pos> <block>）
function setblockPlacesBlock(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 2, z: 3 };

    player.chat(`/setblock ${worldCoords(test, pos)} minecraft:stone`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:stone", pos, true);
        test.succeed();
    });
}

// /setblock <pos> <block> replace 替换已有方块：先摆 stone，再 /setblock replace dirt，断言变 dirt。
// 走 SetBlockCommand::_setBlockReplace（onlyIfAir=false doDrop=false，直接覆盖）。
// Ref: wiki commands/setblock.txt（setblock <pos> <block> replace）
function setblockReplaceOverwrites(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 2, z: 3 };

    test.setBlockType("minecraft:stone", pos);
    test.assertBlockPresent("minecraft:stone", pos, true);

    player.chat(`/setblock ${worldCoords(test, pos)} minecraft:dirt replace`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:dirt", pos, true);
        test.assertBlockPresent("minecraft:stone", pos, false);
        test.succeed();
    });
}

// /setblock <pos> <block> keep 仅空气时放置：先摆 stone 占位，再 /setblock keep dirt，断言仍是 stone。
// 走 SetBlockCommand::_setBlockKeep（onlyIfAir=true，existingBlock非air → sendError 返 0 不放置）。
// Ref: wiki commands/setblock.txt（setblock <pos> <block> keep）
function setblockKeepSkipsNonAir(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 2, z: 3 };

    test.setBlockType("minecraft:stone", pos);

    player.chat(`/setblock ${worldCoords(test, pos)} minecraft:dirt keep`);

    test.runAtTickTime(5, () => {
        // stone 仍在，dirt 未放置（keep 跳过非空气）。
        test.assertBlockPresent("minecraft:stone", pos, true);
        test.assertBlockPresent("minecraft:dirt", pos, false);
        test.succeed();
    });
}

// /setblock <pos> <block> keep 空气时放置：目标为空气，/setblock keep dirt，断言变 dirt。
// 对照 setblockKeepSkipsNonAir，验证 keep 在空气时正常放置。
// Ref: wiki commands/setblock.txt（setblock <pos> <block> keep 空气语义）
function setblockKeepPlacesOnAir(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 2, z: 3 };

    player.chat(`/setblock ${worldCoords(test, pos)} minecraft:dirt keep`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:dirt", pos, true);
        test.succeed();
    });
}

// /setblock <pos> <block> destroy 破坏旧方块掉落物品后放置：先摆 stone（掉落 cobblestone），
// /setblock destroy dirt，断言位置变 dirt 且区域出现 item 实体（掉落物）。
// 走 SetBlockCommand::_setBlockDestroy（doDrop=true，BlockDropHandler::generateDrops+spawnDrops）。
// Ref: wiki commands/setblock.txt（setblock <pos> <block> destroy）
function setblockDestroyDropsAndReplaces(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 2, z: 3 };

    test.setBlockType("minecraft:stone", pos);

    player.chat(`/setblock ${worldCoords(test, pos)} minecraft:dirt destroy`);

    test.runAtTickTime(10, () => {
        test.assertBlockPresent("minecraft:dirt", pos, true);
        // stone 破坏掉落 cobblestone item 实体。查询 pos 周围 2 格内的 item 实体。
        const center = test.worldLocation(pos);
        const items = test.getDimension().getEntities({
            type: "item",
            location: center,
            volume: { x: 3, y: 3, z: 3 },
        });
        test.assert(items.length > 0, `expected dropped item after destroy, got ${items.length}`);
        test.succeed();
    });
}

// /setblock 带方块状态语法 block[state=val]：放 oak_stairs[facing=east]，断言方块类型为 oak_stairs。
// BlockStateArgument 支持 [state=val] 语法（BlockStateArgument.hpp:81）。assertBlockState 是 stub 只能
// 判方块类型不能判状态值（记忆 [[blockstate-property-name-facing-vs-horizontal-and-setblockwithstates-silent-ignore]]），
// 故仅断言类型，状态值正确性留 BlockStateArgument 单元测试覆盖。
// Ref: wiki commands/setblock.txt（setblock <pos> <block>[state=val] 带状态语法）
function setblockWithBlockStateSyntax(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 2, z: 3 };

    player.chat(`/setblock ${worldCoords(test, pos)} minecraft:oak_stairs[facing=east]`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:oak_stairs", pos, true);
        test.succeed();
    });
}

export function registerSetBlockTests(): void {
    GameTest.register("CommandTests", "setblock_places_block", setblockPlacesBlock)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "setblock_replace_overwrites", setblockReplaceOverwrites)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "setblock_keep_skips_non_air", setblockKeepSkipsNonAir)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "setblock_keep_places_on_air", setblockKeepPlacesOnAir)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "setblock_destroy_drops_and_replaces", setblockDestroyDropsAndReplaces)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "setblock_with_block_state_syntax", setblockWithBlockStateSyntax)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
