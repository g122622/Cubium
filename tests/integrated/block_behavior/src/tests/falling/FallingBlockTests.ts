// 下落型方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在 y=1..2 空气层操作。

// 沙子下方无支撑时下落，原格变为空气（wiki tech_沙子.txt#下落：沙子是受重力影响的方块，下方无支撑时下落）。
//
// C++ 链路：FallingBlock::onBlockAdded（FallingBlock.cpp:48-52）放下方块即 scheduleBlockTick(pos, this,
// getFallDelay()=2, Normal)。2 tick 后 tick（:79-129）检查下方 canFallThrough（:133-156，空气/液体/火/
// 可替换方块均返回 true），满足则 setBlockState(pos, airState, 3) 将原格变 air（:104），并 spawn
// FallingBlockEntity（:109）。setBlockType 放沙子（air→sand，blockTypeChanged）触发 onBlockAdded 调度下落。
// 原格变 air 在 tick 内先于实体 spawn 执行，即使后续实体 spawn 失败原格 air 也已成立，断言原格消失安全。
//
// 环境构造：glass_pit 是玻璃坑结构（玻璃墙+内部空气，y=0 层含 glass/cobblestone/air 混合，非 grass_block
// 地板）。其内部 (3,1,1) 结构定义为 air，但运行时该格偶被 worldgen 残留 glass 占据（结构放置 flags=18
// 静默写入，与 worldgen 时序竞态），导致沙子下方非 air 而不下落。故放沙子前显式清空放置列：
// (3,0,1) 铺 stone 作确定性地板，(3,1,1) 显式置 air 确保下方无支撑，(3,2,1) 显式置 air 确保放置位干净。
// 显式 setBlockType 走 ServerWorld::setBlockState(flags=3) 完整路径，可靠覆盖 worldgen 残留。
//
// 判定手段：在 (3,2,1) 放沙子，下方 (3,1,1) 已显式置 air（无支撑）。onBlockAdded 调度 2 tick 后 tick
// → 下方 air 可穿透 → 原格变 air。succeedWhen 持续轮询断言原放置格沙子消失（轮询覆盖 2 tick 延迟窗口）。
// 注意：下落实体最终在 (3,1,1) 落地变回沙子（_tryPlaceBlock），故只断言原放置格 (3,2,1) 无沙子，不断言
// 落地格。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_沙子.txt#下落（受重力下落）
function sandFallsWhenNoSupportBelow(test: Test): void {
  // 显式构造确定的下落环境，排除 glass_pit 内部 worldgen 残留方块干扰：
  // (3,0,1) stone 地板，(3,1,1) air（沙子下方，无支撑），(3,2,1) air（沙子放置位）。
  test.setBlockType("minecraft:stone", { x: 3, y: 0, z: 1 });
  test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });
  test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

  // 在 (3,2,1) 放沙子，下方 (3,1,1) 已置 air（无支撑）。setBlockType（air→sand）触发 onBlockAdded
  // 调度 2 tick 后下落。
  test.setBlockType("minecraft:sand", { x: 3, y: 2, z: 1 });

  // 持续轮询断言原放置格沙子已下落消失（2 tick 延迟后成立）。
  // 不断言落地格 (3,1,1)——下落实体落地变回沙子，时序较长且依赖实体物理。
  test.succeedWhen(() => {
    test.assertBlockPresent("minecraft:sand", { x: 3, y: 2, z: 1 }, false);
  });
}

export function registerFallingBlockTests(): void {
  GameTest.register("BlockBehaviorTests", "sand_falls_when_no_support_below", sandFallsWhenNoSupportBelow)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
}
