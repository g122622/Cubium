// 生物死亡装备掉落 GameTest。
//
// 覆盖 MobEntity::dropCustomDeathLoot 装备死亡掉落链路（对齐 MC Java 1.21.11 Mob.dropCustomDeathLoot，
// Mob.java:846-877）。修复前 Cubium MobEntity 未 override dropCustomDeathLoot（基类空），m_equipmentDropChances
// 概率字段虽有存储却无人读取——mob 死亡时身上装备静默消失，既不掉落也不保留。本次补全 dropCustomDeathLoot：
// 遍历 EquipmentSlot，按 dropChances.byEquipment(slot) 概率 + isPreserved 保整 + recentlyHitByPlayer 门控 +
// 绑定诅咒排除（PREVENT_EQUIPMENT_DROP）+ 耐久度随机化，决定是否掉落该槽位装备，掉落后清空槽位。
//
// 框架补全（解锁装备掉落测试）：
//   1. EquippableComponent.setEquipment 此前仅支持清空（undefined/null），传 ItemStack 抛 TypeError。
//      本次补全 ItemStack unwrap 路径（复用 Container.setItem 的 resolveItemStackClassId + unwrap 范式），
//      使脚本能给 mob 穿装备（此前仅 SimulatedPlayer.setItem 可穿玩家装备，mob 无途径）。
//   2. 新增 EquippableComponent.setEquipmentDropChance(slot, chance) 绑定（MobEntity::setEquipmentDropChance），
//      使脚本能设装备掉落概率——测试用 >1.0（保整）构造确定性掉落，绕过 8.5% 默认概率与
//      recentlyHitByPlayer 门控（保整 f>1.0 不需 recentlyHitByPlayer 即尝试掉落）。
//
// 测试可测性限制（脚本无法读取 ItemEntity itemType，只能数 item 实体数）：
//   - 无法断言"掉落的是钻石剑"，只能断言区域 item 实体数。
//   - 故用【保整掉落多件 + 计数】做正向断言，用【默认概率门控对照】验证 recentlyHitByPlayer 分支，
//     用【doMobLoot 守卫】做干净负向。
//   - 绑定诅咒排除分支（hasBindingCurse → continue）：脚本无法给 ItemStack 附魔（ItemStack JS 仅
//     getEnchantments 只读，无 addEnchantment），无法构造绑定诅咒装备做集成测试。该分支由 C++ 逻辑保证
//     （EnchantmentHelper::hasBindingCurse 已单测覆盖），此处不写集成测试（TODO）。
//   - 掠夺附魔加成（processEquipmentDropChance）：Cubium 未实现 1.21 equipment_drops effect component
//     子系统，dropCustomDeathLoot 暂不应用掠夺加成（TODO，待 effect component 体系就绪）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: MobEntity.cpp dropCustomDeathLoot（本次补全）
// Ref: MinecraftModuleFactory.cpp EquippableComponent.setEquipment/setEquipmentDropChance（本次补全）
// Ref: Mob.java:846-877（vanilla dropCustomDeathLoot）

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
 * 给 mob 的 equippable 组件穿戴指定装备并设为保整掉落（f=2.0，无条件必掉）。
 *
 * setEquipment("Mainhand", stack)：本次补全的 ItemStack unwrap 路径，写入装备数组。
 * setEquipmentDropChance(slot, 2.0)：本次新增绑定，f>1.0 即 isPreserved=true，dropCustomDeathLoot
 *   中 `recentlyHitByPlayer || isPreserved` 恒 true，且 rng.nextFloat() < 2.0 恒成立 → 必掉。
 *
 * @param mob 目标实体
 * @param slotStr 槽位字符串（"Head"/"Chest"/"Legs"/"Feet"/"Mainhand"/"Offhand"/"Body"）
 * @param itemId 物品类型 id（如 "minecraft:diamond_sword"）
 */
function equipGuaranteedDrop(test: Test, mob: any, slotStr: string, itemId: string): void {
  const equippable = (mob as any).getComponent("minecraft:equippable");
  test.assert(equippable !== undefined, `mob has no equippable component (${itemId})`);
  const stack = new ItemStack(itemId, 1);
  equippable.setEquipment(slotStr, stack as unknown as any);
  equippable.setEquipmentDropChance(slotStr, 2.0);
}

// 保整掉落多件装备：mob 死亡后区域 item 实体数 ≥ 装备件数（确定性正向）。
//
// zombie 穿 6 件保整装备（4 护甲 + 主手钻石剑 + 副手盾），test.kill 虚空伤害致死 → die →
// dropAllDeathLoot → shouldDropLoot 守卫通过（doMobLoot=true 且非幼体）→ dropCustomDeathLoot：
// 6 件装备 f=2.0 保整，isPreserved=true，recentlyHitByPlayer||isPreserved 恒 true，nextFloat()<2.0 恒成立
// → 6 件全掉落 + setEquipment(slot, EMPTY) 清空。dropFromLootTable 另掉 rotten_flesh uniform[0,2]（0-2 件）。
// 故区域 item 数 = 6（装备）+ 0~2（rotten_flesh）= 6~8，断言 ≥6 确定性成立。
//
// 修复前 dropCustomDeathLoot 缺失，6 件装备静默消失，区域 item 仅 rotten_flesh（0-2），<6 → 测试超时失败。
//
// 注：test.kill 虚空伤害无玩家来源（m_lastHurtBy=nullptr，recentlyHitByPlayer=false），但保整掉落
// 不依赖 recentlyHitByPlayer（isPreserved 短路），故保整装备仍掉。rotten_flesh 无 killed_by_player 条件
// 也不依赖玩家来源。zombie 稀有掉落 iron/carrot/potato 需 killed_by_player，test.kill 不掉（不干扰计数）。
function mobDropsEquipmentOnDeathGuaranteed(test: Test): void {
  const zombie = test.spawn("minecraft:zombie", { x: 3, y: 2, z: 3 });

  // spawn 后立即穿 6 件保整装备（tick 1 等 spawn 注册稳定）。
  test.runAtTickTime(1, () => {
    equipGuaranteedDrop(test, zombie, "Head", "minecraft:diamond_helmet");
    equipGuaranteedDrop(test, zombie, "Chest", "minecraft:diamond_chestplate");
    equipGuaranteedDrop(test, zombie, "Legs", "minecraft:diamond_leggings");
    equipGuaranteedDrop(test, zombie, "Feet", "minecraft:diamond_boots");
    equipGuaranteedDrop(test, zombie, "Mainhand", "minecraft:diamond_sword");
    equipGuaranteedDrop(test, zombie, "Offhand", "minecraft:shield");
  });

  // tick 8 等装备穿戴生效后杀死（onKillCommand 虚空伤害致死 → die → dropCustomDeathLoot 掉 6 件装备）。
  test.runAtTickTime(8, () => {
    (test as any).kill(zombie);
  });

  // 轮询断言区域 item≥6（6 件保整装备必掉，rotten_flesh 0-2 不影响 ≥6 下限）。
  pollUntilSucceed(test, () => countItems(test) >= 6, {
    startTick: 13,
    maxTick: 60,
    onTimeout: () => test.assert(false,
      `mob death should drop 6 guaranteed equipment, got ${countItems(test)} items`),
  });
}

// 默认概率装备在非玩家击杀时不掉落（recentlyHitByPlayer 门控 + 保整对照）。
//
// zombie 穿 6 件装备：3 件保整（f=2.0，必掉）+ 3 件默认概率（f=0.085，不保整）。test.kill 虚空伤害
// 无玩家来源 recentlyHitByPlayer=false。dropCustomDeathLoot 对每件装备判 `recentlyHitByPlayer || isPreserved`：
//   - 3 件保整（isPreserved=true）：条件恒 true → 进概率判定 nextFloat()<2.0 恒成立 → 必掉。
//   - 3 件默认（isPreserved=false, recentlyHitByPlayer=false）：条件 false → continue，**不进概率判定，
//     100% 不掉**（门控在概率之前，recentlyHitByPlayer=false 时默认概率装备连概率判定都不进入）。
// 故 6 件装备仅 3 件保整掉落。dropFromLootTable 另掉 rotten_flesh uniform[0,2]（0-2 件）。
// 区域 item = 3（保整）+ 0~2（rotten_flesh）= 3~5，断言 ∈[3,5] 精确验证两个分支：
//   - 若保整分支失效（3 件保整误判不掉）：item<3 → 超下界 FAIL。
//   - 若默认门控失效（3 件默认误掉，0.085 概率或门控失效全掉）：item≥6 → 超上界 5 FAIL。
//
// 注：默认概率装备 0.085 即便误进概率判定也仅 8.5% 掉落，单测可能漏判；3 件全误掉概率 0.085^3≈0.06%，
// 配合 ≤5 上界基本可捕获门控失效。保整 3 件必掉是确定性下界。
// test.kill 时序同 mobDropsEquipmentOnDeathGuaranteed。
function mobDefaultChanceEquipmentNotDroppedWithoutPlayer(test: Test): void {
  const zombie = test.spawn("minecraft:zombie", { x: 3, y: 2, z: 3 });

  test.runAtTickTime(1, () => {
    // 3 件保整（必掉）。
    equipGuaranteedDrop(test, zombie, "Head", "minecraft:diamond_helmet");
    equipGuaranteedDrop(test, zombie, "Chest", "minecraft:diamond_chestplate");
    equipGuaranteedDrop(test, zombie, "Legs", "minecraft:diamond_leggings");
    // 3 件默认概率 0.085（recentlyHitByPlayer=false 门控不掉）。setEquipment 穿戴但不调
    // setEquipmentDropChance，保持默认 0.085（MobEntity 构造期 m_equipmentDropChances.fill(0.085)）。
    const equippable = (zombie as any).getComponent("minecraft:equippable");
    equippable.setEquipment("Feet", new ItemStack("minecraft:diamond_boots", 1) as unknown as any);
    equippable.setEquipment("Mainhand", new ItemStack("minecraft:diamond_sword", 1) as unknown as any);
    equippable.setEquipment("Offhand", new ItemStack("minecraft:shield", 1) as unknown as any);
  });

  test.runAtTickTime(8, () => {
    (test as any).kill(zombie);
  });

  // 轮询断言区域 item∈[3,5]（3 保整必掉 + rotten_flesh 0-2，3 默认门控不掉）。
  pollUntilSucceed(test, () => {
    const n = countItems(test);
    return n >= 3 && n <= 5;
  }, {
    startTick: 13,
    interval: 5,
    maxTick: 60,
    onTimeout: () => test.assert(false,
      `mob death should drop only 3 guaranteed equipment (3 default gated by !recentlyHitByPlayer), ` +
      `got ${countItems(test)} items (expected 3-5: 3 guaranteed + 0-2 rotten_flesh)`),
  });
}

// doMobLoot=false 时装备掉落被守卫拦截（shouldDropLoot 守卫负向，确定性 item==0）。
//
// 对齐 vanilla：dropCustomDeathLoot 在 LivingEntity.dropAllDeathLoot 的 shouldDropLoot 守卫【内】
// （!isChild() && doMobLoot）。doMobLoot=false 时守卫返 false，dropCustomDeathLoot 不调，6 件保整装备
// 不掉；dropFromLootTable 同在守卫内也不调，rotten_flesh 不掉。故区域 item==0（确定性，无 rotten_flesh 干扰）。
//
// 区别于 mobDeathDropsDisabledWhenDoMobLootFalse（MobDeathLootTests，验证 cow 物品掉落守卫）：本测试
// 专门验证【装备掉落】也在同一守卫内——确认 dropCustomDeathLoot 被守卫包住（而非在守卫外）。
//
// 【并行污染隔离】doMobLoot 世界级单例状态，独占 batch 串行 + runOnFinish 恢复 true
// （同 gametest-world-state-gamerule 隔离范式，见 MobDeathLootTests mob_loot_solo 注释）。
function mobEquipmentDropsBlockedByDoMobLootFalse(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "op");
  player.chat("/gamerule doMobLoot false");
  test.runOnFinish(() => {
    player.chat("/gamerule doMobLoot true");
  });

  const zombie = test.spawn("minecraft:zombie", { x: 4, y: 2, z: 4 });

  // 穿 6 件保整装备（保整在 doMobLoot=true 时必掉，doMobLoot=false 时被守卫拦截 → 验证守卫包住装备掉落）。
  test.runAtTickTime(1, () => {
    equipGuaranteedDrop(test, zombie, "Head", "minecraft:diamond_helmet");
    equipGuaranteedDrop(test, zombie, "Chest", "minecraft:diamond_chestplate");
    equipGuaranteedDrop(test, zombie, "Legs", "minecraft:diamond_leggings");
    equipGuaranteedDrop(test, zombie, "Feet", "minecraft:diamond_boots");
    equipGuaranteedDrop(test, zombie, "Mainhand", "minecraft:diamond_sword");
    equipGuaranteedDrop(test, zombie, "Offhand", "minecraft:shield");
  });

  // tick 10 等 doMobLoot=false 命令生效后杀死。
  test.runAtTickTime(10, () => {
    (test as any).kill(zombie);
  });

  // 轮询断言区域无 item（守卫拦截 dropCustomDeathLoot + dropFromLootTable，6 件保整装备 + rotten_flesh 全不掉）。
  pollUntilSucceed(test, () => countItems(test) === 0, {
    startTick: 15,
    interval: 5,
    maxTick: 50,
    onTimeout: () => test.assert(false,
      `mob equipment should not drop when doMobLoot=false, got ${countItems(test)} items`),
  });
}

export function registerMobEquipmentDropTests(): void {
  GameTest.register("MobBehaviorTests", "mob_drops_equipment_on_death_guaranteed", mobDropsEquipmentOnDeathGuaranteed)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "mob_default_chance_equipment_not_dropped_without_player", mobDefaultChanceEquipmentNotDroppedWithoutPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  // doMobLoot 世界级状态，独占 batch 串行避免污染同批依赖 mob 掉落的测试 + runOnFinish 恢复 true。
  GameTest.register("MobBehaviorTests", "mob_equipment_drops_blocked_by_do_mob_loot_false", mobEquipmentDropsBlockedByDoMobLootFalse)
    .structureName("gametests:glass_pit")
    .batch("mob_loot_solo")
    .maxTicks(80);
}
