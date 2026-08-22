// 商队羊驼（Trader Llama）行为类 GameTest。
//
// 商队羊驼是 mob_behavior 包中 horse 分类零覆盖实体（LlamaTests 仅测普通羊驼 llama，trader_llama
// 无专属测试）。TraderLlamaEntity.cpp 实现完整：继承 LlamaEntity（含 RangedAttackGoal 吐口水），
// registerGoals 额外加 4 个专属目标（对齐 Java 1.21.11 TraderLlama.registerGoals :58-66，逐段核查一致）：
//   1. PanicGoal(优先级1)——super.registerGoals() 之后 addGoal(1, PanicGoal(2.0))，对齐 vanilla :60；
//   2. TraderLlamaDefendWanderingTraderGoal(优先级1，需被拴在流浪商人上 + 商人受击才触发，测试难构造拴绳)；
//   3. NearestAttackableTargetGoal<ZombieEntity>(优先级2，排除僵尸猪灵)；
//   4. NearestAttackableTargetGoal<AbstractIllagerEntity>(优先级2)。
// 本测试补全其最具辨识度且易测的 vanilla 行为：主动攻击僵尸（对齐 Java 1.21.11 TraderLlama
// registerGoals 的 NearestAttackableTargetGoal<Zombie>，商队羊驼会主动攻击附近僵尸）。
//
// C++ 链路（对齐 Java 1.21.11 TraderLlama.registerGoals）：
//   test.spawn("trader_llama") → TraderLlamaEntity::create() 生成。构造调 registerGoals
//   （TraderLlamaEntity.cpp:64，补调因 AnimalEntity 构造 vtable 不指向派生 override）。
//   registerGoals（TraderLlamaEntity.cpp:140-161）：
//     先 LlamaEntity::registerGoals() 加 RangedAttackGoal(优先级3，吐口水，继承普通羊驼远程攻击) +
//       HurtByTargetGoal(优先级1) + PanicGoal(优先级3) 等；
//     再加 PanicGoal(优先级1)（对齐 vanilla，见下方 panic 机制说明）+
//       NearestAttackableTargetGoal<ZombieEntity>(优先级2, checkSight=true, 谓词排除 ZOMBIFIED_PIGLIN)
//       主动选附近僵尸为 attackTarget。
//   RangedAttackGoal shouldExecute 读 attackTarget(僵尸) 非空 → seenTime 累积 → m_attackTime 倒计 40
//     → performAttack → LlamaEntity::attackEntityWithRangedAttack → _spit 生成 LlamaSpitEntity →
//     口水飞行命中僵尸 onEntityHit→hurt(1.0)。
//
// panic 机制（关键，对齐 vanilla 非 bug）：商队羊驼 registerGoals 在 LlamaEntity::registerGoals()
//   之后又 addGoal(1, PanicGoal(2.0))（对齐 Java TraderLlama.java:60）。该 PanicGoal 优先级1 高于
//   RangedAttackGoal(3)。商队羊驼被僵尸近战攻击后 getLastHurtBy()!=null → PanicGoal(1) 抢占
//   RangedAttackGoal(3) 的 Move flag → RangedAttackGoal 被 reset，商队羊驼只 panic 逃跑不再吐口水。
//   这是 vanilla 设计行为（商队羊驼受击优先逃跑保护货物，区别普通羊驼）。故商队羊驼仅在受击前吐口水：
//   spawn 后 RangedAttackGoal 启动，约 tick 40 首次吐一口口水命中僵尸，随后 tick 60-65 僵尸近身咬
//   商队羊驼触发 panic 中断后续吐口水。普通羊驼测狼（LlamaTests.llamaDefendsAgainstWolf）时狼不主动
//   攻击羊驼故不 panic，羊驼持续吐口水可用 spit 实体判定；商队羊驼单口口水存活仅 1-2 tick（生成即飞行
//   命中 onImpact→remove），succeedWhen 查 spit 实体易错过窗口，故本测试改判僵尸掉血（见判定手段）。
//
// 商队羊驼不消失：m_despawnDelay 默认 47999 tick（约40分钟，TraderLlamaEntity.hpp DEFAULT_DESPAWN_DELAY），
//   测试 maxTicks 远小于此，maybeDespawn 不会触发 discard。test.spawn 经 create() 不走 finalizeSpawn，
//   但 m_despawnDelay 用成员初始值 47999，仍是 47999，无消失风险。
//
// 结构选择：creeper_pit（7×5×7 开放坑）。商队羊驼继承 LlamaEntity 用 RangedAttackGoal 吐口水，
//   开放坑无围墙 canSee 射线通畅 + 投射物飞行无阻挡（同 LlamaTests 范式）。
//   商队羊驼(2,2,3)+僵尸(5,2,3)，水平距 3 格 < RangedAttackGoal 攻击半径 20 + NearestAttackableTargetGoal
//   搜索半径（FOLLOW_RANGE 40）。实体自然下落站 y=1 grass_block 顶部同层。
//
// 判定手段：succeedWhen 每 tick 检查僵尸 HP<20（满血 20）。口水命中僵尸造成 1.0 伤害（20→19），
//   HP<20 是持久状态（僵尸无自然恢复），succeedWhen 必抓到，不依赖口水实体存活窗口。区域限定查僵尸
//   排除并行测试污染；type 用 "minecraft:zombie"（带前缀）。
//
// 时序：NearestAttackableTargetGoal 选僵尸(每 tick，tick 0 起) + RangedAttackGoal seenTime 累积 +
//   首次吐口水 attackTime 倒计 40 tick(约 tick 40) + 口水飞行命中(~tick 41-42)。HP<20 约 tick 42 成立。
//   maxTicks=2000 留充裕余量。
//
// 僵尸反击：僵尸被口水击中后 HurtByTargetGoal(alertAllies=true) 反击商队羊驼，tick 60-65 近身咬触发
//   商队羊驼 panic（vanilla 行为，见 panic 机制）。但此时口水已命中僵尸 HP<20 判定已满足测试 succeed。
//   商队羊驼后续死亡不影响判定。day 批次僵尸不燃烧（creeper_pit 无 skyAccess 配置；即便燃烧 HP<20
//   在燃烧致死前成立）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: TraderLlamaEntity.cpp:140-161（registerGoals NearestAttackableTargetGoal<ZombieEntity> + PanicGoal(1)）
// Ref: Java 1.21.11 TraderLlama.java:58-60（registerGoals super + addGoal(1, PanicGoal)，Cubium 对齐）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_羊驼.txt#行为（吐口水远程攻击，商队羊驼继承）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 商队羊驼主动攻击附近僵尸（wiki tech_羊驼.txt + Java TraderLlama.registerGoals：商队羊驼主动攻击僵尸，
//   排除僵尸猪灵）。
//
// C++ 链路（对齐 Java 1.21.11 TraderLlama.registerGoals，已逐段核查与 vanilla 100% 一致）：
//   test.spawn("trader_llama") → TraderLlamaEntity::create() 生成。构造调 registerGoals
//   （TraderLlamaEntity.cpp:64，补调因 AnimalEntity 构造 vtable 不指向派生 override）。
//   registerGoals（TraderLlamaEntity.cpp:140-161）：
//     先 LlamaEntity::registerGoals()（含 RangedAttackGoal(优先级3) 吐口水 + PanicGoal(优先级3)），
//     再加 NearestAttackableTargetGoal<ZombieEntity>(优先级2, 谓词排除 ZOMBIFIED_PIGLIN) 主动选僵尸。
//   NearestAttackableTargetGoal<ZombieEntity> shouldExecute 选最近僵尸设 attackTarget →
//     RangedAttackGoal(优先级3) seenTime 累积 + m_attackTime 倒计 40 → performAttack →
//     LlamaEntity::_spit 生成 LlamaSpitEntity → 口水飞行命中僵尸 onEntityHit→hurt(1.0)。
//
// 判定手段：succeedWhen 每 tick 检查僵尸 HP < 20（满血）。口水命中僵尸造成 1.0 伤害（20→19），
//   HP<20 是持久状态（僵尸无自然恢复），succeedWhen 必抓到。
//
// 为何不判 llama_spit 实体存在（区别普通羊驼 llamaDefendsAgainstWolf 用 spit 实体判定）：
//   商队羊驼 registerGoals 在 LlamaEntity::registerGoals() 之后又 addGoal(1, PanicGoal(2.0))（对齐
//   vanilla TraderLlama.registerGoals :60，非 bug）。该 PanicGoal 优先级1高于 RangedAttackGoal(3)。
//   商队羊驼被僵尸近战攻击后 getLastHurtBy()!=null → PanicGoal(1) 抢占 RangedAttackGoal(3) 的 Move
//   flag → RangedAttackGoal 被 reset，商队羊驼只 panic 逃跑不再吐口水。这是 vanilla 设计行为
//   （商队羊驼受击优先逃跑保护货物，与普通羊驼不同——普通羊驼测狼时狼不主动攻击羊驼故不 panic 持续吐口水）。
//   因此商队羊驼仅在受击前吐口水：spawn 后 RangedAttackGoal 启动，约 tick 40 首次 performAttack 吐
//   一口口水命中僵尸，随后 tick 60-65 僵尸近身咬商队羊驼触发 panic 中断后续吐口水。
//   单口口水存活仅 1-2 tick（生成即飞行命中 onImpact→remove），succeedWhen 查 spit 实体易错过该窗口；
//   故改判僵尸 HP<20（口水命中的持久伤害效果），稳定可靠。
//
// 时序：NearestAttackableTargetGoal 选僵尸(每 tick，tick 0 起) + RangedAttackGoal seenTime 累积 +
//   首次吐口水 attackTime 倒计 40 tick(约 tick 40) + 口水飞行命中(~tick 41-42)。HP<20 约 tick 42 成立。
//   maxTicks=2000 留充裕余量。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。商队羊驼(2,2,3)+僵尸(5,2,3)，水平距 3 格 < RangedAttackGoal
//   攻击半径 20 + NearestAttackableTargetGoal 搜索半径（FOLLOW_RANGE 40）。实体自然下落站 y=1
//   grass_block 顶部同层。开放坑无围墙 canSee 射线通畅 + 投射物飞行无阻挡。
//
// 僵尸反击：僵尸被口水击中后 HurtByTargetGoal(alertAllies=true) 反击商队羊驼，tick 60-65 近身咬触发
//   商队羊驼 panic。但此时口水已命中僵尸 HP<20 判定已满足，测试 succeed。商队羊驼后续死亡不影响判定。
//   creeper_pit 无 skyAccess 配置，day 批次僵尸不燃烧（即便燃烧 HP<20 在燃烧致死前成立）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: TraderLlamaEntity.cpp:140-161（registerGoals NearestAttackableTargetGoal<ZombieEntity> + PanicGoal(1)）
// Ref: Java 1.21.11 TraderLlama.java:58-60（registerGoals super + addGoal(1, PanicGoal)，Cubium 对齐）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_羊驼.txt#行为（吐口水远程攻击，商队羊驼继承）
function traderLlamaAttacksZombie(test: Test): void {
  const traderLlamaType = "trader_llama";
  const zombieType = "zombie";
  // 商队羊驼(2,2,3) + 僵尸(5,2,3)，水平距 3 格。实体自然下落站 grass_block 顶部同层。
  const traderLlama = test.spawn(traderLlamaType, { x: 2, y: 2, z: 3 });
  const zombie = test.spawn(zombieType, { x: 5, y: 2, z: 3 });
  void traderLlama;
  void zombie;

  // 断言僵尸被口水击中掉血（HP<20，满血 20）。口水命中造成 1.0 伤害，HP<20 持久成立（僵尸无自然恢复）。
  // 区域限定查僵尸排除并行测试污染；type 用 "minecraft:zombie"（带前缀）。
  test.succeedWhen(() => {
    const zombies = test.getDimension().getEntities({
      type: "minecraft:zombie",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(zombies.length > 0, "zombie disappeared before trader_llama spat");
    const health = zombies[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "zombie has no health component");
    test.assert((health as any).currentValue < 20,
      `trader_llama did not damage zombie, zombie hp=${(health as any).currentValue} (expected <20)`);
  });
}

export function registerTraderLlamaTests(): void {
  GameTest.register("MobBehaviorTests", "trader_llama_attacks_zombie", traderLlamaAttacksZombie)
    .structureName("gametests:creeper_pit")
    .maxTicks(2000);
}
