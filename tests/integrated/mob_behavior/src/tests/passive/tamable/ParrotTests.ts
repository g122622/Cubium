// 鹦鹉行为类 GameTest。
//
// 覆盖 wiki tech_鹦鹉.txt 核心驯服链路：手持种子右键未驯服鹦鹉，1/10 概率驯服成功（setTamed(true)）。
//
// C++ 链路（对齐 Java 1.21.11 Parrot.mobInteract，已逐段核查）：
//   1) 玩家持种子 interactWithEntity(鹦鹉) → Player::interactOn → MobEntity::processInitialInteract
//      → ParrotEntity::interactMob（ParrotEntity.cpp:156）分支 `!isTamed() && isTameItem(itemStack)`
//      （:162）命中。
//   2) isTameItem（:68）：WHEAT_SEEDS/PUMPKIN_SEEDS/MELON_SEEDS/BEETROOT_SEEDS 之一。
//   3) 非创造 shrink(1) 消耗种子 + 播放 ENTITY_PARROT_EAT 音效。
//   4) 服务端 rng.nextInt(10)==0（1/10 概率）→ setTamed(true) + setOwnerId(player.uuidBytes())
//      + onTameAnimal + broadcastEntityStatus(TamingSucceeded 心形粒子)。失败广播烟雾粒子。
//   5) 返回 Success。
//
// 判定手段：getComponent("minecraft:is_tamed").value === true（驯服成功）。
// is_tamed 组件经 TameableEntity::isTamed()（DATA_FLAGS_PARAM 位 4）读取，由本次会话补全的
// IsTamedComponent 绑定（MinecraftModuleFactory.cpp getComponent "minecraft:is_tamed" 分支）。
// 此前 WolfTests 驯服判定用 effectiveMax>=20（驯服后血量上限 8→20）间接断言，hacky 且不适用
// 鹦鹉（驯服不改血量）；补 is_tamed 组件后鹦鹉可精确断言驯服状态。
//
// 驯服概率 1/10，单次非确定。创造模式不消耗种子可反复喂。tick 5..200 每 3 tick 喂一次（约65次），
// 65 次内驯服概率 1-(9/10)^65 ≈ 99.88%。pollUntilSucceed 轮询 is_tamed=true，maxTick=700 留足周期。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。鹦鹉会飞，开放坑（creeper_pit）可能飞出区域致 getEntities
// 查询丢失；glass_pen 玻璃墙围住鹦鹉不飞走。Survival 玩家 (2,2,4) 持种子，鹦鹉 (5,2,4)（距 3 格，
// interactWithEntity 远程触发无距离门控）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鹦鹉.txt#驯服（喂种子 1/10 概率驯服）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const PARROT = "minecraft:parrot";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 读鹦鹉 is_tamed 组件 value（true=已驯服）。
function isParrotTamed(parrot: any): boolean {
  const comp = parrot.getComponent("minecraft:is_tamed") as any;
  return comp?.value === true;
}

// 玩家持种子反复右键未驯服鹦鹉，1/10 概率驯服成功（wiki tech_鹦鹉.txt#驯服）。
//
// 创造玩家持小麦种子，tick 5..200 每 3 tick interactWithEntity(鹦鹉) 喂食一次（约65次）。
// 创造模式不消耗种子可反复喂。每次喂食触发 interactMob 驯服分支，1/10 概率 setTamed(true)。
// pollUntilSucceed 轮询 is_tamed=true（65 次内驯服概率 ~99.88%）。
//
// 鹦鹉 (5,2,4)（grass_pen y=0 grass_block 地板，helper y=2→结构 y=1 空气，脚踩 y=0 grass_block）。
// 鹦鹉会飞但 grass_pen 玻璃墙围住不飞出区域。Survival 玩家用创造模式喂食（不消耗种子反复喂）。
// 注：spawnSimulatedPlayer 第三参默认创造模式（不传 gameMode）。
function parrotTamedBySeedsTest(test: Test): void {
  // 鹦鹉 (5,2,4)，创造玩家 (2,2,4) 持小麦种子。
  const parrot = test.spawn(PARROT, { x: 5, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "parrotTamer");
  const seeds = new ItemStack("minecraft:wheat_seeds", 1);
  player.setItem(seeds as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5..200 每 3 tick 喂一次种子（约65次，1/10 概率驯服，65 次内成功率 ~99.88%）。
  // 创造模式不消耗种子可反复喂。驯服后再喂走 isTamed&&isOwner 分支 toggleSitting（无害）。
  for (let t = 5; t <= 200; t += 3) {
    test.runAtTickTime(t, () => {
      (player as any).interactWithEntity(parrot);
    });
  }

  // 轮询断言：鹦鹉 is_tamed.value === true（驯服成功）。
  // startTick=10 等 1-2 次喂食后开始查，interval=3 与喂食周期对齐，maxTick=700 留足 65 次喂食周期。
  pollUntilSucceed(test, () => {
    return isParrotTamed(parrot);
  }, {
    startTick: 10,
    interval: 3,
    maxTick: 700,
    onTimeout: () => {
      test.assert(false,
        `parrot not tamed after feeding seeds 65 times (1/10 chance, `
        + `is_tamed=${isParrotTamed(parrot)} expected true)`);
    },
  });
}

// 对照：未驯服鹦鹉 is_tamed.value === false（验证组件默认值正确，排除"组件恒 true"假通过）。
//
// spawn 鹦鹉但不喂食，断言 is_tamed.value === false。验证 is_tamed 组件对未驯服鹦鹉返回 false
// （而非恒 true 致 parrotTamedBySeedsTest 假通过）。maxTick=40 短窗，不喂食鹦鹉保持未驯服。
function parrotUntamedByDefaultTest(test: Test): void {
  const parrot = test.spawn(PARROT, { x: 5, y: 2, z: 4 });

  // tick 20 断言未驯服（is_tamed.value === false）+ succeed。
  test.runAtTickTime(20, () => {
    test.assert(!isParrotTamed(parrot),
      `parrot should be untamed by default, is_tamed=${isParrotTamed(parrot)} (expected false)`);
    test.succeed();
  });
}

// 读鹦鹉 is_sitting 组件 value（true=已坐下，undefined=组件不存在即绑定缺失）。
// is_sitting 组件经 TameableEntity::isSitting()（m_sitting 字段）读取，由本次补全的
// IsSittingComponent 绑定（MinecraftModuleFactory.cpp getComponent "minecraft:is_sitting" 分支）。
// 返回 undefined（而非 false）以区分"组件不存在"与"值为 false"——前者说明绑定缺失需诊断。
function isParrotSitting(parrot: any): boolean | undefined {
  const comp = parrot.getComponent("minecraft:is_sitting") as any;
  if (comp === undefined || comp === null) {
    return undefined;
  }
  return comp.value === true;
}

// 已驯服鹦鹉右键切换坐下状态（wiki tech_鹦鹉.txt#行为：已驯服的鹦鹉可以通过右键切换坐下/站立）。
//
// C++ 链路（对齐 Java 1.21.11 Parrot.mobInteract，ParrotEntity.cpp:198-203）：
//   玩家 interactWithEntity(已驯服鹦鹉) → Player::interactOn → processInitialInteract
//   → ParrotEntity::interactMob：
//     - 分支1 `!isTamed() && isTameItem`（:162）：已驯服时 !isTamed()=false 跳过；
//     - 分支2 `isTamed() && isOwner(player)`（:199）命中 → toggleSitting()（TameableEntity.hpp:218
//       setSitting(!m_sitting)）翻转 m_sitting + DATA_FLAGS_PARAM 位 0 → 返回 Success。
//   分支2 不检查手持物品（区别分支1需 isTameItem），故驯服后无论持种子或空手右键均走分支2 toggleSitting。
//   toggleSitting 是翻转（非设为 true），故连续两次右键恢复原状。
//
// 判定手段：读 is_sitting 组件 value。先喂种子驯服鹦鹉（复用 parrotTamedBySeedsTest 范式），驯服后
// 继续右键触发分支2 toggleSitting。读 toggle 前的 sitting 基线，右键一次，断言 is_sitting 翻转
// （=== !基线）。验证 toggleSitting 真正"切换"而非"恒设 true"。
//
// 时序设计（全固定 runAtTickTime，不用 pollUntilSucceed）：
//   pollUntilSucceed 满足条件即调 test.succeed() 终止整个测试（poll.ts:80-83），无法在驯服后继续执行
//   后续 toggle 阶段。故改用固定时序：
//   - tick 5..200 每 3 tick 喂种子（约65次，1/10 概率驯服，65 次内成功率 ~99.88%）；
//   - tick 215 兜底检查 is_tamed=true（驯服失败则 assert false 报清晰错误，~0.12% flaky）；
//   - tick 225 读 sitting 基线 baseline（tick 200 后不再喂食，状态稳定）；
//   - tick 235 interactWithEntity 触发 toggleSitting（翻转）；
//   - tick 245 断言 is_sitting === !baseline + succeed。
//
// 驯服后 sitting 基线不可假设：驯服发生在 tick T（喂食循环内某次），T+3..200 间每次喂食走分支2
// toggleSitting，翻转次数 = floor((200-T-3)/3)+1 取决于 T 奇偶性，故基线 true/false 不定。读实际基线
// 后断言翻转——基线不确定但翻转确定，测试稳健。无需清空主手（分支2 不检查物品）。
// 玩家创造模式（不消耗种子可反复喂），鹦鹉 grass_pen 玻璃墙围住不飞走。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鹦鹉.txt#行为（已驯服右键切换坐下）
// Ref: ParrotEntity.cpp:198-203（interactMob 坐下切换分支）/ TameableEntity.hpp:218（toggleSitting）
function parrotTogglesSittingWhenTamedTest(test: Test): void {
  const parrot = test.spawn(PARROT, { x: 5, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "parrotSitter");
  const seeds = new ItemStack("minecraft:wheat_seeds", 1);
  player.setItem(seeds as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 阶段一：tick 5..200 每 3 tick 喂种子驯服（约65次，成功率 ~99.88%）。
  // 创造模式不消耗种子可反复喂。驯服后继续喂走分支2 toggleSitting（无害，基线读实际值）。
  for (let t = 5; t <= 200; t += 3) {
    test.runAtTickTime(t, () => {
      (player as any).interactWithEntity(parrot);
    });
  }

  // 阶段二：tick 215 兜底检查驯服成功（驯服失败则 assert false 报清晰错误，~0.12% flaky）。
  test.runAtTickTime(215, () => {
    test.assert(isParrotTamed(parrot),
      `parrot not tamed before sitting toggle test (1/10 chance, `
      + `is_tamed=${isParrotTamed(parrot)} expected true)`);
  });

  // 阶段三：tick 225 读 sitting 基线 baseline（tick 200 后不再喂食，状态稳定）。
  // 闭包变量记录基线供阶段五断言。undefined 说明 is_sitting 绑定缺失（诊断用）。
  let baseline: boolean | undefined = undefined;
  test.runAtTickTime(225, () => {
    baseline = isParrotSitting(parrot);
    test.assert(baseline !== undefined,
      `parrot has no is_sitting component (binding missing)`);
  });

  // 阶段四：tick 235 interactWithEntity 触发 toggleSitting（翻转）。无需清空主手（分支2不检查物品）。
  test.runAtTickTime(235, () => {
    (player as any).interactWithEntity(parrot);
  });

  // 阶段五：tick 245 断言 is_sitting 翻转（=== !baseline）+ succeed。
  // 留 10 tick 让 interactMob 链路执行 + toggleSitting 同步生效。
  test.runAtTickTime(245, () => {
    const after = isParrotSitting(parrot);
    if (baseline === undefined) {
      test.assert(false, `baseline not captured, is_sitting after=${after}`);
      return;
    }
    test.assert(after === !baseline,
      `parrot did not toggle sitting, is_sitting before=${baseline} after=${after} `
      + `(expected after === !before)`);
    test.succeed();
  });
}

// 已驯服鹦鹉跟随主人（wiki tech_鹦鹉.txt#行为：已驯服的鹦鹉会跟随主人，主人在远处时鹦鹉会飞过去）。
//
// 本测试验证 FollowOwnerGoal（ParrotEntity.cpp:126 注册 FollowOwnerGoal(this, 1.0, 5.0f, 10.0f, 32.0f)）。
// 现有测试覆盖驯服与坐下切换，但未覆盖驯服后的跟随行为——这是"goal 已注册且逻辑完整、但现有测试未覆盖"
// 的典型缺口。
//
// C++ 链路（对齐 Java 1.21.11 FollowOwnerGoal，TameableGoals.cpp:65-100）：
//   1) FollowOwnerGoal::shouldExecute：isTamed() && !isSitting() && getOwner() 非空 &&
//      distanceTo(owner) > minDistance(5.0) → 返回 true。
//   2) FollowOwnerGoal::tick：periodically tryMoveTo(owner.x/y/z, speed) 寻路接近主人；
//      distance > teleportDistance(32) 时 _teleportToOwner() 传送（grass_pen 9×9 对角 ~11 格 < 32，不触发）。
//   3) shouldContinueExecuting：distance < minDistance 时停止（鹦鹉接近到 ~5 格停）。
//
// **关键设计——远程喂食驯服以规避 LandOnOwnersShoulderGoal 抢占**：
//   鹦鹉驯服后若主人近距，LandOnOwnersShoulderGoal（优先级3，Move flag）会启动，其 tick 检查碰撞箱与
//   主人相交时调 mountShoulder()，鹦鹉坐肩（m_isSittingOnShoulder=true）。坐肩后 isPreemptible()=false，
//   FollowOwnerGoal（优先级2，同 Move flag）虽优先级更高却无法抢占（isPreemptedBy 检查 isPreemptible
//   先返回 false），永不启动。且 Cubium shoulder riding 仅是状态标记未接入位置同步，鹦鹉坐肩后逻辑位置
//   卡在原地（mountShoulder 只设 m_onShoulder/m_shoulderPlayerId，无 startRiding 位置跟随）。
//
//   规避：interactWithEntity 远程触发 interactMob 无距离门控，故让玩家始终站在远距 (1,2,1) 远程喂食驯服
//   鹦鹉 (7,2,7)（相距 ~8.5 > minDistance=5）。玩家与鹦鹉碰撞箱永不相交 → LandOnOwnersShoulderGoal.tick
//   的 mountShoulder 永不触发 → m_isSittingOnShoulder 保持 false → isPreemptible=true。
//   Phase 2 goalUpdate 遍历顺序：FollowOwnerGoal(2) 在 LandOnOwnersShoulderGoal(3) 之前评估——初始 Move
//   flag 空闲，驯服后 dist>5 使 FollowOwnerGoal::shouldExecute=true 先启动占 Move flag；LandOnOwnersShoulderGoal
//   后评估时 Move flag 已被同 flag 的 FollowOwnerGoal 占据，且其优先级3 > 2 无法抢占（isPreemptedBy:
//   3<2=false）→ LandOnOwnersShoulderGoal 不启动。FollowOwnerGoal 独占 Move flag 驱动鹦鹉飞向主人。
//
//   注：驯服后继续远程喂食走 interactMob 分支2 toggleSitting 翻转 m_sitting。若 sitting=true 则
//   FollowOwnerGoal::shouldExecute 的 !isSitting() 检查返回 false 不启动。故须确保鹦鹉站立：
//   tick 220 兜底检查驯服 + 若 sitting=true 则 interact 一次翻转 false。toggleSitting 同步设 m_sitting
//   + DATA_FLAGS，下一 GoalSelector 评估（每 2 tick）即生效。tick 230 起轮询跟随。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。鹦鹉会飞，玻璃墙围住不飞出查询区域。
//   鹦鹉 (7,2,7)（grass_pen 一角，helper y=2→结构 y=1 air，脚踩 y=0 grass_block）。
//   玩家 (1,2,1)（对角，距鹦鹉 ~8.5 格 > minDistance=5 触发 FollowOwnerGoal）。
//
// 判定手段：鹦鹉距玩家水平距离 < 5.5 格（初始 ~8.5 格，跟随后接近到 ~5 停）。FollowOwnerGoal 与
//   WaterAvoidingRandomFlyingGoal（优先级2 同级）竞争 Move flag——但 WaterAvoidingRandomFlyingGoal
//   shouldExecute 概率仅 0.1%（chance=0.001），且 FollowOwnerGoal 驯服后持续占 Move flag（shouldContinueExecuting
//   在 dist>5 时 true），WaterAvoidingRandomFlyingGoal 难以抢占。pollUntilSucceed interval=10 + maxTick=800
//   捕获"鹦鹉接近玩家"窗口。区域限定排除并行测试污染；type 用 "minecraft:parrot"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鹦鹉.txt#行为（已驯服跟随主人）
// Ref: ParrotEntity.cpp:126（FollowOwnerGoal 注册）/ TameableGoals.cpp:65-100（FollowOwnerGoal 实现）
function parrotFollowsOwnerWhenTamedTest(test: Test): void {
  // 鹦鹉 (7,2,7)（grass_pen 一角），玩家 (1,2,1)（对角，距 ~8.5 > minDistance=5）远程喂食驯服。
  // 远程喂食规避 LandOnOwnersShoulderGoal 坐肩抢占（详见函数头注释）。
  const parrot = test.spawn(PARROT, { x: 7, y: 2, z: 7 });
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "parrotOwner");
  const seeds = new ItemStack("minecraft:wheat_seeds", 1);
  player.setItem(seeds as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 玩家始终在 (1,2,1)，远程喂食触发驯服（interactWithEntity 无距离门控）。
  const playerPos = test.worldLocation({ x: 1, y: 2, z: 1 });

  // 阶段一：tick 5..210 每 3 tick 远程喂种子驯服（约69次，1/10 概率，69 次内成功率 ~99.93%）。
  // 创造模式不消耗种子可反复喂。驯服后继续远程喂走分支2 toggleSitting（基线读实际值，阶段二修正）。
  for (let t = 5; t <= 210; t += 3) {
    test.runAtTickTime(t, () => {
      (player as any).interactWithEntity(parrot);
    });
  }

  // 阶段二：tick 220 兜底检查驯服 + 确保鹦鹉站立。
  // 读 sitting 基线，若 sitting===true 则 interact 一次翻转 false（站立）——FollowOwnerGoal 需 !isSitting()。
  // 若 sitting===false 已站立则不干预。toggleSitting 同步生效，下一 GoalSelector 评估即读取新值。
  test.runAtTickTime(220, () => {
    test.assert(isParrotTamed(parrot),
      `parrot not tamed before follow test (1/10 chance, `
      + `is_tamed=${isParrotTamed(parrot)} expected true)`);
    const sitting = isParrotSitting(parrot);
    if (sitting === true) {
      (player as any).interactWithEntity(parrot);
    }
  });

  // 阶段三：pollUntilSucceed 从 tick 235 轮询鹦鹉距玩家水平距离 < 5.5 格。
  // 鹦鹉从 (7,2,7) 跟随接近 (1,2,1)，到 minDistance=5 附近停止。初始 ~8.5 格 → 跟随后 <5.5 格。
  // tick 220→235 留 15 tick 让 toggleSitting 生效 + GoalSelector 评估 + FollowOwnerGoal 首次 tick tryMoveTo。
  // 轨迹诊断：每 50 tick 采样鹦鹉位置到闭包数组，onTimeout 打印整条轨迹定位"鹦鹉静止/飞走/寻路失败"。
  const trajectory: string[] = [];
  for (let t = 250; t <= 800; t += 50) {
    test.runAtTickTime(t, () => {
      const parrots = test.getDimension().getEntities({
        type: "minecraft:parrot",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      if (parrots.length > 0) {
        const p = parrots[0].location;
        trajectory.push(`t${t}:(${p.x.toFixed(1)},${p.y.toFixed(1)},${p.z.toFixed(1)})`);
      }
    });
  }
  pollUntilSucceed(test, () => {
    const parrots = test.getDimension().getEntities({
      type: "minecraft:parrot",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (parrots.length === 0) return false;
    const dx = parrots[0].location.x - playerPos.x;
    const dz = parrots[0].location.z - playerPos.z;
    return dx * dx + dz * dz < 5.5 * 5.5;
  }, {
    startTick: 235,
    interval: 10,
    maxTick: 820,
    onTimeout: () => {
      const parrots = test.getDimension().getEntities({
        type: "minecraft:parrot",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const pos = parrots.length > 0
        ? `(${parrots[0].location.x.toFixed(1)},${parrots[0].location.y.toFixed(1)},${parrots[0].location.z.toFixed(1)})`
        : "gone";
      const tamed = parrots.length > 0 ? isParrotTamed(parrots[0]) : "gone";
      const sitting = parrots.length > 0 ? isParrotSitting(parrots[0]) : "gone";
      test.assert(false,
        `parrot did not follow owner, parrotPos=${pos} playerPos=(${playerPos.x.toFixed(1)},${playerPos.y.toFixed(1)},${playerPos.z.toFixed(1)}) `
        + `tamed=${tamed} sitting=${sitting} trajectory=[${trajectory.join(" ")}]`);
    },
  });
}

export function registerParrotTests(): void {
  GameTest.register("MobBehaviorTests", "parrot_tamed_by_seeds", parrotTamedBySeedsTest)
    .structureName("gametests:grass_pen")
    .maxTicks(750);

  GameTest.register("MobBehaviorTests", "parrot_untamed_by_default", parrotUntamedByDefaultTest)
    .structureName("gametests:grass_pen")
    .maxTicks(60);

  GameTest.register("MobBehaviorTests", "parrot_toggles_sitting_when_tamed", parrotTogglesSittingWhenTamedTest)
    .structureName("gametests:grass_pen")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "parrot_follows_owner_when_tamed", parrotFollowsOwnerWhenTamedTest)
    .structureName("gametests:grass_pen")
    .maxTicks(900);
}
