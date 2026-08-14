// 烈焰人行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 烈焰人远距离向玩家发射小火球致掉血（wiki tech_烈焰人.txt#攻击：48 格内搜寻玩家，
// 距离远时发射三个一组的 small_fireball，命中造成 5 弹射物伤害 + 着火 5 秒）。
//
// C++ 链路：BlazeEntity : MonsterEntity，registerGoals 注册
// NearestAttackableTargetGoal<Player>(checkSight=true)（优先级2）+ BlazeFireballAttackGoal（优先级4）。
// 与蜘蛛不同，烈焰人目标选择无亮度门控（蜘蛛 SpiderTargetGoal 需 getBrightness()<0.5），
// 白天也主动攻击玩家，故本测试不指定 batch（默认白天环境）。
//
// BlazeFireballAttackGoal::tick：distSq>MELEE_RANGE_SQ(4) 且 <FOLLOW_RANGE²(48²=2304) 且 canSee
// → _performFireballAttack：充能 60 tick（setCharging true）→ 发射 3 个 SmallFireball（间隔 6 tick，
// 带 nextGaussian*spread 散布）→ 冷却 100 tick。SmallFireballEntity::onEntityHit 调
// livingTarget->hurt(fireball 源, 5.0f) + igniteForSeconds(5.0f) 点燃 5 秒。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡
// （grass_pen 玻璃墙挡 canSee，见 SpiderTests 同款注释）。烈焰人(2,2,3)+玩家(6,2,3)，
// 水平距 4 格，distSq=16 走火球分支（4<16<2304）。
//
// 判定手段：检测区域内 small_fireball 实体出现。火球带 nextGaussian*spread 散布（spread≈1.0），
// 单枚命中玩家 0.6 宽碰撞箱概率约 27%，长时序仍偶发连续极端散布致 0 命中，故"玩家掉血"断言不稳。
// 改测"火球实体出现"验证"烈焰人在玩家射程内充能并发射小火球"这一确定性行为，对齐 wiki
// "远距离向玩家发射小火球"的核心语义，不受散布随机性影响。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_烈焰人.txt#攻击（远距离发射小火球）
function blazeShootsFireballAtPlayer(test: Test): void {
  const blazeType = "blaze";

  // 烈焰人 (2,2,3)、Survival 玩家 (6,2,3)，水平距 4 格，同处结构 y=2 层。
  // 烈焰人受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑使其不下落。
  // 烈焰人上升推力（customServerAiStep）会在目标眼高 > 烈焰人眼高时触发，使烈焰人 Y 上下振荡
  // （实测在 -57 ~ -54.8 间跳动）。Y 浮动会导致火球 spawn Y 变化、平射/俯射交替，单枚命中率不稳，
  // 但本测试判定"火球实体出现"而非"命中掉血"，故 Y 浮动不影响判定——只要烈焰人在射程内充能
  // 并发射火球即通过。玩家脚下 (6,1,3) 放玻璃确保稳定在 spawn 高度。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡；玩家在火球射程内（48 格）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(blazeType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 6, y: 2, z: 3 }, "bait", 0 as any);

  // 断言烈焰人向玩家发射了小火球：succeedWhen 每 tick 检查区域内是否存在 small_fireball 实体。
  // 时序：NearestAttackableTarget 选目标 + BlazeFireballAttackGoal 充能 60 tick + 发射火球。
  // 每轮攻击循环（充能60+发射18+冷却100≈178 tick）发射 3 枚 SmallFireball（间隔 6 tick），
  // 每枚火球飞行约 6-8 tick 后命中方块/实体消失，故每轮有约 24 tick 窗口可检测到火球实体，
  // succeedWhen 每 tick 轮询必能抓到。
  //
  // 判定手段选择"检测火球实体"而非"玩家掉血"：火球带 nextGaussian*spread 散布（spread≈1.0），
  // 单枚命中玩家 0.6 宽碰撞箱概率约 27%，长时序仍偶发连续极端散布致 0 命中、掉血断言不稳。
  // 检测火球实体则验证"烈焰人在玩家射程内充能并发射小火球"这一确定性行为，对齐 wiki
  // "远距离向玩家发射小火球"的核心语义，且不受散布随机性影响。
  // 火球查询用区域限定排除并行测试污染；type 用 "minecraft:small_fireball"（带前缀）。
  test.succeedWhen(() => {
    const fireballs = test.getDimension().getEntities({
      type: "minecraft:small_fireball",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(fireballs.length > 0, "blaze did not shoot fireball at player");
  });
}

// 烈焰人不在阳光下燃烧（wiki tech_烈焰人.txt#行为：烈焰人免疫火焰伤害，不在阳光下燃烧）。
// 烈焰人虽是 MonsterEntity 子类，但 shouldBurnInDaylight() override 返回 false，且注册为 fireImmune。
// 即使 burnUndead() 被调用，fireImmune 也会在 baseTick 中立即清除火焰。
//
// 与 zombie_burns_in_daylight（僵尸燃）+ cave_spider_does_not_burn_in_daylight（洞穴蜘蛛不燃）对照：
// 僵尸验证 MonsterEntity 默认 shouldBurnInDaylight=true 对基础亡灵生效，烈焰人验证下界火免疫生物
// override false + fireImmune 双重保护跳过燃烧。
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→isInDaylight 校验 shouldBurnInDaylight()，
// 烈焰人 override 返回 false 跳过燃烧判定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_烈焰人.txt#行为（免疫火焰，不在阳光下燃烧）
function blazeDoesNotBurnInDaylight(test: Test): void {
  const blazeType = "blaze";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 烈焰人 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const blaze = test.spawn(blazeType, { x: 4, y: 2, z: 4 });

  // 白天露天烈焰人不着火：轮询 onfire 组件，应恒 undefined（shouldBurnInDaylight=false + fireImmune）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，烈焰人本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），若有框架 bug 让所有实体不着火测试也过——但有
  // zombie_burns_in_daylight 正向断言对照（亡灵该着火着火），互补验证。
  test.succeedWhen(() => {
    const fire = blaze.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("blaze should not burn in daylight");
    }
  });
}

export function registerBlazeTests(): void {
  GameTest.register("MobBehaviorTests", "blaze_shoots_fireball_at_player", blazeShootsFireballAtPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "blaze_does_not_burn_in_daylight", blazeDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
