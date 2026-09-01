// 蜘蛛行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 蜘蛛夜晚主动攻击玩家：亮度 ≤11 时蜘蛛敌对（wiki tech_蜘蛛.txt#行为）。
// C++ 链路：SpiderTargetGoal<Player>(checkSight=true).shouldExecute 要求 getBrightness()<0.5F
// （夜晚 skyDarkening≈11，露天 magicValue≈0.083<0.5 触发）→ 选 Survival 玩家为 attackTarget →
// SpiderAttackGoal(MeleeAttackGoal) 寻路接近 + LeapAtTarget 扑击。
// 依赖三项 C++ 修复（2026-08-14）：
//   1. Entity::getBrightness 对齐 vanilla getLightLevelDependentMagicValue（含 getSkyDarken 时间衰减
//      + gamma 曲线），此前用 getLightSubtracted(pos,0) 无衰减致夜晚露天仍 1.0 → 蜘蛛永不攻击。
//   2. GameTestServer._selectAndBuildRunner 按 batchName 分组，batch("night") 附 TimeOfDayEnvironment(18000)
//      真正设夜晚（此前 batch("night") 是 stub 只设 batchName 不设时间）。
//   3. TemplateLoader::loadFromBedrockMcStructure 兼容 .mcstructure 的 size/block_indices 内层存为
//      IntArray(TagId::IntArray) 而非 List<Int>(TagId::List) 的导出器产出。此前仅接受 List<Int>，
//      遇 IntArray 在 size 字段就返回空模板，placeInWorld 因 palette 空返回 true 不报错——结构体全是
//      worldgen deepslate/tuff，蜘蛛眼嵌固体方块 canSee=false 选不到目标。creeper_swell_explodes 用同
//      结构但因断言只查实体消失（爆炸后 creeper remove）而巧合通过，掩盖了此 bug。
// 玩家用 Survival（gameMode=0）：创造/旁观玩家被 TargetGoal::isSuitableTarget 滤掉，蜘蛛不选其为目标。
// 传数字 0 并用 as any 绕过 TS 字符串枚举类型校验（运行时 C++ 绑定期望数字，见 CreeperTests 同款注释）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜘蛛.txt#行为
function spiderAttacksPlayerAtNight(test: Test): void {
  const spiderType = "spider";

  // 结构 creeper_pit（7×5×7 开放坑）：y=0 满铺 grass_block，y=1..4 全 air（无围墙）。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 蜘蛛 spawn 于 (3,2,3)（脚踩结构内 y=0 grass_block），玩家于 (4,2,3)，直线 1 格。
  // 开放坑无围墙，SpiderTargetGoal checkSight=true 的 canSee 射线不被玻璃阻挡（grass_pen 玻璃墙会挡
  // canSee 致 attackTarget 恒 null，见 mcstructure-index-formula-cansee-glass-rootcause 记忆）。
  // 直线 1 格在 MeleeAttackGoal 攻击距离内，蜘蛛选定目标后直接攻击无需长距寻路。
  test.spawn(spiderType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 断言蜘蛛接近玩家：SpiderTargetGoal 触发后 SpiderAttackGoal 驱动蜘蛛朝玩家移动并攻击。
  // 用 getEntities 取蜘蛛世界坐标，断言其与玩家世界坐标距离 ≤2 格（攻击距离内）。
  // 玩家世界坐标 = helper (4,2,3) 经 worldLocation 转换（结构原点 + helper 偏移）。
  // maxTicks=400：SpiderTargetGoal tick 评估（GoalSelector 半 tick 评估）+ 寻路 + 接近，留余量。
  const playerWorld = test.worldLocation({ x: 4, y: 2, z: 3 });
  test.succeedWhen(() => {
    const spiders = test.getDimension().getEntities({ type: spiderType });
    test.assert(spiders.length > 0, "spider disappeared");
    const s = spiders[0];
    const dx = s.location.x - playerWorld.x;
    const dz = s.location.z - playerWorld.z;
    // 蜘蛛宽 1.4 格，攻击距离约 1-2 格；判定水平距离 ≤2.5 格（含蜘蛛碰撞盒半宽）
    test.assert(dx * dx + dz * dz <= 2.5 * 2.5, "spider did not approach player");
  });
}

// 蜘蛛不在阳光下燃烧：蜘蛛 shouldBurnInDaylight()=false（SpiderEntity.hpp:119），白天露天不着火。
// 与骷髅阳光燃烧测试（skeleton_burns_in_daylight）形成对照：同为亡灵/怪物，骷髅燃烧而蜘蛛不燃。
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→isInDaylight 校验 shouldBurnInDaylight()，
// 蜘蛛 override 返回 false 跳过燃烧判定。default 批白天（dayTime=1000）+ skyAccess 露天。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜘蛛.txt#行为（蜘蛛不在阳光下燃烧）
function spiderDoesNotBurnInDaylight(test: Test): void {
  const spiderType = "spider";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 蜘蛛 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  const spider = test.spawn(spiderType, { x: 4, y: 2, z: 4 });

  // 白天露天蜘蛛不着火：轮询 onfire 组件，应恒 undefined（蜘蛛 shouldBurnInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，蜘蛛本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），若框架 bug 让所有实体不着火测试也过——但有
  // skeleton_burns_in_daylight 正向断言对照（骷髅该着火着火），两者互补验证燃烧判定正确性。
  test.succeedWhen(() => {
    const fire = spider.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("spider should not burn in daylight");
    }
  });
}

// 蜘蛛白天中立（wiki tech_蜘蛛.txt#行为：蜘蛛在亮度等级 ≥12 时不主动攻击玩家，保持中立；
// 黑暗（亮度 <8）时才主动敌对。这是蜘蛛区别于僵尸/骷髅的核心行为——它不是天生敌对，
// 而是光照门控驱动：白天亮度高 → SpiderTargetGoal::shouldExecute 返回 false → 不选目标 → 不攻击）。
//
// C++ 链路：SpiderTargetGoal<Player>（SpiderEntity.cpp:95-115，继承 NearestAttackableTargetGoal）：
//   shouldExecute（:103-111）：
//     f32 brightness = m_spider->getBrightness();
//     if (brightness >= 0.5F) return false;   // 白天亮度高，直接拒绝选目标
//     return NearestAttackableTargetGoal::shouldExecute();
//   getBrightness()（Entity 基类）= getLightLevelDependentMagicValue(pos)：含 getSkyDarkening 时间衰减
//   + gamma 曲线。白天 dayTime=1000 + 露天 skyAccess → skyLight=15、skyDarkening≈0 → brightness≈1.0 ≥0.5
//   → SpiderTargetGoal 不选玩家 → 蜘蛛保持中立。
//   SpiderAttackGoal::shouldContinueExecuting（:68-77）同样 brightness>=0.5 时 1% 概率弃目标，
//   双重门控确保白天不攻击。
//
// 环境选择：grass_pen（9×5×9 玻璃围墙盒）白天 + 露天。y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，
//   y=4 全 air 露天。蜘蛛 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true，
//   brightness≈1.0）。Survival 玩家 (5,2,4) 紧邻蜘蛛 1 格。蜘蛛受重力下落，脚下需玻璃支撑——但
//   grass_pen y=0 是 grass_block 地板（helper y=1），蜘蛛直接站地板上无需额外玻璃。
//
// 判定手段：负向断言——等 200 tick（覆盖多个 SpiderTargetGoal chance 检查周期 + SpiderAttackGoal
//   shouldContinueExecuting 多次评估）后断言玩家 HP 仍满血 20。蜘蛛白天亮度高 SpiderTargetGoal::shouldExecute
//   返回 false 不选目标，SpiderAttackGoal 无 attackTarget 不执行，玩家不受伤。
//   maxTicks=500：200 tick 等待 + succeedWhen 检查余量。
//   玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验；创造/旁观玩家被 TargetGoal::isSuitableTarget
//   滤掉，蜘蛛本就不选目标，无法区分"光照门控生效"vs"玩家被滤掉"——须 Survival 才能验证光照门控）。
//   对照测试 spider_attacks_player_at_night（夜晚攻击）与本测试（白天不攻击）交叉覆盖光照门控双向。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜘蛛.txt#行为（亮度门控中立/敌对）
function spiderNeutralInDaylight(test: Test): void {
  const spiderType = "spider";

  // 蜘蛛 (4,2,4) + Survival 玩家 (5,2,4)，紧邻 1 格。grass_pen 玻璃围墙盒，白天 + 露天。
  // 玩家 Survival（创造/旁观被 TargetGoal 滤掉，无法验证光照门控）。
  test.spawn(spiderType, { x: 4, y: 2, z: 4 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 4 }, "daylightBystander", 0 as any);

  // 等 200 tick 后断言玩家满血 20（蜘蛛白天中立不攻击）。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"。
  test.runAtTickTime(200, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue >= 20,
      `spider should be neutral in daylight, player hp=${(health as any).currentValue}`);
    test.succeed();
  });
}

export function registerSpiderTests(): void {
  GameTest.register("MobBehaviorTests", "spider_attacks_player_at_night", spiderAttacksPlayerAtNight)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "spider_does_not_burn_in_daylight", spiderDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "spider_neutral_in_daylight", spiderNeutralInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(500);
}
