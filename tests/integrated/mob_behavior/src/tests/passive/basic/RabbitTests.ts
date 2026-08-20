// 兔子行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// open_grass_hall 结构尺寸（41×7×9），helper 相对坐标。四壁 glass 墙 + 内部/顶部全 air 露天草地长廊，
// y=0 grass_block 地板。兔子繁殖用：41 格长度容纳玩家 teleport 远离兔子（>8 格避 AvoidEntityGoal），
// 玻璃墙在边界阻止兔子跑出查询区域。
const HALL_FROM = { x: 0, y: 0, z: 0 };
const HALL_VOLUME = { x: 41, y: 7, z: 9 };

// 兔子逃离玩家（wiki tech_兔子.txt#行为：除杀手兔外，所有兔子都会躲避 8 格内的生存或冒险模式玩家）。
//
// C++ 链路：RabbitEntity::registerGoals 优先级2 注册 AvoidEntityGoal(this, 8.0, 2.2, 2.2, predicate)
// （RabbitEntity.cpp:583-593），谓词过滤 Player（杀手兔 isKillerRabbit() 短路 return false 不逃离）。
// AvoidEntityGoal 无视线检查（仅按距离+谓词球状搜索，AvoidEntityGoal.cpp:141-142），故 grass_pen
// 玻璃墙不影响兔子检测玩家。shouldExecute 找到 8 格内玩家 → _findEscapePosition 朝远离方向寻路 →
// 兔子位移拉开与玩家距离。
//
// 环境选择：grass_pen（9×5×9，玻璃墙围栏）。关键：不用 creeper_pit（开放坑无墙）——兔子 AvoidEntityGoal
// 朝远离玩家方向逃离，开放坑无墙约束兔子可能游荡出查询区致 getEntities 查不到被误判。grass_pen 玻璃墙
// 把兔子限制在内部空气腔，逃离位移在查询区内可观测。AvoidEntityGoal 不需视线，玻璃墙不影响检测。
//
// 判定手段：兔子从玩家旁(4,2,4)逃离，断言兔子距玩家水平距离 > 4 格（初始 0 格同位 → 逃离后 >4 格）。
// 用 getEntities 区域限定取兔子世界坐标，与玩家世界坐标算水平距离。逃离方向随机（朝远离玩家任意方向），
// 但只要距玩家 >4 格即证明 AvoidEntityGoal 驱动了位移。grass_pen 9×9 对角 ~11 格，兔子逃离到 >4 格
// 容易满足。判定距离阈值（而非固定区域）规避逃离方向随机导致的 flaky。
// 玩家用 Survival（gameMode=0）：wiki 语义"生存/冒险模式玩家"触发逃离，Survival 符合。
// 时序：AvoidEntityGoal tickRate 评估（每 2 tick）+ 寻路 + 逃离位移。2.2 速度拉开 4 格约需 20-40 tick，
// maxTicks=1000 留充裕余量吸收非确定性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_兔子.txt#行为（躲避8格内玩家）
function rabbitFleesPlayer(test: Test): void {
  const rabbitType = "rabbit";

  // 兔子与 Survival 玩家同位于中心 (4,2,4)：兔子 spawn 即在玩家旁（距 0），AvoidEntityGoal 首次
  // 扫描即可检测到 8 格内玩家并逃离。玩家不动（SimulatedPlayer 默认静止），兔子朝远离方向位移。
  // grass_pen 中心 (4,2,4) 为空气腔，helper-y=2→结构内 y=1 空气，脚踩 y=0 grass_block。
  test.spawn(rabbitType, { x: 4, y: 2, z: 4 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "scary", 0 as any);

  // 玩家世界坐标（helper (4,2,4) 经 worldLocation 转换）。
  const playerWorld = test.worldLocation({ x: 4, y: 2, z: 4 });

  // 断言兔子逃离：距玩家水平距离 > 4 格（初始 0 格，逃离后应 >4 格）。
  // 区域限定用 PEN（grass_pen 9×5×9）排除并行测试污染。
  // 逃离速度 2.2，拉开 4 格约需 20-40 tick，maxTicks=1000 留充裕余量吸收非确定性。
  test.succeedWhen(() => {
    const rabbits = test.getDimension().getEntities({
      type: rabbitType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(rabbits.length > 0, "rabbit disappeared");
    const r = rabbits[0];
    const dx = r.location.x - playerWorld.x;
    const dz = r.location.z - playerWorld.z;
    test.assert(dx * dx + dz * dz > 4 * 4,
      `rabbit did not flee from player, distSq=${dx * dx + dz * dz}`);
  });
}

// 杀手兔主动攻击玩家（wiki tech_兔子.txt#杀手兔：非和平难度下迅速跳向 16 格半径内的玩家，
// 近战攻击 8 伤害；杀手兔是 Java 独有变种，{RabbitType:99} 命令生成）。
//
// C++ 链路：applySpawnEvent 派发 "rabbit<spawn_killer>" → setRabbitType(Killer) → applyRabbitType
// （RabbitEntity.cpp:147-176，注册：
//   targetSelector 优先级2 NearestAttackableTargetGoal<Player>(this, true)（checkSight=true 选玩家）→
//   goalSelector 优先级4 MeleeAttackGoal(this, 1.4, true) 寻路接近 →
//   attackEntityAsMob(玩家, ATTACK_DAMAGE)。ATTACK_DAMAGE 基础值 + EVIL_ATTACK_POWER_MODIFIER(+5)，
//   杀手兔近战伤害 8（对齐 wiki {{AutoDmg|8}}）。护甲 ARMOR=8。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙。关键：NearestAttackableTargetGoal checkSight=true，
// grass_pen 外圈玻璃墙会挡视线致 attackTarget 恒 null（同 CreeperTests 苦力怕测试注释），故用 creeper_pit
// 开放坑保证视线通畅。杀手兔(3,2,3)+Survival玩家(4,2,3)，水平距 1 格，NearestAttackableTargetGoal
// 首次扫描即锁定玩家。脚下放玻璃支撑（creeper_pit y=0 草地本可支撑，玻璃确保实体站立稳定）。
// 难度：GameTestServer 默认 Difficulty::Normal（GameTestServer.hpp:45），非和平，杀手兔会攻击（对齐
// wiki"非和平难度下"语义）。
//
// 判定手段：断言玩家 HP 下降（<20）。杀手兔近战攻击 8 伤害，玩家满血 20 → 12。1 次命中即 <20。
// 玩家用 Survival（gameMode=0）：杀手兔 NearestAttackableTargetGoal<Player> 选 Survival 玩家
// （创造/旁观被 TargetGoal 滤掉不可被攻击）。
// 时序：NearestAttackableTargetGoal 选玩家(chance=0 每 tick) + MeleeAttackGoal 接近 1 格 + 攻击冷却
// （MeleeAttackGoal ATTACK_COOLDOWN_TICKS=20）+ hurt(8.0)。杀手兔 1.4 速度接近 1 格约需 10-20 tick，
// maxTicks=800 留充裕余量吸收非确定性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_兔子.txt#杀手兔（攻击玩家8伤害）
function killerRabbitAttacksPlayer(test: Test): void {
  const killerRabbitType = "rabbit<spawn_killer>";

  // 杀手兔 (3,2,3)、Survival 玩家 (4,2,3)，水平距 1 格，同处结构 y=2 层。
  // 脚下放玻璃支撑。creeper_pit 开放坑无墙，checkSight 视线通畅。
  // "rabbit<spawn_killer>" 后缀触发 applySpawnEvent 派发 setRabbitType(Killer)（GameTestHelper.cpp）。
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  test.spawn(killerRabbitType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "victim", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20（被杀手兔近战命中）。
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
      `killer rabbit did not attack player, hp=${(health as any).currentValue}`);
  });
}

// TODO: rabbit_raids_carrot_crop（兔子啃食成熟胡萝卜作物，RaidGardenGoal）暂未实现。
// 判定受阻原因：RaidGardenGoal 啃食成熟胡萝卜(age=7)使 age 减 1 变 age=6，但方块 typeId 仍为
// carrots（仅 age=0 啃食才变 air，而 shouldMoveTo 只选 maxAge=7 胡萝卜，啃后非 maxAge 不再被选，
// 故单格胡萝卜最多啃一次 7→6，不会变 air）。当前 JS 侧 BlockPermutation 仅有 type/isValid 属性
// （MinecraftModuleFactory.cpp:1030-1047），无 getState(key) 读 age；assertBlockState/getBlock 的 JS
// 绑定是 stub（ScriptTestHelper.cpp:1462/1478）。故无法读胡萝卜 age 判定啃食发生。
// 待补全 BlockPermutation.getState / getBlock JS 绑定（暴露 BlockState property 读取）后可实现：
//   铺 farmland + carrots(age=7)，spawn 兔子（wantsMoreFood 默认 true），断言任一胡萝卜 age<7。

// 两只兔子各喂胡萝卜后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小兔子
// （wiki tech_兔子.txt#繁殖：手持胡萝卜/金胡萝卜/蒲公英右键两只成年兔子使其进入"爱心模式"，
//   两只兔子靠近后繁殖出小兔子，双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球）。
//
// 关键设计难点——AvoidEntityGoal 干扰：RabbitEntity::registerGoals（RabbitEntity.cpp:568-619）
//   AvoidEntityGoal(玩家，优先级2，检测距离8格) 优先级高于 BreedGoal(优先级3)。玩家在兔子 8 格内会
//   持续触发 Avoid，BreedGoal 被 Move flag mutex 阻塞无法繁殖；且 Avoid 让两头兔子跑散，BreedGoal::
//   findNearbyMate（8 格内）找不到配偶致繁殖失败（实测约 10-20% 概率失败）。
//
// 解决方案——远程喂食规避 Avoid：interactWithEntity 转发 Player::interactOn（Player.cpp:2843），
//   interactOn 无距离门控直接调 target.processInitialInteract，故玩家可在远处喂兔子。玩家全程站在
//   距兔子 18 格外（>8），AvoidEntityGoal shouldExecute 的 _findEntityToAvoid 找不到 8 格内玩家恒返
//   false，BreedGoal(3) 是最高可执行 goal，isInLove 时独占驱动繁殖。两头兔子不被 Avoid 惊扰跑散，
//   始终在 BreedGoal 检测范围内稳定繁殖。
//
// C++ 链路（对齐 MC Java 1.21.11 Rabbit + BreedGoal）：
//   1) 玩家主手持胡萝卜 + interactWithEntity(rabbit)（远程，玩家在 18 格外）→ Player::interactOn
//      → rabbit.processInitialInteract → MobEntity::interactMob → AnimalEntity::interactMob override
//      （AnimalEntity.cpp:90-141）：RabbitEntity::isBreedingItem(胡萝卜) 命中（RabbitEntity.cpp:234-242
//      item==Items::CARROT||GOLDEN_CARROT）→ 成体 canBreed() → setInLove(player.playerId())。
//      创造模式喂食不消耗胡萝卜，同一根喂两只兔子。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) RabbitEntity::spawnBaby（RabbitEntity.cpp:255-...）：构造 RabbitEntity 幼体 + setChild(true)；
//      BreedGoal:153 兜底 setTypeId(RABBIT) 保证 getEntities 可查。
//
// 环境选择：open_grass_hall（41×7×9 露天草地长廊，四壁玻璃墙）。两头兔子放中心 (20,2,4)+(20,2,6)
//   相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。玩家站远端 (2,2,4) 距兔子 ~18 格 >8，
//   AvoidEntityGoal 不触发。兔子 MOVEMENT_SPEED=0.3，BreedGoal speed=1.0 倍率，moveTo 配偶快。
//
// 判定手段：繁殖完成后区域内 rabbit 数 >=3（原 2 + 幼体 1）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30，maxTick=700
//   （与 cow/sheep 等同范式，兔子不被 Avoid 干扰时繁殖时序与牛一致）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_兔子.txt#繁殖（喂胡萝卜→爱心→繁殖小兔子+冷却+经验球）
function rabbitBreedsWhenFedCarrot(test: Test): void {
  const rabbitType = "rabbit";

  // 两只成年兔子放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  // open_grass_hall helper-y=2 是 air 层，y=0 grass_block 地板，兔子 spawn y=2 落到 y=1 草地顶。
  const rabbit1 = test.spawn(rabbitType, { x: 20, y: 2, z: 4 });
  const rabbit2 = test.spawn(rabbitType, { x: 20, y: 2, z: 6 });

  // 创造玩家站远端 (2,2,4)，距兔子 ~18 格 >8 格避免 AvoidEntityGoal 触发。
  // interactWithEntity 远程生效（interactOn 无距离门控），玩家无需靠近兔子喂食。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "rabbitBreeder");
  const carrot = new ItemStack("minecraft:carrot", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(carrot as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只兔子：interactWithEntity 远程转发 interactOn → AnimalEntity::interactMob → setInLove。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(rabbit1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(rabbit2);
  });

  // 轮询：繁殖完成后区域内 rabbit 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const rabbits = test.getDimension().getEntities({
      type: rabbitType,
      location: test.worldLocation(HALL_FROM),
      volume: HALL_VOLUME,
    });
    return rabbits.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const rabbits = test.getDimension().getEntities({
        type: rabbitType,
        location: test.worldLocation(HALL_FROM),
        volume: HALL_VOLUME,
      });
      test.assert(false,
        `rabbit did not breed: rabbitCount=${rabbits.length} (expected >=3 after feeding carrot)`);
    },
  });
}

export function registerRabbitTests(): void {
  GameTest.register("MobBehaviorTests", "rabbit_flees_player", rabbitFleesPlayer)
    .structureName("gametests:grass_pen")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "killer_rabbit_attacks_player", killerRabbitAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "rabbit_breeds_when_fed_carrot", rabbitBreedsWhenFedCarrot)
    .structureName("gametests:open_grass_hall")
    .maxTicks(1000);
}
