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

// open_grass_hall 结构尺寸（41×7×9），helper 相对坐标。四壁 glass 墙 + 内部/顶部全 air 露天草地长廊，
// y=0 grass_block 地板。狐狸逃避狼用：41 格长度容纳 AvoidEntityGoal ESCAPE_HORIZONTAL_RANGE=16 的逃避位移，
// 玻璃墙在边界阻止狐狸跑出查询区域。
const HALL_FROM = { x: 0, y: 0, z: 0 };
const HALL_VOLUME = { x: 41, y: 7, z: 9 };

// 狐狸免疫甜浆果灌木伤害与减速（wiki tech_狐狸.txt#行为：狐狸不受甜浆果灌木的伤害和减速影响；
// 蜜蜂同理免疫）。成熟（AGE≥2）灌木对其他 LivingEntity 在其中水平移动时每 ~10 tick 造成 1 伤害。
//
// C++ 链路：SweetBerryBushBlock::onEntityCollision（SweetBerryBushBlock.cpp:170-214）对 LivingEntity
//   生效，但 typeId=="minecraft:fox"||"minecraft:bee" 直接 return 免疫伤害+减速；其他 LivingEntity
//   在 AGE>0 灌木中水平移动（|dx|或|dz|≥0.003，对齐 vanilla horizontalDistanceSqr）受 hurt(1.0)
//   （有无敌帧约 10 tick 一次）。 Entity::doBlockCollisions（Entity.cpp:1296-1365）遍历实体 AABB
//   覆盖的所有方块格调 onEntityCollision，故实体 AABB 须覆盖灌木格。
//   碰撞箱对齐：甜浆果灌木不重写 getCollisionShape，继承 BushBlock 返回 empty（对齐 vanilla
//   noCollision()），实体可穿过灌木落入其中持续触发 onEntityCollision。
//
// 布局设计（关键修复，对齐 sweetBerryBushDamagesMovingEntity 范式）：
//   早期版本满铺 7×7 灌木（x,z∈[1,7]）——此布局致实体所在灌木格的水平 8 邻居全是灌木格，
//   而 WalkNodeProcessor::getNodeType 把灌木格判为 PathNodeType::DamageOther，
//   getNeighbors 白名单（Walkable/Water/Climbable/WalkableDoor/DoorOpen/FenceGate）不接纳
//   DamageOther → A* 搜不出路径（仅起点单点）→ 实体静止 → 不触发"水平移动"伤害分支 → 测试假通过/超时。
//   此排除行为与 vanilla Java 1.21.11（WalkNodeEvaluator.DAMAGE_OTHER malus=-1，findAcceptedNode 对
//   malus<0 邻居返回 null）及基岩 BDS 严格一致，非 Cubium 缺陷。满铺灌木场景下 vanilla 也静止。
//   修复：改灌木带（z=4 一行 x∈[1,7]）+ 两侧草地（z=3/z=5 不铺灌木）。实体站在灌木带（z=4）时，
//   水平邻居 z=3/z=5 是草地（Walkable），A* 能搜出穿越灌木-草地边界的路径，实体水平移动触发伤害。
//
// 对照实体选择（关键修复）：必须用"狐狸不主动攻击"的实体作对照。
//   早期版本用鸡——但 FoxEntity 的 NearestAttackableTargetGoal 谓词匹配 CHICKEN/RABBIT
//   （FoxEntity.cpp:690），狐狸会主动咬鸡。被追的鸡逃跑移动 + 被咬致死均使鸡 HP 下降/消失，
//   混淆对照（鸡掉血可能是狐狸咬伤而非灌木造伤，致"灌木造伤"判定假通过）。
//   修复：改用猪。狐狸 NearestAttackableTargetGoal 不匹配 PIG，狐狸完全忽略猪；猪 HP 下降
//   纯粹来自甜浆果灌木伤害，对照干净。猪继承 AnimalEntity MAX_HEALTH=10（与狐狸同），体型 0.9×0.9，
//   有 RandomWalkingGoal（优先级5），移动性与鸡相当，AABB 充分覆盖灌木格。
//
// 环境选择：grass_pen（9×5×9）。结构放置抬高一格（placeOrigin=origin+(0,1,0)），
// helper 相对坐标用 origin（gridStartY=-59），故 helper-y=N → 世界 y=-59+N：
//   helper-y=1 → 世界 y=-58 = 结构内 y=0（满铺 grass_block 草地地板）
//   helper-y=2 → 世界 y=-57 = 结构内 y=1（air，本测试在此铺 z=4 灌木带）
//   helper-y=3 → 世界 y=-56 = 结构内 y=2（air，实体 spawn 位）
// 甜浆果灌木需种在草地/泥土上（canSustain 检查 BlockTags::VALID_SWEET_BERRY_BUSH_GROUND），
// helper-y=2 灌木的下方 helper-y=1 是 grass_block 草地，支撑通过。实体 spawn 在 helper-y=3，
// 落到草地顶（世界 y=-57.0=灌木格底），AABB 覆盖灌木格（世界 y=-57）：猪高 0.9 → AABB y∈[-57,-56.1]
// 覆盖灌木格 y=-57；狐狸用基类默认高 1.8 → AABB y∈[-57,-55.2] 覆盖灌木格 y=-57。两者均触发
// onEntityCollision。
//
// 狐狸睡眠规避：FoxSleepGoal（白天+hasShelter 时触发，mutex 占用 Move 阻塞 RandomWalking）。
//   hasShelter = !canSeeSky。grass_pen 埋在 gridStartY=-59 地下 worldgen 石头中，canSeeSky 恒 false
//   → hasShelter=true → 狐狸白天睡眠不动。故加 skyAccess(true) 清空结构上方制造露天列，canSeeSky=true
//   → hasShelter=false → 狐狸不睡，RandomWalking 可执行（狐狸免疫，移动也不掉血，验证免疫需它移动）。
//
// 双断言（严谨性）：狐狸 HP==10（免疫从未受伤）+ 猪 HP<10（曾受灌木伤害）。
//   猪首次受灌木伤害即 HP<10（满血10→9），比鸡 HP<4 阈值更快达成（鸡需 1 次伤害即 4→3 满足<4，
//   猪也需 1 次伤害即 10→9 满足<10，等效；但猪 10 血更耐折腾，不会因偶发意外死亡消失致对照失效）。
//   猪受伤反证"灌木造伤链路对非狐狸生效"，狐狸不掉血是免疫而非灌木失效。
//
// 判定手段：pollUntilSucceed 每 20 tick 查区域内狐狸与猪，断言狐狸 HP==10（从未受伤）且猪 HP<10（曾受伤）。
// 时序：实体落入灌木 + RandomWalking 水平移动触发 onEntityCollision，猪/鸡每 ~10 tick 1 伤害。
//   首次伤害时间非确定（依赖 RandomWalking 随机穿越灌木带），多猪提高至少一只触发概率。
//   maxTicks=1200 留充裕余量吸收随机移动 + 无敌帧节奏 + 并行 tick 抖动。
//   狐狸/猪查询用区域限定排除并行测试污染；type 用 "minecraft:fox"/"minecraft:pig"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#行为（免疫甜浆果灌木伤害与减速）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_甜浆果.txt#生物（除狐狸蜜蜂外 age≥2 灌木移动受伤）
function foxImmuneToSweetBerryBush(test: Test): void {
  const foxType = "fox";
  const pigType = "pig";

  // 在 helper-y=2（世界 y=-57）的 z=4 一行（x∈[1,7]）铺 AGE=3 甜浆果灌木。下方 helper-y=1 是 grass_pen
  // 草地，支撑灌木（canSustain 通过）。两侧 z=3/z=5 保留草地不铺灌木，作为实体寻路的 Walkable 邻居
  // （灌木带布局原理见函数头注释）。AGE=3（成熟，AGE>0 造伤）。
  // setBlockWithStates flags=3 含邻居更新，放灌木时下方草地已就位，updatePostPlacement 支撑检查通过不移除。
  for (let x = 1; x <= 7; x++) {
    (test as any).setBlockWithStates("minecraft:sweet_berry_bush", { x, y: 2, z: 4 }, "age=3", 3);
  }

  // 狐狸 (2,3,4) + 3 只猪分散 spawn 在 helper-y=3 的灌木带（z=4）上，落入 helper-y=2 灌木层
  // （停在草地顶，AABB 覆盖灌木格）。猪用 z=4 灌木带不同 x 位置分散站位避免相互推挤。
  // 狐狸与猪都是被动生物有 RandomWalkingGoal，灌木带（z=4）两侧 z=3/z=5 是草地（Walkable 邻居），
  // A* 能搜出穿越灌木-草地边界的路径，实体水平移动触发 onEntityCollision 伤害。
  // 用 3 只猪提高至少一只移动触发灌木伤害的概率（单只猪 RandomWalking 随机性强，偶发久不移动致超时；
  // 3 只猪累计移动概率显著提升）。狐狸免疫不掉血（HP 恒 10）；猪受伤掉血（HP<10）作对照。
  // 狐狸不攻击猪（NearestAttackableTargetGoal 谓词仅 CHICKEN/RABBIT），故猪 HP 下降纯粹来自灌木。
  test.spawn(foxType, { x: 2, y: 3, z: 4 });
  test.spawn(pigType, { x: 4, y: 3, z: 4 });
  test.spawn(pigType, { x: 6, y: 3, z: 4 });
  test.spawn(pigType, { x: 3, y: 3, z: 4 });

  // 双断言：狐狸免疫（HP==10）+ 任一猪曾受伤（HP<10）。
  // pollUntilSucceed 轮询：狐狸必须始终满血（免疫从未受伤），任一猪必须掉过血（HP<10）。
  // 猪 10 血，灌木每 ~10 tick 1 伤害，首次伤害即 10→9 满足 HP<10；3 只猪累计提高触发率，maxTick 留
  // 充裕余量吸收 RandomWalking 随机性 + 并行环境 tick 抖动。
  // onTimeout 诊断：超时时打印狐狸/猪 HP 与位置，定位是"猪不动"还是"灌木不伤"还是"狐狸掉血"。
  pollUntilSucceed(test, () => {
    const foxes = test.getDimension().getEntities({
      type: "minecraft:fox",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (foxes.length === 0) return false;
    const foxHealth = foxes[0].getComponent("minecraft:health") as any;
    if (foxHealth === undefined) return false;
    // 狐狸免疫：HP 必须保持满血 10（从未受灌木伤害）。
    if (foxHealth.currentValue !== 10) return false;

    const pigs = test.getDimension().getEntities({
      type: "minecraft:pig",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    // 任一猪 HP<10 即证明灌木造伤链路对非狐狸生效（反证狐狸不掉血是免疫而非灌木失效）。
    for (const p of pigs) {
      const ph = p.getComponent("minecraft:health") as any;
      if (ph !== undefined && ph.currentValue < 10) {
        return true;
      }
    }
    return false;
  }, {
    maxTick: 1180,
    onTimeout: () => {
      const foxes = test.getDimension().getEntities({
        type: "minecraft:fox",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const pigs = test.getDimension().getEntities({
        type: "minecraft:pig",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const foxHp = foxes.length > 0
        ? (foxes[0].getComponent("minecraft:health") as any)?.currentValue
        : "gone";
      const foxPos = foxes.length > 0
        ? `(${foxes[0].location.x.toFixed(1)},${foxes[0].location.y.toFixed(1)},${foxes[0].location.z.toFixed(1)})`
        : "gone";
      const pigInfo = pigs.map(p => {
        const ph = p.getComponent("minecraft:health") as any;
        return `hp=${ph?.currentValue}@(${p.location.x.toFixed(1)},${p.location.y.toFixed(1)},${p.location.z.toFixed(1)})`;
      }).join(" ");
      test.assert(false,
        `fox_immune timeout (fox=${foxes.length} hp=${foxHp} pos=${foxPos}; pigs=${pigs.length} [${pigInfo}])`);
    },
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
// 环境选择：open_grass_hall（41×7×9 露天草地长廊，四壁玻璃墙）+ skyAccess(true)。
//   1. skyAccess(true) 露天使 canSeeSky=true → hasShelter=false → 狐狸白天不睡眠（FoxSleepGoal 需
//      hasShelter）、不躲阳光（FoxFindShelterGoal 需 hasShelter），AvoidEntityGoal(优先级4 占 Move flag) 可触发。
//   2. 41 格长度容纳逃避位移（关键修复）：AvoidEntityGoal._findEscapePosition 用
//      RandomPositionGenerator.findRandomTargetBlockAwayFrom(ESCAPE_HORIZONTAL_RANGE=16) 选远离狼的逃避位，
//      16 格范围在 open_grass_hall 41 长度内可达。早期版本用 grass_pen(9×9) 或 creeper_pit(7×7)：
//      grass_pen 9×9 太小，逃避位常选到墙外不可达位置致 nav->moveTo 寻路失败→shouldExecute 返 false→
//      狐狸不逃避被狼咬死；creeper_pit 开放坑无墙，狐狸逃避跑出 7×7 查询区域致 fox=0。open_grass_hall
//      41×9 内部空间大，逃避位落在墙内可达，玻璃墙阻止狐狸跑出查询区域。
//   3. 修复 Cubium RandomPositionGenerator.generateRandomOffset 偏差（对齐 vanilla
//      RandomPos.generateRandomDirectionWithinRadians）：原"50% 概率叠加 0.3 强度偏好"致逃避位经常
//      朝向威胁源（狐狸逃避狼时朝狼跑被咬死）；改为远离方向 ±PI/2 锥角内随机（vanilla getPosAway 语义），
//      逃避位始终落在远离狼的半圆内。
//   4. 无玩家避免 AvoidEntityGoal(玩家) 干扰。
//
// 关键设计：狐狸逃避远距速度 1.6（farSpeed/walkSpeedModifier）近距 1.4（nearSpeed/sprintSpeedModifier），
//   对齐 vanilla AvoidEntityGoal(this, Wolf.class, 8.0F, 1.6, 1.4)（Fox.java:182）。狼基础移速 0.3 ×
//   MeleeAttackGoal 1.0 = 0.3 格/tick；狐狸逃避 0.3 × 1.6 = 0.48 格/tick（仅快 60%，无法瞬间甩开）。
//   狐狸 spawn 在长廊中部(x=20)，狼在 x=24（距4 < avoidDistance 8 触发逃避），狐狸朝远离狼方向(x-方向)
//   逃避。vanilla AvoidEntityGoal 一次逃避位移 ESCAPE_HORIZONTAL_RANGE=16 格，但因 shouldContinueExecuting
//   路径走完即停 + 逃避位计算偶发失败（findBestPosition 候选越界/不可行走），狐狸走走停停，与狼距离波动。
//   故判定不要求"持续保持远离"，而是捕获"狐狸曾朝远离狼方向(x-)移动明显距离"——狐狸逃避的整体趋势。
//
// 判定手段：狐狸 x 坐标 < 初始 x - 3（狐狸朝远离狼的 x- 方向移动 3 格以上）。狐狸初始 x=20.5，狼在 x+
//   (24.5)，awayDirection=fox-wolf 朝 x-，逃避位 escapePos.x < fox.x，狐狸整体朝 x- 移动。判定 fox.x<17.5
//   捕获逃避成功瞬间，不受与狼距离波动影响（狼追击致 dist 波动但狐狸 x 持续减小）。狐狸逃避速度 0.48/tick，
//   移动 3 格需 ~6 tick，pollUntilSucceed 间隔 4 tick 在 t=4,8,12 采样能捕获。maxTick=400 留充足余量吸收
//   逃避 goal 启停随机性。区域限定 open_grass_hall 41×9 排除并行测试污染。
//   狐狸 HP 10、狼攻 2（WolfEntity ATTACK_DAMAGE=2），狐狸逃避快于狼追击，400 tick 内不被咬死（需 ~20 次命中）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#行为（逃离狼/北极熊/不信任玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_狐狸.txt#天敌（狼主动攻击狐狸）
function foxFleesFromWolf(test: Test): void {
  const foxType = "fox";
  const wolfType = "wolf";

  // 狐狸 (20,2,4)、狼 (24,2,4)，初始水平距 4 格 < AvoidEntityGoal 检测距离 8 格，触发狐狸逃避。
  // open_grass_hall helper-y=2 是 air 层，实体 spawn y=2 落到 y=1 草地顶。狐狸在中部 x=20，朝远离狼
  // (x-方向)逃避，16 格内到 x=4 仍在墙内（墙在 x=0/40）。z=4 在 9 宽度的中部。
  test.spawn(foxType, { x: 20, y: 2, z: 4 });
  test.spawn(wolfType, { x: 24, y: 2, z: 4 });

  const foxStart = test.worldLocation({ x: 20, y: 2, z: 4 });
  // 狐狸朝远离狼(x+)方向(x-)逃避的判定阈值：fox.x < foxStart.x - 3（朝 x- 移动 3 格以上）。
  const fleeThresholdX = foxStart.x - 3.0;

  // 轮询：狐狸 x < 初始 x - 3（狐狸朝远离狼的 x- 方向移动 3 格以上）。间隔 4 tick 捕获逃避瞬间。
  pollUntilSucceed(test, () => {
    const foxes = test.getDimension().getEntities({
      type: "minecraft:fox",
      location: test.worldLocation(HALL_FROM),
      volume: HALL_VOLUME,
    });
    if (foxes.length === 0) {
      return false;
    }
    // 狐狸逃离：x 坐标朝远离狼(x+)方向(x-)移动 3 格以上。
    return foxes[0].location.x < fleeThresholdX;
  }, {
    interval: 4,
    maxTick: 400,
    onTimeout: () => {
      const foxes = test.getDimension().getEntities({
        type: "minecraft:fox",
        location: test.worldLocation(HALL_FROM),
        volume: HALL_VOLUME,
      });
      const wolves = test.getDimension().getEntities({
        type: "minecraft:wolf",
        location: test.worldLocation(HALL_FROM),
        volume: HALL_VOLUME,
      });
      const foxPos = foxes.length > 0
        ? `(${foxes[0].location.x.toFixed(1)},${foxes[0].location.z.toFixed(1)})`
        : "gone";
      const wolfPos = wolves.length > 0
        ? `(${wolves[0].location.x.toFixed(1)},${wolves[0].location.z.toFixed(1)})`
        : "gone";
      const foxHp = foxes.length > 0 ? (foxes[0].getComponent("minecraft:health") as any)?.currentValue : "?";
      const foxX = foxes.length > 0 ? foxes[0].location.x : NaN;
      test.assert(false,
        `fox did not flee from wolf (fox=${foxes.length}@${foxPos} hp=${foxHp} x=${foxX.toFixed(1)}, ` +
        `wolf=${wolves.length}@${wolfPos}, fleeThresholdX=${fleeThresholdX.toFixed(2)})`);
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
    // 3 只猪对照 + maxTicks=1300：吸收 RandomWalking 随机性 + 并行 tick 抖动（pollUntilSucceed maxTick=1180
    // 留余量 < 测试 maxTicks，避免测试先 ExecutionTimeout）。详见测试函数注释。
    .skyAccess(true)
    .maxTicks(1300);

  GameTest.register("MobBehaviorTests", "fox_attacks_chicken", foxAttacksChicken)
    // batch("night")：夜晚 isDaytime()=false 规避 FoxSleepGoal(白天睡眠) 与 FoxFindShelterGoal(白天躲阳光)
    // 占 Move flag 阻塞攻击链路。skyAccess 双保险露天。详见测试函数注释。
    .batch("night")
    .structureName("gametests:creeper_pit")
    .skyAccess(true)
    .maxTicks(1300);

  GameTest.register("MobBehaviorTests", "fox_flees_from_wolf", foxFleesFromWolf)
    // skyAccess(true) 露天使狐狸不睡不躲（hasShelter=false），AvoidEntityGoal 可触发。
    // open_grass_hall（41×7×9）41 格长度容纳 AvoidEntityGoal ESCAPE_HORIZONTAL_RANGE=16 的逃避位移，
    // 玻璃墙阻止狐狸跑出查询区域。详见测试函数注释。
    .structureName("gametests:open_grass_hall")
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
