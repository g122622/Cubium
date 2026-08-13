// 绵羊行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 绵羊吃草：成年羊每 tick 有 1/1000 概率吃掉脚下的草方块，将其变为泥土。
// EatGrassGoal 检查实体脚下方块（entityPos.down()），若是 grass_block 则在 40 tick 动画后
// 调 setBlockState 把它变成 dirt（需 mob_griefing=true，默认开）。本测试在满铺草地的围栏里
// 放多只羊，断言任一草方块变泥土即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_绵羊.txt#吃草
function sheepEatGrass(test: Test): void {
  const sheepType = "sheep";

  // 结构 grass_pen（9×5×9）：y=0 满铺 9×9 grass_block，y=1..3 玻璃墙围栏+内部空气，y=4 开放。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 故草地地板=结构内 y=0 → helper-y=1；羊站立的空气腔=结构内 y=1 → helper-y=2。
  // 4 只羊分散站位在草地上（各占角落附近），最大化覆盖不同草方块，提高触发概率。
  const sheepPositions = [
    { x: 2, y: 2, z: 2 },
    { x: 6, y: 2, z: 2 },
    { x: 2, y: 2, z: 6 },
    { x: 6, y: 2, z: 6 },
  ];
  for (const p of sheepPositions) {
    test.spawn(sheepType, p);
  }

  // 满铺草地：81 格 grass_block（结构内 y=0 对应 helper-y=1，x,z∈[0,8]）。
  // 任一格变 dirt 即通过（OR 语义）。
  const grassPositions: { x: number; y: number; z: number }[] = [];
  for (let x = 1; x <= 7; x++) {
    for (let z = 1; z <= 7; z++) {
      grassPositions.push({ x, y: 1, z });
    }
  }

  // 概率时序：EatGrassGoal 已对齐 vanilla adjustedTickDelay(1000)=500，GoalSelector 每 2 tick
  // 评估一次 → 单羊每 tick 等效概率 ≈ 1/1000（与 vanilla 一致）。4 羊 × 49 格草，
  // 期望约 250 tick 首次触发 + 40 tick 动画（adjustedTickDelay(40)=20）+ 寻路余量。
  // maxTicks=3000 → 期望触发约 12 次，P(0) 极低，近乎必过。
  // succeedWhen 多 assert 是 AND 短路，这里把"任一格变 dirt"的 OR 语义用 try/catch 转换：
  // 任一格 assert 通过（不抛）则 return 视为条件满足；全不满足则主动抛异常让框架继续轮询。
  test.succeedWhen(() => {
    for (const p of grassPositions) {
      try {
        test.assertBlockPresent("minecraft:dirt", p, true);
        return;
      } catch {
        // 该格尚未变泥土，继续检查下一格
      }
    }
    throw new Error("no dirt yet");
  });
}

export function registerSheepTests(): void {
  GameTest.register("MobBehaviorTests", "sheep_eat_grass", sheepEatGrass)
    .structureName("gametests:grass_pen")
    .maxTicks(3000);
}
