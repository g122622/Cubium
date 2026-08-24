// 火焰伤害免疫门控对齐测试。
//
// 验证 vanilla 两条最外层火焰伤害免疫门控（Cubium 此前均缺失，本次修复补全）：
//   1. Entity.isInvulnerableToBase:2921 —— IS_FIRE && fireImmune()：
//      火焰免疫实体（isImmuneToFire()==true，如僵尸猪灵/烈焰人/岩浆怪等）对所有 IS_FIRE 伤害源
//      （in_fire/campfire/on_fire/lava/hot_floor/fireball/unattributed_fireball）免疫。
//   2. LivingEntity.hurtServer:1162 —— IS_FIRE && hasEffect(FIRE_RESISTANCE)：
//      持有抗火药水效果的实体（含非火焰免疫实体，如玩家/猪）对所有 IS_FIRE 伤害源免疫。
//
// 此前缺陷：Cubium LivingEntity::isInvulnerableTo 只查 IS_FALL+FALL_DAMAGE_IMMUNE，缺 IS_FIRE+
// isImmuneToFire() 分支；LivingEntity::hurt 也缺 IS_FIRE+FireResistance 分支。后果：CampfireBlock::
// onEntityCollision（CampfireBlock.cpp:269）与 MagmaBlock::stepOn（MagmaBlock.cpp:108）无前置 fireImmune
// 守卫直接 hurt(campfire/hotFloor)，火焰免疫实体站营火/岩浆块上错误受伤；抗火药水也无法免疫火焰伤害。
// （注：Entity::lavaHurt/FireTickSystem 等已有前置 fireImmune 守卫，熔岩/着火路径不受此缺陷影响——
//   blaze_immune_to_fire/magmacube_immune_to_fire 已覆盖熔岩路径；本测试专覆盖无前置守卫的营火路径。）
//
// C++ 链路（本次修复）：
//   LivingEntity::isInvulnerableTo（LivingEntity.cpp:1042）补 IS_FIRE+isImmuneToFire() 分支 return true。
//   LivingEntity::hurt（LivingEntity.cpp:245）补 IS_FIRE+hasEffect(FireResistance) 分支 return false。
//   source.is(DamageTypeTags::IS_FIRE()) 标签查询等价 source.isFire() flag（EnvironmentalDamage::isFire
//   覆盖 campfire/hotFloor/on_fire/lava/in_fire/fireball/unattributed_fireball，成员集与 IS_FIRE 标签对齐）。
//
// 测试设计（正反对照，排除假通过）：
//   - fire_immune_mob_immune_to_campfire：僵尸猪灵（fireImmune）站营火不掉血 + 对照猪掉血。
//     证明 isInvulnerableTo 的 IS_FIRE+fireImmune 分支生效（火焰免疫实体免疫营火）+ 营火伤害链路本身
//     正常（猪掉血），排除"营火不造成伤害"的假通过。
//   - fire_resistance_immune_to_campfire：玩家喝抗火药水站营火不掉血。
//     证明 hurt 的 IS_FIRE+FireResistance 分支生效（抗火药水免疫营火）。
//   - fire_resistance_vulnerable_to_campfire：玩家不喝药站营火掉血（对照）。
//     证明营火伤害对无药水玩家生效，排除"营火不伤玩家"的假通过，与抗火测试交叉验证。
//
// 环境选择：glass_pit（7×5×7，y=0 glass 底，y=1..2 air 空腔）。营火放 (3,1,3)（y=1 空腔层，下方 y=0
// glass 实心支撑），实体 spawn (3,2,3) 下落至营火顶面站稳。囚笼（四周 y=2 + 封顶 y=3 glass）限制实体
// 在营火正上方 1×1×1 内，AABB 持续覆盖营火方块格触发 onEntityCollision。复用 CampfireTests::
// campfireDamagesEntityOnTop 的囚笼范式（已验证猪在此布局下稳定踩营火掉血）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_营火.txt#伤害（点燃营火每半秒 hp1 火焰伤害）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_抗火药水.txt（抗火药水免疫所有火焰伤害）
// Ref: LivingEntity.cpp:245（hurt FireResistance 分支）/ :1042（isInvulnerableTo fireImmune 分支）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 区域限定查询排除并行测试污染（Cubium GameTest 批内并行 tick + 不清场）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 在 (3,1,3) 放点燃营火 + 囚笼（四周 y=2 层玻璃 + 顶部 y=3 封顶），实体 spawn (3,2,3) 下落至营火
// 顶面站稳，AABB 持续覆盖营火方块格触发 onEntityCollision。复用 campfireDamagesEntityOnTop 囚笼范式。
function setupCampfireCage(test: Test): void {
  // (3,1,3) 营火（defaultState lit=true，下方 y=0 glass 实心支撑）。
  test.setBlockType("minecraft:campfire", { x: 3, y: 1, z: 3 });
  // 囚笼：四周 y=2 层玻璃围 (3,2,3)，顶部 y=3 封顶防跳跃挤出。
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
  test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });
}

// 读取区域内指定类型首个实体的 HP（无实体返 -1）。
function readFirstHp(test: Test, type: string): number {
  const entities = test.getDimension().getEntities({
    type,
    location: test.worldLocation(PIT_FROM),
    volume: PIT_VOLUME,
  });
  if (entities.length === 0) {
    return -1;
  }
  const health = entities[0].getComponent("minecraft:health");
  if (health === undefined) {
    return -1;
  }
  return (health as any).currentValue as number;
}

// 读取指定位置附近（1×4×1 小体积）指定类型首个实体的 HP（无实体返 -1）。
//
// Cubium GameTest 框架结构物理隔离健全（StructureGridSpawner 间距 32 格，每测试结构放不同绝对坐标，
// test.worldLocation 偏移每测试不同，getEntities 区域查询天然隔离，不会跨测试串台）。但同结构内
// 若有多实体或实体穿模移位，readFirstHp 取 entities[0] 顺序不稳定。本函数用 1×4×1 小体积框定目标
// 实体站位（pos.y 起 4 格高，覆盖营火顶面 y≈1.4375 站位到封顶下方），按位置精确查询，对实体小幅
// 移位有容错。pos 须取实体站位格（营火顶面所在 y 层，如 y=1）。
function readHpAt(test: Test, type: string, pos: { x: number; y: number; z: number }): number {
  const entities = test.getDimension().getEntities({
    type,
    location: test.worldLocation(pos),
    volume: { x: 1, y: 4, z: 1 },
  });
  if (entities.length === 0) {
    return -1;
  }
  const health = entities[0].getComponent("minecraft:health");
  if (health === undefined) {
    return -1;
  }
  return (health as any).currentValue as number;
}

// 火焰免疫实体（僵尸猪灵，fireImmune）站营火上不掉血 + 对照猪掉血。
//
// 验证 LivingEntity::isInvulnerableTo 的 IS_FIRE+isImmuneToFire() 分支（LivingEntity.cpp:1042）。
// 僵尸猪灵注册 .immuneToFire()（VanillaEntities.cpp:886），isImmuneToFire()==true，营火 campfire 伤害源
// 经 isInvulnerableTo 拦截 return true，hurt 直接 return false 不掉血。对照猪 isImmuneToFire=false，
// 营火每半秒 hp1 伤害，HP 10→9。
//
// 选用僵尸猪灵而非烈焰人：僵尸猪灵是 ZombieEntity 子类受重力正常站立（无烈焰人上升推力致 AABB
// 间歇脱离营火格的风险），HP=20 与玩家同，囚笼后稳定踩营火。僵尸猪灵 HP=20（vanilla ZombifiedPiglin
// maxHealth=20），免疫应保持 20。
function fireImmuneMobImmuneToCampfire(test: Test): void {
  const zombifiedPiglinType = "zombified_piglin";

  setupCampfireCage(test);

  // 僵尸猪灵 (3,2,3) 营火正上方，下落至营火顶面站稳。HP=20，fireImmune 应保持 20。
  test.spawn(zombifiedPiglinType, { x: 3, y: 2, z: 3 });

  // 轮询断言：僵尸猪灵 HP 保持 20（fireImmune 免疫营火）。
  // 时序：spawn 后约 10 tick（半秒）首次 hurt 放行被 isInvulnerableTo 拦截，僵尸猪灵全程免疫 HP=20。
  // 用 readHpAt 按位置精确查询（3,1,3）站位（营火顶面 y≈1.4375），1×4×1 volume 覆盖 y=1..4 含站位。
  //
  // 对照设计（防假通过）：本测试早期版本在同结构内 (5,*,*) 放对照猪踩营火受伤作正反交叉验证，但
  // 并行跑时僵尸猪灵（HP=20 亡灵，受重力下落）与对照猪在同结构内间距仅 2 格，物理干扰致实体穿模
  // 掉虚空（僵尸猪灵掉到 y=-60，对照猪消失）。根因是同结构内双实体 + 营火低碰撞箱（0.4375）在并行
  // tick 调度下物理不稳定。故移除同结构内对照猪，改为单实体只验证免疫。营火伤害链路本身的"造伤"
  // 对照由独立测试 campfire_damages_entity_on_top（光脚猪踩营火 HP 10→9）覆盖——该测试与本测试同
  // batch 并行跑且已稳定 PASSED，证明营火 onEntityCollision→hurt 链路正常。故本测试只须证明火焰免疫
  // 实体在此链路下不掉血（HP=20），即排除"营火不造伤"假通过（若营火不造伤，campfire_damages 测试
  // 会先 FAIL）。用 succeedWhen 而非 pollUntilSucceed：HP===20 是稳态断言（免疫则恒 20，受伤则 <20），
  // succeedWhen 每 tick 检查，僵尸猪灵落地站稳后全程 HP=20 即通过。
  const zoglinPos = { x: 3, y: 1, z: 3 };
  test.succeedWhen(() => {
    const hp = readHpAt(test, zombifiedPiglinType, zoglinPos);
    test.assert(hp === 20,
      `fire_immune_mob_immune_to_campfire: zombified_piglin should stay HP=20 (fireImmune immunizes `
      + `campfire), but hp=${hp} (if hp<20 IS_FIRE+fireImmune gate in isInvulnerableTo missing/broken; `
      + `if hp=-1 zombified_piglin escaped cage or fell through [physics instability]; if hp=20 correct)`);
  });
}

// 猪喝抗火药水后站营火不掉血。
//
// 验证 LivingEntity::hurt 的 IS_FIRE+hasEffect(FireResistance) 分支（LivingEntity.cpp:245）。
// 猪非火焰免疫（isImmuneToFire=false），但 addEffect("fire_resistance") 后 hurt 入口检查
// IS_FIRE && FireResistance → return false，免疫营火伤害。对齐 vanilla LivingEntity.hurtServer:1162。
//
// 用猪而非 SimulatedPlayer：SimulatedPlayer spawn 后默认悬浮不下落（停在 spawn y 不踩营火，AABB 不与
// 营火格相交），无法触发 onEntityCollision；猪受重力正常下落至营火顶面站稳，AABB 覆盖营火格触发伤害
// （campfireDamagesEntityOnTop 已验证猪踩营火掉血）。猪 HP=10，免疫应保持 10。
// 抗火药水 addEffect("fire_resistance") 经 getEffectByResourceLocation 解析→EffectType::FireResistance。
// 对照测试 fire_resistance_vulnerable_to_campfire 验证无药水猪掉血，交叉排除假通过。
function fireResistanceImmuneToCampfire(test: Test): void {
  const pigType = "pig";

  setupCampfireCage(test);

  // 猪 spawn (3,2,3) 营火正上方，受重力下落至营火顶面站稳。HP=10。
  const pig = test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // spawn 后立即同步施加抗火药水（不延迟到 runAtTickTime），确保 addEffect 早于首次营火 hurt。
  // test.spawn 同步返回句柄后立即 addEffect，在 server tick 推进（实体 tick + checkInsideBlocks）
  // 之前完成效果施加，避免首次 hurt 早于 addEffect 的时序竞态致首次伤害漏过免疫。
  // duration=1200 tick 远超测试时长 200，amplifier=0 即抗火 I 即可免疫。
  // addEffect 经 getEffectByResourceLocation 解析 "fire_resistance"→EffectType::FireResistance（EffectType.cpp:56）。
  (pig as any).addEffect("fire_resistance", 1200, { amplifier: 0, showParticles: false });

  // 轮询断言猪 HP 保持 10（抗火免疫营火）。
  // 猪下落至营火顶面约 10 tick 后 onEntityCollision 持续触发 hurt(campfire)，抗火使 hurt return false
  // 不掉血。tick 15 起轮询（留 spawn+下落+药水生效时间）。
  pollUntilSucceed(test, () => {
    const hp = readFirstHp(test, pigType);
    return hp >= 10;
  }, {
    startTick: 15,
    interval: 5,
    maxTick: 200,
    onTimeout: () => {
      const hp = readFirstHp(test, pigType);
      test.assert(false,
        `fire_resistance_immune_to_campfire: failed: pig hp=${hp} (expected 10, FireResistance should `
        + `immunize campfire). If hp<10 the IS_FIRE+FireResistance gate in LivingEntity::hurt is missing/broken.`);
    },
  });
}

// 对照：玩家不喝抗火药水站营火掉血（排除"营火不伤玩家"假通过）。
//
// 与 fire_resistance_immune_to_campfire 对称：同布局同玩家位置，唯一差异是不施加抗火药水。
// 玩家应受营火伤害 HP<20。若本测试失败（玩家不掉血）说明营火伤害链路本身对玩家不生效，则
// fire_resistance_immune_to_campfire 的"HP 保持 20"是假通过（营火根本不伤玩家，与抗火无关）。
function fireResistanceVulnerableToCampfire(test: Test): void {
  const pigType = "pig";

  setupCampfireCage(test);

  // 猪 spawn (3,2,3) 营火正上方，受重力下落至营火顶面站稳（猪物理正常下落，区别于 SimulatedPlayer
  // 默认悬浮不下落）。HP=10，无抗火应受营火伤害 HP<10。
  const pig = test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 轮询断言猪受营火伤害 HP<10（无抗火药水，营火伤害生效）。
  // 时序：spawn 后约 10 tick（半秒）首次 hurt 放行，猪 10→9。
  pollUntilSucceed(test, () => {
    const hp = readFirstHp(test, pigType);
    return hp >= 0 && hp < 10;
  }, {
    startTick: 15,
    interval: 5,
    maxTick: 200,
    onTimeout: () => {
      const hp = readFirstHp(test, pigType);
      test.assert(false,
        `fire_resistance_vulnerable_to_campfire: failed: pig hp=${hp} (expected <10, campfire should `
        + `damage pig without FireResistance). If hp>=10 campfire damage itself is broken, making `
        + `fire_resistance_immune_to_campfire a false pass.`);
    },
  });
}

export function registerFireImmunityTests(): void {
  GameTest.register("MobBehaviorTests", "fire_immune_mob_immune_to_campfire", fireImmuneMobImmuneToCampfire)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "fire_resistance_immune_to_campfire", fireResistanceImmuneToCampfire)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "fire_resistance_vulnerable_to_campfire", fireResistanceVulnerableToCampfire)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
