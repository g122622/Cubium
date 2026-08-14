// 鱿鱼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 鱿鱼离开水后随氧气耗尽开始受到脱水伤害（wiki tech_鱿鱼.txt#行为：鱿鱼离开水后无法移动，
// 并且会在 300 后开始受到脱水伤害）。
//
// C++ 链路：鱿鱼 SquidEntity : WaterMobEntity。WaterMobEntity::tick（WaterMobEntity.cpp:105-111）
// 每 tick 调 updateAirSupply（:121-164）。水生生物反逻辑：在水中 setAir(maxAir()) 立即回满；
// 不在水中每 tick air()-1，当 shouldTakeDrowningDamage()（LivingEntity.cpp:2216-2221，air()<=-20）
// 为 true 时 setAir(0) + broadcastEntityStatus(67) + hurt(drown, 2.0F)。
// 鱿鱼 maxAir 未重写 = 300（Entity.hpp:1849 默认），离水后 air 300→0 需 300 tick，0→-20 再 20 tick，
// 故首次窒息伤害在第 ~320 tick（对齐 wiki"300 后开始"——300 指氧气耗尽时间，伤害在其后 ~20 tick）。
// 窒息伤害 2.0F（DROWN_DAMAGE_AMOUNT，PhysicsConstants.hpp:205），鱿鱼 maxHealth=10，首次 10→8。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air 无围墙）。陆地空气层
// 即"不在水中"，updateAirSupply 走窒息分支。结构放置有 +1 抬升：helper-y=N → 结构内 y=N-1。
// 鱿鱼 spawn 于 (3,2,3)（helper-y=2 → 结构内 y=1 空气，脚踩结构内 y=0 grass_block），无水即窒息。
// 鱿鱼陆地无法移动（wiki 语义），SquidMoveRandomGoal 在陆地设移动向量但无水浮力扑腾，不影响窒息时序。
//
// 判定手段：succeedWhen 每 tick 检查鱿鱼 health.currentValue < 10（满血 10，首次窒息后掉至 8）。
// air 本身未暴露 JS（无绑定），血量下降是窒息链路生效的等价且更强的断言。区域限定 getEntities
// 取鱿鱼实体读 health 组件，排除并行测试污染。窒息是确定性时序（纯 tick 递减，零随机），非 flaky。
// maxTicks=400 留足 320 tick 首次窒息 + 80 tick 调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鱿鱼.txt#行为（离开水后脱水伤害）
function squidSuffocatesOutOfWater(test: Test): void {
  const squidType = "squid";

  // 鱿鱼 spawn 于 (3,2,3)（creeper_pit 陆地空气层，脚踩结构内 y=0 grass_block，无水）。
  // helper-y=2 → 结构内 y=1 空气。陆地环境触发 updateAirSupply 窒息分支，air 每 tick 递减。
  test.spawn(squidType, { x: 3, y: 2, z: 3 });

  // 断言鱿鱼受脱水伤害：succeedWhen 每 tick 检查 health.currentValue < 10。
  // 时序：air 300→0（300 tick）+ 0→-20（20 tick）= 320 tick 首次窒息 hurt(2.0)，10→8 < 10 满足。
  // 区域限定用 PIT（creeper_pit 7×5×7）排除并行测试污染。
  // 用 getComponent("minecraft:health").currentValue 读血量（全测试套件通用模式），
  // (health as any) 绕过 TS 类型（currentValue 在类型定义中为 readonly 但运行时可读）。
  test.succeedWhen(() => {
    const squids = test.getDimension().getEntities({
      type: squidType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(squids.length > 0, "squid disappeared before suffocating");
    const health = squids[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 10,
      `squid did not take suffocation damage, hp=${(health as any).currentValue}`);
  });
}

export function registerSquidTests(): void {
  GameTest.register("MobBehaviorTests", "squid_suffocates_out_of_water", squidSuffocatesOutOfWater)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);
}
