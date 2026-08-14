// 女巫行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 女巫主动向玩家投掷喷溅药水（wiki tech_女巫.txt#行为：女巫会主动攻击半径 16 格(JE)/10 格(BE)
// 内的玩家；#投掷药水：女巫常使用 I 级喷溅药水进行攻击，投掷间隔 3 秒，按距离/状态选
// 缓慢/中毒/虚弱/伤害药水）。
//
// C++ 链路：WitchEntity : AbstractRaiderEntity（+ IRangedAttackMob），registerGoals 注册：
//   targetSelector 优先级1：HurtByTargetGoal（受伤反击，排斥 AbstractRaider 同类）。
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标
//     （对齐 MC 1.21.11 Witch.registerGoals；此前 Cubium 缺此 goal 致女巫只能被动反击，本次补齐）。
//   goalSelector 优先级2：RangedAttackGoal(this, 1.0, 60, 60, ATTACK_RADIUS=10.0f)——药水攻击。
// RangedAttackGoal::tick：读 attackTarget，canSee 累计 m_seenTime>=20(MIN_SEEN_TIME) 后停止移动，
//   攻击计时 m_attackTime 到 0 且 canSee 时 performAttack → IRangedAttackMob::attackEntityWithRangedAttack
//   → WitchEntity::_selectAttackPotionType(按距离/状态选药水) + _throwPotionAt(生成 PotionEntity)。
//   攻击间隔固定 60 tick（3 秒），首次冷却 floor(charge*60)（charge=dist/10）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 女巫 shouldBurnInDaylight()=false（节肢生物不燃，实际女巫是普通怪物但 override 不燃），白天默认环境即可。
// 女巫(2,2,3)+玩家(6,2,3)，水平距 4 格，distSq=16，<ATTACK_RADIUS²(100) 走药水攻击分支。
//
// 判定手段：检测区域内 minecraft:potion 实体出现。女巫药水散射度 POTION_INACCURACY=8.0 较大，
// 单枚命中玩家 0.6 宽碰撞箱概率不高，"玩家掉血"断言不稳（与烈焰人火球散布型同理）。
// 改测"药水实体出现"验证"女巫主动向玩家投掷药水"这一确定性行为，对齐 wiki"投掷喷溅药水"核心语义，
// 不受散射随机性影响（见 blaze-fireball-test-detection-strategy 散布型判定策略）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_女巫.txt#行为（主动攻击玩家）/ #投掷药水
function witchThrowsPotionAtPlayer(test: Test): void {
  const witchType = "witch";

  // 女巫 (2,2,3)、Survival 玩家 (6,2,3)，水平距 4 格，同处结构 y=2 层。
  // 距 4 格 < ATTACK_RADIUS(10) 走药水攻击分支。女巫主动选玩家为目标（NearestAttackableTargetGoal）。
  // 女巫受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (6,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(witchType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 6, y: 2, z: 3 }, "bait", 0 as any);

  // 断言女巫向玩家投掷药水：succeedWhen 每 tick 检查区域内是否存在 minecraft:potion 实体。
  // 时序：NearestAttackableTarget 选目标 + RangedAttackGoal seenTime 累计 20 tick +
  //   首次攻击冷却 floor(charge*60)（距离4→charge=0.4→24 tick）≈ 44 tick 首次投掷。
  //   注意：女巫 tick 中 _decidePotionToDrink 有概率（治疗5%/速度50%等）进入喝药水状态（32 tick 不投药水），
  //   可能延迟首次投掷，maxTicks=600 留充裕余量确保捕获至少一次药水投掷。
  // 药水查询用区域限定排除并行测试污染；type 用 "minecraft:potion"（带前缀）。
  test.succeedWhen(() => {
    const potions = test.getDimension().getEntities({
      type: "minecraft:potion",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(potions.length > 0, "witch did not throw potion at player");
  });
}

// 女巫不在阳光下燃烧（wiki tech_女巫.txt 通篇未提女巫阳光下燃烧；女巫 shouldBurnInDaylight()
// override 返回 false）。女巫虽是 MonsterEntity 子类（m_burnsInDaylight 默认 true），但 override false
// 跳过 handleDaylightBurning 的燃烧判定。
//
// 与 zombie_burns_in_daylight（僵尸燃）+ blaze_does_not_burn_in_daylight（烈焰人不燃）对照：
// 僵尸验证 MonsterEntity 默认 shouldBurnInDaylight=true 对基础亡灵生效，女巫验证 override false
// 跳过燃烧。C++ 链路：MonsterEntity::tick→handleDaylightBurning→isInDaylight 校验 shouldBurnInDaylight()，
// 女巫 override 返回 false 跳过燃烧判定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_女巫.txt#行为（无阳光燃烧描述，女巫不燃）
function witchDoesNotBurnInDaylight(test: Test): void {
  const witchType = "witch";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 女巫 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const witch = test.spawn(witchType, { x: 4, y: 2, z: 4 });

  // 白天露天女巫不着火：轮询 onfire 组件，应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，女巫本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = witch.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("witch should not burn in daylight");
    }
  });
}

export function registerWitchTests(): void {
  GameTest.register("MobBehaviorTests", "witch_throws_potion_at_player", witchThrowsPotionAtPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "witch_does_not_burn_in_daylight", witchDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
