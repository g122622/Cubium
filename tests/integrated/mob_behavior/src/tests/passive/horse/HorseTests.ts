// 马（Horse）行为类 GameTest。
//
// 覆盖马的骑乘、驯服、装鞍核心链路（对齐 MC Java 1.21.11 AbstractHorseEntity/HorseEntity）。
// 修复前 mob_behavior 包对普通马（horse）零集成测试覆盖（仅 SkeletonHorse/ZombieHorse/Llama 有测）。
// 普通马是玩家最常用的可骑乘生物，驯服骑乘体系是核心玩法，本测试补全覆盖。
//
// 框架补全（解锁马驯服状态脚本断言 + 已驯服马生成）：
//   1. is_tamed 组件原仅覆盖 TameableEntity（狼/猫/鹦鹉基类），马类 AbstractHorseEntity 不继承
//      TameableEntity（继承 AnimalEntity+IJumpingMount+IEquipable）但有独立 isTame()/setTame()，
//      故 getComponent("minecraft:is_tamed") 对马返 undefined，脚本无法断言马驯服。本次补
//      dynamic_cast<AbstractHorseEntity*> 分支（与 is_saddled 对马类的双路范式一致），使马驯服
//      状态可经 is_tamed.value 精确断言。
//   2. applySpawnEvent 新增 set_tamed spawn 事件（对齐 set_trap 范式）：test.spawn("horse<minecraft:set_tamed>", pos)
//      生成已驯服马（setTame(true)）。解锁装鞍测试——装鞍需先驯服，而驯服后玩家留在马上（setTamedBy 不甩人），
//      玩家在马上时 HorseEntity::interactMob:270 !isBeingRidden()=false 短路到基类不装鞍，故无法在同一次
//      驯服流程中接着装鞍。set_tamed 事件生成已驯服且无人骑的马，玩家持鞍 interact 直接走装鞍分支。
//
// C++ 链路（对齐 vanilla）：
//   - 空手 interactWithEntity(horse) → Player::interactOn → HorseEntity::interactMob（HorseEntity.cpp:263-298）：
//     空手时 item==nullptr 跳过食物/愤怒分支 → AbstractHorseEntity::interactMob（:570-608）→ doPlayerRide
//     （:606）→ player.startRiding(*this)。未驯服也能骑（doPlayerRide 不查 isTame，注释 :605）。
//   - 手持鞍 interactWithEntity(horse)：未驯服 HorseEntity::interactMob :287-290 !isTame() → makeMad
//     返回 Success（不装鞍）；已驯服跳过愤怒 → AbstractHorseEntity::interactMob :589-596
//     SaddleItem::itemInteractionForEntity 装鞍（setSaddle(true)）。故装鞍需先驯服 + 玩家不在马上。
//   - 驯服链路：玩家骑上未驯服马 → RunAroundLikeCrazyGoal（SpecialGoals.cpp:160-252）shouldExecute
//     (!isTame() && isBeingRidden()) → tick 每 tick 1/50 概率检查：rng.nextInt(maxTemper) < temper
//     则 setTamedBy(player) 驯服（不甩人，玩家留在马上）；否则 increaseTemper(5)（脾气+5）+
//     removePassengers（甩人）+ makeMad。temper 初始 0 单调递增，maxTemper=100，达 100 时
//     nextInt(100)<100 恒成立 → 必然驯服（约 20 次骑乘检查 × 平均 50 tick/次 ≈ 1000-2000 tick 收敛）。
//     驯服成功后玩家留在马上（setTamedBy 不甩人），故驯服与装鞍必须分两个测试。
//
// 设计要点：
//   1. 骑乘判定无 passengers API，用位置水平距离间接判定（复刻 PigTests 乘客附着范式）：
//      骑乘前玩家距马 2 格，骑乘后乘客附着马位置 dx²+dz²<1.0。
//   2. 驯服循环：玩家被甩下（removePassengers）后需重新 interactWithEntity 再骑。runAtTickTime 是单次执行
//      （poll.ts 注释），驯服循环用预注册检查点范式：预生成 tick 列表 [3,6,9,...] 逐个 runAtTickTime 注册，
//      每个检查点检测玩家是否仍在马上，不在则重新 interact，驱动反复骑乘直至驯服。避免运行时递归注册的
//      vector 扩容迭代器失效（poll.ts:16-22 警告）。
//   3. 装鞍测试用 set_tamed 事件生成已驯服马（绕开驯服后玩家在马上短路装鞍的时序死结），玩家持鞍 interact
//      后断言 is_saddled 组件存在（=== !undefined）。
//   4. 创造模式不消耗鞍/食物，可反复 interact。马空手 interact 走 doPlayerRide（不查 isTame）。
//   5. grass_pen（9×5×9 露天草地）参照 PigTests，马 (4,2,4) 脚踩草地存活，玩家 (2,2,4) 距 2 格。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: AbstractHorseEntity.cpp interactMob/doPlayerRide/setTamedBy/setTame、HorseEntity.cpp interactMob
// Ref: SpecialGoals.cpp RunAroundLikeCrazyGoal（驯服判定）、HorseStatusComponent.hpp（temper/maxTemper）

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

/** 判断玩家是否正骑在马上（位置水平距离<1.0，复刻 PigTests 乘客附着判定）。 */
function isPlayerRidingHorse(test: Test): boolean {
  const horse = firstEntity(test, "horse");
  const player = firstEntity(test, "minecraft:player");
  if (horse == null || player == null) return false;
  const dx = player.location.x - horse.location.x;
  const dz = player.location.z - horse.location.z;
  return dx * dx + dz * dz < 1.0;
}

// 玩家空手右键骑上未驯服的马（触发驯服流程的前置骑乘）。
//
// 空手 interactWithEntity(horse) → HorseEntity::interactMob 空手分支 → AbstractHorseEntity::interactMob
// → doPlayerRide → player.startRiding(*this)。未驯服也能骑（doPlayerRide 不查 isTame）。
// 判定：玩家骑上后位置附着马，dx²+dz²<1.0（骑乘前 2 格）。
//
// 注：马未驯服被骑后 RunAroundLikeCrazyGoal 会甩人，但 tick 5 interact 后短期内（首 1/50 检查前）
// 玩家仍在马上，轮询窗口 startTick=8..40 可捕获骑乘状态。本测试只验证"能骑上"这一前置行为，
// 不等待驯服（驯服由 horse_tamed_by_repeated_riding 覆盖）。
function horseRiddenByPlayerUntrained(test: Test): void {
  const horse = test.spawn("horse", { x: 4, y: 2, z: 4 });
  // 创造玩家 (2,2,4) 空手（不 setItem，主手空），距马 2 格。创造模式可 interact。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "rider");

  // tick 5 空手 interactWithEntity(horse) → doPlayerRide → startRiding。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(horse);
  });

  // 轮询断言玩家骑上马（位置距离<1.0）。startTick=8 留 3 tick 骑乘同步，maxTick=40 捕获甩人前窗口。
  pollUntilSucceed(test, () => isPlayerRidingHorse(test), {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const horse = firstEntity(test, "horse");
      const player = firstEntity(test, "minecraft:player");
      test.assert(false,
        `player did not ride untrained horse (doPlayerRide broken), `
        + `horseLoc=${JSON.stringify(horse?.location)} playerLoc=${JSON.stringify(player?.location)}`);
    },
  });
}

// 反复骑乘驯服马（验证 is_tamed 组件补全 + RunAroundLikeCrazyGoal 驯服链路）。
//
// 驯服循环：玩家被甩下后重新 interactWithEntity 再骑，驱动 RunAroundLikeCrazyGoal 反复检查累积 temper。
// temper 单调递增（每次检查失败 +5），maxTemper=100，达 100 时 nextInt(100)<100 恒成立 → 必然驯服。
// 驯服后 is_tamed.value===true（验证 is_tamed 组件对 AbstractHorseEntity 的补全分支）。
//
// 注：驯服成功 setTamedBy 不甩人（玩家留在马上），故本测试只验证驯服（is_tamed.value===true），
// 不接着装鞍——装鞍需玩家先下马，脚本无 dismount API，故装鞍由独立的 horse_saddled_when_tamed
// 测试用 set_tamed 事件生成已驯服马覆盖。
//
// 【关键：interact 无距离门控】Player::interactOn（Player.cpp:2881）与 Entity::startRiding
// （Entity.cpp:1677）均无距离检查——interactOn 直接调 processInitialInteract→interactMob→doPlayerRide
// →startRiding，startRiding 仅查自骑/循环/canBeRidden/canAddPassenger，不查玩家与马的距离。故即便马被
// RunAroundLikeCrazyGoal 乱跑到结构角落、玩家原地不动，interactWithEntity(horse) 仍能跨距离让玩家重新骑上。
// 这使驯服驱动极简：每个检查点无条件 interactWithEntity(horse) 即可——马未被骑时 doPlayerRide 重新骑上，
// 马已被骑（goal 还没甩人）时 interactMob 的 isBeingRidden 短路走基类（无副作用）。
//
// 【关键：骑乘冷却已移除】原项目自定义的 m_rideCooldown（60 tick 骑乘冷却，非 vanilla 机制）会阻止
// 甩人后立即重骑（canBeRidden 查冷却），使每次重新骑乘延迟 60 tick，驯服链路形同失效。该冷却已整体
// 移除（对齐 vanilla，详见 Entity.cpp addPassenger 注释），故甩人后玩家可立即重骑。
//
// 【弃用 teleport + 位置判定】早期方案用 isPlayerRidingHorse（位置水平距离<1.0）判定玩家是否在马上，
// 不在则 teleport 玩家到马位置再 interact。但甩人后玩家位置残留贴马（上一检查点 teleport 的痕迹），
// isPlayerRidingHorse 误判"仍在骑"→ 跳过 interact → 玩家永不重骑 → temper 停滞 → flaky。改用无条件
// interact（利用无距离门控）彻底消除该时序依赖。围栏方案（glass 围马防乱跑）亦弃用——马碰撞箱大能
// 穿单层围栏，且既然 interact 无距离门控，乱跑不再是问题。
//
// 时序：驯服需 ~20 次骑乘检查 × 平均 50 tick/次 ≈ 1000-2000 tick（1/50 概率 + temper 累积）。
// maxTicks=6000 留充裕余量（驯服方差大）。
//
// 【关键：runAtTickTime 是单次执行】（poll.ts 注释：在指定 tick 跑一次 callback），驯服循环不能用单次
// runAtTickTime。用预注册检查点范式（同 pollUntilSucceed）：预生成 tick 列表 [3,6,9,...,5997]，逐个
// runAtTickTime 注册，每个检查点无条件 interactWithEntity 驱动骑乘。避免运行时递归注册的 vector 扩容
// 迭代器失效（poll.ts:16-22 警告）。
//
// 【独占 batch】驯服测试 maxTicks=6000 长时序 + 大量 runAtTickTime 检查点，与同 grass_pen 区域的短测试
// 并行可能受 tick 调度影响致 flaky。独占 batch horse_tame_solo 串行跑，消除并行干扰。
function horseTamedByRepeatedRiding(test: Test): void {
  const horse = test.spawn("horse", { x: 4, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "tamer");

  // 预注册驯服循环检查点：tick 3,6,9,...,5997（每 3 tick），共 1999 个检查点。
  // 每个检查点无条件 interactWithEntity(horse)：马未被骑时 doPlayerRide 重新骑上（驱动 RunAroundLikeCrazyGoal
  // 反复检查累积 temper）；马已被骑时 interactMob isBeingRidden 短路走基类（无副作用）。
  // interactOn/startRiding 无距离门控，马乱跑到角落也能跨距离骑上，无需 teleport。
  // 驯服成功后 setTamedBy 不甩人，玩家留在马上，后续 interact 走基类无害，由下方 pollUntilSucceed
  // 检测 is_tamed.value===true 终止。
  const driveTicks: number[] = [];
  for (let t = 3; t < 5997; t += 3) {
    driveTicks.push(t);
  }
  for (const tick of driveTicks) {
    test.runAtTickTime(tick, () => {
      (player as any).interactWithEntity(horse);
    });
  }

  // 轮询断言驯服成功：is_tamed 组件 value===true（验证组件补全 + 驯服链路）。
  // startTick=40 留驯服时序，maxTick=5980 在 driveTicks 范围内。
  pollUntilSucceed(test, () => {
    const h = firstEntity(test, "horse");
    if (h == null) return false;
    const tamedComp = (h as any).getComponent("minecraft:is_tamed");
    return tamedComp !== undefined && tamedComp.value === true;
  }, {
    startTick: 40,
    interval: 10,
    maxTick: 5980,
    onTimeout: () => {
      const h = firstEntity(test, "horse");
      const tamedComp = h != null ? (h as any).getComponent("minecraft:is_tamed") : null;
      const riding = isPlayerRidingHorse(test);
      test.assert(false,
        `horse not tamed after repeated riding (RunAroundLikeCrazyGoal broken or is_tamed comp missing), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `playerRiding=${riding}`);
    },
  });
}

// 已驯服马持鞍装鞍（验证 set_tamed spawn 事件 + AbstractHorseEntity::interactMob 装鞍链路 + is_saddled 组件）。
//
// 用 set_tamed spawn 事件生成已驯服马（test.spawn("horse<minecraft:set_tamed>", pos) → applySpawnEvent
// 调 setTame(true)），绕开驯服流程。玩家持鞍 interactWithEntity → HorseEntity::interactMob 已驯服跳过
// makeMad → AbstractHorseEntity::interactMob :589-596 SaddleItem::itemInteractionForEntity 装鞍
// （setSaddle(true)）。马未被骑（玩家不先骑），HorseEntity::interactMob:270 !isBeingRidden()=true 不短路，
// 走完整装鞍分支。
//
// 判定：装鞍后 is_saddled 组件存在（=== !undefined）+ is_tamed.value===true（双重断言确认 set_tamed 生效）。
function horseSaddledWhenTamed(test: Test): void {
  // set_tamed 事件生成已驯服马（setTame(true)），无需先驯服。
  const horse = test.spawn("horse<minecraft:set_tamed>", { x: 4, y: 2, z: 4 });
  // 创造玩家 (2,2,4) 持鞍，距马 2 格（不骑，避免 interactMob isBeingRidden 短路）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "saddler");
  const saddle = new ItemStack("minecraft:saddle", 1);
  // slot 0=主手，参照 SaddleConsumptionTests 装鞍范式。
  player.setItem(saddle as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 持鞍 interactWithEntity → AbstractHorseEntity::interactMob → SaddleItem::itemInteractionForEntity 装鞍。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(horse);
  });

  // 轮询断言装鞍成功：is_saddled 组件存在 + is_tamed.value===true（双重断言）。
  pollUntilSucceed(test, () => {
    const h = firstEntity(test, "horse");
    if (h == null) return false;
    const tamedComp = (h as any).getComponent("minecraft:is_tamed");
    const saddledComp = (h as any).getComponent("minecraft:is_saddled");
    return tamedComp !== undefined && tamedComp.value === true && saddledComp !== undefined;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const h = firstEntity(test, "horse");
      const tamedComp = h != null ? (h as any).getComponent("minecraft:is_tamed") : null;
      const saddledComp = h != null ? (h as any).getComponent("minecraft:is_saddled") : null;
      test.assert(false,
        `tamed horse not saddled (SaddleItem::itemInteractionForEntity broken or set_tamed event missing), `
        + `is_tamed=${tamedComp === undefined ? "undefined" : `value=${tamedComp.value}`}, `
        + `is_saddled=${saddledComp === undefined ? "undefined" : "present"}`);
    },
  });
}

export function registerHorseTests(): void {
  GameTest.register("MobBehaviorTests", "horse_ridden_by_player_untrained", horseRiddenByPlayerUntrained)
    .structureName("gametests:grass_pen")
    .maxTicks(80);

  // 驯服周期长（temper 累积 ~1000-2000 tick + 方差），maxTicks=6000 留充裕余量。
  // 独占 batch horse_tame_solo 串行跑，避免与同 grass_pen 区域短测试并行致 tick 调度 flaky。
  GameTest.register("MobBehaviorTests", "horse_tamed_by_repeated_riding", horseTamedByRepeatedRiding)
    .structureName("gametests:grass_pen")
    .batch("horse_tame_solo")
    .maxTicks(6000);

  GameTest.register("MobBehaviorTests", "horse_saddled_when_tamed", horseSaddledWhenTamed)
    .structureName("gametests:grass_pen")
    .maxTicks(80);
}
