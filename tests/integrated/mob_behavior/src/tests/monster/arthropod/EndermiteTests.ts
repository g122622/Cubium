// 末影螨行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 末影螨主动攻击玩家致掉血（wiki world_末影螨.txt#行为：末影螨会主动攻击 16 格内的玩家）。
//
// C++ 链路：EndermiteEntity : MonsterEntity（位于 monster/arthropod/，节肢生物），registerGoals 注册：
//   targetSelector 优先级2：NearestAttackableTargetGoal<LivingEntity>(checkSight=true, chance=0,
//     谓词仅放行 PLAYER)——每 tick 评估，选最近 Survival 玩家为 attackTarget。
//   goalSelector 优先级2：MeleeAttackGoal(this, 1.0, false)——读 attackTarget，navigator->moveTo 贴近，
//     distSq <= getAttackReachSqr((0.4*2)^2+0.6=1.24, 即约1.11格) 且攻击冷却结束时造成 ATTACK_DAMAGE(2.0) 伤害。
//   MeleeAttackGoal 攻击冷却 ATTACK_COOLDOWN_TICKS=20 经 adjustedTickDelay 减半约 10 tick。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 末影螨无 RestrictSun/FleeSun goal（区别于骷髅），白天默认环境即可主动攻击，不需 batch("night")。
// 末影螨是节肢生物陆地行走（width 0.4 / height 0.3），不会飞，creeper_pit 平地寻路通畅。
// 末影螨(2,2,3)+玩家(3,2,3)，水平距 1 格，<1.11 攻击距离 → 末影螨选目标后立即近战命中。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布， MeleeAttackGoal 到冷却即 hurt），
// 伤害 2.0，玩家满血 20 → 18。与守卫者激光同属确定型，用"玩家掉血"判定稳定
// （区别于烈焰人火球散布型需用"检测投射物实体"，见 blaze-fireball-test-detection-strategy）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 谓词滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_末影螨.txt#行为（主动攻击 16 格内玩家）
function endermiteAttacksPlayer(test: Test): void {
  const endermiteType = "endermite";

  // 末影螨 (2,2,3)、Survival 玩家 (3,2,3)，水平距 1 格，同处结构 y=2 层。
  // 距 1 格 < 1.11 攻击距离，末影螨选目标后 MeleeAttackGoal 直接命中（无需寻路接近）。
  // 末影螨受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (3,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(endermiteType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标（chance=0 每 tick 评估）+ MeleeAttackGoal 攻击冷却约 10 tick。
  // 完整周期约 10-20 tick，maxTicks=400 留充裕余量（末影螨攻击前可能先游荡几 tick 选目标）。
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
      `endermite did not damage player, hp=${(health as any).currentValue}`);
  });
}

export function registerEndermiteTests(): void {
  GameTest.register("MobBehaviorTests", "endermite_attacks_player", endermiteAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);
}
