// 光灵箭（Spectral Arrow）命中施加发光（Glowing）效果对齐测试。
//
// 验证 Cubium 光灵箭效果施加链路（BowItem::onPlayerStoppedUsing → SpectralArrowItem::createArrow
// → SpectralArrowEntity::shootFrom → onEntityHit → hurt 成功 → doPostHurtEffects → addEffect(Glowing)）
// 对齐 MC Java 1.21.11。
//
// 任务 #285 修复背景：Cubium 此前 SpectralArrowEntity::onEntityHit 调父类后【无条件】施加 Glowing，
// 不检查 hurt 返回值；vanilla AbstractArrow.onHitEntity:453-468 在 if(hurtOrSimulate) 成功块内调
// doPostHurtEffects（SpectralArrow.java:40-45 重写施加 Glowing），hurt 失败（无敌帧/免疫）不施加。
// 修复引入 doPostHurtEffects 虚函数范式：父类 onEntityHit 在 hurt 成功后调 doPostHurtEffects，
// 子类重写施加效果。本测试验证修复后【正向链路仍通】——光灵箭命中 hurt 成功 → 施加 Glowing，无回归。
//
// 伤害公式（对齐 vanilla AbstractArrow.onHitEntity:420 + SpectralArrow 默认 baseDamage=2.0）：
//   baseDamage = 2.0（SpectralArrowEntity 构造 setDamage(2.0)，对齐 vanilla AbstractArrow.baseDamage=2.0）
//   满弓 velocity = 1.0（BowItem getArrowVelocity(chargeTicks≥20)=1.0），shootFrom 速度 = velocity*3.0 = 3.0
//   命中伤害 = ceil(speed * baseDamage) = ceil(3.0*2.0) = 6，暴击 0~4 → 总 6~10
//   villager HP 20 → 剩 10~14，存活可读 Glowing 效果。
//
// Glowing 效果（对齐 vanilla SpectralArrow.doPostHurtEffects:41-45）：
//   duration = glowDuration() = 200 ticks（10 秒，SpectralArrow.java:17 DEFAULT_DURATION=200）
//   amplifier = 0（Glowing I）
//   getEffect("glowing") 返回 { typeId, amplifier, duration }，amplifier===0 即 Glowing I。
//
// 时序（同 BowArrowDamageTests 拉弓释放范式）：
//   tick 5 useItem(弓) 拉弓（setActiveHand，useDuration=72000），tick 25 stopUsingItem 释放
//   （蓄力 20 tick，velocity=1.0 满弓，speed=3.0）。tick 26+ 光灵箭飞行 1 tick 命中 villager。
//   弓发射时 _findAmmoSlot 取副手 spectral_arrow（ArrowItem 子类，getAmmoPredicate 接受），
//   dynamic_cast<SpectralArrowItem*> 命中 → createArrow 创建 SpectralArrowEntity。
//
// 防假通过设计：
//   - spectral_arrow_inflicts_glowing：光灵箭命中后 villager 获得 Glowing（amplifier===0）。
//     若 doPostHurtEffects 链路断裂（修复时漏调/重写错误），villager 无 Glowing → 超时 FAIL。
//     若 Glowing amplifier≠0（效果构造错误），amplifier===0 断言失败 → FAIL。
//   - plain_arrow_no_glowing：负向对照，普通箭命中 villager 不施加 Glowing。
//     防 spectral_arrow_inflicts_glowing 假通过（如 villager 因其他原因发光）。
//     两测试交叉验证：光灵箭有 Glowing + 普通箭无 Glowing = 光灵箭 doPostHurtEffects 链路对齐。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ killAllEntities 清场（隔离自然刷怪干扰效果读取）。
// 玩家 (3,2,3) 默认 yaw=0 pitch=0 朝 +Z，villager (3,2,4) 距 1 格正前方（光灵箭 1 tick 命中，衰减极小）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: AbstractArrowEntity.cpp:533（onEntityHit hurt 成功后调 doPostHurtEffects）
//      + SpectralArrowEntity::doPostHurtEffects（addEffect Glowing glowDuration）
// Ref: vanilla AbstractArrow.java:453-468（hurtOrSimulate 成功块内 doPostHurtEffects）
//      + SpectralArrow.java:40-45（doPostHurtEffects 施加 Glowing DEFAULT_DURATION=200）
// Ref: SpectralArrowItem.cpp:44 createArrow（弓发射光灵箭创建 SpectralArrowEntity）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const BOW = "minecraft:bow";
const SPECTRAL_ARROW = "minecraft:spectral_arrow";
const ARROW = "minecraft:arrow";
const VILLAGER_TYPE = "minecraft:villager";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const ARCHER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_POS = { x: 3, y: 2, z: 4 }; // 距玩家 1 格正前方（+Z），光灵箭 1 tick 命中

// 读取实体当前血量（HP）。实体句柄由闭包持有，规避区域查询污染。实体死亡移除后返回 -1。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 BowArrowDamageTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 拉弓释放射箭通用流程：玩家主手弓 + 副手指定箭矢，tick 5 拉弓 tick 25 释放（满弓 20 tick）。
// arrowItemId 由调用方决定（spectral_arrow 或 arrow）。返回靶 villager 引用供断言。
function setupArcherAndVictim(test: Test, arrowItemId: string): { player: any; victim: any } {
    (test as any).killAllEntities();
    const player = test.spawnSimulatedPlayer(ARCHER_POS, "archer", 0 as any); // 0=Survival
    player.setItem(makeItem(BOW), 0, true); // 主手弓 slot 0
    const arrow = new ItemStack(arrowItemId, 5);
    player.setItem(arrow as unknown as Parameters<typeof player.setItem>[0], 40, false); // 副手 slot 40

    const victim = test.spawn(VILLAGER_TYPE, VICTIM_POS);

    // tick 5 useItem(弓) → setActiveHand 拉弓（useDuration=72000）。
    test.runAtTickTime(5, () => {
        (player as any).useItem(makeItem(BOW) as unknown as Parameters<typeof player.useItem>[0]);
    });
    // tick 25 stopUsingItem 释放 → 满弓 20 tick（chargeTicks=20）→ velocity=1.0 → speed=3.0。
    test.runAtTickTime(25, () => {
        (player as any).stopUsingItem();
    });

    return { player, victim };
}

// 光灵箭命中 villager 施加发光效果（验证 SpectralArrowEntity::doPostHurtEffects 链路通）。
//
// 光灵箭满弓命中：hurt 成功（damage 6~10 < 20，villager 存活）→ doPostHurtEffects → addEffect(Glowing, 200, 0)。
// 判定：pollUntilSucceed 轮询 villager.getEffect("glowing") 非 undefined 且 amplifier === 0（Glowing I）。
//   若 doPostHurtEffects 链路断裂（修复漏调 onEntityHit 内 doPostHurtEffects / 重写错误），
//   villager 无 Glowing → 超时 FAIL。
//   Glowing duration 200 tick，轮询 interval 5 tick，窗口 100 tick 足够捕获。
function spectralArrowInflictsGlowingTest(test: Test): void {
    const { victim } = setupArcherAndVictim(test, SPECTRAL_ARROW);

    // 轮询 villager 获得 Glowing I（amplifier=0）。villager 存活（6~10<20）可读效果。
    pollUntilSucceed(test, () => {
        const glowing = (victim as any).getEffect("glowing");
        if (glowing === undefined) {
            return false;
        }
        // amplifier === 0 即 Glowing I（SpectralArrow.doPostHurtEffects amplifier=0）。
        return (glowing as any).amplifier === 0;
    }, {
        startTick: 28,
        interval: 5,
        maxTick: 100,
        onTimeout: () => {
            const glowing = (victim as any).getEffect("glowing");
            const amp = glowing !== undefined ? (glowing as any).amplifier : "N/A";
            test.assert(false,
                `spectral_arrow_inflicts_glowing: failed: villager glowing=${glowing === undefined ? "absent" : `present(amplifier=${amp})`} `
                + `HP=${readHp(victim)} (if absent, doPostHurtEffects not called after hurt success — `
                + `task #285 SpectralArrowEntity::doPostHurtEffects chain broken)`);
        },
    });
}

// 普通箭命中 villager 不施加发光效果（负向对照，防 spectral_arrow_inflicts_glowing 假通过）。
//
// 普通箭（ArrowEntity）无 Glowing 效果逻辑，doPostHurtEffects 基类空实现（ArrowEntity 重写仅施加药水效果，
// 普通箭无 effects 列表故不施加任何效果）。命中 villager 掉血（证明攻击发生）但无 Glowing。
//
// 判定：pollUntilSucceed 轮询"villager HP<20（被攻击掉血，证明箭命中链路通）且 villager 无 Glowing"。
//   - 若 villager 根本没被攻击（HP==20），condition 不满足 → 超时 FAIL（避免"无 Glowing 但没攻击"假通过）。
//   - 若普通箭误施加 Glowing（ArrowEntity::doPostHurtEffects 逻辑错误），villager 有 Glowing → FAIL。
//   只有"villager 被攻击掉血 + 无 Glowing"同时成立才 succeed，严格验证普通箭不发光。
//   两测试交叉验证：光灵箭有 Glowing + 普通箭无 Glowing = 光灵箭 doPostHurtEffects 链路对齐。
function plainArrowNoGlowingTest(test: Test): void {
    const { victim } = setupArcherAndVictim(test, ARROW);

    // 负向对照：villager 被攻击掉血（HP<20，证明箭命中链路通）+ villager 无 Glowing。
    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        const glowing = (victim as any).getEffect("glowing");
        // villager 存活（HP>0）+ 已掉血（HP<20，证明攻击发生）+ 无 Glowing。
        return hp > 0 && hp < 20 && glowing === undefined;
    }, {
        startTick: 28,
        interval: 5,
        maxTick: 100,
        onTimeout: () => {
            const hp = readHp(victim);
            const glowing = (victim as any).getEffect("glowing");
            test.assert(false,
                `plain_arrow_no_glowing: failed: villager should be damaged but have no glowing. `
                + `HP=${hp}, glowing=${glowing === undefined ? "absent" : "present"} `
                + `(if HP==20 arrow never hit; if glowing present, plain arrow wrongly inflicts glowing)`);
        },
    });
}

export function registerSpectralArrowTests(): void {
    GameTest.register("MobBehaviorTests", "spectral_arrow_inflicts_glowing", spectralArrowInflictsGlowingTest)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
    GameTest.register("MobBehaviorTests", "plain_arrow_no_glowing", plainArrowNoGlowingTest)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
}
