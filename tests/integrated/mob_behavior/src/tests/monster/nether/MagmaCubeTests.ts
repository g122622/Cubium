// 岩浆怪行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick（同 batch 的测试同一世界 tick 同时推进），
// 且测试结束不清场，全维度 getEntities({type}) 会数到其他并行/残留测试的实体（跨测试污染）。
// 各测试 origin 在 X 方向错开 9 格（结构 7 + padding 2），7×5×7 体积查询不覆盖相邻测试区域。
const GLASS_PIT_FROM = { x: 0, y: 0, z: 0 };
const GLASS_PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 大型岩浆怪死亡时分裂出 2-4 只中型岩浆怪（wiki tech_岩浆怪.txt#行为）。
// 尺寸 4（大型）死亡 → performSplit 生成 2-4 只尺寸 2（中型），新尺寸 = 原尺寸 / 2 向下取整。
// 与史莱姆分裂同源（MagmaCubeEntity 继承 SlimeEntity::performSplit），但分裂体必须是岩浆怪而非史莱姆。
// C++ 链路：LivingEntity::onKillCommand（killEntity 设施）→ 死亡链路 tickDeath deathTime>=20 →
// remove()（虚分派到 SlimeEntity::remove）→ canSplit()(size>1) → performSplit()。
// performSplit 此前硬编码 entity::EntityTypeKeys::SLIME 创建分裂体，致岩浆怪分裂出" typeId 为
// magma_cube 但 C++ 对象为 SlimeEntity "的混乱态；2026-08-14 改用 entityType() 取当前实体类型，
// 岩浆怪分裂出真正的 MagmaCubeEntity（对齐 Java Slime.remove 的 convertTo(本类 EntityType)）。
// 依赖 C++ 改动：
//   1. GameTestHelper::spawn 解析 <spawnEvent> 后缀：magma_cube<minecraft:spawn_large> →
//      applySpawnEvent 调 setSlimeSize(4) 生成大型岩浆怪（岩浆怪 override setSlimeSize 额外设护甲=size*3）。
//   2. GameTestHelper::killEntity（JS kill）：走 onKillCommand 死亡链路触发 performSplit。
//   3. SlimeEntity::performSplit 用 entityType() 而非硬编码 SLIME，岩浆怪分裂出岩浆怪。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_岩浆怪.txt#行为（尺寸大于1的岩浆怪死亡时分裂）
function magmaCubeLargeSplitsOnDeath(test: Test): void {
  const magmaCubeType = "magma_cube";

  // 大型岩浆怪尺寸 4，碰撞箱 2.08×2.08×2.08，glass_pit 7×5×7 空间充足。
  // spawn 于 (3,2,3)；<minecraft:spawn_large> 经 applySpawnEvent 设尺寸 4（HP=16，护甲=12）。
  const magmaCube = test.spawn(`${magmaCubeType}<minecraft:spawn_large>`, { x: 3, y: 2, z: 3 });

  // tick 5 杀死大岩浆怪：onKillCommand 虚空伤害致死 → die → deathTime 倒计时。
  // test.kill 是项目测试设施（基岩 Test 类无），TS 类型未声明，用 as any 绕过。
  (test as any).kill(magmaCube);

  // 大岩浆怪死亡后约 20 tick（deathTime 倒计时）调 remove() → performSplit 生成分裂体。
  // 用 succeedWhen 持续检查（而非 runAtTickTime 单点）——并行测试负载下死亡链路时序会偏移，
  // 单点 tick 30 可能早于分裂体生成（got 0），succeedWhen 每 tick 检查更稳健。
  // 区域内 magma_cube 数量演变：spawn 前=0；kill 后大岩浆怪仍活=1；约 tick25 死亡分裂后=2-4。
  // 检查 [2,4] 自然排除大岩浆怪存活期（=1）与 spawn 前空窗（=0），只在分裂完成后满足。
  // 区域限定（location+volume）只数本测试 7×5×7 区域内的岩浆怪，排除并行/残留测试污染。
  test.succeedWhen(() => {
    const magmaCubes = test.getDimension().getEntities({
      type: magmaCubeType,
      location: test.worldLocation(GLASS_PIT_FROM),
      volume: GLASS_PIT_VOLUME,
    });
    test.assert(magmaCubes.length >= 2 && magmaCubes.length <= 4,
      `expected 2-4 split magma_cubes, got ${magmaCubes.length}`);
  });
}

// 小型岩浆怪（尺寸 1）仍能伤害玩家（wiki tech_岩浆怪.txt#行为）——这是岩浆怪与史莱姆的关键差异：
// 小型史莱姆没有攻击能力（SlimeEntity::canDamagePlayer 返回 m_size>1，size=1 为假），
// 而小型岩浆怪 canDamagePlayer override 始终为真（不限尺寸），伤害 = getAttackDamage() = 属性值+2 = 3。
// 验证：小型岩浆怪 + Survival 玩家贴近，若干 tick 后玩家 HP 下降（< 20）。
// C++ 链路：Player::checkEntityCollisions 每 tick 扫描附近实体调 onCollideWithPlayer(Player&) →
// SlimeEntity::onCollideWithPlayer → dealDamage → canDamagePlayer()(岩浆怪 true) →
// getAttackDamage()(属性值+2) → target.hurt(mobAttack, 3)。
// 依赖 C++ 改动（2026-08-14）：SlimeEntity::onCollideWithPlayer 签名由 LivingEntity& 改 Player&
// override 基类（此前签名不匹配 + using 引入基类空实现致碰撞伤害全链路死代码）；
// dealDamage 去掉 m_size<=1 return 改用 canDamagePlayer 门控 + getAttackDamage 伤害值。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_岩浆怪.txt#行为（小型岩浆怪近战伤害3）
function magmaCubeSmallCanDamagePlayer(test: Test): void {
  const magmaCubeType = "magma_cube";

  // 小型岩浆怪尺寸 1 碰撞箱 0.52，伤害 3。
  // 岩浆怪 spawn 于 (3,2,3)，Survival 玩家于 (4,2,3)（直线 1 格，碰撞箱重叠触发 checkEntityCollisions）。
  // 玩家用 Survival（gameMode=0）：创造玩家被 TargetGoal 滤掉，岩浆怪不选目标也不碰撞伤害。
  // 传数字 0 并用 as any 绕过 TS 字符串枚举类型校验（运行时 C++ 绑定期望数字，见 CreeperTests 同款注释）。
  test.spawn(magmaCubeType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 玩家初始满血 20。小型岩浆怪碰撞应造成 3 点伤害，HP 应降至 17（受击后伤害免疫约 0.5s 一次）。
  // maxTicks=200：岩浆怪 SlimeHopGoal 跳跃接近 + 碰撞判定 + 余量。
  // 此为正向断言（验证确实造成伤害），与 slime_small_cannot_damage_player 的负向断言互补对照：
  // 同样小型 size=1 + Survival 玩家贴近场景，史莱姆不掉血、岩浆怪掉血，交叉验证尺寸/类型门控正确。
  // 玩家查询用区域限定排除并行测试的玩家污染（每个测试 spawn 的 SimulatedPlayer 名字不同但都是 player 类型）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(GLASS_PIT_FROM),
      volume: GLASS_PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const player = players[0];
    const health = player.getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    // currentValue 对齐 HealthComponent.currentValue（LivingEntity::health）。
    // 玩家应已受伤（HP < 20）。允许一定波动（多次碰撞），只要 < 20 即证明造成了伤害。
    test.assert((health as any).currentValue < 20,
      `small magma_cube should damage player, hp=${(health as any).currentValue}`);
  });
}

// 岩浆怪免疫火焰/熔岩伤害（wiki tech_岩浆怪.txt#行为：免疫火焰伤害，不会被熔岩伤害）。
// 由 MagmaCubeEntity::isImmuneToFire() override 返回 true 保证：Entity::lavaHurt/lavaIgnite 开头
// 检查 isImmuneToFire() 为真则直接 return（不造成岩浆伤害、不点燃）。
// 验证：岩浆怪浸入熔岩若干 tick 后 HP 保持满血；对照实体（猪，不免疫火焰）在同环境受伤/死亡。
// 对照实体用于排除"熔岩伤害机制未实现"的假性通过——若猪不掉血说明机制本身没触发，测试失败。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_岩浆怪.txt#行为（免疫火焰伤害和摔落伤害）
function magmaCubeImmuneToFire(test: Test): void {
  const magmaCubeType = "magma_cube";
  const pigType = "pig";

  // 把 y=0..1 铺成 lava（两层），y=2..4 保持 air。实体 spawn 于 y=3 下落浸入 y=1 熔岩层，
  // 碰撞箱与 lava 方块重叠触发 LiquidBlock::entityInside → lavaIgnite + lavaHurt。
  // 两层 lava 是必要的：单层 y=0 时实体站在 lava 表面碰撞箱不触及 y=0，entityInside 不触发。
  // 岩浆怪 isImmuneToFire=true → lavaHurt 跳过；猪 isImmuneToFire=false → lavaHurt 造成 4 点伤害/次。
  fillBlock(test, "lava", 0, 0, 0, 6, 1, 6);

  // 岩浆怪于 (2,3,2)，猪于 (4,3,4)，两者间隔足够不互相干扰，都落入 lava 层。
  // 大型岩浆怪 HP=16，免疫火焰应保持 16；猪 HP=10，浸入熔岩应掉血（< 10）或死亡。
  test.spawn(`${magmaCubeType}<minecraft:spawn_large>`, { x: 2, y: 3, z: 2 });
  test.spawn(pigType, { x: 4, y: 3, z: 4 });

  // maxTicks=300：实体下落 + 浸入熔岩 + lavaHurt 每 tick 判定 + 余量。
  // 断言：岩浆怪 HP 仍为满血 16（免疫）；猪 HP < 10 或已消失（lavaHurt 生效）。
  // 两端同时成立才证明"熔岩伤害机制有效 + 岩浆怪免疫"——任一不成立则测试失败。
  // 实体查询用区域限定排除并行测试污染。
  test.succeedWhen(() => {
    const magmaCubes = test.getDimension().getEntities({
      type: magmaCubeType,
      location: test.worldLocation(GLASS_PIT_FROM),
      volume: GLASS_PIT_VOLUME,
    });
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(GLASS_PIT_FROM),
      volume: GLASS_PIT_VOLUME,
    });

    // 岩浆怪应存活且满血（免疫熔岩）。
    test.assert(magmaCubes.length > 0, "magma_cube died in lava (should be immune)");
    const magmaHealth = magmaCubes[0].getComponent("minecraft:health");
    test.assert(magmaHealth !== undefined, "magma_cube has no health component");
    test.assert((magmaHealth as any).currentValue >= 16,
      `magma_cube should be immune to lava, hp=${(magmaHealth as any).currentValue}`);

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

export function registerMagmaCubeTests(): void {
  GameTest.register("MobBehaviorTests", "magmacube_large_splits_on_death", magmaCubeLargeSplitsOnDeath)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "magmacube_small_can_damage_player", magmaCubeSmallCanDamagePlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "magmacube_immune_to_fire", magmaCubeImmuneToFire)
    .structureName("gametests:glass_pit")
    .maxTicks(300);
}
