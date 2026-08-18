// /enchant 命令 GameTest：给玩家主手物品附魔。
//
// 覆盖 wiki 命令章节核心行为：
//   - /enchant <player> <enchantment> [level]：给目标玩家主手物品添加附魔（Ref: wiki commands/enchant.txt）
//
// 设计要点：
//   1. EnchantCommand 此前有两个缺陷：
//      a. playerData 前置守卫（playerManager.getPlayer）对不进 PlayerManager 的 SimulatedPlayer 返 nullptr
//         跳过，致 /enchant 对其失效。已删除守卫，改用 ServerPlayerEntityManager.getPlayerEntity 解析实体。
//      b. getSelectedStack() 按值返回副本，addEnchantment 改副本未回写 player->inventory()，致附魔对全玩家
//         （含真实玩家）静默失效——命令报 "Applied ..." 但主手物品无附魔。已改用 getSelectedStackRef()
//         拿可变引用原地修改权威槽修复。
//   2. 判定附魔生效用 ItemStack.getEnchantments()（已绑定，对齐基岩 ItemStack.getEnchantments），
//      返回 { type, level } 对象数组。经 EquippableComponent.getEquipment("Mainhand") 拿主手 ItemStack
//      （owned 拷贝含 NBT 深拷贝，附魔在 NBT 中），调 getEnchantments 读附魔。
//   3. 主手物品经 SimulatedPlayer.setItem(stack, 0, true) 设入槽0并选中（selectSlot=true 同步主手）。
//   4. SimulatedPlayer::chat permLevel 已固定为 4，任意模式可执行管理命令。
//   5. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_enchant.txt

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// Cubium 的 @minecraft/server 与 @minecraft/server-gametest 依赖的 @minecraft/server 是两个独立包实例，
// ItemStack 类型不兼容（setItem 期望 gametest 子依赖的 ItemStack）。用 as any 绕过类型校验
// （同 MobSpawnerTests useItemOnBlock 的 as unknown 范式）。
function makeItem(typeId: string): any {
    return new ItemStack(typeId, 1);
}

// 取玩家主手物品的附魔列表（经 EquippableComponent.getEquipment 拿主手 ItemStack 拷贝）。
function getMainhandEnchantments(player: any): Array<{ type: string; level: number }> {
    const equippable = player.getComponent("minecraft:equippable");
    if (equippable === undefined) {
        return [];
    }
    const mainhand = equippable.getEquipment("Mainhand");
    if (mainhand === undefined) {
        return [];
    }
    return mainhand.getEnchantments() as Array<{ type: string; level: number }>;
}

// /enchant @s <enchantment> 给自身主手物品附魔（默认 level 1）。
// Ref: wiki commands/enchant.txt（enchant <player> <enchantment> [<level>]）
function enchantAddsSharpnessToHeldItem(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 主手放钻石剑（槽0并选中）。
    player.setItem(makeItem("minecraft:diamond_sword"), 0, true);

    player.chat("/enchant @s sharpness");

    const enchantments = getMainhandEnchantments(player);
    const sharpness = enchantments.find((e) => e.type === "minecraft:sharpness");
    test.assert(sharpness !== undefined, `expected sharpness enchantment, got ${JSON.stringify(enchantments)}`);
    test.assert(sharpness!.level === 1, `expected sharpness level 1, got ${sharpness?.level}`);
    test.succeed();
}

// /enchant @s <enchantment> <level> 指定等级（sharpness III）。
// Ref: wiki commands/enchant.txt（level 参数）
function enchantAddsSpecifiedLevel(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    player.setItem(makeItem("minecraft:diamond_sword"), 0, true);

    player.chat("/enchant @s sharpness 3");

    const enchantments = getMainhandEnchantments(player);
    const sharpness = enchantments.find((e) => e.type === "minecraft:sharpness");
    test.assert(sharpness !== undefined, `expected sharpness enchantment, got ${JSON.stringify(enchantments)}`);
    test.assert(sharpness!.level === 3, `expected sharpness level 3, got ${sharpness?.level}`);
    test.succeed();
}

// /enchant 对空手玩家不附魔（主手无物品时命令报错，附魔列表仍为空）。
// Ref: wiki commands/enchant.txt（无手持物品时报错）
function enchantFailsOnEmptyHand(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    // 不放任何物品（主手空）。

    player.chat("/enchant @s sharpness");

    // 空手不应有任何附魔。
    const enchantments = getMainhandEnchantments(player);
    test.assert(enchantments.length === 0, `expected no enchantments on empty hand, got ${JSON.stringify(enchantments)}`);
    test.succeed();
}

// /enchant 不兼容附魔被拒绝（保护与火焰保护互斥，第二次附魔不应生效）。
// Ref: wiki commands/enchant.txt（不兼容附魔报错）
function enchantRejectsIncompatible(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    player.setItem(makeItem("minecraft:diamond_chestplate"), 0, true);

    // 先附保护。
    player.chat("/enchant @s protection");
    test.assert(
        getMainhandEnchantments(player).some((e) => e.type === "minecraft:protection"),
        "protection should be applied first",
    );

    // 再附火焰保护（与保护互斥，应被拒绝，保护仍在）。
    player.chat("/enchant @s fire_protection");

    const enchantments = getMainhandEnchantments(player);
    test.assert(
        enchantments.some((e) => e.type === "minecraft:protection"),
        "protection should remain after incompatible fire_protection rejected",
    );
    test.assert(
        !enchantments.some((e) => e.type === "minecraft:fire_protection"),
        "fire_protection should not be applied (incompatible with protection)",
    );
    test.succeed();
}

export function registerEnchantTests(): void {
    GameTest.register("CommandTests", "enchant_adds_sharpness_to_held_item", enchantAddsSharpnessToHeldItem)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "enchant_adds_specified_level", enchantAddsSpecifiedLevel)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "enchant_fails_on_empty_hand", enchantFailsOnEmptyHand)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "enchant_rejects_incompatible", enchantRejectsIncompatible)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
