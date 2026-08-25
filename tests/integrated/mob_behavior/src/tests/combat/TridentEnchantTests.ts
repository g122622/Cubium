// 三叉戟基础伤害与附魔（穿刺/引雷）对齐测试（验证 TridentEntity::onEntityHit 伤害与引雷链路）。
//
// 验证 Cubium 三叉戟伤害链路（TridentItem::onPlayerStoppedUsing → 创建 TridentEntity → setItemStack 复制附魔 →
// shootFrom 用玩家朝向发射 → TridentEntity::onEntityHit）伤害数值与引雷召唤对齐 MC Java 1.21.11。
//
// 三叉戟伤害公式（对齐 vanilla ThrownTrident.onHitEntity，TridentEntity.cpp:243-286）：
//   baseDamage = 8.0（vanilla AbstractArrow baseDamage 默认 2.0，但 ThrownTrident 覆盖为 8.0，硬编码不随机）
//   穿刺附魔加成（ImpalingEnchantment::getDamageBonus）：目标在 SENSITIVE_TO_IMPALING(=AQUATIC) 标签时
//     每级 +2.5（对齐 vanilla Enchantments.java:989-996）。damage += getTotalDamageBonus(stack, target)。
//   引雷附魔（onEntityHit:299-340）：hasChanneling(stack) && isThundering && canSeeSky(命中点) →
//     召唤 LightningBoltEntity 于目标位置 + 播放 ITEM_TRIDENT_THUNDER 音效。
//
// 投掷链路（TridentItem.cpp:98-211 onPlayerStoppedUsing）：
//   - chargeTicks ≥ MIN_CHARGE_TICKS(10) 才投掷；chargeTicks = useDuration(72000) - timeLeft。
//   - 无激流（非潮湿）走正常投掷分支：创建 TridentEntity，setItemStack(stack) 复制手持三叉戟附魔到实体，
//     setLoyaltyLevel 查 LOYALTY，shootFrom(player, pitch, yaw, 0, 2.5, 1.0) 用玩家朝向发射。
//   - Survival 模式 stack.shrink(1) 消耗；Creative 跳过消耗（本测试用 Creative 玩家，三叉戟不消耗可重复验证）。
//
// 附魔施加：脚本 ItemStack.addEnchantment({type, level})（Cubium 扩展，同 BowArrowDamageTests Power V 范式）。
//   - 穿刺：addEnchantment({type:"minecraft:impaling", level:5})，每级 +2.5，V 级 +12.5。
//   - 引雷：addEnchantment({type:"minecraft:channeling", level:1})，仅 I 级。
//   setItemStack 把带附魔的 stack 复制到 TridentEntity，onEntityHit 经 trident->m_tridentStack 查附魔生效。
//
// 命中精度（玩家朝向 + 近距离水平飞行）：
//   - shootFrom（ProjectileEntity.cpp:187-213）用玩家 pitch/yaw 算方向向量，yaw=0→+Z pitch=0→水平。
//   - lookAtEntity(victim)（SimulatedPlayer.cpp:141-156）setRotation 精确对准靶眼部，三叉戟水平飞向靶。
//   - 靶距 1 格（z=4 距玩家 z=3），速度 2.5/tick，1 tick 命中，靶无移动窗口（近距消除 AI 游荡干扰）。
//   - inaccuracy=1.0 高斯散布 0.0075rad≈0.43°，1 格偏移≈0.0075 格可忽略。
//   - 三叉戟重力 0.05/tick²，1 tick 下落 <0.05 格，仍在靶碰撞盒内。
//   靶 villager/squid/cow 碰撞盒高 ≥0.9，玩家眼部高度落在靶碰撞盒垂直范围内，水平射线可靠命中。
//
// 防假通过设计（正反对照）：
//   - trident_base_damage：无附魔三叉戟命中 villager，HP 20 下降 ∈[7,9]（8.0 基础，硬编码无随机扰动）。
//     下界 ≥7 排除"未命中/0 伤害"；上界 ≤9 排除"伤害偏高（如基础伤害错误）"。
//   - trident_impaling_aquatic_bonus：穿刺 V 三叉戟命中 squid（水生，HP 10）→ 8+12.5=20.5 ≥10 致死（squid 消失）。
//   - trident_impaling_non_aquatic_no_bonus：穿刺 V 三叉戟命中 cow（非水生，HP 10）→ 仅 8 伤害，cow 存活（剩 2 HP）。
//     两测试交叉验证：穿刺对水生致死 + 对非水生不致死 = SENSITIVE_TO_IMPALING 标签门控正确。
//     若穿刺标签门控失效（误对 cow 加成）：cow 也被致死 → trident_impaling_non_aquatic_no_bonus FAIL。
//     若穿刺加成失效（对 squid 仍 8 伤害）：squid 存活 HP 2 → trident_impaling_aquatic_bonus FAIL。
//   - trident_channeling_summons_lightning：引雷三叉戟 + 雷暴 + 露天命中 villager → lightning_bolt 实体生成。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ skyAccess(true) 制造露天（引雷测试需 canSeeSky=true）。
//   killAllEntities 清场隔离自然刷怪干扰。Creative 玩家（默认 gameMode）permLevel=2 可执行 /weather。
//
// 时序（近距 1 格命中）：
//   - tick 0：spawn 玩家 + 靶 + lookAtEntity 朝向 +（引雷测试）/weather thunder 100000。
//   - tick 5：useItem(三叉戟) 拉弓（setActiveHand，useDuration=72000）。
//   - tick 19：释放前重新 lookAtEntity 校准朝向（补偿靶 AI 游荡移动）。
//   - tick 20：stopUsingItem 释放（蓄力 15 tick ≥ MIN_CHARGE_TICKS=10，投掷）。
//   - tick 21+：三叉戟飞行 1 tick 命中靶。
//
// 雷暴强度渐变（vanilla 真实行为，非 Cubium 缺陷）：
//   Level.isThundering() = getThunderLevel(1.0) > 0.9（强度阈值，对齐 vanilla Level.java:906），
//   与 Cubium WeatherState::isThundering()（thunderStrength > 0.9）一致。
//   /weather thunder 只设 thundering=true 不设强度（对齐 vanilla setWeatherParameters），
//   强度靠 WeatherManager::tick 每 tick 渐变 +0.01，故设雷暴后约 91 tick 才 isThundering()=true。
//   引雷测试须 tick 0 设雷暴后延迟到 tick 110+ 投掷（thunderStrength 达 0.9），否则 isThundering()=false
//   channeling 正确不触发闪电（此延迟是 vanilla 行为，测试须遵守）。
//
// 雷暴天气隔离（世界级单例，跨测试持久污染）：
//   引雷测试独占 batch("thunder") 串行 + runOnFinish 执行 /weather clear 恢复晴天，防污染同批/后续测试。
//   （对齐 EntityCommandTests summonBlockedOnPeacefulForMonster 独占 batch + runOnFinish 恢复 difficulty 范式。）
//
// className 恒为 MobBehaviorTests。
// Ref: TridentEntity.cpp:243-286（onEntityHit：baseDamage 8.0 + 穿刺 + 引雷召唤闪电）
// Ref: TridentEntity.cpp:299-340（channeling 分支：hasChanneling + isThundering + canSeeSky → spawnEntity lightning）
// Ref: TridentItem.cpp:98-211（onPlayerStoppedUsing：setItemStack 复制附魔 + shootFrom 投掷）
// Ref: ImpalingEnchantment.cpp:32-52（getDamageBonus：SENSITIVE_TO_IMPALING 标签门控，每级 +2.5）
// Ref: EnchantmentHelper.cpp:271-286（getTotalDamageBonus：遍历 stack 附魔累加 getDamageBonus）
// Ref: WeatherState.hpp:181（isThundering：thunderStrength > 0.9 强度阈值，对齐 vanilla Level.java:906）
// Ref: Level.java:906（vanilla isThundering = getThunderLevel(1.0) > 0.9）
// Ref: ServerLevel.java:705-770（vanilla advanceWeatherCycle：强度每 tick ±0.01 渐变）
// Ref: tech_穿刺.txt（JE 穿刺仅对水生生物有效，每级 +2.5）
// Ref: tech_引雷.txt（雷暴 + 露天 + 命中生物 → 召唤闪电；晴朗不召）
// Ref: BowArrowDamageTests.ts（addEnchantment + useItem/stopUsingItem + readHp 范式）
// Ref: EntityCommandTests.js（独占 batch + runOnFinish 恢复世界级状态范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { asDim } from "../../utils/script/cubiumExtensions.js";

const TRIDENT = "minecraft:trident";
const VILLAGER_TYPE = "minecraft:villager";
const SQUID_TYPE = "minecraft:squid";
const COW_TYPE = "minecraft:cow";
const LIGHTNING_TYPE = "minecraft:lightning_bolt";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 玩家 (3,2,3)，靶放 (3,2,4) 距玩家 1 格正前方（+Z），三叉戟 1 tick 水平命中（同 BowArrowDamageTests 近距范式）。
const THROWER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_POS = { x: 3, y: 2, z: 4 };
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取实体当前血量（HP）。-1 表示实体/组件未就绪（死亡后组件消失）。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造三叉戟 ItemStack（@minecraft/server 与 server-gametest 依赖的 @minecraft/server 是两个独立包实例，
// ItemStack 类型不兼容，用 as any 绕过，同 BowArrowDamageTests 范式）。
// count 为堆叠数量（默认 1，忠诚测试用 2 验证消耗+拾回）。
function makeTrident(count: number = 1): any {
    return new ItemStack(TRIDENT, count);
}

// 投掷三叉戟通用流程：Creative 玩家主手三叉戟，lookAtEntity 朝向靶，tick 5 拉弓 tick 20 释放（蓄力 15 tick）。
// tridentStack 已由调用方决定附魔。返回靶实体引用供断言。
//   - Creative 玩家：三叉戟不消耗（onPlayerStoppedUsing 跳过 shrink），permLevel=2 可执行 /weather。
//   - lookAtEntity(victim)：精确朝向靶眼部，shootFrom 用此 yaw/pitch 发射，三叉戟水平飞向靶。
//   - tick 19 释放前重新 lookAtEntity 校准朝向（补偿靶 AI 游荡移动：cow/villager 在 tick 0-20 间可能位移，
//     重新对准确保三叉戟飞向靶当前位置而非 spawn 位置）。
function setupThrowerAndVictim(test: Test, tridentStack: any, victimType: string): { player: any; victim: any } {
    (test as any).killAllEntities();
    const player = test.spawnSimulatedPlayer(THROWER_POS, "thrower"); // 默认 Creative
    player.setItem(tridentStack, 0, true); // 主手三叉戟 slot 0

    const victim = test.spawn(victimType, VICTIM_POS);
    // 玩家朝向靶实体（精确对准眼部，shootFrom 用此朝向发射三叉戟飞向靶）。
    (player as any).lookAtEntity(victim);

    // tick 5 useItem(三叉戟) → TridentItem::onItemRightClick → setActiveHand 拉弓（useDuration=72000）。
    test.runAtTickTime(5, () => {
        (player as any).useItem(tridentStack as unknown as Parameters<typeof player.useItem>[0]);
    });
    // tick 19 释放前重新 lookAtEntity 校准朝向（补偿靶 AI 游荡移动）。
    test.runAtTickTime(19, () => {
        (player as any).lookAtEntity(victim);
    });
    // tick 20 stopUsingItem 释放 → 蓄力 15 tick（chargeTicks=72000-71985=15 ≥ MIN_CHARGE_TICKS=10）→ 投掷。
    test.runAtTickTime(20, () => {
        (player as any).stopUsingItem();
    });

    return { player, victim };
}

// 无附魔三叉戟命中 villager 造成 8.0 基础伤害（验证 TridentEntity baseDamage=8.0 链路）。
//
// 无附魔三叉戟：onEntityHit damage=8.0（硬编码，无随机扰动，不经 setBaseDamageFromMob）。
// villager HP 20 → 剩 12，断言 HP 下降 ∈[7,9]（容忍少量舍入/无敌帧）。
//   - 下界 ≥7 排除"三叉戟未命中/0 伤害"假通过。
//   - 上界 ≤9 排除"伤害偏高（如 baseDamage 错误为更高或穿刺误加成）"。
//   注：villager 无护甲，8.0 伤害全额作用（hurt→actuallyHurt 盔甲 0 减免）。
function tridentBaseDamageTest(test: Test): void {
    const { victim } = setupThrowerAndVictim(test, makeTrident(), VILLAGER_TYPE);

    // 轮询断言：villager HP 从 20 下降 ∈[7,9]（无附魔 8.0 基础伤害）。
    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        if (hp < 0) {
            return false;
        } // 实体/组件未就绪
        const damage = 20 - hp;
        return damage >= 7 && damage <= 9;
    }, {
        startTick: 23,
        interval: 2,
        maxTick: 80,
        onTimeout: () => {
            const hp = readHp(victim);
            const damage = hp >= 0 ? 20 - hp : -1;
            test.assert(false,
                `trident_base_damage: failed: villager HP=${hp} damage=${damage} `
                + `(expected damage in [7,9] = base 8.0; if damage=0 trident missed or 0 damage; `
                + `if damage>9 baseDamage wrong or impaling mis-applied)`);
        },
    });
}

// 穿刺 V 三叉戟命中 squid（水生，HP 10）一击致死（验证 ImpalingEnchantment SENSITIVE_TO_IMPALING 标签门控 + 每级 +2.5）。
//
// 穿刺 V 三叉戟：onEntityHit damage = 8.0 + getTotalDamageBonus(stack, squid)。
//   squid 在 SENSITIVE_TO_IMPALING(=AQUATIC) 标签（EntityTypeTags.cpp:519-533 含 minecraft:squid），
//   ImpalingEnchantment::getDamageBonus(5, squid) = 5*2.5 = 12.5。
//   总伤害 8.0 + 12.5 = 20.5 ≥ squid HP 10 → 一击致死（squid 消失）。
//
// 判定（兼容 squid 陆地窒息/挣扎位移，用 getEntities 查 squid 存活）：
//   - squid 被穿刺 V 三叉戟命中 → 伤害 20.5 ≥ 10 致死 → squid 消失（getEntities 返 0）→ 通过。
//   - 若穿刺加成失效（对 squid 仍仅 8 伤害）：squid 存活 HP 2 → getEntities 返 1 → 超时 FAIL。
//   - 若三叉戟未命中 squid：squid 存活 HP 10 → 超时 FAIL。
//   注：squid 是水生生物陆地短时存活（测试 <100 tick），无 randomTick 窒息致死（窒息需长时间），
//   故 squid 存活时 HP 稳定 10，仅被三叉戟伤害改变。
function tridentImpalingAquaticBonusTest(test: Test): void {
    const trident = makeTrident();
    trident.addEnchantment({ type: "minecraft:impaling", level: 5 });
    setupThrowerAndVictim(test, trident, SQUID_TYPE);

    // 轮询断言：squid 被穿刺 V 三叉戟一击致死消失（getEntities 返 0）。
    pollUntilSucceed(test, () => {
        const squids = test.getDimension().getEntities({
            type: SQUID_TYPE,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return squids.length === 0;
    }, {
        startTick: 23,
        interval: 2,
        maxTick: 80,
        onTimeout: () => {
            const squids = test.getDimension().getEntities({
                type: SQUID_TYPE,
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const hp = squids.length > 0 ? readHp(squids[0]) : -1;
            test.assert(false,
                `trident_impaling_aquatic_bonus: failed: squidCount=${squids.length} HP=${hp} `
                + `(expected squid killed = 0 count; impaling V should deal 8+12.5=20.5 >= squid HP 10; `
                + `if squid alive HP=2 impaling bonus not applied [SENSITIVE_TO_IMPALING tag query broken]; `
                + `if squid alive HP=10 trident missed)`);
        },
    });
}

// 穿刺 V 三叉戟命中 cow（非水生，HP 10）仅 8 伤害存活（负向对照，验证穿刺不对非水生加成）。
//
// 穿刺 V 三叉戟：onEntityHit damage = 8.0 + getTotalDamageBonus(stack, cow)。
//   cow 不在 SENSITIVE_TO_IMPALING 标签，ImpalingEnchantment::getDamageBonus(5, cow) = 0。
//   总伤害 8.0 + 0 = 8.0 < cow HP 10 → cow 存活（剩 2 HP）。
//
// 判定：cow 存活且 HP 下降 ∈[7,9]（8.0 伤害，无穿刺加成）。
//   - 若穿刺标签门控失效（误对 cow 加成 12.5）：伤害 20.5 ≥ 10 致死 → cow 消失 → getEntities 返 0 →
//     "存活"断言失败 → 超时 FAIL（此为穿刺对非水生误加成的明确捕获）。
//   - 若三叉戟未命中 cow：cow 存活 HP 10 下降 0 ∉[7,9] → 超时 FAIL。
//   此测试与 trident_impaling_aquatic_bonus 交叉验证：水生致死 + 非水生存活 = 标签门控正确。
function tridentImpalingNonAquaticNoBonusTest(test: Test): void {
    const trident = makeTrident();
    trident.addEnchantment({ type: "minecraft:impaling", level: 5 });
    setupThrowerAndVictim(test, trident, COW_TYPE);

    // 轮询断言：cow 存活且 HP 从 10 下降 ∈[7,9]（穿刺 V 对非水生仅 8.0 基础，无加成）。
    pollUntilSucceed(test, () => {
        const cows = test.getDimension().getEntities({
            type: COW_TYPE,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (cows.length === 0) {
            return false; // cow 不应消失（8 伤害 < HP 10）；若消失说明穿刺误加成，下方超时 FAIL
        }
        const hp = readHp(cows[0]);
        if (hp < 0) {
            return false;
        }
        const damage = 10 - hp;
        return damage >= 7 && damage <= 9;
    }, {
        startTick: 23,
        interval: 2,
        maxTick: 80,
        onTimeout: () => {
            const cows = test.getDimension().getEntities({
                type: COW_TYPE,
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const hp = cows.length > 0 ? readHp(cows[0]) : -1;
            const damage = hp >= 0 ? 10 - hp : -1;
            test.assert(false,
                `trident_impaling_non_aquatic_no_bonus: failed: cowCount=${cows.length} HP=${hp} damage=${damage} `
                + `(expected cow alive HP in [1,3] = 8.0 base only, no impaling bonus; `
                + `if cowCount=0 cow killed = impaling mis-applied to non-aquatic [SENSITIVE_TO_IMPALING tag gating broken]; `
                + `if damage=0 trident missed)`);
        },
    });
}

// 引雷三叉戟 + 雷暴天气 + 露天命中 villager → 召唤 lightning_bolt（验证 TridentEntity channeling 分支）。
//
// 引雷三叉戟：onEntityHit channeling 分支（TridentEntity.cpp:299-340）：
//   hasChanneling(stack) && isThundering && canSeeSky(命中点) → spawnEntity(LightningBoltEntity) 于目标位置。
//
// 雷暴强度渐变（vanilla 真实行为）：/weather thunder 100000 只设 thundering=true 不设 thunderStrength（对齐
//   vanilla setWeatherParameters），强度靠 tick 渐变 +0.01/tick。Level.isThundering()=thunderStrength>0.9，
//   故设雷暴后约 91 tick 才 isThundering()=true。测试须 tick 0 设雷暴后延迟到 tick 110 投掷（强度达 0.9+），
//   否则 isThundering()=false channeling 正确不触发（此延迟非缺陷，是 vanilla 行为，测试须遵守）。
//
// 判定：命中 villager 后区域内出现 minecraft:lightning_bolt 实体（引雷召唤闪电）。
//   - 若引雷分支缺失（hasChanneling/isThundering/canSeeSky 任一门控失效或未 spawnEntity）：
//     lightning_bolt 不生成 → getEntities 返 0 → 超时 FAIL。
//   - 若三叉戟未命中 villager：villager 未受击，不触发 onEntityHit channeling 分支 → 无闪电 → FAIL。
//   - 若雷暴强度未达标（投掷过早）：isThundering()=false channeling 正确跳过 → 无闪电 → FAIL（须延迟投掷）。
//
// 雷暴天气隔离（世界级单例）：独占 batch("thunder") 串行 + runOnFinish 恢复 /weather clear，
//   防雷暴污染同批/后续依赖晴天的测试（对齐 EntityCommandTests 独占 batch + runOnFinish 恢复 difficulty 范式）。
function tridentChannelingSummonsLightningTest(test: Test): void {
    (test as any).killAllEntities();
    const player = test.spawnSimulatedPlayer(THROWER_POS, "thrower"); // Creative，permLevel=2 可执行 /weather

    // 引雷三叉戟（仅 I 级）。
    const trident = makeTrident();
    trident.addEnchantment({ type: "minecraft:channeling", level: 1 });
    player.setItem(trident, 0, true);

    const victim = test.spawn(VILLAGER_TYPE, VICTIM_POS);
    (player as any).lookAtEntity(victim);

    // tick 0 设雷暴天气（世界级，独占 batch 串行 + runOnFinish 恢复晴天）。
    // 强度从 0 渐变到 >0.9 约需 91 tick，故延迟到 tick 110 投掷确保 isThundering()=true。
    player.chat("/weather thunder 100000");
    test.runOnFinish(() => {
        player.chat("/weather clear");
    });

    // tick 105 useItem(三叉戟) 拉弓，tick 120 释放（蓄力 15 tick 投掷）。此时雷暴强度已达 0.9+，isThundering()=true。
    test.runAtTickTime(105, () => {
        (player as any).useItem(trident as unknown as Parameters<typeof player.useItem>[0]);
    });
    // tick 119 释放前重新 lookAtEntity 校准（补偿 villager 雷暴/下雨躲避 AI 移动）。
    test.runAtTickTime(119, () => {
        (player as any).lookAtEntity(victim);
    });
    test.runAtTickTime(120, () => {
        (player as any).stopUsingItem();
    });

    // 轮询断言：区域内出现 lightning_bolt 实体（引雷三叉戟命中 villager 召唤闪电）。
    pollUntilSucceed(test, () => {
        const lightnings = test.getDimension().getEntities({
            type: LIGHTNING_TYPE,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return lightnings.length >= 1;
    }, {
        startTick: 123,
        interval: 2,
        maxTick: 180,
        onTimeout: () => {
            const lightnings = test.getDimension().getEntities({
                type: LIGHTNING_TYPE,
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const villagers = test.getDimension().getEntities({
                type: VILLAGER_TYPE,
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const dim = asDim(test.getDimension());
            const vEnt = villagers.length > 0 ? (villagers[0] as any) : null;
            const vLoc = vEnt?.location;
            const vInfo = vEnt
                ? `villager pos=(${vLoc?.x},${vLoc?.y},${vLoc?.z}) HP=${readHp(vEnt)}`
                : `villager count=${villagers.length}`;
            test.assert(false,
                `trident_channeling_summons_lightning: failed: lightningCount=${lightnings.length} `
                + `isThundering=${dim.isThundering()} ${vInfo} `
                + `(expected >=1 lightning; if 0 channeling branch missing [hasChanneling/isThundering/canSeeSky gate] `
                + `or trident missed villager [villager HP should drop from 20 if hit] `
                + `or thunderStrength not yet >0.9 [vanilla strength ramp, needs ~91 ticks after /weather thunder])`);
        },
    });
}

// 三叉戟忠诚附魔回返集成测试（验证 TridentEntity tick→_tickReturning→onPlayerPickup 回返拾取链路）。
//
// 验证 Cubium 忠诚附魔回返链路对齐 MC Java 1.21.11 ThrownTrident：
//   投掷 → 三叉戟命中实体 onEntityHit setDealtDamage(true) → tick 检测 hasDealtDamage && loyalty>0 &&
//   _shouldReturnToThrower → setNoClip(true) + _tickReturning（穿墙指数逼近玩家）→
//   distance<2 时 onPlayerPickup → inventory.add(tridentStack) + remove()。
//
// 链路实现（TridentEntity.cpp）：
//   - tick（:108-139）：(hasDealtDamage||isInGround) && loyalty>0 && shooter 存活 → _tickReturning。
//   - _tickReturning（:155-240）：速度=currentVel*0.95+dir*(0.05*loyalty)/tick 指数逼近玩家眼部，
//     distance<2 且玩家射手 → onPlayerPickup。setNoClip 穿墙返回不走碰撞。
//   - onPlayerPickup（:410-457）：noClip && shooter.uuid==player.uuid 授权拾取 → inventory.add + remove。
//   - onEntityHit（:280）：命中实体 setDealtDamage(true)（触发返回门控，无需插地）。
//   - setItemStack（:394-408）：投掷时从手持 stack 提取 loyalty 等级写入 TridentStateComponent。
//
// 触发路径选择（命中实体而非命中方块）：
//   朝近距 villager（z=4 距玩家 1 格）投掷，三叉戟 1 tick 命中实体 onEntityHit setDealtDamage(true)，
//   速度反弹停在 villager 附近，下一 tick tick 检测 hasDealtDamage 触发 _tickReturning 飞回玩家。
//   命中方块路径（朝地板投掷）三叉戟向下飞易穿过 grass_block 进入地板下方虚空飞出查询区域（creeper_pit
//   是开放坑，地板下方无阻挡），不可靠；命中实体路径三叉戟停在 villager 附近（z=4），稳定在查询区内。
//
// 数量选择：count=1（对齐 vanilla 真实玩法，单把三叉戟投掷→消耗→空槽→拾取回主手槽）。
//   任务 #335 调研发现真实对齐缺陷：TridentItem.cpp:190 此前直接 setItemStack(stack) 用原始 stack，
//   未对齐 vanilla TridentItem.java:141 `copyWithCount(1)`（投掷的三叉戟实体只代表 1 把，与手持数量解耦）。
//   三叉戟 maxStack=1，若用 count=2 投掷，实体存 count=2，忠诚回返拾取 onPlayerPickup 把 count=2 整堆
//   塞回背包（PlayerInventory::add 不拆分不合并 maxStack=1 物品），主手槽已有 count=1 不接收→加别处，
//   主手停留 1 不恢复 2 → 测试假失败。修复 TridentItem 对齐 copyWithCount(1) 后，count=1 范式下：
//   投掷消耗 1（主手 1→0 空槽），回返拾取 add(count=1) 三叉戟进空主手槽（主手 0→1 恢复）。
//   count=1 是 vanilla 玩家持单把忠诚三叉戟的真实玩法，断言干净无 maxStack 干扰。
//
// 防假通过设计（正反对照）：
//   - trident_loyalty_returns_to_thrower：Survival 玩家持忠诚 III 三叉戟×1 朝近距 villager 投掷 →
//     命中实体 → 飞回玩家 → 拾取入主手槽。双重断言：trident 实体消失 + 主手数量 1→0（消耗）→1（拾回恢复）。
//     - 若回返链路断裂（_tickReturning 未触发/onPlayerPickup 未拾取）：三叉戟停 villager 附近不消失，主手停留 0 → 超时 FAIL。
//     - 若拾取但未入背包（inventory.add 失败）：主手停留 0 → 超时 FAIL。
//     - 若三叉戟未命中 villager：不 setDealtDamage 不触发返回，实体停留 → 超时 FAIL。
//     - 若 TridentItem 未对齐 copyWithCount(1)（实体存 count>1）：拾取 add 落别槽，主手停留 0 → 超时 FAIL。
//   - trident_no_loyalty_does_not_return：负向对照，无忠诚三叉戟命中 villager 后不返回（实体持续存在，主手不恢复）。
//     - 若门控 loyalty>0 失效（无忠诚也返回）：实体消失主手恢复 → 与"不返回"断言冲突 → FAIL。
//     两测试交叉验证：忠诚返回 + 无忠诚不返回 = loyalty 门控正确。
//
// 模式选择：Survival（spawnSimulatedPlayer 第三参数 0）。Creative 跳过 shrink（投掷不消耗），无"消耗→恢复"
//   干净断言；Survival 投掷消耗 1（1→0），返回拾取恢复（0→1），数量变化是回返链路的明确证据。
//   Survival 下 pickupStatus=Allowed（TridentEntity 构造默认），onPlayerPickup 门控正常。
//
// 靶选择：villager（HP 20，受 8 伤害存活 HP 12，被动不反击）。villager 受伤后 AI 可能移动，但三叉戟返回
//   目标点是玩家位置（非 villager），villager 移动不影响返回路径。三叉戟命中后停在 villager 命中点附近。
//
// 玩家静止：返回期间不调任何 moveTo* 方法（SimulatedPlayer 无自主 AI），玩家位置是稳定返回目标点。
//   任务 #314 已修复服务端物理空转，玩家站立稳定。
//
// 返回时序：loyalty III speed=0.15/tick 指数衰减逼近，近距离 1 格返回约 10-40 tick。
//   pollUntilSucceed startTick=25（留命中+启动返回余量）maxTick=130 留足返回+拾取时间。
//
// className 恒为 MobBehaviorTests。
// Ref: TridentEntity.cpp:108-139（tick：hasDealtDamage+loyalty 门控触发 _tickReturning）
// Ref: TridentEntity.cpp:155-240（_tickReturning：指数逼近玩家 + distance<2 onPlayerPickup）
// Ref: TridentEntity.cpp:410-457（onPlayerPickup：noClip+uuid 授权 + inventory.add + remove）
// Ref: TridentEntity.cpp:280（onEntityHit：setDealtDamage 触发返回门控）
// Ref: TridentEntity.cpp:394-408（setItemStack：提取 loyalty 等级）
// Ref: TridentItem.cpp:190（setItemStack(thrownStack) 对齐 vanilla copyWithCount(1)，任务 #335 修复）
// Ref: TridentItem.java:141（vanilla copyWithCount(1)：投掷实体只代表 1 把三叉戟）
// Ref: TridentConsumptionTests.ts（Survival setItem + getMainHandAmount 范式）
// Ref: tech_三叉戟.txt（忠诚附魔：三叉戟命中后飞回投掷者）

// 读取玩家主手槽（slot 0）物品数量。0 表示空槽（同 TridentConsumptionTests 范式）。
function getMainHandAmount(player: any): number {
    const inv = player.getComponent("minecraft:inventory") as any;
    const mainHand = inv?.container?.getItem?.(0) as any;
    return mainHand?.amount ?? 0;
}

// Survival 玩家朝近距 villager 投掷忠诚/无忠诚三叉戟的通用流程。tridentStack 已由调用方决定附魔（count=1）。
//   - Survival 模式：投掷消耗 1（数量 1→0 空槽），忠诚返回拾取恢复（数量 0→1 回主手槽），数量变化是回返证据。
//   - lookAtEntity(villager)：精确朝向 villager 眼部，shootFrom 用此朝向发射，三叉戟 1 tick 命中实体。
//   - 命中实体 onEntityHit setDealtDamage(true) 触发返回门控（无需插地）。
//   - tick 19 重新 lookAtEntity 校准（补偿 villager AI 移动）。
function setupLoyaltyThrower(test: Test, tridentStack: any): { player: any; victim: any } {
    (test as any).killAllEntities();
    // Survival 玩家（第三参数 0），主手三叉戟数量 1（slot 0）。
    const player = test.spawnSimulatedPlayer(THROWER_POS, "thrower", 0 as any);
    player.setItem(tridentStack as unknown as Parameters<typeof player.setItem>[0], 0, true);

    const victim = test.spawn(VILLAGER_TYPE, VICTIM_POS);
    (player as any).lookAtEntity(victim);

    // tick 5 useItem(三叉戟) → setActiveHand 拉弓（useDuration=72000）。
    test.runAtTickTime(5, () => {
        (player as any).useItem(tridentStack as unknown as Parameters<typeof player.useItem>[0]);
    });
    // tick 19 释放前重新 lookAtEntity 校准朝向（补偿 villager AI 移动）。
    test.runAtTickTime(19, () => {
        (player as any).lookAtEntity(victim);
    });
    // tick 20 stopUsingItem 释放 → 蓄力 15 tick ≥ MIN_CHARGE_TICKS=10 → 投掷（Survival 消耗 1）。
    test.runAtTickTime(20, () => {
        (player as any).stopUsingItem();
    });

    return { player, victim };
}

// 忠诚 III 三叉戟命中实体后飞回玩家并被拾取（验证 _tickReturning + onPlayerPickup 回返链路）。
//
// Survival 玩家持忠诚 III 三叉戟×1 朝近距 villager 投掷：
//   - 投掷消耗 1（主手 1→0 空槽），三叉戟命中 villager onEntityHit setDealtDamage(true)。
//   - tick 检测 hasDealtDamage && loyalty=3>0 → _tickReturning 穿墙飞回玩家。
//   - distance<2 → onPlayerPickup → inventory.add(count=1 三叉戟)（主手空槽 0→1 恢复）+ remove()。
//
// 判定（双重断言，pollUntilSucceed）：
//   - trident 实体消失（getEntities 返 0）：回返拾取后 remove()。
//   - 主手数量恢复 1（1→0 消耗 → 0→1 拾回）。
//   - 若回返断裂：实体停 villager 附近不消失 + 主手停留 0 → 超时 FAIL。
//   - 若未命中 villager：不 setDealtDamage 不返回，实体停留 → 超时 FAIL。
//   - 若 TridentItem 未对齐 copyWithCount(1)（实体存 count>1）：拾取 add 落别槽，主手停留 0 → 超时 FAIL。
function tridentLoyaltyReturnsToThrowerTest(test: Test): void {
    const trident = makeTrident(1); // 主手持 1 个，验证消耗 1 + 拾回 1 恢复
    trident.addEnchantment({ type: "minecraft:loyalty", level: 3 });
    const { player } = setupLoyaltyThrower(test, trident);

    // 轮询双重断言：trident 实体消失（回返拾取 remove）+ 主手数量恢复 1（消耗 1 后拾回 1）。
    pollUntilSucceed(test, () => {
        const tridents = test.getDimension().getEntities({
            type: "minecraft:trident",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (tridents.length !== 0) {
            return false;
        }
        return getMainHandAmount(player) === 1;
    }, {
        startTick: 25,
        interval: 2,
        maxTick: 130,
        onTimeout: () => {
            const tridents = test.getDimension().getEntities({
                type: "minecraft:trident",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            const tEnt = tridents.length > 0 ? (tridents[0] as any) : null;
            const tLoc = tEnt?.location;
            const tInfo = tEnt
                ? `trident pos=(${tLoc?.x},${tLoc?.y},${tLoc?.z})`
                : `trident count=${tridents.length}`;
            test.assert(false,
                `trident_loyalty_returns_to_thrower: failed: ${tInfo} mainHandAmount=${getMainHandAmount(player)} `
                + `(expected trident gone + mainHand=1 [1->0 consume ->0->1 pickup]; `
                + `if trident stuck near villager loyalty return broken [_tickReturning not triggered] `
                + `or pickup failed [onPlayerPickup not adding to inventory] `
                + `or trident missed villager [no onEntityHit, no setDealtDamage] `
                + `or TridentItem not copyWithCount(1) [entity stored count>1, pickup landed off-hand slot])`);
        },
    });
}

// 无忠诚三叉戟命中实体后不返回（负向对照，验证 loyalty>0 门控）。
//
// Survival 玩家持无附魔三叉戟×1 朝近距 villager 投掷：
//   - 投掷消耗 1（主手 1→0 空槽），三叉戟命中 villager setDealtDamage(true) 但 loyalty=0。
//   - tick 检测 loyalty=0 → 不触发 _tickReturning，三叉戟停在 villager 附近（反弹后落地/插地）。
//
// 判定：三叉戟实体持续存在（不返回）+ 主手停留 0（消耗后不拾回恢复）。
//   - 若门控 loyalty>0 失效（无忠诚也返回）：实体消失 + 主手恢复 1 → 与"不返回"冲突 → FAIL。
//   此测试与 trident_loyalty_returns_to_thrower 交叉验证：忠诚返回 + 无忠诚不返回 = 门控正确。
function tridentNoLoyaltyDoesNotReturnTest(test: Test): void {
    const trident = makeTrident(1);
    const { player } = setupLoyaltyThrower(test, trident);

    // 轮询断言：三叉戟实体持续存在（不返回）+ 主手停留 0（消耗后不拾回）。
    // startTick=25 确保三叉戟已命中稳定，maxTick=80 充分覆盖（无忠诚恒不返回，超时即 FAIL）。
    pollUntilSucceed(test, () => {
        const tridents = test.getDimension().getEntities({
            type: "minecraft:trident",
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return tridents.length === 1 && getMainHandAmount(player) === 0;
    }, {
        startTick: 25,
        interval: 2,
        maxTick: 80,
        onTimeout: () => {
            const tridents = test.getDimension().getEntities({
                type: "minecraft:trident",
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(false,
                `trident_no_loyalty_does_not_return: failed: tridentCount=${tridents.length} `
                + `mainHandAmount=${getMainHandAmount(player)} `
                + `(expected tridentCount=1 [stuck near villager, no return] + mainHand=0 [consumed, not picked up]; `
                + `if tridentCount=0 or mainHand=1 loyalty gate broken [returning without loyalty enchant])`);
        },
    });
}

export function registerTridentEnchantTests(): void {
    GameTest.register("MobBehaviorTests", "trident_base_damage", tridentBaseDamageTest)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "trident_impaling_aquatic_bonus", tridentImpalingAquaticBonusTest)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "trident_impaling_non_aquatic_no_bonus", tridentImpalingNonAquaticNoBonusTest)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    // 引雷测试独占 batch("thunder") 串行 + skyAccess(true) 露天 + runOnFinish 恢复晴天（防雷暴污染）。
    // maxTicks 200：留足雷暴强度渐变（~91 tick）+ 投掷（tick 120）+ 命中检测余量。
    GameTest.register("MobBehaviorTests", "trident_channeling_summons_lightning", tridentChannelingSummonsLightningTest)
        .batch("thunder")
        .structureName("gametests:creeper_pit")
        .skyAccess(true)
        .maxTicks(200);

    // 忠诚回返测试：Survival 投掷→插地→返回→拾取，maxTicks 170 留足返回+拾取时序余量。
    GameTest.register("MobBehaviorTests", "trident_loyalty_returns_to_thrower", tridentLoyaltyReturnsToThrowerTest)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(170);

    // 无忠诚不返回对照：maxTicks 100 充分覆盖（无忠诚恒不返回）。
    GameTest.register("MobBehaviorTests", "trident_no_loyalty_does_not_return", tridentNoLoyaltyDoesNotReturnTest)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(100);
}
