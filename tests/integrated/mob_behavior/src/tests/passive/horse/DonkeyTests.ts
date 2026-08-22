// 驴（Donkey）/骡（Mule）行为类 GameTest。
//
// 覆盖驴/骡的箱子装备核心链路（对齐 MC Java 1.21.11 AbstractChestedHorseEntity）。
// 驴/骡继承 AbstractChestedHorseEntity（区别于普通马 HorseEntity），可装备箱子扩展背包（15 格）。
// 修复前 mob_behavior 包对驴/骡零集成测试覆盖（仅普通马 HorseTests/LlamaTests/SkeletonHorse/ZombieHorse 有测）。
// 装箱是驴/骡区别于普通马的核心功能，本测试补全覆盖。
//
// 框架补全（解锁驴/骡箱子状态脚本断言）：
//   is_chested 组件原未绑定（基岩 EntityIsChestedComponent，componentId="minecraft:is_chested"），
//   脚本无法断言驴/骡是否已装箱。本次补 dynamic_cast<AbstractChestedHorseEntity*> + hasChest() 分支
//   （对齐 is_saddled/is_tamed 范式），使驴/骡箱子状态可经 getComponent("minecraft:is_chested") 存在性断言。
//
// C++ 链路（对齐 vanilla AbstractChestedHorseEntity::interactMob，AbstractChestedHorseEntity.cpp:40-81）：
//   - 未驯服驴持箱子 interactWithEntity → interactMob :64-67 !isTame() → makeMad 返回 Success（不装箱）。
//   - 已驯服驴持箱子 interactWithEntity → interactMob :70-72 !hasChest() && item==CHEST → equipChest
//     （setChest(true) + 音效 + 消耗箱子）。需先驯服（isSaddleable=isTame 同款守卫，且未驯服走 makeMad 分支）。
//   - 已驯服驴空手 interactWithEntity → 委托 AbstractHorseEntity::interactMob → doPlayerRide（骑乘）。
//
// 设计要点：
//   1. set_tamed spawn 事件生成已驯服驴（test.spawn("donkey<minecraft:set_tamed>", pos) → setTame(true)），
//      绕开驯服流程（驯服链路由 horse_tamed_by_repeated_riding 覆盖，驴/骡驯服机制与普通马同构不重复测）。
//   2. 装箱判定用 is_chested 组件存在性（=== !undefined），对齐基岩 EntityIsChestedComponent 存在性语义。
//   3. 创造模式不消耗箱子，可反复 interact。玩家持箱子 setItem(slot=0 主手)。
//   4. grass_pen（9×5×9 露天草地）参照 HorseTests，驴 (4,2,4) 脚踩草地存活，玩家 (2,2,4) 距 2 格。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: AbstractChestedHorseEntity.cpp interactMob/equipChest/setChest/hasChest

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

/** 取区域内指定类型实体的首个（区域限定避免批内并行污染）。 */
function firstEntity(test: Test, type: string): any | null {
  const arr = test.getDimension().getEntities({
    type,
    location: test.worldLocation(PEN_FROM),
    volume: PEN_VOLUME,
  });
  return arr.length > 0 ? arr[0] : null;
}

// 已驯服驴持箱子装箱（验证 set_tamed spawn 事件 + AbstractChestedHorseEntity::equipChest 链路 + is_chested 组件）。
//
// 用 set_tamed spawn 事件生成已驯服驴（绕开驯服流程），玩家持箱子 interactWithEntity → interactMob
// 已驯服跳过 makeMad → :70-72 !hasChest() && item==CHEST → equipChest（setChest(true)）。
//
// 判定：装箱后 is_chested 组件存在（=== !undefined）+ is_tamed.value===true（双重断言确认 set_tamed 生效）。
function donkeyChestedWhenTamed(test: Test): void {
  // set_tamed 事件生成已驯服驴（setTame(true)），无需先驯服。
  const donkey = test.spawn("donkey<minecraft:set_tamed>", { x: 4, y: 2, z: 4 });
  // 创造玩家 (2,2,4) 持箱子，距驴 2 格。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "chester");
  const chest = new ItemStack("minecraft:chest", 1);
  // slot 0=主手，参照 SaddleConsumptionTests 装备范式。
  player.setItem(chest as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 持箱 interactWithEntity → AbstractChestedHorseEntity::interactMob → equipChest 装箱。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(donkey);
  });

  // 轮询断言装箱成功：is_chested 组件存在 + is_tamed.value===true（双重断言）。
  pollUntilSucceed(test, () => {
    const d = firstEntity(test, "donkey");
    if (d == null) return false;
    const tamedComp = (d as any).getComponent("minecraft:is_tamed");
    const chestedComp = (d as any).getComponent("minecraft:is_chested");
    return tamedComp !== undefined && tamedComp.value === true && chestedComp !== undefined;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const d = firstEntity(test, "donkey");
      const tamedComp = d != null ? (d as any).getComponent("minecraft:is_tamed") : null;
      const chestedComp = d != null ? (d as any).getComponent("minecraft:is_chested") : null;
      test.assert(false,
        `tamed donkey not chested (equipChest broken or is_chested comp missing), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `is_chested=${chestedComp === undefined ? "undefined" : "present"}`);
    },
  });
}

// 未驯服驴持箱子不装箱（验证 interactMob !isTame() makeMad 守卫）。
//
// 未驯服驴持箱子 interactWithEntity → interactMob :64-67 !isTame() → makeMad 返回 Success（不装箱）。
// 对照组：确认未驯服守卫生效，区别于 donkey_chested_when_tamed 的已驯服装箱路径。
//
// 判定：interact 后 is_chested 组件不存在（未装箱）。is_tamed.value===false 确认未驯服。
function donkeyUntrainedRejectsChest(test: Test): void {
  // 普通 spawn 生成未驯服驴（默认 isTame=false）。
  const donkey = test.spawn("donkey", { x: 4, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "chestrej");
  const chest = new ItemStack("minecraft:chest", 1);
  player.setItem(chest as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 持箱 interactWithEntity → interactMob !isTame() → makeMad（不装箱）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(donkey);
  });

  // 轮询断言未装箱：is_chested 组件不存在 + is_tamed.value===false。
  // maxTick=40 在 makeMad 后稳定，未驯服驴不会装箱。
  pollUntilSucceed(test, () => {
    const d = firstEntity(test, "donkey");
    if (d == null) return false;
    const tamedComp = (d as any).getComponent("minecraft:is_tamed");
    const chestedComp = (d as any).getComponent("minecraft:is_chested");
    return tamedComp !== undefined && tamedComp.value === false && chestedComp === undefined;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const d = firstEntity(test, "donkey");
      const tamedComp = d != null ? (d as any).getComponent("minecraft:is_tamed") : null;
      const chestedComp = d != null ? (d as any).getComponent("minecraft:is_chested") : null;
      test.assert(false,
        `untrained donkey should not be chested (makeMad guard broken), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `is_chested=${chestedComp === undefined ? "undefined" : "present"}`);
    },
  });
}

// 骡（mule）与驴同属 AbstractChestedHorseEntity，装箱链路同构。用骡验证装箱覆盖骡子类
// （区别于 donkey_chested_when_tamed 仅测驴，确保 MuleEntity 不被遗漏）。
function muleChestedWhenTamed(test: Test): void {
  const mule = test.spawn("mule<minecraft:set_tamed>", { x: 4, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "mulechester");
  const chest = new ItemStack("minecraft:chest", 1);
  player.setItem(chest as unknown as Parameters<typeof player.setItem>[0], 0, true);

  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(mule);
  });

  pollUntilSucceed(test, () => {
    const m = firstEntity(test, "mule");
    if (m == null) return false;
    const tamedComp = (m as any).getComponent("minecraft:is_tamed");
    const chestedComp = (m as any).getComponent("minecraft:is_chested");
    return tamedComp !== undefined && tamedComp.value === true && chestedComp !== undefined;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const m = firstEntity(test, "mule");
      const tamedComp = m != null ? (m as any).getComponent("minecraft:is_tamed") : null;
      const chestedComp = m != null ? (m as any).getComponent("minecraft:is_chested") : null;
      test.assert(false,
        `tamed mule not chested (equipChest broken for mule or is_chested comp missing), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `is_chested=${chestedComp === undefined ? "undefined" : "present"}`);
    },
  });
}

export function registerDonkeyTests(): void {
  GameTest.register("MobBehaviorTests", "donkey_chested_when_tamed", donkeyChestedWhenTamed)
    .structureName("gametests:grass_pen")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "donkey_untrained_rejects_chest", donkeyUntrainedRejectsChest)
    .structureName("gametests:grass_pen")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "mule_chested_when_tamed", muleChestedWhenTamed)
    .structureName("gametests:grass_pen")
    .maxTicks(80);
}
