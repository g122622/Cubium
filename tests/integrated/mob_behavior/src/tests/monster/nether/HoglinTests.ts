// 疣猪兽（Hoglin）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 成年疣猪兽近战攻击玩家（wiki mob_疣猪兽_ED.txt#行为：成年疣猪兽敌对，会主动攻击玩家）。
//
// C++ 链路（对齐 Java Hoglin.doHurtTarget，Hoglin.java:119-130 → HoglinBase.hurtAndThrowTarget）：
//   HoglinEntity::attackEntityAsMob override 自管完整攻击链：攻击动画（m_attackAnimationTicks=10 +
//   广播 HoglinAttack 状态）+ 随机化伤害（成年 f1/2 + random(0..f1-1)，ATTACK_DAMAGE=6 → 3~8）+
//   flingTarget 抛飞 + onAttackEntity 附魔后续 + setLastHurtBy + 音效。不调基类避免双重伤害
//   （基类 MobEntity::attackEntityAsMob 用固定伤害+causeExtraKnockback，语义不匹配 Hoglin 抛飞）。
//
// MeleeAttackGoal 委托：通用 MeleeAttackGoal::_attackTarget 调 m_creature->attackEntityAsMob(target)
// （对齐 vanilla MeleeAttackGoal.checkAndPerformAttack 调 mob.doHurtTarget），使 Hoglin override 生效。
// 历史上 Hoglin 把专用攻击逻辑放在未被调用的 attackLivingTarget（死代码），MeleeAttackGoal 调虚函数
// attackEntityAsMob 走基类→Hoglin 攻击退化为基类固定伤害无抛飞无动画。改为 override attackEntityAsMob 后修复。
//
// 判别性说明：本测试只断言玩家掉血（HP<20），对"Hoglin override vs 基类 attackEntityAsMob"无判别性
// （基类也会 hurt 掉血）。其真正价值是验证 MeleeAttackGoal→attackEntityAsMob 攻击链路通（Hoglin 命中玩家），
// 回归保护 MeleeAttackGoal 委托修复。Hoglin override 的差异（随机化伤害/fling/动画）集成测试难以判别：
// fling 对玩家不产生位移（Cubium 玩家物理对 addVelocity 不响应），由单元测试 FlingingSupportTypesTest
// 覆盖（直接断言 flingTarget 对 target 的 velocity 应用 + 动画字段）。
//
// 环境选择：creeper_pit 开放坑无围墙（NearestAttackableTarget checkSight 射线不被玻璃阻挡）。
// Hoglin 不燃（setBurnsInDaylight(false) 构造期设）且无 FleeSun/RestrictSun goal，白天可测无需 night batch。
// Hoglin 在主世界理论上会僵尸化（timeInOverworld>300 转 Zoglin），但 Cubium HoglinEntity::tick 未实现
// 僵尸化逻辑（仅 cooldown/animation 递减），无转化干扰；且 maxTicks<300 远早于转化阈值。
// 成年 hoglin (2,2,3) + Survival 玩家 (3,2,3) 紧邻 1 格 < 近战攻击距离，选目标后直接命中。
//
// 判定手段：玩家 HP 下降（<20）。Hoglin 攻击伤害 3~8，首击即掉血。pollUntilSucceed 正向断言。
// Survival 玩家（gameMode=0）：创造玩家被 NearestAttackableTarget 滤掉不可被攻击。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_疣猪兽_ED.txt#行为（成年敌对攻击玩家）
function hoglinAttacksPlayer(test: Test): void {
  const hoglinType = "hoglin";

  // hoglin (2,2,3)、Survival 玩家 (3,2,3)，紧邻 1 格 < 近战攻击距离。
  // hoglin/玩家脚下各放玻璃支撑（creeper_pit y=1 air，防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(hoglinType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 轮询断言玩家掉血。hoglin 选目标 + MeleeAttackGoal 首次攻击冷却 20 tick，命中即 hurt。
  // startTick=30 留 spawn 注册 + 选目标 + 首攻时间，maxTick=200 留余量。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length === 0) return false;
    const health = players[0].getComponent("minecraft:health") as any;
    if (health === undefined) return false;
    return health.currentValue < 20;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 200,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const health = players.length > 0 ? (players[0].getComponent("minecraft:health") as any)?.currentValue : undefined;
      test.assert(false,
        `hoglin did not hit player (attackEntityAsMob override or MeleeAttackGoal delegation broken), hp=${health}`);
    },
  });
}

export function registerHoglinTests(): void {
  GameTest.register("MobBehaviorTests", "hoglin_attacks_player", hoglinAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(250);
}
