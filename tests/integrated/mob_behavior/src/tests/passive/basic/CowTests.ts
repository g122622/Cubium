// 牛行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// 牛跟随手持小麦的玩家。
// CowEntity 注册了 TemptGoal（诱惑物品=小麦，检测范围 10 格，scaredByMovement=false）。
// TemptGoal 经 getEntitiesInRange + dynamic_cast<Player*> 识别附近持麦玩家（含 SimulatedPlayer），
// 调 navigator()->moveTo(player) 驱动牛走向玩家。玩家静止持麦即可触发，无需玩家移动。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_牛.txt#繁殖
function cowFollowsWheat(test: Test): void {
  const cowType = "cow";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // mediumglass 内部空腔 helper y=2（结构内 y=1 空气），地板 helper y=1（结构内 y=0 圆石）。
  // 走廊 helper y=2, z=5, x=2..10（9 格）。玩家与牛分置走廊两端，距离 8 格 < TemptRange 10。
  // 官方签名 spawnSimulatedPlayer(blockLocation, name?, gameMode?)，位置在前。
  // gameMode 省略走 Cubium 默认创造模式（创造模式不影响 TemptGoal 的手持物品检测）。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持小麦：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回小麦，TemptGoal 才能识别诱惑源。
  const wheat = new ItemStack("minecraft:wheat", 1);
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(wheat as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 牛 spawn 在走廊远端，距玩家 8 格，在 TemptRange(10) 内
  test.spawn(cowType, { x: 10, y: 2, z: 5 });

  // 牛被诱惑后从 x=10 朝玩家 x=2 方向移动。断言牛出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:3,z:6)，覆盖玩家附近 5×2×3 区域。
  test.succeedWhen(() => {
    assertEntityInVolume(test, cowType, 2, 2, 4, 6, 3, 6);
  });
}

export function registerCowTests(): void {
  GameTest.register("MobBehaviorTests", "cow_follows_wheat", cowFollowsWheat)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);
}
