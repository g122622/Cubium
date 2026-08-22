// 羊驼（Llama）装箱行为 GameTest。
//
// 覆盖羊驼的箱子装备链路（对齐 MC Java 1.21.11 AbstractChestedHorseEntity）。
// 羊驼继承 AbstractChestedHorseEntity（与驴/骡同构），可装备箱子扩展背包。
// 修复前 LlamaTests 仅覆盖吐口水/防狼，装箱链路（equipChest）零覆盖。
// 驴/骡装箱已由 DonkeyTests 覆盖，本测试补全羊驼这一独立实体类型的装箱覆盖，
// 关闭 mob_behavior 包"箱实体（AbstractChestedHorseEntity 子类）装箱"覆盖的最后一个缺口。
//
// 框架依赖（已在先前提交补全）：
//   - is_chested 脚本组件（MinecraftModuleFactory.cpp）：dynamic_cast<AbstractChestedHorseEntity*>
//     + hasChest() 存在性判定，羊驼命中（继承 AbstractChestedHorseEntity）。
//   - set_tamed spawn 事件（GameTestHelper.cpp）：test.spawn("llama<minecraft:set_tamed>", pos)
//     → setTame(true)。set_tamed 的 dynamic_cast<AbstractHorseEntity*> 覆盖羊驼（羊驼继承
//     AbstractHorseEntity→AbstractChestedHorseEntity），故生成已驯服羊驼，绕开驯服流程
//     （羊驼驯服机制与普通马同构 RunAroundLikeCrazyGoal，由 horse_tamed_by_repeated_riding
//     覆盖，此处不重复测驯服）。
//
// C++ 链路（AbstractChestedHorseEntity::interactMob，AbstractChestedHorseEntity.cpp:40-81）：
//   - 未驯服羊驼持箱子 interactWithEntity → interactMob :64-67 !isTame() → makeMad 返回 Success（不装箱）。
//   - 已驯服羊驼持箱子 interactWithEntity → interactMob :70-72 !hasChest() && item==CHEST →
//     equipChest（setChest(true) + 音效 + 消耗箱子）。
//
// 设计要点（照搬 DonkeyTests.ts 范式）：
//   1. set_tamed 事件生成已驯服羊驼，绕开驯服流程。
//   2. 装箱判定用 is_chested 组件存在性（=== !undefined），对齐基岩 EntityIsChestedComponent。
//   3. 创造模式不消耗箱子，可反复 interact。玩家持箱子 setItem(slot=0 主手)。
//   4. grass_pen（9×5×9 露天草地）参照 DonkeyTests，羊驼 (4,2,4) 脚踩草地存活，玩家 (2,2,4) 距 2 格。
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

// 已驯服羊驼持箱子装箱（验证 set_tamed spawn 事件 + AbstractChestedHorseEntity::equipChest 链路 + is_chested 组件）。
//
// 用 set_tamed spawn 事件生成已驯服羊驼，玩家持箱子 interactWithEntity → interactMob 已驯服跳过
// makeMad → :70-72 !hasChest() && item==CHEST → equipChest（setChest(true)）。
//
// 判定：装箱后 is_chested 组件存在（=== !undefined）+ is_tamed.value===true（双重断言确认 set_tamed 生效）。
function llamaChestedWhenTamed(test: Test): void {
  // set_tamed 事件生成已驯服羊驼（setTame(true)），无需先驯服。
  const llama = test.spawn("llama<minecraft:set_tamed>", { x: 4, y: 2, z: 4 });
  // 创造玩家 (2,2,4) 持箱子，距羊驼 2 格。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "llamachester");
  const chest = new ItemStack("minecraft:chest", 1);
  // slot 0=主手，参照 DonkeyTests 装备范式。
  player.setItem(chest as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 持箱 interactWithEntity → AbstractChestedHorseEntity::interactMob → equipChest 装箱。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(llama);
  });

  // 轮询断言装箱成功：is_chested 组件存在 + is_tamed.value===true（双重断言）。
  pollUntilSucceed(test, () => {
    const l = firstEntity(test, "llama");
    if (l == null) return false;
    const tamedComp = (l as any).getComponent("minecraft:is_tamed");
    const chestedComp = (l as any).getComponent("minecraft:is_chested");
    return tamedComp !== undefined && tamedComp.value === true && chestedComp !== undefined;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const l = firstEntity(test, "llama");
      const tamedComp = l != null ? (l as any).getComponent("minecraft:is_tamed") : null;
      const chestedComp = l != null ? (l as any).getComponent("minecraft:is_chested") : null;
      test.assert(false,
        `tamed llama not chested (equipChest broken for llama or is_chested comp missing), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `is_chested=${chestedComp === undefined ? "undefined" : "present"}`);
    },
  });
}

// 未驯服羊驼持箱子不装箱（验证 interactMob !isTame() makeMad 守卫）。
//
// 未驯服羊驼持箱子 interactWithEntity → interactMob :64-67 !isTame() → makeMad 返回 Success（不装箱）。
// 对照组：确认未驯服守卫生效，区别于 llamaChestedWhenTamed 的已驯服装箱路径。
//
// 判定：interact 后 is_chested 组件不存在（未装箱）。is_tamed.value===false 确认未驯服。
function llamaUntrainedRejectsChest(test: Test): void {
  // 普通 spawn 生成未驯服羊驼（默认 isTame=false）。
  const llama = test.spawn("llama", { x: 4, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "llamarej");
  const chest = new ItemStack("minecraft:chest", 1);
  player.setItem(chest as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 持箱 interactWithEntity → interactMob !isTame() → makeMad（不装箱）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(llama);
  });

  // 轮询断言未装箱：is_chested 组件不存在 + is_tamed.value===false。
  pollUntilSucceed(test, () => {
    const l = firstEntity(test, "llama");
    if (l == null) return false;
    const tamedComp = (l as any).getComponent("minecraft:is_tamed");
    const chestedComp = (l as any).getComponent("minecraft:is_chested");
    return tamedComp !== undefined && tamedComp.value === false && chestedComp === undefined;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const l = firstEntity(test, "llama");
      const tamedComp = l != null ? (l as any).getComponent("minecraft:is_tamed") : null;
      const chestedComp = l != null ? (l as any).getComponent("minecraft:is_chested") : null;
      test.assert(false,
        `untrained llama should not be chested (makeMad guard broken), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `is_chested=${chestedComp === undefined ? "undefined" : "present"}`);
    },
  });
}

export function registerLlamaChestTests(): void {
  GameTest.register("MobBehaviorTests", "llama_chested_when_tamed", llamaChestedWhenTamed)
    .structureName("gametests:grass_pen")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "llama_untrained_rejects_chest", llamaUntrainedRejectsChest)
    .structureName("gametests:grass_pen")
    .maxTicks(80);
}
