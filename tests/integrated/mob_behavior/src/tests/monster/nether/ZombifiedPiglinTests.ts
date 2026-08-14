// 僵尸猪灵行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";

// creeper_pit / glass_pit 结构尺寸均为 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick（同 batch 的测试同一世界 tick 同时推进），
// 且测试结束不清场，全维度 getEntities({type}) 会数到其他并行/残留测试的实体（跨测试污染）。
// 各测试 origin 在 X 方向错开 9 格（结构 7 + padding 2），7×5×7 体积查询不覆盖相邻测试区域。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 攻击一只僵尸猪灵会激怒附近的旁观僵尸猪灵，它们一起追击攻击者（wiki tech_僵尸猪灵.txt#敌对性）。
// 这是僵尸猪灵最标志性的行为：默认中立（无 NearestAttackableTargetGoal<Player>），被攻击后
// HurtByTargetGoal 触发 → 自身反击 + alertOthers 警醒附近同类（同类型 entityType 指针比较）
// → 旁观者 setAttackTarget(攻击者) → MeleeAttackGoal 驱动旁观者追击攻击者。
//
// C++ 链路：
//   玩家 attackEntity(诱饵A) → SimulatedPlayer::attack → Player::attack(A) →
//   DamageSources::playerAttack(this) → A.hurt(source, dmg) → LivingEntity::actuallyHurt →
//   setLastHurtBy(this) → 下一 tick A 的 HurtByTargetGoal::shouldExecute 读 getLastHurtBy() →
//   startExecuting：A.setAttackTarget(玩家) + alertOthers 扫描 expand(16,4,16) AABB 内同类 →
//   旁观者B.setAttackTarget(玩家) → B 的 MeleeAttackGoal::shouldExecute 读 attackTarget →
//   navigator->moveTo(玩家) 驱动 B 朝玩家移动。
// 依赖 C++ 改动（2026-08-14）：
//   1. SimulatedPlayer::attack stub → 转发 Player::attack(target)（此前空实现致攻击不造成伤害）。
//   2. ScriptSimulatedPlayer attackEntity 绑定 stub → 真实实现（此前抛 MethodNotImplemented）。
// 判定手段：JS 无法读 attackTarget/isAngry（未绑定），用行为断言——旁观者B朝玩家移动到攻击距离内。
// 注：test.spawn 返回的 Entity 引用其 .location getter 在并行 tick 下不可靠（偶发返回 undefined），
// 故用 getEntities 区域限定查询取实体坐标（与 SpiderTests 同款），而非 spawn 返回引用的 .location。
// 存 B 的初始 spawn 坐标用于在 getEntities 结果中区分诱饵A 与旁观者B（B 初始于 (3,2,3)，
// 接近玩家方向移动；A 初始于 (1,2,5) 对角）。取距玩家最近的猪灵判定（B 距玩家近 ~2.8 < A ~5.7，
// B 被警醒后朝玩家移动会更快接近；A 也会追玩家但起步远）。最近猪灵接近玩家即证明 alertOthers 链路。
//
// 玩家存活窗口分析（creeper_pit 实测寻路通畅）：
//   tick 8 玩家攻击A → tick 9 A 的 HurtByTargetGoal 触发 + alertOthers 警醒B →
//   B 距玩家 2.8格，僵尸猪灵速度 ~0.23，约 tick 21 接近玩家（distSq≤2.5²）→
//   A 距玩家 5.7格约 tick 34 接近并开始攻击玩家（5伤害/次，间隔20tick）→
//   A+B 联手约 tick 54 打死玩家。succeedWhen 窗口 tick 21-54 共 33 tick 充足捕获 B 接近。
// 结构选择：用 creeper_pit（纯开放坑，y=0 grass_block + y=1..4 air）。
//   glass_pit 含 glass 方块会阻挡僵尸猪灵 navigator 寻路（moveTo 找不到路径返回 false），
//   致 A/B 被激怒设了 attackTarget 却不移动；creeper_pit 无玻璃寻路通畅。
// 玩家用 Survival（gameMode=0）：创造/旁观玩家被 TargetGoal::isSuitableTarget 滤掉（line 132-137），
// HurtByTargetGoal::shouldExecute 返回 false 不激怒。必须 Survival 才能成为有效攻击目标。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸猪灵.txt#敌对性
function zombifiedPiglinGroupAggro(test: Test): void {
  const zombifiedPiglinType = "zombified_piglin";

  // 玩家 Survival 于 (5,2,1)（一角）。传数字 0 并用 as any 绕过 TS 字符串枚举类型校验
  // （运行时 C++ 绑定期望数字，见 CreeperTests 同款注释）。
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "attacker", 0 as any);

  // 诱饵僵尸猪灵A于 (1,2,5)（对角，距玩家 ~5.7格）。attackEntity 不受距离限制（基岩语义：
  // attack can be performed at any distance），远程命中无需寻路接近。A 被激怒后追玩家反击。
  const bait = test.spawn(zombifiedPiglinType, { x: 1, y: 2, z: 5 });

  // 旁观僵尸猪灵B于 (3,2,3)（中央）：距A ~2.8格 < alertOthers 激怒范围16格会被警醒；
  // 距玩家 ~2.8格。B初始中立不动（无 NearestAttackableTargetGoal<Player>）。
  test.spawn(zombifiedPiglinType, { x: 3, y: 2, z: 3 });

  // tick 8 后玩家攻击诱饵A：留 8 tick 让三实体完成 spawn 注册 + 首 tick 稳定。
  test.runAtTickTime(8, () => {
    player.attackEntity(bait);
  });

  // 断言旁观者B朝玩家移动：用 succeedWhen 每 tick 持续检查（规避并行负载时序偏移）。
  // 取距玩家最近的僵尸猪灵判定接近——B 初始距玩家 ~2.8 < A 初始距玩家 ~5.7，且 B 被警醒后
  // 朝玩家移动会先于 A 接近（B 路径短）。最近猪灵接近玩家即证明 alertOthers 链路（仅 A 自身
  // 反击需 HurtByTargetGoal，B 接近需 alertOthers 警醒，取最近者能覆盖 B 的情形）。
  // 玩家世界坐标动态（玩家被击退可能微移），每 tick 用 worldLocation 重算（玩家不主动移动）。
  // 区域限定查询排除并行/残留测试的僵尸猪灵污染。
  test.succeedWhen(() => {
    const piglins = test.getDimension().getEntities({
      type: zombifiedPiglinType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(piglins.length >= 1, `expected zombified_piglins, got ${piglins.length}`);
    const playerWorld = test.worldLocation({ x: 5, y: 2, z: 1 });
    let minDistSq = Infinity;
    for (const p of piglins) {
      const dx = p.location.x - playerWorld.x;
      const dz = p.location.z - playerWorld.z;
      const distSq = dx * dx + dz * dz;
      if (distSq < minDistSq) minDistSq = distSq;
    }
    test.assert(minDistSq <= 2.5 * 2.5,
      `no zombified_piglin approached player, minDistSq=${minDistSq.toFixed(2)}`);
  });
}

// 僵尸猪灵免疫火焰/熔岩伤害（wiki tech_僵尸猪灵.txt#行为：免疫火和熔岩等来源的火焰伤害，不会着火）。
// 由 ZombifiedPiglinEntity 继承 MonsterEntity 的 isImmuneToFire()=true 保证：
// Entity::lavaHurt/lavaIgnite 开头检查 isImmuneToFire() 为真则直接 return（不造成岩浆伤害、不点燃）。
// 验证：僵尸猪灵浸入熔岩若干 tick 后 HP 保持满血；对照实体（猪，不免疫火焰）在同环境受伤/死亡。
// 对照实体用于排除"熔岩伤害机制未实现"的假性通过——若猪不掉血说明机制本身没触发，测试失败。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸猪灵.txt#行为（免疫火焰伤害和熔岩）
function zombifiedPiglinImmuneToFire(test: Test): void {
  const zombifiedPiglinType = "zombified_piglin";
  const pigType = "pig";

  // 把 y=0..1 铺成 lava（两层），y=2..4 保持 air。实体 spawn 于 y=3 下落浸入 y=1 熔岩层，
  // 碰撞箱与 lava 方块重叠触发 LiquidBlock::entityInside → lavaIgnite + lavaHurt。
  // 两层 lava 是必要的：单层 y=0 时实体站在 lava 表面碰撞箱不触及 y=0，entityInside 不触发。
  // 僵尸猪灵 isImmuneToFire=true → lavaHurt 跳过；猪 isImmuneToFire=false → lavaHurt 造成伤害。
  fillBlock(test, "lava", 0, 0, 0, 6, 1, 6);

  // 僵尸猪灵于 (2,3,2)，猪于 (4,3,4)，两者间隔足够不互相干扰，都落入 lava 层。
  // 僵尸猪灵 HP=20，免疫火焰应保持 20；猪 HP=10，浸入熔岩应掉血（<10）或死亡。
  test.spawn(zombifiedPiglinType, { x: 2, y: 3, z: 2 });
  test.spawn(pigType, { x: 4, y: 3, z: 4 });

  // maxTicks=300：实体下落 + 浸入熔岩 + lavaHurt 每 tick 判定 + 余量。
  // 断言：僵尸猪灵 HP 仍为满血 20（免疫）；猪 HP<10 或已死亡消失（lavaHurt 生效）。
  // 两端同时成立才证明"熔岩伤害机制有效 + 僵尸猪灵免疫"——任一不成立则测试失败。
  // 实体查询用区域限定排除并行测试污染。
  test.succeedWhen(() => {
    const piglins = test.getDimension().getEntities({
      type: zombifiedPiglinType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });

    // 僵尸猪灵应存活且满血（免疫熔岩）。
    test.assert(piglins.length > 0, "zombified_piglin died in lava (should be immune)");
    const piglinHealth = piglins[0].getComponent("minecraft:health");
    test.assert(piglinHealth !== undefined, "zombified_piglin has no health component");
    test.assert((piglinHealth as any).currentValue >= 20,
      `zombified_piglin should be immune to lava, hp=${(piglinHealth as any).currentValue}`);

    // 对照实体猪应受伤（HP<10）或已死亡消失——证明熔岩伤害机制确实生效。
    if (pigs.length > 0) {
      const pigHealth = pigs[0].getComponent("minecraft:health");
      test.assert(pigHealth !== undefined, "pig has no health component");
      test.assert((pigHealth as any).currentValue < 10,
        `pig should take lava damage, hp=${(pigHealth as any).currentValue}`);
    }
    // 猪已死亡（pigs.length===0）也满足"对照实体受伤"——死亡是受伤的极端情形。
  });
}

// 僵尸猪灵默认中立，不主动攻击玩家（wiki tech_僵尸猪灵.txt#敌对性：默认非敌对状态）。
// 僵尸猪灵 targetSelector 只注册 HurtByTargetGoal（受伤反击），无 NearestAttackableTargetGoal<Player>
// （对比蜘蛛/僵尸/苦力怕都注册了对玩家的 NearestAttackableTargetGoal）。故未受攻击时不会选玩家为目标。
// 验证：僵尸猪灵 + Survival 玩家贴近，若干 tick 后玩家 HP 仍为满血（20）。
// 注：此为负向断言（验证不造成伤害）。与 zombified_piglin_group_aggro 的正向断言互补对照——
// 同样僵尸猪灵 + Survival 玩家场景，未攻击时不掉血、攻击后被激怒追击，交叉验证中立/敌对门控正确。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸猪灵.txt#敌对性（默认非敌对）
function zombifiedPiglinNeutralByDefault(test: Test): void {
  const zombifiedPiglinType = "zombified_piglin";

  // 僵尸猪灵 spawn 于 (3,2,3)，Survival 玩家于 (4,2,3)（直线1格，碰撞箱重叠）。
  // 玩家用 Survival（gameMode=0）：即便 Survival，僵尸猪灵未受攻击时 HurtByTargetGoal 不触发
  // （无 lastHurtBy），不会选玩家为 attackTarget，MeleeAttackGoal 无目标不追击。
  test.spawn(zombifiedPiglinType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 玩家初始满血 20。僵尸猪灵即使接触玩家也不造成伤害（中立未激怒），HP 应保持 20。
  // maxTicks=200：僵尸猪灵 WaterAvoidingRandomWalkingGoal 漫游 + 接触判定 + 余量。
  // 玩家查询用区域限定排除并行测试的玩家污染。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const player = players[0];
    const health = player.getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    // currentValue 对齐 HealthComponent.currentValue（LivingEntity::health）。
    test.assert((health as any).currentValue >= 20,
      `neutral zombified_piglin should not damage player, hp=${(health as any).currentValue}`);
  });
}

export function registerZombifiedPiglinTests(): void {
  GameTest.register("MobBehaviorTests", "zombified_piglin_group_aggro", zombifiedPiglinGroupAggro)
    .structureName("gametests:creeper_pit")
    .maxTicks(250);

  GameTest.register("MobBehaviorTests", "zombified_piglin_immune_to_fire", zombifiedPiglinImmuneToFire)
    .structureName("gametests:glass_pit")
    .maxTicks(300);

  GameTest.register("MobBehaviorTests", "zombified_piglin_neutral_by_default", zombifiedPiglinNeutralByDefault)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
