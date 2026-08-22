// /execute 命令 GameTest：条件与上下文修改子命令。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/execute.txt，Java 1.21.11 ExecuteCommand）：
//   - /execute run <command>：直接执行嵌套命令
//   - /execute positioned <pos> run <command>：把执行位置改为 <pos>，嵌套命令相对坐标 ~ 以 <pos> 为基准
//   - /execute at <entity> run <command>：把执行位置/旋转/维度改为目标实体，嵌套命令 ~ 以实体位置为基准（不改执行者）
//   - /execute as <entity> run <command>：把执行者改为目标实体，嵌套命令 @s 解析为目标实体（不改位置）
//   - /execute if block <pos> <block> run <command>：指定位置是指定方块时才执行
//   - /execute unless block <pos> <block> run <command>：指定位置不是指定方块时才执行
//
// 待实现子命令（不测）：align / facing / rotated / anchored（ExecuteCommand.hpp:47-50 标注待实现）。
// 维度切换子命令 in <dimension> 暂不测：GameTest 仅主世界有结构与可断言实体，下界/末地跨维度 setblock
// 难做确定性断言（维度是否在 GameTest 环境加载不可假设）。
//
// 设计要点：
//   1. 核心机制（已通过只读核查确认）：_executeNestedCommand 把修改后的 ServerCommandSource 按引用传给
//      registry.execute（ExecuteCommand.cpp:113），dispatcher 用该 source 重新解析嵌套命令全部参数。
//      相对坐标 ~ 经 _getAnchorPosition 取 source.position()（GameModeArgument.hpp:633-640，默认 Feet 锚点），
//      @s 经 EntityResolver::resolve 取 source.entity()（EntityResolver.cpp:662-698）。
//      故 withPosition/withEntity 的修改被嵌套命令正确消费。
//   2. as 不改 position/dimension（withEntity 仅改 m_entity + 条件改 m_player，ServerCommandSource.cpp:165-188），
//      at 改 position 不改 entity（ExecuteCommand.cpp:308-312 注释明确）——与原版语义一致。
//      故"在实体位置放方块"用 at，"以实体身份杀自身"用 as @e run kill @s。
//   3. 嵌套 setblock 经 chat 命令队列下一 tick 执行，断言用 runAtTickTime 延迟读回（对齐 SetBlockTests 范式）。
//   4. 坐标：命令用世界绝对坐标（worldCoords 转换），断言用结构相对坐标。cmd_arena 9×7×9。
//   5. 结构放置 Y 偏移（MinecraftStructurePlacer.cpp:115-122 有意设计）：结构内容放在 origin+1，
//      故 helper 相对 y=N 对应结构文件 y=N-1。cmd_arena 文件 y=0 stone 地板、y=1..5 空气腔、y=6 封顶，
//      映射到 helper 坐标：y=1 stone 地板、y=2..6 空气腔（封顶 y=7 超出 helper bounds）。
//      故空气腔在 helper y∈[2,6]，x,z∈[1,7]。玩家/方块目标必须落在 helper y∈[2,6] 空气腔，
//      不可用 y=1（stone 地板层，会被 assertBlockPresent 误判为已有 stone）。
//   6. as @e[type=cow] run kill @s：kill @s 杀目标 cow。@s 经 withEntity 设为 cow。需 distance=..10 区域
//      限定避免全维度 @e 误杀同批并行测试实体（同 EntityCommandTests kill 范式）。
//   7. if/unless block：用 setBlockType 预摆方块制造条件，嵌套 setblock 放置结果方块，断言结果方块
//      存在/不存在验证条件分支。if/unless 各测"满足执行"与"不满足不执行"两正反例。
//   8. SimulatedPlayer::chat permLevel 固定=4（≥2 满足 execute 权限）。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_execute.txt（execute 命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 玩家站立格：helper y=2（空气腔底部），下方 helper y=1 是 stone 地板（放置偏移后=文件 y=0 地板）支撑。
const PLAYER_POS = { x: 5, y: 2, z: 5 };

// 区域计数范围：cmd_arena 内部空气腔（helper 坐标）(1,2,1)..(7,6,7)。
const AREA_FROM = { x: 1, y: 2, z: 1 };
const AREA_VOLUME = { x: 7, y: 5, z: 7 };

// 结构相对坐标 → 世界绝对坐标字符串（命令用）。
function worldCoords(test: Test, rel: { x: number; y: number; z: number }): string {
    const w = test.worldLocation(rel);
    return `${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)}`;
}

/** 区域内指定 type 实体数量（排除 SimulatedPlayer，因 player 是独立 type）。 */
function countEntities(test: Test, type: string): number {
    return test.getDimension().getEntities({
        type,
        location: test.worldLocation(AREA_FROM),
        volume: AREA_VOLUME,
    }).length;
}

// /execute run <command> 直接执行嵌套命令：run setblock 在目标格放 stone，断言出现。
// 走 ExecuteCommand::_executeRun → _executeNestedCommand（不改 source，原样执行）。
// Ref: wiki execute.txt（execute run <command>）
function executeRunRunsNestedCommand(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const pos = { x: 3, y: 3, z: 3 };

    player.chat(`/execute run setblock ${worldCoords(test, pos)} minecraft:stone`);

    test.runAtTickTime(5, () => {
        test.assertBlockPresent("minecraft:stone", pos, true);
        test.succeed();
    });
}

// /execute positioned <pos> run setblock ~ ~ ~：把执行位置改为 <pos>，嵌套 ~ 基准为 <pos>。
// 验证 positioned 子命令 withPosition 修改 m_position 后，嵌套 setblock ~ ~ ~ 的 ~ 取 modifiedSource.position()。
// 玩家站 (5,2,5)，positioned 到 (3,3,3)，setblock ~ ~ ~ 应在 (3,3,3) 放方块（不是玩家位置）。
// Ref: wiki execute.txt（execute positioned <pos>）
function executePositionedSetsRelativeBase(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const target = { x: 3, y: 3, z: 3 };

    player.chat(`/execute positioned ${worldCoords(test, target)} run setblock ~ ~ ~ minecraft:stone`);

    test.runAtTickTime(5, () => {
        // 方块应在 positioned 的目标格 (3,3,3)，而非玩家格 (5,2,5)。
        test.assertBlockPresent("minecraft:stone", target, true);
        test.assertBlockPresent("minecraft:stone", PLAYER_POS, false);
        test.succeed();
    });
}

// /execute at <entity> run setblock ~ ~ ~：把执行位置改为实体位置，嵌套 ~ 基准为实体位置。
// 验证 at 子命令 withPosition 改为 entity->position()。spawn cow 在 (2,2,2)（空气腔底部，下方 y=1
// stone 地板支撑，牛稳定站立不因下落位移），at @e[type=cow]，setblock ~ ~ ~ 应在牛脚位置 (2,2,2) 放方块。
// Ref: wiki execute.txt（execute at <entity>，at 改位置不改执行者）
function executeAtUsesEntityPosition(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const cowPos = { x: 2, y: 2, z: 2 };
    test.spawn("cow", cowPos);

    // 等 cow 生成稳定后执行 at（at 外层 @e[type=cow] 解析需 cow 已存在）。
    test.runAtTickTime(5, () => {
        player.chat("/execute at @e[type=cow,distance=..10] run setblock ~ ~ ~ minecraft:stone");
    });

    test.runAtTickTime(10, () => {
        // 方块应在牛位置 cowPos（牛站 cowPos 脚位置，setblock ~ ~ ~ 在牛脚放方块）。
        test.assertBlockPresent("minecraft:stone", cowPos, true);
        test.assertBlockPresent("minecraft:stone", PLAYER_POS, false);
        test.succeed();
    });
}

// /execute as <entity> run kill @s：把执行者改为目标实体，嵌套 @s 解析为目标实体。
// 验证 as 子命令 withEntity 改 m_entity 后，嵌套 kill @s 的 @s 取 modifiedSource.entity()=cow。
// spawn cow + chicken，as @e[type=cow] run kill @s 应只杀 cow，chicken 保留。
// Ref: wiki execute.txt（execute as <entity>，as 改执行者不改位置）
function executeAsResolvesSelfToTarget(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.spawn("cow", { x: 2, y: 3, z: 2 });
    test.spawn("chicken", { x: 4, y: 3, z: 4 });

    test.runAtTickTime(5, () => {
        // as @e[type=cow,distance=..10]：选 cow 作执行者；kill @s 杀该 cow。
        // distance=..10 区域限定避免 @e 全维度误选同批并行测试的 cow（同 EntityCommandTests 范式）。
        player.chat("/execute as @e[type=cow,distance=..10] run kill @s");
    });

    pollUntilSucceed(test, () => countEntities(test, "cow") === 0 && countEntities(test, "chicken") >= 1, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `execute as cow kill @s failed: cow=${countEntities(test, "cow")}, chicken=${countEntities(test, "chicken")}`),
    });
}

// /execute as <entity> 对多实体：as @e 遍历所有目标，每个都执行一次嵌套命令。
// spawn 2 cow，as @e[type=cow] run kill @s 应杀全部 cow（验证 as 对多实体迭代）。
// Ref: wiki execute.txt（execute as 对每个实体执行一次）
function executeAsIteratesMultipleEntities(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    test.spawn("cow", { x: 2, y: 3, z: 2 });
    test.spawn("cow", { x: 6, y: 3, z: 6 });

    test.runAtTickTime(5, () => {
        player.chat("/execute as @e[type=cow,distance=..10] run kill @s");
    });

    pollUntilSucceed(test, () => countEntities(test, "cow") === 0, {
        startTick: 10,
        maxTick: 80,
        onTimeout: () => test.assert(false,
            `execute as multiple cows kill @s failed: cow=${countEntities(test, "cow")}`),
    });
}

// /execute if block <pos> <block> run：指定位置是指定方块时执行嵌套命令。
// 预摆 stone 在 (3,3,3)，if block <pos> stone → 嵌套 setblock 在 (4,3,3) 放 dirt，断言 dirt 出现。
// 走 ExecuteCommand::_executeIfBlock（matches=true → 执行嵌套命令）。
// Ref: wiki execute.txt（execute if block <pos> <block>）
function executeIfBlockRunsWhenMatches(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const condPos = { x: 3, y: 3, z: 3 };
    const resultPos = { x: 4, y: 3, z: 3 };

    test.setBlockType("minecraft:stone", condPos);
    test.assertBlockPresent("minecraft:stone", condPos, true);

    player.chat(`/execute if block ${worldCoords(test, condPos)} minecraft:stone run setblock ${worldCoords(test, resultPos)} minecraft:dirt`);

    test.runAtTickTime(5, () => {
        // 条件满足 → 嵌套 setblock 执行 → resultPos 出现 dirt。
        test.assertBlockPresent("minecraft:dirt", resultPos, true);
        test.succeed();
    });
}

// /execute if block <pos> <block> run：指定位置不是指定方块时不执行嵌套命令（反例）。
// 预摆 stone 在 (3,3,3)，if block <pos> dirt（实际是 stone 非 dirt）→ 嵌套 setblock 不执行，resultPos 无方块。
// 走 ExecuteCommand::_executeIfBlock（matches=false → return 0 不执行）。
// Ref: wiki execute.txt（execute if block 条件不满足不执行）
function executeIfBlockSkipsWhenNoMatch(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const condPos = { x: 3, y: 3, z: 3 };
    const resultPos = { x: 4, y: 3, z: 3 };

    test.setBlockType("minecraft:stone", condPos);

    player.chat(`/execute if block ${worldCoords(test, condPos)} minecraft:dirt run setblock ${worldCoords(test, resultPos)} minecraft:dirt`);

    test.runAtTickTime(5, () => {
        // 条件不满足（实际 stone 非 dirt）→ 嵌套 setblock 不执行 → resultPos 仍为空气。
        test.assertBlockPresent("minecraft:dirt", resultPos, false);
        test.succeed();
    });
}

// /execute unless block <pos> <block> run：指定位置不是指定方块时执行嵌套命令。
// 预摆 stone 在 (3,3,3)，unless block <pos> dirt（实际 stone 非 dirt，unless 成立）→ 嵌套 setblock 执行。
// 走 ExecuteCommand::_executeUnlessBlock（matches=false → 执行嵌套命令）。
// Ref: wiki execute.txt（execute unless block <pos> <block>）
function executeUnlessBlockRunsWhenNoMatch(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const condPos = { x: 3, y: 3, z: 3 };
    const resultPos = { x: 4, y: 3, z: 3 };

    test.setBlockType("minecraft:stone", condPos);

    player.chat(`/execute unless block ${worldCoords(test, condPos)} minecraft:dirt run setblock ${worldCoords(test, resultPos)} minecraft:dirt`);

    test.runAtTickTime(5, () => {
        // unless 成立（stone 非 dirt）→ 嵌套 setblock 执行 → resultPos 出现 dirt。
        test.assertBlockPresent("minecraft:dirt", resultPos, true);
        test.succeed();
    });
}

// /execute unless block <pos> <block> run：指定位置是指定方块时不执行嵌套命令（反例）。
// 预摆 stone 在 (3,3,3)，unless block <pos> stone（实际是 stone，unless 不成立）→ 嵌套 setblock 不执行。
// 走 ExecuteCommand::_executeUnlessBlock（matches=true → return 0 不执行）。
// Ref: wiki execute.txt（execute unless block 条件满足不执行）
function executeUnlessBlockSkipsWhenMatches(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const condPos = { x: 3, y: 3, z: 3 };
    const resultPos = { x: 4, y: 3, z: 3 };

    test.setBlockType("minecraft:stone", condPos);

    player.chat(`/execute unless block ${worldCoords(test, condPos)} minecraft:stone run setblock ${worldCoords(test, resultPos)} minecraft:dirt`);

    test.runAtTickTime(5, () => {
        // unless 不成立（实际是 stone）→ 嵌套 setblock 不执行 → resultPos 仍为空气。
        test.assertBlockPresent("minecraft:dirt", resultPos, false);
        test.succeed();
    });
}

export function registerExecuteTests(): void {
    GameTest.register("CommandTests", "execute_run_runs_nested_command", executeRunRunsNestedCommand)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "execute_positioned_sets_relative_base", executePositionedSetsRelativeBase)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "execute_at_uses_entity_position", executeAtUsesEntityPosition)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "execute_as_resolves_self_to_target", executeAsResolvesSelfToTarget)
        .structureName("gametests:cmd_arena")
        .maxTicks(120);

    GameTest.register("CommandTests", "execute_as_iterates_multiple_entities", executeAsIteratesMultipleEntities)
        .structureName("gametests:cmd_arena")
        .maxTicks(120);

    GameTest.register("CommandTests", "execute_if_block_runs_when_matches", executeIfBlockRunsWhenMatches)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "execute_if_block_skips_when_no_match", executeIfBlockSkipsWhenNoMatch)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "execute_unless_block_runs_when_no_match", executeUnlessBlockRunsWhenNoMatch)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "execute_unless_block_skips_when_matches", executeUnlessBlockSkipsWhenMatches)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
