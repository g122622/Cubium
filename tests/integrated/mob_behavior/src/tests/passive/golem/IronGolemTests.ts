// 铁傀儡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

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

// 玩家用铁块摆 T 形图案 + 顶部放雕刻南瓜，最后放南瓜触发建造生成铁傀儡
// （wiki tech_铁傀儡.txt#创建：4 个铁块摆成 T 形，最后在顶部放雕刻南瓜即生成铁傀儡）。
//
// C++ 链路：CarvedPumpkinBlock::onBlockAdded → trySpawnGolem（MelonPumpkinBlocks.cpp:196-243）。
// trySpawnGolem 按优先级 雪>铁>铜 检测，checkIronGolemPattern（:278-348）校验 T 形：
//   以南瓜 headPos 为顶，headPos.down()=armCenter（中层中央铁块），
//   headPos.down(2)=body（底层中央铁块），手臂方向东-西或南-北两端各一铁块。
//   顶层南瓜两侧 + 底层 body 两侧必须为 air。
//   先尝试东西手臂再南北手臂，任一匹配即 spawnIronGolem：移除 5 格方块（设 air）+
//   在 bodyPos 生成 iron_golem + setPlayerCreated(true)（玩家建造的不攻击玩家）。
//
// 关键：只有放南瓜（CarvedPumpkinBlock onBlockAdded）才触发检测，铁块放置不触发——
// 故测试顺序：先摆 4 铁块（T 形缺南瓜顶），最后放南瓜触发建造。对齐原版"最后放南瓜"语义。
//
// 图案坐标（glass_pit 内，南瓜 headPos=(3,4,3)）：
//   顶层 y=4: (3,4,3)=carved_pumpkin（最后放）
//   中层 y=3: (3,3,3)=iron_block(armCenter) + (3,3,2)=iron_block(东臂) + (3,3,4)=iron_block(西臂)
//   底层 y=2: (3,2,3)=iron_block(body)
//   顶层南瓜两侧 (3,4,2)(3,4,4)、底层 body 两侧 (3,2,2)(3,2,4) 须 air——glass_pit 内部全 air 满足。
//   注：东西方向用 z 轴偏移（Cubium east/west 对应 +z/-z，南北对应 +x/-x，此处用 z 轴作手臂方向，
//   pattern 校验 east()/west() 即 z±1，与 z 轴手臂等价）。
//
// 判定手段：建造成功后 5 格方块变 air + iron_golem 出现。succeedWhen 轮询区域内 iron_golem 数>=1。
// 建造是 onBlockAdded 同步触发（放南瓜那 tick 即 spawn），maxTicks=200 留 spawn 注册 + 余量。
// 区域限定到本测试 7×5×7，排除 iron_golem_arena 并行测试的铁傀儡污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#创建（T 形铁块 + 顶部南瓜）
function ironGolemBuiltByPlayer(test: Test): void {
  const ironGolemType = "iron_golem";

  // 先摆 4 个铁块（T 形缺南瓜顶）。铁块放置不触发建造检测（仅南瓜 onBlockAdded 触发）。
  // 底层 body：(3,2,3)。中层 armCenter：(3,3,3)。中层东西手臂：(3,3,2)(3,3,4)。
  test.setBlockType("minecraft:iron_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 2 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 4 });

  // 最后放南瓜（顶层 3,4,3）：onBlockAdded → trySpawnGolem → checkIronGolemPattern 匹配 →
  // spawnIronGolem 移除 5 格 + 生成 iron_golem。
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 4, z: 3 });

  // 建造是放南瓜那 tick 同步触发，iron_golem 立即 spawn 注册到世界。
  // succeedWhen 轮询区域内 iron_golem>=1 即通过。
  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(golems.length >= 1, `iron_golem not built, count=${golems.length}`);
  });
}

// 铁傀儡 T 形图案支持东西与南北两个手臂方向（wiki tech_铁傀儡.txt#创建：T 形可朝任意水平方向）。
// checkIronGolemPattern 先尝试东西手臂（armCenter.east/west）再南北手臂（armCenter.north/south），
// 两方向均合法。本测试用南北手臂方向（手臂沿 x 轴）验证第二个分支也能生成。
//
// 图案坐标（南瓜 headPos=(3,4,3)，南北手臂沿 x 轴）：
//   顶层 y=4: (3,4,3)=carved_pumpkin
//   中层 y=3: (3,3,3)=iron_block(armCenter) + (2,3,3)=iron_block(南臂) + (4,3,3)=iron_block(北臂)
//   底层 y=2: (3,2,3)=iron_block(body)
//   顶层南瓜两侧 (2,4,3)(4,4,3)、底层 body 两侧 (2,2,3)(4,2,3) 须 air——glass_pit 内部全 air 满足。
//
// 与 iron_golem_built_by_player（东西手臂）互补：两方向均能生成，交叉验证 pattern 双分支。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#创建（T 形可朝任意水平方向）
function ironGolemBuiltNorthSouthArms(test: Test): void {
  const ironGolemType = "iron_golem";

  // 先摆 4 个铁块（南北手臂 T 形缺南瓜顶）。
  // 底层 body：(3,2,3)。中层 armCenter：(3,3,3)。中层南北手臂：(2,3,3)(4,3,3)。
  test.setBlockType("minecraft:iron_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 2, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 4, y: 3, z: 3 });

  // 最后放南瓜触发建造（南北手臂分支匹配）。
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 4, z: 3 });

  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(golems.length >= 1, `iron_golem not built (N-S arms), count=${golems.length}`);
  });
}

export function registerIronGolemTests(): void {
  GameTest.register("MobBehaviorTests", "iron_golem_arena", ironGolemArena)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(810);

  GameTest.register("MobBehaviorTests", "iron_golem_built_by_player", ironGolemBuiltByPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "iron_golem_built_north_south_arms", ironGolemBuiltNorthSouthArms)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
