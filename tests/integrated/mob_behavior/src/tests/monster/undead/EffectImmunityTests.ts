// 亡灵/铁傀儡效果免疫行为类 GameTest。
//
// 验证 Cubium 亡灵生物 + 铁傀儡免疫中毒/再生效果（对齐 MC Java 1.21.11
// LivingEntity.canBeAffected：EntityTypeTags.IGNORES_POISON_AND_REGEN 标签实体免疫
// REGENERATION + POISON）。
//
// 此前缺陷：Cubium EffectManager::addEffect 不调 isPotionApplicable（vanilla canBeAffected
// 等价物），亡灵被中毒扣血/被再生治疗——与 vanilla 直接相反。本次修复：
//   1. EffectManager::addEffect 开头调 entity.isPotionApplicable(effect)，false 则拒绝施加
//      （对齐 vanilla LivingEntity.forceAddEffect:1027 → canBeAffected 门控）。
//   2. LivingEntity::isPotionApplicable 基类实现加 IGNORES_POISON_AND_REGEN 标签免疫 Poison/Regen
//      （对齐 vanilla LivingEntity.canBeAffected:1014-1024）。
//   此修复同时激活既有 EnderDragon（全免疫）/ Wither（免疫凋零）override，此前形同虚设。
//
// C++ 链路：
//   脚本 Entity.addEffect("poison", duration, {amplifier})（MinecraftModuleFactory.cpp:1770-1808）
//     → living->addEffect(EffectInstance(Poison, duration, amplifier))
//     → EffectManager::addEffect（EffectManager.cpp:40）→ 开头 entity.isPotionApplicable(effect)
//     → LivingEntity::isPotionApplicable（LivingEntity.cpp:2618）：IGNORES_POISON_AND_REGEN 标签
//       含 zombie（亡灵），Poison 返回 false → addEffect return false 不施加。
//   非亡灵（creeper）不在标签内，isPotionApplicable 返 true → 正常施加，Poison 每 25 tick 扣 1 血。
//
// 效果字符串映射（EffectType.cpp:44-85）：poison→Poison, regeneration→Regeneration,
//   instant_health→InstantHealth（非 instant_healing，注意正确字符串）。
// 瞬间治疗对亡灵反转（EffectInstance.cpp:246-258）：amplifier=0 伤害量=4+2*0=4（zombie 20→16）。
//
// 免疫判定用 EntityTypeTags::IGNORES_POISON_AND_REGEN 标签（13 亡灵 + iron_golem，
// EntityTypeTags.cpp:629-645），非 getCreatureAttribute 枚举（对齐 vanilla 标签判定）。
// zombie 在标签内（亡灵），creeper 不在。
//
// 防假通过设计：负向断言"zombie 不中毒"若单独存在，addEffect 绑定整体失效也会假过。故配正向对照
//   测试 nonUndeadAffectedByPoison（creeper 确实中毒）——若 addEffect 链路失效，正向测试 FAIL 暴露。
//
// 实体身份隔离：用闭包直接持有 test.spawn 返回的实体句柄读 getEffect/getComponent，不按 type 区域
//   查询。night batch 下 doMobSpawning 默认开启，creeper_pit 开放坑低亮度可能在结构内自然刷怪，
//   按 type 查询会污染；闭包持句柄只读自己 spawn 的实体，规避污染（对齐 CaveSpiderTests
//   caveSpiderDoesNotBurnInDaylight 闭包范式）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 亡灵生物（僵尸）免疫中毒效果（wiki other_中毒.txt#免疫：亡灵生物免疫中毒；对齐 vanilla
//   LivingEntity.canBeAffected IGNORES_POISON_AND_REGEN 标签免疫 POISON）。
//
// 本测试专项验证 LivingEntity::isPotionApplicable 的 IGNORES_POISON_AND_REGEN + Poison 免疫分支。
// zombie 是亡灵（UNDEAD 标签成员，亦在 IGNORES_POISON_AND_REGEN 标签内），addEffect("poison")
// 经 isPotionApplicable 返 false 被拒绝，zombie 不获 Poison 效果、HP 不因中毒下降。
//
// 判定（双重）：tick 100 后
//   1. zombie.getEffect("poison") === undefined（免疫未施加）。
//   2. zombie HP == 20（满血，中毒未扣血；无其他伤害源）。
//
// 时序：addEffect 在 tick 5 施加（被免疫拒绝），tick 100 后断言。Poison amplifier=0 每 25 tick 扣 1
//   （EffectInstance.cpp:285-296，interval=25>>0=25，health>1 才扣），若未免疫 100 tick 内应扣 ~3-4 血。
//   tick 100 留足 Poison 多个伤害周期验证"确实未扣血"。
//
// 环境选择：creeper_pit + night batch。zombie 是亡灵白天燃烧扣血干扰 HP 断言，night batch 避开阳光。
//   creeper_pit 开放坑无顶，night 无阳光，zombie 不燃。zombie (3,2,3) 脚下 grass_block 支撑。
//   不 spawn 玩家（避免 zombie 追击玩家走动/攻击致 HP 变化干扰）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_中毒.txt#免疫（亡灵免疫中毒）
// Ref: LivingEntity.cpp:2618（isPotionApplicable IGNORES_POISON_AND_REGEN 免疫）/ EffectManager.cpp:40（addEffect 门控）
function undeadImmuneToPoison(test: Test): void {
  const zombieType = "zombie";

  // zombie (3,2,3) 脚踩结构内 y=0 grass_block（helper-y=2→结构内 y=1 air）。night batch 不燃。
  const zombie = test.spawn(zombieType, { x: 3, y: 2, z: 3 });

  // tick 5 施加 Poison 200 tick amplifier=0。zombie 亡灵免疫，isPotionApplicable 返 false 拒绝施加。
  // (zombie as any).addEffect 绕过 TS 类型（自定义绑定无类型定义）。
  test.runAtTickTime(5, () => {
    (zombie as any).addEffect("poison", 200, { amplifier: 0 });
  });

  // tick 100 后断言：zombie 未中毒（getEffect undefined）+ HP 仍满血 20（中毒未扣血）。
  // Poison amplifier=0 每 25 tick 扣 1，100 tick 内若未免疫应扣 ~3-4 血。tick 100 验证多周期未扣血。
  // 用闭包持有的 zombie 句柄读 getEffect/getComponent，不按 type 区域查询，规避 night 自然刷怪污染。
  test.runAtTickTime(100, () => {
    const poison = (zombie as any).getEffect("poison");
    test.assert(poison === undefined,
      `undead zombie should be immune to poison (IGNORES_POISON_AND_REGEN), `
      + `but getEffect("poison")=${poison === undefined ? "undefined" : "present"}`);
    const health = zombie.getComponent("minecraft:health");
    test.assert(health !== undefined, "zombie has no health component");
    test.assert((health as any).currentValue === 20,
      `undead zombie HP should remain 20 (poison immune), hp=${(health as any).currentValue}`);
    test.succeed();
  });
}

// 非亡灵生物（苦力怕）受中毒效果影响（正向对照，防 undeadImmuneToPoison 假通过）。
//
// creeper 非亡灵（不在 IGNORES_POISON_AND_REGEN 标签），addEffect("poison") 正常施加，
// Poison 每 25 tick 扣 1 血。本测试证明 addEffect 链路 + Poison 效果本身有效——若链路失效，
// 本测试 FAIL（creeper 不中毒），从而暴露 undeadImmuneToPoison 的假通过风险。
//
// 判定（双重）：tick 100 后
//   1. creeper.getEffect("poison") !== undefined（效果已施加）。
//   2. creeper HP < 20（中毒已扣血）。
//
// 与 undeadImmuneToPoison 配对：creeper 中毒 + zombie 不中毒，交叉验证免疫判定（标签内 vs 标签外）。
//   若 isPotionApplicable 整体失效（恒返 true），zombie 也会中毒 → undeadImmuneToPoison FAIL。
//   若 Poison 效果本身失效，creeper 不中毒 → 本测试 FAIL。两测试互补。
//
// 环境同 undeadImmuneToPoison：creeper_pit + night batch。creeper 非亡灵不燃，night 仅统一环境。
//   creeper (3,2,3) 脚踩 grass_block。不 spawn 玩家避免 creeper 膨胀/爆炸致 HP 变化干扰。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_中毒.txt（非亡灵受中毒扣血）
function nonUndeadAffectedByPoison(test: Test): void {
  const creeperType = "creeper";

  // creeper (3,2,3) 脚踩结构内 y=0 grass_block。creeper 非亡灵不燃。
  const creeper = test.spawn(creeperType, { x: 3, y: 2, z: 3 });

  // tick 5 施加 Poison 200 tick amplifier=0。creeper 非亡灵，正常施加。
  test.runAtTickTime(5, () => {
    (creeper as any).addEffect("poison", 200, { amplifier: 0 });
  });

  // tick 100 后断言：creeper 已中毒（getEffect 非 undefined）+ HP<20（中毒扣血）。
  // Poison amplifier=0 每 25 tick 扣 1，100 tick 内扣 ~3-4 血（HP 16-17）。
  test.runAtTickTime(100, () => {
    const poison = (creeper as any).getEffect("poison");
    test.assert(poison !== undefined,
      `non-undead creeper should be affected by poison, but getEffect("poison")=undefined `
      + `(addEffect link or Poison effect broken — undeadImmuneToPoison may be false-passing)`);
    const health = creeper.getComponent("minecraft:health");
    test.assert(health !== undefined, "creeper has no health component");
    test.assert((health as any).currentValue < 20,
      `non-undead creeper HP should drop from poison, hp=${(health as any).currentValue}`);
    test.succeed();
  });
}

// 亡灵生物（僵尸）免疫再生效果（wiki：亡灵免疫再生；对齐 vanilla canBeAffected
//   IGNORES_POISON_AND_REGEN 标签免疫 REGENERATION）。
//
// zombie 亡灵免疫 Regen：addEffect("regeneration") 被拒绝，zombie 不获 Regen、HP 不回血。
// 为验证"免疫 Regen 不回血"，先让 zombie 残血：addEffect("instant_health") 对亡灵反转=伤害
// （EffectInstance.cpp:246-258，instant_health 不在 IGNORES_POISON_AND_REGEN 免疫集，正常施加 +
// applyEffectTick 内 getCreatureAttribute==Undead 反转走 hurt(magic, 4+2*amp)）。
//   amplifier=0 伤害量=4（zombie 20→16）。再 addEffect("regeneration") → 免疫不施加 → HP 不回血。
//
// 注意：瞬间治疗对亡灵伤害走 getCreatureAttribute==Undead 反转（EffectInstance.cpp:252），
//   非 isInvertedHealAndHarm 标签判定（Cubium 瞬间效果反转实际用 getCreatureAttribute，与 vanilla
//   标签判定的差异是既有实现，本测试不验证反转标签本身，只借反转造伤测 Regen 免疫）。
//
// 判定（三阶段）：
//   1. tick 5 addEffect("instant_health") → zombie 受伤掉血（瞬间治疗对亡灵=伤害，20→16）。
//   2. tick 20 断言 zombie HP<20（残血，证明瞬间治疗造伤成功，为 Regen 测试铺垫）。
//   3. tick 25 addEffect("regeneration", 200) → 亡灵免疫不施加。
//   4. tick 120 断言 zombie.getEffect("regeneration")===undefined + HP 未回升（仍<=阶段2残血值）。
//
// 时序：瞬间治疗 tick 5 立即造伤（InstantEffect apply 即时，EffectManager.cpp:56-63 isInstantEffect
//   分支调 applyInstantly→_applyEffect 立即执行），tick 20 验证残血；Regen tick 25 施加（免疫拒绝），
//   Regen amplifier=0 每 50 tick 回 1（EffectInstance.cpp:273-283，interval=50>>0=50），tick 120 验证
//   ~2 个 Regen 周期未回血。
//
// 环境同前：creeper_pit + night batch（zombie 不燃）。不 spawn 玩家避免 HP 干扰。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_生命恢复.txt（亡灵免疫再生）
// Ref: LivingEntity.cpp:2618（isPotionApplicable IGNORES_POISON_AND_REGEN 免疫 Regen）
function undeadImmuneToRegeneration(test: Test): void {
  const zombieType = "zombie";

  const zombie = test.spawn(zombieType, { x: 3, y: 2, z: 3 });

  // 阶段1：tick 5 施加瞬间治疗。亡灵瞬间治疗反转=伤害，zombie 掉血（instant_health 不在免疫集，正常施加+反转造伤）。
  // 瞬间治疗 amplifier=0 对亡灵造成 4 伤害（EffectInstance.cpp:251 amount=4.0+2*0=4.0），zombie 20→16。
  test.runAtTickTime(5, () => {
    (zombie as any).addEffect("instant_health", 1, { amplifier: 0 });
  });

  // 用于阶段2/4 对比 HP 的快照变量（闭包捕获）。
  let hpAfterDamage = 20;

  // 阶段2：tick 20 断言 zombie 残血（瞬间治疗造伤成功），记录残血值供阶段4对比。
  test.runAtTickTime(20, () => {
    const health = zombie.getComponent("minecraft:health");
    test.assert(health !== undefined, "zombie has no health component");
    const hp = (health as any).currentValue as number;
    test.assert(hp < 20,
      `zombie should take damage from instant_health (undead inversion), hp=${hp} `
      + `(if hp==20, instant_health inversion broken — cannot test regen immunity)`);
    hpAfterDamage = hp;
  });

  // 阶段3：tick 25 施加 Regen 200 tick amplifier=0。zombie 亡灵免疫，isPotionApplicable 返 false 拒绝。
  test.runAtTickTime(25, () => {
    (zombie as any).addEffect("regeneration", 200, { amplifier: 0 });
  });

  // 阶段4：tick 120 断言 zombie 未获 Regen（getEffect undefined）+ HP 未回升（<=hpAfterDamage）。
  // Regen amplifier=0 每 50 tick 回 1，tick 25→120 = 95 tick 约 2 周期，若未免疫应回 ~2 血。
  test.runAtTickTime(120, () => {
    const regen = (zombie as any).getEffect("regeneration");
    test.assert(regen === undefined,
      `undead zombie should be immune to regeneration (IGNORES_POISON_AND_REGEN), `
      + `but getEffect("regeneration")=${regen === undefined ? "undefined" : "present"}`);
    const health = zombie.getComponent("minecraft:health");
    test.assert(health !== undefined, "zombie has no health component");
    const hp = (health as any).currentValue as number;
    test.assert(hp <= hpAfterDamage,
      `undead zombie HP should not rise from regeneration (immune), hp=${hp} `
      + `(should be <= ${hpAfterDamage} after instant_health damage)`);
    test.succeed();
  });
}

export function registerEffectImmunityTests(): void {
  GameTest.register("MobBehaviorTests", "undead_immune_to_poison", undeadImmuneToPoison)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "non_undead_affected_by_poison", nonUndeadAffectedByPoison)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "undead_immune_to_regen", undeadImmuneToRegeneration)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(250);
}
