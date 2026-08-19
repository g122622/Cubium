// 溺尸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 溺尸在阳光下着火（wiki mob_溺尸_ED.txt#行为：溺尸是亡灵生物，会在阳光下着火，与僵尸一致）。
// 溺尸是亡灵生物（drowned = 僵尸的水生变种），分类"亡灵生物"，白天露天陆地燃烧。
//
// C++ 链路：DrownedEntity : ZombieEntity : MonsterEntity。MonsterEntity::handleDaylightBurning()
// 调用虚函数 shouldBurnInDaylight()，DrownedEntity override 返回 !isInWater()——陆地燃、水中不燃
// （对齐原版 Drowned 只在陆地燃烧）。grass_pen 陆地 spawn 溺尸不在水中（isInWater=false），
// shouldBurnInDaylight=true。burnUndead()→isInDaylight() 校验 isDaytime + brightness>0.5 + !isWet
// + canSeeSky，全部满足则 igniteForSeconds(8.0f) 点燃。
//
// 与 zombie_burns_in_daylight（僵尸燃）+ husk_does_not_burn_in_daylight（尸壳不燃）
// 形成对照：溺尸同为亡灵但与尸壳（沙漠变种不燃）不同，水生变种在陆地仍燃。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_溺尸_ED.txt#行为（亡灵生物，阳光下着火）
function drownedBurnsInDaylight(test: Test): void {
  const drownedType = "drowned";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 溺尸 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙；整个空气腔头顶均露天无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const drowned = test.spawn(drownedType, { x: 4, y: 2, z: 4 });

  // 时序：isInDaylight() 确定性检查（亮度达标即每 tick 必然燃烧，vanilla 无随机检查）。
  // 溺尸陆地 canSeeSky=true，首 tick 即 burnUndead→igniteForSeconds 持续燃烧。
  // grass_pen 无阴影，溺尸无处可躲。
  // maxTicks=500 留充足余量（与 zombie_burns/skeleton_burns/stray_burns/bogged_burns 同款）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = drowned.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("drowned not on fire yet");
    }
  });
}

// 溺尸夜间主动近战攻击玩家致掉血（wiki mob_溺尸_ED.txt#行为：溺尸追逐和攻击玩家；
// #行为 白天未持三叉戟的溺尸对陆地生物被动，夜间则主动攻击陆地玩家）。
//
// C++ 链路：DrownedEntity registerGoals：
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标。
//   goalSelector 优先级2：DrownedAttackGoal(this, 1.0, true)（继承 MeleeAttackGoal）。
// DrownedAttackGoal::shouldExecute 调 okTarget(target) 过滤：
//   非白天（!isBrightOutside）返回 true（夜间陆地目标有效）；
//   白天仅 target->isInWater() 返回 true。
// 故夜间陆地溺尸近战攻击玩家——okTarget 通过。MeleeAttackGoal 到攻击冷却即 hurt，伤害 3.0。
//
// 环境选择：必须夜晚 batch("night") + creeper_pit（开放坑）。原因：
//   1. 白天陆地 okTarget=false，溺尸近战 goal 不执行（被动），必须夜间测攻击。
//   2. 溺尸是亡灵白天会燃烧，夜间避免燃烧干扰。
// creeper_pit 开放坑无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 溺尸(2,2,3)+玩家(3,2,3)，水平距 1 格 < 1.11 近战攻击距离，选目标后直接命中。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 3.0，玩家满血 20 → 17。
// 确定型近战用"玩家掉血"判定稳定。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_溺尸_ED.txt#行为（追逐攻击玩家，夜间主动）
function drownedAttacksPlayerAtNight(test: Test): void {
  const drownedType = "drowned";

  // 溺尸 (2,2,3)、Survival 玩家 (3,2,3)，水平距 1 格，同处结构 y=2 层。
  // 距 1 格 < 1.11 攻击距离，溺尸选目标后 DrownedAttackGoal 直接命中（无需寻路接近）。
  // 溺尸受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (3,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(drownedType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：夜间 okTarget 通过 + NearestAttackableTarget 选目标 + DrownedAttackGoal 攻击冷却约 20 tick。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `drowned did not damage player, hp=${(health as any).currentValue}`);
  });
}

// 僵尸在水中浸没足够时间后转化为溺尸（wiki mob_僵尸_ED.txt#僵尸变种：僵尸在水下 30 秒后
// 开始转化为溺尸，转化过程 15 秒，转化完成变为溺尸）。
//
// C++ 链路：ZombieEntity::_updateDrowning（ZombieEntity.cpp:661-691）每 tick 由 tick() 调用：
//   if (isInWater() && shouldDrown()) { m_inWaterTime++; 若 >=IN_WATER_TIME_THRESHOLD(600) 且
//   !m_converting 则 startDrowning(CONVERSION_DURATION=300); } else { m_inWaterTime=0; }
//   startDrowning 设 m_converting=true、m_conversionTime=300，每 tick 递减，到 0 调 convertToDrowned。
// convertToDrowned（ZombieEntity.cpp:563-659）：创建 drowned 实体，复制位置/生命值比例/装备/婴儿状态/
//   持久化，spawnEntity 生成溺尸，清空原僵尸装备防掉落，播放转化音效，remove() 移除原僵尸。
// shouldDrown()：ZombieEntity 返回 true（可转化），DrownedEntity/ZombieVillagerEntity 返回 false。
// 无 gamerule/难度门控（doMobSpawning 不影响溺水转化）。
//
// 时序：水下 600 tick（30s）启动 + 转化 300 tick（15s）= 900 tick 完成。spawn 注册 + 下落浸水 + 余量，
// maxTicks=1100。
//
// 环境选择：glass_pit（7×5×7），y=0 grass_block 地板保留（支撑僵尸防下落出结构），fillBlock 铺
//   y=1..3 三层 water 覆盖僵尸全身。僵尸 spawn 于 (3,2,3) 下落到 y=1 站 grass 上，身体 y=1..2.95
//   全浸入 y=1..3 水层 → isInWater=true。night batch：水中 isWet=true 本就不燃，night 双保险避免
//   任何白天燃烧干扰（溺水转化不依赖时间，night 不影响）。
//
// 判定手段：转化完成后原 zombie 消失（remove）+ drowned 出现。pollUntilSucceed 轮询：
//   区域内 drowned 数>=1 且 zombie 数==0。两条同时成立证明转化完成。
//   转化是 900 tick 后确定性发生（无随机），pollUntilSucceed 在 maxTick=1100 内捕获。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_僵尸_ED.txt#僵尸变种（水下转化为溺尸）
function zombieConvertsToDrownedInWater(test: Test): void {
  const zombieType = "zombie";
  const drownedType = "drowned";

  // 铺三层水（y=1..3 全 7×7），y=0 保留 grass_block 地板支撑僵尸。
  // 三层水保证僵尸身体（高 1.95）完全浸没触发 isInWater。
  fillBlock(test, "water", 0, 1, 0, 6, 3, 6);

  // 僵尸 spawn 于 (3,2,3)，下落到 y=1 站 grass 上，身体浸入 y=1..3 水层。
  test.spawn(zombieType, { x: 3, y: 2, z: 3 });

  // 轮询：转化完成后 drowned>=1 且 zombie==0。
  // 900 tick 转化确定性发生，maxTick=1100 留 spawn+下落+余量。
  pollUntilSucceed(test, () => {
    const drowned = test.getDimension().getEntities({
      type: drownedType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    const zombies = test.getDimension().getEntities({
      type: zombieType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return drowned.length >= 1 && zombies.length === 0;
  }, {
    maxTick: 1100,
    onTimeout: () => test.assert(false,
      `zombie did not convert to drowned (drowned=${0}, zombie=${0})`),
  });
}

export function registerDrownedTests(): void {
  GameTest.register("MobBehaviorTests", "drowned_burns_in_daylight", drownedBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
    .skyAccess(true)
    // setupTicks(20)：清空上方方块后 skyLight 入队需 tick 重算稳定（见 zombie_burns_in_daylight 同款注释）。
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "drowned_attacks_player_at_night", drownedAttacksPlayerAtNight)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "zombie_converts_to_drowned_in_water", zombieConvertsToDrownedInWater)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(1100);
}
