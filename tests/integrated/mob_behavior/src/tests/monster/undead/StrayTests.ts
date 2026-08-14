// 流浪者行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 流浪者在阳光下着火（wiki tech_流浪者.txt#生物族群：流浪者是亡灵生物，会在阳光下着火）。
// 流浪者是骷髅的冰雪群系变种，分类"亡灵生物"，与普通骷髅一样白天露天燃烧。
//
// C++ 链路：StrayEntity : AbstractSkeletonEntity : MonsterEntity，继承 MonsterEntity 默认
// shouldBurnInDaylight()=true（m_burnsInDaylight=true）。MonsterEntity::tick→handleDaylightBurning
// →isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky + shouldBurnInDaylight()，
// 全部满足则 burnUndead→igniteForSeconds(8.0f) 点燃 8 秒。
//
// 历史 bug：StrayEntity 曾错误 override shouldBurnInDaylight() 返回 false（误以为流浪者不燃），
// 与原版 1.21.11 不一致。本次已删除该 override，恢复继承基类 true。此测试即验证修复后流浪者白天燃烧。
//
// 与 skeleton_burns_in_daylight（骷髅燃）+ wither_skeleton_does_not_burn_in_daylight（凋零骷髅不燃）
// 形成三方对照：同为 AbstractSkeletonEntity 子类，骷髅燃/流浪者燃/凋零骷髅不燃，
// 交叉验证 shouldBurnInDaylight 门控（基类 true + 凋零骷髅 override false）正确。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_流浪者.txt#生物族群（亡灵生物，阳光下着火）
function strayBurnsInDaylight(test: Test): void {
  const strayType = "stray";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 流浪者 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，流浪者 AI 游荡不触及围栏；整个空气腔头顶均露天无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const stray = test.spawn(strayType, { x: 4, y: 2, z: 4 });

  // 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
  // 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，
  // 流浪者无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（与 skeleton_burns 同款）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = stray.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("stray not on fire yet");
    }
  });
}

// 流浪者远程射击玩家（wiki tech_流浪者.txt#行为：流浪者使用弓箭远程攻击，射出的箭附带缓慢效果）。
// C++ 链路：StrayEntity 继承 AbstractSkeletonEntity 的 setCombatTask（持弓→RangedBowAttackGoal）+
// NearestAttackableTargetGoal<Player>(checkSight=true) 选 Survival 玩家为 attackTarget →
// RangedBowAttackGoal::tick 在射程内（15格）seenTime>=20 后蓄力 20 tick 发射 →
// attackEntityWithRangedAttack 创建 ArrowEntity（customizeArrow 钩子注入缓慢效果）→
// 箭矢命中玩家 → AbstractArrowEntity::onEntityHit 造成伤害 + ArrowEntity::onEntityHit 施加缓慢。
//
// 环境选择：必须夜晚 batch("night") + creeper_pit（开放坑）。AbstractSkeleton 注册了
// RestrictSunGoal（限制阳光）+ FleeSunGoal（逃离阳光），白天露天流浪者会优先逃离阳光而非攻击玩家
// （wiki: 流浪者像骷髅一样不主动离开阴凉处）。夜晚无阳光 FleeSun 不触发，流浪者主动选玩家射击。
// creeper_pit 开放坑无围墙阻挡 checkSight 视线 + 寻路通畅（glass_pit 玻璃挡寻路）。
//
// 判定手段：断言玩家 HP 下降（<20）。流浪者远程箭伤害约 2-3（setBaseDamageFromMob），
// 玩家初始满血 20，被 1 箭命中即掉至 <20。箭矢命中玩家即证明 RangedBowAttackGoal 远程攻击链路通。
// 不直接断言箭矢实体出现（箭矢飞行命中后消失，getEntities 轮询撞窗口不稳，见 SnowGolemTests 同款坑）；
// 不断言玩家获缓慢效果（药水效果组件未绑定 JS 不可读）。玩家掉血是远程攻击命中最直接证据。
// 玩家用 Survival（gameMode=0）：创造/旁观被 isSuitableTarget 滤掉，流浪者不选其为目标。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_流浪者.txt#行为（使用弓箭远程攻击）
function strayShootsArrowAtPlayer(test: Test): void {
  const strayType = "stray";

  // 流浪者于 (1,2,1)（一角），Survival 玩家于 (5,2,5)（对角，距 ~5.7格 < 15格射程）。
  // 流浪者在射程内锁定玩家后停止移动 + strafe 射击（RangedBowAttackGoal distSq<=15² 且 seenTime>=20）。
  // 玩家会被箭命中掉血（流浪者箭伤害 ~2-3）。玩家 HP 20，约 1 箭即掉至 <20。
  test.spawn(strayType, { x: 1, y: 2, z: 1 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：流浪者 seenTime>=20（约 tick 20）+ 蓄力 20 tick（约 tick 40）首箭 + 箭飞行几 tick命中，
  // 约 tick 45-60 玩家首伤。maxTicks=400 留寻路/锁定/蓄力 + 余量。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `stray did not shoot player, hp=${(health as any).currentValue}`);
  });
}

export function registerStrayTests(): void {
  GameTest.register("MobBehaviorTests", "stray_burns_in_daylight", strayBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false。skyAccess=true 让 MinecraftStructurePlacer 清空结构 footprint
    // 正上方至世界顶部的所有方块，制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后光照变更入队，需若干世界 tick 由 ServerWorld::tick 批量
    // 重算 skyLight 达 15。setupTicks 阶段让世界先 tick 20 次让光照稳定，再正式跑测试体。
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "stray_shoots_arrow_at_player", strayShootsArrowAtPlayer)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(400);
}
