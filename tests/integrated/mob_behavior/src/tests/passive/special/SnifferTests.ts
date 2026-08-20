// 嗅探兽行为类 GameTest。
//
// 嗅探兽(Sniffer)是 1.21.11 标志性新生物,mob_behavior 包此前零测试。其核心状态机(嗅探→搜索→挖掘→
// 掉种子)暂未实现(SnifferEntity.cpp:307-314 TODO 列出 6 个 Brain goal 全缺),但诱惑(TemptGoal)、
// 繁殖(BreedGoal)、跟随父母等基础动物 AI 已实现。本测试验证已实现的种子诱惑行为——手持火把花种子
// 的玩家会吸引嗅探兽靠近,对齐 cat_tempted_by_fish 范式,填补 Sniffer 零测试覆盖。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// 嗅探兽被手持火把花种子的玩家诱惑靠近（wiki other_嗅探兽.txt#驯服与诱惑：嗅探兽会被手持
//   火把花种子/瓶草荚果的玩家吸引，可用于引导其移动）。
//
// C++ 链路（对齐 MC Java 1.21.11 Sniffer + TemptGoal）：
//   SnifferEntity : AnimalEntity（SnifferEntity.cpp:271-315 registerGoals）：
//   goalSelector 优先级3：TemptGoal(speed=1.25, 诱惑物=TORCHFLOWER_SEEDS||PITCHER_POD,
//     scaredByMovement=false)（SnifferEntity.cpp:278-293）。scaredByMovement=false 区别于猫
//     (CatTemptGoal scaredByMovement=true)——嗅探兽不怕玩家移动,但玩家静止更稳,照搬 cat 范式。
//     TemptGoal 经 getEntitiesInRange(TEMPT_RANGE=10) + dynamic_cast<Player*> 识别附近持种子玩家
//     (含 SimulatedPlayer),调 navigator()->moveTo(player) 驱动嗅探兽走向玩家。
//   registerAttributes（SnifferEntity.cpp:319-329）：MOVEMENT_SPEED=SNIFFER_MOVEMENT_SPEED=0.1
//     (远慢于猫 0.3),诱惑速度 1.25 倍 = 0.125/tick。MAX_HEALTH=14.0。
//
// 环境选择：mediumglass（12×9×11 走廊，helper y=2 z=5 x=2..10 共 9 格，同 CowTests/CatTests）。
//   嗅探兽体积 1.9×1.75,mediumglass 高度 9(helper y=2 层头顶 y=3..8 充足空气)容纳无碍。
//   玩家手持火把花种子(minecraft:torchflower_seeds)静止站立(不调 moveToLocation)。
//   嗅探兽 spawn 在走廊远端距玩家 8 格 < TemptRange 10。
//
// 判定手段：嗅探兽被诱惑后从 x=10 朝玩家 x=2 方向移动，断言嗅探兽出现在玩家附近体积（x:2..6）即通过。
//   对齐 cat_tempted_by_fish 的 assertEntityInVolume 范式。
// 时序：TemptGoal 每 tick 评估 + 寻路。嗅探兽 0.125/tick 诱惑速度接近 8 格约需 64+ tick 寻路起步,
//   加寻路延迟与路径绕行,maxTicks=1000 留充裕余量(对齐 cat_tempted_by_fish)。
//   诱惑是半确定行为(TemptGoal 每 tick 评估,寻路稳定),但寻路路径可能受走廊几何影响略有波动,
//   体积判定 x:2..6 覆盖 5 格宽吸收路径偏差。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_嗅探兽.txt#驯服与诱惑（被手持种子的玩家吸引）
function snifferTemptedByTorchflowerSeeds(test: Test): void {
  const snifferType = "sniffer";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // mediumglass 内部空腔 helper y=2（结构内 y=1 空气），地板 helper y=1（结构内 y=0 圆石）。
  // 走廊 helper y=2, z=5, x=2..10（9 格）。玩家与嗅探兽分置走廊两端，距离 8 格 < TemptRange 10。
  // 玩家静止站立——虽 scaredByMovement=false 不怕玩家动,但静止更稳,照搬 cat 范式。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持火把花种子：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回种子，TemptGoal lambda（item==TORCHFLOWER_SEEDS||PITCHER_POD）判定通过。
  const seeds = new ItemStack("minecraft:torchflower_seeds", 1);
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(seeds as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 嗅探兽 spawn 在走廊远端，距玩家 8 格，在 TemptRange(10) 内。
  test.spawn(snifferType, { x: 10, y: 2, z: 5 });

  // 嗅探兽被诱惑后从 x=10 朝玩家 x=2 方向移动。断言嗅探兽出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:3,z:6)，覆盖玩家附近 5×2×3 区域（同 cat 范式）。
  test.succeedWhen(() => {
    assertEntityInVolume(test, snifferType, 2, 2, 4, 6, 3, 6);
  });
}

export function registerSnifferTests(): void {
  GameTest.register("MobBehaviorTests", "sniffer_tempted_by_torchflower_seeds", snifferTemptedByTorchflowerSeeds)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);
}
