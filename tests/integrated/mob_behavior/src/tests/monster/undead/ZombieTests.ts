// 僵尸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { addFourNotchedWalls } from "../../../utils/block/build.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// 僵尸在有缺口的砖墙间追逐村民，验证僵尸寻路 AI。
//
// 判定看僵尸而不是村民：村民带 AvoidHostileGoal，会朝远离僵尸的方向持续逃跑；
// 玻璃坑围墙的角落存在引擎层面的对角挤入缺陷，村民可能被挤出围墙（与本测试要验证的
// 僵尸寻路无关）。因此不再断言「村民 180 tick 后仍在区域内」（那会把村民逃跑误判为
// 寻路失败），改为断言僵尸从东南角出生点 (5,5) 绕过/穿过缺口砖墙迷宫，抵达村民出生点
// 所在的西北角（水平距离 < 3.5 格）。
// TODO: 修复 PhysicsEngine 的对角挤入（实体 AABB 被推入单个实心角落方块）后，可恢复
//       「村民仍在区域内」的原始断言。
function zombieVillagerChase(test: Test): void {
  const villagerType = "villager_v2";
  const zombieType = "zombie";

  addFourNotchedWalls(test, "minecraft:brick_block", 2, 1, 2, 4, 6, 4);

  test.spawn(villagerType, { x: 1, y: 3, z: 1 });
  const zombie = test.spawn(zombieType, { x: 5, y: 3, z: 5 });

  const target = test.worldLocation({ x: 1, y: 2, z: 1 });
  pollUntilSucceed(test, () => {
    return Math.hypot(zombie.location.x - target.x, zombie.location.z - target.z) < 3.5;
  }, {
    startTick: 40,
    interval: 10,
    maxTick: 300,
    onTimeout: () => {
      test.assert(false,
        `zombie did not path to villager side: zombie=${JSON.stringify(zombie.location)} target=${JSON.stringify(target)}`);
    },
  });
}

// 僵尸在阳光下着火（wiki mob_僵尸_ED.txt + 僵尸主条目#行为：僵尸是亡灵生物，会在阳光下着火）。
// 僵尸是最基础的亡灵怪物，分类"亡灵生物"，白天露天燃烧。
//
// C++ 链路：ZombieEntity : MonsterEntity，不 override shouldBurnInDaylight()，
// 继承 MonsterEntity 默认 true（m_burnsInDaylight=true）。MonsterEntity::tick→handleDaylightBurning
// →isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky + shouldBurnInDaylight()，
// 全部满足则 burnUndead→igniteForSeconds(8.0f) 点燃 8 秒。
//
// 与 skeleton_burns_in_daylight（骷髅燃）+ stray_burns_in_daylight（流浪者燃）
// + bogged_burns_in_daylight（沼骸燃）+ wither_skeleton_does_not_burn_in_daylight（凋零骷髅不燃）
// 形成五方对照：基础亡灵僵尸燃烧验证 MonsterEntity 默认 shouldBurnInDaylight=true 门控对最基础
// 亡灵生效，骷髅系子类继承同门控。zombie_villager_chase（batch night）已验证僵尸夜间寻路追击，
// 此测试补白天燃烧维度。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_僵尸_ED.txt（亡灵生物，阳光下着火）
function zombieBurnsInDaylight(test: Test): void {
  const zombieType = "zombie";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 僵尸 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，僵尸 AI 游荡不触及围栏；整个空气腔头顶均露天无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const zombie = test.spawn(zombieType, { x: 4, y: 2, z: 4 });

  // 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
  // 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，
  // 僵尸无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（与 skeleton_burns/stray_burns/bogged_burns 同款）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = zombie.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("zombie not on fire yet");
    }
  });
}

export function registerZombieTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_villager_chase", zombieVillagerChase)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(2000);

  GameTest.register("MobBehaviorTests", "zombie_burns_in_daylight", zombieBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false。skyAccess=true 让 MinecraftStructurePlacer 清空结构 footprint
    // 正上方至世界顶部的所有方块，制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后光照变更入队，需若干世界 tick 由 ServerWorld::tick 批量
    // 重算 skyLight 达 15。setupTicks 阶段让世界先 tick 20 次让光照稳定，再正式跑测试体。
    .setupTicks(20)
    .maxTicks(500);
}
