// 海龟行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// 海龟跟随手持海草的玩家（wiki tech_海龟.txt#繁殖：海龟可用海草繁殖，故海草是其诱惑/繁殖物品）。
//
// C++ 链路：TurtleEntity : AnimalEntity（非 WaterMobEntity，陆地不溺水），registerGoals
// （TurtleEntity.cpp:248-285）优先级2 注册 TurtleTemptGoal(this, 1.1)。TurtleTemptGoal
// （TurtleGoals.cpp:655-663）是 TemptGoal 的薄包装，构造转发 TemptGoal(turtle, 1.1, isSeagrass, false)
// （scaredByMovement=false 硬编码）。isSeagrass 判定 item==Items::SEAGRASS（minecraft:seagrass）。
// TemptGoal::findTemptingPlayer 用 getEntitiesInRange(pos, TEMPT_RANGE=10, ...) 球体搜索 10 格内
// 主手或副手持海草的玩家（含 SimulatedPlayer），命中后调 nav->moveTo(player, 1.1) 驱动海龟走向玩家。
// 玩家静止持海草即可触发，无需玩家移动（与 cow_follows_wheat 同范式）。
//
// 陆地可行性：海龟继承 AnimalEntity 非水生，陆地不窒息。TurtleGoToWaterGoal(优先级3) 无水时
// _findWater 返 false 不干扰；TurtleTravelGoal(7) 要求 isInWater 陆地不触发；TurtleWanderGoal(9)
// 优先级低于 TemptGoal(2) 被 Move flag mutex 阻；TurtleGoHomeGoal(4) 需 hasHomePos() spawn 的
// 海龟无 homePos 不触发。故陆地 mediumglass 中 TurtleTemptGoal 独占 Move flag，海龟被诱惑移动。
// 海龟陆地速度 = max(baseSpeed*0.5, 0.06) = 0.125（TurtleEntity.cpp:415），慢于牛(0.2)，maxTicks 加大。
//
// 环境选择：mediumglass（12×9×11 陆地走廊）。玩家 (2,2,5) ↔ 海龟 (10,2,5)，水平距 8 格 < temptRange 10。
// 结构放置有 +1 抬升：helper-y=2 对应结构内 y=1 空气腔，地板 helper-y=1（结构内 y=0 圆石）。
// 海龟 setStepHeight(1.0) 可上 1 格台阶，走廊无障碍寻路通畅。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海龟.txt#繁殖（海草诱惑/繁殖）
function turtleTemptedBySeagrass(test: Test): void {
  const turtleType = "turtle";

  // 玩家 (2,2,5) 主手持海草。gameMode 省略走 Cubium 默认创造模式（创造不影响 TemptGoal 手持物品检测）。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持海草：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回海草，TemptGoal 才能识别诱惑源。
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  const seagrass = new ItemStack("minecraft:seagrass", 1);
  farmer.setItem(seagrass as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 海龟 spawn 在走廊远端 (10,2,5)，距玩家 8 格 < temptRange(10)，在 TurtleTemptGoal 检测范围内。
  test.spawn(turtleType, { x: 10, y: 2, z: 5 });

  // 海龟被诱惑后从 x=10 朝玩家 x=2 方向移动。断言海龟出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:3,z:6)，覆盖玩家附近 5×2×3 区域（同 cow_follows_wheat）。
  // 海龟陆地速度 0.125（慢于牛 0.2），8 格走廊需更长时间，maxTicks=2000 留充裕余量。
  test.succeedWhen(() => {
    assertEntityInVolume(test, turtleType, 2, 2, 4, 6, 3, 6);
  });
}

export function registerTurtleTests(): void {
  GameTest.register("MobBehaviorTests", "turtle_tempted_by_seagrass", turtleTemptedBySeagrass)
    .structureName("gametests:mediumglass")
    .maxTicks(2000);
}
