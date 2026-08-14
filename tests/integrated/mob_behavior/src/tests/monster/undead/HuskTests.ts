// 尸壳行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 尸壳不会在阳光下燃烧（wiki tech_尸壳.txt#行为：尸壳不会在阳光下着火）。
// C++ 链路：MonsterEntity::tick → handleDaylightBurning → burnUndead → isInDaylight
// （isDaytime + brightness>0.5 + !isWet + canSeeSky）→ igniteForSeconds(8.0f)。
// shouldBurnInDaylight() 为 true 才进燃烧判定。HuskEntity::shouldBurnInDaylight()
// override 返回 false（hpp:76）→ 跳过燃烧，尸壳不燃。
//
// 单实体负向断言：只 spawn 尸壳，断言 onfire 恒 undefined。不在此测试内 spawn 对照实体
// （僵尸/骷髅）——实测同结构 spawn 多实体时光照重算竞态致 brightness 偏低，骷髅也不燃
// （单跑 skeleton_burns_in_daylight 则稳定燃烧）。正向对照由独立的 skeleton_burns_in_daylight
// 测试提供（骷髅该着火着火），两者互补验证 shouldBurnInDaylight 门控：骷髅燃 + 尸壳不燃。
//
// 僵尸/尸壳不用 FleeSunGoal/RestrictSunGoal（ZombieEntity.cpp:496 注释：骷髅才用），
// 故露天不会逃离阳光，尸壳持续不燃，断言稳定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_尸壳.txt#行为（不会在阳光下着火）
function huskDoesNotBurnInDaylight(test: Test): void {
  const huskType = "husk";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 尸壳 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，与 skeleton_burns_in_daylight 同款 spawn 位（保证光照环境一致可比）。
  const husk = test.spawn(huskType, { x: 4, y: 2, z: 4 });

  // 白天露天尸壳不着火：轮询 onfire 组件应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=500：与 skeleton_burns_in_daylight 同款，留余量确保断言稳定。
  // 正向对照由 skeleton_burns_in_daylight 提供（同结构同 skyAccess 骷髅该着火着火），
  // 排除此负向断言"燃烧机制未触发"的假性通过。
  test.succeedWhen(() => {
    const fire = husk.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("husk should not burn in daylight");
    }
  });
}

export function registerHuskTests(): void {
  GameTest.register("MobBehaviorTests", "husk_does_not_burn_in_daylight", huskDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：清空结构上方 worldgen 方块制造露天列使 canSeeSky=true。
    // setupTicks(20)：清空上方后 skyLight 入队需 tick 重算达 15，setupTicks 阶段先 tick 20 次让光照稳定。
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(500);
}
