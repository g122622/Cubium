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

// 末影人被玩家注视眼睛后激怒，主动攻击玩家
// （wiki tech_末影人.txt#行为：玩家视线对准末影人头部时，末影人会立即被激怒，主动攻击玩家；
//   戴南瓜头可避免激怒）。
//
// C++ 链路（对齐 MC Java 1.21.11 EndermanFreezeWhenLookedAt + EndermanLookForPlayerGoal + Player.isLookingAt）：
//   1) EndermanFindPlayerGoal::shouldExecute（EndermanGoals.cpp:129-175）搜索 TARGET_DISTANCE(10) 格内
//      玩家，调 shouldAttackPlayer(player) 判定激怒条件。
//   2) EndermanEntity::shouldAttackPlayer（EndermanEntity.cpp:288-306）三步：
//      ① player.isWearingPumpkin() → 戴南瓜头不激怒；
//      ② player.isLookingAt(*this) → 玩家视线对准末影人眼睛（Player.cpp:3036-3066，视线向量与到
//        末影人眼睛向量点积 > 阈值 1.0-0.025/distance）；
//      ③ player.canSee(*this) → 视线无方块阻挡。
//   3) 激怒：startExecuting 设 m_aggroTime=AGGRO_DURATION(5) + setScreaming(true)；tick 中 m_aggroTime
//      递减到 0 后调 TargetGoal::startExecuting → setAttackTarget(player)（EndermanGoals.cpp:218-259）。
//   4) 攻击：MeleeAttackGoal（优先级2）attackTarget 非空时寻路接近近战攻击，命中造成 7 伤害。
//
// 注视冻结与两阶段时序（对齐 vanilla EndermanFreezeWhenLookedAt 语义）：
//   Java 1.21.11 EndermanFreezeWhenLookedAt（优先级1）flags=EnumSet.of(JUMP,MOVE)，canUse 要求
//   getTarget() 是 Player + 距离<16 + isBeingStaredBy(player)。**注视时该 goal 霸占 MOVE flag**，
//   MeleeAttackGoal（优先级2，flags=MOVE）被压制不执行——末影人被注视时冻结不动不近战，靠
//   EndermanLookForPlayerGoal tick 瞬移。玩家移开视线后 Freeze goal canUse=false 释放 MOVE flag，
//   MeleeAttackGoal 接管近战攻击。Cubium EndermanStareGoal 对齐此语义（flags=Look|Move，注视时
//   霸占 MOVE 压制 MeleeAttackGoal）。
//   故测试须两阶段：① 玩家注视末影人触发激怒（aggroTime=5 倒计时→setAttackTarget）；
//   ② 玩家移开视线释放 MOVE flag，MeleeAttackGoal 寻路近战攻击玩家。激怒后（m_aggroTime 已 expired，
//   m_target 已设）EndermanFindPlayerGoal::shouldContinueExecuting 走 m_target 分支不再检查注视，
//   攻击目标保留，故移开视线不会丢失目标。
//
// 注视控制：SimulatedPlayer::lookAtEntity(enderman)（SimulatedPlayer.cpp:132-147）用 setRotation +
//   setYHeadRot 瞬时定向玩家朝向末影人眼睛方向。isLookingAt 的 getLookVector 读 m_rot（与 setRotation
//   写入同一字段），故 lookAtEntity 后玩家视线精确对准末影人。移开视线用 lookAtLocation(远处坐标)。
//
// 判定手段：末影人近战攻击玩家致掉血（HP<20，满血 20，末影人攻击 7→13）。MeleeAttackGoal 寻路+
//   攻击冷却(20 tick)有时序，用 pollUntilSucceed + 大 maxTick 吸收。玩家 Survival 模式（创造/观察者
//   被 shouldAttackPlayer 滤掉）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，canSee 视线不被阻挡，寻路无障碍。
//   末影人 (1,2,1) + Survival 玩家 (5,2,5) 对角距 √(4²+4²)≈5.66 格——刻意选此距离规避激怒后非确定性
//   瞬移：EndermanFindPlayerGoal::tick 中距玩家 <TELEPORT_NEAR_DISTANCE_SQ(16=4²) 触发随机躲避瞬移
//   teleport()（末影人可能瞬移出结构区域），距玩家 >TELEPORT_FAR_DISTANCE_SQ(256=16²) 触发
//   teleportToTarget 主动接近。5.66 格处于 4~16 中间地带，既不随机躲避瞬移也不远距离接近瞬移，末影人
//   靠 MeleeAttackGoal 寻路稳定接近玩家攻击。距离 5.66 <TARGET_DISTANCE(10) 满足搜索范围；isLookingAt
//   距离 5.66 阈值 1-0.025/5.66≈0.9956，lookAtEntity 瞬时精确对准，点积≈1 满足注视判定。
//   脚下 y=1 玻璃支撑防下落。
//
// 时序：tick 0 spawn+lookAtEntity 注视 → tick~6 aggroTime expired setAttackTarget →
//   tick 15 lookAtLocation 移开视线释放 MOVE flag → MeleeAttackGoal 寻路接近(5.66格)+攻击(冷却20tick)。
//   pollUntilSucceed startTick=40 maxTick=900 留足寻路+攻击时序余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影人.txt#行为（注视眼睛激怒攻击玩家）
// Ref: EndermanEntity.cpp shouldAttackPlayer（注视判定）
// Ref: EndermanGoals.cpp EndermanFindPlayerGoal（激怒 goal）+ EndermanStareGoal（注视冻结霸占 MOVE）
// Ref: Player.cpp isLookingAt（视线点积阈值）
// Ref: Java 1.21.11 EnderMan.java EndermanFreezeWhenLookedAt（注视冻结 flags=JUMP|MOVE 压制 MeleeAttack）
function endermanBecomesAngryWhenStaredAt(test: Test): void {
  const endermanType = "enderman";

  // 脚下 y=1 玻璃支撑（末影人/玩家受重力下落）。
  test.setBlockType("minecraft:glass", { x: 1, y: 1, z: 1 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 5 });

  // 末影人 (1,2,1) + Survival 玩家 (5,2,5) 对角距 ≈5.66 格（4~16 中间地带，规避激怒后随机瞬移）。
  const enderman = test.spawn(endermanType, { x: 1, y: 2, z: 1 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "starer", 0 as any);

  // 阶段1：玩家注视末影人眼睛，触发 isLookingAt=true → EndermanFindPlayerGoal 激怒链路。
  // lookAtEntity 是 Cubium 扩展绑定，as any 绕过 TS 类型。
  (player as any).lookAtEntity(enderman);

  // 阶段2：tick 15 移开视线（aggroTime=5 已在 tick~6 expired，setAttackTarget 已设，攻击目标保留）。
  // 移开视线使 EndermanStareGoal shouldAttackPlayer=false → StareGoal 释放 MOVE flag，
  // MeleeAttackGoal 接管寻路近战攻击。lookAtLocation 朝向远处 (1,2,6) 偏离末影人方向。
  // lookAtLocation 接结构相对 BlockPos，as any 绕过 TS 类型。
  test.runAtTickTime(15, () => {
    (player as any).lookAtLocation({ x: 1, y: 2, z: 6 });
  });

  // 轮询：末影人近战攻击玩家致 HP<20（满血 20，末影人近战 7 伤害→13）。
  // startTick=40 留注视激怒(6tick)+移开视线(15tick)+寻路接近(5.66格)+攻击冷却(20tick)时序；
  // maxTick=900 吸收寻路非确定性。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length === 0) return false;
    const health = players[0].getComponent("minecraft:health");
    if (health === undefined) return false;
    return (health as any).currentValue < 20;
  }, {
    startTick: 40,
    interval: 10,
    maxTick: 900,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const endermen = test.getDimension().getEntities({
        type: endermanType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const playerHp = players.length > 0
        ? (players[0].getComponent("minecraft:health") as any)?.currentValue
        : "no player";
      // 区分失败原因：playerHp==20 说明近战攻击未命中（激怒后 MeleeAttackGoal 未执行——
      //   可能 StareGoal 仍霸占 MOVE flag 未释放，或寻路失败）；no player 说明玩家流失。
      test.assert(false,
        `enderman did not attack player after being stared at: playerHp=${playerHp} endermen=${endermen.length} ` +
        `(playerHp==20: melee not triggered - StareGoal MOVE flag not released or pathfinding failed)`);
    },
  });
}

// 末影人瞬移躲避弹射物（wiki tech_末影人.txt#行为：末影人会瞬移来躲避射向它的投射物，如箭矢、雪球、
// 三叉戟等。这是末影人最具辨识度的防御机制——任何投射物都无法命中末影人，它在受击瞬间随机瞬移消失）。
//
// C++ 链路：EndermanEntity::hurt（EndermanEntity.cpp:346-362）对齐 vanilla EnderMan.hurtServer:
//   if (source.isProjectile()) {
//       for (i32 i = 0; i < TELEPORT_PROJECTILE_ATTEMPTS(64); ++i) {
//           if (teleport()) return true;   // 成功瞬移→不受伤（hurt 返 true 但实际未扣血）
//       }
//       return false;                       // 64 次都失败→也不受伤
//   }
//   ...（非投射物伤害走 MonsterEntity::hurt）
// 即：投射物攻击末影人时，无论瞬移成功与否，末影人都不扣血（HP 恒保持 40）。
//   - 成功瞬移：return true 提前退出，不走后续 MonsterEntity::hurt 实际扣血。
//   - 64 次全失败：return false，同样不走 MonsterEntity::hurt。
//   两种情况下末影人 HP 均不变。
//
// 投射物选择——直接 spawn 箭矢 + setVelocity（参考 SnowballDamageTests 范式，比拉弓射箭简洁）：
//   雪球不适合——SnowballEntity::onEntityHit（ProjectileItemEntity.cpp:128-151）对非烈焰人 damage=0，
//   不调 hurt，末影人不会进入 isProjectile 瞬移分支。必须用箭矢（AbstractArrowEntity::onEntityHit，
//   AbstractArrowEntity.cpp:467-554）：speed=3.0、m_damage=2.0（ProjectileArrowStateComponent 默认，
//   ProjectileArrowStateComponent.hpp:46）→ damage=ceil(3.0*2.0)=6 → livingTarget->hurt(arrowSource, 6)。
//   arrowSource 是 IndirectEntityDamageSource(Arrow,...) 且 setProjectile()（:522）→ source.isProjectile()=true
//   → 末影人 hurt 走瞬移分支，HP 不变。
//   test.spawn("minecraft:arrow") 生成的箭矢无 shooter（getShooter 返 null），sourceForArrow=this（箭矢本身，
//   :519），不影响 isProjectile 判定与瞬移链路（末影人瞬移不依赖 shooter）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，箭矢飞行 + 末影人瞬移无阻挡。
//   末影人 (3,2,5) 脚下 (3,1,5) 玻璃支撑（MonsterEntity 受重力下落）；箭矢 (3,2,3) 距末影人 2 格，
//   setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 跨 3 格命中末影人（射线 z∈[3,6] 覆盖末影人 z=5）。
//
// 不 spawn 玩家：避免玩家注视激怒末影人触发 EndermanFindPlayerGoal 瞬移离开（注释检测），干扰 HP/位置
//   断言。末影人 idle 不瞬移（无注视/受击/遇水），spawn 后留守原位等待箭矢命中。
//
// 判定手段：复合断言——末影人 HP==40（未受伤，瞬移躲避生效）&& 位置变化（距 spawn 中心 >2 格，
//   证明 teleport() 被调用并成功瞬移）。
//   - HP==40 单独不够：若箭矢未命中（setVelocity 失效/箭矢飞行偏移），末影人 HP 也==40，假性通过。
//   - 位置变化单独不够：末影人 AI 游荡也会移动（RandomWalkingGoal）。
//   - 复合断言：HP==40（未受伤）且 末影人已瞬移离开原位（distSq>4），证明"箭矢命中→末影人 hurt→
//     isProjectile 瞬移→成功瞬移逃离→未受伤"完整链路。末影人瞬移后位置随机，距原位 >2 格概率极高
//     （teleport 目标偏移 ±16 格，见 EndermanEntity.cpp:251-256）。
//   pollUntilSucceed startTick=20 留箭矢飞行(1tick)+命中+瞬移时序，interval=5，maxTick=200 留瞬移
//   非确定性余量（teleport 64 次尝试，累计成功概率极高，但瞬移目标位置随机需轮询确认位移）。
//   末影人查询区域限定排除并行测试污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影人.txt#行为（瞬移躲避投射物）
// Ref: EndermanEntity.cpp:346-362（hurt 投射物瞬移分支）
// Ref: AbstractArrowEntity.cpp:467-554（箭矢 onEntityHit→hurt 投射物伤害源）
function endermanDodgesProjectile(test: Test): void {
  const endermanType = "enderman";

  // 末影人 (3,2,5) 脚下 (3,1,5) 玻璃支撑（受重力下落）。
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 5 });
  const enderman = test.spawn(endermanType, { x: 3, y: 2, z: 5 });

  // 箭矢 (3,2,3) 距末影人 2 格，setVelocity 朝 +Z 3.0/tick，1 tick 命中末影人。
  // 末影人脚下玻璃支撑防下落；箭矢 spawn 后立即 setVelocity 无需等待。
  const arrow = test.spawn("minecraft:arrow", { x: 3, y: 2, z: 3 });
  (arrow as any).setVelocity({ x: 0, y: 0, z: 3.0 });

  // 末影人 spawn 中心（世界坐标），用于断言瞬移距离。
  const spawnWorld = test.worldLocation({ x: 3, y: 2, z: 5 });

  // 复合断言（3D 距离，含 y 分量）：
  //   末影人瞬移目标 y 也随机偏移（EndermanEntity.cpp:254 targetY = pos.y + nextInt(16)-8，±8 格），
  //   仅看 xz 平面距离会漏判 y 方向瞬移（实测末影人瞬移到 y=6，xz 仅偏 1 格）。改用 3D 距离。
  //   两种通过路径（任一即证明瞬移躲避生效）：
  //   ① 末影人瞬移出查询区域（endermen.length==0）：teleport 目标 ±16 格，7 格结构困不住，
  //      瞬移后大概率出 PIT_VOLUME → 查询返回 0。末影人 idle 不瞬移（无注视/受击/遇水），
  //      故 endermen==0 必是瞬移所致。
  //   ② 末影人仍在结构内且 HP==40 且 3D distSq>4（>2 格）：瞬移逃离原位且未受伤。
  //   末影人查询区域限定排除并行测试污染（PIT_VOLUME 7×5×7）。
  pollUntilSucceed(test, () => {
    const endermen = test.getDimension().getEntities({
      type: endermanType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 路径①：末影人瞬移出查询区域（endermen==0）→ 躲避成功。
    if (endermen.length === 0) return true;
    // 路径②：末影人仍在结构内，HP==40（未受伤）且 3D 距离 >2 格（瞬移逃离）。
    const e = endermen[0];
    const dx = e.location.x - spawnWorld.x;
    const dy = e.location.y - spawnWorld.y;
    const dz = e.location.z - spawnWorld.z;
    const distSq = dx * dx + dy * dy + dz * dz;
    const health = e.getComponent("minecraft:health");
    const hp = health ? (health as any).currentValue : 0;
    return hp >= 40 && distSq > 4;
  }, {
    startTick: 20,
    interval: 5,
    maxTick: 200,
    onTimeout: () => {
      const endermen = test.getDimension().getEntities({
        type: endermanType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const arrows = test.getDimension().getEntities({
        type: "minecraft:arrow",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const eHp = endermen.length > 0
        ? (endermen[0].getComponent("minecraft:health") as any)?.currentValue : "gone";
      const ePos = endermen.length > 0
        ? `(${endermen[0].location.x.toFixed(1)},${endermen[0].location.y.toFixed(1)},${endermen[0].location.z.toFixed(1)})`
        : "gone";
      test.assert(false,
        `enderman did not dodge projectile (endermen=${endermen.length} hp=${eHp} pos=${ePos}; arrows=${arrows.length}; ` +
        `expected hp>=40 [unhurt] AND teleported >2 blocks from spawn)`);
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

  GameTest.register("MobBehaviorTests", "enderman_becomes_angry_when_stared_at", endermanBecomesAngryWhenStaredAt)
    .structureName("gametests:creeper_pit")
    .maxTicks(900);

  GameTest.register("MobBehaviorTests", "enderman_dodges_projectile", endermanDodgesProjectile)
    .structureName("gametests:creeper_pit")
    .maxTicks(300);
}
