// 洞穴蜘蛛行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 洞穴蜘蛛夜晚主动攻击玩家（wiki tech_洞穴蜘蛛.txt#行为：亮度≤11 时敌对，与普通蜘蛛相同）。
// 洞穴蜘蛛是普通蜘蛛的洞穴变种，继承 SpiderEntity 的 SpiderTargetGoal<Player>(checkSight=true)
// + SpiderAttackGoal(MeleeAttackGoal) + LeapAtTarget。区别于普通蜘蛛：洞穴蜘蛛攻击附带中毒
// （普通难度 7 秒中毒 I，困难 15 秒；对应原版 CaveSpider.doHurtTarget 的 addEffect(POISON, i*20, 0)）。
//
// C++ 链路：CaveSpiderEntity : SpiderEntity : MonsterEntity，attackEntityAsMob override 调
// SpiderEntity::attackEntityAsMob 基础攻击后按难度施加中毒。SpiderTargetGoal.shouldExecute 要求
// getBrightness()<0.5F（夜晚 skyDarkening≈11，露天 magicValue≈0.083<0.5 触发）→ 选 Survival 玩家
// 为 attackTarget → SpiderAttackGoal 寻路接近 + 攻击。
//
// 环境选择：必须夜晚 batch("night") + creeper_pit（开放坑）。SpiderTargetGoal 需亮度<0.5 触发敌对，
// 白天亮度高不攻击。creeper_pit 开放坑无围墙，checkSight 射线不被玻璃阻挡（grass_pen 玻璃墙挡 canSee，
// 见 SpiderTests 同款注释）。
//
// 判定手段：断言玩家 HP 下降（<20）。洞穴蜘蛛近战伤害 2（registerAttributes ATTACK_DAMAGE=2.0），
// 玩家初始满血 20，被命中即掉至 <20。玩家掉血证明洞穴蜘蛛近战攻击链路通。
// 不直接断言中毒效果（药水效果组件未绑定 JS 不可读，中毒攻击仅 C++ 侧对齐原版无测试覆盖，
// 见 BoggedTests/StrayTests 同款约束）。玩家用 Survival（gameMode=0）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_洞穴蜘蛛.txt#行为（敌对攻击 + 中毒）
function caveSpiderAttacksPlayerAtNight(test: Test): void {
  const caveSpiderType = "cave_spider";

  // 洞穴蜘蛛于 (3,2,3)（脚踩结构内 y=0 grass_block），Survival 玩家于 (4,2,3)，直线 1 格。
  // 开放坑无围墙，SpiderTargetGoal checkSight 射线不被阻挡；直线 1 格在 MeleeAttackGoal 攻击距离内，
  // 洞穴蜘蛛选定目标后直接攻击无需长距寻路。玩家被命中掉血（洞穴蜘蛛攻击伤害 2）。
  test.spawn(caveSpiderType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：SpiderTargetGoal tick 评估（GoalSelector 半 tick 评估）+ 选目标 + 攻击，
  // 约 tick 20-40 玩家首伤。maxTicks=400 留寻路/锁定/攻击 + 余量。
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
      `cave spider did not hit player, hp=${(health as any).currentValue}`);
  });
}

// 洞穴蜘蛛不在阳光下燃烧（wiki tech_洞穴蜘蛛.txt#行为：蜘蛛不在阳光下燃烧，洞穴蜘蛛继承）。
// 洞穴蜘蛛继承 SpiderEntity 的 shouldBurnInDaylight()=false（SpiderEntity.hpp），白天露天不着火。
// 与 spider_does_not_burn_in_daylight（普通蜘蛛不燃）+ skeleton_burns_in_daylight（骷髅燃）对照：
// 同为 MonsterEntity 子类，蜘蛛系（普通蜘蛛/洞穴蜘蛛）shouldBurn false 不燃，亡灵系燃烧。
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→isInDaylight 校验 shouldBurnInDaylight()，
// 洞穴蜘蛛继承蜘蛛 override 返回 false 跳过燃烧判定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_洞穴蜘蛛.txt#行为（不在阳光下燃烧）
function caveSpiderDoesNotBurnInDaylight(test: Test): void {
  const caveSpiderType = "cave_spider";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 洞穴蜘蛛 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const caveSpider = test.spawn(caveSpiderType, { x: 4, y: 2, z: 4 });

  // 白天露天洞穴蜘蛛不着火：轮询 onfire 组件，应恒 undefined（继承蜘蛛 shouldBurnInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，洞穴蜘蛛本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），若有框架 bug 让所有实体不着火测试也过——但有
  // skeleton_burns_in_daylight/zombie_burns_in_daylight 正向断言对照（亡灵该着火着火），互补验证。
  test.succeedWhen(() => {
    const fire = caveSpider.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("cave spider should not burn in daylight");
    }
  });
}

// 洞穴蜘蛛近战攻击玩家时施加中毒效果（wiki tech_洞穴蜘蛛.txt#行为：洞穴蜘蛛攻击附带中毒，
// 普通难度 7 秒中毒 I，困难 15 秒，简单无中毒）。
//
// C++ 链路（对齐 Java CaveSpider.doHurtTarget）：CaveSpiderEntity::attackEntityAsMob override 调
// SpiderEntity::attackEntityAsMob 基础攻击后，按难度施加中毒（Normal 7*20=140 ticks，等级 I）。
// GameTestServer 默认 Normal 难度，中毒生效。
//
// MeleeAttackGoal 委托：通用 MeleeAttackGoal::_attackTarget 调 m_creature->attackEntityAsMob(target)
// （对齐 vanilla doHurtTarget 派发），使 CaveSpider override 生效。此前 MeleeAttackGoal 直接
// target->hurt 绕过虚派发，中毒 override 从不触发——本测试验证该修复（与 husk_inflicts_hunger/
// wither_skeleton_inflicts_wither 同源修复）。getEffect JS 绑定已就绪（elder_guardian_applies_mining_fatigue
// 用 getEffect("mining_fatigue") 验证），此前注释"药水效果组件未绑定 JS 不可读"已过时。
//
// 环境选择：creeper_pit 开放坑无围墙（checkSight 射线不被玻璃阻挡）。SpiderTargetGoal 需亮度<0.5
// 触发敌对，故用 night batch。洞穴蜘蛛 (2,2,3) + Survival 玩家 (3,2,3) 紧邻 1 格直接命中。
//
// 判定手段：玩家获得 poison 效果（getEffect("poison") !== undefined）。近战确定性命中，首击即
// 施加中毒 140 ticks。pollUntilSucceed 正向断言。Survival 玩家（gameMode=0）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_洞穴蜘蛛.txt#行为（攻击附带中毒）
function caveSpiderInflictsPoison(test: Test): void {
  const caveSpiderType = "cave_spider";

  // 洞穴蜘蛛 (2,2,3)、Survival 玩家 (3,2,3)，紧邻 1 格 < 近战攻击距离。
  // 洞穴蜘蛛/玩家脚下各放玻璃支撑（creeper_pit y=1 air，防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(caveSpiderType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 轮询断言玩家获得 poison 效果。SpiderTargetGoal night 选目标 + MeleeAttackGoal 首攻冷却 20 tick，
  // 命中即 addEffect(Poison)。startTick=30 留 spawn 注册 + 选目标 + 首攻时间，maxTick=250 留余量。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length === 0) return false;
    const poison = (players[0] as any).getEffect("poison");
    return poison !== undefined;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 250,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const poison = players.length > 0 ? (players[0] as any).getEffect("poison") : undefined;
      test.assert(false,
        `cave_spider did not inflict poison on player (attackEntityAsMob override or MeleeAttackGoal delegation broken), poison=${poison ? "present" : "absent"}`);
    },
  });
}

export function registerCaveSpiderTests(): void {
  GameTest.register("MobBehaviorTests", "cave_spider_attacks_player_at_night", caveSpiderAttacksPlayerAtNight)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "cave_spider_does_not_burn_in_daylight", caveSpiderDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "cave_spider_inflicts_poison", caveSpiderInflictsPoison)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(300);
}
