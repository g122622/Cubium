// 猪灵蛮兵（PiglinBrute）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 猪灵蛮兵无条件攻击玩家（wiki tech_猪灵蛮兵.txt#行为：猪灵蛮兵会主动攻击 12 格内的玩家，
// 无论玩家是否穿着金质盔甲）。
//
// 区别于普通猪灵（piglin_ignores_player_wearing_gold：穿金装备时猪灵不攻击），猪灵蛮兵的目标选择
// 不检查金装备——NearestAttackableTargetGoal<Player>(checkSight=true) 无金装备门控，直接选 Survival
// 玩家为 attackTarget。这是猪灵蛮兵的核心行为特征。
//
// C++ 链路：PiglinBruteEntity::registerGoals 注册 MeleeAttackGoal(优先级2) +
// NearestAttackableTargetGoal<Player>(优先级3，不检查金装备) + HurtByTargetGoal(优先级2，呼叫支援)。
// PiglinBrute 无 attackEntityAsMob override，走基类 MobEntity::attackEntityAsMob（普通近战，ATTACK_DAMAGE=7）。
// MeleeAttackGoal::_attackTarget 委托 attackEntityAsMob（对齐 vanilla doHurtTarget 派发）。
//
// 环境选择：creeper_pit 开放坑无围墙（NearestAttackableTarget checkSight 射线不被玻璃阻挡）。
// PiglinBrute 继承 AbstractPiglinEntity 的 setBurnsInDaylight(false)，不燃，白天可测无需 night batch。
// PiglinBrute 无 tick override（无僵尸化逻辑），主世界不转化，无干扰。
// PiglinBrute (2,2,3) + Survival 玩家 (3,2,3) 紧邻 1 格 < 近战攻击距离，选目标后直接命中。
//
// 判定手段：玩家 HP 下降（<20）。PiglinBrute ATTACK_DAMAGE=7，首击即掉血。pollUntilSucceed 正向断言。
// Survival 玩家（gameMode=0）：创造玩家被 NearestAttackableTarget 滤掉不可被攻击。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪灵蛮兵.txt#行为（无条件攻击玩家）
function piglinBruteAttacksPlayerUnconditional(test: Test): void {
  const piglinBruteType = "piglin_brute";

  // piglin_brute (2,2,3)、Survival 玩家 (3,2,3)，紧邻 1 格 < 近战攻击距离。
  // 脚下各放玻璃支撑（creeper_pit y=1 air，防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(piglinBruteType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 轮询断言玩家掉血。piglin_brute 选目标 + MeleeAttackGoal 首次攻击冷却 20 tick，命中即 hurt。
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
        `piglin_brute did not hit player (MeleeAttackGoal→attackEntityAsMob delegation broken or target selection failed), hp=${health}`);
    },
  });
}

export function registerPiglinBruteTests(): void {
  GameTest.register("MobBehaviorTests", "piglin_brute_attacks_player_unconditional", piglinBruteAttacksPlayerUnconditional)
    .structureName("gametests:creeper_pit")
    .maxTicks(250);
}
