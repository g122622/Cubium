// 美西螈行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

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

export function registerAxolotlTests(): void {
  GameTest.register("MobBehaviorTests", "axolotl_attacks_squid", axolotlAttacksSquid)
    .structureName("gametests:creeper_pit")
    .maxTicks(300);
}
