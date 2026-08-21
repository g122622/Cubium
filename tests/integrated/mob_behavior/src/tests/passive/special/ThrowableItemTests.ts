// 投掷物物品（雪球/鸡蛋/末影珍珠/经验瓶）右键投掷生成弹射物实体对齐测试。
//
// 验证 ThrowableItem::onItemRightClick 投掷链路（createProjectile → setWorld → setShooter →
// spawnEntity + shootFrom + 非创造 shrink 消耗）。区别弓/三叉戟的拉弓释放（onPlayerStoppedUsing 语义）：
// 投掷物 getUseDuration=0，useItem 即时调 onItemRightClick，单次右键即生成弹射物实体并消耗1个。
//
// wiki 参考：
//   - tech_雪球.txt：右键投掷生成 snowball 实体，非创造消耗1个（maxStackSize=16）。
//     命中烈焰人造成3伤害（wiki tech_烈焰人.txt#雪球/鸡蛋/末影珍珠对烈焰人特判）。
//   - tech_鸡蛋.txt：右键投掷生成 egg 实体，非创造消耗1个。落地 1/8 概率孵化小鸡。
//   - tech_末影珍珠.txt：右键投掷生成 ender_pearl 实体，非创造消耗1个。落地传送投掷者 +5 坠落伤害。
//   - tech_附魔之瓶.txt（经验瓶）：投掷落地生成 3-11 点经验的经验球（experience_orb 实体）。
//
// 本文件覆盖"投掷生成实体 + 消耗"核心链路（雪球/鸡蛋/末影珍珠，确定性，spawnEntity 即生成可区域查询）
// + 末影珍珠落地传送投掷者（onImpact 传送链路）+ 经验瓶落地生成经验球（onImpact 生成 experience_orb）。
// 雪球命中烈焰人3伤害涉及投射物飞行命中判定 + 烈焰人移动 + 重力叠加致 raytrace 命中不稳定，按
// "难确定性不设计测试"准则暂不覆盖。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板，弹射物可飞行）。
// Survival 玩家 (1,2,3) 主手持 N 个投掷物（slot 0）。spawn 默认 yaw=0 pitch=0 → 朝 +Z 水平投掷。
//
// 时序：tick 5 useItem(投掷物) → ThrowableItem::onItemRightClick → createProjectile（spawnEntity
// 生成弹射物实体）+ shootFrom（速度 1.5 朝 +Z）+ 非创造 shrink(1)。
//
// 判定手段（双重断言）：
//   1. pit 区域内出现对应弹射物实体（minecraft:snowball/egg/ender_pearl，投掷生成成功）；
//   2. 主手槽（slot 0）物品数量减1（非创造消耗1个）。
// Survival 模式（创造跳过 shrink 无消耗证据）。弹射物速度 1.5 格/tick，creeper_pit 开放坑无墙约 3 tick
// 飞出 z=6 边界，查询 startTick=6（投掷 tick 5 后首 tick）尽早捕获飞行中的弹射物。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const SNOWBALL = "minecraft:snowball";
const EGG = "minecraft:egg";
const ENDER_PEARL = "minecraft:ender_pearl";
const EXPERIENCE_BOTTLE = "minecraft:experience_bottle";
const EXPERIENCE_ORB = "minecraft:experience_orb";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
// 投掷物朝 +Z 飞 ~5 格可能飞出 pit（z>6），扩大查询范围覆盖落点（经验瓶落地生成经验球散开）。
const SEARCH_FROM = { x: 0, y: 0, z: 0 };
const SEARCH_VOLUME = { x: 15, y: 8, z: 15 };

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 雪球投掷生成 snowball 实体 + 消耗1个：Survival 玩家持5雪球 useItem，断言 snowball 实体出现 + 主手剩4。
function snowballThrownSpawnsEntityTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "thrower", 0 as any);
  const snowballs = new ItemStack(SNOWBALL, 5);
  player.setItem(snowballs as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(雪球) → ThrowableItem::onItemRightClick → spawnEntity(snowball) + shrink(1)。
  test.runAtTickTime(5, () => {
    (player as any).useItem(snowballs as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询双重断言：pit 区域 ≥1 个 snowball 实体 + 主手剩4（5-1）。
  // 雪球速度 1.5 格/tick 朝 +Z 飞，creeper_pit 开放坑无墙约 3 tick 飞出 z=6 边界，
  // 故 startTick=6（投掷 tick 5 后首 tick）尽早查询飞行中的雪球。
  pollUntilSucceed(test, () => {
    const projectiles = test.getDimension().getEntities({
      type: SNOWBALL,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (projectiles.length < 1) return false;
    return getMainHandAmount(player) === 4;
  }, {
    startTick: 6,
    interval: 1,
    maxTick: 40,
    onTimeout: () => {
      const projectiles = test.getDimension().getEntities({
        type: SNOWBALL,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const loc = projectiles[0]?.location;
      test.assert(false,
        `snowball_thrown: failed: snowballCount=${projectiles.length} (expected >=1) `
        + `mainHand amount=${getMainHandAmount(player)} (expected 4) loc=${JSON.stringify(loc)}`);
    },
  });
}

// 鸡蛋投掷生成 egg 实体 + 消耗1个：Survival 玩家持5鸡蛋 useItem，断言 egg 实体出现 + 主手剩4。
function eggThrownSpawnsEntityTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "thrower", 0 as any);
  const eggs = new ItemStack(EGG, 5);
  player.setItem(eggs as unknown as Parameters<typeof player.setItem>[0], 0, true);

  test.runAtTickTime(5, () => {
    (player as any).useItem(eggs as unknown as Parameters<typeof player.useItem>[0]);
  });

  pollUntilSucceed(test, () => {
    const projectiles = test.getDimension().getEntities({
      type: EGG,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (projectiles.length < 1) return false;
    return getMainHandAmount(player) === 4;
  }, {
    startTick: 6,
    interval: 1,
    maxTick: 40,
    onTimeout: () => {
      const projectiles = test.getDimension().getEntities({
        type: EGG,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const loc = projectiles[0]?.location;
      test.assert(false,
        `egg_thrown: failed: eggCount=${projectiles.length} (expected >=1) `
        + `mainHand amount=${getMainHandAmount(player)} (expected 4) loc=${JSON.stringify(loc)}`);
    },
  });
}

// 末影珍珠投掷生成 ender_pearl 实体 + 消耗1个：Survival 玩家持3末影珍珠 useItem，
// 断言 ender_pearl 实体出现 + 主手剩2（3-1）。末影珍珠 maxStackSize=16（1.21 已堆叠）。
function enderPearlThrownSpawnsEntityTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "thrower", 0 as any);
  const pearls = new ItemStack(ENDER_PEARL, 3);
  player.setItem(pearls as unknown as Parameters<typeof player.setItem>[0], 0, true);

  test.runAtTickTime(5, () => {
    (player as any).useItem(pearls as unknown as Parameters<typeof player.useItem>[0]);
  });

  pollUntilSucceed(test, () => {
    const projectiles = test.getDimension().getEntities({
      type: ENDER_PEARL,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (projectiles.length < 1) return false;
    return getMainHandAmount(player) === 2;
  }, {
    startTick: 6,
    interval: 1,
    maxTick: 40,
    onTimeout: () => {
      const projectiles = test.getDimension().getEntities({
        type: ENDER_PEARL,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const loc = projectiles[0]?.location;
      test.assert(false,
        `ender_pearl_thrown: failed: enderPearlCount=${projectiles.length} (expected >=1) `
        + `mainHand amount=${getMainHandAmount(player)} (expected 2) loc=${JSON.stringify(loc)}`);
    },
  });
}

// 末影珍珠落地传送投掷者对齐测试（验证 EnderPearlEntity::onImpact 传送 shooter 到落点）。
//
// wiki 参考 tech_末影珍珠.txt：末影珍珠投出后落地（撞方块）将投掷者传送到落点，并造成5坠落伤害。
// EnderPearlEntity::onImpact（ProjectileItemEntity.cpp:252）检查 result.type != Entity（撞方块非实体）
// 时 setPosition(hitPosition) 传送 shooter + hurt(fall, 5.0f)。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。Survival 玩家 (1,2,3) 持1末影珍珠，
// 默认 yaw=0 pitch=0 朝 +Z 水平投掷，末影珍珠飞 ~5 格落地，传送玩家到落点（z 增大）。
//
// 时序：tick 5 useItem(末影珍珠) → spawnEntity(ender_pearl) + shrink(1) + shootFrom 朝 +Z 水平
// → 末影珍珠飞行（速度1.5/tick + 重力下沉）→ 撞地面方块 onImpact → setPosition 传送玩家到落点。
//
// 判定手段：玩家 z 坐标从 3 变化 >1（传送到落点，z 增大）。末影珍珠落地落点 z>4，传送后玩家 z>4。
// 用 getEntities({type:"player"}) 查玩家位置（spawnSimulatedPlayer 返回对象 .location 未绑定，
// 须经 getEntities 查询实体位置）。宽松断言 z 变化 >1 降低落点随机性 flaky。
function enderPearlTeleportsThrowerTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "thrower", 0 as any);
  const pearl = new ItemStack(ENDER_PEARL, 1);
  player.setItem(pearl as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(末影珍珠) → 末影珍珠朝 +Z 飞 → 落地 onImpact 传送玩家。
  test.runAtTickTime(5, () => {
    (player as any).useItem(pearl as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询断言：玩家 z 坐标变化 >1（从 3 传送到落点 z>4）。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length < 1) return false;
    const z = players[0].location.z;
    return Math.abs(z - 3) > 1;
  }, {
    startTick: 10,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const z = players[0]?.location?.z;
      test.assert(false,
        `ender_pearl_teleports: failed: player z=${z} (expected |z-3|>1, `
        + `ender pearl should teleport thrower to landing point)`);
    },
  });
}

// 经验瓶投掷落地生成经验球对齐测试（验证 ExperienceBottleEntity::onImpact 生成 experience_orb）。
//
// wiki 参考 tech_附魔之瓶.txt（经验瓶）：投掷落地破裂生成 3-11 点经验的经验球（experience_orb 实体）。
// ExperienceBottleEntity::onImpact（ProjectileItemEntity.cpp:459）rng.nextInt(3,11) 个 ExperienceOrbEntity
// + setPosition 散开 + spawnEntity。经验瓶是 ThrowableItem 子类（onItemRightClick 投掷生成 experience_bottle
// 实体 + 非创造 shrink 消耗），区别雪球/鸡蛋的命中效果——经验瓶落地必生成经验球（确定性，数量随机但 >=3）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。Survival 玩家 (1,2,3) 持1经验瓶，默认朝 +Z 投掷。
//
// 时序：tick 5 useItem(经验瓶) → ThrowableItem::onItemRightClick spawnEntity(experience_bottle) + shrink
// → 经验瓶飞行 → 落地 onImpact 生成 3-11 个 experience_orb 散开。
//
// 判定手段：扩大区域内出现 >=1 个 experience_orb 实体（落地生成经验球）。经验瓶落点可能飞出 pit，
// 用 SEARCH_VOLUME（15×8×15）覆盖。经验球存活时间长（不立即 remove），查询窗口宽。
function experienceBottleSpawnsOrbsTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "thrower", 0 as any);
  const bottle = new ItemStack(EXPERIENCE_BOTTLE, 1);
  player.setItem(bottle as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(经验瓶) → 投掷生成 experience_bottle → 落地生成 experience_orb。
  test.runAtTickTime(5, () => {
    (player as any).useItem(bottle as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询断言：扩大区域内出现 >=1 个 experience_orb 实体（经验瓶落地生成经验球）。
  pollUntilSucceed(test, () => {
    const orbs = test.getDimension().getEntities({
      type: EXPERIENCE_ORB,
      location: test.worldLocation(SEARCH_FROM),
      volume: SEARCH_VOLUME,
    });
    return orbs.length >= 1;
  }, {
    startTick: 12,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const orbs = test.getDimension().getEntities({
        type: EXPERIENCE_ORB,
        location: test.worldLocation(SEARCH_FROM),
        volume: SEARCH_VOLUME,
      });
      const bottles = test.getDimension().getEntities({
        type: EXPERIENCE_BOTTLE,
        location: test.worldLocation(SEARCH_FROM),
        volume: SEARCH_VOLUME,
      });
      test.assert(false,
        `experience_bottle: failed: orbCount=${orbs.length} (expected >=1) `
        + `bottleCount=${bottles.length} (experience bottle should spawn experience_orbs on impact)`);
    },
  });
}

export function registerThrowableItemTests(): void {
  GameTest.register("MobBehaviorTests", "snowball_thrown_spawns_entity", snowballThrownSpawnsEntityTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
  GameTest.register("MobBehaviorTests", "egg_thrown_spawns_entity", eggThrownSpawnsEntityTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
  GameTest.register("MobBehaviorTests", "ender_pearl_thrown_spawns_entity", enderPearlThrownSpawnsEntityTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
  GameTest.register("MobBehaviorTests", "ender_pearl_teleports_thrower", enderPearlTeleportsThrowerTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);
  GameTest.register("MobBehaviorTests", "experience_bottle_spawns_orbs", experienceBottleSpawnsOrbsTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);
}