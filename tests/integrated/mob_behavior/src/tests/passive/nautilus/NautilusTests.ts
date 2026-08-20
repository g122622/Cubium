// 鹦鹉螺行为类 GameTest。
//
// 鹦鹉螺是 1.21.11 新生物,mob_behavior 包零测试。本测试验证其陆地干涸(dryout)伤害机制——
// 区别于鳕鱼/鱿鱼的 drown(溺水)伤害,鹦鹉螺用独立的 dryout 伤害类型(对齐 MC 1.21.11
// Nautilus.handleAirSupply 原版语义)。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 鹦鹉螺离开水后随氧气耗尽开始受到干涸伤害（wiki tech_鹦鹉螺.txt#行为：鹦鹉螺是水生生物，
//   离开水后会逐渐干涸并受到伤害；与鱼类的溺水伤害不同，鹦鹉螺使用 dryout 伤害类型）。
//
// C++ 链路（对齐 MC Java 1.21.11 Nautilus.handleAirSupply）：
//   NautilusEntity : AbstractNautilusEntity : TameableEntity : AnimalEntity（不继承 WaterMobEntity，
//   故水生行为由子类手动覆写）。NautilusEntity::updateAirSupply（NautilusEntity.cpp:199-220）是
//   虚函数覆写，完整逻辑：
//     - !isInWater() 时每 tick air()-1，setAir(newAir)；
//     - newAir <= -20 时 setAir(0) + hurt(DamageSources::dryout(), 2.0f)（dryout 伤害类型，非 drown）；
//     - isInWater() 时 setAir(maxAir()) 立即回满。
//   调用链：NautilusEntity::tick（未重写）→ AbstractNautilusEntity::tick → ... → LivingEntity::tick
//   → updateAirSupply()（虚调用，派发到 NautilusEntity::updateAirSupply）。
//
//   maxAir() 覆写返回 MAX_AIR_SUPPLY=300（AbstractNautilusEntity.hpp:433），构造时 setAir(300)。
//   故陆地 air 300→0 需 300 tick + 0→-20 需 20 tick = 首次干涸伤害在第 ~320 tick。
//   MAX_HEALTH=15.0（AbstractNautilusEntity.cpp:726-736），首次 hurt(2.0) 后 15→13。
//
// 与鳕鱼/鱿鱼窒息机制差异（本测试核心价值）：
//   - 伤害类型：鹦鹉螺 dryout（DamageSources::dryout），鳕鱼/鱿鱼 drown（DamageSources::drown）。
//     两者数值相同（2.0）但类型独立，对齐 1.21.11 原版语义。
//   - maxAir：鹦鹉螺 300，鳕鱼 480（AbstractFishEntity.hpp:222）。鹦鹉螺首伤 ~320 tick，
//     鳕鱼首伤 ~500 tick。故 maxTicks 不同（鹦鹉螺 400，鳕鱼 580）。
//   - 鹦鹉螺无 updateFlopping（无扑腾随机跳跃漂移），鳕鱼有（需围栏防漂移）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air 无围墙）。陆地空气层
//   即"不在水中"，updateAirSupply 走干涸分支。结构放置有 +1 抬升：helper-y=N → 结构内 y=N-1。
//   鹦鹉螺 spawn 于 (3,2,3)（helper-y=2 → 结构内 y=1 空气，脚踩结构内 y=0 grass_block），无水即干涸。
//   鹦鹉螺受重力（未 setNoGravity）下落 1 格落地稳定，无需额外垫底。
//
// 围栏：鹦鹉螺无 updateFlopping 扑腾（理论上不需围栏），但 MOVEMENT_SPEED=1.0 较快且
//   setStepHeight(1.0) 可走 1 格高方块，陆地可能游荡。保守照搬鳕鱼测试的 3×3 内圈 2 格高玻璃
//   围栏（y=2,y=3），防止游荡出 7×7 区域。围栏不影响干涸时序（updateAirSupply 每 tick 独立递减
//   air，与位置无关）。2 格高围栏足以阻挡（鹦鹉螺 setStepHeight 1.0 仅走 1 格，跳不出 2 格墙）。
//
// 判定手段：succeedWhen 每 tick 检查鹦鹉螺 health.currentValue < 15（满血 15，首次干涸后掉至 13）。
//   air 本身未暴露 JS（无绑定），血量下降是干涸链路生效的等价且更强的断言。区域限定 getEntities
//   取鹦鹉螺实体读 health 组件，排除并行测试污染。干涸是确定性时序（纯 tick 递减，零随机），非 flaky。
//   **maxTicks=400 留足 320 tick 首次干涸 + 80 tick 调度余量**——切勿照搬 CodTests 的 580，
//   鹦鹉螺 maxAir=300 远小于鳕鱼 480，580 tick 内鹦鹉螺早已多次干涸伤害（400 即够，对齐 SquidTests
//   的 400，鱿鱼 maxAir=300 首伤时序与鹦鹉螺相同）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鹦鹉螺.txt#行为（离开水后干涸伤害）
function nautilusDrysOutOnLand(test: Test): void {
  const nautilusType = "nautilus";

  // 鹦鹉螺 spawn 于 (3,2,3)（creeper_pit 陆地空气层，脚踩结构内 y=0 grass_block，无水）。
  // helper-y=2 → 结构内 y=1 空气。陆地环境触发 updateAirSupply 干涸分支，air 每 tick 递减。
  test.spawn(nautilusType, { x: 3, y: 2, z: 3 });

  // 玻璃围栏：在鹦鹉螺周围 3×3 内圈设 2 格高（y=2,y=3）玻璃墙，顶部不封。
  // 鹦鹉螺无扑腾但 MOVEMENT_SPEED=1.0 较快，围栏防游荡出 7×7 区域。跳过中心格 (3,3)（鹦鹉螺所在）。
  for (const dx of [-1, 0, 1]) {
    for (const dz of [-1, 0, 1]) {
      if (dx === 0 && dz === 0) {
        continue;
      }
      test.setBlockType("minecraft:glass", { x: 3 + dx, y: 2, z: 3 + dz });
      test.setBlockType("minecraft:glass", { x: 3 + dx, y: 3, z: 3 + dz });
    }
  }

  // 断言鹦鹉螺受干涸伤害：succeedWhen 每 tick 检查 health.currentValue < 15。
  // 时序：air 300→0（300 tick）+ 0→-20（20 tick）= 320 tick 首次干涸 hurt(2.0)，15→13 < 15 满足。
  // 区域限定用 PIT（creeper_pit 7×5×7）排除并行测试污染。
  // (health as any) 绕过 TS 类型（currentValue 在类型定义中为 readonly 但运行时可读）。
  test.succeedWhen(() => {
    const nautiluses = test.getDimension().getEntities({
      type: nautilusType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(nautiluses.length > 0, "nautilus disappeared before drying out");
    const health = nautiluses[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 15,
      `nautilus did not take dryout damage, hp=${(health as any).currentValue}`);
  });
}

export function registerNautilusTests(): void {
  GameTest.register("MobBehaviorTests", "nautilus_drys_out_on_land", nautilusDrysOutOnLand)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);
}
