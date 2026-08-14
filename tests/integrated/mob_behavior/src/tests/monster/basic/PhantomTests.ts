// 幻翼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// grass_pen 结构尺寸（9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 幻翼在阳光下着火：白天露天环境（canSeeSky=true 且亮度>0.5）下，亡灵生物每 tick 有概率
// 被点燃 8 秒。C++ 链路：PhantomEntity::tick → FlyingEntity::tick → burnUndead() →
// MonsterEntity::handleDaylightBurning → igniteForSeconds(8.0f)；isInDaylight 校验
// isDaytime + brightness>0.5 + !isWet + canSeeSky。幻翼 getCreatureAttribute()=Undead，
// 默认 m_burnsInDaylight=true（MonsterEntity 基类），无 isImmuneToFire，故露天白天必燃。
//
// 与骷髅不同（wiki tech_幻翼.txt：幻翼会在阳光下着火，但幻翼不会寻找阴凉处、水域或细雪来避免
// 着火），幻翼无 FleeSun/RestrictSun goal，着火后持续燃烧不逃避，故断言 onfire 必然出现。
//
// JS 侧读火焰状态：Entity.getComponent("minecraft:onfire") 未着火返回 undefined，
// 着火返回 OnFireComponent（对齐基岩 OnFireComponent 语义"组件存在即着火"）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_幻翼.txt（阳光下着火，亡灵生物）
function phantomBurnsInDaylight(test: Test): void {
  const phantomType = "phantom";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙围栏+内部空气，y=4 全 air 露天。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 幻翼 spawn 于 helper-y=2（结构内 y=1 空气腔），头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true。
  // 中心位置（4,2,4）远离玻璃墙，幻翼环绕飞行不会触及围栏；整个空气腔头顶均露天，无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自 @minecraft/server-gametest 内嵌的
  // @minecraft/server，与顶层包的 Entity 类型因 Dimension 属性差异不兼容，显式标注会触发 TS2322。
  const phantom = test.spawn(phantomType, { x: 4, y: 2, z: 4 });

  // 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
  // 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影、
  // 幻翼无处可躲（不逃离阳光），着火后持续燃烧。maxTicks=500 留充足余量（期望 25 tick + 余量）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = phantom.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("phantom not on fire yet");
    }
  });
}

// 幻翼俯冲攻击玩家（wiki tech_幻翼.txt：幻翼只与玩家敌对；发现玩家后在距玩家水平 15-25 格、
// 垂直 24-35 格范围徘徊，每 8-11 秒俯冲攻击）。
//
// C++ 链路（修复三处非 vanilla 偏差后攻击链贯通）：
//   targetSelector PhantomAttackPlayerTargetGoal(优先级1) shouldExecute 每 60 tick 搜玩家
//   （SEARCH_RANGE=64 球内，过滤 spectator/creative——对齐 TargetingConditions.forCombat；
//   旧实现误加的 SEA_LEVEL 海平面过滤已移除，否则 GameTest 玩家 world y≈-57<63 全被滤掉）→
//   setAttackTarget(玩家)。
//   goalSelector PhantomPickAttackGoal(优先级1, 无 flag) startExecuting 设 phase=CIRCLE +
//   anchor=玩家上方20-40格（clamp 海平面以上，对齐 vanilla setAnchorAboveTarget）→
//   PhantomOrbitPointGoal(Move,优先级3) CIRCLE 阶段环绕 anchor 飞行 →
//   PickAttack.tick 10 tick 后切 phase=SWOOP →
//   PhantomSweepAttackGoal(Move,优先级2) 抢占 OrbitPoint，setOrbitOffset=玩家眼睛位置俯冲 →
//   碰撞箱相交时 attackEntityAsMob(玩家, ATTACK_DAMAGE=6.0)。
//
// 三处修复（2026-08-14，详见 PhantomGoals.cpp 注释）：
//   1. _findAttackablePlayer 移除非 vanilla 的 SEA_LEVEL 过滤（阻塞选目标）。
//   2. PhantomAttackPlayerTargetGoal::shouldContinueExecuting 移除非 vanilla 的距离检查
//      （vanilla DEFAULT range=-1 不检查距离；旧检查致幻翼飞高后误丢目标循环无法俯冲）。
//   3. PhantomPickAttackGoal 移除非 vanilla 的 GoalFlag::Move（vanilla AttackStrategyGoal 无 flag；
//      旧 Move flag 长期占据 m_flagGoals[Move] 压制 SweepAttackGoal 致攻击链断裂，幻翼永不俯冲）。
//
// 环境选择：必须夜晚 batch("night")（白天幻翼阳光燃烧致死无法攻击）+ grass_pen + skyAccess(true)。
// skyAccess 清空结构上方 worldgen 方块制造露天列：幻翼 CIRCLE 阶段环绕 anchor（y≈64，被 setAnchor
// clamp 到海平面以上）需飞出结构顶部（grass_pen 仅 5 格高），上方必须无方块阻挡飞行。
//
// 判定手段：断言玩家 HP 下降（<20）。幻翼俯冲碰撞 attackEntityAsMob 伤害 6.0（ATTACK_DAMAGE），
// 玩家初始满血 20，被 1 次俯冲命中即掉至 14<20。玩家用 Survival（gameMode=0，0 as any 绕过 TS
// 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击）。
// 时序：target goal 首次搜目标（初始 m_tickDelay=20，约 tick 20）+ CIRCLE 10 tick + SWOOP 俯冲
// （幻翼从 CIRCLE 位置俯冲向玩家，距离短则数十 tick）+ 碰撞命中。maxTicks=1000 留充裕余量吸收
// 攻击链非确定性（环绕/俯冲/转向时机随机）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_幻翼.txt（俯冲攻击玩家）
function phantomAttacksPlayer(test: Test): void {
  const phantomType = "phantom";

  // 幻翼与 Survival 玩家同位于中心 (4,2,4)：幻翼 spawn 即在玩家旁，target goal 首次扫描即可锁定。
  // 玩家不动（SimulatedPlayer 默认静止），幻翼环绕后俯冲回玩家位置命中。
  // grass_pen 中心 (4,2,4) 为空气腔，helper-y=2→结构内 y=1 空气，脚踩 y=0 grass_block。
  test.spawn(phantomType, { x: 4, y: 2, z: 4 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20（被幻翼俯冲命中）。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `phantom did not attack player, hp=${(health as any).currentValue}`);
  });
}

// 幻翼俯冲时被猫驱赶停止攻击（wiki tech_幻翼.txt：幻翼会尝试待在猫切比雪夫距离 16 格以外；
// 猫会向正在攻击玩家的幻翼发出嘶嘶声）。
//
// C++ 链路：PhantomSweepAttackGoal::shouldContinueExecuting 每 20 tick 调 _checkForCats()
// 搜索幻翼碰撞箱 grow(16.0) 范围内的 Cat（对齐 vanilla PhantomSweepAttackGoal.canContinueToUse
// 的 getEntitiesOfClass(Cat.class, getBoundingBox().inflate(16.0))），检测到猫则
// isScaredOfCat=true → shouldContinueExecuting 返回 false → resetTask 切回 CIRCLE，
// 幻翼停止俯冲攻击。注意：猫仅在 SWOOP（俯冲）阶段驱赶幻翼，CIRCLE（环绕）阶段无影响；
// 且幻翼不会主动飞离猫，仅停止俯冲（对齐 vanilla，区别于 wiki 字面"飞离"）。
//
// 与 phantom_attacks_player 形成正反例对照：无猫时幻翼俯冲命中玩家（HP<20），
// 有猫时幻翼停止俯冲（玩家 HP 保持 20）。这交叉验证 _checkForCats 的猫驱赶逻辑。
//
// 环境选择：夜晚 batch("night") + grass_pen + skyAccess（与 phantom_attacks_player 同环境，
// 仅多一只猫）。猫放玩家旁，确保幻翼俯冲至玩家时进入猫的 16 格检测范围。
//
// 判定手段：断言玩家 HP 保持 20（未被攻击）。猫阻止幻翼俯冲，玩家不受伤。
// 注意：此为依赖 phantom_attacks_player 攻击链可命中（无猫时）的反向断言——若攻击链本身
// 未触发（幻翼未锁定/未俯冲），玩家 HP 也保持 20，会假通过。故本测试须与 phantom_attacks_player
// 配对解读：后者证明攻击链可命中，前者证明猫能阻止该命中。
// maxTicks 与 phantom_attacks_player 一致（1000），确保若有猫时幻翼仍试图俯冲足够长时间，
// 验证猫的阻止效果（而非因时间不足未命中）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_幻翼.txt（猫驱赶幻翼）
function phantomAvoidsCatWhenAttacking(test: Test): void {
  const phantomType = "phantom";
  const catType = "cat";

  // 幻翼、Survival 玩家、猫同位于中心区域：幻翼 (4,2,4)、玩家 (4,2,4) 同位锁定目标，
  // 猫 (3,2,4) 紧邻玩家（距 1 格），幻翼俯冲至玩家时距猫 <16 格必触发 _checkForCats 驱赶。
  test.spawn(phantomType, { x: 4, y: 2, z: 4 });
  test.spawn(catType, { x: 3, y: 2, z: 4 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "bait", 0 as any);

  // 断言玩家未掉血：succeedWhen 每 tick 持续检查玩家 HP==20（猫阻止幻翼俯冲）。
  // 用 runAtTickTime 在 tick 200（远超攻击链命中时机）后断言玩家仍满血再 succeed，
  // 确保幻翼有充足时间锁定+俯冲，但因猫阻止未命中——证明猫驱赶生效而非时间不足。
  // 200 tick 内无猫场景幻翼应已命中玩家（见 phantom_attacks_player 时序），故 200 tick 后
  // 玩家仍 20 血即可判定猫阻止了攻击。
  test.runAtTickTime(200, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue >= 20,
      `phantom attacked player despite cat, hp=${(health as any).currentValue}`);
    test.succeed();
  });
}

export function registerPhantomTests(): void {
  GameTest.register("MobBehaviorTests", "phantom_burns_in_daylight", phantomBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false（亡灵阳光燃烧测试稳定失败的根因）。skyAccess=true 让
    // MinecraftStructurePlacer 清空结构 footprint 正上方至世界顶部的所有方块，制造露天列使
    // canSeeSky=true。详见 SkeletonTests.skeleton_burns_in_daylight 同款注释。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后光照变更入队 m_lightQueue，需若干世界 tick 由
    // ServerWorld::tick 的 drainAndProcess 批量重算 skyLight 达 15。setupTicks 阶段（负 tickCount）
    // 让世界先 tick 20 次让光照稳定，再正式跑测试体，避免首 tick canSeeSky 仍为 false。
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "phantom_attacks_player", phantomAttacksPlayer)
    .batch("night")
    .structureName("gametests:grass_pen")
    // skyAccess(true)：幻翼 CIRCLE 阶段环绕 anchor（clamp 到海平面 y≈64）需飞出 grass_pen 顶部
    // （仅 5 格高），上方 worldgen 方块会阻挡飞行。清空上方制造露天列供幻翼环绕飞行。
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "phantom_avoids_cat_when_attacking", phantomAvoidsCatWhenAttacking)
    .batch("night")
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(1000);
}
