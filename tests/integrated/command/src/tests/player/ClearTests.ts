// /clear 命令 GameTest：清除玩家背包物品（专项覆盖）。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/clear.txt）：
//   - /clear [player]：清除全部物品（通配符谓词，_clearSelf / _clearPlayer 分支）
//   - /clear <player> <item>：清除指定物品（Mode::Item 谓词）
//   - /clear <player> <item> <maxCount>：清除指定物品并限最大数量
//   - /clear <player> <#tag>：清除匹配标签的所有物品（Mode::Tag 谓词）
//   - /clear <player> *：通配符清除全部物品（Mode::Any 谓词，区别于无参 _clearSelf）
//
// 设计要点：
//   1. ClearCommand 经 support::resolvePlayerInventory（PlayerResolver）取背包：优先 InventoryManager
//      （真实玩家网络层权威），回退实体层 Player::m_inventory（SimulatedPlayer 权威背包，不在
//      InventoryManager 注册）。此前直接用 server->playerInventory 对 SimulatedPlayer 返 nullptr 致
//      /clear 失效，已修复（与 GiveCommand 同源缺陷，见 GiveTests.ts 头注）。本批专项覆盖此前未测的
//      item 谓词 / maxCount / 标签 / 通配符 / 选择器分支。
//   2. 判定背包物品用 EntityInventoryComponent（getComponent("minecraft:inventory")）.container（Container），
//      Container 包装 Player::m_inventory（实体层，与修复后 clear 写入层一致），getItem(slot) 返回 ItemStack
//      拷贝（typeId/amount）。遍历 0..size-1 求和（add 可能跨槽分散，求和比单槽更准确）。
//   3. ItemPredicateArgumentType（ItemArgument.cpp:137）支持三种模式：*（Any）/ #tag（Tag）/ item（Item）。
//      test() 走 ItemTags::getTag 查标签成员（服务端 RegistryBootstrap 已 initialize + 数据包加载，
//      #minecraft:flowers 含 dandelion/poppy 等小型花朵，#minecraft:carpets 含 16 色地毯）。
//   4. maxCount 限额：clearInventory 遍历槽累加 removedCount，剩余 remaining 初始=maxCount，移除到
//      remaining<=0 停止。故 give 32 stone + /clear @s stone 10 应只移除 10，余 22。
//   5. SimulatedPlayer::chat permLevel 固定=4（与游戏模式解耦），任意模式可执行管理命令（permLevel≥2）。
//   6. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//   7. 每个 SimulatedPlayer 是独立实例，背包初始为空，无跨测试污染；用普通 batch（默认）即可并行。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_clear.txt（清除物品命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// Container / EntityInventoryComponent 是 @minecraft/server 类型，但 getComponent 返回联合类型，
// 取 container 后 getItem 返回 ItemStack | undefined。用 any 绕过 TS 联合类型校验（运行时 Cubium opaque）。
interface PlayerContainer {
    readonly size: number;
    getItem(slot: number): { typeId: string; amount: number } | undefined;
}

// 遍历玩家背包，统计指定 typeId 的物品总数量（跨槽合并求和）。
function countItemInInventory(container: PlayerContainer, typeId: string): number {
    let total = 0;
    for (let i = 0; i < container.size; ++i) {
        const item = container.getItem(i);
        if (item !== undefined && item.typeId === typeId) {
            total += item.amount;
        }
    }
    return total;
}

// 统计背包中所有非空槽的物品总数（跨 typeId 求和），用于断言全清后归零。
function countAllItems(container: PlayerContainer): number {
    let total = 0;
    for (let i = 0; i < container.size; ++i) {
        const item = container.getItem(i);
        if (item !== undefined) {
            total += item.amount;
        }
    }
    return total;
}

// 取玩家背包 Container（经 minecraft:inventory 组件）。
function playerContainer(player: unknown): PlayerContainer {
    const comp = (player as any).getComponent("minecraft:inventory") as any;
    return comp.container as PlayerContainer;
}

// /clear @s <item> 清除指定物品（Mode::Item 谓词）：先 give 16 stone，再 /clear @s stone，断言归零。
// 走 ClearCommand::_clearPlayerItem 分支（带 player+item 参数，无 maxCount）。
// 区别 GiveTests.give_then_clear_removes_item：此处独立验证 _clearPlayerItem 谓词分支（不混入 give 语义）。
// Ref: wiki commands/clear.txt（clear <player> <item>）
function clearSpecificItem(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 16");
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 16,
        "precondition: 16 stone after give",
    );

    player.chat("/clear @s stone");

    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 0,
        "stone should be 0 after /clear @s stone",
    );
    test.succeed();
}

// /clear @s <item> <maxCount> 限额清除：give 32 stone + /clear @s stone 10，只移除 10，余 22。
// 走 ClearCommand::_clearPlayerItemCount 分支（带 player+item+maxCount）。验证 maxCount 上限语义
// （clearInventory remaining=maxCount 累减，到 0 停止）。
// Ref: wiki commands/clear.txt（clear <player> <item> <maxCount>）
function clearItemWithMaxCount(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 32");

    player.chat("/clear @s stone 10");

    const after = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(after === 22, `expected 22 stone after clearing 10 of 32, got ${after}`);
    test.succeed();
}

// /clear @s <item> <maxCount> 当 maxCount ≥ 持有量时清空全部：give 8 stone + /clear @s stone 100，余 0。
// 验证 maxCount 超过持有量不报错、全清（remaining 够用，遍历完所有槽）。
// Ref: wiki commands/clear.txt（maxCount 超量语义）
function clearItemMaxCountExceedsHeld(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 8");
    player.chat("/clear @s stone 100");

    const after = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(after === 0, `expected 0 stone after clearing 100 of 8, got ${after}`);
    test.succeed();
}

// /clear @s <#tag> 标签匹配清除：give dandelion + poppy（均属 #minecraft:flowers 标签）+ stone（非花），
// /clear @s #minecraft:flowers，断言 dandelion/poppy 归零、stone 保留。
// 走 ClearCommand::_clearPlayerItem 分支 Mode::Tag 谓词（ItemTags::getTag("minecraft:flowers").contains）。
// 验证标签谓词跨多物品匹配 + 不误伤标签外物品。
// Ref: wiki commands/clear.txt（clear <player> <#tag> 标签语法）
function clearByItemTag(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s dandelion 4");
    player.chat("/give @s poppy 4");
    player.chat("/give @s stone 4");
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:dandelion") === 4,
        "precondition: 4 dandelion",
    );

    player.chat("/clear @s #minecraft:flowers");

    const container = playerContainer(player);
    test.assert(
        countItemInInventory(container, "minecraft:dandelion") === 0,
        "dandelion (#flowers) should be cleared",
    );
    test.assert(
        countItemInInventory(container, "minecraft:poppy") === 0,
        "poppy (#flowers) should be cleared",
    );
    test.assert(
        countItemInInventory(container, "minecraft:stone") === 4,
        "stone (not #flowers) should remain",
    );
    test.succeed();
}

// /clear @s * 通配符清除全部物品（Mode::Any 谓词，显式 *）：give 多种物品，/clear @s *，断言全空。
// 走 ClearCommand::_clearPlayerItem 分支 Mode::Any 谓词（与无参 _clearSelf 同为通配，但路径不同：
// 此处经 item 参数解析 *，_clearSelf 是无参构造 anyPredicate）。验证显式 * 解析与全清。
// Ref: wiki commands/clear.txt（clear <player> <*> 通配符）
function clearWithWildcardAll(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 8");
    player.chat("/give @s dirt 8");
    player.chat("/give @s cobblestone 8");
    test.assert(countAllItems(playerContainer(player)) === 24, "precondition: 24 items total");

    player.chat("/clear @s *");

    test.assert(countAllItems(playerContainer(player)) === 0, "all items should be cleared by /clear @s *");
    test.succeed();
}

// /clear <player> 带选择器目标（@s）清除全部（_clearPlayer 分支，无 item）：give 物品，/clear @s，断言全空。
// 区别 _clearSelf（无参）：此处显式带 @s player 参数走 _clearPlayer。验证选择器目标解析 + 全清。
// Ref: wiki commands/clear.txt（clear <player> 清除全部）
function clearPlayerSelectorAll(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 16");
    player.chat("/give @s dirt 16");
    test.assert(countAllItems(playerContainer(player)) === 32, "precondition: 32 items total");

    player.chat("/clear @s");

    test.assert(countAllItems(playerContainer(player)) === 0, "all items should be cleared by /clear @s");
    test.succeed();
}

// /clear @s <item> 对背包中不存在的物品：give stone，/clear @s dirt（背包无 dirt），断言 stone 保留、
// 无副作用。clearInventory 遍历无匹配槽 removedCount=0，sendClearMessage 走 "No matching items" 分支
// （removedCount<=0），不报错、不改背包。验证无匹配物品不误清。
// Ref: wiki commands/clear.txt（clear 不匹配物品语义）
function clearNonExistentItemNoOp(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 16");

    player.chat("/clear @s dirt");

    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 16,
        "stone should remain when clearing absent dirt",
    );
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:dirt") === 0,
        "dirt should still be 0",
    );
    test.succeed();
}

// /clear @s <item> <maxCount> 跨槽累加限额：give 64 stone（满单槽）+ 再 give 64 stone（溢出到第二槽，
// 共 128），/clear @s stone 70，应移除 70（第一槽 64 全清 + 第二槽 6），余 58。
// 验证 clearInventory 跨槽累减 remaining 正确（非单槽就停）。
// Ref: wiki commands/clear.txt（maxCount 跨槽累加语义）
function clearMaxCountAcrossSlots(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // 两次 give 各 64，单槽 maxStack=64，第二次溢出到新槽，共 128。
    player.chat("/give @s stone 64");
    player.chat("/give @s stone 64");
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 128,
        "precondition: 128 stone across slots",
    );

    player.chat("/clear @s stone 70");

    const after = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(after === 58, `expected 58 stone after clearing 70 of 128, got ${after}`);
    test.succeed();
}

export function registerClearTests(): void {
    GameTest.register("CommandTests", "clear_specific_item", clearSpecificItem)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_item_with_max_count", clearItemWithMaxCount)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_item_max_count_exceeds_held", clearItemMaxCountExceedsHeld)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_by_item_tag", clearByItemTag)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_with_wildcard_all", clearWithWildcardAll)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_player_selector_all", clearPlayerSelectorAll)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_non_existent_item_no_op", clearNonExistentItemNoOp)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_max_count_across_slots", clearMaxCountAcrossSlots)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
