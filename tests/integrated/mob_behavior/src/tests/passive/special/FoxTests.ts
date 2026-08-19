// 狐狸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸（9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。开放坑无围墙，y=0 grass_block 地板、y=1..4 air。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 狐狸免疫甜浆果灌木伤害与减速（wiki tech_狐狸.txt#行为：狐狸不受甜浆果灌木的伤害和减速影响；
// 蜜蜂同理免疫）。成熟（AGE≥2）灌木对其他 LivingEntity 在其中水平移动时每 ~10 tick 造成 1 伤害。
//
// C++ 链路：SweetBerryBushBlock::onEntityCollision（SweetBerryBushBlock.cpp:170-214）对 LivingEntity
//   生效，但 typeId=="minecraft:fox"||"minecraft:bee" 直接 return 免疫伤害+减速；其他 LivingEntity
//   在 AGE>0 灌木中水平移动（|dx|或|dz|≥0.003，对齐 vanilla horizontalDistanceSqr）受 hurt(1.0)
//   （有无敌帧约 10 tick 一次）。 Entity::doBlockCollisions（Entity.cpp:1296-1365）遍历实体 AABB
//   覆盖的所有方块格调 onEntityCollision，故实体 AABB 须覆盖灌木格。
//   碰撞箱对齐：甜浆果灌木不重写 getCollisionShape，继承 BushBlock 返回 empty（对齐 vanilla
//   noCollision()），实体可穿过灌木落入其中持续触发 onEntityCollision。此前 override 返回 fullShape
//   致实体被挡在灌木外，伤害链路变死代码——已修复（删除 getCollisionShape override）。
//
// 环境选择：grass_pen（9×5×9）。结构放置抬高一格（placeOrigin=origin+(0,1,0)），
// helper 相对坐标用 origin（gridStartY=-59），故 helper-y=N → 世界 y=-59+N：
//   helper-y=1 → 世界 y=-58 = 结构内 y=0（满铺 grass_block 草地地板）
//   helper-y=2 → 世界 y=-57 = 结构内 y=1（air，本测试在此满铺 AGE=3 灌木）
//   helper-y=3 → 世界 y=-56 = 结构内 y=2（air，实体 spawn 位）
// 甜浆果灌木需种在草地/泥土上（canSustain 检查 BlockTags::VALID_SWEET_BERRY_BUSH_GROUND），
// helper-y=2 灌木的下方 helper-y=1 是 grass_block 草地，支撑通过。实体 spawn 在 helper-y=3，
// 落到草地顶（世界 y=-57.0=灌木格底），AABB 覆盖灌木格（世界 y=-57）：鸡高 0.7 → AABB y∈[-57,-56.3]
// 覆盖灌木格 y=-57；狐狸用基类默认高 1.8 → AABB y∈[-57,-55.2] 覆盖灌木格 y=-57。两者均触发
// onEntityCollision。满铺 7×7（x,z∈[1,7]）确保实体随机走动始终处于灌木格水平投影内。
//
// 实体移动前提（关键修复）：onEntityCollision 伤害需实体水平移动。RandomWalkingGoal 经
//   RandomPositionGenerator::findRandomTarget→isPositionWalkable 找可行走目标。此前 isPositionWalkable
//   用 !isAir()&&!isLiquid() 判阻挡，把 noCollision 的灌木误判为"被阻挡"，致 RandomWalking 找不到
//   目标、实体静止、水平位移为 0、伤害永不触发。已修复为 !isAir()&&!isLiquid()&&blocksMovement()
//   （对齐 vanilla isPathfindable(LAND)：noCollision 方块不阻挡寻路）。
//
// 狐狸睡眠规避：FoxSleepGoal（白天+hasShelter 时触发，mutex 占用 Move 阻塞 RandomWalking）。
//   hasShelter = !canSeeSky。grass_pen 埋在 gridStartY=-59 地下 worldgen 石头中，canSeeSky 恒 false
//   → hasShelter=true → 狐狸白天睡眠不动。故加 skyAccess(true) 清空结构上方制造露天列，canSeeSky=true
//   → hasShelter=false → 狐狸不睡，RandomWalking 可执行（狐狸免疫，移动也不掉血，验证免疫需它移动）。
//
// 对照设计（严谨性）：单断言"狐狸不掉血"无法排除"灌木根本不造成伤害"的假阴性。故加鸡作对照——
//   鸡在灌木中水平移动受伤（HP<4 或死亡），证明灌木伤害链路对非狐狸生效，反证狐狸"不掉血"是免疫
//   而非灌木失效。双断言：狐狸 HP==10（免疫）+ 鸡 HP<4 或消失（灌木造伤）。
//
// 判定手段：succeedWhen 每 tick 查区域内狐狸与鸡，断言狐狸 HP==10（从未受伤）且鸡 HP<4 或消失。
// 时序：实体落入灌木 + RandomWalking 水平移动触发 onEntityCollision，鸡 4 血每 ~10 tick 1 伤害，
// 4 次约 40+ tick 致死。狐狸始终免疫 HP=10。maxTicks=800 留充裕余量吸收随机移动 + 无敌帧节奏 +
//   GameTest 非确定性。狐狸/鸡查询用区域限定排除并行测试污染；type 用 "minecraft:fox"/"minecraft:chicken"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#行为（免疫甜浆果灌木伤害与减速）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_甜浆果.txt#生物（除狐狸蜜蜂外 age≥2 灌木移动受伤）
function foxImmuneToSweetBerryBush(test: Test): void {
  const foxType = "fox";
  const chickenType = "chicken";

  // 在 helper-y=2（世界 y=-57）满铺 7×7 AGE=3 甜浆果灌木（x,z∈[1,7]）。下方 helper-y=1 是 grass_pen
  // 草地，支撑灌木（canSustain 通过）。setBlockWithStates 设 age=3（成熟，AGE>0 造伤）。
  // flags=3 含邻居更新，放灌木时下方草地已就位，updatePostPlacement 支撑检查通过不移除。
  for (let x = 1; x <= 7; x++) {
    for (let z = 1; z <= 7; z++) {
      (test as any).setBlockWithStates("minecraft:sweet_berry_bush", { x, y: 2, z }, "age=3", 3);
    }
  }

  // 狐狸 (2,3,2)、鸡 (5,3,5) spawn 在 helper-y=3，落入 helper-y=2 灌木层（停在草地顶，AABB 覆盖灌木格）。
  // 分散站位避免相互推挤。两者都是被动生物有 RandomWalkingGoal，灌木（noCollision，blocksMovement=false）
  // 修复后不阻挡寻路，实体可在灌木中水平移动触发 onEntityCollision。
  test.spawn(foxType, { x: 2, y: 3, z: 2 });
  test.spawn(chickenType, { x: 5, y: 3, z: 5 });

  // 双断言：狐狸免疫（HP==10）+ 鸡受伤（HP<4 或消失）。
  // succeedWhen 每 tick 持续检查：狐狸必须始终满血（免疫从未受伤），鸡必须掉血或死亡。
  // 鸡 4 血，灌木每 ~10 tick 1 伤害，4 次约 40+ tick 致死；maxTicks=800 留充裕余量吸收随机性。
  test.succeedWhen(() => {
    const foxes = test.getDimension().getEntities({
      type: "minecraft:fox",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(foxes.length > 0, "fox disappeared (should be immune, not die)");
    const foxHealth = foxes[0].getComponent("minecraft:health");
    test.assert(foxHealth !== undefined, "fox has no health component");
    // 狐狸免疫：HP 必须保持满血 10（从未受灌木伤害）。
    test.assert((foxHealth as any).currentValue === 10,
      `fox took sweet berry bush damage (should be immune), hp=${(foxHealth as any).currentValue}`);

    const chickens = test.getDimension().getEntities({
      type: "minecraft:chicken",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    // 鸡受伤死亡消失——灌木伤害链路对非狐狸生效。
    if (chickens.length === 0) {
      return;
    }
    const chickenHealth = chickens[0].getComponent("minecraft:health");
    test.assert(chickenHealth !== undefined, "chicken has no health component");
    // 鸡必须在灌木中受伤（HP<4 满血），证明灌木造伤链路生效（反证狐狸不掉血是免疫而非灌木失效）。
    test.assert((chickenHealth as any).currentValue < 4,
      `chicken not damaged by sweet berry bush (bush damage link may be broken), hp=${(chickenHealth as any).currentValue}`);
  });
}

// 狐狸主动攻击鸡（wiki tech_狐狸.txt#攻击：狐狸会主动攻击鸡、兔子、鳕鱼、鲑鱼、热带鱼和陆地上的幼年海龟）。
//
// C++ 链路（FoxEntity.cpp registerGoals）：
//   targetSelector 优先级4：NearestAttackableTargetGoal<LivingEntity>（checkSight=false, chance=10）
//     谓词匹配 CHICKEN/RABBIT（FoxEntity.cpp:678-686）→ 选最近鸡设 attackTarget。
//   goalSelector 优先级5：FoxFollowTargetGoal（FoxGoals.cpp:159-247）shouldExecute 读 attackTarget(鸡)
//     且 distSq>START_FOLLOW_DISTANCE_SQ 时跟踪；tick 中 distSq<=STOP_FOLLOW_DISTANCE_SQ 时
//     setInterested(true)+setCrouching(true)+clearNavigation（蹲伏蓄力）。
//   goalSelector 优先级6：FoxPounceGoal（FoxGoals.cpp:300-340）shouldExecute 检查 isFullyCrouched()
//     + isPathClear → 扑击跳跃 attackEntityAsMob。
//   goalSelector 优先级7：FoxBiteGoal（FoxGoals.cpp:444-480）继承 MeleeAttackGoal，checkAndPerformAttack
//     在攻击距离内 attackEntityAsMob(*鸡) 造成近战伤害。
//   attackEntityAsMob → hurt(鸡, ATTACK_DAMAGE=2)（FoxEntity registerAttributes）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ batch("night") + skyAccess(true)。
//   1. FoxSleepGoal canFoxStart = isDaytime()&&hasShelter()&&!hasAlertableTarget()（FoxGoals.cpp:564）。
//      night batch 夜晚 isDaytime()=false → FoxSleepGoal 不触发（不睡眠占 Move flag 阻塞攻击）。
//   2. FoxFindShelterGoal canFoxStart = isDaytime()&&hasShelter()（FoxGoals.cpp:515，无 hasAlertableTarget 检查）。
//      night batch 夜晚 isDaytime()=false → FoxFindShelterGoal 不触发（不躲阳光寻路分心）。
//   3. skyAccess(true) 清空结构上方 worldgen 制造露天列。creeper_pit 埋 gridStartY=-59 地下，无 skyAccess
//      则 canSeeSky=false → hasShelter=true，白天会睡/躲（虽 night batch 已规避，skyAccess 双保险）。
//   4. 开放坑无围墙：targetSelector 攻击鸡 checkSight=false 不查视线，无墙遮挡选目标；FoxFollowTargetGoal
//      isPathClear 检查路径方块 canBeReplaced，开放坑 air 路径通畅不挡扑击（grass_pen 玻璃墙会挡 isPathClear
//      致扑击失败，故用 creeper_pit）。
//   5. 无 SimulatedPlayer：AvoidEntityGoal(玩家) 优先级4 会让未信任玩家 16 格内狐狸逃跑，干扰攻击。不 spawn
//      玩家则该 goal 不触发，狐狸专注攻鸡。
//
// 判定手段：鸡 HP 下降（<4 满血）或鸡死亡消失。狐狸近战 2 伤害，鸡满血 4，2 次咬致死。
// pollUntilSucceed 轮询区域内鸡 HP<4 或 length==0。攻击链路时序非确定（targetSelector chance=10 半 tick
//   评估 + Follow 跟踪 + Crouch 蓄力 + Pounce/Bite），maxTick=1200 留充裕余量。
// 区域限定排除并行测试污染；type 用 "minecraft:chicken"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#攻击（主动攻击鸡兔鱼幼海龟）
function foxAttacksChicken(test: Test): void {
  const foxType = "fox";
  const chickenType = "chicken";

  // 狐狸 (2,2,3)、鸡 (4,2,3)，水平距 2 格。creeper_pit 开放坑，实体 spawn y=2 落到 y=1 同层。
  // 距离 2 格 < targetSelector 搜索范围，狐狸迅速选定鸡为 attackTarget。
  test.spawn(foxType, { x: 2, y: 2, z: 3 });
  test.spawn(chickenType, { x: 4, y: 2, z: 3 });

  // 轮询：鸡 HP<4（受击）或鸡死亡消失（length==0）。
  pollUntilSucceed(test, () => {
    const chickens = test.getDimension().getEntities({
      type: "minecraft:chicken",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 鸡已被狐狸咬死消失——攻击行为生效。
    if (chickens.length === 0) {
      return true;
    }
    const health = chickens[0].getComponent("minecraft:health");
    if (health === undefined) {
      return false;
    }
    // 鸡受击掉血（<4 满血）——狐狸近战 2 伤害生效。
    return (health as any).currentValue < 4;
  }, {
    maxTick: 1200,
    onTimeout: () => {
      const chickens = test.getDimension().getEntities({
        type: "minecraft:chicken",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const hp = chickens.length > 0
        ? (chickens[0].getComponent("minecraft:health") as any)?.currentValue
        : "gone";
      test.assert(false,
        `fox did not attack chicken (chicken=${chickens.length}, hp=${hp})`);
    },
  });
}

// 狐狸逃离附近的狼（wiki tech_狐狸.txt#行为：狐狸会逃离附近的狼、北极熊或不信任的玩家；
//   tech_狐狸.txt#天敌：未驯服狼主动攻击 16 格内狐狸）。
//
// C++ 链路（FoxEntity.cpp registerGoals）：
//   goalSelector 优先级4：AvoidEntityGoal(狼/北极熊)（FoxEntity.cpp:607-616）检测距离 8 格，
//     谓词匹配 WOLF/POLAR_BEAR，近距逃跑速度 1.6、远距 1.4。shouldExecute 用 _findEntityToAvoid
//     （AABB 距离搜索，不查视线）找到 8 格内狼即 _findEscapePosition 逃离寻路。
//   AvoidEntityGoal 占 Move flag，狐狸朝远离狼方向移动（_isEscapePositionValid 保证逃跑位更远离狼）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ batch("night") + skyAccess(true)。
//   1. night batch 夜晚 isDaytime()=false 规避 FoxSleepGoal/FoxFindShelterGoal（见 foxAttacksChicken 注释）。
//   2. 开放坑无墙寻路通畅：AvoidEntityGoal _findEscapePosition 用 RandomPositionGenerator 找远离狼的位置，
//      玻璃墙（grass_pen）会阻挡逃跑寻路致 shouldExecute 返 false 不逃避；开放坑 air 路径通畅逃避正常。
//   3. 无玩家避免 AvoidEntityGoal(玩家) 干扰。
//
// 关键设计：狐狸逃避速度 1.6 远快于狼 0.3，狼追不上。判定阈值只需 +1.5 格（狐狸逃避 ~2 tick 即拉开），
//   在狐狸逃出 7×7 区域前 succeed（creeper_pit 开放坑狐狸逃远会跑出区域查询范围，故阈值小、maxTick 短，
//   在跑出前捕获逃避行为）。狐狸 HP 10、狼攻 4，短窗口内狼来不及咬死狐狸。
//
// 判定手段：狐狸与狼水平距离 > 初始距离 + 1.5 格（狐狸主动逃离拉开距离）。pollUntilSucceed 轮询，
//   逃避 goal 触发后狐狸朝远离狼方向移动，距离拉开即 succeed。maxTick=400 短窗口在狐狸跑出区域前捕获。
//   区域限定排除并行测试污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#行为（逃离狼/北极熊/不信任玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#天敌（狼主动攻击狐狸）
function foxFleesFromWolf(test: Test): void {
  const foxType = "fox";
  const wolfType = "wolf";

  // 狐狸 (2,2,3)、狼 (4,2,3)，初始水平距 2 格 < AvoidEntityGoal 检测距离 8 格，触发狐狸逃避。
  test.spawn(foxType, { x: 2, y: 2, z: 3 });
  test.spawn(wolfType, { x: 4, y: 2, z: 3 });

  const foxStart = test.worldLocation({ x: 2, y: 2, z: 3 });
  const wolfStart = test.worldLocation({ x: 4, y: 2, z: 3 });
  const initialDist = Math.hypot(foxStart.x - wolfStart.x, foxStart.z - wolfStart.z);

  // 轮询：狐狸与狼水平距离 > 初始距离 + 1.5 格（狐狸逃离拉开距离）。
  pollUntilSucceed(test, () => {
    const foxes = test.getDimension().getEntities({
      type: "minecraft:fox",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    const wolves = test.getDimension().getEntities({
      type: "minecraft:wolf",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (foxes.length === 0 || wolves.length === 0) {
      return false;
    }
    const fx = foxes[0].location.x;
    const fz = foxes[0].location.z;
    const wx = wolves[0].location.x;
    const wz = wolves[0].location.z;
    const dist = Math.hypot(fx - wx, fz - wz);
    // 狐狸逃离：与狼距离拉开到初始距离 + 1.5 格以上。
    return dist > initialDist + 1.5;
  }, {
    maxTick: 400,
    onTimeout: () => {
      const foxes = test.getDimension().getEntities({
        type: "minecraft:fox",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const wolves = test.getDimension().getEntities({
        type: "minecraft:wolf",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const dist = (foxes.length > 0 && wolves.length > 0)
        ? Math.hypot(foxes[0].location.x - wolves[0].location.x,
            foxes[0].location.z - wolves[0].location.z)
        : -1;
      test.assert(false,
        `fox did not flee from wolf (fox=${foxes.length}, wolf=${wolves.length}, initialDist=${initialDist.toFixed(2)}, curDist=${dist.toFixed(2)})`);
    },
  });
}

// 狐狸拾取地上的掉落物（wiki tech_狐狸.txt#行为：狐狸会拾取地上的任何物品，含食物与杂物；
//   拾取后叼在嘴里（主手槽），可吐出旧物换新物）。
//
// C++ 链路（两层协作，本测试同时覆盖两者）：
//   1. FoxFindItemsGoal（FoxGoals.cpp:877-958）goalSelector 优先级11：shouldExecute 需
//      !isHoldingItem && attackTarget==null && lastHurtBy==null && canAct(!坐/蹲/睡/卡/激怒)
//      && 1/10 概率 && 8 格内有可拾取 ItemEntity；startExecuting/tick 调 tryMoveTo 导航狐狸走向物品。
//      该 goal 仅负责"导航靠近"，不执行拾取。
//   2. MobEntity::tick 的 looting 扫描段（MobEntity.cpp，对齐 vanilla Mob.aiStep Mob.java:444-462）：
//      canPickUpLoot()&&isAlive()&&mobGriefing → 扫描 boundingBox().inflate(getPickupReach=(1,0,1))
//      内的 ItemEntity → canBePickedUp(pickupDelay<=0) && wantsToPickUp(=canHoldItem) → pickUpItem。
//      FoxEntity::pickUpItem（FoxEntity.cpp:339-375）：取 1 个入主手、多余掉落、吐出旧手持物、
//      ItemEntity.remove()。FoxEntity 构造 setCanPickUpLoot(true)（对齐 vanilla Fox.java:148）。
//      此 looting 段是本次新增的通用 Mob 拾取机制（此前 Cubium 仅 ItemPickupManager 处理玩家拾取，
//      Mob 拾取链路完全断开，FoxEntity::pickUpItem 写了但从未被调用）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ batch("night") + skyAccess(true)。
//   1. night batch 夜晚 isDaytime()=false 规避 FoxSleepGoal(白天睡眠占 Move flag 阻塞 FoxFindItemsGoal)
//      与 FoxFindShelterGoal(白天躲阳光)。canAct 要求 !isSleeping()，夜晚不睡→FindItemsGoal 可执行。
//   2. 开放坑无墙寻路通畅：FoxFindItemsGoal tryMoveTo 导航向物品，玻璃墙会挡寻路致导航失败。
//   3. skyAccess 双保险露天（canSeeSky=true → hasShelter=false，白天也不睡；虽 night 已规避）。
//   4. 无玩家避免 AvoidEntityGoal(玩家) 干扰狐狸走向物品。
//
// 物品选择：emerald（绿宝石）。狐狸 canHoldItem 主手空时可拾取任何物品（FoxEntity.cpp:296-314），
//   emerald 非食物不影响进食逻辑，纯验证拾取链路。spawnItem 落在狐狸附近 (3,2,3)，距狐狸 (2,2,3) 1 格。
//   狐狸随机走动或 FoxFindItemsGoal 导航靠近后，AABB.inflate(1,0,1) 覆盖物品格即拾取。
//
// 判定手段：区域内 item 实体数降为 0（被狐狸拾取，ItemEntity.remove()）。狐狸拾取后主手持物，
//   但狐狸本身非 item 实体不影响计数。pollUntilSucceed 轮询区域内 minecraft:item 实体数==0。
// 时序：spawnItem 落地(pickupDelay 默认 10 tick 后可拾取) + FoxFindItemsGoal 1/10 概率触发 +
//   导航靠近 + looting 扫描拾取。maxTick=1200 留充裕余量吸收 1/10 概率随机性。
//   区域限定排除并行测试污染；type 用 "minecraft:item"（掉落物实体类型）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#行为（拾取地上物品叼在嘴里）
function foxPicksUpItem(test: Test): void {
  const foxType = "fox";

  // 狐狸 (2,2,3) spawn 在 creeper_pit y=2 落到 y=1 同层。emerald 掉落物 (3,2,3)，
  // 距狐狸 1 格。狐狸随机走动或 FoxFindItemsGoal 导航靠近后拾取。
  test.spawn(foxType, { x: 2, y: 2, z: 3 });
  (test.spawnItem as any)("minecraft:emerald", { x: 3, y: 2, z: 3 });

  // 轮询：区域内 item 实体数==0（emerald 被狐狸拾取，ItemEntity.remove）。
  pollUntilSucceed(test, () => {
    const items = test.getDimension().getEntities({
      type: "minecraft:item",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 掉落物被狐狸拾取后移除——拾取链路生效。
    return items.length === 0;
  }, {
    maxTick: 1200,
    onTimeout: () => {
      const items = test.getDimension().getEntities({
        type: "minecraft:item",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const foxes = test.getDimension().getEntities({
        type: "minecraft:fox",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `fox did not pick up item (items=${items.length}, foxes=${foxes.length})`);
    },
  });
}

export function registerFoxTests(): void {
  GameTest.register("MobBehaviorTests", "fox_immune_to_sweet_berry_bush", foxImmuneToSweetBerryBush)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：清空结构上方 worldgen 制造露天列使 canSeeSky=true → hasShelter=false →
    // 狐狸白天不睡眠（FoxSleepGoal 不触发），RandomWalking 可执行移动验证免疫机制。
    .skyAccess(true)
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "fox_attacks_chicken", foxAttacksChicken)
    // batch("night")：夜晚 isDaytime()=false 规避 FoxSleepGoal(白天睡眠) 与 FoxFindShelterGoal(白天躲阳光)
    // 占 Move flag 阻塞攻击链路。skyAccess 双保险露天。详见测试函数注释。
    .batch("night")
    .structureName("gametests:creeper_pit")
    .skyAccess(true)
    .maxTicks(1300);

  GameTest.register("MobBehaviorTests", "fox_flees_from_wolf", foxFleesFromWolf)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .skyAccess(true)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "fox_picks_up_item", foxPicksUpItem)
    // batch("night")：夜晚规避 FoxSleepGoal(白天睡眠占 Move flag 阻塞 FoxFindItemsGoal)。
    // skyAccess 双保险露天。详见测试函数注释。
    .batch("night")
    .structureName("gametests:creeper_pit")
    .skyAccess(true)
    .maxTicks(1300);
}
