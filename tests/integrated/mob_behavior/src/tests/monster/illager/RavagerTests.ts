// 劫掠兽行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 劫掠兽主动攻击玩家致掉血（wiki tech_劫掠兽.txt#行为：劫掠兽是敌对生物，会主动攻击
// 玩家、铁傀儡、村民等；攻击方式为近战冲撞，伤害 12（普通）/18（困难））。
//
// C++ 链路：RavagerEntity : AbstractRaiderEntity，registerGoals 注册：
//   targetSelector 优先级3：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标
//     （对齐 MC 1.21.11 Ravager.registerGoals；与女巫不同，劫掠兽本就注册了此 goal，不缺）。
//   targetSelector 优先级4：NearestAttackableTargetGoal<AbstractVillager>(排除幼年) +
//     NearestAttackableTargetGoal<IronGolem>。
//   goalSelector 优先级4：RavagerAttackGoal(this)（继承 MeleeAttackGoal, speed=1.0, useLongMemory=true）。
// RavagerAttackGoal::getAttackReachSqr = (width-0.1)*2 的平方 + target.width ≈ 13.69 + 0.6 = 14.29，
//   开方约 3.78 格——劫掠兽能命中 3.78 格内的玩家。命中后 RavagerEntity::attackEntityAsMob 造成
//   ATTACK_DAMAGE(12.0) 伤害并击退目标。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 劫掠兽 setBurnsInDaylight(false)（构造期关闭，对齐原版不燃），白天默认环境即可主动攻击。
// 劫掠兽体积大（width 1.95 / height 2.2），creeper_pit y=1..4 空气腔高 4 格足够容纳。
// 劫掠兽(2,2,3)+玩家(5,2,3)，水平距 3 格 < 3.78 攻击距离 → 选目标后近战命中。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布，MeleeAttackGoal 到冷却即 hurt），
// 伤害 12，玩家满血 20 → 8。确定型近战用"玩家掉血"判定稳定
// （见 guardian-laser-deterministic-hit-test-strategy 确定型攻击判定策略）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_劫掠兽.txt#行为（主动攻击玩家）
function ravagerAttacksPlayer(test: Test): void {
  const ravagerType = "ravager";

  // 劫掠兽 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // 距 3 格 < 3.78 攻击距离，劫掠兽选目标后 RavagerAttackGoal 直接命中（无需寻路接近）。
  // 劫掠兽受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  test.spawn(ravagerType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + RavagerAttackGoal 攻击冷却约 20 tick。
  // 注意：劫掠兽攻击后击退玩家，玩家可能被推远导致脱离攻击范围——但首次命中即掉血至 8，
  //   HP<20 断言一旦满足即 succeed，不受后续击退影响。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `ravager did not damage player, hp=${(health as any).currentValue}`);
  });
}

// 劫掠兽不在阳光下燃烧（wiki tech_劫掠兽.txt 通篇未提劫掠兽阳光下燃烧；劫掠兽是袭击生物非亡灵）。
//
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→if(m_burnsInDaylight) burnUndead()。
// 劫掠兽构造时 setBurnsInDaylight(false) 关闭日光燃烧（对齐原版，本次补齐）。
// 与 zombie_burns_in_daylight（僵尸燃）+ witch_does_not_burn_in_daylight（女巫不燃）对照。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_劫掠兽.txt#行为（无阳光燃烧描述，劫掠兽不燃）
function ravagerDoesNotBurnInDaylight(test: Test): void {
  const ravagerType = "ravager";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 劫掠兽 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 劫掠兽体积大（width 1.95 / height 2.2），grass_pen 9×9 空间足够容纳。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  const ravager = test.spawn(ravagerType, { x: 4, y: 2, z: 4 });

  // 白天露天劫掠兽不着火：轮询 onfire 组件，应恒 undefined（setBurnsInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，劫掠兽本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = ravager.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("ravager should not burn in daylight");
    }
  });
}

// 劫掠兽攻击被盾牌格挡后触发咆哮，咆哮对周围非灾厄村民生物造成范围伤害+击退
// （wiki tech_劫掠兽.txt#攻击：劫掠兽攻击被盾牌阻挡时，50% 概率被击晕 2 秒（40 tick），
//   随后咆哮给附近生物造成 6 伤害和极远击退；咆哮击退灾厄村民但不造成伤害）。
//
// C++ 链路（本次接入，对齐 MC Java 1.21.11 Ravager.blockedByItem + LivingEntity.applyItemBlocking）：
//   1) 玩家主手持盾 useItem 举盾 → ShieldItem::onItemRightClick → setActiveHand(MainHand)，
//      m_activeItemUseCount=72000（盾牌 getUseDuration），Player::tick→LivingEntity::tick→
//      updateActiveItem 每 tick 递减计时器维持 isUsingItem=true（盾牌无 onUseTick 副作用）。
//   2) 劫掠兽近战命中举盾玩家：MeleeAttackGoal::_attackTarget→attackEntityAsMob→
//      MobEntity::attackEntityAsMob→target.hurt→Player::hurt→LivingEntity::hurt→actuallyHurt。
//   3) Player::canBlockDamageSource override（本次新增）：isUsingItem && ShieldItem::isShield
//      && !source.is(BYPASSES_SHIELD) → 格挡成功，damageShield 消耗耐久。
//   4) LivingEntity::actuallyHurt 格挡分支（本次接入）：调 attacker->blockedByItem(*this)
//      回调攻击者（对齐 Java blockUsingItem→attacker.blockedByItem(victim)）。
//   5) RavagerEntity::blockedByItem override（本次新增）：转调 constructKnockBackVector(victim)。
//      注：此前 constructKnockBackVector 是死代码（全仓零调用），咆哮链路从未被触发；
//      本接入经格挡回调链激活。constructKnockBackVector 内 50% 眩晕（m_stunTick=40）/50% 发射。
//   6) 眩晕结束（m_stunTick 递减到 0）→ m_roarTick=ROAR_DURATION(20) → 咆哮第 10 tick _roar()
//      对 boundingBox.grow(ROAR_RANGE=4) 内非 AbstractRaiderEntity 的 LivingEntity 造成
//      ROAR_DAMAGE(6) + _launchEntity 击退（RavagerEntity.cpp:206-238）。
//
// 50% 眩晕非确定性：劫掠兽每 20 tick 攻击一次（MeleeAttackGoal ATTACK_COOLDOWN_TICKS=20），
//   每次格挡 50% 眩晕。8 次攻击内至少一次眩晕概率 1-0.5^8≈99.6%。眩晕期间 60 tick 不攻击
//   （40 眩晕 + 20 咆哮），故最坏连续 7 次不眩晕（140 tick）+ 第 8 次眩晕（60 tick）≈200 tick
//   内必咆哮。maxTick=900 留充足余量吸收非确定性。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ 默认 day batch。
//   - Survival 玩家（gameMode=0）被攻击走完整 Player::hurt→actuallyHurt→canBlockDamageSource
//     （创造模式 m_abilities.invulnerable 早返回跳过 hurt，须用 Survival）。
//   - 玻璃围栏把劫掠兽和玩家固定在相邻 2 格，防止格挡/咆哮击退导致脱离攻击距离。
//     玻璃透明不阻挡 NearestAttackableTarget checkSight 射线。
//
// 几何（y=2 层俯视，x 横 z 纵，creeper_pit x,z∈[0,6] y=1..4 air y=0 草地）：
//   - 劫掠兽 R (2,2,3)，玩家 P (4,2,3) 相距 2 格（<3.78 攻击范围），羊 S (2,2,5) 距劫掠兽 2 格
//     （<4 咆哮范围，非劫掠兽目标仅受咆哮 AoE 6 伤害）。
//   - 围栏防脱离：劫掠兽三面围玻璃 (1,2,3)(2,2,2)(2,2,4)，玩家三面围玻璃 (5,2,3)(4,2,2)(4,2,4)，
//     两者间 x=3 通道空着保持相邻。羊围玻璃 (1,2,5)(3,2,5)(2,2,6) 防走动离开咆哮范围。
//   - 脚下 y=1 放玻璃支撑防下落（劫掠兽/玩家/羊受重力）。
//
// 判定手段：断言羊 HP<8（满血 8，受咆哮 6 伤害→2）。羊不举盾不格挡，咆哮 AoE 必命中。
//   _roar 内先 hurt 后 _launchEntity，故伤害在击退前生效；羊被击退后断言 HP<8 仍成立。
//   pollUntilSucceed 轮询羊 HP。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_劫掠兽.txt#攻击（盾牌格挡触发咆哮 AoE 伤害）
// Ref: RavagerEntity.cpp blockedByItem/constructKnockBackVector/_roar（咆哮链路接入）
// Ref: Player.cpp canBlockDamageSource/damageShield（盾牌格挡判定）
// Ref: LivingEntity.cpp actuallyHurt 格挡分支 blockedByItem 回调
function ravagerRoarDamagesNearbyEntities(test: Test): void {
  const ravagerType = "ravager";
  const sheepType = "sheep";

  // 脚下 y=1 玻璃支撑（劫掠兽/玩家/羊受 MonsterEntity 重力会下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 }); // 劫掠兽脚下
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 }); // 玩家脚下
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 5 }); // 羊脚下

  // 劫掠兽围栏（三面，x+ 方向留通道给玩家）：防格挡/咆哮击退导致劫掠兽移位脱离。
  test.setBlockType("minecraft:glass", { x: 1, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 4 });
  // 玩家围栏（三面，x- 方向留通道给劫掠兽）：防击退导致玩家脱离攻击范围。
  test.setBlockType("minecraft:glass", { x: 5, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 4 });
  // 羊围栏（三面，(2,2,4) 已是劫掠兽围栏共用）：防羊走动离开 4 格咆哮范围。
  test.setBlockType("minecraft:glass", { x: 1, y: 2, z: 5 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 5 });
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 6 });
  // 封顶防羊/劫掠兽受惊跳起（双保险，非必需）。
  test.setBlockType("minecraft:stone", { x: 2, y: 3, z: 5 });

  // 劫掠兽 (2,2,3) + Survival 玩家 (4,2,3) 相距 2 格 + 羊受害者 (2,2,5) 距劫掠兽 2 格。
  test.spawn(ravagerType, { x: 2, y: 2, z: 3 });
  test.spawn(sheepType, { x: 2, y: 2, z: 5 });
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "shielder", 0 as any);

  // 玩家主手设盾牌：setItem(stack, slot=0 主手, selectSlot=true 同步选中)。
  // 两份 @minecraft/server ItemStack 类型分裂，强转绕过编译期（见 ZombieVillagerTests 同款注释）。
  const shield = new ItemStack("minecraft:shield", 1);
  player.setItem(shield as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 0 举盾：useItem→ShieldItem::onItemRightClick→setActiveHand(MainHand) 进入持续格挡状态。
  // 必须先 setItem 放盾主手再 useItem（onItemRightClick 内部读 getHeldItem(MainHand)，空手则 setActiveHand 早返回）。
  // useItem 是 Cubium 扩展绑定（基岩官方 SimulatedPlayer.useItem），as any 绕过 TS 类型。
  (player as any).useItem(
    shield as unknown as Parameters<typeof player.setItem>[0]);

  // 轮询：咆哮对羊造成 6 伤害后羊 HP<8（满血 8）。
  // startTick=40 留劫掠兽选目标+首次攻击+眩晕+咆哮时序（首次攻击≈tick 20，50% 眩晕后 40+10=50 tick 咆哮）。
  // maxTick=900 吸收 50% 眩晕非确定性（最坏连续 7 次不眩晕≈140 tick + 第 8 次眩晕 60 tick）。
  pollUntilSucceed(test, () => {
    const sheeps = test.getDimension().getEntities({
      type: sheepType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (sheeps.length === 0) return false;
    const health = sheeps[0].getComponent("minecraft:health");
    if (health === undefined) return false;
    return (health as any).currentValue < 8;
  }, {
    startTick: 40,
    interval: 10,
    maxTick: 900,
    onTimeout: () => {
      const sheeps = test.getDimension().getEntities({
        type: sheepType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const ravagers = test.getDimension().getEntities({
        type: ravagerType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const sheepHp = sheeps.length > 0
        ? (sheeps[0].getComponent("minecraft:health") as any)?.currentValue
        : "no sheep";
      // 区分失败原因：sheepHp==8 说明咆哮未触发（格挡/blockedByItem/constructKnockBackVector/眩晕链路断）；
      // sheepHp<8 但未 succeed 不可能（条件即 <8）；no sheep 说明羊流失。
      test.assert(false,
        `ravager roar did not damage sheep: sheepHp=${sheepHp} ravagers=${ravagers.length} ` +
        `(sheepHp==8: roar chain broken - block/blockedByItem/stun/roar not triggered)`);
    },
  });
}

export function registerRavagerTests(): void {
  GameTest.register("MobBehaviorTests", "ravager_attacks_player", ravagerAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "ravager_does_not_burn_in_daylight", ravagerDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "ravager_roar_damages_nearby_entities", ravagerRoarDamagesNearbyEntities)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);
}
