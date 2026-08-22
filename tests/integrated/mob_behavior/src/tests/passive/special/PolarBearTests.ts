// 北极熊行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 北极熊受击后反击玩家（wiki tech_北极熊.txt#行为：成年北极熊会在受到攻击后对玩家主动敌对，
// 并攻击玩家。近战攻击伤害 6）。
//
// C++ 链路：PolarBearEntity : AnimalEntity（友好/中立），registerGoals：
//   targetSelector 优先级1：PolarBearHurtByTargetGoal（继承 HurtByTargetGoal(this, false)，
//     成年熊被攻击时 startExecuting 调 HurtByTargetGoal::startExecuting 设 attackTarget=攻击者）。
//   goalSelector 优先级1：PolarBearMeleeAttackGoal（继承 MeleeAttackGoal(bear, 1.25, true)），
//     shouldExecute 读 attackTarget，接近到攻击距离内 attackEntityAsMob→hurt(玩家, 6.0)。
// registerAttributes：MAX_HEALTH=30, ATTACK_DAMAGE=6.0, MOVEMENT_SPEED=0.25, FOLLOW_RANGE=20。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，MeleeAttackGoal 寻路通畅 + checkSight 射线不被阻挡。
// 北极熊(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。北极熊脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
// 玩家 tick 8 后 attackEntity(北极熊) 触发 HurtByTargetGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 ZombifiedPiglinTests 同款注释）。
// 北极熊被攻击后设 attackTarget=玩家，MeleeAttackGoal 寻路接近 3 格 + 攻击冷却后 hurt(玩家, 6.0)。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 6.0，玩家满血 20 → 14。
// 北极熊 MOVEMENT_SPEED=0.25 较慢，3 格接近 + 攻击需时，maxTicks=800 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_北极熊.txt#行为（受击后敌对攻击玩家）
function polarBearRetaliatesWhenAttacked(test: Test): void {
  const polarBearType = "polar_bear";

  // 北极熊 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // 北极熊脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，北极熊反击寻路通畅。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const bear = test.spawn(polarBearType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击北极熊：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 PolarBearHurtByTargetGoal → 设 attackTarget=玩家。
  test.runAtTickTime(8, () => {
    player.attackEntity(bear);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(6.0)。
  // 北极熊 0.25 速度接近 3 格约需 50+ tick，maxTicks=800 留充裕余量。
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
      `polar bear did not retaliate, hp=${(health as any).currentValue}`);
  });
}

// 北极熊主动攻击狐狸（wiki tech_北极熊.txt#行为：北极熊会主动攻击狐狸。幼年北极熊不会攻击狐狸。
// 历史 19w07a：北极熊现在会攻击狐狸）。
//
// C++ 链路：PolarBearEntity registerGoals targetSelector 优先级4：
//   NearestAttackableTargetGoal<FoxEntity>(this, true, 10, nullptr)——成年北极熊主动选狐狸为目标
//   （checkSight=true，chance=10tick 检查一次）。选定后 PolarBearMeleeAttackGoal 接近攻击，
//   attackEntityAsMob→hurt(狐狸, 6.0)。狐狸满血 10，一击 6 伤害 → 4。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。北极熊(2,2,3)+狐狸(4,2,3)，水平距 2 格（近距，避免
// 狐狸逃跑后北极熊追不上——北极熊 0.25 速度慢于狐狸）。脚下放玻璃支撑。
// 狐狸是被动生物会被北极熊攻击，狐狸有逃跑 AI 但近距 2 格北极熊 FOLLOW_RANGE=20 选目标后
// MeleeAttackGoal 首击应快速命中（狐狸初始满血 10，被击 6 → HP<10）。
//
// 判定手段：断言狐狸 HP 下降（<10）或狐狸已死亡消失。近战确定性命中，伤害 6.0。
// 狐狸满血 10，被击即 <10。狐狸可能逃跑导致北极熊追击慢，maxTicks=1000 留充裕余量。
// 狐狸查询用区域限定排除并行测试污染；type 用 "minecraft:fox"。
// 注意：北极熊 targetSelector 优先级4 选狐狸无 Peaceful 门控（AnimalEntity 非怪物），
// GameTest 默认难度不影响。北极熊不会主动攻击玩家（无幼崽时），故此测试不需玩家。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_北极熊.txt#行为（主动攻击狐狸）
function polarBearAttacksFox(test: Test): void {
  const polarBearType = "polar_bear";
  const foxType = "fox";

  // 北极熊 (2,2,3)、狐狸 (4,2,3)，水平距 2 格，同处结构 y=2 层。
  // 近距 2 格确保北极熊选目标后快速命中（避免狐狸逃跑追不上）。
  // 北极熊脚下 (2,1,3) 放玻璃支撑；狐狸脚下 (4,1,3) 放玻璃。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  test.spawn(polarBearType, { x: 2, y: 2, z: 3 });
  test.spawn(foxType, { x: 4, y: 2, z: 3 });

  // 断言狐狸掉血或死亡：succeedWhen 每 tick 持续检查狐狸 HP<10 或已消失。
  // 时序：NearestAttackableTargetGoal 选狐狸(chance=10tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(6.0)。
  // 北极熊 0.25 速度接近 2 格约需 30+ tick，maxTicks=1000 留充裕余量吸收狐狸逃跑 + 非确定性。
  // 狐狸可能被多击杀死消失，length==0 也算通过（已受攻击死亡）。
  test.succeedWhen(() => {
    const foxes = test.getDimension().getEntities({
      type: "minecraft:fox",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 狐狸已死亡消失（被北极熊打死）——攻击行为生效。
    if (foxes.length === 0) {
      return;
    }
    const health = foxes[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "fox has no health component");
    test.assert((health as any).currentValue < 10,
      `polar bear did not attack fox, hp=${(health as any).currentValue}`);
  });
}

// 北极熊免疫细雪的冰冻伤害（wiki tech_北极熊.txt#行为：不同于绝大多数生物，北极熊免疫细雪的
// 冰冻伤害。历史 1.17 21w13a：现在北极熊免疫细雪的冰冻伤害）。
//
// C++ 链路：EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES() 包含 minecraft:polar_bear
// （EntityTypeTags.cpp:605）。Entity::canFreeze() 返回 !FREEZE_IMMUNE_ENTITY_TYPES().contains(typeId)，
// 故北极熊 canFreeze()=false。LivingEntity::tickFreeze 中 !canFreeze() 则不累积 ticksFrozen、
// 不造成冰冻伤害（tickFreeze line 1138/1148 双重 canFreeze 门控）。PowderSnowBlock::onEntityCollision
// 同样 canFreeze() 门控才递增 ticksFrozen。北极熊浸入细雪不冰冻不掉血。
//
// 环境选择：creeper_pit。铺细雪 y=1,2 两层（实体落入后碰撞箱与细雪重叠触发 onEntityCollision），
// y=0 铺实心底（玻璃）托住实体（细雪 getCollisionShape 返回 empty 无碰撞，实体穿过细雪下落，
// 需实心底托住使碰撞箱停留在细雪层中持续触发 onEntityCollision）。北极熊(2,3,2)+对照猪(4,3,4)
// spawn 于 y=3 下落浸入 y=1,2 细雪层，停在 y=0 玻璃上。
//
// 判定手段：双端断言（同 zombified_piglin_immune_to_fire 模式）——
//   1. 北极熊 HP 保持满血 30（canFreeze=false 免疫细雪冰冻伤害）；
//   2. 对照实体猪 HP<10 或死亡消失（猪不免疫细雪，浸入细雪累积冰冻后掉血）——证明细雪冰冻
//      伤害机制确实生效，排除"细雪伤害未实现"的假性通过。
// 时序：实体下落浸入细雪 + ticksFrozen 累积到完全冰冻（vanilla 140 tick）+ 每 40 tick 1.0 伤害。
// 猪需 140+40=180 tick 首次掉血，maxTicks=600 留充裕余量。
// 实体查询用区域限定排除并行测试污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_北极熊.txt#行为（免疫细雪冰冻伤害）
function polarBearImmuneToPowderSnow(test: Test): void {
  const polarBearType = "polar_bear";
  const pigType = "pig";

  // y=0 铺实心底（玻璃）托住实体；y=1,2 铺细雪两层（实体碰撞箱浸入触发 onEntityCollision）。
  // y=3,4 保持 air 供实体 spawn 下落。细雪 empty 碰撞，实体穿过细雪落至 y=0 玻璃，碰撞箱
  // 覆盖 y=0..1.4（北极熊高1.4）/ y=0..0.7（猪高0.7），触及 y=1 细雪层持续触发冰冻。
  fillBlock(test, "minecraft:glass", 0, 0, 0, 6, 0, 6);
  fillBlock(test, "minecraft:powder_snow", 0, 1, 0, 6, 2, 6);

  // 北极熊 (2,3,2)，对照猪 (4,3,4)，间隔足够不互相干扰，都落入细雪层。
  // 北极熊 HP=30 免疫细雪应保持 30；猪 HP=10 不免疫应掉血(<10)或死亡消失。
  test.spawn(polarBearType, { x: 2, y: 3, z: 2 });
  test.spawn(pigType, { x: 4, y: 3, z: 4 });

  // 断言：北极熊 HP 仍为满血 30（免疫细雪）；猪 HP<10 或已死亡消失（细雪伤害生效）。
  // 两端同时成立才证明"细雪冰冻机制有效 + 北极熊免疫"——任一不成立则测试失败。
  test.succeedWhen(() => {
    const bears = test.getDimension().getEntities({
      type: "minecraft:polar_bear",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    const pigs = test.getDimension().getEntities({
      type: "minecraft:pig",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });

    // 北极熊应存活且满血（免疫细雪冰冻）。
    test.assert(bears.length > 0, "polar bear died in powder snow (should be immune)");
    const bearHealth = bears[0].getComponent("minecraft:health");
    test.assert(bearHealth !== undefined, "polar bear has no health component");
    test.assert((bearHealth as any).currentValue >= 30,
      `polar bear should be immune to powder snow freeze, hp=${(bearHealth as any).currentValue}`);

    // 对照实体猪应受伤（HP<10）或已死亡消失——证明细雪冰冻伤害机制确实生效。
    if (pigs.length > 0) {
      const pigHealth = pigs[0].getComponent("minecraft:health");
      test.assert(pigHealth !== undefined, "pig has no health component");
      test.assert((pigHealth as any).currentValue < 10,
        `pig should take powder snow freeze damage, hp=${(pigHealth as any).currentValue}`);
    }
    // 猪已死亡消失（pigs.length==0）也算通过——已被细雪冰冻致死。
  });
}

// 成年北极熊保护幼崽：附近有幼熊时主动攻击玩家（wiki tech_北极熊.txt#行为：成年北极熊会在
// 幼崽附近时变得具有敌意，主动攻击靠近的玩家。这是北极熊最具辨识度的 vanilla 行为）。
//
// C++ 链路：PolarBearEntity registerGoals targetSelector 优先级2：PolarBearAttackPlayerGoal
//   （PolarBearEntity.cpp:484-514，继承 NearestAttackableTargetGoal<Player>(this, true, 20)）。
//   shouldExecute 顺序：
//     1. m_bear->isChild() 成年熊才执行（幼熊返 false）；
//     2. NearestAttackableTargetGoal<Player>::shouldExecute() 选 20 格内玩家为目标；
//     3. boundingBox().expand(8,4,8) 扫附近是否有幼熊（bear->isChild()）——有幼熊才返 true。
//   选定玩家后 PolarBearMeleeAttackGoal（goalSelector 优先级1，MeleeAttackGoal(bear,1.25,true)）
//   接近 attackEntityAsMob→hurt(玩家, 6.0)。
//   注：targetSelector 优先级3 还有一个带相同幼熊检测谓词的 NearestAttackableTargetGoal<Player>
//   （PolarBearEntity.cpp:366-383），与优先级2 逻辑重复，两者都要求附近有幼熊才攻击玩家。
//
// 环境选择：creeper_pit（7×5×7 开放坑无围墙），MeleeAttackGoal 寻路通畅。
// 布局：成年熊(3,2,3)中心，幼熊(2,2,3)相邻 1 格（<8 格检测范围），Survival 玩家(5,2,3)距成年熊
//   2 格（<20 格选目标范围）。成年熊检测到 8 格内有幼熊 → 选玩家 → MeleeAttack 接近 2 格攻击。
//   脚下 y=1 放玻璃支撑三方。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中，伤害 6.0，玩家满血 20 → 14。
// 时序：PolarBearAttackPlayerGoal shouldExecute（chance 检查）+ MeleeAttackGoal 寻路接近 2 格 +
//   攻击冷却 + hurt(6.0)。北极熊 0.25 速度接近 2 格约需 30+ tick，maxTicks=1000 留充裕余量。
//   玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验；创造模式被 TargetGoal 滤掉不可被攻击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_北极熊.txt#行为（保护幼崽敌对玩家）
// Ref: PolarBearEntity.cpp:484-514（PolarBearAttackPlayerGoal shouldExecute 幼熊检测分支）
function polarBearProtectsCubAttacksPlayer(test: Test): void {
  const polarBearType = "polar_bear";

  // 成年熊(3,2,3)、幼熊(2,2,3)、Survival 玩家(5,2,3)。脚下 y=1 玻璃支撑三方。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  // 成年熊（默认成年）。
  test.spawn(polarBearType, { x: 3, y: 2, z: 3 });
  // 幼熊：spawn_baby 事件经 applySpawnEvent → setChild(true) 生成幼年北极熊。
  test.spawn(`${polarBearType}<minecraft:spawn_baby>`, { x: 2, y: 2, z: 3 });
  // Survival 玩家（0 as any 绕过 TS 枚举校验）。
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "cubProtectorVictim", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"。
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
      `polar bear did not protect cub by attacking player, hp=${(health as any).currentValue}`);
  });

  // 引用 player 避免未使用警告（玩家存在即触发目标选择，无需主动操作）。
  void player;
}

// 幼熊被攻击后呼唤附近成年熊反击（wiki tech_北极熊.txt#行为：幼年北极熊被攻击时会呼唤
// 附近的成年北极熊前来协助攻击攻击者。这是北极熊幼崽保护的另一核心交互）。
//
// C++ 链路：PolarBearEntity registerGoals targetSelector 优先级1：PolarBearHurtByTargetGoal
//   （PolarBearEntity.cpp:454-480，继承 HurtByTargetGoal(this, false)）。
//   startExecuting：
//     - m_bear->isChild() 幼熊分支：boundingBox().expand(16,4,16) 扫附近成年熊（!isChild()），
//       对每只成年熊 setAttackTarget(m_target=攻击者) + setAngry(true)，然后 resetTask()
//       （幼熊自身不反击，仅呼唤）。
//     - 成年熊分支：调 HurtByTargetGoal::startExecuting() 设自身 attackTarget=攻击者。
//   被呼唤的成年熊经 PolarBearMeleeAttackGoal 接近 attackEntityAsMob→hurt(玩家, 6.0)。
//
// 环境选择：creeper_pit（7×5×7）。幼熊(2,2,3)、成年熊(5,2,3)距 3 格（<16 格呼唤范围），
//   Survival 玩家(4,2,4)距成年熊约 1.4 格（<20 格寻路范围）。玩家攻击幼熊 → 幼熊呼唤成年熊 →
//   成年熊设 attackTarget=玩家 → MeleeAttack 接近攻击玩家。脚下 y=1 玻璃支撑。
//
// 判定手段：断言玩家 HP 下降（<20）。被呼唤的成年熊攻击玩家，近战命中 hurt(6.0)。
// 时序：玩家 attackEntity(幼熊)（tick 8）+ 幼熊 PolarBearHurtByTargetGoal 呼唤成年熊 +
//   成年熊 MeleeAttackGoal 寻路接近 + 攻击冷却 + hurt(6.0)。maxTicks=1000 留充裕余量。
//   玩家用 Survival（创造模式被 TargetGoal 滤掉）。玩家攻击幼熊用 attackEntity（远程命中，
//   基岩语义 attack can be performed at any distance）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_北极熊.txt#行为（幼崽呼唤成年熊）
// Ref: PolarBearEntity.cpp:459-480（PolarBearHurtByTargetGoal isChild 呼唤分支）
function polarBearCubCallsAdultWhenHurt(test: Test): void {
  const polarBearType = "polar_bear";

  // 幼熊(2,2,3)、成年熊(5,2,3)、Survival 玩家(4,2,4)。脚下 y=1 玻璃支撑。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 4 });
  // 幼熊：spawn_baby 事件生成幼年北极熊。
  const cub = test.spawn(`${polarBearType}<minecraft:spawn_baby>`, { x: 2, y: 2, z: 3 });
  // 成年熊（默认成年），距幼熊 3 格在 16 格呼唤范围内。
  test.spawn(polarBearType, { x: 5, y: 2, z: 3 });
  // Survival 玩家。
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 4 }, "cubAttacker", 0 as any);

  // tick 8 后玩家攻击幼熊：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发幼熊 PolarBearHurtByTargetGoal isChild 分支呼唤成年熊。
  test.runAtTickTime(8, () => {
    player.attackEntity(cub);
  });

  // 断言玩家掉血：被呼唤的成年熊攻击玩家。succeedWhen 每 tick 检查玩家 HP<20。
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
      `adult polar bear did not come to protect hurt cub, hp=${(health as any).currentValue}`);
  });
}

// 对照组：无幼熊时成年北极熊不主动攻击玩家（验证 PolarBearAttackPlayerGoal 的幼熊检测守卫，
// 排除"成年熊无条件攻击玩家"的假阳性）。
//
// C++ 链路：PolarBearAttackPlayerGoal::shouldExecute（PolarBearEntity.cpp:489-514）第三步扫 8×4×8
//   范围内幼熊，无幼熊时返 false 不攻击玩家。targetSelector 优先级3 的带谓词
//   NearestAttackableTargetGoal<Player> 同样要求附近有幼熊（谓词逻辑同）。优先级4 仅攻击狐狸。
//   故无幼熊时成年熊对玩家无任何主动攻击 goal，玩家靠近不掉血。
//
// 环境选择：creeper_pit（7×5×7）。成年熊(2,2,3)、Survival 玩家(5,2,3)距 3 格，无幼熊。
//   玩家不主动攻击熊（避免触发 HurtByTargetGoal 反击），仅靠近。成年熊无幼熊守卫不攻击玩家。
//
// 判定手段：等 200 tick（覆盖若干 PolarBearAttackPlayerGoal chance 检查周期）后断言玩家 HP 仍满血 20。
//   玩家保持 20 证明成年熊未主动攻击（幼熊检测守卫生效）。若守卫失效成年熊会攻击玩家致 HP<20。
//   maxTicks=400：200 tick 等待 + succeedWhen 检查余量。
//   玩家用 Survival（若用创造模式，TargetGoal 滤掉创造玩家，无法验证攻击守卫——须 Survival）。
// Ref: PolarBearEntity.cpp:489-514（shouldExecute 幼熊检测守卫）
function polarBearNoAggroWithoutCub(test: Test): void {
  const polarBearType = "polar_bear";

  // 成年熊(2,2,3)、Survival 玩家(5,2,3)距 3 格，无幼熊。脚下 y=1 玻璃支撑。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  test.spawn(polarBearType, { x: 2, y: 2, z: 3 });
  // Survival 玩家，不主动攻击熊（仅靠近），验证成年熊无幼熊时不主动攻击。
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "neutralObserver", 0 as any);

  // 等 200 tick（覆盖多个 PolarBearAttackPlayerGoal 检查周期）后断言玩家满血 20。
  // 玩家查询用区域限定排除并行测试污染。
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
      `polar bear should not attack player without cub nearby, hp=${(health as any).currentValue}`);
    test.succeed();
  });
}

export function registerPolarBearTests(): void {
  GameTest.register("MobBehaviorTests", "polar_bear_retaliates_when_attacked", polarBearRetaliatesWhenAttacked)
    .structureName("gametests:creeper_pit")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "polar_bear_attacks_fox", polarBearAttacksFox)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "polar_bear_immune_to_powder_snow", polarBearImmuneToPowderSnow)
    .structureName("gametests:creeper_pit")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "polar_bear_protects_cub_attacks_player", polarBearProtectsCubAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "polar_bear_cub_calls_adult_when_hurt", polarBearCubCallsAdultWhenHurt)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "polar_bear_no_aggro_without_cub", polarBearNoAggroWithoutCub)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);
}
