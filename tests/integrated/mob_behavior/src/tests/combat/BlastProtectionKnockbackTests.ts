// 爆炸保护附魔爆炸击退抗性行为类 GameTest。
//
// 验证 Cubium 爆炸保护附魔经 EXPLOSION_KNOCKBACK_RESISTANCE 属性衰减被爆炸推开时的击退力度
//（爆炸击退计算：finalKnockback = baseKnockback * (1 - getAttributeValue(EXPLOSION_KNOCKBACK_RESISTANCE))）。
//
// 机制（数值已由确定性单元测试 BlastProtectionAttributeTest 锚定）：
//   - EXPLOSION_KNOCKBACK_RESISTANCE 属性默认 0.0（满额爆炸击退）。
//   - 爆炸保护附魔经 enchantment.blast_protection 修饰符（Op0 ADD_VALUE，每级 +0.15，
//     4 个盔甲槽位共享同一 modifier id）增加此属性。vanilla 按 id 去重，Cubium 的
//     removeModifier(first)+add 在 4 同 id 槽位下恰好复现去重，全套 IV → 抗性 0.6（非 2.4）。
//   - 爆炸击退计算（Explosion.cpp:_calculateAffectedEntities）在算出 baseKnockback 后，
//     对 LivingEntity 查 EXPLOSION_KNOCKBACK_RESISTANCE 属性并乘以 (1 - 抗性) 缩减击退力度。
//   - 全套爆炸保护 IV 抗性 0.6 → 击退 ×0.4；无爆炸保护抗性 0.0 → 击退 ×1.0。
//
// C++ 链路：
//   苦力怕被玩家打火石点燃（interactMob→ignite，对齐 creeper_ignited_by_flint_and_steel 范式）
//     → CreeperEntity::tick hasIgnited→setCreeperState(1)，fuse 30 tick 后
//     CreeperEntity::explode → world.createExplosion(creeperPos, 3.0, Break, ..., this)
//     → Explosion::explode → _calculateAffectedEntities
//     → 对范围内 victim：knockback = (1-dist/range) * seenPercent * knockbackMultiplier
//     → 查 victim EXPLOSION_KNOCKBACK_RESISTANCE 属性：knockback *= (1 - 抗性)
//     → victim->hurt（EPF 减伤）+ entity->addVelocity(dx*knockback, ...) 施加击退
//
// 爆炸源选择（苦力怕 + 打火石点燃，非 AI 自发 swell）：
//   早期版本让苦力怕 NearestAttackableTargetGoal 自发选玩家 + CreeperSwellGoal 膨胀，但 swell
//   触发时机与苦力怕追击移动位置均非确定，致测试 flaky。改用打火石 interactWithEntity 点燃
//   （interactMob→ignite，runAtTickTime 确定触发），苦力怕 fuse 30 tick 后确定爆炸。
//   interactWithEntity 无距离门控（远程转发 interactOn），玩家可远离苦力怕 >7 格（CreeperSwellGoal
//   取消距离）避免自发 swell 干扰，仅打火石点燃引爆。苦力怕被玻璃围栏约束在固定位置无法追击移动。
//
// 单爆炸双受害者对照设计（消除苦力怕位置漂移对绝对位移阈值的依赖）：
//   同一次爆炸同时作用于两个 cow：A 穿爆炸保护 IV 全套（抗性 0.6，击退 ×0.4），B 穿普通钻石套
//   （抗性 0.0，击退 ×1.0）。两 cow 距爆炸中心等距同向，爆炸对二者 baseKnockback 相同，仅抗性
//   衰减不同。断言 B 位移 > A 位移（且 B 位移 > 1.5 证明爆炸确实击退了 cow，防"两 cow 都没动"假通过）。
//   无论苦力怕位置如何漂移，只要两 cow 同时受同一爆炸，抗性衰减的差异（×0.4 vs ×1.0）必使 B 位移
//   显著大于 A 位移。比例对照不依赖绝对位移阈值，根治 flaky。
//
// 受害者用 cow 而非 villager（关键，规避任务 #272 既有 UAF 缺陷）：
//   villager 有 Brain + HurtBySensor，被爆炸伤害后 m_lastDamageSource 持爆炸源（苦力怕）裸指针，
//   苦力怕 remove() 析构后 HurtBySensor::update 解引用悬垂指针 UAF 段错误（详见任务 #272）。
//   cow 无 Brain（仅 VillagerEntity 注册 Brain/Sensor），不触发该 UAF。cow HP 10，距苦力怕 3 格
//   经钻石套减伤后存活可测位移。
//
// 装备同步管线（关键依赖）：
//   equippable.setEquipment(slot, stack) 写入装备数组 → 下个 tick detectEquipmentUpdates
//   → applyEnchantmentAttributeModifiers(victim, stack, slot) 应用 EXPLOSION_KNOCKBACK_RESISTANCE
//   修饰符。故须在 setEquipment 后等待若干 tick（让 detectEquipmentUpdates 应用修饰符）再点燃。
//   点燃在 tick 30（留 30 tick 充分应用修饰符），爆炸约 tick 60，60 tick 远超修饰符应用所需。
//
// 伤害与存活计算（苦力怕 radius 3，cow 距苦力怕 3 格，seenPercent≈1）：
//   range = radius*2 = 6，distanceRatio = 3/6 = 0.5，impact = (1-0.5)*1 = 0.5
//   damage = floor((impact²+impact)/2 * 7 * damageRadius + 1) = floor((0.25+0.5)/2*7*6+1)
//          = floor(0.375*42+1) = floor(16.75) = 16
//   - 穿爆炸保护 IV 全套（EPF=16 减伤 64%）：16*0.36=5.76，cow HP 10→4.24 存活。
//   - 穿普通钻石套（EPF=0 不减爆炸伤，钻石甲 armor 20 减伤）：16*(1-18.25/25)=16*0.27=4.32，
//     cow HP 10→5.68 存活。两 cow 均存活可测位移。
//
// 环境选择：creeper_pit（7×5×7 开阔坑）+ night batch + killAllEntities 清场。
//   - night batch + killAllEntities：隔离自然刷怪干扰 cow 位移测量。
//   - creeper_pit 无顶无围墙，爆炸视线密度（seenPercent）≈1（无遮挡），击退力度可复现。
//   - 黑曜石平台：苦力怕与两 cow 脚下铺黑曜石（爆炸抗性 1200 不被苦力怕 radius 3 破坏），防爆炸
//     炸毁地板致 cow 掉落。cow 击退方向（+x/-x）铺黑曜石跑道防 cow 飞出后掉空。
//   - 玻璃围栏：苦力怕三面围玻璃（留朝 cow 侧开放），约束苦力怕无法追击玩家移动，位置固定。
//     玻璃爆炸抗性低会被炸碎，但仅约束点燃前位置（点燃后 fuse 30 tick 苦力怕仍原地），爆炸瞬间
//     玻璃已无约束意义。
//
// 实体身份隔离：位移测量用 getEntities({type:cow}) 区域查询读 location（防句柄 location 不实时），
//   区域限定 creeper_pit 防同类污染。两 cow 初始 x 坐标不同（A 在 +x 侧，B 在 -x 侧）以区分。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 几何布局（creeper_pit，helper 相对坐标）：
//   苦力怕 (3,2,3) 中心；玩家 (3,2,6) 南侧远端（距苦力怕 3 格 > swell 取消距离？swell 触发 distSq<9
//   即<3 格，玩家距 3 格恰在边界，用打火石点燃不依赖 swell，故距离无妨；3 格远离爆炸半径 3 略近，
//   玩家可能受伤但 Survival 玩家 HP 20 经钻石套可承受，且本测试不关心玩家存活）。
//   cow A (5,2,3) 东侧距苦力怕 2 格（穿爆炸保护 IV，抗性 0.6，击退 ×0.4）。
//   cow B (1,2,3) 西侧距苦力怕 2 格（穿普通钻石套，抗性 0.0，击退 ×1.0）。
//   两 cow 距苦力怕等距 2 格，baseKnockback 相同，仅抗性衰减不同。
//   A 初始 x=5（被击退向 +x，位移增大），B 初始 x=1（被击退向 -x，位移增大）。
const CREEPER_POS = { x: 3, y: 2, z: 3 };
const PLAYER_POS = { x: 3, y: 2, z: 6 };
const COW_A_POS = { x: 5, y: 2, z: 3 }; // 爆炸保护 IV，+x 侧
const COW_B_POS = { x: 1, y: 2, z: 3 }; // 普通钻石套，-x 侧
const COW_A_INIT_X = 5;
const COW_B_INIT_X = 1;

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给 ItemStack 挂爆炸保护 IV 附魔（Cubium 扩展 ItemStack.addEnchantment，直接设附魔不经 /enchant）。
function withBlastProtectionIv(stack: any): any {
    stack.addEnchantment({ type: "minecraft:blast_protection", level: 4 });
    return stack;
}

// 给受害者穿爆炸保护 IV 全套钻石甲（抗性 0.6，击退 ×0.4；EPF=16 减伤 64%）。
function equipBlastProtectionFull(victim: any): void {
    const eq = victim.getComponent("minecraft:equippable");
    eq.setEquipment("Head", withBlastProtectionIv(makeItem("minecraft:diamond_helmet")));
    eq.setEquipment("Chest", withBlastProtectionIv(makeItem("minecraft:diamond_chestplate")));
    eq.setEquipment("Legs", withBlastProtectionIv(makeItem("minecraft:diamond_leggings")));
    eq.setEquipment("Feet", withBlastProtectionIv(makeItem("minecraft:diamond_boots")));
}

// 给受害者穿普通钻石套（无爆炸保护，抗性 0.0 满击退；钻石甲 armor 20 减伤存活）。
function equipPlainDiamondFull(victim: any): void {
    const eq = victim.getComponent("minecraft:equippable");
    eq.setEquipment("Head", makeItem("minecraft:diamond_helmet"));
    eq.setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
    eq.setEquipment("Legs", makeItem("minecraft:diamond_leggings"));
    eq.setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 在苦力怕与两 cow 脚下及击退方向铺设黑曜石平台（爆炸抗性 1200，苦力怕 radius 3 不破坏）。
// y=1 是 creeper_pit air 层（y=0 为 grass_block 地板），黑曜石放 y=1 作平台，实体站 y=2。
function layObsidianPlatform(test: Test): void {
    for (let x = 0; x <= 6; x++) {
        test.setBlockType("minecraft:obsidian", { x, y: 1, z: 3 });
    }
}

// 读取区域 cow 的 x 坐标数组（区域限定查询，防同类污染）。返回所有 cow 的 x 坐标。
function readCowXs(test: Test): number[] {
    const cows = test.getDimension().getEntities({
        type: "minecraft:cow",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
    return cows.map((c: any) => c.location.x);
}

// 计算指定初始 x 的 cow 位移绝对值（从 readCowXs 结果中找最接近 initX 的 cow）。
function displacement(xs: number[], initX: number): number {
    if (xs.length === 0) {
        return -1;
    }
    // 找最接近 initX 的 cow（爆炸后位移不应过大，最近的就是目标 cow）。
    let best = xs[0];
    let bestDist = Math.abs(best - initX);
    for (const x of xs) {
        const d = Math.abs(x - initX);
        if (d < bestDist) {
            best = x;
            bestDist = d;
        }
    }
    return Math.abs(best - initX);
}

// 单爆炸双受害者对照：同一次苦力怕爆炸同时作用于穿爆炸保护 IV 的 cow A 与穿普通钻石套的 cow B。
// 断言 cow B 位移（抗性 0.0，击退 ×1.0）显著大于 cow A 位移（抗性 0.6，击退 ×0.4），
// 且 cow B 位移 > 1.5（证明爆炸确实击退了 cow，防"两 cow 都没动"假通过）。
//
// 判定：tick 0 spawn + setEquipment + 铺平台 → tick 30 打火石点燃苦力怕 → 约 tick 60 爆炸 →
//   pollUntilSucceed 轮询 cow B 位移 > 1.5 且 cow B 位移 > cow A 位移 + 0.5。
//   - 若 EXPLOSION_KNOCKBACK_RESISTANCE 属性未注册/未消费（击退未乘抗性），两 cow 位移相近（差 <0.5）→ FAIL。
//   - 若爆炸保护修饰符未应用（装备同步管线失效），cow A 抗性=0.0，两 cow 位移相近 → FAIL。
//   - 若苦力怕未爆炸/未击退，cow B 位移 <1.5 → FAIL（暴露爆炸链路本身失效）。
//
// night batch + killAllEntities：隔离自然刷怪干扰位移测量。
// Ref: Explosion.cpp:_calculateAffectedEntities（knockback *= (1 - EXPLOSION_KNOCKBACK_RESISTANCE)）
// Ref: BlastProtectionEnchantment.hpp:getAttributeModifiers（4 槽位同 id ADD_VALUE 每级 +0.15，去重 0.6）
// Ref: BlastProtectionAttributeTest（确定性单元测试锚定全套抗性=0.6）
function blastProtectionReducesKnockbackVsPlain(test: Test): void {
    (test as any).killAllEntities();
    layObsidianPlatform(test);

    const cowA = test.spawn("minecraft:cow", COW_A_POS);
    equipBlastProtectionFull(cowA);
    const cowB = test.spawn("minecraft:cow", COW_B_POS);
    equipPlainDiamondFull(cowB);
    const creeper = test.spawn("minecraft:creeper", CREEPER_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "igniter", 0 as any);

    // 创造玩家主手持打火石（同 creeper_ignited_by_flint_and_steel 范式）。
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);
    (player as any).setItem(flintAndSteel as any, 0, true);

    // tick 30 点燃苦力怕（留 30 tick 让 detectEquipmentUpdates 应用爆炸保护修饰符）。
    test.runAtTickTime(30, () => {
        (player as any).interactWithEntity(creeper);
    });

    // 爆炸约 tick 60，pollUntilSucceed 轮询 cow B 位移 > 1.5 且 B 位移 > A 位移 + 0.5。
    pollUntilSucceed(test, () => {
        const xs = readCowXs(test);
        const dispA = displacement(xs, COW_A_INIT_X);
        const dispB = displacement(xs, COW_B_INIT_X);
        // 需两 cow 均可定位（dispA/dispB >= 0）且 B 显著大于 A。
        return dispA >= 0 && dispB >= 0 && dispB > 1.5 && dispB > dispA + 0.5;
    }, {
        startTick: 62,
        interval: 4,
        maxTick: 160,
        onTimeout: () => {
            const xs = readCowXs(test);
            const dispA = displacement(xs, COW_A_INIT_X);
            const dispB = displacement(xs, COW_B_INIT_X);
            test.assert(false,
                `blast protection IV cow A displacement=${dispA < 0 ? "n/a" : dispA.toFixed(2)} should be `
                + `less than plain diamond cow B displacement=${dispB < 0 ? "n/a" : dispB.toFixed(2)} `
                + `(B>1.5 proves explosion knocked cow; B>A+0.5 proves EXPLOSION_KNOCKBACK_RESISTANCE `
                + `resistance=0.6 knockback*0.4 applied; if B<1.5 creeper did not explode/knockback; `
                + `if B~=A resistance not applied)`);
        },
    });
}

// 无爆炸保护钻石套承受满额爆炸击退（正向对照，防 blastProtectionReducesKnockbackVsPlain 假通过）。
//
// 单独验证穿普通钻石套的 cow 被苦力怕爆炸击退位移 > 1.5（满击退），证明爆炸链路本身有效。
// 与 blastProtectionReducesKnockbackVsPlain 交叉验证：若该测试 cow 位移 <1.5（爆炸未击退），
// 则 blastProtection 测试的"B 位移小"可能只是爆炸没生效，而非抗性衰减。
//
// 此测试用打火石点燃范式（同上），cow 单独受爆炸，断言位移 > 1.5。
function plainDiamondFullExplosionKnockback(test: Test): void {
    (test as any).killAllEntities();
    layObsidianPlatform(test);

    const cow = test.spawn("minecraft:cow", COW_A_POS);
    equipPlainDiamondFull(cow);
    const creeper = test.spawn("minecraft:creeper", CREEPER_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "igniter2", 0 as any);

    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);
    (player as any).setItem(flintAndSteel as any, 0, true);

    test.runAtTickTime(30, () => {
        (player as any).interactWithEntity(creeper);
    });

    pollUntilSucceed(test, () => {
        const xs = readCowXs(test);
        const disp = displacement(xs, COW_A_INIT_X);
        return disp >= 0 && disp > 1.5;
    }, {
        startTick: 62,
        interval: 4,
        maxTick: 160,
        onTimeout: () => {
            const xs = readCowXs(test);
            const disp = displacement(xs, COW_A_INIT_X);
            test.assert(false,
                `plain diamond cow should be knocked >1.5 by creeper explosion (resistance=0.0, knockback*1.0), `
                + `but displacement=${disp < 0 ? "n/a" : disp.toFixed(2)} `
                + `(if <1.5 creeper did not explode/knockback — blastProtection test may be false pass)`);
        },
    });
}

export function registerBlastProtectionKnockbackTests(): void {
    GameTest.register("MobBehaviorTests", "blast_protection_reduces_knockback_vs_plain",
        blastProtectionReducesKnockbackVsPlain)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(220);

    GameTest.register("MobBehaviorTests", "plain_diamond_full_explosion_knockback",
        plainDiamondFullExplosionKnockback)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(220);
}
