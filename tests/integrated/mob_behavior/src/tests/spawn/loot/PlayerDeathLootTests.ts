// 玩家死亡掉落库存物品 GameTest。
//
// 覆盖 Player 死亡掉落库存链路（对齐 MC Java 1.21.11 Player.dropEquipment，Player.java:558-565）。
// 修复前 Cubium LivingEntity::dropAllDeathLoot 不调用 dropEquipment（基类空实现），且 Player 未 override
// dropEquipment，致玩家死亡时库存物品静默保留在实体上不掉落（与 vanilla 偏差：vanilla 玩家非
// keepInventory 死亡会掉落全部库存 + 销毁带消失诅咒的物品）。本次补齐：
//   1. LivingEntity 新增虚函数 dropEquipment()（基类空），dropAllDeathLoot 在守卫之外调用（对齐 vanilla
//      LivingEntity.dropAllDeathLoot 第 1491 行 dropEquipment 在 shouldDropLoot 守卫之外）。
//   2. Player override dropEquipment：keepInventory 守卫 + destroyVanishingCursedItems（销毁消失诅咒物品）
//      + m_inventory.dropAllItems()（遍历槽位调 dropItem 生成 ItemEntity）。
//   3. 顺手修复 Player::die 重复掉经验 Bug——原 Player::die 额外调一次 dropExperience，而
//      LivingEntity::die → dropAllDeathLoot 也调 dropExperience，致玩家死亡掉两次经验球。vanilla
//      Player.die 不重复调 dropExperience（经 isAlwaysExperienceDropper=true 让基类守卫通过掉一次）。
//      本次移除 Player::die 中的重复 dropExperience 调用，经验由 dropAllDeathLoot 统一编排（仅调一次）。
//
// 设计要点：
//   1. SimulatedPlayer 无 kill/damage 方法，用 (test as any).kill(player)（GameTestHelper::killEntity）
//      走 LivingEntity::onKillCommand 虚空伤害致死 → actuallyHurt 扣血至 0 → Player::die override →
//      LivingEntity::die → dropAllDeathLoot → dropEquipment()（守卫之外，第 558 行）→ Player::dropEquipment
//      → keepInventory 守卫 → destroyVanishingCursedItems → dropAllItems → spawnItemEntity。
//   2. 玩家库存放物品用 player.setItem(stack, slot, selectSlot)：slot 0=主手。Survival 模式（第三参 0）
//      确保死亡真实掉落（创造模式被 kill 会 respawn 不掉落）。
//   3. 区域限定 getEntities({type:"item", location, volume})：批内并行 tick + 不清场，全维度查询跨测试
//      污染。glass_pit 7×5×7 区域限定排除。
//   4. 关键差异（Player vs Mob 装备掉落）：Player.dropEquipment 在 shouldDropLoot（doMobLoot）守卫
//      之外，只受 keepInventory 守卫约束——故 doMobLoot=false 时玩家库存仍掉落（与 mob 装备掉落
//      受 doMobLoot 守卫不同）。测试 3 专门覆盖此差异。
//   5. keepInventory 是世界级单例状态，独占 batch 串行 + runOnFinish 恢复 false 避免污染同批测试
//      （同 gametest-world-state-gamerule 隔离范式）。
//   6. 无法读取 item 实体的 itemType（Cubium 脚本未绑定 minecraft:item 组件），只能断言区域 item
//      实体数 ≥1/==0，不能断言特定物品类型。玩家放 1 组物品（64 个），dropAllItems 掉落 1 个 item
//      实体（整组），故区域 item≥1 稳定。
//
// 经验重复掉落修复（第 3 点）的可测性说明：经验球是 minecraft:experience_orb，数量由
// calculateDeathDropXp 拆分总经验为多个经验球（vanilla 行为，数量随机），脚本读不了经验球 value，
// 无法确定性断言"只掉一次"。故经验重复修复靠代码审查 + 编译保证正确性（见 Player.cpp die 注释），
// 本测试不单独验证经验球数量。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: Player.cpp dropEquipment/destroyVanishingCursedItems/die（本次补齐）
// Ref: LivingEntity.cpp dropAllDeathLoot（启用 dropEquipment 调用）
// Ref: Player.java:533-565（vanilla Player.die/dropEquipment/destroyVanishingCursedItems）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：批内并行 tick + 不清场，全维度 getEntities({type:"item"}) 跨测试污染。
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

/**
 * 给玩家主手槽（slot 0）放入 1 组指定物品。
 *
 * player.setItem(stack, 0, true)：写入 PlayerInventory 主手槽（selectSlot=true 同步选中主手）。
 * 真实实现（SimulatedPlayer.cpp:194-204 转发 PlayerInventory::setItem）。参照 BowDurabilityTests 范式。
 */
function putItemInMainHand(player: any, itemId: string, amount: number): void {
  const stack = new ItemStack(itemId, amount);
  player.setItem(stack as unknown as Parameters<typeof player.setItem>[0], 0, true);
}

// 玩家死亡掉落库存物品（Survival，主手放 64 石头，kill 后区域 item≥1）。
//
// Survival 玩家 (1,2,3) 主手放 64 cobblestone。tick 1 setItem 写入库存。tick 8 (test as any).kill(player)
// 走 onKillCommand 虚空伤害致死 → Player::die → LivingEntity::die → dropAllDeathLoot → dropEquipment()
// （守卫之外）→ Player::dropEquipment：keepInventory=false（默认）守卫通过 → destroyVanishingCursedItems
// （cobblestone 无消失诅咒，跳过）→ dropAllItems 遍历槽位，主手 64 cobblestone 调 dropItem 生成 1 个
// item 实体（整组掉落）+ 清空槽位。故区域 item≥1 稳定成立。
//
// 修复前 Player::dropEquipment 缺失（基类 LivingEntity::dropEquipment 空实现），玩家死亡库存物品
// 静默保留在实体上不掉落，区域 item=0，测试超时失败。
//
// Survival 模式（spawnSimulatedPlayer 第三参 0=Survival）确保死亡真实掉落——创造模式玩家被 kill
// 会 respawn 不掉落库存（参照 EntityCommandTests 注释）。
function playerDropsInventoryOnDeath(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "victim", 0 as any);

  // tick 1 等玩家 spawn 注册稳定后放入主手物品。
  test.runAtTickTime(1, () => {
    putItemInMainHand(player, "minecraft:cobblestone", 64);
  });

  // tick 8 等物品写入生效后杀死（onKillCommand 虚空伤害致死 → die → dropEquipment 掉落库存）。
  test.runAtTickTime(8, () => {
    (test as any).kill(player);
  });

  // 轮询断言区域 item≥1（主手 64 cobblestone 整组掉落为 1 个 item 实体）。
  // startTick=13 给 test.kill(tick8) 后 die 链路 + 掉落物生成留时序余量。
  pollUntilSucceed(test, () => countItems(test) >= 1, {
    startTick: 13,
    maxTick: 60,
    onTimeout: () => test.assert(false,
      `player death should drop inventory items, got ${countItems(test)} items`),
  });
}

// keepInventory=true 时玩家死亡不掉落库存（keepInventory 守卫）。
//
// 对齐 vanilla Player.dropEquipment（Player.java:561）：keepInventory=true 时直接 return，不销毁消失诅咒
// 物品也不掉落库存。玩家死亡后库存保留在实体上（重生后仍有），区域无 item 实体。确定性负向断言
// （item==0）。
//
// 【并行污染隔离】keepInventory 是世界级单例状态，GameTest 共享单一 ServerWorld 跨测试持久化不自动
// 重置，设 true 会污染同批依赖"玩家死亡掉落"的测试（player_drops_inventory_on_death 的 item 断言）。
// 故独占 batch（player_keepinv_solo）串行执行 + runOnFinish 恢复 false。
// Ref: Player.cpp dropEquipment（keepInventory 守卫）、Player.java:561（vanilla 守卫）
function playerInventoryKeptWithKeepInventory(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "keeper", 0 as any);

  // 创造玩家执行管理命令设 keepInventory=true（permLevel=4）。第三参省略默认创造（1）。
  // 实际这里用 Survival 玩家也可（chat permLevel 固定=4，与游戏模式解耦），但 setItem 需 Survival
  // 才有真实库存——Survival 玩家 chat 同样可设 keepInventory。
  player.chat("/gamerule keepInventory true");
  // runOnFinish 恢复 false 防污染后续批次（keepInventory 世界级跨测试持久化）。
  test.runOnFinish(() => {
    player.chat("/gamerule keepInventory false");
  });

  // tick 1 放主手物品，tick 8 杀死（keepInventory=true 守卫拦截掉落，区域无 item）。
  test.runAtTickTime(1, () => {
    putItemInMainHand(player, "minecraft:cobblestone", 64);
  });

  test.runAtTickTime(8, () => {
    (test as any).kill(player);
  });

  // 轮询断言区域无 item 实体（keepInventory=true 守卫拦截，库存保留实体上不掉落）。
  // startTick=15 给 keepInventory 生效（chat 命令 tick 延迟）+ test.kill(tick8) + die 链路留余量。
  pollUntilSucceed(test, () => countItems(test) === 0, {
    startTick: 15,
    interval: 5,
    maxTick: 50,
    onTimeout: () => test.assert(false,
      `player inventory should be kept (no drops) when keepInventory=true, got ${countItems(test)} items`),
  });
}

// doMobLoot=false 时玩家库存仍掉落（Player.dropEquipment 在 doMobLoot 守卫之外，与 mob 不同）。
//
// 关键差异验证：mob 装备掉落在 dropCustomDeathLoot 内（受 shouldDropLoot=doMobLoot 守卫），doMobLoot=false
// 时不掉；而 Player.dropEquipment 在 dropAllDeathLoot 的 shouldDropLoot 守卫**之外**（LivingEntity.cpp:558），
// 只受 keepInventory 守卫约束——故 doMobLoot=false 时玩家库存仍掉落。对齐 vanilla：Player.dropEquipment
// 不查 doMobLoot，仅查 keepInventory（Player.java:561）。
//
// doMobLoot=false + keepInventory=false（默认）：玩家死亡 dropEquipment 守卫通过（keepInventory=false）→
// 掉落库存。区域 item≥1（doMobLoot 不影响玩家库存掉落）。此测试确认玩家掉落链路与 mob 掉落受不同
// 守卫约束，防止未来误把 Player.dropEquipment 移入 shouldDropLoot 守卫内。
//
// 【并行污染隔离】doMobLoot 是世界级单例状态，独占 batch（player_domobloot_solo）串行 + runOnFinish
// 恢复 true（同 mob_death_drops_disabled_when_do_mob_loot_false 隔离范式）。
// Ref: LivingEntity.cpp dropAllDeathLoot（dropEquipment 在 shouldDropLoot 守卫之外）、Player.java:561
function playerInventoryDropsEvenWhenDoMobLootFalse(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "victim", 0 as any);

  // doMobLoot 默认 true，显式设 false。Survival 玩家 chat permLevel=4 可执行。
  player.chat("/gamerule doMobLoot false");
  // runOnFinish 恢复 true 防污染后续批次（doMobLoot 世界级跨测试持久化）。
  test.runOnFinish(() => {
    player.chat("/gamerule doMobLoot true");
  });

  // tick 1 放主手物品，tick 10 杀死（留 doMobLoot=false 命令生效 tick 延迟）。
  test.runAtTickTime(1, () => {
    putItemInMainHand(player, "minecraft:cobblestone", 64);
  });

  test.runAtTickTime(10, () => {
    (test as any).kill(player);
  });

  // 轮询断言区域 item≥1（doMobLoot=false 不影响玩家库存掉落，dropEquipment 在守卫之外）。
  // startTick=15 给 doMobLoot 生效 + test.kill(tick10) + die 链路留余量。
  pollUntilSucceed(test, () => countItems(test) >= 1, {
    startTick: 15,
    interval: 5,
    maxTick: 60,
    onTimeout: () => test.assert(false,
      `player inventory should drop even when doMobLoot=false (dropEquipment outside guard), `
      + `got ${countItems(test)} items`),
  });
}

export function registerPlayerDeathLootTests(): void {
  GameTest.register("MobBehaviorTests", "player_drops_inventory_on_death", playerDropsInventoryOnDeath)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  // keepInventory 是世界级状态，独占 batch 串行避免污染同批"玩家死亡掉落"测试 + runOnFinish 恢复 false。
  GameTest.register("MobBehaviorTests", "player_inventory_kept_with_keep_inventory", playerInventoryKeptWithKeepInventory)
    .structureName("gametests:glass_pit")
    .batch("player_keepinv_solo")
    .maxTicks(80);

  // doMobLoot 是世界级状态，独占 batch 串行避免污染同批 mob 掉落测试 + runOnFinish 恢复 true。
  GameTest.register("MobBehaviorTests", "player_inventory_drops_even_when_do_mob_loot_false", playerInventoryDropsEvenWhenDoMobLootFalse)
    .structureName("gametests:glass_pit")
    .batch("player_domobloot_solo")
    .maxTicks(80);
}
