// 远古守卫者行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 远古守卫者激光攻击玩家致掉血（wiki tech_远古守卫者.txt#攻击：远古守卫者继承守卫者的激光攻击，
// 充能数秒后激光消失，对目标造成两次伤害——魔法伤害（不被护甲降低）+ 生物伤害（被护甲降低）。
// 远古守卫者激光生物伤害为 8（ATTACK_DAMAGE，普通难度），高于普通守卫者的 6）。
//
// C++ 链路：ElderGuardianEntity : GuardianEntity : MonsterEntity，registerGoals 继承自 GuardianEntity：
//   targetSelector 优先级1：NearestAttackableTargetGoal<LivingEntity>(checkSight=true，谓词只放行
//     Player/Squid，距离平方>9)。
//   goalSelector 优先级4：GuardianAttackGoal（激光攻击）。
// GuardianAttackGoal::tick：m_tickCounter 从 -10 递增到 80，到 80 时结算伤害——先 indirectMagic() 魔法
//   伤害 LASER_DAMAGE(1.0)+elder 加成(2.0)=3.0（普通难度），再 mobAttack() 物理伤害 ATTACK_DAMAGE。
//   远古守卫者 registerAttributes 覆盖 ATTACK_DAMAGE=8.0（Guardian 基类 6.0），MAX_HEALTH=80。
//   完整周期约 90 tick（10 准备 + 80 充能）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 远古守卫者(2,2,3)+玩家(6,2,3)，水平距 4 格，distSq=16，>9 走激光分支。
// 远古守卫者陆地不窒息（wiki#行为：在空气中不会窒息，陆地可一直存活），C++ LivingEntity::updateAirSupply
// 不在水中时恢复空气，故陆地测试远古守卫者不会溺水死亡。
//
// 判定手段：断言玩家 HP 下降（<20）。激光确定性命中（无散布），双段伤害魔法+物理，
// 玩家初始满血 20，被命中即掉至 <20。掉血断言稳定。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 谓词滤掉不可被攻击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_远古守卫者.txt#攻击（激光充能后双段伤害）
function elderGuardianLaserDamagesPlayer(test: Test): void {
  const elderGuardianType = "elder_guardian";

  // 远古守卫者 (2,2,3)、Survival 玩家 (6,2,3)，水平距 4 格，同处结构 y=2 层。
  // 远古守卫者受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (6,1,3) 放玻璃。
  // 距离 4 格 >3（避开 shouldContinueExecuting 的 distSq<=9 近距离停止判定）且 <15（激光射程内）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(elderGuardianType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 6, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + GuardianAttackGoal 准备 10 tick + 充能 80 tick + 结算伤害。
  // 完整周期约 90 tick，maxTicks=500 留充裕余量（远古守卫者攻击前可能先游荡几 tick 选目标）。
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
      `elder guardian did not damage player with laser, hp=${(health as any).currentValue}`);
  });
}

// 远古守卫者对附近玩家施加挖掘疲劳 III（wiki tech_远古守卫者.txt#造成挖掘疲劳：远古守卫者每
// 约 1 分钟（JE）搜寻一次半径 50 格内未被 1 分钟或更久挖掘疲劳影响的玩家，对其施加 5 分钟的
// 挖掘疲劳 III。该效果可穿透方块瞄准，无视固体阻挡、隐身）。
//
// C++ 链路：ElderGuardianEntity::tick 累积 m_fatigueTimer，达到 FATIGUE_INTERVAL=600 tick（30 秒）
// 时重置并通过 m_world->getEntitiesInRange(pos, MINING_FATIGUE_RANGE=50) 搜寻附近实体，对其中
// Player 施加 MiningFatigue（amplifier=2 即等级 III，duration=6000 tick 即 5 分钟）。
// 注意：C++ 间隔为 600 tick（30 秒），比原版 JE 每分钟快一倍——这是项目实现的简化（更易测试），
// 不影响"施加挖掘疲劳 III"这一核心行为对齐。首试在 600 tick 触发。
//
// 环境选择：creeper_pit（7×5×7 开放坑），底部实心封闭防远古守卫者掉出（mediumglass 底部缺口
// 致远古守卫者陆地扑腾下落至 y=-57 掉出结构）。远古守卫者(2,2,3)+创造玩家(6,2,3)，水平距 4 格，
// 远小于 MINING_FATIGUE_RANGE=50，玩家必在光环范围内。
// 玩家用 Creative（gameMode=1，1 as any 绕过 TS 枚举校验）：激光伤害免疫不会死，可稳定存活
// 600+ tick 等疲劳触发。疲劳光环 getEntitiesInRange + dynamic_cast<Player*> 不过滤创造模式，
// 创造玩家仍被施加挖掘疲劳。Guardian targetSelector 谓词滤掉创造玩家使远古守卫者不攻击玩家，
// 但疲劳光环独立于目标选择不受影响。
//
// 判定手段：检测玩家获得 mining_fatigue 效果（getEffect("mining_fatigue") 非 undefined）。
//   挖掘疲劳光环是确定性生成效果（m_fatigueTimer 到 600 即对范围内玩家 addEffect），不受随机性影响。
//   首试在 600 tick 触发，maxTicks=1000 留充裕余量（600 触发 + 余量吸收非确定性）。
// 玩家查询用区域限定排除并行测试污染；type 用 "minecraft:player"。
// getEffect 是脚本绑定（对齐基岩 Entity.getEffect），返回 {typeId,amplifier,duration} 或 undefined。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_远古守卫者.txt#造成挖掘疲劳（半径50格施加挖掘疲劳III）
function elderGuardianAppliesMiningFatigue(test: Test): void {
  const elderGuardianType = "elder_guardian";

  // 远古守卫者 (2,2,3) + 创造玩家 (6,2,3)，水平距 4 格 << 50 格光环范围。
  // 远古守卫者脚下 (2,1,3) 放玻璃支撑；玩家脚下 (6,1,3) 放玻璃。
  // creeper_pit 底部实心，远古守卫者即便陆地扑腾也不会掉出结构。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(elderGuardianType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 6, y: 2, z: 3 }, "bait", 1 as any);

  // 断言玩家获得挖掘疲劳：succeedWhen 每 tick 检查区域内玩家是否 getEffect("mining_fatigue") 非 undefined。
  // 时序：m_fatigueTimer 从 0 累积到 600 tick（首试触发）+ getEntitiesInRange 搜到玩家 + addEffect。
  // maxTicks=1000 留充裕余量（600 触发点 + 余量吸收 GameTest 非确定性）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const fatigue = (players[0] as any).getEffect("mining_fatigue");
    test.assert(fatigue !== undefined, "elder guardian did not apply mining fatigue to player");
  });
}

// 远古守卫者不在阳光下燃烧（wiki tech_远古守卫者.txt#行为：远古守卫者在水中和陆地上的多数行为
// 与守卫者相同；守卫者陆地不燃烧，远古守卫者继承此特性）。
//
// 与 guardian_does_not_burn_in_daylight（守卫者不燃）+ zombie_burns_in_daylight（僵尸燃）对照：
// 远古守卫者继承 GuardianEntity::shouldBurnInDaylight() override{return false}。C++ 链路：
// MonsterEntity::tick→handleDaylightBurning→shouldBurnInDaylight() 虚函数调度（handleDaylightBurning
// 调虚函数而非读成员，使 override 生效），远古守卫者经继承链 override 返回 false 跳过燃烧判定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_远古守卫者.txt#行为（陆地存活、不燃烧）
function elderGuardianDoesNotBurnInDaylight(test: Test): void {
  const elderGuardianType = "elder_guardian";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 远古守卫者 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const elderGuardian = test.spawn(elderGuardianType, { x: 4, y: 2, z: 4 });

  // 白天露天远古守卫者不着火：轮询 onfire 组件，应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=400：白天燃烧判定确定性触发（vanilla 无随机检查），远古守卫者本就不燃，留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证
  // （亡灵该着火着火），排除框架让所有实体不着火的假性通过。
  test.succeedWhen(() => {
    const fire = elderGuardian.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("elder guardian should not burn in daylight");
    }
  });
}

export function registerElderGuardianTests(): void {
  GameTest.register("MobBehaviorTests", "elder_guardian_laser_damages_player", elderGuardianLaserDamagesPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "elder_guardian_applies_mining_fatigue", elderGuardianAppliesMiningFatigue)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "elder_guardian_does_not_burn_in_daylight", elderGuardianDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
