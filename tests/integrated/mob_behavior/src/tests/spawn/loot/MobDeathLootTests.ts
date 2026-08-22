// 生物死亡物品掉落 GameTest。
//
// 覆盖 LivingEntity.dropAllDeathLoot 死亡掉落链路（对齐 MC Java 1.21.11
// LivingEntity.java:1484-1493）。修复前 Cubium 普通生物死亡仅掉经验（LivingEntity::die 只调
// dropExperience），物品掉落链路（dropFromLootTable）整体缺失，致 cow 不掉皮革/牛肉、zombie
// 不掉腐肉。本次补齐 dropAllDeathLoot → dropFromLootTable（entity 参数集 LootContext +
// getLootTableId 取表 generate + spawnItemAtEntity 掉落），并加 doMobLoot gamerule 守卫
// （shouldDropLoot = !isChild() && getBoolean(DO_MOB_LOOT)，对齐 vanilla shouldDropLoot）。
//
// 设计要点：
//   1. test.kill(entity) 走 LivingEntity::onKillCommand 虚空伤害致死 → actuallyHurt 扣血至 0
//      → die → dropAllDeathLoot（同步生成掉落物 item 实体）。掉落物在 die 时立即生成，
//      pollUntilSucceed 轮询区域 item 实体数验证。
//   2. 区域限定 getEntities({type:"item", location, volume})：Cubium GameTest 批内并行 tick +
//      不清场，全维度查询会数到其他测试的掉落物。glass_pit 7×5×7 区域限定排除污染。
//   3. 无法读取 item 实体的 itemType（Cubium 脚本未绑定 minecraft:item 组件，getComponent 返
//      undefined）——故只能断言"区域 item 实体数 ≥1/==0"，不能断言特定物品类型。
//   4. cow 掉落表（cow.json）：leather(uniform[0,2]) + beef(uniform[1,3])。beef 必掉 1-3，
//      故 cow 死亡必出 item（稳定正向断言）。
//   5. zombie 掉落表（zombie.json）：rotten_flesh(uniform[0,2]) 可能掉 0；iron/carrot/potato
//      需 killed_by_player 条件（test.kill 用虚空伤害无玩家来源，killed_by_player=false 不掉）。
//      故单只 zombie test.kill 有约 25% 概率不掉任何 item（rotten_flesh 取 0）。为降低 flaky，
//      spawn 5 只 zombie 同时 test.kill，5 只 rotten_flesh 全 0 的概率 ≈ 0.25^5 ≈ 0.1%，可接受。
//   6. doMobLoot 守卫测试：doMobLoot=false 时 shouldDropLoot 返 false，dropFromLootTable 不调，
//      cow 死亡不掉任何 item（beef 也不掉）。确定性负向断言（item==0）。doMobLoot 是世界级单例
//      状态，独占 batch 串行 + runOnFinish 恢复 true（同 gametest-world-state-gamerule 隔离范式）。
//
// 注：经验掉落（dropExperience）不受 doMobLoot 守卫（vanilla 在守卫外），故 doMobLoot=false 时
// 经验球仍掉落——本测试只断言 item 实体，不断言经验球，故不受影响。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: LivingEntity.cpp dropAllDeathLoot/shouldDropLoot/dropFromLootTable（本次补齐）
// Ref: LivingEntity.java:1484-1493（vanilla dropAllDeathLoot）、:567-569（shouldDropLoot）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——批内并行 tick + 不清场，全维度 getEntities({type:"item"}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

/** 统计 glass_pit 区域内 item 实体数量（区域限定避免批内并行污染）。 */
function countItems(test: Test): number {
  return test.getDimension().getEntities({
    type: "item",
    location: test.worldLocation(PIT_FROM),
    volume: PIT_VOLUME,
  }).length;
}

// 牛死亡掉落物品（leather + beef）。
//
// test.kill(cow) 走 onKillCommand 虚空伤害致死 → die → dropAllDeathLoot → dropFromLootTable：
// getLootTableId 取 "minecraft:entities/cow" → LootTableManager 查 cow.json → 构建 entity 参数集
// LootContext（THIS_ENTITY=cow + DAMAGE_SOURCE=虚空伤害）→ generate 生成 leather(0-2)+beef(1-3)
// → spawnItemAtEntity 掉落。beef 必掉 1-3，故区域 item 实体数 ≥1 稳定成立。
//
// 修复前 dropFromLootTable 链路缺失，cow 死亡不掉任何 item（区域 item=0），测试将超时失败。
//
// 注：虚空伤害 outOfWorld 无实体来源，m_lastHurtBy=nullptr，recentlyHitByPlayer=false，
// KILLER_PLAYER 不设——但 cow 掉落表 leather/beef pool 无 killed_by_player 条件，不受影响。
// beef 的 furnace_smelt 条件（is_on_fire / direct_attacker 熔炼附魔）因 cow 未着火且无
// direct_attacker 而判 false，掉生牛肉（不影响"是否掉落"，只影响物品子类型）。
// Ref: cow.json（leather+beef 掉落表）、LivingEntity.cpp dropFromLootTable
function cowDropsItemsOnDeath(test: Test): void {
  // cow 放 (3,2,3)（glass_pit y=1 空气腔，脚下 y=0 grass_block 支撑）。
  const cow = test.spawn("minecraft:cow", { x: 3, y: 2, z: 3 });

  // tick 5 等 cow 完成 spawn 注册稳定后杀死（onKillCommand 虚空伤害致死 → die → 掉落物生成）。
  test.runAtTickTime(5, () => {
    (test as any).kill(cow);
  });

  // 轮询断言区域出现 item 实体（beef 必掉 1-3，item≥1 稳定）。
  // startTick=10 给 test.kill(tick5) 后 die 链路 + 掉落物生成留时序余量。
  pollUntilSucceed(test, () => countItems(test) >= 1, {
    startTick: 10,
    maxTick: 60,
    onTimeout: () => test.assert(false,
      `cow death should drop items (beef guaranteed), got ${countItems(test)} items`),
  });
}

// 僵尸死亡掉落物品（rotten_flesh 等）。
//
// spawn 5 只 zombie 同时 test.kill，断言区域 item≥1。zombie 掉落表 rotten_flesh uniform[0,2]
// 单只约 25% 概率掉 0（test.kill 无玩家来源，iron/carrot/potato 需 killed_by_player 不掉），
// 5 只全 0 概率 ≈ 0.25^5 ≈ 0.1%，可接受。用 5 只而非 1 只降低 rotten_flesh 取 0 的 flaky 风险。
//
// test.kill 虚空伤害同步致死，5 只 zombie 在 tick 5 同时被杀，掉落物立即生成。
// glass_pit 无围墙但 zombie test.kill 立即死不会燃烧干扰（燃烧需 tick 累积）。
// Ref: zombie.json（rotten_flesh+稀有掉落表）、LivingEntity.cpp dropFromLootTable
function zombieDropsItemsOnDeath(test: Test): void {
  // 5 只 zombie 分散放置于 glass_pit 内不同坐标（避免重叠），均 y=2 站立层。
  const positions = [
    { x: 1, y: 2, z: 1 },
    { x: 5, y: 2, z: 1 },
    { x: 1, y: 2, z: 5 },
    { x: 5, y: 2, z: 5 },
    { x: 3, y: 2, z: 3 },
  ];
  const zombies = positions.map((pos) => test.spawn("minecraft:zombie", pos));

  // tick 5 等 zombie spawn 稳定后全部杀死。
  test.runAtTickTime(5, () => {
    for (const zombie of zombies) {
      (test as any).kill(zombie);
    }
  });

  // 轮询断言区域 item≥1（5 只 rotten_flesh 全 0 概率极低）。
  pollUntilSucceed(test, () => countItems(test) >= 1, {
    startTick: 10,
    maxTick: 60,
    onTimeout: () => test.assert(false,
      `zombie death should drop items (rotten_flesh, 5 zombies), got ${countItems(test)} items`),
  });
}

// doMobLoot=false 时生物死亡不掉落物品（doMobLoot gamerule 守卫）。
//
// 对齐 vanilla shouldDropLoot = !isBaby() && level.getGameRules().get(MOB_DROPS)：
// doMobLoot=false 时 shouldDropLoot 返 false，dropAllDeathLoot 跳过 dropFromLootTable，
// cow 死亡不掉任何 item（即使 beef 必掉也被守卫拦截）。确定性负向断言（item==0）。
//
// 经验掉落不受 doMobLoot（vanilla 在守卫外），doMobLoot=false 时经验球仍掉——但本测试只断言
// item 实体，经验球是 xp_orb 类型不计入 type:"item"，故不受影响。
//
// 【并行污染隔离】doMobLoot 是世界级单例状态，GameTest 共享单一 ServerWorld 跨测试持久化不自动
// 重置，设 false 会污染同批依赖 mob 掉落的测试（cow_drops/zombie_drops 的 item 断言）。故独占
// batch（mob_loot_solo）串行执行 + runOnFinish 恢复 true（同 gametest-world-state-gamerule 隔离）。
// Ref: LivingEntity.cpp shouldDropLoot（doMobLoot 守卫）、LivingEntity.java:567-569（vanilla 守卫）
function mobDeathDropsDisabledWhenDoMobLootFalse(test: Test): void {
  // 创造玩家执行管理命令（permLevel=4 ≥2）。doMobLoot 默认 true，显式设 false。
  const player = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "op");
  player.chat("/gamerule doMobLoot false");
  // runOnFinish 恢复 true 防污染后续批次（doMobLoot 世界级跨测试持久化）。
  test.runOnFinish(() => {
    player.chat("/gamerule doMobLoot true");
  });

  const cow = test.spawn("minecraft:cow", { x: 4, y: 2, z: 4 });

  // tick 10 等 doMobLoot=false 命令生效（chat 命令队列有 tick 延迟）后杀死 cow。
  test.runAtTickTime(10, () => {
    (test as any).kill(cow);
  });

  // 轮询断言区域无 item 实体（doMobLoot=false 守卫拦截掉落，beef 也不掉）。
  // startTick=15 给 doMobLoot 生效 + test.kill(tick10) + die 链路留时序余量。
  pollUntilSucceed(test, () => countItems(test) === 0, {
    startTick: 15,
    interval: 5,
    maxTick: 50,
    onTimeout: () => test.assert(false,
      `cow death should drop no items when doMobLoot=false, got ${countItems(test)} items`),
  });
}

export function registerMobDeathLootTests(): void {
  GameTest.register("MobBehaviorTests", "cow_drops_items_on_death", cowDropsItemsOnDeath)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "zombie_drops_items_on_death", zombieDropsItemsOnDeath)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  // doMobLoot 是世界级状态，独占 batch 串行避免污染同批依赖 mob 掉落的测试 + runOnFinish 恢复 true
  //（见 mobDeathDropsDisabledWhenDoMobLootFalse 注释的并行污染隔离说明）。
  GameTest.register("MobBehaviorTests", "mob_death_drops_disabled_when_do_mob_loot_false", mobDeathDropsDisabledWhenDoMobLootFalse)
    .structureName("gametests:glass_pit")
    .batch("mob_loot_solo")
    .maxTicks(80);
}
