// 僵尸马行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 僵尸马在阳光下着火（wiki 僵尸马主条目：僵尸马是亡灵生物，会在阳光下着火，与普通马不同）。
//
// C++ 链路：ZombieHorseEntity : AbstractHorseEntity : AnimalEntity（非 MonsterEntity！）。
// ZombieHorseEntity::tick（ZombieHorseEntity.cpp:64-70）调 AbstractHorseEntity::tick() 后直接调
// burnUndead()——不经 MonsterEntity::handleDaylightBurning 的 m_burnsInDaylight 门控（僵尸马不是
// MonsterEntity，无该机制），而是类自身在 tick 显式调 burnUndead 实现亡灵燃烧。
// MobEntity::burnUndead（MobEntity.cpp:488-524）：isAlive() && isInDaylight() 时，取 sunProtectionSlot
// 装备；空（无头盔）则 igniteForSeconds(8.0f) 点燃 8 秒。僵尸马 GameTest spawn 无装备，走点燃分支。
// isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky（同僵尸/骷髅燃烧链路）。
//
// 与 skeleton_horse（骷髅马）形成对照：骷髅马同是亡灵马但 SkeletonHorseEntity::tick 不调 burnUndead，
// 故骷髅马阳光下不燃烧（已测 skeleton_horse 陷阱激活，未覆盖燃烧维度——骷髅马本就不燃无需测）。
// 僵尸马燃烧验证亡灵马族的 burnUndead 显式调用分支。与 zombie_burns_in_daylight（僵尸燃，走
// MonsterEntity 默认 shouldBurnInDaylight=true 门控）对照：两者都 burnUndead→igniteForSeconds，
// 但触发路径不同（僵尸经 handleDaylightBurning 门控，僵尸马 tick 显式直调）。
//
// 环境选择：grass_pen（9×5×9 露天）。僵尸马 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、
// y=4 露天 → canSeeSky=true）。中心位置远离玻璃墙，僵尸马 AI 游荡不触及围栏；空气腔头顶均露天无阴影。
// 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity 因 Dimension
// 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
// 不 batch("day")：GameTestServer 默认白天，isDaytime=true（同 zombie_burns_in_daylight 不 batch）。
//
// 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
// 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，僵尸马
// 无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（与 zombie_burns/skeleton_burns 同款）。
// succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_马.txt#僵尸马（亡灵生物，阳光下着火）
function zombieHorseBurnsInDaylight(test: Test): void {
  const zombieHorseType = "zombie_horse";

  // 僵尸马 spawn 于 (4,2,4)（grass_pen 露天空气腔，头顶露天 canSeeSky=true）。
  // helper-y=2 → 结构内 y=1 空气。白天露天触发 burnUndead→igniteForSeconds 点燃。
  const zombieHorse = test.spawn(zombieHorseType, { x: 4, y: 2, z: 4 });

  // 断言僵尸马着火：succeedWhen 每 tick 轮询 onfire 组件，非 undefined 即通过。
  // 时序：isInDaylight 概率检查 ~25 tick 首次点燃 + 160 tick 燃烧，maxTicks=500 余量充足。
  test.succeedWhen(() => {
    const fire = zombieHorse.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("zombie horse not on fire yet");
    }
  });
}

export function registerZombieHorseTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_horse_burns_in_daylight", zombieHorseBurnsInDaylight)
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
