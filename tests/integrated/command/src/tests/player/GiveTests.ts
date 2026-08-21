// /give 与 /clear 命令 GameTest：给予/清除玩家背包物品。
//
// 覆盖 wiki 命令章节核心行为：
//   - /give <player> <item> [count]：给予玩家指定物品（Ref: wiki commands/give.txt）
//   - /clear [player] [item] [maxCount]：清除玩家背包物品（Ref: wiki commands/clear.txt）
//
// 设计要点：
//   1. GiveCommand/ClearCommand 此前经 server->playerInventory(playerId)（InventoryManager 数据层）取背包，
//      但 SimulatedPlayer 不走登录流程（LoginFlow::initializeInventory），不在 InventoryManager 注册，
//      getInventory 返 nullptr 致命令 continue 跳过——/give /clear 对 SimulatedPlayer 完全失效。
//      已补 support::resolvePlayerInventory（PlayerResolver）：优先 InventoryManager（真实玩家网络层权威背包），
//      回退经 ServerPlayerEntityManager 取实体层 Player::m_inventory（SimulatedPlayer 权威背包）。对齐
//      EffectCommand/GameModeCommand/TeleportCommand 经 ServerPlayerEntityManager 旁路 SimulatedPlayer 模式。
//      真实玩家必须操作 InventoryManager 背包（BlockInteractionManager 等以 InventoryManager 为权威，操作前
//      从其同步到 Player::m_inventory，若 give 写实体层会被下次操作覆盖丢失），故不可全用实体层。
//   2. 判定背包物品用 EntityInventoryComponent（getComponent("minecraft:inventory")）.container（Container），
//      Container 包装 Player::m_inventory（实体层，与修复后 give/clear 写入层一致），getItem(slot) 返回 ItemStack
//      拷贝（typeId/amount 属性）。遍历 0..size-1 找目标物品，避免硬编码槽位（add 优先合并选中槽但选中槽可能
//      因前序测试残留偏移，遍历更稳健）。
//   3. SimulatedPlayer::chat permLevel 已固定为 4（与游戏模式解耦），任意模式可执行管理命令。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_give.txt（给予物品）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_clear.txt（清除物品）

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
// give 的 add 可能将物品分散到多个槽（如超过单槽 maxStack 64），求和断言比单槽更准确。
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

// 取玩家背包 Container（经 minecraft:inventory 组件）。
function playerContainer(player: unknown): PlayerContainer {
    const comp = (player as any).getComponent("minecraft:inventory") as any;
    return comp.container as PlayerContainer;
}

// /give @s stone 给予自身 1 个石头（默认 count=1）。
// give 同步执行，背包立即写入。断言背包中 stone 总数 === 1。
// 同时验证 PlayerInventory::add 返回值语义修复（此前 add 返回"已添加"而非"剩余"，
// GiveCommand 把已添加当 notAdded 误判未加进 → while remaining 不减死循环无限掉落，
// /give @s stone 得 2368 个石头塞满 41 槽；修复后 add 返回 0 致 notAdded=0 正常加 1 个）。
// Ref: wiki commands/give.txt（give <player> <item> [count]，默认 1）
function givePlacesItemInInventory(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    const before = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(before === 0, `expected empty inventory before give, got stone=${before}`);

    player.chat("/give @s stone");

    const after = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(after === 1, `expected 1 stone after /give @s stone, got ${after}`);
    test.succeed();
}

// /give @s stone 32 给予自身 32 个石头（指定 count）。
// 32 < stone maxStack(64)，全部放入单槽。断言 stone 总数 === 32。
// Ref: wiki commands/give.txt（count 参数）
function giveCountParam(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 32");

    const after = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(after === 32, `expected 32 stone after /give @s stone 32, got ${after}`);
    test.succeed();
}

// /give 后 /clear 清除指定物品往返：先 give 16 stone，再 /clear @s stone，断言背包无 stone。
// 同时验证 /clear 对 SimulatedPlayer 生效（与 give 同源 InventoryManager 缺陷修复，ClearCommand 改用
// resolvePlayerInventory 回退实体层）。
// Ref: wiki commands/clear.txt（clear <player> <item> 清除指定物品）
function giveThenClearRemovesItem(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/give @s stone 16");
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 16,
        "give 16 stone should place 16 stone before clear",
    );

    player.chat("/clear @s stone");

    const after = countItemInInventory(playerContainer(player), "minecraft:stone");
    test.assert(after === 0, `expected 0 stone after /clear @s stone, got ${after}`);
    test.succeed();
}

// /clear 不带参数清除所有物品：先 give 多种物品，再 /clear @s，断言背包全空。
// 走 ClearCommand::_clearSelf 通配符谓词分支（清除所有槽所有物品）。
// Ref: wiki commands/clear.txt（clear <player> 清除全部）
function clearAllEmptiesInventory(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    // give 三种不同物品，散布在背包。
    player.chat("/give @s stone 8");
    player.chat("/give @s dirt 8");
    player.chat("/give @s cobblestone 8");
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 8,
        "give 8 stone should place 8 stone",
    );

    player.chat("/clear @s");

    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:stone") === 0,
        "stone should be cleared by /clear @s",
    );
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:dirt") === 0,
        "dirt should be cleared by /clear @s",
    );
    test.assert(
        countItemInInventory(playerContainer(player), "minecraft:cobblestone") === 0,
        "cobblestone should be cleared by /clear @s",
    );
    test.succeed();
}

export function registerGiveTests(): void {
    GameTest.register("CommandTests", "give_places_item_in_inventory", givePlacesItemInInventory)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "give_count_param", giveCountParam)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "give_then_clear_removes_item", giveThenClearRemovesItem)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "clear_all_empties_inventory", clearAllEmptiesInventory)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
