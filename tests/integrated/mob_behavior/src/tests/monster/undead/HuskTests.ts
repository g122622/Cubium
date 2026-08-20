// 尸壳行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 尸壳不会在阳光下燃烧（wiki tech_尸壳.txt#行为：尸壳不会在阳光下着火）。
// C++ 链路：MonsterEntity::tick → handleDaylightBurning → burnUndead → isInDaylight
// （isDaytime + brightness>0.5 + !isWet + canSeeSky）→ igniteForSeconds(8.0f)。
// shouldBurnInDaylight() 为 true 才进燃烧判定。HuskEntity::shouldBurnInDaylight()
// override 返回 false（hpp:76）→ 跳过燃烧，尸壳不燃。
//
// 单实体负向断言：只 spawn 尸壳，断言 onfire 恒 undefined。不在此测试内 spawn 对照实体
// （僵尸/骷髅）——实测同结构 spawn 多实体时光照重算竞态致 brightness 偏低，骷髅也不燃
// （单跑 skeleton_burns_in_daylight 则稳定燃烧）。正向对照由独立的 skeleton_burns_in_daylight
// 测试提供（骷髅该着火着火），两者互补验证 shouldBurnInDaylight 门控：骷髅燃 + 尸壳不燃。
//
// 僵尸/尸壳不用 FleeSunGoal/RestrictSunGoal（ZombieEntity.cpp:496 注释：骷髅才用），
// 故露天不会逃离阳光，尸壳持续不燃，断言稳定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_尸壳.txt#行为（不会在阳光下着火）
function huskDoesNotBurnInDaylight(test: Test): void {
  const huskType = "husk";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 尸壳 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，与 skeleton_burns_in_daylight 同款 spawn 位（保证光照环境一致可比）。
  const husk = test.spawn(huskType, { x: 4, y: 2, z: 4 });

  // 白天露天尸壳不着火：轮询 onfire 组件应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=500：与 skeleton_burns_in_daylight 同款，留余量确保断言稳定。
  // 正向对照由 skeleton_burns_in_daylight 提供（同结构同 skyAccess 骷髅该着火着火），
  // 排除此负向断言"燃烧机制未触发"的假性通过。
  test.succeedWhen(() => {
    const fire = husk.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("husk should not burn in daylight");
    }
  });
}

// 尸壳空手近战攻击玩家时施加饥饿效果（wiki tech_尸壳.txt#行为：尸壳在攻击时会对目标施加
// 区域难度×7秒的饥饿效果；手持物品的尸壳不施加饥饿）。
//
// C++ 链路（对齐 Java Husk.doHurtTarget，Husk.java:57-65）：
//   HuskEntity::attackEntityAsMob override 调父类（ZombieEntity::attackEntityAsMob →
//   MonsterEntity::attackEntityAsMob）执行基础攻击，命中 + 主手为空 + 目标是 LivingEntity 时，
//   取区域难度 effectiveDifficulty，addEffect(Hunger, 140 * (int)f ticks)。
//   GameTestServer 默认 Normal 难度，effectiveDifficulty≈2.0 → 280 ticks ≈ 14 秒饥饿 I。
//
// MeleeAttackGoal 委托：通用 MeleeAttackGoal::_attackTarget 调 m_creature->attackEntityAsMob(target)
// （对齐 vanilla MeleeAttackGoal.checkAndPerformAttack 调 mob.doHurtTarget），使 Husk 的 override
// 生效。此前 MeleeAttackGoal 直接 target->hurt 绕过 attackEntityAsMob 虚派发，Husk 饥饿 override
// 从不触发——本测试暴露并验证该修复。
//
// 空手判定：test.spawn 不走 finalizeSpawn/populateDefaultEquipmentSlots，husk 主手默认空，
// 满足 getMainHandItem().isEmpty() 条件施加饥饿。
//
// 环境选择：creeper_pit 开放坑无围墙（NearestAttackableTarget checkSight 射线不被玻璃阻挡），
// husk 不燃（shouldBurnInDaylight=false）白天可测无需 night batch。husk (2,2,3) + Survival 玩家
// (3,2,3) 紧邻 1 格 < 1.11 近战攻击距离，husk 选目标后 MeleeAttackGoal 直接命中无需寻路接近。
//
// 判定手段：玩家获得 hunger 效果（getEffect("hunger") !== undefined）。近战确定性命中（无散布），
// husk 首次攻击冷却 20 tick 即施加饥饿。pollUntilSucceed 正向断言（条件满足即 succeed 合理）。
// 玩家用 Survival（gameMode=0）：创造玩家被 NearestAttackableTarget 滤掉不可被攻击。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_尸壳.txt#行为（攻击施加饥饿效果）
function huskInflictsHunger(test: Test): void {
  const huskType = "husk";

  // husk (2,2,3)、Survival 玩家 (3,2,3)，紧邻 1 格 < 1.11 近战攻击距离。
  // husk/玩家脚下各放玻璃支撑（creeper_pit y=1 air，防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(huskType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 轮询断言玩家获得 hunger 效果。husk 选目标 + MeleeAttackGoal 首次攻击冷却 20 tick，
  // 命中即 addEffect(Hunger)。startTick=30 留 spawn 注册 + 选目标 + 首攻时间，maxTick=200 留余量。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length === 0) return false;
    const hunger = (players[0] as any).getEffect("hunger");
    return hunger !== undefined;
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
      const hunger = players.length > 0 ? (players[0] as any).getEffect("hunger") : undefined;
      test.assert(false,
        `husk did not inflict hunger on player (attackEntityAsMob override or MeleeAttackGoal delegation broken), hunger=${hunger ? "present" : "absent"}`);
    },
  });
}

export function registerHuskTests(): void {
  GameTest.register("MobBehaviorTests", "husk_does_not_burn_in_daylight", huskDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：清空结构上方 worldgen 方块制造露天列使 canSeeSky=true。
    // setupTicks(20)：清空上方后 skyLight 入队需 tick 重算达 15，setupTicks 阶段先 tick 20 次让光照稳定。
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "husk_inflicts_hunger", huskInflictsHunger)
    .structureName("gametests:creeper_pit")
    .maxTicks(250);
}
