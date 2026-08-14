// 骷髅行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 骷髅在阳光下着火：白天露天环境（canSeeSky=true 且亮度>0.5）下，亡灵生物每 tick 有概率
// 被点燃 8 秒。C++ 链路：MonsterEntity::tick → handleDaylightBurning → burnUndead →
// igniteForSeconds(8.0f)；isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky。
// 骷髅默认 m_burnsInDaylight=true（MonsterEntity 基类），无 isImmuneToFire，故露天白天必燃。
// JS 侧读火焰状态：Entity.getComponent("minecraft:onfire") 未着火返回 undefined，
// 着火返回 OnFireComponent（对齐基岩 OnFireComponent 语义"组件存在即着火"）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅.txt#阳光下燃烧
function skeletonBurnsInDaylight(test: Test): void {
  const skeletonType = "skeleton";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙围栏+内部空气，y=4 全 air 露天。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 骷髅 spawn 于 helper-y=2（结构内 y=1 空气腔），头顶 y=2/y=3 空气、y=4 露天 → canSeeSky=true。
  // 中心位置（4,2,4）远离玻璃墙，骷髅 AI 游荡不会触及围栏；整个空气腔头顶均露天，无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自 @minecraft/server-gametest 内嵌的
  // @minecraft/server，与顶层包的 Entity 类型因 Dimension 属性差异不兼容，显式标注会触发 TS2322。
  const skeleton = test.spawn(skeletonType, { x: 4, y: 2, z: 4 });

  // 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
  // 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，
  // 骷髅无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（期望 25 tick + 余量）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = skeleton.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("skeleton not on fire yet");
    }
  });
}

export function registerSkeletonTests(): void {
  GameTest.register("MobBehaviorTests", "skeleton_burns_in_daylight", skeletonBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false（骷髅阳光燃烧测试稳定失败的根因）。skyAccess=true 让
    // MinecraftStructurePlacer 清空结构 footprint 正上方至世界顶部的所有方块，制造露天列使
    // canSeeSky=true。不设此值则 grass_pen 顶部被 worldgen 石头覆盖，第一次 PASSED 是结构偶然
    // 落在 worldgen 洞穴位置的 flaky 假象。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后，光照变更入队 m_lightQueue，需若干世界 tick 由
    // ServerWorld::tick 的 drainAndProcess 批量重算 skyLight 达 15。setupTicks 阶段（负 tickCount）
    // 让世界先 tick 20 次让光照稳定，再正式跑测试体，避免首 tick canSeeSky 仍为 false。
    .setupTicks(20)
    .maxTicks(500);
}
