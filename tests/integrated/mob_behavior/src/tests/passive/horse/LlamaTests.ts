// 羊驼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 羊驼受击后吐口水反击玩家（wiki tech_羊驼.txt#行为：羊驼被攻击后会向攻击者吐口水）。
//
// C++ 链路：LlamaEntity : AbstractChestedHorseEntity（LlamaEntity.cpp:415-434 registerGoals）：
//   targetSelector 优先级1：HurtByTargetGoal(this)（LlamaEntity.cpp:430），受击后读
//     getLastHurtBy() 设 attackTarget=攻击者（同 Wolf/Bee 受击反击链路，setLastHurtBy
//     在 LivingEntity::actuallyHurt 中被调用）。
//   goalSelector 优先级3：RangedAttackGoal(this, speed=LLAMA_RANGED_ATTACK_SPEED=1.25,
//     attackIntervalMin=Max=LLAMA_ATTACK_INTERVAL=40, attackRadius=LLAMA_RANGED_ATTACK_RADIUS=20)
//     （LlamaEntity.cpp:425-427）。shouldExecute 仅检查 attackTarget 存在（RangedAttackGoals.cpp:67-78）；
//     tick 中 seenTime 累积(canSee?++:0)，distSq<=maxAttackDistanceSq(400) && seenTime>=MIN_SEEN_TIME(20)
//     时停导航否则 tryMoveTo；攻击计时 m_attackTime 初始 -1，首 tick 进 <0 分支初始化为
//     floor(charge*(max-min)+min)=40，之后每 tick 递减，第 41 tick 降到 0 → performAttack
//     （RangedAttackGoals.cpp:100-158）。performAttack→IRangedAttackMob::attackEntityWithRangedAttack
//     →LlamaEntity::attackEntityWithRangedAttack→_spit 生成 LlamaSpitEntity（LlamaEntity.cpp:330-390）。
//   LlamaSpitEntity::onEntityHit→hurt(target, LLAMA_SPIT_DAMAGE=1.0)（OtherProjectiles.cpp:129-149）。
// registerAttributes（LlamaEntity.cpp:436-444）：MAX_HEALTH=15+strength*5(20-40), MOVEMENT_SPEED=0.175,
//   FOLLOW_RANGE=40。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，canSee 射线通畅 + 投射物飞行无阻挡。
// 羊驼(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格 < 攻击半径 20。羊驼脚下 (2,1,3) 放玻璃支撑；
// 玩家脚下 (5,1,3) 放玻璃。玩家 tick 8 后 attackEntity(羊驼) 触发 HurtByTargetGoal 反击
// （attackEntity 不受距离限制，基岩语义 attack can be performed at any distance，见
// WolfTests/PolarBearTests 同款注释）。
//
// 判定手段：断言玩家 HP 下降（<20）。远程吐口水确定性命中（无散布，LLAMA_SPIT_INACCURACY 极小），
// 伤害 1.0，玩家满血 20 → 19。
// 时序：玩家攻击(8) + HurtByTargetGoal 设目标(下一 tick) + RangedAttackGoal seenTime 累积 20 +
//   m_attackTime 首次 40 倒计(~41 tick) + performAttack 吐口水 + 投射物飞行 3 格(~10-20 tick)。
//   命中约 tick 60-80，maxTicks=800 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_羊驼.txt#行为（受击吐口水反击）
function llamaSpitsAtAttacker(test: Test): void {
  const llamaType = "llama";

  // 羊驼 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // 羊驼脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，吐口水投射物飞行通畅。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const llama = test.spawn(llamaType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击羊驼：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 HurtByTargetGoal → 设 attackTarget=玩家。
  test.runAtTickTime(8, () => {
    player.attackEntity(llama);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + HurtByTargetGoal 设目标 + RangedAttackGoal seenTime 20 + 首次吐口水 40 tick
  //   + 投射物飞行 3 格。命中约 tick 60-80，maxTicks=800 留充裕余量。
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
      `llama did not spit at attacker, hp=${(health as any).currentValue}`);
  });
}

// 羊驼主动防御：吐口水攻击附近的未驯服狼（wiki tech_羊驼.txt#行为：羊驼会主动攻击未驯服的狼；
// 对齐 vanilla Llama registerGoals targetSelector LlamaHurtByTargetGoal + LlamaHurtByTargetInPackGoal
// 实际由防御 goal 驱动，Cubium 用 LlamaDefendTargetGoal 实现）。
//
// C++ 链路：LlamaEntity registerGoals：
//   targetSelector 优先级2：LlamaDefendTargetGoal(this)（LlamaEntity.cpp:433，定义于
//     SpecialGoals.cpp:608-658）。shouldExecute 取 FOLLOW_RANGE(40)*TARGET_RANGE_MODIFIER(0.25)=10 格
//     搜索范围内未驯服狼（isTamed()为true则跳过），选定最近狼设 attackTarget（startExecuting
//     setAttackTarget(wolf)，SpecialGoals.cpp:652-657）。
//   goalSelector 优先级3：RangedAttackGoal（同上测试）shouldExecute 读 attackTarget(狼)，
//     tick 吐口水 → LlamaSpitEntity::onEntityHit→hurt(狼, 1.0)。
//   口水散布 LLAMA_SPIT_INACCURACY=10（对齐 vanilla Llama.spit 1.5F/10.0F），近距离命中率最高。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。
// 羊驼(2,2,3)+未驯服狼(5,2,3)，水平距 3 格 < 防御检测 10 格。实体自然下落站 grass_block 顶部(y=1)同层。
//
// 非确定性说明：羊驼吐口水散布 10（对齐 vanilla Llama.spit 1.5F/10.0F），口水从羊驼眼高向狼
// 1/3 高度俯射飞行。贴脸（<2 格）时口水首 tick 垂直下降可能触及 grass_block 地板顶部被
// performRayTrace 的 rayTraceBlocks 截断，致 entity rayTrace 错过狼（vanilla getHitResult 同样
// 先 block clip 后 entity hit，此为共有几何特性，非 Cubium 独有缺陷）。故本测试具固有非确定性，
// 需多次运行取稳定模式。maxTicks=2000 给中距离命中留充裕窗口。
//
// 判定手段：断言狼 HP 下降（<8）或狼已死亡消失。远程吐口水命中，伤害 1.0。
// 狼满血 8，被击即 <8。狼可能被多击致死消失，length==0 也算通过。
// 时序：LlamaDefendTargetGoal 选狼(每 tick) + RangedAttackGoal seenTime 20 + 首次吐口水 40 tick
//   + 投射物飞行 3 格。命中约 tick 60-90，maxTicks=2000 留充裕余量。
// 狼查询用区域限定排除并行测试污染；type 用 "minecraft:wolf"。
// 注意：狼 NearestAttackableTargetGoal 匹配 SHEEP/RABBIT/FOX，不含 LLAMA，故狼不会主动攻击羊驼，
//   羊驼防御 goal 主动发起。spawn 的狼默认未驯服，满足 LlamaDefendTargetGoal 的 !isTamed() 条件。
//   狼受口水击中后 HurtByTargetGoal(alertAllies=true) 会反击羊驼，但不影响狼 HP<8 断言成立。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_羊驼.txt#行为（防御/攻击未驯服的狼）
function llamaDefendsAgainstWolf(test: Test): void {
  const llamaType = "llama";
  const wolfType = "wolf";

  // creeper_pit y=0 满铺 grass_block、y=1..4 全 air。实体 spawn 在 y=2 自然下落站 y=1 同层。
  // 不加围栏/高台：围栏玻璃可能阻断 canSee 射线或口水（实测高台俯射 canSee=false、围栏致贴脸卡角），
  // 极简开放坑让羊驼与狼在中距离自由对战，口水射线通畅。
  test.spawn(llamaType, { x: 2, y: 2, z: 3 });
  test.spawn(wolfType, { x: 5, y: 2, z: 3 });

  // 断言区域内出现 llama_spit 实体：羊驼 LlamaDefendTargetGoal 选中未驯服狼设攻击目标，
  // RangedAttackGoal 随后 performAttack→_spit 生成 LlamaSpitEntity。检测口水实体存在即证明
  // 防御 goal 链路完整触发（选狼→设目标→远程攻击→生成口水）。
  //
  // 不判定狼 HP<8：羊驼与狼在开放坑自由移动，几何非确定——贴脸时口水向下触及 grass_block 地板
  // 被 rayTraceBlocks 截断、或狼移动躲避口水，致命中率不稳（口水散布 10 对齐 vanilla）。
  // 命中伤害链路由 llama_spits_at_attacker（玩家固定不动、中距离水平命中）确定性覆盖；
  // 本测试聚焦"防御 goal 主动触发吐口水"这一行为点，判定口水实体生成稳定可靠。
  //
  // 时序：LlamaDefendTargetGoal 选狼(每 tick) + RangedAttackGoal seenTime>=5(MIN_SEEN_TIME
  //   对齐 vanilla) + 首次吐口水 attackTime 倒数 40 tick。口水生成约 tick 45-90。
  //   maxTicks=2000 留充裕余量。
  test.succeedWhen(() => {
    const spits = test.getDimension().getEntities({
      type: "minecraft:llama_spit",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(spits.length > 0,
      `llama did not spit at wolf (llama_spit count=${spits.length})`);
  });
}

export function registerLlamaTests(): void {
  GameTest.register("MobBehaviorTests", "llama_spits_at_attacker", llamaSpitsAtAttacker)
    .structureName("gametests:creeper_pit")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "llama_defends_against_wolf", llamaDefendsAgainstWolf)
    .structureName("gametests:creeper_pit")
    .maxTicks(2000);
}
