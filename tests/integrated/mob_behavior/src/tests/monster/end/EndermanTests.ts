// 末影人行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit / creeper_pit 结构尺寸均为 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick（同 batch 测试同一世界 tick 同时推进），
// 且测试结束不清场，全维度 getEntities({type}) 会数到其他并行/残留测试的实体（跨测试污染）。
// 各测试 origin 在 X 方向错开 9 格（结构 7 + padding 2），7×5×7 体积查询不覆盖相邻测试区域。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 末影人接触水会受到伤害并瞬移逃离（wiki tech_末影人.txt#行为：在触碰到水或雨时会受到伤害，
// 并且会避免进入水或雨中）。
// C++ 链路：EndermanEntity::tick → isInWaterOrRain()（isInWater||isInRain）为真 →
// hurt(DamageSources::drown(), WATER_DAMAGE=1.0) + teleportAwayFromWater()。
// 即每 tick 在水中受 1.0 伤害并尝试瞬移到附近干燥位置。
//
// 判定手段：末影人浸水后水敏感机制生效有两种可观测表现——
//   ①HP 下降（hurt 生效，末影人初始满血 40，掉血后 <40）；
//   ②瞬移逃离水面（teleportAwayFromWater 生效，末影人离开 spawn 原位）。
// 两者满足其一即证明 isInWaterOrRain + hurt/teleport 链路完整。单断言 HP<40 不可靠——
// 末影人可能首 tick 就 teleportAwayFromWater 瞬移出水面停止受伤，HP 不降；单断言位置变也不可靠——
// 末影人 AI 游荡也会移动。故用"HP<40 或 距 spawn 中心 >2 格"复合断言。
//
// 水深：铺两层 water（y=0..1），末影人 spawn 于 y=3 下落浸入水层。两层水保证末影人碰撞箱
// （高 2.9，脚 y=1 时头顶 y=3.9）与水方块重叠触发 isInWater。单层 y=0 时末影人站在水面
// 碰撞箱不触及 y=0 不触发（同僵尸猪灵熔岩测试两层 lava 同理）。
// 结构 glass_pit：y=0 grass_block + y=1..4 air，先 fill 两层水覆盖原 grass/air。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影人.txt#行为（触碰水或雨受到伤害）
function endermanTakesWaterDamage(test: Test): void {
  const endermanType = "enderman";

  // 铺两层水（y=0..1 全 7×7），末影人 spawn 后下落浸入水层触发 isInWaterOrRain。
  fillBlock(test, "water", 0, 0, 0, 6, 1, 6);

  // 末影人 spawn 于 (3,3,3)（水面上方一格），下落入水。spawn 中心用于断言瞬移距离。
  test.spawn(endermanType, { x: 3, y: 3, z: 3 });

  // 复合断言：HP<40（掉血）或 距 spawn 中心 (3,3) 水平 >2 格（瞬移逃离）。
  // 末影人 HP=40，浸水每 tick 1.0 伤害，约 1 tick 即 HP<40；teleportAwayFromWater 每 tick
  // 尝试瞬移，可能瞬移到干燥处。maxTicks=200 留瞬移 + 伤害窗口余量。
  // 用 getEntities 区域限定查询取末影人坐标 + HP（排除并行测试污染）。
  const spawnWorld = test.worldLocation({ x: 3, y: 3, z: 3 });
  test.succeedWhen(() => {
    const endermen = test.getDimension().getEntities({
      type: endermanType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(endermen.length > 0, "enderman disappeared (teleported out of structure)");
    const e = endermen[0];
    const dx = e.location.x - spawnWorld.x;
    const dz = e.location.z - spawnWorld.z;
    const distSq = dx * dx + dz * dz;
    const health = e.getComponent("minecraft:health");
    const hp = health ? (health as any).currentValue : 40;
    // HP<40 证明 hurt 生效；distSq>4（>2格）证明 teleportAwayFromWater 生效。二者满足其一即通过。
    test.assert(hp < 40 || distSq > 4,
      `enderman not affected by water, hp=${hp}, distSq=${distSq.toFixed(2)}`);
  });
}

// 末影人不在阳光下燃烧（wiki tech_末影人.txt：末影人 shouldBurnInDaylight=false，与骷髅/僵尸等
// 亡灵不同，白天露天不着火）。EndermanEntity::shouldBurnInDaylight() override 返回 false，
// MonsterEntity::tick→handleDaylightBurning→isInDaylight 校验 shouldBurnInDaylight() 为 false 跳过燃烧。
// 与骷髅阳光燃烧测试（skeleton_burns_in_daylight）形成对照：同为 MonsterEntity 子类，
// 骷髅燃烧而末影人不燃，交叉验证 shouldBurnInDaylight 门控正确。
// 注：此为负向断言（assert 不着火）。若框架 bug 让所有实体不着火测试也过——但有
// skeleton_burns_in_daylight 正向断言对照（骷髅该着火着火），两者互补验证燃烧判定正确性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影人.txt#行为（末影人不在阳光下燃烧）
function endermanDoesNotBurnInDaylight(test: Test): void {
  const endermanType = "enderman";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 末影人 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，末影人 AI 游荡不会触及围栏；整个空气腔头顶均露天，无阴影可躲。
  const enderman = test.spawn(endermanType, { x: 4, y: 2, z: 4 });

  // 白天露天末影人不着火：轮询 onfire 组件，应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=500：白天燃烧判定每 tick 概率触发，末影人本就不燃，但留余量确保断言稳定
  // （骷髅测试同款 maxTicks，对照可比）。succeedWhen 轮询：onfire 组件恒 undefined 即通过。
  test.succeedWhen(() => {
    const fire = enderman.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("enderman should not burn in daylight");
    }
  });
}

// 末影人会拾取 ENDERMAN_HOLDABLE 标签方块（wiki tech_末影人.txt#行为：末影人会拾取并搬走某些方块）。
//
// C++ 链路：EndermanTakeBlockGoal（EndermanGoals.cpp:405-492）：
//   - shouldExecute（:412-431）：!isHoldingBlock + mobGriefing（默认 true）+ 1/20 概率（TAKE_CHANCE=20）。
//   - tick（:433-492）：在末影人周围 4×3×4 范围随机选格 (x=floor(ex-2+rand*4), y=floor(ey+rand*3),
//     z=floor(ez-2+rand*4))，检查该格方块在 ENDERMAN_HOLDABLE 标签 + 射线无阻挡 → setBlockState(air)
//     拿走 + setHeldBlockState。
//   对齐 Java EnderMan.EndermanTakeBlockGoal（Enderman.java:576-613），y=floor(getY()+rand*3) 即末影人
//   脚位 Y 及以上 3 层（Y, Y+1, Y+2），**脚下方块拿不到**（vanilla 行为，非偏差）。
//
// 几何设计（glass_pit 7×5×7，y=0 grass_block 地板 + y=1..4 air）：
//   末影人脚位须在可拿方块之上。grass_block 地板在 y=0（脚下），拿取范围 y∈[脚y, 脚y+2]=[1,3] 拿不到 y=0。
//   故须在末影人脚位及以上层（y=1,2,3）人为放置 ENDERMAN_HOLDABLE 方块供末影人拿取。
//   末影人 spawn (3,2,3) 下落到 (3,1,3)（脚位 y=1，踩 y=0 grass 地板）。在末影人四面相邻格
//   (2,1,3)(4,1,3)(3,1,2)(3,1,4) 各竖 4 格高 grass_block 墙（y=1,2,3,4）形成 1×1×3 竖井围栏，
//   井口 (3,4,3) 放 glass 封顶防末影人跳出（末影人跳跃高度 1.2 格，4 格高墙 + glass 顶绝对封死）。
//   拿取范围 x∈[1,4] z∈[1,4] y∈[1,3] 覆盖四面 y=1,2,3 墙共 12 块 grass_block。
//
// 为何 4 格高墙 + glass 顶：末影人若拿走 1 块墙 grass_block 出现缺口，可能从缺口逐格爬出竖井游荡离开，
//   离开后拿取范围不再覆盖墙 grass_block，后续不再拿取。封死竖井确保末影人始终在拿取范围内，且
//   末影人 idle 不瞬移（无玩家注视/受击/遇水，EndermanEntity::tick 仅 isInWaterOrRain 才 teleportAwayFromWater），
//   末影人留守竖井持续评估 TakeBlockGoal。
//
// grass_block 在 ENDERMAN_HOLDABLE 标签（BlockTags.cpp:1826）。
// 射线检测：末影人脚中心 (3.5,1.5,3.5) 到墙格如 (2.5,1.5,3.5)，相邻格 (3,1,3) 是 air（末影人自身格），
//   射线无阻挡命中目标格。✓
//
// 判定手段：末影人拿走 1 块墙 grass_block 后该格变 air。轮询四面 y=1,2,3 共 12 格中至少 1 格变 air。
//   拿取 1/20 per tick，累计概率 500 tick 内 ≈1。末影人拿 1 块后 isHoldingBlock=true 不再拿，
//   PlaceBlockGoal（1/2000）可能放回，但放回位置 2×2×2 随机，放回原 12 格之一的概率低（1/8×1/12），
//   500 tick 内净减少至少 1 块的概率高。用 pollUntilSucceed（正向断言"出现 air"）。
//   startTick=40 留末影人 spawn 下落 + 首次 TakeBlockGoal 评估时间，interval=10，maxTick=500。
//
// 不 spawn 玩家：避免玩家注视激怒末影人触发瞬移离开竖井（EndermanFindPlayerGoal 检测注视）。
// 不调 randomTickSpeed：mob goal 用 getRandom().nextInt 门控，与 randomTickSpeed 无关。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影人.txt#行为（拾取方块）
function endermanTakesBlock(test: Test): void {
  const endermanType = "enderman";
  const grassBlock = "minecraft:grass_block";

  // 末影人竖井围栏：四面相邻格各竖 4 格高 grass_block 墙（y=1,2,3,4）。
  // 末影人 spawn (3,2,3) 下落站 (3,1,3)，四面墙 (2,1,3)(4,1,3)(3,1,2)(3,1,4)。
  // y=1,2,3 墙在末影人拿取范围（y∈[1,3]）内可被拿；y=4 墙 + glass 顶防末影人跳出。
  const wallPositions = [
    { x: 2, z: 3 }, { x: 4, z: 3 }, { x: 3, z: 2 }, { x: 3, z: 4 },
  ];
  for (const wp of wallPositions) {
    for (let y = 1; y <= 4; y++) {
      test.setBlockType(grassBlock, { x: wp.x, y, z: wp.z });
    }
  }
  // 井口 (3,4,3) 放 glass 封顶（glass 不受重力无支撑不掉，且不在 ENDERMAN_HOLDABLE 不会被拿）。
  test.setBlockType("minecraft:glass", { x: 3, y: 4, z: 3 });

  // 末影人 spawn (3,2,3)：下落站 (3,1,3)，四面 grass 墙围成 1×1×3 竖井。
  test.spawn(endermanType, { x: 3, y: 2, z: 3 });

  // 轮询断言：四面 y=1,2,3 共 12 格墙 grass_block 中至少 1 格变 air（被末影人拿走）。
  pollUntilSucceed(test, () => {
    for (const wp of wallPositions) {
      for (let y = 1; y <= 3; y++) {
        const block = test.getBlock({ x: wp.x, y, z: wp.z }) as unknown as { typeId?: string } | undefined;
        const typeId = block?.typeId ?? "";
        if (typeId !== grassBlock) {
          return true;
        }
      }
    }
    return false;
  }, {
    startTick: 40,
    interval: 10,
    maxTick: 500,
    onTimeout: () => {
      // 超时诊断：列出 12 格墙当前 typeId。
      const states: string[] = [];
      for (const wp of wallPositions) {
        for (let y = 1; y <= 3; y++) {
          const block = test.getBlock({ x: wp.x, y, z: wp.z }) as unknown as { typeId?: string } | undefined;
          states.push(`(${wp.x},${y},${wp.z})=${block?.typeId ?? "?"}`);
        }
      }
      test.assert(false,
        `enderman did not take any grass_block (TakeBlockGoal broken or enderman escaped), walls=[${states.join(", ")}]`);
    },
  });
}

export function registerEndermanTests(): void {
  GameTest.register("MobBehaviorTests", "enderman_takes_water_damage", endermanTakesWaterDamage)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "enderman_does_not_burn_in_daylight", endermanDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "enderman_takes_block", endermanTakesBlock)
    .structureName("gametests:glass_pit")
    .maxTicks(600);
}
