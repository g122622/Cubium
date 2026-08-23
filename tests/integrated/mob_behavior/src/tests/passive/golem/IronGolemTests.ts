// 铁傀儡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 铁傀儡的韧性测试：验证铁傀儡能击败骷髅和僵尸。
function ironGolemArena(test: Test): void {
  const ironGolemType = "iron_golem";
  const skeletonType = "skeleton";
  const zombieType = "zombie";

  test.spawn(ironGolemType, { x: 4, y: 3, z: 3 });
  test.spawn(skeletonType, { x: 5, y: 3, z: 5 });
  test.spawn(skeletonType, { x: 4, y: 3, z: 4 });
  test.spawn(skeletonType, { x: 3, y: 3, z: 3 });
  test.spawn(zombieType, { x: 4, y: 3, z: 6 });
  test.spawn(zombieType, { x: 3, y: 3, z: 5 });
  test.spawn(zombieType, { x: 2, y: 3, z: 4 });
  test.spawn(zombieType, { x: 5, y: 3, z: 2 });

  test.succeedWhen(() => {
    test.assertEntityPresentInArea(zombieType, false);
    test.assertEntityPresentInArea(skeletonType, false);
    test.assertEntityPresentInArea(ironGolemType, true);
  });
}

// 玩家用铁块摆 T 形图案 + 顶部放雕刻南瓜，最后放南瓜触发建造生成铁傀儡
// （wiki tech_铁傀儡.txt#创建：4 个铁块摆成 T 形，最后在顶部放雕刻南瓜即生成铁傀儡）。
//
// C++ 链路：CarvedPumpkinBlock::onBlockAdded → trySpawnGolem（MelonPumpkinBlocks.cpp:196-243）。
// trySpawnGolem 按优先级 雪>铁>铜 检测，checkIronGolemPattern（:278-348）校验 T 形：
//   以南瓜 headPos 为顶，headPos.down()=armCenter（中层中央铁块），
//   headPos.down(2)=body（底层中央铁块），手臂方向东-西或南-北两端各一铁块。
//   顶层南瓜两侧 + 底层 body 两侧必须为 air。
//   先尝试东西手臂再南北手臂，任一匹配即 spawnIronGolem：移除 5 格方块（设 air）+
//   在 bodyPos 生成 iron_golem + setPlayerCreated(true)（玩家建造的不攻击玩家）。
//
// 关键：只有放南瓜（CarvedPumpkinBlock onBlockAdded）才触发检测，铁块放置不触发——
// 故测试顺序：先摆 4 铁块（T 形缺南瓜顶），最后放南瓜触发建造。对齐原版"最后放南瓜"语义。
//
// 图案坐标（glass_pit 内，南瓜 headPos=(3,4,3)）：
//   顶层 y=4: (3,4,3)=carved_pumpkin（最后放）
//   中层 y=3: (3,3,3)=iron_block(armCenter) + (3,3,2)=iron_block(东臂) + (3,3,4)=iron_block(西臂)
//   底层 y=2: (3,2,3)=iron_block(body)
//   顶层南瓜两侧 (3,4,2)(3,4,4)、底层 body 两侧 (3,2,2)(3,2,4) 须 air——glass_pit 内部全 air 满足。
//   注：东西方向用 z 轴偏移（Cubium east/west 对应 +z/-z，南北对应 +x/-x，此处用 z 轴作手臂方向，
//   pattern 校验 east()/west() 即 z±1，与 z 轴手臂等价）。
//
// 判定手段：建造成功后 5 格方块变 air + iron_golem 出现。succeedWhen 轮询区域内 iron_golem 数>=1。
// 建造是 onBlockAdded 同步触发（放南瓜那 tick 即 spawn），maxTicks=200 留 spawn 注册 + 余量。
// 区域限定到本测试 7×5×7，排除 iron_golem_arena 并行测试的铁傀儡污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#创建（T 形铁块 + 顶部南瓜）
function ironGolemBuiltByPlayer(test: Test): void {
  const ironGolemType = "iron_golem";

  // 先摆 4 个铁块（T 形缺南瓜顶）。铁块放置不触发建造检测（仅南瓜 onBlockAdded 触发）。
  // 底层 body：(3,2,3)。中层 armCenter：(3,3,3)。中层东西手臂：(3,3,2)(3,3,4)。
  test.setBlockType("minecraft:iron_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 2 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 4 });

  // 最后放南瓜（顶层 3,4,3）：onBlockAdded → trySpawnGolem → checkIronGolemPattern 匹配 →
  // spawnIronGolem 移除 5 格 + 生成 iron_golem。
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 4, z: 3 });

  // 建造是放南瓜那 tick 同步触发，iron_golem 立即 spawn 注册到世界。
  // succeedWhen 轮询区域内 iron_golem>=1 即通过。
  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(golems.length >= 1, `iron_golem not built, count=${golems.length}`);
  });
}

// 铁傀儡 T 形图案支持东西与南北两个手臂方向（wiki tech_铁傀儡.txt#创建：T 形可朝任意水平方向）。
// checkIronGolemPattern 先尝试东西手臂（armCenter.east/west）再南北手臂（armCenter.north/south），
// 两方向均合法。本测试用南北手臂方向（手臂沿 x 轴）验证第二个分支也能生成。
//
// 图案坐标（南瓜 headPos=(3,4,3)，南北手臂沿 x 轴）：
//   顶层 y=4: (3,4,3)=carved_pumpkin
//   中层 y=3: (3,3,3)=iron_block(armCenter) + (2,3,3)=iron_block(南臂) + (4,3,3)=iron_block(北臂)
//   底层 y=2: (3,2,3)=iron_block(body)
//   顶层南瓜两侧 (2,4,3)(4,4,3)、底层 body 两侧 (2,2,3)(4,2,3) 须 air——glass_pit 内部全 air 满足。
//
// 与 iron_golem_built_by_player（东西手臂）互补：两方向均能生成，交叉验证 pattern 双分支。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#创建（T 形可朝任意水平方向）
function ironGolemBuiltNorthSouthArms(test: Test): void {
  const ironGolemType = "iron_golem";

  // 先摆 4 个铁块（南北手臂 T 形缺南瓜顶）。
  // 底层 body：(3,2,3)。中层 armCenter：(3,3,3)。中层南北手臂：(2,3,3)(4,3,3)。
  test.setBlockType("minecraft:iron_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 2, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 4, y: 3, z: 3 });

  // 最后放南瓜触发建造（南北手臂分支匹配）。
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 4, z: 3 });

  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(golems.length >= 1, `iron_golem not built (N-S arms), count=${golems.length}`);
  });
}

// 铁傀儡不攻击苦力怕（wiki tech_铁傀儡.txt#行为：铁傀儡主动攻击敌对生物，但唯独不攻击苦力怕）。
//
// C++ 链路：IronGolemEntity::canAttackType（IronGolemEntity.cpp:217-231）对 CREEPER 类型硬返 false：
//   if (&type == VanillaEntityTypeKeys::CREEPER) return false;
// 该过滤在 TargetGoal::isSuitableTarget（TargetGoals.cpp:124-126）中自动调用，覆盖两条目标获取路径：
//   1. NearestAttackableTargetGoal（优先级3，选 MonsterEntity 子类）：苦力怕虽是 MonsterEntity 子类，
//      但 isSuitableTarget→canAttackType(CREEPER) 返 false，故不设为 attackTarget，铁傀儡不追不攻。
//   2. HurtByTargetGoal（优先级2，受击反击）：苦力怕不攻击铁傀儡（苦力怕 NearestAttackableTargetGoal
//      仅匹配 PLAYER），铁傀儡不被伤害，HurtByTargetGoal 不触发。
// 双路径均被 canAttackType 拦截，苦力怕与铁傀儡和平共存。
//
// 对照 iron_golem_arena：铁傀儡会主动攻击 zombie/skeleton（同为 MonsterEntity 子类，canAttackType 放行），
// 证明铁傀儡 AI 正常——本测试排除"铁傀儡 AI 整体失效"的假阴性。
//
// 判定手段：铁傀儡若攻击苦力怕，attackEntityAsMob 随机化伤害 7~21（IronGolemEntity.cpp:184-189），
// 苦力怕 20 血，1~2 击秒杀。铁傀儡需先移动到苦力怕旁（MoveTowardsTargetGoal 32 格范围 + MeleeAttackGoal），
// 但因 canAttackType 拦截根本无 target，不会移动。故等 150 tick（远超铁傀儡接近+攻击时序）后苦力怕仍存活，
// 即证明铁傀儡未把苦力怕当目标、未攻击。
//
// 苦力怕不爆炸：无 SimulatedPlayer，苦力怕 NearestAttackableTargetGoal 不选任何目标，CreeperSwellGoal
// 需 attackTarget 且 distSq<9，attackTarget 恒 null，不膨胀。苦力怕不燃（shouldBurnInDaylight=false），
// day/night 均可，用默认 day 批次。
//
// 区域限定到本测试 7×5×7（PIT_FROM/PIT_VOLUME），排除 iron_golem_arena 等并行测试的实体污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#行为（不攻击苦力怕）
function ironGolemDoesNotAttackCreeper(test: Test): void {
  const ironGolemType = "iron_golem";
  const creeperType = "creeper";

  // 铁傀儡 (4,3,3)、苦力怕 (3,3,3)，紧邻 1 格。若 canAttackType 失效，铁傀儡立即选苦力怕为目标，
  // 1~2 击（约 20-40 tick）秒杀苦力怕。等 150 tick 后苦力怕仍存活即证明未受攻击。
  test.spawn(ironGolemType, { x: 4, y: 3, z: 3 });
  test.spawn(creeperType, { x: 3, y: 3, z: 3 });

  // tick 150 断言苦力怕仍存活（区域内 creeper 数 >= 1）。150 tick 远超铁傀儡接近+秒杀时序。
  test.runAtTickTime(150, () => {
    const creepers = test.getDimension().getEntities({
      type: creeperType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(creepers.length >= 1,
      `creeper was attacked/killed by iron_golem (canAttackType CREEPER filter failed), count=${creepers.length}`);
    test.succeed();
  });
}

// 玩家建造的铁傀儡被玩家攻击后不反击（wiki tech_铁傀儡.txt#行为：玩家建造的铁傀儡不会攻击玩家）。
//
// C++ 链路：IronGolemEntity::canAttackType（IronGolemEntity.cpp:217-231）对玩家建造的铁傀儡走专属守卫：
//   if (isPlayerCreated() && &type == VanillaEntityTypeKeys::PLAYER) return false;
// 该过滤在 TargetGoal::isSuitableTarget（TargetGoals.cpp:124-126）中自动调用。玩家攻击铁傀儡后，
// HurtByTargetGoal::shouldExecute（TargetGoals.cpp:267-299）取 lastHurtBy=player，调 isSuitableTarget(player)
// → canAttackType(PLAYER) → isPlayerCreated=true 命中守卫返 false → isSuitableTarget 返 false →
// HurtByTargetGoal 不触发，铁傀儡不设玩家为 attackTarget，不反击。
//
// 对照：自然生成的铁傀儡（isPlayerCreated=false）被玩家攻击会反击（canAttackType 守卫不走 isPlayerCreated
// 分支，返回 MobEntity::canAttackType 默认 true）。本测试用建造流程生成铁傀儡（spawnIronGolem 内
// setPlayerCreated(true)，MelonPumpkinBlocks.cpp:479），确保 isPlayerCreated=true 触发守卫。
//
// 建造流程：复用 iron_golem_built_by_player 的东西手臂 T 形（4 铁块 + 顶南瓜），铁傀儡生成在 bodyPos=(3,2,3)。
// Survival 玩家 (4,2,3) 紧邻（直线 1 格，attackEntity 远程命中不受距离限）。玩家脚下 (4,1,3) 放 glass 支撑
// 防下落（glass_pit y=1 air，参照 SilverfishTests 同款支撑范式）。
//
// 判定手段：玩家攻击铁傀儡后，轮询断言玩家 HP 恒 20（未掉血）。铁傀儡若反击（canAttackType 守卫失效），
// attackEntityAsMob 随机化伤害 7~21（IronGolemEntity.cpp:184-189），玩家 20 血 1~2 击秒杀，HP 必降。
// HP 恒 20 即证明铁傀儡未反击。攻击在 tick 10 执行（留建造 spawn + 玩家 spawn 注册稳定时间），
// 轮询 maxTick=150 覆盖铁傀儡反击时序（铁傀儡 HurtByTargetGoal 触发+MeleeAttackGoal 攻击约 20-40 tick）。
//
// 注意：玩家攻击铁傀儡造伤害 1（空手），铁傀儡 100 血不会死。铁傀儡 HurtByTargetGoal 不触发故不反击。
// 铁傀儡 NearestAttackableTargetGoal 只选 MonsterEntity 不选玩家，双路径均不攻击玩家。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#行为（玩家建造不攻击玩家）
function ironGolemPlayerCreatedDoesNotAttackPlayer(test: Test): void {
  const ironGolemType = "iron_golem";

  // 先摆 4 个铁块（东西手臂 T 形缺南瓜顶），与 iron_golem_built_by_player 同款图案。
  // 底层 body：(3,2,3)。中层 armCenter：(3,3,3)。中层东西手臂：(3,3,2)(3,3,4)。
  test.setBlockType("minecraft:iron_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 3 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 2 });
  test.setBlockType("minecraft:iron_block", { x: 3, y: 3, z: 4 });

  // 最后放南瓜触发建造：onBlockAdded → trySpawnGolem → checkIronGolemPattern 匹配 →
  // spawnIronGolem 移除 5 格 + 生成 iron_golem + setPlayerCreated(true)。
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 4, z: 3 });

  // 玩家脚下 (4,1,3) 放 glass 支撑（glass_pit y=1 air，防 Survival 玩家下落）。
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });

  // Survival 玩家 (4,2,3)，紧邻铁傀儡 (3,2,3) 直线 1 格。gameMode=0=Survival（attackEntity 需造伤害）。
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 10 玩家攻击铁傀儡：留建造 spawn + 玩家 spawn 注册稳定时间。
  // attackEntity 转发 Player::attack → playerAttack(EntitySource) → 铁傀儡 hurt（1 伤害，100 血不死）→
  // 铁傀儡 lastHurtBy=player + lastHurtByTimestamp 更新。HurtByTargetGoal 下次评估取 lastHurtBy=player，
  // isSuitableTarget→canAttackType(PLAYER)→isPlayerCreated 守卫返 false→不触发，铁傀儡不反击。
  test.runAtTickTime(10, () => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (golems.length > 0) {
      player.attackEntity(golems[0]);
    }
  });

  // tick 150 断言玩家 HP 仍满血 20（铁傀儡未反击）。
  // 攻击在 tick 10，铁傀儡 HurtByTargetGoal 若误触发 + MeleeAttackGoal 反击约 tick 30-50，
  // 玩家 HP 必降（伤害 7~21，1~2 击秒杀）。等 150 tick 远超反击时序，HP 仍 20 即证明铁傀儡全程未反击。
  // 用 runAtTickTime 而非 pollUntilSucceed：本测试是负向断言（HP 不降），须等完整反击窗口过才判定，
  // pollUntilSucceed 的"条件满足即 succeed"会在 tick 60 首检 HP=20 立即 succeed，漏掉延迟反击。
  test.runAtTickTime(150, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    const hp = (health as any).currentValue;
    test.assert(hp >= 20,
      `player-created iron_golem attacked player (canAttackType PLAYER guard failed), player hp=${hp}`);
    test.succeed();
  });
}

// 自然生成的铁傀儡被玩家攻击后会反击（wiki tech_铁傀儡.txt#行为：非玩家建造的铁傀儡受击反击攻击者）。
//
// 本测试是 iron_golem_player_created_does_not_attack_player 的**对照组**，交叉验证 canAttackType 的
// isPlayerCreated 守卫真正生效：
//   - 自然铁傀儡（isPlayerCreated=false）：canAttackType(PLAYER) 不走 isPlayerCreated 守卫分支，
//     返回 MobEntity::canAttackType 默认值（仅排除恶魂，对 PLAYER 返 true）→ HurtByTargetGoal 触发 →
//     铁傀儡反击玩家，玩家 HP 降。本测试断言此行为。
//   - 玩家建造铁傀儡（isPlayerCreated=true）：canAttackType(PLAYER) 命中守卫返 false → 不反击，HP 不降。
//
// 两测试互补：若玩家攻击对铁傀儡无效（attackEntity 未造伤害、lastHurtBy 未设），则两测试都会"假通过"
// （自然铁傀儡也不反击）。本对照测试要求自然铁傀儡**必须反击**，强制验证 attackEntity 造伤害链路通 +
// HurtByTargetGoal 评估链路通，从而排除 player_created 测试的假通过风险。
//
// C++ 链路：test.spawn("iron_golem") 创建自然铁傀儡（m_playerCreated=false，IronGolemEntity.hpp:209）。
// 玩家 attackEntity → 铁傀儡 hurt → lastHurtBy=player。HurtByTargetGoal::shouldExecute（TargetGoals.cpp:267）
// 取 lastHurtBy=player，isSuitableTarget→canAttackType(PLAYER)→isPlayerCreated=false 不走守卫→
// MobEntity::canAttackType 返 true→isSuitableTarget 返 true→设 player 为 attackTarget。
// MeleeAttackGoal（优先级1）+ MoveTowardsTargetGoal（优先级2，32 格范围）驱动铁傀儡接近玩家攻击。
//
// 判定手段：玩家攻击铁傀儡后，轮询断言玩家 HP<20（铁傀儡反击造伤害）。铁傀儡 attackEntityAsMob
// 随机化伤害 7~21，玩家 20 血，1~2 击即 HP<20。攻击在 tick 10，反击约 tick 30-60，maxTick=200 留余量。
// 用 pollUntilSucceed（正向断言 HP 降，条件满足即 succeed 合理）。
//
// 注：铁傀儡 100 血，玩家空手 1 伤害/次不致死；玩家 20 血，铁傀儡 7~21 伤害 1~2 击秒杀，但 pollUntilSucceed
// 首次检测到 HP<20 即 succeed，不会等玩家死亡。Survival 玩家（gameMode=0）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#行为（受击反击）
function ironGolemNaturalAttacksPlayerWhenHurt(test: Test): void {
  const ironGolemType = "iron_golem";

  // 自然铁傀儡 (3,2,3)（test.spawn 创建，isPlayerCreated=false）。
  test.spawn(ironGolemType, { x: 3, y: 2, z: 3 });

  // 玩家脚下 (4,1,3) 放 glass 支撑（glass_pit y=1 air，防 Survival 玩家下落）。
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });

  // Survival 玩家 (4,2,3)，紧邻铁傀儡直线 1 格。
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 10 玩家攻击铁傀儡：留 spawn 注册稳定时间。攻击触发铁傀儡 hurt→lastHurtBy=player。
  test.runAtTickTime(10, () => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (golems.length > 0) {
      player.attackEntity(golems[0]);
    }
  });

  // 轮询断言玩家 HP<20（铁傀儡反击造伤害）。攻击 tick 10，反击约 tick 30-60，maxTick=200 留余量。
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
    startTick: 30,
    interval: 10,
    maxTick: 200,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const hp = players.length > 0
        ? (players[0].getComponent("minecraft:health") as any)?.currentValue
        : "player gone";
      test.assert(false,
        `natural iron_golem did not counterattack player when hurt (attackEntity ineffective or HurtByTargetGoal broken), player hp=${hp}`);
    },
  });
}

// 读取区域内某类型实体的当前 HP（currentValue）。找不到返回 NaN。
function readIronGolemHp(test: Test, from: { x: number; y: number; z: number }, volume: { x: number; y: number; z: number }): number {
  const ents = test.getDimension().getEntities({
    type: "iron_golem",
    location: test.worldLocation(from),
    volume,
  });
  if (ents.length === 0) {
    return NaN;
  }
  const health = ents[0].getComponent("minecraft:health") as unknown as { currentValue?: number } | undefined;
  return health?.currentValue ?? NaN;
}

// 玩家手持铁锭右键残血铁傀儡可治疗它（wiki tech_铁傀儡.txt#治疗：玩家可手持铁锭右键铁傀儡，
// 每个铁锭回复 25 点生命值，播放修理音效；满血时不消耗铁锭）。
//
// C++ 链路（对齐 Java 1.21.11 IronGolem.mobInteract）：
//   玩家主手持 iron_ingot + interactWithEntity(iron_golem)（ScriptSimulatedPlayer 扩展绑定）
//   → Player::interactOn(iron_golem, MainHand)（Player.cpp:2843）
//   → iron_golem.processInitialInteract → MobEntity::processInitialInteract（MobEntity.cpp:639）
//     → 命名牌/刷怪蛋/拴绳/剪刀装备分支均不命中 → interactMob(player, hand)（MobEntity.cpp:753）
//   → IronGolemEntity::interactMob override（对齐 Java IronGolem.mobInteract）：
//     heldItem.getItem()==IRON_INGOT → 记录 healthBefore=health() → heal(25.0F)
//     → health()!=healthBefore（残血，治疗生效）→ 播 ENTITY_IRON_GOLEM_REPAIR 音效（pitch=1.0±0.2）
//     + 消耗 1 铁锭（创造模式跳过）→ 返 Success。
//
// 此前 Cubium 铁傀儡无 interactMob override（基类 MobEntity::interactMob 返 Pass），Player::interactOn
// 第3步返 Pass 后第4步走 Item::itemInteractionForEntity——而 IronIngotItem 未 override
// itemInteractionForEntity，致铁锭右键铁傀儡完全不治疗（对齐缺陷）。本次新增 IronGolemEntity::interactMob
// override 补全此链路。
//
// 残血构造：铁傀儡满血 100（MAX_HEALTH=100，IronGolemEntity.cpp:138）。用 addEffect("instant_damage")
// 造残血——InstantDamage 公式 amount=6<<amplifier（HealOrHarmMobEffect 伤害基数 6、指数 6*2^level；
// EffectInstance.cpp），amplifier=2 造 6<<2=24 点魔法伤害（addEffect 同步 applyInstantly 扣血）。
// 铁傀儡 isInvertedHealAndHarm()=false（Golem 非 INVERTED_HEALING_AND_HARM 标签成员），正常受魔法伤害
// （非亡灵治疗分支）。HP 降至 76，再用铁锭 heal(25) → 101 被 maxHealth 100 夹紧为 100（+24），HP 上升 24 完整可断言。
// 不用 Survival 玩家攻击造残血：自然铁傀儡（isPlayerCreated=false）受击会反击玩家干扰测试；addEffect
// 不触发 HurtByTargetGoal（无 lastHurtBy），铁傀儡不反击，环境干净。
//
// 环境选择：glass_pit（7×5×7）。铁傀儡 (3,2,3)，创造玩家 (5,2,3) 距 2 格。interactWithEntity 无距离
// 门控可远程治疗。创造模式不消耗铁锭，同一铁锭可重复用（但本测试单次治疗即可）。
//
// 判定手段：记录治疗前后 HP，断言 HP 上升（治疗生效）。不精确断言 +25（兼容 instant_damage 公式浮点
// 误差），断言 hpAfter > hpBefore 且 hpAfter - hpBefore 在合理范围（≥20，治疗量 25 减去 instant_damage
// 残血误差）。用 pollUntilSucceed 轮询（heal 同步生效，但留 tick 让 HP 同步到组件）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁傀儡.txt#治疗（铁锭右键治疗 +25）
function ironGolemHealedByIronIngot(test: Test): void {
  const ironGolemType = "iron_golem";

  // 铁傀儡 (3,2,3)（glass_pit 内部空气腔，helper y=2 → 结构 y=1 air，脚踩 y=0 glass_pit 地板）。
  // 创造玩家 (5,2,3)，距铁傀儡 2 格（interactWithEntity 远程转发 interactOn，无需贴脸）。
  const golem = test.spawn(ironGolemType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "healer");

  // 创造玩家主手持铁锭。setItem(slot=0 主手, true 强制覆盖)。
  // ItemStack 类型分裂（顶层 vs server-gametest 嵌套），as unknown 强转绕过编译期。
  const ironIngot = new ItemStack("minecraft:iron_ingot", 1);
  player.setItem(ironIngot as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 记录治疗前后 HP 的闭包变量。
  let hpBeforeDamage = NaN;
  let hpAfterDamage = NaN;

  // tick 5：读满血 HP 基准（应 100）。
  test.runAtTickTime(5, () => {
    hpBeforeDamage = readIronGolemHp(test, PIT_FROM, PIT_VOLUME);
  });

  // tick 6：施加瞬间伤害（amplifier=2 → 6<<2=24 魔法伤害）造残血。addEffect 同步 applyInstantly 扣血。
  test.runAtTickTime(6, () => {
    (golem as any).addEffect("instant_damage", 1, { amplifier: 2, showParticles: false });
  });

  // tick 8：读残血 HP，断言已下降（确认造残血成功），随后玩家持铁锭治疗铁傀儡。
  test.runAtTickTime(8, () => {
    hpAfterDamage = readIronGolemHp(test, PIT_FROM, PIT_VOLUME);
    // 确认造残血成功：HP 必须下降且未致死（>0）。amplifier=2 造 24 伤害，100→76。
    test.assert(hpAfterDamage < hpBeforeDamage && hpAfterDamage > 0,
      `iron_golem not damaged by instant_damage (amplifier=2), hpBefore=${hpBeforeDamage} hpAfter=${hpAfterDamage}`);
    // 玩家持铁锭右键铁傀儡 → interactMob → heal(25)。
    (player as any).interactWithEntity(golem);
  });

  // 轮询断言 HP 上升（治疗生效）。heal 同步生效，留 tick 让 HP 同步到 health 组件。
  // 治疗量 25（76+25=101>100 被 maxHealth 夹紧为 100，+24），断言 hpAfter > hpAfterDamage 且增量 ≥20。
  pollUntilSucceed(test, () => {
    const hpAfterHeal = readIronGolemHp(test, PIT_FROM, PIT_VOLUME);
    if (Number.isNaN(hpAfterHeal)) return false;
    return hpAfterHeal > hpAfterDamage && (hpAfterHeal - hpAfterDamage) >= 20;
  }, {
    startTick: 12,
    interval: 5,
    maxTick: 100,
    onTimeout: () => {
      const hpAfterHeal = readIronGolemHp(test, PIT_FROM, PIT_VOLUME);
      test.assert(false,
        `iron_golem not healed by iron_ingot (interactMob override broken), hpBeforeDamage=${hpBeforeDamage} hpAfterDamage=${hpAfterDamage} hpAfterHeal=${hpAfterHeal}`);
    },
  });
}

export function registerIronGolemTests(): void {
  GameTest.register("MobBehaviorTests", "iron_golem_arena", ironGolemArena)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(810);

  GameTest.register("MobBehaviorTests", "iron_golem_built_by_player", ironGolemBuiltByPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "iron_golem_built_north_south_arms", ironGolemBuiltNorthSouthArms)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "iron_golem_does_not_attack_creeper", ironGolemDoesNotAttackCreeper)
    .structureName("gametests:glass_pit")
    .maxTicks(250);

  GameTest.register("MobBehaviorTests", "iron_golem_player_created_does_not_attack_player", ironGolemPlayerCreatedDoesNotAttackPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(250);

  GameTest.register("MobBehaviorTests", "iron_golem_natural_attacks_player_when_hurt", ironGolemNaturalAttacksPlayerWhenHurt)
    .structureName("gametests:glass_pit")
    .maxTicks(250);

  GameTest.register("MobBehaviorTests", "iron_golem_healed_by_iron_ingot", ironGolemHealedByIronIngot)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
