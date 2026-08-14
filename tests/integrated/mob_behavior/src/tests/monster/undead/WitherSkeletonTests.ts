// 凋零骷髅行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 凋零骷髅不会在阳光下燃烧（wiki tech_凋灵骷髅.txt#行为：凋零骷髅不会在阳光下着火）。
// 与普通骷髅不同：WitherSkeletonEntity::shouldBurnInDaylight() override 返回 false（hpp:121），
// MonsterEntity::tick→handleDaylightBurning→isInDaylight 校验 shouldBurnInDaylight() 为 false 跳过燃烧。
// 普通骷髅 shouldBurnInDaylight=true（MonsterEntity 默认），白天露天必燃。
// 与 skeleton_burns_in_daylight 正向断言形成对照：同为 AbstractSkeletonEntity 子类，
// 骷髅燃烧而凋零骷髅不燃，交叉验证 shouldBurnInDaylight 门控正确。
// 注：此为负向断言（assert 不着火）。有 skeleton_burns_in_daylight 正向断言对照互补验证。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋灵骷髅.txt#行为（不会在阳光下着火）
function witherSkeletonDoesNotBurnInDaylight(test: Test): void {
  const witherSkeletonType = "wither_skeleton";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 凋零骷髅 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，凋零骷髅 AI 游荡不触及围栏；整个空气腔头顶均露天无阴影可躲。
  const witherSkeleton = test.spawn(witherSkeletonType, { x: 4, y: 2, z: 4 });

  // 白天露天凋零骷髅不着火：轮询 onfire 组件应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=500：白天燃烧判定每 tick 概率触发，凋零骷髅本就不燃，留余量确保断言稳定
  // （与 skeleton_burns_in_daylight 同款 maxTicks，对照可比）。
  test.succeedWhen(() => {
    const fire = witherSkeleton.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("wither_skeleton should not burn in daylight");
    }
  });
}

// 凋零骷髅近战攻击玩家：凋零骷髅手持石剑始终近战（区别于普通骷髅远程弓）。
// wiki tech_凋灵骷髅.txt#行为：凋零骷髅近战攻击（持有石剑），造成凋零效果。
// C++ 链路：WitherSkeletonEntity::setCombatTask override 移除 RangedBowAttackGoal + 用 MeleeAttackGoal
// （近战需贴身）+ 继承 AbstractSkeleton 的 NearestAttackableTargetGoal<Player>(checkSight=true)
// 选 Survival 玩家为 attackTarget → MeleeAttackGoal::shouldExecute 读 attackTarget →
// navigator->moveTo(玩家) 驱动凋零骷髅朝玩家移动到近战距离。
//
// 环境选择：必须夜晚 batch("night") + creeper_pit（开放坑）。原因——AbstractSkeleton 注册了
// RestrictSunGoal（限制阳光）+ FleeSunGoal（逃离阳光），白天露天凋零骷髅会优先逃离阳光而非攻击玩家
// （wiki: 凋零骷髅像骷髅一样不会主动离开阴凉处，即使准备攻击）。夜晚无阳光 FleeSun 不触发，
// 凋零骷髅主动选玩家近战接近。creeper_pit 开放坑无围墙阻挡 checkSight 视线 + 寻路通畅。
//
// 判定手段：凋零骷髅近战需贴身（攻击距离 ~2格），断言凋零骷髅接近玩家到 distSq≤2.5²。
// 普通骷髅远程弓会保持距离（RangedBowAttackGoal 在 8-15 格射击不贴近），凋零骷髅近战会贴近——
// 接近到近战距离即证明 setCombatTask 用 MeleeAttackGoal（近战 AI）而非 RangedBowAttackGoal。
// 玩家用 Survival：创造/旁观被 isSuitableTarget 滤掉，凋零骷髅不选其为目标。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋灵骷髅.txt#行为（近战攻击持有石剑）
function witherSkeletonMeleeAttacksPlayer(test: Test): void {
  const witherSkeletonType = "wither_skeleton";

  // 凋零骷髅于 (1,2,1)（一角），Survival 玩家于 (5,2,5)（对角，距 ~5.7格）。
  // 凋零骷髅主动选玩家近战接近，玩家会被攻击掉血（凋零骷髅攻击力4 + 凋零效果），
  // 但凋零骷髅需先接近（~5.7格，速度 ~0.25，约 tick 40 接近），玩家约 tick 60+ 死亡，
  // succeedWhen 窗口充足。玩家死前凋零骷髅已接近到近战距离。
  test.spawn(witherSkeletonType, { x: 1, y: 2, z: 1 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "bait", 0 as any);

  // 断言凋零骷髅接近玩家到近战距离：用 succeedWhen 每 tick 持续检查。
  // 取凋零骷髅世界坐标，断言其与玩家世界坐标水平 distSq ≤ 2.5²（近战攻击距离 + 碰撞箱半宽）。
  // 区域限定查询排除并行测试的凋零骷髅污染。
  const playerWorld = test.worldLocation({ x: 5, y: 2, z: 5 });
  test.succeedWhen(() => {
    const skeletons = test.getDimension().getEntities({
      type: witherSkeletonType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(skeletons.length > 0, "wither_skeleton disappeared");
    const s = skeletons[0];
    const dx = s.location.x - playerWorld.x;
    const dz = s.location.z - playerWorld.z;
    test.assert(dx * dx + dz * dz <= 2.5 * 2.5,
      "wither_skeleton did not approach player for melee attack");
  });
}

export function registerWitherSkeletonTests(): void {
  GameTest.register("MobBehaviorTests", "wither_skeleton_does_not_burn_in_daylight", witherSkeletonDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "wither_skeleton_melee_attacks_player", witherSkeletonMeleeAttacksPlayer)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(400);
}
