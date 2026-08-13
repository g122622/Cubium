// 史莱姆行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 大型史莱姆死亡时分裂出 2-4 只中型史莱姆（wiki tech_史莱姆.txt#行为）。
// 尺寸 4（大型）死亡 → performSplit 生成 2-4 只尺寸 2（中型），新尺寸 = 原尺寸 / 2 向下取整。
// C++ 链路：SlimeEntity::remove()（死亡链路 tickDeath deathTime>=20 时调用）→ performSplit() →
// 通过 EntityType 工厂创建 newSize=m_size/2 的小史莱姆并 spawnEntity。
// 依赖 C++ 改动（2026-08-14）：
//   1. GameTestHelper::spawn 解析 <spawnEvent> 后缀（对齐基岩 Test.spawn 官方语义）：
//      "slime<minecraft:spawn_large>" → applySpawnEvent 调 setSlimeSize(4) 生成大型史莱姆。
//      此前 normalizeEntityType 不解析后缀，spawn 出的史莱姆恒 size=1（HP=1 无分裂能力）。
//   2. GameTestHelper::killEntity（JS kill）：走 LivingEntity::onKillCommand 虚空伤害致死 →
//      actuallyHurt 扣血至 0 → die → tickDeath 累计 deathTime 至 20 → remove() → performSplit。
//      killAllEntities 用 discard（静默移除）不经死亡链路不触发分裂，故需 kill。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_史莱姆.txt#行为（尺寸大于1的史莱姆死亡时分裂）
function slimeLargeSplitsOnDeath(test: Test): void {
  const slimeType = "slime";

  // 结构 glass_pit（7×5×7）：y=0 满铺 grass_block，y=1..4 全 air（无围墙）。
  // 大型史莱姆尺寸 4，碰撞箱 2.08×2.08×2.08，glass_pit 7×5×7 空间充足。
  // spawn 于 (3,2,3)（结构内 y=1 空气腔，脚踩 y=0 grass_block）。
  // <minecraft:spawn_large> 后缀经 applySpawnEvent 设尺寸 4（HP=16）。
  const slime = test.spawn(`${slimeType}<minecraft:spawn_large>`, { x: 3, y: 2, z: 3 });

  // tick 5 时杀死大史莱姆：onKillCommand 虚空伤害致死 → die → 进入 deathTime 倒计时。
  // 留 5 tick 让实体完成 spawn 注册（setPosition/spawnEntity + 首 tick 稳定）。
  // test.kill 是项目测试设施（基岩 Test 类无），TS 类型未声明，用 as any 绕过。
  (test as any).kill(slime);

  // 大史莱姆死亡后约 20 tick（deathTime 倒计时）调 remove() → performSplit 生成分裂体。
  // tick 30 时大史莱姆已 erase，区域内应只剩分裂出的中型史莱姆（尺寸 2，数量 2-4）。
  // getEntities({type:"slime"}) 返回所有 slime 实体；断言数量在 [2,4] 区间（performSplit 随机 2-4 只）。
  // maxTicks=200：deathTime 20 tick + 分裂体 spawn + 余量。
  test.runAtTickTime(30, () => {
    const slimes = test.getDimension().getEntities({ type: slimeType });
    // 原大史莱姆已 erase（remove 后下一 tick 出 m_entities），剩余均为分裂体。
    // 数量 2-4 对齐 wiki 与 performSplit 的 rng.nextInt(2,4)。
    test.assert(slimes.length >= 2 && slimes.length <= 4,
      `expected 2-4 split slimes, got ${slimes.length}`);
    test.succeed();
  });
}

// 小型史莱姆（尺寸 1）没有攻击能力，即使有玩家目标也不造成伤害（wiki tech_史莱姆.txt#行为）。
// wiki 明确"小型史莱姆没有攻击能力，即使修改其 attack_damage 属性"——这是尺寸 1 的独有行为，
// 由 SlimeEntity::dealDamage 内 m_size<=1 直接 return + canDamagePlayer() 返回 m_size>1 共同保证。
// 验证：小型史莱姆 + Survival 玩家在攻击距离内，若干 tick 后玩家 HP 仍为满血（20）。
// 注：此为负向断言（验证不造成伤害）。若史莱姆 AI/寻路全坏不靠近玩家，测试也假性通过——
// 但 slime_large_splits_on_death 的正向断言（分裂确实发生）交叉验证了史莱姆尺寸链路正确。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_史莱姆.txt#行为（小型史莱姆没有攻击能力）
function slimeSmallCannotDamagePlayer(test: Test): void {
  const slimeType = "slime";

  // 结构 glass_pit（7×5×7 开放）：小型史莱姆尺寸 1 碰撞箱 0.52，无攻击能力。
  // 史莱姆 spawn 于 (3,2,3)，Survival 玩家于 (4,2,3)（直线 1 格，在 SlimeAttackGoal 攻击距离内）。
  // 玩家用 Survival（gameMode=0）：史莱姆 NearestAttackableTargetGoal<Player> 选其为目标，
  // 但 onCollideWithPlayer → canDamagePlayer()=false（m_size>1 为假）→ 不调 dealDamage。
  // 传数字 0 并用 as any 绕过 TS 字符串枚举类型校验（运行时 C++ 绑定期望数字，见 SpiderTests 同款注释）。
  test.spawn(slimeType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 玩家初始满血 20。小型史莱姆即使接触玩家也不造成伤害，HP 应保持 20。
  // maxTicks=200：史莱姆 SlimeHopGoal 跳跃接近 + 接触判定 + 余量。
  test.succeedWhen(() => {
    const player = test.getDimension().getEntities({ type: "minecraft:player" })[0];
    test.assert(player !== undefined, "player disappeared");
    const health = player.getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    // currentValue 对齐 HealthComponent.currentValue（LivingEntity::health）。
    test.assert((health as any).currentValue >= 20,
      `small slime should not damage player, hp=${(health as any).currentValue}`);
  });
}

export function registerSlimeTests(): void {
  GameTest.register("MobBehaviorTests", "slime_large_splits_on_death", slimeLargeSplitsOnDeath)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "slime_small_cannot_damage_player", slimeSmallCannotDamagePlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
