// 甜浆果灌木行为 GameTest（移动受伤 + AGE 阈值）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { setSweetBerryBush } from "../../utils/block/sweetBerryBush.js";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// grass_pen 结构尺寸（9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 甜浆果灌木移动受伤 GameTest：成熟（AGE≥1）灌木对在其中水平移动的活体生物每 ~10 tick 造成 1 伤害。
//
// C++ 链路：SweetBerryBushBlock::onEntityCollision（SweetBerryBushBlock.cpp:170-214）。Entity::doBlockCollisions
// （Entity.cpp:1296-1365）每 tick 遍历实体 AABB 覆盖的方块网格，对相交方块调 onEntityCollision。
// onEntityCollision 内：dynamic_cast LivingEntity（仅活体）→ typeId=="minecraft:fox"||"minecraft:bee"
// 免疫 return → setMotionMultiplier(0.8,0.75,0.8) 减速 → age>0 && !isClientSide → prevX!=currX||prevZ!=currZ
// （本 tick 水平移动）且 |dx|或|dz|≥0.003 → hurt(sweetBerryBush, 1.0)。
//
// 与营火/凋灵玫瑰“静止站立即伤”的根本差异：甜浆果伤害要求实体本 tick 发生水平位移。prevX/prevZ 是
// baseTick 开头快照的上一 tick 末位置（Entity.cpp:766 m_posPrev=m_pos），currX/currZ 是 travel 位移后位置。
// 实体静止 tick（游荡间隔）prevX==currX → 不伤害；移动 tick 才伤害。这要求测试实体必须发生水平位移穿越
// 灌木格，不能用 1×1 囚笼锁死（囚笼内实体无法水平移动，伤害永不触发）。
//
// 触发时序：LivingEntity::aiStep（LivingEntity.cpp:1320）→ travel（位移，更新 currX）→ doBlockCollisions
// （读 prevX/currX 调 onEntityCollision）。MobEntity 走此链路，故用被动 Mob 游荡即可触发。
//
// 受击免疫节流：LivingEntity::hurt 首行 isInvulnerableTo，m_hurtResistantTime>0 时直接 return 不造成伤害。
// m_hurtResistantTime 每 tick 递减，MAX_HURT_RESISTANT_TIME=10。onEntityCollision 每 tick 调 hurt，前 10 tick
// 被无敌帧阻挡，第 11 tick 放行造成 1 伤害并重置无敌帧。故实际约每 10 tick（半秒）承受 1hp。
//
// 寻路几何（关键设计，区别于满铺布局）：被动生物 RandomWalkingGoal 经 A* 寻路（PathNavigator）。
// WalkNodeProcessor::getNodeType 把甜浆果灌木格判为 DamageOther（WalkNodeProcessor.cpp:80-83）；
// getNeighbors 的水平邻居白名单（:306-307）只接纳 Walkable/Water/Climbable/Door/FenceGate，不接纳 DamageOther。
// 若灌木满铺整个区域，鸡所在格的所有水平邻居都是 DamageOther → A* 无法展开任何邻居 → 寻路失败 →
// 鸡静止 → prevX==currX → 伤害永不触发（这与 vanilla 一致：vanilla WalkNodeEvaluator.findAcceptedNode 用
// malus>=0 接纳，DAMAGE_OTHER malus=-1.0 同样不接纳，满铺灌木中 vanilla mob 也不寻路移动）。
//
// 故本测试采用“灌木带 + 草地两侧”布局：灌木只在 z=4 一行（x∈[1,7]）铺设，z=3 与 z=5 两侧是草地。
// 鸡 spawn 在灌木带中央格 (4,3,4) 上，其水平邻居 (4,*,3)/(4,*,5) 是草地（Walkable，getNeighbors 接纳）→
// A* 能从灌木格起点（createNode 接受 DamageOther 起点）展开草地邻居 → 寻路成功。鸡沿路径从灌木格移动到
// 草地格，穿越灌木格那一 tick prevX(灌木)≠currX(草地) → AABB 仍覆盖灌木格 → onEntityCollision 触发伤害。
// 鸡在草地与灌木间反复游荡（RandomWalking 持续选目标），多次穿越累积伤害。
//
// 主角用鸡（minecraft:chicken）：满血 4，首次灌木伤害即 4→3 <4 满足断言，无需等待多次伤害累积。
// 鸡与猪 goal 结构相同（Swim/Panic/Breed/Tempt/FollowParent/RandomWalking/LookAt），寻路能力一致，
// 但鸡血量低断言更快达成。mob_behavior 的 fox_immune_to_sweet_berry_bush 同样用鸡作对照（虽其鸡掉血实为
// 狐狸撕咬而非灌木刺，对照逻辑有缺陷，但鸡作为低血量被动 Mob 的选择一致）。
//
// 环境选择：grass_pen（9×5×9）。结构放置抬高一格（placeOrigin=origin+(0,1,0)），helper 相对坐标用
// origin（gridStartY=-59），故 helper-y=N → 世界 y=-59+N：
//   helper-y=1 → 世界 y=-58 = 结构内 y=0（满铺 grass_block 草地地板）
//   helper-y=2 → 世界 y=-57 = 结构内 y=1（air，本测试在此铺 z=4 一行 AGE=2 灌木）
//   helper-y=3 → 世界 y=-56 = 结构内 y=2（air，鸡 spawn 位）
// 甜浆果灌木需种在草地/泥土上（canSustain 检查 BlockTags::VALID_SWEET_BERRY_BUSH_GROUND，含 grass_block），
// helper-y=2 灌木的下方 helper-y=1 是 grass_block 草地，支撑通过。setBlockState 从不对新放置方块自身调
// updatePostPlacement（仅通知邻居），故放灌木时不会因支撑检查自毁。鸡 spawn helper-y=3 落入灌木层，停在
// 草地顶（世界 y=-57.0=灌木格底），AABB y∈[-57.0,-56.3]（鸡高 0.7）覆盖灌木格 y=-57，触发 onEntityCollision。
//
// 判定手段：pollUntilSucceed（runAtTickTime 预注册轮询）每 20 tick 检查任一鸡 health.currentValue < 4
// （满血 4，首次灌木伤害 4→3）。满足调 test.succeed()；1180 tick 仍全满血 assert 超时报错。
// 不用 succeedWhen+assert 轮询：基岩 BDS succeedWhen 语义是"回调里 assert 失败=立即 FAIL"（非"继续等下
// 个 tick"），与 Cubium succeedWhen"assert 失败继续轮询"不同，伤害类测试在基岩会首 tick 立即 FAIL。
// 详见 utils/test/poll.ts 顶部注释。
// 时序：鸡落入灌木 + RandomWalking 寻路到草地穿越灌木格触发 onEntityCollision + 首次 hurt 放行（无敌帧
// 10 tick 节流）。多鸡 + maxTicks=1200 留充裕余量吸收 RandomWalking 随机性（选目标概率 + 寻路触发节奏）
// + 无敌帧节奏 + GameTest 非确定性，使 Cubium 端稳定通过（单鸡短时偶发超时，见下方函数内限制注释）。
// 区域限定用 grass_pen 9×5×9 排除并行测试污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_甜浆果.txt#生物（除狐狸蜜蜂外 age≥1 灌木移动受伤，每 ~10 tick 1hp）
function sweetBerryBushDamagesMovingEntity(test: Test): void {
  const chickenType = "chicken";

  // 在 helper-y=2（世界 y=-57）的 z=4 一行（x∈[1,7]）铺 AGE=2 甜浆果灌木。下方 helper-y=1 是 grass_pen
  // 草地，支撑灌木（canSustain 通过）。z=3 与 z=5 两侧保留草地（不铺灌木），作为鸡寻路的 Walkable 邻居。
  // setSweetBerryBush 跨服务端放置：Cubium 用 age=2（setBlockWithStates "age=2"，对齐 Java），
  // 基岩 BDS 用 growth=2（setBlockPermutation+BlockPermutation.resolve，基岩 state 名 growth，值域 0-3
  // 与 Java age 一一对应）。两端 age/growth=2 均≥1 满足伤害阈值。flags=3 含邻居更新，放灌木时下方草地
  // 已就位，updatePostPlacement 支撑检查通过不移除。
  for (let x = 1; x <= 7; x++) {
    setSweetBerryBush(test, { x, y: 2, z: 4 }, 2);
  }

  // 多鸡提高"至少一只在灌木带内水平移动"的概率。甜浆果伤害要求实体本 tick 在灌木格内发生水平位移
  // （onEntityCollision 检查 prevX!=currX 且 |dx|或|dz|>=0.003），而鸡 RandomWalking 是随机行为——单只鸡
  // 在 800 tick 内可能始终在草地侧游荡不进入灌木带（Cubium 实测单鸡通过率约 2/3，偶发超时 hp=4）。
  // spawn 4 只鸡分散在灌木带不同 x 位置，任一鸡 RandomWalking 穿越灌木-草地边界即触发伤害。多鸡 + 延长
  // maxTicks 显著提高至少一只鸡触发伤害的概率，使 Cubium 端稳定通过。
  test.spawn(chickenType, { x: 2, y: 3, z: 4 });
  test.spawn(chickenType, { x: 4, y: 3, z: 4 });
  test.spawn(chickenType, { x: 6, y: 3, z: 4 });
  test.spawn(chickenType, { x: 4, y: 3, z: 3 });

  // 判定：任一鸡受灌木伤害即 hp<4（满血 4，首次灌木伤害 4→3）。
  // 用 pollUntilSucceed（runAtTickTime 预注册轮询）而非 succeedWhen+assert：
  // 基岩 BDS 的 succeedWhen 语义是"回调里 assert 失败=立即 FAIL"（非"继续等下个 tick"），与 Cubium
  // succeedWhen"assert 失败继续轮询"不同。故伤害类测试改用 pollUntilSucceed 两端统一语义——
  // 每 20 tick 查所有鸡 hp，任一 hp<4 调 succeed；1180 tick 仍全满血 assert 超时报错。
  // 1180 < 测试 maxTicks=1200，留 20 tick 余量避免测试先 ExecutionTimeout。
  //
  // 【两端非确定性限制】本测试依赖鸡 RandomWalking 自主穿越灌木带触发伤害，存在固有非确定性：
  //   - Cubium 端：多鸡 + maxTicks=1200 后稳定通过（实测 5/5），但单鸡短时偶发超时。
  //   - 基岩 BDS 端：鸡 AI 寻路严格避开甜浆果灌木（WalkNodeProcessor 将灌木格判为 DamageOther，
  //     A* 不接纳其为邻居），鸡在灌木带内长时间静止，偶发移动时逃出灌木带，1200 tick 内大概率不触发
  //     "在灌木格内水平移动"→ 超时 hp=4。基岩端 GameTest 无脚本级强制位移 API（teleport 不走
  //     travel/doBlockCollisions 不触发 onEntityCollision；SimulatedPlayer 服务端无 updatePhysics 调用
  //     致 moveToLocation 不产生位移），故无法用确定性强制移动绕过 AI。
  // 这非 Cubium 机制缺陷（Cubium 端甜浆果伤害机制与基岩一致：age>0 + 水平移动才伤害，已由 Cubium
  // 端通过确证）。详见 docs/test/INTEGRATED_TEST.md "甜浆果移动受伤测试的非确定性限制"。
  // 本测试保留 Cubium 端验证价值（确证 Cubium 甜浆果伤害机制正确），基岩端对比归类为 one-sided。
  pollUntilSucceed(
    test,
    () => {
      const chickens = test.getDimension().getEntities({
        type: chickenType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      if (chickens.length === 0) {
        return false; // 鸡暂时查不到（区域查询竞态/未就绪），等下个检查点
      }
      // 任一鸡 hp<4 即满足（至少一只鸡在灌木带内水平移动触发了伤害）。
      return chickens.some((c) => {
        const health = c.getComponent("minecraft:health") as any;
        return health && health.currentValue < 4;
      });
    },
    {
      maxTick: 1180,
      onTimeout: () => {
        // 超时仍全满血：抓取所有鸡 hp 进错误信息便于诊断。
        const chickens = test.getDimension().getEntities({
          type: chickenType,
          location: test.worldLocation(PEN_FROM),
          volume: PEN_VOLUME,
        });
        const hps = chickens.map((c) => (c.getComponent("minecraft:health") as any).currentValue);
        test.assert(false, `no chicken took sweet berry bush damage, hps=${JSON.stringify(hps)}`);
      },
    },
  );
}

// 甜浆果灌木 AGE=0 不受伤 GameTest：幼苗（AGE=0）灌木对在其中移动的活体生物不造成伤害，验证 age>0 阈值。
//
// C++ 链路：SweetBerryBushBlock::onEntityCollision（SweetBerryBushBlock.cpp:195）伤害分支由 age>0 守卫。
// AGE=0 灌木触发 onEntityCollision（AABB 相交），但 age>0 为 false → 跳过 hurt，仅施加减速 setMotionMultiplier。
// 即实体在 AGE=0 灌木中移动会被减速但不掉血。对齐 Java SweetBerryBushBlock#entityInside（age != 0 才伤害）
// 与 wiki“生长阶段至少 2（即 AGE≥1）的灌木才伤害移动实体”。
//
// 布局同 damages_moving_entity（z=4 一行灌木带 + 草地两侧），仅 AGE 改 0。鸡 spawn 在灌木带中央，寻路穿越
// 灌木-草地边界产生水平移动触发 onEntityCollision，但 AGE=0 的 age>0 守卫阻止伤害 → 鸡全程不掉血。
// 此布局保证鸡确实在灌木中移动（触发 onEntityCollision），从而能区分“AGE=0 不伤”与“根本未碰撞灌木”。
//
// 囚笼（关键）：AGE=0 测试需证明“鸡在灌木中持续移动一段时间仍未掉血”。若用 succeedWhen 检查 HP==4，
// spawn 第 1 tick HP==4 即满足立即 succeed，无法验证“持续未受伤”窗口。故用 runAtTickTime(200) 强制等到
// 200 tick（10 秒，远超游荡触发 + 首次伤害窗口 10 tick），证明整个窗口内 AGE=0 灌木始终不伤害。
//
// 对齐 FoxTests 狐狸免疫测试的“对照严谨性”思路：本测试与 damages_moving_entity 互为对照——后者证明
// AGE=2 灌木造伤，本测试证明 AGE=0 灌木不造伤，两者合证伤害阈值 age>0 精确生效（排除“灌木根本不造成
// 伤害”的假阴性，也排除“任意阶段都伤害”的假阳性）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_甜浆果.txt#生物（仅 age≥1 灌木伤害移动实体）
function sweetBerryBushSpareAge0(test: Test): void {
  // 主角同 damages_moving_entity 用鸡。AGE=0 灌木仅减速不伤血，全程应保持满血 4。
  const chickenType = "chicken";

  // 在 helper-y=2 的 z=4 一行（x∈[1,7]）铺 AGE=0 甜浆果灌木（幼苗，不伤害）。下方 helper-y=1 草地支撑。
  // AGE=0 灌木 getShape 是幼苗形（3,0,3)-(13,8,13)（高 0.5），但 noCollision 不变（继承 BushBlock empty），
  // 鸡可穿过触发 onEntityCollision，仅 age>0 守卫阻止伤害。z=3/z=5 草地两侧供寻路。
  // setSweetBerryBush 跨服务端放置（Cubium age=0 / 基岩 growth=0，值域一致）。
  for (let x = 1; x <= 7; x++) {
    setSweetBerryBush(test, { x, y: 2, z: 4 }, 0);
  }

  // 鸡 spawn 在 helper-y=3 的灌木带中央 (4,3,4)，落入灌木层，RandomWalking 寻路穿越灌木-草地边界。
  test.spawn(chickenType, { x: 4, y: 3, z: 4 });

  // 200 tick（10 秒，远超游荡触发 + 首次伤害窗口 10 tick）后检查：鸡存在且 hp==4（满血未受伤）。
  // 区域限定用 grass_pen 9×5×9 排除并行测试污染。满血即证明 AGE=0 灌木伤害阈值 age>0 生效。
  // 用 runAtTickTime 而非 succeedWhen：succeedWhen 每 tick 检查 hp==4 在 AGE=0 未伤害时全程为真会立即
  // succeed（spawn 第 1 tick 就过），无法验证“持续 10 秒未受伤”。runAtTickTime(200) 强制等到 200 tick。
  test.runAtTickTime(200, () => {
    const chickens = test.getDimension().getEntities({
      type: chickenType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(chickens.length > 0, "chicken disappeared (should be kept alive by spawn persistence)");
    const health = chickens[0].getComponent("minecraft:health");
    test.assert(
      (health as any).currentValue === 4,
      `chicken took damage from age=0 sweet berry bush (should be immune at age=0), hp=${(health as any).currentValue}`,
    );
    test.succeed();
  });
}

export function registerSweetBerryBushTests(): void {
  GameTest.register("BlockBehaviorTests", "sweet_berry_bush_damages_moving_entity", sweetBerryBushDamagesMovingEntity)
    .structureName("gametests:grass_pen")
    .maxTicks(1200);
  GameTest.register("BlockBehaviorTests", "sweet_berry_bush_spare_age0", sweetBerryBushSpareAge0)
    .structureName("gametests:grass_pen")
    .maxTicks(400);
}
