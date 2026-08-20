// 海龟行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。海龟繁殖测试用。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 海龟跟随手持海草的玩家（wiki tech_海龟.txt#繁殖：海龟可用海草繁殖，故海草是其诱惑/繁殖物品）。
//
// C++ 链路：TurtleEntity : AnimalEntity（非 WaterMobEntity，陆地不溺水），registerGoals
// （TurtleEntity.cpp:248-285）优先级2 注册 TurtleTemptGoal(this, 1.1)。TurtleTemptGoal
// （TurtleGoals.cpp:655-663）是 TemptGoal 的薄包装，构造转发 TemptGoal(turtle, 1.1, isSeagrass, false)
// （scaredByMovement=false 硬编码）。isSeagrass 判定 item==Items::SEAGRASS（minecraft:seagrass）。
// TemptGoal::findTemptingPlayer 用 getEntitiesInRange(pos, TEMPT_RANGE=10, ...) 球体搜索 10 格内
// 主手或副手持海草的玩家（含 SimulatedPlayer），命中后调 nav->moveTo(player, 1.1) 驱动海龟走向玩家。
// 玩家静止持海草即可触发，无需玩家移动（与 cow_follows_wheat 同范式）。
//
// 陆地可行性：海龟继承 AnimalEntity 非水生，陆地不窒息。TurtleGoToWaterGoal(优先级3) 无水时
// _findWater 返 false 不干扰；TurtleTravelGoal(7) 要求 isInWater 陆地不触发；TurtleWanderGoal(9)
// 优先级低于 TemptGoal(2) 被 Move flag mutex 阻；TurtleGoHomeGoal(4) 需 hasHomePos() spawn 的
// 海龟无 homePos 不触发。故陆地 mediumglass 中 TurtleTemptGoal 独占 Move flag，海龟被诱惑移动。
// 海龟陆地速度 = max(baseSpeed*0.5, 0.06) = 0.125（TurtleEntity.cpp:415），慢于牛(0.2)，maxTicks 加大。
//
// 环境选择：mediumglass（12×9×11 陆地走廊）。玩家 (2,2,5) ↔ 海龟 (10,2,5)，水平距 8 格 < temptRange 10。
// 结构放置有 +1 抬升：helper-y=2 对应结构内 y=1 空气腔，地板 helper-y=1（结构内 y=0 圆石）。
// 海龟 setStepHeight(1.0) 可上 1 格台阶，走廊无障碍寻路通畅。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海龟.txt#繁殖（海草诱惑/繁殖）
function turtleTemptedBySeagrass(test: Test): void {
  const turtleType = "turtle";

  // 玩家 (2,2,5) 主手持海草。gameMode 省略走 Cubium 默认创造模式（创造不影响 TemptGoal 手持物品检测）。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持海草：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回海草，TemptGoal 才能识别诱惑源。
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  const seagrass = new ItemStack("minecraft:seagrass", 1);
  farmer.setItem(seagrass as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 海龟 spawn 在走廊远端 (10,2,5)，距玩家 8 格 < temptRange(10)，在 TurtleTemptGoal 检测范围内。
  test.spawn(turtleType, { x: 10, y: 2, z: 5 });

  // 海龟被诱惑后从 x=10 朝玩家 x=2 方向移动。断言海龟出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:3,z:6)，覆盖玩家附近 5×2×3 区域（同 cow_follows_wheat）。
  // 海龟陆地速度 0.125（慢于牛 0.2），8 格走廊需更长时间，maxTicks=2000 留充裕余量。
  test.succeedWhen(() => {
    assertEntityInVolume(test, turtleType, 2, 2, 4, 6, 3, 6);
  });
}

// 两头海龟喂海草后繁殖出小海龟（wiki tech_海龟.txt#繁殖：手持海草右键两头成年海龟使其进入
// "求爱模式"，两头海龟靠近后繁殖出小海龟，小海龟会记住出生地，长大后回出生地产卵）。
//
// 这是 TurtleEntity::registerGoals 漏注册基础 AI 缺陷的回归测试（与 WolfEntity/PandaEntity 同款，
// 见 [[wolf-registergoals-missing-breedgoal-fix]]/[[panda-registergoals-missing-breedgoal-fix]]）。
// 修复前 TurtleEntity 旧注释错误声称"由 AnimalEntity::registerGoals() 注册 SwimGoal/
// FollowParentGoal 等基础动物 AI"（空操作），实际缺 SwimGoal/FollowParentGoal/LookRandomlyGoal。
// 但 TurtleEntity 已注册 TurtleMateGoal(优先级1，继承 BreedGoal) 驱动繁殖，故繁殖链路本身未断；
// 本测试验证 TurtleMateGoal 繁殖链路完整 + 修复后基础 goal 补全无副作用。
//
// C++ 链路（对齐 MC Java 1.21.11 Turtle + TurtleMateGoal/BreedGoal）：
//   1) 玩家主手持海草 + interactWithEntity(turtle) → Player::interactOn → turtle.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      isBreedingItem(海草) 命中 → 成体 canBreed()（TurtleEntity.cpp:146-150 额外查 !hasEgg()）
//      → setInLove(player.playerId())。TurtleEntity::isBreedingItem：item==Items::SEAGRASS。
//      创造模式喂食不消耗海草（同一根海草喂两头海龟）。海龟非 Tameable，无需驯服。
//   2) TurtleMateGoal::shouldExecute（TurtleGoals.cpp:627-634）：!hasEgg() && BreedGoal::shouldExecute()
//      （isInLove() && findNearbyMate() 非空）。
//   3) BreedGoal::tick（TurtleMateGoal 继承）：navigator.moveTo(配偶) + m_spawnBabyDelay++，
//      达 adjustedTickDelay(SPAWN_BABY_DELAY=60)=30 且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) TurtleEntity::spawnBaby（TurtleEntity.cpp:152-176）：构造 TurtleEntity 幼体 + setTypeId(TURTLE)
//      + setChild(true) + 继承 homePos + setPosition。子代 typeId=TURTLE 可被 getEntities 查到。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两头海龟放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9 已在繁殖距离内），spawnBaby 几乎只需等 30 tick spawnBabyDelay。
//   陆地无水，TurtleGoToWaterGoal(3)/TurtleTravelGoal(7) 不触发；TurtleWanderGoal(9) 优先级低于
//   TurtleMateGoal(1) 被 mutex 阻；TurtleGoHomeGoal(4) 需 hasHomePos() spawn 海龟无 homePos 不触发。
//   故陆地 grass_pen 中 TurtleMateGoal 独占高优先级驱动繁殖。
//
// 判定手段：繁殖完成后区域内 turtle 数 >=3（原 2 头成年 + 1 头幼体）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ TurtleMateGoal 评估 + 30 tick spawnBabyDelay + 余量。海龟陆地速度
//   0.125 较慢，但两头相距仅 2 格无需长距离靠近，spawnBabyDelay 30 tick 内触发。startTick=30 留
//   喂食+选配偶时间，maxTick=1000 留充足余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海龟.txt#繁殖（喂海草→求爱→繁殖小海龟）
function turtleBreedsWhenFedSeagrass(test: Test): void {
  const turtleType = "turtle";

  // 两头成年海龟放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  // 脚下 y=1 grass_block 支撑防下落（grass_pen y=0 grass_block 地板，y=1 air 腔，helper y=2 = 结构 y=1 air）。
  const turtle1 = test.spawn(turtleType, { x: 4, y: 2, z: 4 });
  const turtle2 = test.spawn(turtleType, { x: 4, y: 2, z: 6 });

  // 创造玩家持海草：创造模式喂食不消耗海草（同一根海草喂两头海龟）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "turtleBreeder");
  const seagrass = new ItemStack("minecraft:seagrass", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(seagrass as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两头海龟：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  // 间隔 5 tick 确保第一头海龟 setInLove 写入后再喂第二头。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(turtle1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(turtle2);
  });

  // 轮询：繁殖完成后区域内 turtle 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const turtles = test.getDimension().getEntities({
      type: turtleType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return turtles.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 1000,
    onTimeout: () => {
      const turtles = test.getDimension().getEntities({
        type: turtleType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `turtle did not breed: turtleCount=${turtles.length} (expected >=3 after feeding seagrass)`);
    },
  });
}

export function registerTurtleTests(): void {
  GameTest.register("MobBehaviorTests", "turtle_tempted_by_seagrass", turtleTemptedBySeagrass)
    .structureName("gametests:mediumglass")
    .maxTicks(2000);

  GameTest.register("MobBehaviorTests", "turtle_breeds_when_fed_seagrass", turtleBreedsWhenFedSeagrass)
    .structureName("gametests:grass_pen")
    .maxTicks(1000);
}
