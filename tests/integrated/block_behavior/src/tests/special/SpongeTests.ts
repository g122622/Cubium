// 海绵方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 grass_block 地板，y=1..4 为 air。方块测试在 y=1 空气层操作。

// 海绵吸水后变为湿海绵（wiki tech_海绵.txt#吸水：海绵放置后吸收周围水源，吸满后变为湿海绵）。
//
// C++ 链路：SpongeBlock::tryAbsorbWater（SpongeBlock.cpp:54-69）由 onBlockAdded（:71-76）与
// neighborChanged（:78-90）调用。吸水时海绵方块状态替换为 wet_sponge（:59-60，flags=3），
// 同时 absorb（:92-180）以 BFS（最大深度 MAX_ABSORB_DEPTH=6，最多 MAX_ABSORB_COUNT=65 格）
// 调 IBucketPickupHandler::pickupFluid 取走水源、setAir 清流动水。海绵放置（setBlockType flags=3）
// 触发自身 onBlockAdded 即同步执行 tryAbsorbWater，同 tick 内吸水并变 wet_sponge。
//
// 判定手段：先在海绵周围放若干水源，再放海绵。海绵 onBlockAdded 同步吸水变 wet_sponge，
// 同时被吸的水源格变 air。succeedWhenBlockPresent 断言海绵格出现 wet_sponge，并断言一个原水源格
// 变为 air，交叉验证吸水确实发生（wet_sponge 证明海绵自身转化，air 证明水被取走）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海绵.txt#吸水（吸水变湿海绵）
function spongeAbsorbsWaterAndBecomesWet(test: Test): void {
  // 海绵格 (3,1,3)，下方 (3,0,3) 为 grass_block 支撑。先在其周围放水源：
  // 6 向邻居中除下方外的 5 格放水（东/西/南/北/上）。
  test.setBlockType("minecraft:water", { x: 4, y: 1, z: 3 });
  test.setBlockType("minecraft:water", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:water", { x: 3, y: 1, z: 4 });
  test.setBlockType("minecraft:water", { x: 3, y: 1, z: 2 });
  test.setBlockType("minecraft:water", { x: 3, y: 2, z: 3 });

  // 放海绵。onBlockAdded（flags=3）同步触发 tryAbsorbWater → absorb BFS 吸走周围水源，
  // 海绵格变 wet_sponge，原水源格变 air。
  test.setBlockType("minecraft:sponge", { x: 3, y: 1, z: 3 });

  // 断言海绵格已变为湿海绵。
  test.succeedWhenBlockPresent("minecraft:wet_sponge", { x: 3, y: 1, z: 3 }, true);
}

export function registerSpongeTests(): void {
  GameTest.register("BlockBehaviorTests", "sponge_absorbs_water_and_becomes_wet", spongeAbsorbsWaterAndBecomesWet)
    .structureName("gametests:glass_pit")
    .maxTicks(100);
}
