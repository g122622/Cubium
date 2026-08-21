// 钓鱼竿抛杆生成浮标 + 收杆移除浮标对齐测试（验证 FishingRodItem::onItemRightClick 抛杆/收杆链路）。
//
// 验证 FishingRodItem::onItemRightClick（FishingRodItem.cpp:70）两个分支：
//   - 抛杆（无浮标）：创建 FishingBobberEntity + setWorld + setPosition(玩家眼部) + setShooter
//     + shootFrom(BOBBER_VELOCITY=1.5) + spawnEntity + player.setFishingBobber(bobberId)。
//   - 收杆（hasBobber）：getBobber + reelIn + hurtAndBreak + setFishingBobber(0)。
//
// wiki 参考 tech_钓鱼竿.txt：右键抛杆投出浮标（fishing_bobber 实体），浮标落水后等待鱼上钩；
// 再次右键收杆收回浮标（reelIn 移除浮标实体）。钓鱼竿即时使用（getUseDuration=0 默认）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。Survival 玩家 (1,2,3) 主手钓鱼竿。
// spawn 默认 yaw=0 pitch=0 → 朝 +Z 水平抛杆，浮标飞 ~5 格落地进 Bobbing 状态存活（不 remove）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const FISHING_ROD = "minecraft:fishing_rod";
const FISHING_BOBBER = "minecraft:fishing_bobber";
// 浮标抛杆后朝 +Z 飞 ~5 格可能飞出 pit（z>6），扩大查询范围覆盖落点。
const BOBBER_SEARCH_FROM = { x: 0, y: 0, z: 0 };
const BOBBER_SEARCH_VOLUME = { x: 15, y: 8, z: 15 };

// 钓鱼竿抛杆生成浮标实体对齐测试（验证 FishingRodItem::onItemRightClick 抛杆链路）。
function fishingRodCastSpawnsBobberTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "angler", 0 as any);
  const rod = new ItemStack(FISHING_ROD, 1);
  player.setItem(rod as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(钓鱼竿) → 抛杆 spawnEntity(fishing_bobber)。
  test.runAtTickTime(5, () => {
    (player as any).useItem(rod as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询断言：扩大范围内出现 fishing_bobber 实体（抛杆成功）。浮标朝 +Z 飞可能飞出 pit，
  // 故用 BOBBER_SEARCH_VOLUME（15×8×15）覆盖落点。
  pollUntilSucceed(test, () => {
    const bobbers = test.getDimension().getEntities({
      type: FISHING_BOBBER,
      location: test.worldLocation(BOBBER_SEARCH_FROM),
      volume: BOBBER_SEARCH_VOLUME,
    });
    return bobbers.length >= 1;
  }, {
    startTick: 8,
    interval: 2,
    maxTick: 50,
    onTimeout: () => {
      const bobbers = test.getDimension().getEntities({
        type: FISHING_BOBBER,
        location: test.worldLocation(BOBBER_SEARCH_FROM),
        volume: BOBBER_SEARCH_VOLUME,
      });
      test.assert(false,
        `fishing_rod_cast: failed: bobberCount=${bobbers.length} (expected >=1, `
        + `fishing rod cast should spawn fishing_bobber entity)`);
    },
  });
}

// 钓鱼竿收杆移除浮标对齐测试（验证 FishingRodItem::onItemRightClick 收杆分支 reelIn 移除浮标）。
//
// 验证收杆分支：hasBobber 时 getBobber + reelIn + hurtAndBreak + setFishingBobber(0)。
// reelIn 移除浮标实体（FishingBobberEntity tick 检测 isFishing false 后 remove，或 reelIn 直接 remove）。
// 抛杆后再 useItem 触发收杆，浮标消失。
//
// 时序：tick 5 useItem(钓鱼竿) 抛杆 → tick 15 useItem(钓鱼竿) 收杆 → 浮标 remove 消失。
// 判定：收杆后 pit 区域 fishing_bobber 实体数为 0（浮标被移除）。
function fishingRodReelRemovesBobberTest(test: Test): void {
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "angler", 0 as any);
  const rod = new ItemStack(FISHING_ROD, 1);
  player.setItem(rod as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 抛杆 → tick 15 收杆。
  test.runAtTickTime(5, () => {
    (player as any).useItem(rod as unknown as Parameters<typeof player.useItem>[0]);
  });
  test.runAtTickTime(15, () => {
    (player as any).useItem(rod as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询断言：收杆后（tick 15 后）扩大范围内 fishing_bobber 实体数为 0（浮标被 reelIn 移除）。
  pollUntilSucceed(test, () => {
    const bobbers = test.getDimension().getEntities({
      type: FISHING_BOBBER,
      location: test.worldLocation(BOBBER_SEARCH_FROM),
      volume: BOBBER_SEARCH_VOLUME,
    });
    return bobbers.length === 0;
  }, {
    startTick: 18,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const bobbers = test.getDimension().getEntities({
        type: FISHING_BOBBER,
        location: test.worldLocation(BOBBER_SEARCH_FROM),
        volume: BOBBER_SEARCH_VOLUME,
      });
      test.assert(false,
        `fishing_rod_reel: failed: bobberCount=${bobbers.length} (expected 0, `
        + `reel should remove fishing_bobber entity)`);
    },
  });
}

export function registerFishingRodTests(): void {
  GameTest.register("MobBehaviorTests", "fishing_rod_cast_spawns_bobber", fishingRodCastSpawnsBobberTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
  GameTest.register("MobBehaviorTests", "fishing_rod_reel_removes_bobber", fishingRodReelRemovesBobberTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(90);
}
