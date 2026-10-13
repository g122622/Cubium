// 荆棘附魔：用受控的真实玩家近战验证装备反伤及无附魔负向对照。
// 每次攻击间隔二十 tick，恢复受害者血量与双方位置，避免死亡、击退和寻路减少采样次数。
// 荆棘 III 每次触发概率 45%，三十次确认攻击均未触发的理论概率约 1.6e-8。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";

// 读取实体当前血量（HP）。实体句柄由闭包持有，规避区域查询污染。实体死亡移除后返回 -1。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 MeleeEnchantDamageTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给玩家穿钻石胸甲并施加荆棘 III（经 addEnchantment + setEquipment）。
//   - addEnchantment({type:"minecraft:thorns", level:3})：直接给 ItemStack 设附魔（/enchant 仅对玩家
//     生效但需 permLevel，mob 须用此 API；SimulatedPlayer 亦可用 setEquipment 直接穿戴已附魔物品）。
//   - setEquipment("Chest", stack)：写入装备数组。荆棘在 actuallyHurt 内直接查 getEnchantmentLevel，
//     不经属性管线，故 setEquipment 后无需等修饰符应用。
function equipThornsChest(player: any): void {
    const chest = makeItem("minecraft:diamond_chestplate");
    chest.addEnchantment({ type: "minecraft:thorns", level: 3 });
    player.getComponent("minecraft:equippable").setEquipment("Chest", chest);
}

// 给玩家穿无附魔钻石胸甲（反向对照，验证无荆棘护甲不触发反伤）。
function equipPlainChest(player: any): void {
    player.getComponent("minecraft:equippable").setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
}

// 确认真实受击后再检查反伤，避免攻击未发生时把寻路失败误报为附魔失败。
function thornsIiiReflectsDamageToAttacker(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "victim", 0 as any);
    const attacker = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);
    equipThornsChest(victim);
    const victimPosition = { ...victim.location };
    const attackerPosition = { ...attacker.location };
    for (let tick = 30; tick <= 610; tick += 20) {
        test.runAtTickTime(tick, () => {
            victim.teleport(victimPosition);
            attacker.teleport(attackerPosition);
            (victim.getComponent("minecraft:health") as any).setCurrentValue(20);
            const before = readHp(attacker);
            attacker.lookAtEntity(victim);
            test.assert(attacker.attackEntity(victim), "controlled melee attack should succeed");
            test.assert(readHp(victim) < 20, "thorns victim must receive actual melee damage");
            if (readHp(attacker) < before) {
                test.succeed();
            } else if (tick === 610) {
                test.assert(false, "thorns III failed to reflect damage across 30 confirmed melee attacks");
            }
        });
    }
}

// 无附魔护甲必须承受同一真实攻击，但不能反伤攻击者。
function noThornsNoReflection(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "victim", 0 as any);
    const attacker = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);
    equipPlainChest(victim);
    test.runAtTickTime(30, () => {
        attacker.lookAtEntity(victim);
        test.assert(attacker.attackEntity(victim), "controlled melee attack should succeed");
        test.assert(readHp(victim) < 20, "plain armor victim must receive actual melee damage");
        test.assert(readHp(attacker) === 20, "unenchanted armor must not reflect melee damage");
        test.succeed();
    });
}

export function registerThornsEnchantTests(): void {
    GameTest.register("MobBehaviorTests", "thorns_iii_reflects_damage_to_attacker", thornsIiiReflectsDamageToAttacker)
        .batch("night_thorns_reflection")
        .structureName("gametests:creeper_pit")
        .maxTicks(700);

    GameTest.register("MobBehaviorTests", "no_thorns_no_reflection", noThornsNoReflection)
        .batch("night_without_thorns")
        .structureName("gametests:creeper_pit")
        .maxTicks(500);
}
