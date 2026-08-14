// 鸡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// 鸡下蛋：成年鸡每隔 6000-12000 tick（5-10 分钟）下 1 个鸡蛋，鸡蛋以掉落物实体（minecraft:item，
// 持 EGG 物品）形式 spawn 在鸡身旁。ChickenEntity::tick 内 eggTimer 每 tick 递减，到 0 时
// spawn ItemEntity(EGG) + 播放音效 + 重置计时器（仅成年、非鸡骑士）。
// 本测试 spawn 成年鸡，断言结构内出现 item 掉落物实体即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#下蛋
function chickenLayEgg(test: Test): void {
  const chickenType = "chicken";

  // 结构 grass_pen（9×5×9 开放玻璃围栏 + 满铺草地，y=0 草地 helper-y=1，y=1 空气腔 helper-y=2）。
  // spawn 2 只成年鸡分散站位，任一只下蛋即通过（提高触发概率，缩短期望等待时间）。
  test.spawn(chickenType, { x: 3, y: 2, z: 3 });
  test.spawn(chickenType, { x: 5, y: 2, z: 5 });

  // eggTimer 初值 6000-12000 随机。2 只鸡取最小值期望约 3000-6000 tick 首次下蛋，最坏 12000 tick。
  // maxTicks=13000 留余量。下蛋 spawn 的 item 掉落物实体类型="item"（minecraft:item）。
  // 用 assertEntityInVolume（基于 getEntities，指定 worldLocation 体积）覆盖整个 grass_pen 内腔，
  // 断言 item 实体出现。item 掉落在草地（helper y=1）上，鸡在 y=2，体积覆盖 y=1..4 全内腔。
  test.succeedWhen(() => {
    assertEntityInVolume(test, "item", 1, 1, 1, 7, 4, 7);
  });
}

export function registerChickenTests(): void {
  GameTest.register("MobBehaviorTests", "chicken_lay_egg", chickenLayEgg)
    .structureName("gametests:grass_pen")
    .maxTicks(13000);
}
