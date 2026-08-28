// 弓箭基础伤害与力量（Power）附魔加成数值对齐测试。
//
// 验证 Cubium 弓箭伤害链路（BowItem::onPlayerStoppedUsing → createArrow → shootFrom →
// applyBowEnchantments → AbstractArrowEntity::onEntityHit）伤害数值对齐 MC Java 1.21.11。
//
// 伤害公式（对齐 vanilla AbstractArrow.onHitEntity:410-441 + power.json 附魔定义）：
//   baseDamage = 2.0（ArrowEntity 构造默认，vanilla AbstractArrow.baseDamage=2.0 行75）
//   满弓 velocity = 1.0（getArrowVelocity(chargeTicks≥20)=1.0），shootFrom 速度 = velocity*3.0 = 3.0
//   Power 附魔加成（power.json: linear base=1.0, per_level_above_first=0.5）：
//     bonus = 1.0 + 0.5*(level-1) = 0.5*level + 0.5  （level=5 → 3.0）
//   Cubium applyBowEnchantments 射出时 m_damage += level*0.5+0.5（与 vanilla modifyDamage 加成等价）
//   命中伤害 = ceil(speed * (baseDamage + powerBonus))（vanilla Mth.ceil；Cubium static_cast<i32> 截断）
//   暴击加成（满弓 velocity>=1.0 setCritical）：bonus = nextInt(damage/2+2)，damage += bonus
//
// 数值核算（满弓，speed≈3.0，近距离飞行衰减极小）：
//   无附魔：m_damage=2.0，命中 damage = 3.0*2.0 = 6，暴击 nextInt(6/2+2)=nextInt(5)=0~4 → 总 6~10
//   Power V：m_damage=2.0+3.0=5.0，命中 damage = 3.0*5.0 = 15，暴击 nextInt(15/2+2)=nextInt(9)=0~8 → 总 15~23
//
// 注：Cubium 已用 std::ceil 对齐 vanilla Mth.ceil（AbstractArrowEntity.cpp:488）。满弓 speed=3.0
//   整数倍时 ceil 与截断一致；speed 飞行衰减略 <3.0 时 ceil 比 vanilla 少算情形已消除。故无附魔
//   断言下界 5（容忍暴击 0 起步 + 飞行衰减），Power V 下界 14（基础 15 容忍衰减）。
//
// 受害者用 villager（HP 20，被动不反击，attackEntity 可解包 mob 目标）：
//   - 满弓无附魔：伤害 5~10 < 20，villager 存活，HP 20→10~15，断言 HP 下降 ∈[5,10]。
//   - 满弓 Power V：伤害 15~23，villager HP 20，可能致死（HP=0，下降20）或不致死（剩 0~5），
//     断言 HP 下降 ≥14（基础15 容忍截断到14；致死时下降20≥14 仍满足）。
//
// 防假通过设计（正反对照）：
//   - bow_no_power_full_charge_damage：无附魔伤害 5~10（证明基础伤害链路正常，非"射箭失效 0 伤害"）。
//   - bow_power_v_full_charge_damage：Power V 伤害 ≥14（证明力量加成生效，非"加成失效 仍 5~10"）。
//   两测试交叉验证：基础伤害正常 + Power 加成生效 = 弓箭伤害链路对齐。
//   若 Power 加成失效（applyBowEnchantments 未接 POWER），Power V 伤害仍 5~10 <14 → FAIL。
//   若基础伤害链路断裂（箭矢未命中/0 伤害），无附魔 HP 下降 0 ∉[5,10] → FAIL。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ killAllEntities 清场（隔离自然刷怪干扰 HP 读取）。
// 玩家 (3,2,3) 默认 yaw=0 pitch=0 朝 +Z，villager (3,2,4) 距 1 格正前方（箭矢 1 tick 命中，衰减极小）。
//
// 时序：tick 5 useItem(弓) 拉弓（setActiveHand，useDuration=72000），tick 25 stopUsingItem 释放
//   （蓄力 20 tick，chargeTicks=20，velocity=getArrowVelocity(20)=1.0 满弓，speed=3.0，setCritical）。
//   tick 26+ 箭矢飞行 1 tick 命中 villager。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_弓.txt（满弓伤害 6-10，Power V 加成）
// Ref: AbstractArrowEntity.cpp:484（speed*m_damage）+ :610（Power 加成 level*0.5+0.5）+ :489（暴击）
// Ref: power.json（linear base=1.0 per_level_above_first=0.5）
// Ref: BowItem.cpp:210 getArrowVelocity（满弓 chargeTicks≥20 → 1.0）+ :156 setCritical

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const BOW = "minecraft:bow";
const ARROW = "minecraft:arrow";
const VILLAGER_TYPE = "minecraft:villager";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const ARCHER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_POS = { x: 3, y: 2, z: 4 }; // 距玩家 1 格正前方（+Z），箭矢 1 tick 命中
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取实体当前血量（HP）。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 拉弓释放射箭通用流程：玩家主手弓 + 副手箭，tick 5 拉弓 tick 25 释放（满弓 20 tick）。
// bowStack 已由调用方决定是否附魔 Power。返回玩家与靶 villager 引用供断言。
function setupArcherAndVictim(test: Test, bowStack: any): { player: any; victim: any } {
    (test as any).killAllEntities();
    const player = test.spawnSimulatedPlayer(ARCHER_POS, "archer", 0 as any); // 0=Survival
    player.setItem(bowStack, 0, true); // 主手弓 slot 0
    const arrow = new ItemStack(ARROW, 5);
    player.setItem(arrow as unknown as Parameters<typeof player.setItem>[0], 40, false); // 副手 slot 40

    const victim = test.spawn(VILLAGER_TYPE, VICTIM_POS);

    // tick 5 useItem(弓) → setActiveHand 拉弓（useDuration=72000）。
    test.runAtTickTime(5, () => {
        (player as any).useItem(bowStack as unknown as Parameters<typeof player.useItem>[0]);
    });
    // tick 25 stopUsingItem 释放 → 满弓 20 tick（chargeTicks=72000-71980=20）→ velocity=1.0 → speed=3.0。
    test.runAtTickTime(25, () => {
        (player as any).stopUsingItem();
    });

    return { player, victim };
}

// 满弓无附魔箭矢对 villager 造成基础伤害（验证 baseDamage=2.0 + 满弓 speed=3.0 链路）。
//
// 满弓无附魔：m_damage=2.0，命中 damage = 3.0*2.0 = 6，暴击 0~4 → 总 5~10（容忍截断到 5）。
// villager HP 20 → 剩 10~15，断言 HP 下降 ∈[5,10]。
// 下界 ≥5 排除"箭矢未命中/0 伤害"假通过；上界 ≤10 排除"伤害计算偏高"（如 baseDamage 错误为更高）。
function bowNoPowerFullChargeDamageTest(test: Test): void {
    const { victim } = setupArcherAndVictim(test, makeItem(BOW));

    // 轮询断言：villager HP 从 20 下降 ∈[5,10]（满弓无附魔 6±暴击，容忍截断）。
    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        if (hp < 0) return false; // 实体/组件未就绪
        const damage = 20 - hp;
        return damage >= 5 && damage <= 10;
    }, {
        startTick: 28,
        interval: 2,
        maxTick: 80,
        onTimeout: () => {
            const hp = readHp(victim);
            const damage = hp >= 0 ? 20 - hp : -1;
            test.assert(false,
                `bow_no_power_damage: failed: villager HP=${hp} damage=${damage} `
                + `(expected damage in [5,10] = full-charge base 6 ± crit, tolerating truncation)`);
        },
    });
}

// 满弓 Power V 箭矢对 villager 造成力量加成伤害（验证 applyBowEnchantments Power 加成生效）。
//
// 满弓 Power V：m_damage=2.0+3.0=5.0，命中 damage = ceil(3.0*5.0) = 15，暴击 0~8 → 总 15~23。
// villager HP 20，伤害 15~23：可能致死（HP→0 消失）或不致死（剩 0~5，下降 15~20）。
//
// 判定（兼容致死/不致死两种结果，零 flaky）：
//   - 用 getEntities 查 villager（避免 victim 引用在死亡后失效返回 -1）。
//   - 若 villager 仍存活：读 HP，断言 20-HP ≥14（基础15 容忍截断；不致死时下降 15~20）。
//   - 若 villager 已消失（被箭矢致死，伤害≥20≥14）：直接判定通过。
//   - 若箭矢未命中/0 伤害：villager 存活 HP=20 下降 0 <14 → 不通过 → 超时 FAIL。
//   - 若 Power 加成失效（仅基础 5~10）：villager 存活下降 5~10 <14 → 不通过 → FAIL。
// 下界 ≥14 排除"Power 加成失效"；致死时 villager 消失自动通过，无 flaky。
function bowPowerVFullChargeDamageTest(test: Test): void {
    const bow = makeItem(BOW);
    bow.addEnchantment({ type: "minecraft:power", level: 5 });
    setupArcherAndVictim(test, bow);

    // 轮询断言：villager HP 下降 ≥14，或 villager 已被致死消失。
    pollUntilSucceed(test, () => {
        const villagers = test.getDimension().getEntities({
            type: VILLAGER_TYPE,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (villagers.length === 0) {
            // villager 已消失（被满弓 Power V 致死，伤害≥20≥14）→ 通过。
            return true;
        }
        const hp = readHp(villagers[0]);
        if (hp < 0) return false; // 组件未就绪
        const damage = 20 - hp;
        return damage >= 14;
    }, {
        startTick: 28,
        interval: 2,
        maxTick: 80,
        onTimeout: () => {
            const villagers = test.getDimension().getEntities({
                type: VILLAGER_TYPE,
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const hp = villagers.length > 0 ? readHp(villagers[0]) : -1;
            const damage = hp >= 0 ? 20 - hp : -1;
            test.assert(false,
                `bow_power_v_damage: failed: villagerCount=${villagers.length} HP=${hp} damage=${damage} `
                + `(expected damage >=14 or villager killed; if alive damage<14 Power bonus not applied)`);
        },
    });
}

export function registerBowArrowDamageTests(): void {
    GameTest.register("MobBehaviorTests", "bow_no_power_full_charge_damage", bowNoPowerFullChargeDamageTest)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
    GameTest.register("MobBehaviorTests", "bow_power_v_full_charge_damage", bowPowerVFullChargeDamageTest)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
}
