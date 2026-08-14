// 幻术师行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// mediumglass 结构尺寸（12×9×11），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const MED_FROM = { x: 0, y: 0, z: 0 };
const MED_VOLUME = { x: 12, y: 9, z: 11 };

// 幻术师施放镜像法术使自己隐身（wiki mob_幻术师_ED.txt#SpellTicks：幻术师施法设 SpellTicks 倒计时；
// Illusioner.java IllusionerMirrorSpellGoal：未隐身且冷却结束时施放 Disappear 法术，对自身施加
// 隐身 1200 ticks(60秒)）。镜像法术无需攻击目标（IllusionerMirrorSpellGoal.canUse 仅校验
// !isCastingSpell && !hasInvisibility，不读 getTarget），只要幻术师未隐身即可施放。
//
// C++ 链路：IllusionerEntity : SpellcastingIllagerEntity : AbstractRaiderEntity，registerGoals 注册：
//   goalSelector 优先级4：IllusionerMirrorSpellGoal（镜像隐身，冷却 COOLDOWN=340）。
//   goalSelector 优先级1：IllusionerCastingSpellGoal（施法时停步+看目标，Move+Look flag）。
// IllusionerMirrorSpellGoal 继承 IllusionerSpellGoal，shouldExecute 要求：未施法 + 冷却结束。
//   startExecuting 设 warmup(WARMUP_TIME=20)+spellTicks，tick 中 warmup 递减到 0 时 castSpell→
//   对自身 addEffect(Invisibility, 1200)。
//   flag 修复：IllusionerSpellGoal 经修复后不占 flag（对齐原版 SpellcasterUseSpellGoal 无 flag），
//   与 IllusionerCastingSpellGoal(优先级1, Move+Look) 不再互斥抢占，warmup 持续递减到 0 执行施法。
//
// 环境选择：mediumglass（12×9×11 玻璃盒空腔）。幻术师(2,2,5) 站在地板上方（helper-y=2，结构内
// y=1 空气腔，地板 y=0 圆石）。镜像法术无需目标，玩家可不放；但放一个 Survival 玩家(10,2,5) 避免幻术师
// 因无目标而只随机漫步（NearestAttackableTarget 选到玩家后幻术师进入敌对态，更贴近实战施法触发）。
//
// 判定手段：检测幻术师获得 invisibility 效果（getEffect("invisibility") 非 undefined）。
//   镜像施法是确定性生成效果（warmup 结束即对自身 addEffect），不受命中随机性影响。
//   首次施法冷却为 0 可直接触发，warmup=20 + 余量。
// 幻术师 setBurnsInDaylight(false)（构造期关闭，对齐原版灾厄村民不燃），白天默认环境即可施法。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_幻术师_ED.txt#SpellTicks（镜像法术隐身）
function illusionerCastsMirror(test: Test): void {
  const illusionerType = "illusioner";

  // 幻术师 (2,2,5)，mediumglass 空气腔内（x∈[2,10]/z∈[1,9]），不卡玻璃墙。
  // 放 Survival 玩家 (10,2,5) 让幻术师选目标进入敌对态（镜像法术本身不依赖目标，但敌对态下施法更稳定）。
  test.spawn(illusionerType, { x: 2, y: 2, z: 5 });
  test.spawnSimulatedPlayer({ x: 10, y: 2, z: 5 }, "bait", 0 as any);

  // 断言幻术师获得隐身效果：succeedWhen 每 tick 检查区域内幻术师是否 getEffect("invisibility") 非 undefined。
  // 时序：IllusionerMirrorSpellGoal shouldExecute(冷却0+未施法+未隐身) + warmup(20) + castSpell
  //   addEffect(Invisibility,1200)。首次冷却 0 直接触发。maxTicks=800 留充裕余量（GameTest 非确定性）。
  // 幻术师查询用区域限定排除并行测试污染；type 用 "minecraft:illusioner"（带前缀）。
  // getEffect 是本次补全的脚本绑定（对齐基岩 Entity.getEffect），返回 {typeId,amplifier,duration} 或 undefined。
  test.succeedWhen(() => {
    const illusioners = test.getDimension().getEntities({
      type: "minecraft:illusioner",
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(illusioners.length > 0, "illusioner disappeared");
    const invis = (illusioners[0] as any).getEffect("invisibility");
    test.assert(invis !== undefined, "illusioner did not cast mirror (no invisibility)");
  });
}

// 幻术师对玩家施放失明法术（Illusioner.java IllusionerBlindnessSpellGoal：对目标施加 Blindness 400 ticks；
// 难度 >= Normal 才施放，且不重复对同一目标施放）。
//
// C++ 链路：IllusionerEntity registerGoals 优先级5：IllusionerBlindnessSpellGoal（失明，冷却 COOLDOWN=180）。
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true, setUnseenMemoryTicks=300)——主动选玩家为目标。
// IllusionerBlindnessSpellGoal::shouldExecute 要求：基类(未施法+冷却0) + 有攻击目标 + 目标.id != m_lastTargetId
//   + 难度 >= Normal。startExecuting 记 m_lastTargetId。castSpell 对目标 addEffect(Blindness, 400)。
//   GameTest 默认难度 Normal（IntegratedServer.cpp:142），满足 difficulty < Normal 为 false 门控。
//
// 环境选择：mediumglass。幻术师(2,2,5)+Survival 玩家(10,2,5)，水平距 8 格 < 18 FOLLOW_RANGE 可选目标。
//   距 8 格无 AvoidEntityGoal（幻术师 unlike 唤魔者不避开玩家），checkSight 射线沿 x 轴穿过空气腔不被玻璃阻挡。
//
// 判定手段：检测玩家获得 blindness 效果（getEffect("blindness") 非 undefined）。
//   失明施法是确定性生成效果（warmup 结束即对目标 addEffect），不受命中随机性影响。
//   注意：失明 goal 优先级5 高于镜像 goal 优先级4，但有"不重复对同一目标施法"门控（m_lastTargetId），
//   首次对玩家施放即可触发；镜像 goal 可能先触发使幻术师隐身但不影响失明 goal 评估（两者 flag 修复后独立）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_幻术师_ED.txt#SpellTicks（失明法术）
function illusionerBlindsPlayer(test: Test): void {
  const illusionerType = "illusioner";

  // 幻术师 (2,2,5) + Survival 玩家 (10,2,5)，水平距 8 格。幻术师选玩家为目标后施放失明法术。
  test.spawn(illusionerType, { x: 2, y: 2, z: 5 });
  test.spawnSimulatedPlayer({ x: 10, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家获得失明效果：succeedWhen 每 tick 检查区域内玩家是否 getEffect("blindness") 非 undefined。
  // 时序：NearestAttackableTarget 选目标(checkSight 通畅) + IllusionerBlindnessSpellGoal shouldExecute
  //   (冷却0+未施法+目标新+难度Normal) + warmup(20) + castSpell addEffect(Blindness,400)。
  //   maxTicks=800 留充裕余量（GameTest 非确定性）。
  // 玩家查询用区域限定排除并行测试污染；type 用 "minecraft:player"。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const blind = (players[0] as any).getEffect("blindness");
    test.assert(blind !== undefined, "illusioner did not blind player");
  });
}

// 幻术师用弓远程攻击玩家致掉血（Illusioner.java performRangedAttack：发射箭矢，速度 1.6，
// 不精确度 14-difficulty*4；wiki mob_幻术师_ED.txt：幻术师使用弓进行远程攻击）。
//
// C++ 链路：IllusionerEntity registerGoals 优先级6：RangedBowAttackGoal(0.5, 20, 20)。
//   RangedBowAttackGoal::shouldExecute 要求：主手持弓(getUseAction==Bow) + 有攻击目标。
//   构造期补弓（setEquipment MainHand BOW）确保 GameTest spawn 的幻术师持弓可远程攻击。
//   tick 中蓄力 20 ticks 后 performAttack→attackEntityWithRangedAttack 发射箭矢。
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player> 选玩家为目标。
//
// 环境选择：mediumglass。幻术师(2,2,5)+Survival 玩家(10,2,5)，水平距 8 格 < RangedBow 攻击半径 15。
//   距 8 格在攻击半径内，幻术师蓄力后射箭命中玩家。
//
// 判定手段：断言玩家 HP 下降（<20）。弓箭攻击伤害确定性命中（箭矢弹道有少量不精确度但 8 格近距易命中），
//   玩家满血 20 → 受击后 <20。用"玩家掉血"判定稳定（区别于效果判定的确定性生成）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_幻术师_ED.txt（弓远程攻击）
function illusionerShootsPlayer(test: Test): void {
  const illusionerType = "illusioner";

  // 幻术师 (2,2,5) + Survival 玩家 (10,2,5)，水平距 8 格。幻术师持弓选玩家为目标后蓄力射箭。
  test.spawn(illusionerType, { x: 2, y: 2, z: 5 });
  test.spawnSimulatedPlayer({ x: 10, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + RangedBowAttackGoal shouldExecute(持弓+有目标) +
  //   蓄力(20) + 发射箭矢 + 命中。maxTicks=1000 留充裕余量吸收蓄力时序 + 弹道随机性。
  // 玩家查询用区域限定排除并行测试污染；type 用 "minecraft:player"。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `illusioner did not damage player, hp=${(health as any).currentValue}`);
  });
}

export function registerIllusionerTests(): void {
  GameTest.register("MobBehaviorTests", "illusioner_casts_mirror", illusionerCastsMirror)
    .structureName("gametests:mediumglass")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "illusioner_blinds_player", illusionerBlindsPlayer)
    .structureName("gametests:mediumglass")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "illusioner_shoots_player", illusionerShootsPlayer)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);
}
