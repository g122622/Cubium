---
name: mc-add-integrated-test
description: 基于现有集成测试框架，为项目增加集成测试，以测试与原版MC行为是否对齐
---

# 1. 任务简介

根据用户指定的范围，基于现有集成测试框架，为项目增加集成测试，以测试与原版MC行为是否对齐。

# 2. 详细过程

1. 调用 `docs\minecraft-wiki-source\.claude\skills\minecraft-wiki-retrieval\SKILL.md` 中的 `minecraft-wiki-retrieval` 技能，根据用户指定的范围，检索原版MC的行为、数据、逻辑等的wiki原文，并将检索结果作为参考。
2. 思考要设计怎样的集成测试。设计准则见下面章节。
3. 开始着手编写typescript测试用例。必须参考现有测试用例的语法和结构，以及随时查询官方ScriptAPI的文档：E:\dev\MC\Mods\minecraft-creator\creator\ScriptAPI\minecraft\server-gametest
4. 有时候测试失败、崩溃的原因可能不在于测试用例或者被测代码本身，而是项目的集成测试框架、脚本系统等的bug导致的（这里的bug多种多样，比如：api实际行为与官方文档不一致或与实际基岩版api不一致、api未完整实现等）。这种情况下请修复、补全集成测试框架、脚本系统等的bug，确保测试用例能够顺利运行，然后继续编写测试用例。脚本系统可从src\common\mod\bedrock\addon\modules\MinecraftModuleFactory.cpp入手。

# 3. 集成测试设计准则和案例

## 3.1 设计准则

- 尽可能覆盖原版MC wiki中的大部分行为、数据、逻辑等，确保与原版MC行为对齐。
- 【重要】有的特性可能基岩版和Java版的行为不一致、或者官方文档中没有明确说明行为。这种情况下请不要为相关特性设计集成测试！
- 对于随机性较强的行为（如生物 AI、掉落物概率），可增加maxTicks参数，延长测试时间，确保测试结果的可靠性，并在需要多次重复运行。

## 3.2 代码准则

- 优先查阅 tests\integrated\utils 和 当前\tests\integrated\xxx\src\utils 目录下已有的辅助函数，尽可能做到代码复用；对于应该复用的代码，也应该提升到utils目录下，避免重复造轮子。
- 为每个测试添加清晰的注释，说明其验证的原版 Wiki 章节（如 // Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#自然生成），方便追溯
- 单一职责：每个测试只验证一个“行为点”，避免过度复杂导致失败时无法定位原因

## 3.3 实际案例

下面只是一些最基础的示例，你能调用的脚本api远不止下面这些，测试手段也远不止下面这些：

```ts
// 生物行为类 GameTest：僵尸追村民、铁傀儡竞技场、Zoglin 浮空、幻翼避猫。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { addFourNotchedWalls } from "./utils/block/build.js";
import { assertEntityInVolume } from "./utils/entity/assert.js";

// 僵尸在有缺口的砖墙间追逐村民，验证僵尸寻路 AI。
function zombieVillagerChase(test: Test): void {
  const villagerType = "villager_v2";
  const zombieType = "zombie";

  addFourNotchedWalls(test, "minecraft:brick_block", 2, 1, 2, 4, 6, 4);

  test.spawn(villagerType, { x: 1, y: 3, z: 1 });
  test.spawn(zombieType, { x: 5, y: 3, z: 5 });

  test.runAtTickTime(180, () => {
    test.assertEntityPresentInArea(villagerType, true);
    test.succeed();
  });
}

// 铁傀儡的韧性测试：验证铁傀儡能击败骷髅和僵尸。
function ironGolemArena(test: Test): void {
  const ironGolemType = "iron_golem";
  const skeletonType = "skeleton";
  const zombieType = "zombie";

  test.spawn(ironGolemType, { x: 4, y: 3, z: 3 });
  test.spawn(skeletonType, { x: 5, y: 3, z: 5 });
  test.spawn(skeletonType, { x: 4, y: 3, z: 4 });
  test.spawn(skeletonType, { x: 3, y: 3, z: 3 });
  test.spawn(zombieType, { x: 4, y: 3, z: 6 });
  test.spawn(zombieType, { x: 3, y: 3, z: 5 });
  test.spawn(zombieType, { x: 2, y: 3, z: 4 });
  test.spawn(zombieType, { x: 5, y: 3, z: 2 });

  test.succeedWhen(() => {
    test.assertEntityPresentInArea(zombieType, false);
    test.assertEntityPresentInArea(skeletonType, false);
    test.assertEntityPresentInArea(ironGolemType, true);
  });
}

// Shulker 的攻击会使 Zoglin 浮空至笼顶。
function zoglinFloat(test: Test): void {
  const zoglinType = "zoglin";
  const shulkerType = "shulker";

  test.spawn(zoglinType, { x: 5, y: 2, z: 5 });
  test.spawn(shulkerType, { x: 2, y: 2, z: 2 });

  test.succeedWhen(() => {
    // zoglin 是否已浮至笼顶？
    assertEntityInVolume(test, zoglinType, 1, 7, 1, 10, 10, 10);
  });
}

export function registerMobBehaviorTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_villager_chase", zombieVillagerChase)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(2000);

  GameTest.register("MobBehaviorTests", "iron_golem_arena", ironGolemArena)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(810);

  GameTest.register("MobBehaviorTests", "zoglin_float", zoglinFloat)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(210);
}

```
