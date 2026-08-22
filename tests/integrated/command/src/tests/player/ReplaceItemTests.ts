// /replaceitem 命令 GameTest：替换实体物品槽。
//
// 覆盖 wiki 命令章节核心行为：
//   - /replaceitem entity <target> <slot> <item> [count]：替换指定槽位物品
//     （Ref: wiki commands/replaceitem.txt）
//
// 语法差异（重要）：vanilla Java 语法为 /replaceitem entity <t> <slot> with <item> [count]，
// 但 Cubium 的 CommandNode 注册省略了 "with" 关键字，实际语法为
// /replaceitem entity <targets> <slot> <item> [count]（见 ReplaceItemCommand.cpp:336 元数据
// /replaceitem <entity|block> <target> <slot> <item> [count]，slot→item→[count] 链式节点）。
// 故本测试命令不带 "with"。
//
// 设计要点：
//   1. ReplaceItemCommand 经 support::resolvePlayerInventory(source, playerId) 取玩家库存
//      （优先 InventoryManager 真实玩家，回退实体层 Player::m_inventory SimulatedPlayer），
//      再 setEntitySlotItem 写入槽位。此前直接用 playerManager().getPlayer 对 SimulatedPlayer
//      失效（SimulatedPlayer 不在 PlayerManager 注册，记忆 [[simulated-player-wear-armor-via-setitem-not-replaceitem]]
//      记录的旧缺陷），已随 resolvePlayerInventory 修复（commit b2d2dc04d）对 SimulatedPlayer 生效。
//      本测试验证该修复 + 覆盖 replaceitem 命令本身。
//   2. 槽位名（Java NBT slot 名，ItemSlotArgument.hpp:412-413）：
//      - weapon.mainhand(98) → 玩家当前选中快捷栏槽位（selectedSlot）
//      - armor.chest(37) → 胸甲槽
//      - weapon.offhand(40) → 副手槽
//   3. 断言用已绑定的 EquippableComponent.getEquipment(slot) 读装备槽（返 owned ItemStack 拷贝，
//      有 typeId；slot 参数 "Mainhand"/"Chest"/"Offhand"，MinecraftModuleFactory.cpp:1528-1541）。
//      equippable 组件对 SimulatedPlayer 已重绑（ScriptSimulatedPlayer.cpp getComponent）。
//   4. @s 选择器经 source.entity() 返回 SimulatedPlayer 自身，无跨测试污染。独占 batch 串行 +
//      runOnFinish 清空槽位（replaceitem air）防污染后续依赖空装备槽的测试。
//   5. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_replaceitem.txt（替换物品命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 读玩家装备槽物品 typeId，空槽返 undefined。
function getEquipTypeId(player: any, slot: string): string | undefined {
    const equippable = player.getComponent("minecraft:equippable");
    if (equippable === undefined) {
        return undefined;
    }
    const stack = equippable.getEquipment(slot);
    if (stack === undefined) {
        return undefined;
    }
    return stack.typeId;
}

// /replaceitem entity @s armor.chest iron_chestplate 替换胸甲槽为铁胸甲，
// 断言 getEquipment("Chest").typeId=="minecraft:iron_chestplate"。
// runOnFinish 清空胸甲槽防污染。
// Ref: wiki commands/replaceitem.txt（replaceitem entity ... armor.chest with ...）
function replaceItemArmorChest(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    test.assert(getEquipTypeId(player, "Chest") === undefined, "chest should be empty before");

    player.chat("/replaceitem entity @s armor.chest iron_chestplate");

    const typeId = getEquipTypeId(player, "Chest");
    test.assert(
        typeId === "minecraft:iron_chestplate",
        `expected chest=iron_chestplate, got "${typeId}"`,
    );

    test.runOnFinish(() => {
        player.chat("/replaceitem entity @s armor.chest air");
    });
    test.succeed();
}

// /replaceitem entity @s weapon.mainhand diamond_sword 替换主手为钻石剑，
// 断言 getEquipment("Mainhand").typeId=="minecraft:diamond_sword"。
// runOnFinish 清空主手。
// Ref: wiki commands/replaceitem.txt（weapon.mainhand）
function replaceItemWeaponMainhand(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    test.assert(getEquipTypeId(player, "Mainhand") === undefined, "mainhand should be empty before");

    player.chat("/replaceitem entity @s weapon.mainhand diamond_sword");

    const typeId = getEquipTypeId(player, "Mainhand");
    test.assert(
        typeId === "minecraft:diamond_sword",
        `expected mainhand=diamond_sword, got "${typeId}"`,
    );

    test.runOnFinish(() => {
        player.chat("/replaceitem entity @s weapon.mainhand air");
    });
    test.succeed();
}

// /replaceitem entity @s weapon.offhand shield 替换副手为盾牌，
// 断言 getEquipment("Offhand").typeId=="minecraft:shield"。
// runOnFinish 清空副手。
// Ref: wiki commands/replaceitem.txt（weapon.offhand）
function replaceItemWeaponOffhand(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    test.assert(getEquipTypeId(player, "Offhand") === undefined, "offhand should be empty before");

    player.chat("/replaceitem entity @s weapon.offhand shield");

    const typeId = getEquipTypeId(player, "Offhand");
    test.assert(
        typeId === "minecraft:shield",
        `expected offhand=shield, got "${typeId}"`,
    );

    test.runOnFinish(() => {
        player.chat("/replaceitem entity @s weapon.offhand air");
    });
    test.succeed();
}

// 替换覆盖：先 replaceitem 主手放 stone，再 replaceitem 主手放 dirt，
// 断言主手是 dirt（验证 replaceitem 覆盖旧物品非追加）。
// Ref: wiki commands/replaceitem.txt（替换覆盖原槽位物品）
function replaceItemOverwrites(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/replaceitem entity @s weapon.mainhand stone");
    test.assert(
        getEquipTypeId(player, "Mainhand") === "minecraft:stone",
        "after first replace should be stone",
    );

    player.chat("/replaceitem entity @s weapon.mainhand dirt");
    test.assert(
        getEquipTypeId(player, "Mainhand") === "minecraft:dirt",
        "after second replace should overwrite to dirt",
    );

    test.runOnFinish(() => {
        player.chat("/replaceitem entity @s weapon.mainhand air");
    });
    test.succeed();
}

// replaceitem air 清空槽位：先放铁胸甲，再 replaceitem air，断言胸甲槽空。
// 验证 air 作为物品清空槽位（与 runOnFinish 恢复同机制）。
// Ref: wiki commands/replaceitem.txt（with air 清空）
function replaceItemAirClearsSlot(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/replaceitem entity @s armor.chest iron_chestplate");
    test.assert(
        getEquipTypeId(player, "Chest") === "minecraft:iron_chestplate",
        "precondition: chest should have iron_chestplate",
    );

    player.chat("/replaceitem entity @s armor.chest air");
    test.assert(
        getEquipTypeId(player, "Chest") === undefined,
        "after replace air, chest should be empty",
    );

    // 已清空，无需 runOnFinish。
    test.succeed();
}

export function registerReplaceItemTests(): void {
    // @s 操作自身玩家无跨测试污染，独占 batch 串行避免同批并行干扰。
    GameTest.register("CommandTests", "replaceitem_armor_chest", replaceItemArmorChest)
        .structureName("gametests:cmd_arena")
        .batch("replaceitem_chest_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "replaceitem_weapon_mainhand", replaceItemWeaponMainhand)
        .structureName("gametests:cmd_arena")
        .batch("replaceitem_mainhand_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "replaceitem_weapon_offhand", replaceItemWeaponOffhand)
        .structureName("gametests:cmd_arena")
        .batch("replaceitem_offhand_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "replaceitem_overwrites", replaceItemOverwrites)
        .structureName("gametests:cmd_arena")
        .batch("replaceitem_overwrite_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "replaceitem_air_clears_slot", replaceItemAirClearsSlot)
        .structureName("gametests:cmd_arena")
        .batch("replaceitem_air_solo")
        .maxTicks(60);
}
