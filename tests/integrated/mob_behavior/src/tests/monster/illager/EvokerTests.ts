// 唤魔者行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// mediumglass 结构尺寸（12×9×11），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const MED_FROM = { x: 0, y: 0, z: 0 };
const MED_VOLUME = { x: 12, y: 9, z: 11 };

// 唤魔者施法召唤尖牙攻击玩家（wiki tech_唤魔者.txt#尖牙攻击：唤魔者对 12 格内目标施法，
// 远距离生成 16 个直线尖牙，尖牙造成 6 点魔法伤害；#行为：主动攻击 12 格内玩家）。
//
// C++ 链路：EvokerEntity : SpellcastingIllagerEntity : AbstractRaiderEntity，registerGoals 注册：
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true, setUnseenMemoryTicks=300)
//     ——主动选玩家为目标。
//   goalSelector 优先级5：EvokerAttackSpellGoal（尖牙攻击，冷却 FANGS_COOLDOWN=100）。
// EvokerAttackSpellGoal 继承 EvokerSpellGoal，shouldExecute 要求：有攻击目标 + 冷却结束 + 未在施法。
//   startCasting 进入 warmup(getCastWarmupTime=0)，warmup 结束 castSpell→castFangsAttack 生成
//   EvokerFangsEntity。远距离(distSq>=9)生成16个直线尖牙；近距离(<9)生成两圈13个尖牙。
//
// 环境选择：mediumglass（12×9×11 玻璃盒空腔）。结构内 x∈[2,10]/z∈[1,9] 为空气腔，四周 x=1/x=11、
// z=0/z=10 是玻璃墙，y=0 圆石地板。结构放置有 +1 抬升：helper 坐标对应结构内同号坐标，实体需站在
// 地板上方故 helper-y=2（结构内 y=1 空气腔，地板 y=0 圆石）。
//   唤魔者 helper(2,2,5)+玩家 helper(10,2,5)：与 CowTests 同款坐标布局（已验证空腔可用）。
//   本测试早期版本误把唤魔者放 helper(1,3,5)，helper-x=1 落结构内 x=1 玻璃墙，唤魔者卡墙致
//   canSee 射线被自身所处玻璃阻挡，NearestAttackableTarget(checkSight) 选不到目标，永不施法。
//
// 唤魔者 setBurnsInDaylight(false)（构造期关闭，对齐原版灾厄村民不燃），白天默认环境即可施法。
// 距离设计 8 格：唤魔者有 AvoidEntityGoal(玩家,8格,优先级2)——玩家8格内触发逃跑占 Move flag。
//   但 EvokerSpellGoal(尖牙/召唤施法,优先级5)经 flag 修复后仅占 Look（对齐原版 SpellcasterUseSpellGoal
//   不占 flag），与 AvoidEntityGoal 不再互斥，两者并行：唤魔者一边 flee 一边施法 warmup，warmup 结束
//   仍生成尖牙。施法期间停步由 EvokerCastingSpellGoal(优先级1, Move+Look) 接管，优先级1 高于
//   AvoidEntity(2) 阻止 flee 打断施法。距 8 格 < 12 FOLLOW_RANGE 可选目标，唤魔者正常施法尖牙攻击。
//
// 判定手段：检测区域内 minecraft:evoker_fangs 实体出现。尖牙施法是确定性生成实体（warmup 结束即生成），
// 不受伤害命中随机性影响（区别于"玩家掉血"判定）。对齐 wiki"召唤尖牙"核心语义。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_唤魔者.txt#尖牙攻击（召唤尖牙攻击玩家）
function evokerCastsFangs(test: Test): void {
  const evokerType = "evoker";

  // 唤魔者 (2,2,5)、Survival 玩家 (10,2,5)，水平距 8 格，同处结构 y=2 空气腔（地板 y=1 圆石支撑）。
  // 距 8 格 < 12 FOLLOW_RANGE 选目标后施法尖牙攻击；AvoidEntity 8 格触发但不阻塞施法（flag 修复后并行）。
  // mediumglass 空气腔内 checkSight 射线沿 x 轴穿过空气，不触玻璃墙。
  // 坐标布局同 CowTests（已验证空腔可用），避免唤魔者卡玻璃墙。
  test.spawn(evokerType, { x: 2, y: 2, z: 5 });
  test.spawnSimulatedPlayer({ x: 10, y: 2, z: 5 }, "bait", 0 as any);

  // 断言唤魔者施法召唤尖牙：succeedWhen 每 tick 检查区域内是否存在 minecraft:evoker_fangs 实体。
  // 时序：NearestAttackableTarget 选目标(checkSight 通畅) + EvokerAttackSpellGoal shouldExecute
  //   (冷却0+未施法) + warmup(0) + castFangsAttack 生成尖牙。首次施法冷却为0可直接触发。
  // 注意：EvokerSummonSpellGoal(优先级4) 高于 AttackSpellGoal(5)，但召唤 goal 有 rng.nextInt(8)+1
  //   >vexCount 概率门控，尖牙 goal 用基类 shouldExecute 无此门控，两者按 flag 独立评估交替触发。
  //   maxTicks=800 留充裕余量确保至少一次尖牙施法触发（GameTest 非确定性）。
  // 尖牙查询用区域限定排除并行测试污染；type 用 "minecraft:evoker_fangs"（带前缀）。
  test.succeedWhen(() => {
    const fangs = test.getDimension().getEntities({
      type: "minecraft:evoker_fangs",
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(fangs.length > 0, "evoker did not cast fangs at player");
  });
}

// 唤魔者不在阳光下燃烧（wiki tech_唤魔者.txt 通篇未提阳光燃烧；唤魔者是灾厄村民非亡灵）。
//
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→if(m_burnsInDaylight) burnUndead()。
// 唤魔者构造时 setBurnsInDaylight(false) 关闭日光燃烧（对齐原版，本次补齐）。
// 与 zombie_burns_in_daylight（僵尸燃）+ ravager_does_not_burn_in_daylight（劫掠兽不燃）对照。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_唤魔者.txt#行为（无阳光燃烧描述，灾厄村民不燃）
function evokerDoesNotBurnInDaylight(test: Test): void {
  const evokerType = "evoker";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 唤魔者 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  const evoker = test.spawn(evokerType, { x: 4, y: 2, z: 4 });

  // 白天露天唤魔者不着火：轮询 onfire 组件，应恒 undefined（setBurnsInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，唤魔者本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = evoker.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("evoker should not burn in daylight");
    }
  });
}

export function registerEvokerTests(): void {
  GameTest.register("MobBehaviorTests", "evoker_casts_fangs", evokerCastsFangs)
    .structureName("gametests:mediumglass")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "evoker_does_not_burn_in_daylight", evokerDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
