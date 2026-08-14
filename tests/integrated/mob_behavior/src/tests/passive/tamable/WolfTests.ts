// 狼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 狼受击后反击玩家（wiki tech_狼.txt#攻击:205：野生的狼会对攻击它的生物产生敌意）。
//
// C++ 链路：WolfEntity : TameableEntity（友好/中立），registerGoals（WolfEntity.cpp:894-1009）：
//   targetSelector 优先级3：HurtByTargetGoal(this, true)（alertAllies=true，受击后设
//     attackTarget=攻击者并呼叫附近同类，WolfEntity.cpp:957）。
//   goalSelector 优先级5：MeleeAttackGoal(this, 1.0, true)（speed=1.0，longMemory=true，
//     WolfEntity.cpp:932），shouldExecute 读 attackTarget，接近到攻击距离内
//     attackEntityAsMob→hurt(玩家, ATTACK_DAMAGE)。
//   注意：狼未注册 NearestAttackableTargetGoal<Player>（优先级4被注释，WolfEntity.cpp:959-963），
//   故狼不会主动攻击玩家，仅在受击后才反击——这正是本测试要验证的"受击后敌对攻击玩家"。
// registerAttributes（WolfEntity.cpp:1011-1022）：MAX_HEALTH=8.0, MOVEMENT_SPEED=0.3,
//   ATTACK_DAMAGE=2.0（野生值；C++ 与 wiki 野生4 不符，属已知偏差，但本测试断言"行为发生"
//   即玩家掉血，不依赖精确伤害数值对齐）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，MeleeAttackGoal 寻路通畅 + checkSight 射线不被阻挡。
// 狼(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。狼脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
// 玩家 tick 8 后 attackEntity(狼) 触发 HurtByTargetGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 ZombifiedPiglinTests/PolarBearTests 同款注释）。
// 狼被攻击后设 attackTarget=玩家，MeleeAttackGoal 寻路接近 3 格 + 攻击冷却后 hurt(玩家, 2.0)。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 2.0，玩家满血 20 → 18。
// 狼 MOVEMENT_SPEED=0.3 较快，3 格接近 + 攻击冷却（MeleeAttackGoal ATTACK_COOLDOWN_TICKS=20，
// 对齐 vanilla 20 tick）约需 30-50 tick。maxTicks=800 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狼.txt#攻击（受击后敌对攻击玩家）
function wolfRetaliatesWhenAttacked(test: Test): void {
  const wolfType = "wolf";

  // 狼 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // 狼脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，狼反击寻路通畅。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const wolf = test.spawn(wolfType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击狼：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 HurtByTargetGoal → 设 attackTarget=玩家。
  test.runAtTickTime(8, () => {
    player.attackEntity(wolf);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(2.0)。
  // 狼 0.3 速度接近 3 格约需 30-50 tick，maxTicks=800 留充裕余量。
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
      `wolf did not retaliate, hp=${(health as any).currentValue}`);
  });
}

// 未驯服狼主动攻击绵羊（wiki tech_狼.txt#攻击:197：野生的狼会主动攻击...绵羊。
// 历史 19w07a：狼现在会攻击狐狸；wiki#攻击:199 绵羊在未受到攻击时会忽视狼，但被狼攻击后
// 仍然会四处逃窜——故羊首击前静止，狼必中首击）。
//
// C++ 链路：WolfEntity registerGoals targetSelector 优先级5：
//   NearestAttackableTargetGoal<LivingEntity>(this, true, 0, lambda)（checkSight=true，chance=0 每 tick，
//   WolfEntity.cpp:967-977）匹配 SHEEP/RABBIT/FOX 类型。选定后 MeleeAttackGoal(优先级5) 寻路接近，
//   attackEntityAsMob→hurt(羊, 2.0)。羊满血 8，被击即 <8。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。狼(2,2,3)+羊(4,2,3)，水平距 2 格（近距，避免羊首击后
// 逃跑追不上——狼 0.3 速度 = 羊 0.25 速度，狼略快可追）。脚下放玻璃支撑。
// 选羊而非狐狸/兔作为目标：①羊未受击时忽视狼不逃跑（wiki#攻击:199），首击前静止，狼必中；
//   ②羊 8 血较耐打，被多击也不会过快消失；③羊无对狼的逃跑 AI 专精。
// 狐狸有逃跑 AI、兔 3 血易 2 击致死消失，稳定性均不如羊。
//
// 判定手段：断言羊 HP 下降（<8）或羊已死亡消失。近战确定性命中，伤害 2.0。
// 羊满血 8，被击即 <8。羊可能被多击（8 血 / 2 伤害 = 4 击致死，每击 20 tick 冷却 → 约 80+ tick）
// 致死消失，length==0 也算通过（已被狼攻击死亡）。
// 时序：NearestAttackableTargetGoal 选羊(chance=0 每 tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(2.0)。
// 狼 0.3 速度接近 2 格约需 20-30 tick，maxTicks=1000 留充裕余量吸收羊逃跑 + 非确定性。
// 羊查询用区域限定排除并行测试污染；type 用 "minecraft:sheep"。
// 注意：狼 targetSelector 优先级5 选羊无 Peaceful 门控（TameableEntity 非怪物），GameTest 默认难度不影响。
// 狼不会主动攻击玩家（优先级4 NearestAttackableTargetGoal<Player> 被注释），故此测试不需玩家。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狼.txt#攻击（主动攻击绵羊等被动生物）
function wolfAttacksSheep(test: Test): void {
  const wolfType = "wolf";
  const sheepType = "sheep";

  // 狼 (2,2,3)、羊 (4,2,3)，水平距 2 格，同处结构 y=2 层。
  // 近距 2 格确保狼选目标后快速命中（羊首击前静止不逃，首击后逃跑但狼 0.3>羊 0.25 可追）。
  // 狼脚下 (2,1,3) 放玻璃支撑；羊脚下 (4,1,3) 放玻璃。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  test.spawn(wolfType, { x: 2, y: 2, z: 3 });
  test.spawn(sheepType, { x: 4, y: 2, z: 3 });

  // 断言羊掉血或死亡：succeedWhen 每 tick 持续检查羊 HP<8 或已消失。
  // 时序：NearestAttackableTargetGoal 选羊(每 tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(2.0)。
  // 羊可能被多击杀死消失，length==0 也算通过（已受攻击死亡）。
  test.succeedWhen(() => {
    const sheeps = test.getDimension().getEntities({
      type: "minecraft:sheep",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 羊已死亡消失（被狼打死）——攻击行为生效。
    if (sheeps.length === 0) {
      return;
    }
    const health = sheeps[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "sheep has no health component");
    test.assert((health as any).currentValue < 8,
      `wolf did not attack sheep, hp=${(health as any).currentValue}`);
  });
}

export function registerWolfTests(): void {
  GameTest.register("MobBehaviorTests", "wolf_retaliates_when_attacked", wolfRetaliatesWhenAttacked)
    .structureName("gametests:creeper_pit")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "wolf_attacks_sheep", wolfAttacksSheep)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);
}
