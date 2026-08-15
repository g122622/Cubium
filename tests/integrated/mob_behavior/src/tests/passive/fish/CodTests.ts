// 鳕鱼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 鳕鱼离开水后随氧气耗尽开始受到脱水伤害（wiki tech_鳕鱼.txt#行为：鱼离开水后会扑腾，
// 并在一段时间后开始受到脱水伤害）。
//
// C++ 链路：CodEntity : AbstractGroupFishEntity : AbstractFishEntity : WaterMobEntity。
// AbstractFishEntity 重写 maxAir() 返回 MAX_AIR_SUPPLY=480（AbstractFishEntity.hpp:222），
// 构造时 setAir(maxAir()) 初始化为 480（AbstractFishEntity.cpp:76）。WaterMobEntity::tick
// （WaterMobEntity.cpp:105-111）每 tick 调 updateAirSupply（:121-164）。水生生物反逻辑：
// 在水中 setAir(maxAir()) 立即回满；不在水中每 tick air()-1，当 shouldTakeDrowningDamage()
// （LivingEntity.cpp:2216-2221，air()<=-20）为 true 时 setAir(0) + broadcastEntityStatus(67)
// + hurt(drown, 2.0F)。鳕鱼离水后 air 480→0 需 480 tick，0→-20 再 20 tick，故首次窒息伤害
// 在第 ~500 tick（区别于鱿鱼 maxAir=300 首伤 ~320 tick）。
// 窒息伤害 2.0F（DROWN_DAMAGE_AMOUNT），鳕鱼 MAX_HEALTH=3.0（AbstractFishEntity.cpp:127），
// 首次 3→1。
//
// 扑腾与围栏（关键）：AbstractFishEntity::updateFlopping 每 100 tick 且 onGround 时
// addVelocity(dx,dz=±0.05, dy=0.4)（AbstractFishEntity.cpp:143-164）。鱼跳起后滞空约 8-10 tick，
// 水平速度 ±0.05 在滞空期间持续作用，单次扑腾水平位移可达 ~0.5 格。580 tick 内约 5 次扑腾，
// 累积水平漂移可达数格——足以让鳕鱼漂出 creeper_pit 7×7 区域进入相邻区块。鳕鱼跨区块移动会触发
// 区块加载/卸载与光照引擎异步任务交错，命中已存在的光照引擎区块卸载竞态（PalettedContainer
// 悬垂访问，performLightDecrease 崩溃 0xC0000005）。故在鳕鱼周围设玻璃围栏将其限制在 (3,2,3)
// 附近 3×3 内圈，杜绝跨区块漂移。围栏 2 格高（y=2,y=3）阻挡跳跃的鳕鱼，顶部不封（鱼跳不出 2 格）。
// 围栏不影响窒息时序（updateAirSupply 每 tick 独立递减 air，与位置无关）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air 无围墙）。陆地空气层
// 即"不在水中"，updateAirSupply 走窒息分支。结构放置有 +1 抬升：helper-y=N → 结构内 y=N-1。
// 鳕鱼 spawn 于 (3,2,3)（helper-y=2 → 结构内 y=1 空气，脚踩结构内 y=0 grass_block），无水即窒息。
//
// 判定手段：succeedWhen 每 tick 检查鳕鱼 health.currentValue < 3（满血 3，首次窒息后掉至 1）。
// air 本身未暴露 JS（无绑定），血量下降是窒息链路生效的等价且更强的断言。区域限定 getEntities
// 取鳕鱼实体读 health 组件，排除并行测试污染。窒息是确定性时序（纯 tick 递减，零随机），非 flaky。
// **maxTicks=580 留足 500 tick 首次窒息 + 80 tick 调度余量**——切勿照搬 SquidTests 的 400，
// 鱼类 maxAir=480 远大于鱿鱼 300，400 tick 内鳕鱼 air 尚剩 80 未窒息，测试会超时失败。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鳕鱼.txt#行为（离开水后脱水伤害）
function codSuffocatesOutOfWater(test: Test): void {
  const codType = "cod";

  // 鳕鱼 spawn 于 (3,2,3)（creeper_pit 陆地空气层，脚踩结构内 y=0 grass_block，无水）。
  // helper-y=2 → 结构内 y=1 空气。陆地环境触发 updateAirSupply 窒息分支，air 每 tick 递减。
  test.spawn(codType, { x: 3, y: 2, z: 3 });

  // 玻璃围栏：在鳕鱼周围 3×3 内圈设 2 格高（y=2,y=3）玻璃墙，顶部不封（鱼跳不出 2 格）。
  // 阻挡 updateFlopping 扑腾的水平漂移（单次扑腾水平位移 ~0.5 格，580 tick 累积数格可跨区块），
  // 杜绝鳕鱼漂出 creeper_pit 进入相邻区块触发区块加载/卸载交错。
  // 光照竞态已修（ChunkData 区块级读写锁串行化 worker 光照读与主线程 setBlockState 写），
  // 围栏 setBlockType 触发的光照传播不再引发 PalettedContainer 悬垂访问崩溃。
  // 围栏不影响窒息时序（updateAirSupply 每 tick 独立递减 air，与位置无关）。
  for (const dx of [-1, 0, 1]) {
    for (const dz of [-1, 0, 1]) {
      // 跳过中心格 (3,3)（鳕鱼所在，放玻璃会卡住实体）
      if (dx === 0 && dz === 0) {
        continue;
      }
      test.setBlockType("minecraft:glass", { x: 3 + dx, y: 2, z: 3 + dz });
      test.setBlockType("minecraft:glass", { x: 3 + dx, y: 3, z: 3 + dz });
    }
  }

  // 断言鳕鱼受脱水伤害：succeedWhen 每 tick 检查 health.currentValue < 3。
  // 时序：air 480→0（480 tick）+ 0→-20（20 tick）= 500 tick 首次窒息 hurt(2.0)，3→1 < 3 满足。
  // 区域限定用 PIT（creeper_pit 7×5×7）排除并行测试污染。
  // 用 getComponent("minecraft:health").currentValue 读血量（全测试套件通用模式），
  // (health as any) 绕过 TS 类型（currentValue 在类型定义中为 readonly 但运行时可读）。
  test.succeedWhen(() => {
    const cods = test.getDimension().getEntities({
      type: codType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(cods.length > 0, "cod disappeared before suffocating");
    const health = cods[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 3,
      `cod did not take suffocation damage, hp=${(health as any).currentValue}`);
  });
}

export function registerCodTests(): void {
  GameTest.register("MobBehaviorTests", "cod_suffocates_out_of_water", codSuffocatesOutOfWater)
    .structureName("gametests:creeper_pit")
    .maxTicks(580);
}
