// 爆炸保护击退对照：固定实体身份和移动速度，比较同一次爆炸产生的水平速度。
import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 对称斜向布局使两头牛距爆点约 2.83 格，普通钻石甲也能承受爆炸伤害。
const CREEPER_POS = { x: 3, y: 2, z: 1 };
const PLAYER_POS = { x: 3, y: 2, z: 6 };
const COW_A_POS = { x: 5, y: 2, z: 3 };
const COW_B_POS = { x: 1, y: 2, z: 3 };
const ARMOR = [
    ["Head", "minecraft:diamond_helmet"],
    ["Chest", "minecraft:diamond_chestplate"],
    ["Legs", "minecraft:diamond_leggings"],
    ["Feet", "minecraft:diamond_boots"],
] as const;

function equipDiamond(victim: any, protectedArmor: boolean): void {
    const equipment = victim.getComponent("minecraft:equippable");
    for (const [slot, itemId] of ARMOR) {
        const stack = new ItemStack(itemId, 1) as any;
        if (protectedArmor) {
            stack.addEnchantment({ type: "minecraft:blast_protection", level: 4 });
        }
        equipment.setEquipment(slot, stack);
    }
    // 消除游走和受伤后的逃跑加速度，保留爆炸施加的物理速度。
    victim.getComponent("minecraft:movement").setCurrentValue(0);
}

function prepareExplosion(test: Test): void {
    for (let x = 0; x <= 6; x++) {
        for (let z = 0; z <= 6; z++) {
            test.setBlockType("minecraft:obsidian", { x, y: 1, z });
        }
    }
    const creeper = test.spawn("minecraft:creeper", CREEPER_POS);
    (creeper.getComponent("minecraft:movement") as any).setCurrentValue(0);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "igniter");
    (player as any).setItem(new ItemStack("minecraft:flint_and_steel", 1) as any, 0, true);
    // 等装备属性生效后点燃；创造玩家不会引发苦力怕自主膨胀。
    test.runAtTickTime(30, () => player.interactWithEntity(creeper));
}

function horizontalSpeed(entity: any): number {
    const velocity = entity.getVelocity();
    return Math.hypot(velocity.x, velocity.z);
}

function health(entity: any): number {
    return entity.getComponent("minecraft:health").currentValue;
}

function blastProtectionReducesKnockbackVsPlain(test: Test): void {
    const cowA = test.spawn("minecraft:cow", COW_A_POS);
    const cowB = test.spawn("minecraft:cow", COW_B_POS);
    equipDiamond(cowA, true);
    equipDiamond(cowB, false);
    prepareExplosion(test);
    const healthA = health(cowA);
    const healthB = health(cowB);
    let speedA = 0;
    let speedB = 0;

    // 每 tick 采样峰值，不累计游走位移，也不在牛离开结构后丢失其身份。
    pollUntilSucceed(test, () => {
        speedA = Math.max(speedA, horizontalSpeed(cowA));
        speedB = Math.max(speedB, horizontalSpeed(cowB));
        return health(cowA) < healthA && health(cowB) < healthB &&
            health(cowA) > 0 && health(cowB) > 0 &&
            speedB > 0.2 && speedB > speedA * 1.5;
    }, {
        startTick: 31,
        interval: 1,
        maxTick: 100,
        onTimeout: () => test.assert(false,
            `explosion knockback: protected speed=${speedA.toFixed(3)}, plain speed=${speedB.toFixed(3)}, ` +
            `health=${health(cowA).toFixed(2)}/${health(cowB).toFixed(2)}; expected two surviving victims ` +
            `with plain speed>0.2 and plain speed>protected speed*1.5`),
    });
}

function plainDiamondFullExplosionKnockback(test: Test): void {
    const cow = test.spawn("minecraft:cow", COW_B_POS);
    equipDiamond(cow, false);
    prepareExplosion(test);
    const healthBefore = health(cow);
    let peakSpeed = 0;
    pollUntilSucceed(test, () => {
        peakSpeed = Math.max(peakSpeed, horizontalSpeed(cow));
        return health(cow) > 0 && health(cow) < healthBefore && peakSpeed > 0.2;
    }, {
        startTick: 31,
        interval: 1,
        maxTick: 100,
        onTimeout: () => test.assert(false,
            `plain diamond cow: peak speed=${peakSpeed.toFixed(3)}, health=${health(cow).toFixed(2)}; ` +
            `expected explosion damage, survival and horizontal speed>0.2`),
    });
}

export function registerBlastProtectionKnockbackTests(): void {
    GameTest.register("MobBehaviorTests", "blast_protection_reduces_knockback_vs_plain",
        blastProtectionReducesKnockbackVsPlain)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
    GameTest.register("MobBehaviorTests", "plain_diamond_full_explosion_knockback", plainDiamondFullExplosionKnockback)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
}
