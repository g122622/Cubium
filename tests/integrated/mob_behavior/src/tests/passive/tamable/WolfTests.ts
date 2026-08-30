// 狼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed, waitForCondition } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。狼繁殖测试用。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 狼受击后反击玩家（wiki tech_狼.txt#攻击:205：野生的狼会对攻击它的生物产生敌意）。
//
// C++ 链路：WolfEntity : TameableEntity（友好/中立），registerGoals（WolfEntity.cpp:894-1009）：
//   targetSelector 优先级3：HurtByTargetGoal(this, true)（alertAllies=true，受击后设
//     attackTarget=攻击者并呼叫附近同类，WolfEntity.cpp:957）。
//   goalSelector 优先级5：MeleeAttackGoal(this, 1.0, true)（speed=1.0，longMemory=true，
//     WolfEntity.cpp:932），shouldExecute 读 attackTarget，接近到攻击距离内
//     attackEntityAsMob→hurt(玩家, ATTACK_DAMAGE)。
//   注意：狼未注册 NearestAttackableTargetGoal<Player>（优先级4被注释，WolfEntity.cpp:959-963），
//   故狼不会主动攻击玩家，仅在受击后才反击——这正是本测试要验证的"受击后敌对攻击玩家"。
// registerAttributes（WolfEntity.cpp:1011-1022）：MAX_HEALTH=8.0, MOVEMENT_SPEED=0.3,
//   ATTACK_DAMAGE=2.0（野生值；C++ 与 wiki 野生4 不符，属已知偏差，但本测试断言"行为发生"
//   即玩家掉血，不依赖精确伤害数值对齐）。
//
// 环境选择：grass_pen（9×5×9 玻璃围墙盒）。狼被玩家近战击退（-x 方向）后若无围墙会弹出
// 结构边界持续下落出世界，AI 虽在追击玩家但物理上永远追不到（实测 creeper_pit 下狼飞出
// x=-1.3 后 y 一路降到 -800+），succeedWhen 永不满足 → 超时失败。grass_pen 的玻璃围墙把
// 击退运动限制在结构内，狼留在玩家近战范围内完成反击。
// 狼(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。grass_pen y=0 是 grass_block 地板（helper y=1），
// y=1..3 是玻璃围墙 air 腔（helper y=2..4），狼/玩家直接站地板上无需玻璃支撑。
// 玩家 tick 8 后 attackEntity(狼) 触发 HurtByTargetGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 ZombifiedPiglinTests/PolarBearTests 同款注释）。
// 狼被攻击后设 attackTarget=玩家，MeleeAttackGoal 寻路接近 3 格 + 攻击冷却后 hurt(玩家, 2.0)。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 2.0，玩家满血 20 → 18。
// 狼 MOVEMENT_SPEED=0.3 较快，3 格接近 + 攻击冷却（MeleeAttackGoal ATTACK_COOLDOWN_TICKS=20，
// 对齐 vanilla 20 tick）约需 30-50 tick。maxTicks=800 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狼.txt#攻击（受击后敌对攻击玩家）
function wolfRetaliatesWhenAttacked(test: Test): void {
  const wolfType = "wolf";

  // 狼 (2,2,3) + Survival 玩家 (5,2,3)，水平距 3 格，同处 grass_pen air 腔 y=2 层。
  // grass_pen 自带玻璃围墙（helper y=2..4 三层玻璃，结构 y=1..3），把狼被玩家击退（-x 方向）
  // 后的运动限制在结构内——此前用 creeper_pit（无围墙开放坑），狼被击退弹出结构边界后
  // 持续下落出世界，AI 虽追击玩家但物理上永远追不到，测试超时失败。
  const wolf = test.spawn(wolfType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击狼：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 HurtByTargetGoal → 设 attackTarget=玩家。
  test.runAtTickTime(8, () => {
    player.attackEntity(wolf);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(2.0)。
  // 狼 0.3 速度接近 3 格约需 30-50 tick，maxTicks=800 留充裕余量。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `wolf did not retaliate, hp=${(health as any).currentValue}`);
  });
}

// 未驯服狼主动攻击绵羊（wiki tech_狼.txt#攻击:197：野生的狼会主动攻击...绵羊。
// 历史 19w07a：狼现在会攻击狐狸；wiki#攻击:199 绵羊在未受到攻击时会忽视狼，但被狼攻击后
// 仍然会四处逃窜——故羊首击前静止，狼必中首击）。
//
// C++ 链路：WolfEntity registerGoals targetSelector 优先级5：
//   NearestAttackableTargetGoal<LivingEntity>(this, true, 0, lambda)（checkSight=true，chance=0 每 tick，
//   WolfEntity.cpp:967-977）匹配 SHEEP/RABBIT/FOX 类型。选定后 MeleeAttackGoal(优先级5) 寻路接近，
//   attackEntityAsMob→hurt(羊, 2.0)。羊满血 8，被击即 <8。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。狼(2,2,3)+羊(4,2,3)，水平距 2 格（近距，避免羊首击后
// 逃跑追不上——狼 0.3 速度 = 羊 0.25 速度，狼略快可追）。脚下放玻璃支撑。
// 选羊而非狐狸/兔作为目标：①羊未受击时忽视狼不逃跑（wiki#攻击:199），首击前静止，狼必中；
//   ②羊 8 血较耐打，被多击也不会过快消失；③羊无对狼的逃跑 AI 专精。
// 狐狸有逃跑 AI、兔 3 血易 2 击致死消失，稳定性均不如羊。
//
// 判定手段：断言羊 HP 下降（<8）或羊已死亡消失。近战确定性命中，伤害 2.0。
// 羊满血 8，被击即 <8。羊可能被多击（8 血 / 2 伤害 = 4 击致死，每击 20 tick 冷却 → 约 80+ tick）
// 致死消失，length==0 也算通过（已被狼攻击死亡）。
// 时序：NearestAttackableTargetGoal 选羊(chance=0 每 tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(2.0)。
// 狼 0.3 速度接近 2 格约需 20-30 tick，maxTicks=1000 留充裕余量吸收羊逃跑 + 非确定性。
// 羊查询用区域限定排除并行测试污染；type 用 "minecraft:sheep"。
// 注意：狼 targetSelector 优先级5 选羊无 Peaceful 门控（TameableEntity 非怪物），GameTest 默认难度不影响。
// 狼不会主动攻击玩家（优先级4 NearestAttackableTargetGoal<Player> 被注释），故此测试不需玩家。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狼.txt#攻击（主动攻击绵羊等被动生物）
function wolfAttacksSheep(test: Test): void {
  const wolfType = "wolf";
  const sheepType = "sheep";

  // 狼 (2,2,3)、羊 (4,2,3)，水平距 2 格，同处结构 y=2 层。
  // 近距 2 格确保狼选目标后快速命中（羊首击前静止不逃，首击后逃跑但狼 0.3>羊 0.25 可追）。
  // 狼脚下 (2,1,3) 放玻璃支撑；羊脚下 (4,1,3) 放玻璃。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  test.spawn(wolfType, { x: 2, y: 2, z: 3 });
  test.spawn(sheepType, { x: 4, y: 2, z: 3 });

  // 断言羊掉血或死亡：succeedWhen 每 tick 持续检查羊 HP<8 或已消失。
  // 时序：NearestAttackableTargetGoal 选羊(每 tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(2.0)。
  // 羊可能被多击杀死消失，length==0 也算通过（已受攻击死亡）。
  test.succeedWhen(() => {
    const sheeps = test.getDimension().getEntities({
      type: "minecraft:sheep",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 羊已死亡消失（被狼打死）——攻击行为生效。
    if (sheeps.length === 0) {
      return;
    }
    const health = sheeps[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "sheep has no health component");
    test.assert((health as any).currentValue < 8,
      `wolf did not attack sheep, hp=${(health as any).currentValue}`);
  });
}

// 两头狼驯服后喂肉类繁殖出幼狼（wiki tech_狼.txt#繁殖:318：对两只生命值已满的**驯服**的成年狼
// 使用任意食物可使其进入"求爱模式"，产生爱心粒子，两只狼都站立时才能繁殖出一只幼年狼）。
//
// 这是 WolfEntity::registerGoals 漏注册 BreedGoal 缺陷的回归测试（与 PandaEntity 同款缺陷，见
// [[panda-registergoals-missing-breedgoal-fix]]）。修复前 WolfEntity 误调 TameableEntity::
// registerGoals()（空操作）+ 旧注释错误声称基类注册了 BreedGoal 等基础 goal，实际只注册了
// SitGoal/AvoidLlama/Leap/Melee/FollowOwner/Beg，缺 BreedGoal 致驯服狼喂肉 setInLove 后无 goal
// 驱动繁殖。修复后照搬 CatEntity 范式补全 SwimGoal(0)/PanicGoal(1)/BreedGoal(2)/FollowParentGoal(7)/
// WaterAvoidingRandomWalkingGoal(8)/LookAtGoal(10)/LookRandomlyGoal(11)。
//
// C++ 链路（对齐 MC Java 1.21.11 Wolf + BreedGoal）：
//   1) 驯服：玩家主手持骨头 + interactWithEntity(wolf) → WolfEntity::interactMob 未驯服分支
//      （WolfEntity.cpp:376-395）：isTameItem(骨头) 命中 → _tryToTame(player)（WolfEntity.cpp:401-426）：
//      rng.nextInt(3)==0 即 1/3 概率驯服成功 → setTamed(true) → onTamed(true) 把 MAX_HEALTH 8→20
//      + setHealth(20)（WolfEntity.cpp:1080-1088）→ setSitting(true) 默认坐下。创造模式喂骨头不消耗
//      （同一根骨头可反复喂直到驯服）。
//   2) 驯服检测：getComponent("minecraft:health").effectiveMax 走 LivingEntity::maxHealth()（属性系统
//      真相源，MinecraftModuleFactory.cpp:1054）。野生 effectiveMax=8，驯服后=20。用 effectiveMax>=20
//      判定驯服成功（不依赖 currentValue，因狼可能受伤降当前血）。
//   3) 繁殖：玩家主手持熟牛肉 + interactWithEntity(wolf) → WolfEntity::interactMob 已驯服分支
//      （WolfEntity.cpp:220-360）：跳过喂食治疗（满血）/狼铠/染色 → 优先级6 isBreedingItem(熟牛肉)
//      命中 + canBreed() → setInLove(player.playerId())（WolfEntity.cpp:322-359）。
//   4) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//      BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 30 tick 且 distSq<9 时 spawnBaby。
//   5) WolfEntity::spawnBaby 生成幼狼 + setTypeId(WOLF) 兜底保证 getEntities 可查。
//
// 坐姿说明：wiki 行 318"两只狼都站立时才能繁殖"是 vanilla 行为。Cubium 的 SitGoal 用 GoalFlag::Target
// 互斥标志（TameableGoals.cpp:221），BreedGoal 用 Move+Look 标志（BreedGoal.cpp:59），两者 mutex 不
// 冲突可共存；SitGoal 无 tick 不会持续 clearNavigation 抵消 BreedGoal 的 moveTo。故 Cubium 中坐着的
// 驯服狼喂肉后 BreedGoal 仍能驱动移动繁殖（与 vanilla 偏差，本次不修，留 TODO）。测试不刻意站起。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两头狼放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9 已在繁殖距离内），spawnBaby 几乎只需等 30 tick spawnBabyDelay。
//   玩家站 (2,2,4) 持骨头/肉。
//
// 非确定性来源：①驯服 1/3 概率——用反复喂骨头（每 3 tick 交替喂两头，约 20 次/头）将两头都未驯服
//   概率压到 (2/3)^20≈0.00025^2≈6e-8，可忽略；②BreedGoal 时序——pollUntilSucceed 轮询吸收。
//
// 时序编排（多阶段）：
//   阶段1 驯服（tick 5..62，每 3 tick 交替喂两头狼骨头）：玩家持骨头，约 20 次/头。
//   阶段2 检测驯服+喂肉（tick 70 起 waitForCondition 每 4 tick 检测两头 effectiveMax>=20，满足后
//     玩家切熟牛肉，喂两头狼 setInLove）。
//   阶段3 繁殖（pollUntilSucceed startTick=90, interval=10, maxTick=1000 轮询 wolf>=3）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狼.txt#繁殖（驯服狼喂肉→求爱→繁殖幼狼）
function wolfBreedsWhenTamedAndFedMeat(test: Test): void {
  const wolfType = "wolf";

  // 两头成年狼放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  // 脚下 y=1 grass_block 支撑防下落（grass_pen y=0 grass_block 地板，y=1 air 腔，helper y=2 = 结构 y=1 air）。
  const wolf1 = test.spawn(wolfType, { x: 4, y: 2, z: 4 });
  const wolf2 = test.spawn(wolfType, { x: 4, y: 2, z: 6 });

  // 创造玩家持骨头：创造模式喂骨头不消耗（同一根骨头可反复喂直到驯服）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "wolfBreeder");
  const bone = new ItemStack("minecraft:bone", 1);
  player.setItem(bone as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 读取狼 effectiveMax（驯服后 8→20）。狼句柄 owned=false，但狼不被攻击不会死亡，句柄安全。
  const effectiveMax = (wolf: any): number => {
    const h = wolf.getComponent("minecraft:health");
    return h !== undefined ? (h as any).effectiveMax : 0;
  };

  // 阶段1：驯服——tick 5..62 每 3 tick 交替喂两头狼骨头（约 20 次/头，1/3 概率驯服）。
  // 每次喂骨头触发 _tryToTame，创造模式不消耗骨头可连续喂。驯服后再喂骨头走已驯服分支优先级7
  // 切换坐/站（无害，不影响驯服状态）。
  for (let t = 5; t <= 62; t += 3) {
    test.runAtTickTime(t, () => {
      // 交替喂两头狼：偶数 tick 喂 wolf1，奇数 tick 喂 wolf2。
      if ((t - 5) % 6 < 3) {
        (player as any).interactWithEntity(wolf1);
      } else {
        (player as any).interactWithEntity(wolf2);
      }
    });
  }

  // 阶段2：检测两头狼都驯服（effectiveMax>=20）后，玩家切熟牛肉喂两头狼 setInLove。
  // waitForCondition 满足后调 onReady：切物品 + 喂食 + 注册阶段3 繁殖轮询。
  waitForCondition(test,
    () => effectiveMax(wolf1) >= 20 && effectiveMax(wolf2) >= 20,
    () => {
      // 玩家主手切换为熟牛肉（isBreedingItem 命中，满血跳过治疗走优先级6 setInLove）。
      const meat = new ItemStack("minecraft:cooked_beef", 1);
      player.setItem(meat as unknown as Parameters<typeof player.setItem>[0], 0, true);

      // 同步喂两头狼：interactWithEntity 转发 interactOn → 已驯服分支 → setInLove。
      // onReady 在某个 tick（>=70）的 callback 内执行，runAtTickTime(5/10) 用绝对 tick 会因已过期
      // 永不触发，故此处直接同步喂食（interactWithEntity 是同步调用，setInLove 同步写入 m_loveTimer）。
      // 两头狼同 tick setInLove，下一 tick BreedGoal::shouldExecute 评估时双方 isInLove 互为配偶。
      (player as any).interactWithEntity(wolf1);
      (player as any).interactWithEntity(wolf2);

      // 阶段3：轮询繁殖完成（区域内 wolf 数 >=3，原 2 头成年 + 1 头幼体）。
      pollUntilSucceed(test, () => {
        const wolves = test.getDimension().getEntities({
          type: wolfType,
          location: test.worldLocation(PEN_FROM),
          volume: PEN_VOLUME,
        });
        return wolves.length >= 3;
      }, {
        startTick: 20,
        interval: 10,
        maxTick: 250,
        onTimeout: () => {
          const wolves = test.getDimension().getEntities({
            type: wolfType,
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
          });
          test.assert(false,
            `wolf did not breed: wolfCount=${wolves.length} ` +
            `tamed1=${effectiveMax(wolf1) >= 20} tamed2=${effectiveMax(wolf2) >= 20} ` +
            `(expected >=3 after tamed wolves bred)`);
        },
      });
    },
    {
      startTick: 70,
      interval: 4,
      maxTick: 700,
      onTimeout: () => {
        // 驯服阶段超时：两头狼未都在 700 tick 内驯服（1/3 概率下极罕见，或 effectiveMax 读取链路异常）。
        test.assert(false,
          `wolves not both tamed in time: max1=${effectiveMax(wolf1)} max2=${effectiveMax(wolf2)} ` +
          `(expected both >=20 after feeding bones; 1/3 tame chance, ~20 feeds each)`);
      },
    });
}

export function registerWolfTests(): void {
  // 狼被玩家击退（-x 方向）后若处无围墙结构（creeper_pit）会弹出结构边界持续下落，
  // AI 追击玩家但物理上永远追不到 → 超时失败。换用 grass_pen（自带玻璃围墙）困住狼
  // 在玩家近战范围内，确保 HurtByTargetGoal 反击链路可达。
  GameTest.register("MobBehaviorTests", "wolf_retaliates_when_attacked", wolfRetaliatesWhenAttacked)
    .structureName("gametests:grass_pen")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "wolf_attacks_sheep", wolfAttacksSheep)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "wolf_breeds_when_tamed_and_fed_meat", wolfBreedsWhenTamedAndFedMeat)
    .structureName("gametests:grass_pen")
    .maxTicks(1000);
}
