// 美西螈行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 美西螈主动攻击鱿鱼（wiki tech_美西螈.txt#攻击：美西螈只会主动攻击鱼、鱿鱼、发光鱿鱼、蝌蚪、
// 溺尸、守卫者和远古守卫者，每次攻击造成 2 点伤害，且优先攻击其中的敌对生物）。
//
// C++ 链路：AxolotlEntity : WaterMobEntity。registerGoals（AxolotlEntity.cpp:171-215）：
//   targetSelector 优先级2：AxolotlTargetGoal（继承 NearestAttackableTargetGoal<LivingEntity>，
//     checkSight=true, chance=10）。AxolotlTargetGoal 谓词（AxolotlGoals.cpp:91-123）始终攻击
//     DROWNED/GUARDIAN/ELDER_GUARDIAN，无狩猎冷却时攻击 TROPICAL_FISH/PUFFERFISH/SALMON/COD/SQUID
//     （SQUID 匹配在 :115）。装死时不选目标（:128-130）。
//   goalSelector 优先级4：MeleeAttackGoal(this, 1.5, true)——读 attackTarget 寻路接近后
//     attackEntityAsMob→hurt(鱿鱼, ATTACK_DAMAGE=2.0)。
//   AxolotlEntity::registerAttributes: ATTACK_DAMAGE=2.0（AxolotlEntity.cpp:227）。
//   AxolotlEntity::hurt 装死分支需 isInWater()（:230-252）——陆地 false，不装死。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block + y=1..4 air，陆地无水）。关键约束：
//   1. 陆地无水——美西螈 hurt 中 isInWater()=false 跳过装死分支（装死需水中 33% 概率，陆地不触发），
//      确保美西螈持续主动攻击而非装死静止。
//   2. 美西螈 maxAir=6000（远高于鱿鱼 300），测试窗口内不窒息，干扰排除。
//   3. 美西螈 navigator 是 MobEntity 默认 GroundPathNavigation（WalkNodeProcessor），陆地可寻路。
//   4. 鱿鱼陆地"无法移动"（wiki 语义），位置固定，美西螈寻路接近确定。
// 结构放置 +1 抬升：helper-y=N → 结构内 y=N-1。美西螈 (2,2,3)、鱿鱼 (4,2,3)，水平距 2 格，
// 脚踩结构内 y=0 grass_block（helper-y=2→结构内 y=1 空气）。
//
// 判定手段：succeedWhen 每 tick 检查鱿鱼 health.currentValue < 10（满血 10，美西螈一击 2.0→8）
// 或鱿鱼已死亡消失（被多击杀死）。区域限定 getEntities 取鱿鱼读 health 组件，排除并行测试污染。
// **关键时序约束：maxTicks=300 < 320**。鱿鱼陆地窒息首次伤害在第 ~320 tick（air 300→0 耗 300 tick
// + 0→-20 再 20 tick，见 SquidTests 同款 WaterMobEntity::updateAirSupply 链路）。maxTicks=300 确保
// 测试窗口内鱿鱼窒息尚未触发，掉血只能来自美西螈攻击——排除窒息干扰，断言纯粹验证 AxolotlTargetGoal
// 主动攻击鱿鱼链路。美西螈 MOVEMENT_SPEED=1.0，2 格距离接近 + AxolotlTargetGoal chance=10 + 攻击冷却，
// 首击应在 ~50 tick 内，300 tick 余量充足吸收非确定性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_美西螈.txt#攻击（主动攻击鱿鱼）
function axolotlAttacksSquid(test: Test): void {
  const axolotlType = "axolotl";
  const squidType = "squid";

  // 美西螈 (2,2,3)、鱿鱼 (4,2,3)，水平距 2 格。陆地无水（creeper_pit 开放坑 grass_block 地板）。
  // 近距 2 格确保美西螈 AxolotlTargetGoal 选鱿鱼后 MeleeAttackGoal 快速接近命中。
  // helper-y=2 → 结构内 y=1 空气，脚踩结构内 y=0 grass_block。
  test.spawn(axolotlType, { x: 2, y: 2, z: 3 });
  test.spawn(squidType, { x: 4, y: 2, z: 3 });

  // 断言鱿鱼被美西螈攻击掉血或死亡：succeedWhen 每 tick 检查鱿鱼 HP<10 或已消失。
  // 时序：AxolotlTargetGoal 选鱿鱼(chance=10tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(2.0)。
  // 美西螈 1.0 速度接近 2 格约需 20-40 tick，首击应在 ~50 tick 内。
  // maxTicks=300 < 320 窒息线——鱿鱼窒息尚未触发，掉血必来自美西螈攻击（排除窒息干扰）。
  // 鱿鱼可能被多击杀死消失，length==0 也算通过（已受攻击死亡）。
  test.succeedWhen(() => {
    const squids = test.getDimension().getEntities({
      type: squidType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 鱿鱼已死亡消失（被美西螈打死）——攻击行为生效。
    if (squids.length === 0) {
      return;
    }
    const health = squids[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "squid has no health component");
    test.assert((health as any).currentValue < 10,
      `axolotl did not attack squid, hp=${(health as any).currentValue}`);
  });
}

// 美西螈在水中受击触发装死并给予自身再生 I（wiki tech_美西螈.txt#装死：美西螈在水中受到伤害时
// 有概率装死，装死期间获得再生效果恢复生命）。
//
// C++ 链路（对齐 Java 1.21.11 Axolotl，已逐段核查）：
//   1) 玩家 attackEntity(美西螈) → Player 攻击 → AxolotlEntity::hurt（AxolotlEntity.cpp:231-253）。
//      hurt 调 WaterMobEntity::hurt 处理伤害，随后装死分支（:236）：
//        result && isInWater() && !isPlayingDead() && health()>0 && amount<health()
//        && getRandom().nextInt(3)==0（33% 概率） && source.getEntity()!=null（攻击者存在）
//        → setPlayingDead(true)（:246，设 m_playingDead=true + m_playingDeadTimer=PLAY_DEAD_DURATION=200）。
//      关键：amount<health() 要求伤害不致死。玩家空手伤害 1.0 < 美西螈满血 14，满足。
//   2) 下一 tick AxolotlPlayDeadGoal::shouldExecute（AxolotlGoals.cpp:53-57，isPlayingDead()&&isInWater()）
//      → true → startExecuting（:64-74）→ addEffect(Regeneration, 200)（:73，给予自身再生 I 200 tick）。
//   3) AxolotlEntity::tick（:158-170）调 _updatePlayingDead（:255-263）每 tick 递减 m_playingDeadTimer，
//      归零（200 tick 后）m_playingDead=false 装死结束。再生效果 duration 200 tick 独立递减。
//
// 环境选择：creeper_pit（7×5×7）。装死需 isInWater()，故构造水池：用玻璃围栏围 3×3×4 内空区域
//   (2..4, 1..4, 2..4)，围栏内 (3, 1..3, 3) 单水柱（三层水源，四面玻璃封闭水稳定不流动）。
//   美西螈 spawn (3,2,3) 在水中（首 tick baseTick→updateEnvironmentState 设 m_inWater=true，
//   tick 5 首次攻击时 isInWater 已 true）。玻璃围栏困住美西螈防游走出水柱。
//   玩家 (1,2,3) 站围栏外空气（脚踩 helper-y=1 grass_block），与美西螈隔玻璃墙。
//   attackEntity 不受距离/方块阻挡（Player::attack 无距离/射线检查，直接 hurt），玩家隔玻璃可攻击水中美西螈。
//
// 攻击时序：每 12 tick attackEntity 一次。
//   **关键——无敌帧时序**：LivingEntity::hurt 无敌帧 MAX_HURT_RESISTANT_TIME=20 tick（LivingEntity.hpp:1869）：
//   受伤后 m_hurtResistantTime=20 每 tick 递减；当 m_hurtResistantTime>10（即受伤后 10 tick 内）新伤害
//   amount<=m_lastDamage（空手恒 1.0<=1.0）→ hurt 返回 false（LivingEntity.cpp:240-244），装死分支 result&&
//   跳过——攻击被无敌帧吞掉。故攻击间隔须 >10 tick 让 m_hurtResistantTime 降到 <=10 进入"新伤害重置"
//   分支，hurt 返回 true 才能进装死分支。每 12 tick 攻击确保每次都真正造成伤害并进装死判断。
//   33% 概率装死，约 3 次内触发（36 tick）；maxTicks=600 内约 50 次攻击，触发概率 1-(2/3)^50 ≈ 100%。
//   美西螈满血 14，空手 1.0 伤害——装死期间（200 tick）!isPlayingDead()=false 不重复触发，且再生回血，
//   amount<health 守卫 + 装死静止 + 再生回血三重保障不致死。
//
// 判定手段：succeedWhen 每 tick 检查美西螈 getEffect("regeneration") 非空。装死触发后下一 tick
//   PlayDeadGoal::startExecuting addEffect(Regeneration,200)，duration 200 tick 窗口内 succeedWhen 必抓到。
//   getEffect 返回 { typeId, amplifier, duration } 或 undefined（对齐 PufferfishTests.ts:109 对 getEntities
//   返回实体调 getEffect 的范式）。区域限定查美西螈排除并行测试污染。
//   非确定性：33% 概率触发，但 50 次攻击内触发概率 ~100%，maxTicks=600 留充裕余量。
//
// 对照排除：陆地无水时 hurt 装死分支 isInWater()=false 跳过（axolotlAttacksSquid 即陆地不装死）。
//   本测试专门验证水中受击装死+自身再生链路（区别 applySupportingEffects 给玩家的再生 buff）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: AxolotlEntity.cpp:231-253（hurt 装死分支）/ AxolotlGoals.cpp:64-74（PlayDeadGoal startExecuting addEffect）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_美西螈.txt#装死（水中受击装死+再生）
function axolotlPlaysDeadWithRegenerationTest(test: Test): void {
  const axolotlType = "axolotl";

  // 玻璃围栏：(2..4, 1..4, 2..4) 外围 4 面玻璃墙（围住 3×3 内空），困住美西螈防游走。
  // 围栏高 4 格（y=1..4）防美西螈跳出。围栏内填水池。
  // 先填玻璃墙（外围），再填水（内圈），最后清出美西螈 spawn 格。
  // x=2 与 x=4 两个面（z=2..4）+ z=2 与 z=4 两个面（x=3，跳过角柱已由 x 面填）。
  for (let y = 1; y <= 4; y++) {
    for (let x = 2; x <= 4; x++) {
      test.setBlockType("minecraft:glass", { x, y, z: 2 });
      test.setBlockType("minecraft:glass", { x, y, z: 4 });
    }
    for (let z = 3; z <= 3; z++) {
      test.setBlockType("minecraft:glass", { x: 2, y, z });
      test.setBlockType("minecraft:glass", { x: 4, y, z });
    }
  }
  // 围栏内 (3,1..3,3) 单水柱（美西螈所在格 + 上下水），让美西螈 isInWater=true。
  // 单水柱封闭于玻璃围栏中，水源稳定不流动。y=1..3 三层水，y=4 顶部空气（美西螈不会跳出 3 格水柱）。
  fillBlock(test, "minecraft:water", 3, 1, 3, 3, 3, 3);

  // 美西螈 spawn (3,2,3) 水中。玩家 (1,2,3) 围栏外，隔玻璃墙攻击水中美西螈。
  // gameMode 传 0：ScriptTestHelper.cpp:543-548 `auto gm = ctx.toInt32(args[idx])` 返回 std::optional<i32>，
  //   `if (gm)` 检查 optional 是否有值（engaged），**不是检查值是否为 0**。传 0 时 optional 有值（值为 0），
  //   `if (gm)` 为 true → `gameMode = GameMode(0) = Survival`。故传 0 正确得到 Survival 玩家（非 Creative）。
  //   Survival 玩家空手 Player::attack 走 hurt 链路（无 Creative 早返回，仅 isSpectator 早返回 Player.cpp:2512），
  //   ATTACK_DAMAGE 默认 1.0 无 Creative 加成（仅交互距离有加成），能正常触发 hurt 装死分支。
  const axolotl = test.spawn(axolotlType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "axolotlAttacker", 0 as any);

  // 每 12 tick attackEntity 一次（>10 tick 无敌帧间隔，见头部"攻击时序"段说明）。
  // 空手玩家伤害 1.0 < 美西螈满血 14，满足 hurt 装死分支 amount<health() 守卫。
  for (let t = 5; t <= 540; t += 12) {
    test.runAtTickTime(t, () => {
      (player as any).attackEntity(axolotl);
    });
  }

  // 断言美西螈获得再生效果：succeedWhen 每 tick 检查 getEffect("regeneration") 非空。
  // 装死触发后 PlayDeadGoal startExecuting addEffect(Regeneration,200)，duration 200 tick 窗口内必抓到。
  test.succeedWhen(() => {
    const axolotls = test.getDimension().getEntities({
      type: axolotlType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(axolotls.length > 0, "axolotl disappeared before playing dead");
    const regen = (axolotls[0] as any).getEffect("regeneration");
    test.assert(regen !== undefined,
      `axolotl did not gain regeneration from playing dead, regen=${regen}`);
  });
}

export function registerAxolotlTests(): void {
  GameTest.register("MobBehaviorTests", "axolotl_attacks_squid", axolotlAttacksSquid)
    .structureName("gametests:creeper_pit")
    .maxTicks(300);

  GameTest.register("MobBehaviorTests", "axolotl_plays_dead_with_regeneration", axolotlPlaysDeadWithRegenerationTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(600);
}
