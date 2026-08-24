// 节肢杀手 Slowness 副作用行为类 GameTest。
//
// 验证 Cubium 节肢杀手附魔（BANE_OF_ARTHROPODS）近战命中节肢生物时施加缓慢 IV 副作用。
//
// 节肢杀手两项效果：
//   1. 伤害加成（DAMAGE 组件）：对节肢生物每级 +2.5，由 DamageEnchantment::getDamageBonus 实现
//      （查 SENSITIVE_TO_BANE_OF_ARTHROPODS 标签）。已由 bane_v_devastates_arthropod 测试覆盖。
//   2. 缓慢副作用（POST_ATTACK ATTACKER→VICTIM，施加 SLOWNESS 效果）：近战命中节肢生物时
//      施加缓慢 IV（amplifier=3 固定），持续时间 round(randomBetween(1.5, 1.5+0.5*(level-1))*20) tick。
//      由 BaneOfArthropodsEnchantment::onEntityDamaged 实现。本测试文件专测此项。
//
// C++ 链路（任务 #277 修复前断裂，修复后接通）：
//   脚本 player.attackEntity(spider) → Player::attack（Player.cpp:2654 if(attacked) 块）
//     → onAttackEntity(target)（任务 #277 新增调用，接通此前全仓零调用的死代码）
//     → LivingEntity::onAttackEntity（LivingEntity.cpp:2373）读主手武器
//     → EnchantmentHelper::applyArthropodEnchantmentDamage(user, target, mainHand)
//     → 遍历武器附魔调 onEntityDamaged
//     → BaneOfArthropodsEnchantment::onEntityDamaged：查 SENSITIVE_TO_BANE_OF_ARTHROPODS 标签命中
//       → addEffect(Slowness, duration, amplifier=3)
//   同理 MobEntity::attackEntityAsMob 基类（MobEntity.cpp:753）与 PolarBear/Ocelot 自管 override
//   均已补 onAttackEntity 调用，覆盖 mob 持节肢杀手武器近战的副作用链路。
//
// 此前缺陷（任务 #277 修复）：Player::attack 只用 getEnchantmentDamageBonus 取伤害数值，从不调
//   onAttackEntity（PlayerAttackHelper::applyEnchantmentEffects 封装了此调用却全仓零调用，为死代码），
//   致玩家持节肢杀手剑攻击节肢生物时缓慢副作用完全不触发。MobEntity::attackEntityAsMob 基类同病。
//   修复：Player::attack 与 MobEntity::attackEntityAsMob 攻击成功后显式调 onAttackEntity 接通回调链。
//
// 测试设计（正向 + 负向对照）：
//   - bane_inflicts_slowness_on_arthropod：玩家持节肢杀手 I 木剑攻击 spider（节肢），spider 存活
//     且获得 Slowness 效果（amplifier=3 = Slowness IV）。
//   - no_bane_no_slowness：玩家持无附魔木剑攻击 spider，spider 掉血（证明攻击发生）但无 Slowness。
//     防 bane_inflicts_slowness_on_arthropod 假通过（如 spider 因其他原因获得 Slowness）。
//     两测试交叉验证：持节肢杀手有 Slowness + 无附魔无 Slowness = 节肢杀手缓慢副作用正确。
//
// 伤害与存活：用节肢杀手 I（+2.5）+ 木剑（baseDamage 4.0）= 6.5 < spider HP 16，spider 存活可读效果。
//   不用节肢杀手 V（+12.5+7.0 钻石剑=19.5>16 一击致死，spider 死后 getEffect 不可读）。
//   缓慢 IV duration level I = round(randomBetween(1.5,1.5)*20)=30 tick（1.5 秒），轮询窗口 120 tick 足够。
//
// 攻击冷却：SimulatedPlayer spawn 后未攻击过，tick 30 首次 attackEntity 时冷却 progress≈1.0（满冷却），
//   baseDamage 全额生效。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch。
//   - night batch 避免 spider 白天被动不敌对干扰（虽本测试玩家主动 attackEntity 不依赖 spider AI，
//     但 night 统一隔离避免 spider 受惊逃跑离开攻击范围）。
//   - killAllEntities 清场：night batch doMobSpawning 可能自然刷怪干扰，清场后仅本测试 spawn 的
//     spider + 玩家，闭包持实体句柄读效果规避区域查询污染。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑（helper y=1）。
// 玩家与目标距 2 格（ENTITY_INTERACTION_RANGE 默认 3.0 内，attackEntity 可命中）。
const PLAYER_POS = { x: 3, y: 2, z: 3 };
const TARGET_POS = { x: 3, y: 2, z: 5 };

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
function makeWoodenSword(): any {
    return new ItemStack("minecraft:wooden_sword", 1);
}

// 装备木剑到玩家主手并施加节肢杀手 I。
//   - setItem(stack, 0, true)：slot=0 主手，selectSlot=true 同步选中。
//   - /enchant @s bane_of_arthropods 1：permLevel=4（SimulatedPlayer 与 gameMode 解耦）survival 亦可执行。
function equipBaneSword(player: any): void {
    player.setItem(makeWoodenSword(), 0, true);
    player.chat(`/enchant @s bane_of_arthropods 1`);
}

// 装备无附魔木剑到玩家主手（负向对照，验证无节肢杀手不施加 Slowness）。
function equipPlainSword(player: any): void {
    player.setItem(makeWoodenSword(), 0, true);
}

// 玩家持节肢杀手 I 木剑攻击蜘蛛，蜘蛛存活且获得 Slowness IV（验证节肢杀手缓慢副作用端到端链路接通）。
//
// spider 节肢（ARTHROPOD/SENSITIVE_TO_BANE_OF_ARTHROPODS 标签成员），节肢杀手 I 副作用施加缓慢 IV
//（amplifier=3 固定，duration 30 tick）。木剑 baseDamage 4.0 + 节肢 I 加成 2.5 = 6.5 < spider HP 16，
// spider 存活可读效果。
//
// 判定：tick 30 玩家首次 attackEntity（满冷却），pollUntilSucceed 轮询 spider.getEffect("slowness")
// 非 undefined 且 amplifier === 3（Slowness IV）。
//   若节肢杀手副作用链路断裂（Player::attack 未调 onAttackEntity / onEntityDamaged 未接标签 /
//   addEffect 未调），spider 无 Slowness → 超时 FAIL。
//   缓慢 IV duration 30 tick，轮询 interval 5 tick，窗口 120 tick 足够捕获。
//
// night batch + killAllEntities：隔离自然刷怪污染 + spider 被动状态干扰。
// Ref: BaneOfArthropodsEnchantment.cpp:38（onEntityDamaged 查标签 + addEffect Slowness）
// Ref: Player.cpp:2654（attack 成功后调 onAttackEntity 接通回调链，任务 #277 修复）
function baneInflictsSlownessOnArthropod(test: Test): void {
    (test as any).killAllEntities();
    const spider = test.spawn("minecraft:spider", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any); // 0=Survival

    equipBaneSword(player);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(spider);
    });

    // 轮询 spider 获得 Slowness IV（amplifier=3）。spider 存活（6.5<16）可读效果。
    // getEffect 返回 {amplifier, duration} 或 undefined。amplifier 是 0-based，Slowness IV → amplifier=3。
    pollUntilSucceed(test, () => {
        const slowness = (spider as any).getEffect("slowness");
        if (slowness === undefined) {
            return false;
        }
        // amplifier === 3 即 Slowness IV（节肢杀手固定施加 IV）。
        return (slowness as any).amplifier === 3;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => {
            const slowness = (spider as any).getEffect("slowness");
            const amp = slowness !== undefined ? (slowness as any).amplifier : "N/A";
            test.assert(false,
                `bane of arthropods I should inflict Slowness IV (amplifier=3) on spider, `
                + `but spider slowness=${slowness === undefined ? "absent" : `present(amplifier=${amp})`} `
                + `(onAttackEntity not wired in Player::attack or onEntityDamaged not adding Slowness — `
                + `if absent, task #277 callback chain broken; spider HP=${readHp(spider)})`);
        },
    });
}

// 无附魔木剑攻击蜘蛛，蜘蛛掉血但无 Slowness（负向对照，防 baneInflictsSlownessOnArthropod 假通过）。
//
// 玩家持无附魔木剑攻击 spider → onAttackEntity 调 applyArthropodEnchantmentDamage 遍历武器无节肢附魔
// → 不调 onEntityDamaged → spider 无 Slowness。spider 受 baseDamage 4.0 掉血（HP 16→12）证明攻击发生。
//
// 判定：pollUntilSucceed 轮询"spider HP<16（被攻击掉血，证明攻击链路通）且 spider 无 Slowness"。
//   - 若 onEntityDamaged 误对无附魔武器触发（getEnchantmentLevel 判定失效），spider 有 Slowness →
//     condition 不满足 → 超时 FAIL，从而暴露 baneInflictsSlownessOnArthropod 假通过。
//   - 若 spider 根本没被攻击（HP==16），condition 不满足 → 超时 FAIL（避免"spider 无 Slowness 但也没攻击"
//     的假通过——必须证明攻击确实发生）。
//   - 只有"spider 被攻击掉血 + spider 无 Slowness"同时成立才 succeed，严格验证无附魔不施加 Slowness。
//   两测试交叉验证：持节肢杀手有 Slowness + 无附魔无 Slowness = 节肢杀手缓慢副作用正确。
//
// night batch + killAllEntities：同 baneInflictsSlownessOnArthropod。
function noBaneNoSlowness(test: Test): void {
    (test as any).killAllEntities();
    const spider = test.spawn("minecraft:spider", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any);

    equipPlainSword(player);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(spider);
    });

    // 负向对照：spider 被攻击掉血（HP<16，证明攻击链路通）+ spider 无 Slowness。
    pollUntilSucceed(test, () => {
        const hp = readHp(spider);
        const slowness = (spider as any).getEffect("slowness");
        // spider 存活（HP>0）+ 已掉血（HP<16，证明攻击发生）+ 无 Slowness。
        return hp > 0 && hp < 16 && slowness === undefined;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => {
            const hp = readHp(spider);
            const slowness = (spider as any).getEffect("slowness");
            test.assert(false,
                `plain wooden sword should NOT inflict Slowness: spider should be damaged but have no slowness. `
                + `spider HP=${hp}, slowness=${slowness === undefined ? "absent" : "present"} `
                + `(if slowness present, onEntityDamaged triggered on unenchanted weapon — getEnchantmentLevel `
                + `check broken, baneInflictsSlownessOnArthropod may be false pass; if HP==16 spider never attacked)`);
        },
    });
}

export function registerBaneOfArthropodsSlownessTests(): void {
    GameTest.register("MobBehaviorTests", "bane_inflicts_slowness_on_arthropod", baneInflictsSlownessOnArthropod)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(250);

    GameTest.register("MobBehaviorTests", "no_bane_no_slowness", noBaneNoSlowness)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(250);
}
