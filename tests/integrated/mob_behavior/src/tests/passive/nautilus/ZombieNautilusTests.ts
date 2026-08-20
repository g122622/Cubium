// 僵尸鹦鹉螺行为类 GameTest。
//
// 僵尸鹦鹉螺(ZombieNautilus)是 1.21.11 新生物,与普通鹦鹉螺(nautilus)同目录同实体族。本测试验证其
// 亡灵特性——阳光下燃烧,与 NautilusTests 的"陆地干涸(dryout)"形成姊妹对照:同一实体族的活体变种
// 走 dryout 伤害(陆地空气耗尽),亡灵变种走阳光下燃烧(亡灵白天点火),覆盖 nautilus 族两套陆地伤害机制。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 僵尸鹦鹉螺在阳光下着火（wiki other_僵尸鹦鹉螺.txt#亡灵分类：僵尸鹦鹉螺是亡灵生物，会在阳光下着火，
//   与普通鹦鹉螺不同——普通鹦鹉螺是活体水生生物不燃烧，陆地受干涸伤害）。
//
// C++ 链路（对齐 MC Java 1.21.11 ZombieNautilus + MobEntity::burnUndead）：
//   ZombieNautilusEntity : AbstractNautilusEntity : TameableEntity : AnimalEntity（非 MonsterEntity！）。
//   ZombieNautilusEntity::tick（ZombieNautilusEntity.cpp:137-145）调 AbstractNautilusEntity::tick() 后
//   直接调 burnUndead()——不经 MonsterEntity::handleDaylightBurning 的 m_burnsInDaylight 门控（僵尸
//   鹦鹉螺不是 MonsterEntity，无该机制），而是类自身在 tick 显式调 burnUndead 实现亡灵燃烧。
//   MobEntity::burnUndead（MobEntity.cpp:525-561）：isAlive() && isInDaylight() 时，取 sunProtectionSlot
//   装备；空（无鹦鹉螺铠甲）则 igniteForSeconds(8.0f) 点燃 8 秒。GameTest spawn 的实体 Body 槽空，走点燃分支。
//   isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky（同僵尸/骷髅/僵尸马燃烧链路）。
//   sunProtectionSlot 返回 EquipmentSlot::Body（鹦鹉螺铠甲槽），区别于僵尸用头盔槽。
//
// 与 zombie_horse_burns_in_daylight（僵尸马）同款模式：僵尸马同样是"非 MonsterEntity 但 tick 显式调
//   burnUndead"的亡灵变种。两者判定手段完全一致（轮询 onfire 组件）。本测试与 NautilusTests 的
//   nautilus_drys_out_on_land 对照：同目录同实体族，一个验证亡灵变种阳光下燃烧、一个验证活体变种陆地干涸。
//
// 亡灵属性修复：ZombieNautilusEntity 此前未覆写 getCreatureAttribute（基类 LivingEntity 默认返回
//   Undefined），致亡灵杀手附魔无加成、瞬间治疗/伤害药水不反转、凋灵玫瑰不免疫。本测试编写时已补覆写
//   返回 CreatureAttribute::Undead（对齐 Java ZombieNautilus.getMobType()==UNDEAD，照搬 ZombieHorseEntity
//   修复范式）。注意 burnUndead 不查 getCreatureAttribute（只查 isInDaylight + sunProtectionSlot），
//   故亡灵属性缺陷不阻塞燃烧测试，但影响附魔/药水语义，须一并修复。
//
// 环境选择：grass_pen（9×5×9 露天）。僵尸鹦鹉螺 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、
//   y=4 露天 → canSeeSky=true）。中心位置远离玻璃墙，MOVEMENT_SPEED=1.1 游荡不触及围栏；空气腔头顶均
//   露天无阴影。不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
//   因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 ZombieHorseTests/SkeletonTests 同款注释）。
//   不 batch("day")：GameTestServer 默认白天，isDaytime=true（同 zombie_burns/zombie_horse_burns 不 batch）。
//
// 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
//   期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，僵尸鹦鹉螺
//   无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（与 zombie_burns/zombie_horse_burns 同款）。
//   succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_僵尸鹦鹉螺.txt#亡灵分类（亡灵生物，阳光下着火）
function zombieNautilusBurnsInDaylight(test: Test): void {
  const zombieNautilusType = "zombie_nautilus";

  // 僵尸鹦鹉螺 spawn 于 (4,2,4)（grass_pen 露天空气腔，头顶露天 canSeeSky=true）。
  // helper-y=2 → 结构内 y=1 空气。白天露天触发 burnUndead→igniteForSeconds 点燃。
  const zombieNautilus = test.spawn(zombieNautilusType, { x: 4, y: 2, z: 4 });

  // 断言僵尸鹦鹉螺着火：succeedWhen 每 tick 轮询 onfire 组件，非 undefined 即通过。
  // 时序：isInDaylight 概率检查 ~25 tick 首次点燃 + 160 tick 燃烧，maxTicks=500 余量充足。
  test.succeedWhen(() => {
    const fire = zombieNautilus.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("zombie_nautilus not on fire yet");
    }
  });
}

export function registerZombieNautilusTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_nautilus_burns_in_daylight", zombieNautilusBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false。skyAccess=true 让 MinecraftStructurePlacer 清空结构 footprint
    // 正上方至世界顶部的所有方块，制造露天列使 canSeeSky=true（见 ZombieHorseTests/SkeletonTests 同款注释）。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后光照变更入队，需若干世界 tick 由 ServerWorld::tick 批量
    // 重算 skyLight 达 15。setupTicks 阶段让世界先 tick 20 次让光照稳定，再正式跑测试体。
    .setupTicks(20)
    .maxTicks(500);
}
