// 火焰附加附魔行为类 GameTest。
//
// 验证 Cubium 近战攻击链路（Player::attack）正确接入火焰附加附魔：持有火焰附加剑近战攻击实体，
// 攻击成功后将目标剩余着火时间设置为 level×4 秒（对齐 MC Java 1.21.11）。
//
// wiki 行为（docs\minecraft-wiki-source\minecraft_wiki\tech_火焰附加.txt#造成实体着火）：
//   "持有具有火焰附加魔咒的物品近战攻击实体……会将其剩余着火时间设置为 等级×80 tick"
//   即 level×4 秒（1 秒=20 tick）。火焰附加最大 II 级，II 级=8 秒=160 tick。
//
// C++ 链路：
//   脚本 player.attackEntity(target)（ScriptSimulatedPlayer.cpp，转发 Player::attack）
//     → Player::attack（Player.cpp:2588-2600）：取主手 fireAspectLevel
//         （EnchantmentHelper::getEnchantmentLevel(mainHand, FIRE_ASPECT)）。
//       攻击前若 fireAspectLevel>0 且目标未着火，先 igniteForTicks(20)（用于燃烧传递判定，wasBurning 标志）。
//     → Player.cpp:2624 livingTarget->hurt(damageSource, totalDamage) 应用近战伤害。
//     → Player.cpp:2703-2707（attacked 成功分支）：
//         if (fireAspectLevel > 0) livingTarget->igniteForSeconds(level * 4.0f);
//       即 FireComponent.m_fire = level×80（仅在新值大于当前剩余时更新）。
//
// 着火判定（Entity.hpp:1585）：isOnFire() = !isImmuneToFire() && getRemainingFireTicks() > 0。
//   脚本侧 Entity.getComponent("minecraft:onfire")（MinecraftModuleFactory.cpp:1340-1345）：
//   isOnFire() 为 true 时返回 OnFireComponent 对象，否则返回 undefined。
//   故攻击后读 getComponent("minecraft:onfire") 非 undefined 即证明目标着火。
//
// 火焰伤害时序（FireTickSystem.cpp:24-40）：m_fire>0 时每 20 tick（1 秒）造成 1 点 onFire 伤害并递减 m_fire。
//   火焰附加 II=160 tick，最多 8 点火焰伤害。目标 zombie HP 20，剑掉 7（HP→13），8 秒火焰最多再掉 8（HP→5），
//   不会在轮询窗口内致死移除实体，onfire 组件可持续读取。
//
// 附魔施加：脚本无 addEnchantment 绑定，用 player.chat("/enchant @s fire_aspect 2")
//   （SimulatedPlayer permLevel=4 与 gameMode 解耦，survival 亦可执行，同 MeleeEnchantDamageTests 范式）。
//
// 防假通过设计（正反对照）：
//   - fire_aspect_ii_ignites_target：火焰附加 II 剑攻击 zombie，攻击后 zombie 着火（onfire 非 undefined）。
//     若火焰附加未接入（Player.cpp:2703 分支缺失或 fireAspectLevel 恒 0），zombie 不着火，onfire undefined → 超时 FAIL。
//   - no_enchant_does_not_ignite_target：无附魔剑攻击 zombie，攻击后 zombie 不着火（onfire 恒 undefined）。
//     若攻击链路对所有攻击都点火（fireAspectLevel 判定失效恒 >0），zombie 着火 → 断言 FAIL。
//     两测试交叉验证：附魔剑点火 + 无附魔不点火 = 火焰附加接入正确。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch + killAllEntities 清场。
//   - night batch：zombie 亡灵白天燃烧干扰 onfire 判定（daylight burning 也会让 onfire 非 undefined），
//     night 避开日光燃烧，zombie 仅在火焰附加攻击后着火，onfire 来源纯粹。
//   - killAllEntities 清场：night batch doMobSpawning 可能自然刷 zombie，清场后单 zombie + 玩家隔离。
//   - creeper_pit 无顶无雨，FireTickSystem 雨中扑灭分支（isInRain）不触发，火焰持续至自然熄灭。
//
// 实体身份隔离：闭包持有 test.spawn 返回的 zombie 句柄读 getComponent("minecraft:onfire")，
// 不按 type 区域查询，规避 night 自然刷怪污染。
//
// 攻击冷却：SimulatedPlayer spawn 后未攻击过，首次 attackEntity 时冷却 progress≈1.0（满冷却）。
//   玩家 spawn 后等 30 tick（就位 + AI 稳定）再首次攻击，确保满冷却 + 玩家已落地。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑（helper y=1）。
// 玩家与目标距 2 格（ENTITY_INTERACTION_RANGE 默认 3.0 内）。
const PLAYER_POS = { x: 3, y: 2, z: 3 };
const TARGET_POS = { x: 3, y: 2, z: 5 };

// 读取实体着火状态：onfire 组件非 undefined 即着火（对齐基岩 OnFireComponent 语义）。
function isOnFire(entity: any): boolean {
    return entity.getComponent("minecraft:onfire") !== undefined;
}

// 构造钻石剑 ItemStack。Cubium 的 @minecraft/server 与 @minecraft/server-gametest 依赖的
// @minecraft/server 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过（同 MeleeEnchantDamageTests 范式）。
function makeDiamondSword(): any {
    return new ItemStack("minecraft:diamond_sword", 1);
}

// 装备钻石剑到玩家主手并施加火焰附加 II。
//   - setItem(stack, 0, true)：slot=0 主手，selectSlot=true 同步选中。
//   - /enchant @s fire_aspect 2：permLevel=4（SimulatedPlayer 与 gameMode 解耦）survival 亦可执行。
function equipFireAspectSword(player: any): void {
    player.setItem(makeDiamondSword(), 0, true);
    player.chat("/enchant @s fire_aspect 2");
}

// 装备无附魔钻石剑到玩家主手（反向对照，验证攻击本身不点火）。
function equipPlainSword(player: any): void {
    player.setItem(makeDiamondSword(), 0, true);
}

// 火焰附加 II 剑近战攻击使目标着火（验证火焰附加接入 + 着火时间设置）。
//
// zombie 非火焰免疫（isImmuneToFire()=false），night 不自燃，火焰附加 II 攻击后着火 8 秒（160 tick）。
// isOnFire()=true → getComponent("minecraft:onfire") 返回 OnFireComponent 非 undefined。
//
// 判定：tick 30 玩家首次 attackEntity（满冷却），pollUntilSucceed 轮询 zombie onfire 非 undefined。
//   攻击在 tick 30 同步执行 igniteForSeconds(8) 设 m_fire=160，isOnFire 立即为 true，
//   startTick 35 轮询即可读到 onfire。
//   若火焰附加未接入（Player.cpp:2703 分支缺失或 fireAspectLevel 恒 0），zombie 不着火 → 超时 FAIL。
//
// night batch + killAllEntities：zombie 亡灵白天燃烧干扰，night 避开；清场隔离自然刷怪。
// Ref: Player.cpp:2703-2707（attacked 成功分支 igniteForSeconds(level*4)）
// Ref: tech_火焰附加.txt#造成实体着火（着火时间=等级×80 tick）
function fireAspectIIIgnitesTarget(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn("minecraft:zombie", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any); // 0=Survival

    equipFireAspectSword(player);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(zombie);
    });

    // 攻击成功后 zombie 着火 8 秒。onfire 组件非 undefined 即通过。
    pollUntilSucceed(test, () => isOnFire(zombie), {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `fire aspect II should ignite target on melee hit (zombie onfire after attack), `
            + `but zombie onfire=${isOnFire(zombie)} (fire aspect not wired — `
            + `Player.cpp:2703 igniteForSeconds branch missing or fireAspectLevel always 0)`),
    });
}

// 无附魔剑近战攻击不使目标着火（正向对照，防 fireAspectIIIgnitesTarget 假通过）。
//
// 无附魔钻石剑攻击 zombie，fireAspectLevel=0，Player.cpp:2703 分支不执行，zombie 不着火。
// night batch zombie 不自燃，onfire 恒 undefined。
//
// 判定：tick 30 玩家 attackEntity，tick 70（攻击后 40 tick）断言 zombie 仍不着火（onfire undefined）。
//   用 runAtTickTime 延迟断言而非 succeedWhen：succeedWhen 对"恒 undefined"会立即通过（攻击前就 undefined），
//   无法验证"攻击后也不着火"。延迟到攻击后 40 tick 断言，若攻击链路意外点火（fireAspectLevel 判定失效恒 >0），
//   40 tick 内 zombie 已着火（m_fire=160 持续 8 秒），onfire 非 undefined → 断言 FAIL。
//   40 tick 远小于火焰附加 160 tick 燃烧时间，足以检出误点火。
//
// night batch + killAllEntities：同 fireAspectIIgnitesTarget，隔离日光燃烧与自然刷怪。
// Ref: Player.cpp:2597（fireAspectLevel>0 守卫，无附魔 level=0 不点火）
function noEnchantDoesNotIgniteTarget(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn("minecraft:zombie", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any);

    equipPlainSword(player);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(zombie);
    });

    // 攻击后 40 tick 断言 zombie 仍未着火（无附魔攻击不点火）。
    test.runAtTickTime(70, () => {
        if (isOnFire(zombie)) {
            test.assert(false,
                `plain sword (no enchant) should NOT ignite target, `
                + `but zombie onfire=${isOnFire(zombie)} (fireAspectLevel guard broken — `
                + `attack ignites even without fire aspect)`);
        }
        test.succeed();
    });
}

export function registerFireAspectEnchantTests(): void {
    GameTest.register("MobBehaviorTests", "fire_aspect_ii_ignites_target", fireAspectIIIgnitesTarget)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "no_enchant_does_not_ignite_target", noEnchantDoesNotIgniteTarget)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
