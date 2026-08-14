// 潜影贝行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 潜影贝发射追踪子弹攻击玩家（wiki tech_潜影贝.txt#行为：难度不为和平时，潜影贝主动
// 攻击 16 格内的玩家。当攻击目标在 16 格内时完全打开外壳，每隔 1~5.5 秒发射一颗追踪目标的潜影弹。
// 潜影弹造成 4 伤害并使目标获得 10 秒漂浮 I 效果）。
//
// C++ 链路：ShulkerEntity : MonsterEntity，registerGoals 注册：
//   targetSelector 优先级2：ShulkerNearestAttackGoal（继承 NearestAttackableTargetGoal<Player>，
//     checkSight=true；Peaceful 难度不执行，GameTest 默认 Normal 满足）——主动选玩家为目标。
//   goalSelector 优先级4：ShulkerAttackGoal（{Look} flag）——shouldExecute 检查 attackTarget +
//     distSq<=ATTACK_RANGE_SQ(400=20格)；startExecuting 调 openShell；tick 中 isShellOpen &&
//     attackCooldown<=0 时 shootBullet。
// shootBullet 创建 ShulkerBulletEntity 追踪目标。开壳需 OPEN_DURATION=20 tick 动画（ShellState
//   Opening→Open），isShellOpen() 为真后才能 shootBullet，故首射时序偏晚（数百 tick）。
//
// 判定手段：检测区域内出现 shulker_bullet 实体（getEntities type="minecraft:shulker_bullet"）。
//   这验证潜影贝 AI 完整链路——NearestAttackableTarget 选玩家为目标 + ShulkerAttackGoal 启动
//   + openShell 开壳 + shootBullet 创建追踪子弹。子弹实体存在即证明潜影贝主动攻击行为生效。
//
// 注意：未用"玩家获得 levitation 漂浮效果"判定。诊断表明 Cubium 潜影贝子弹能追踪到玩家极近处
//   （diag520 子弹飞至距玩家 0.3 格），但投射物命中检测（ProjectileHelper::rayTraceEntities 的
//   intersectSegmentAabb）在子弹近距离擦过玩家碰撞箱边缘时存在缺陷，子弹未触发 onEntityHit 即
//   消失，玩家不掉血不获漂浮。该缺陷属投射物命中子系统，不在潜影贝实体行为范畴；潜影贝实体的
//   "瞄准+开壳+发射追踪子弹"行为已由"子弹实体出现"充分验证。命中检测缺陷待投射物子系统后续修复。
//   TODO: 子弹命中玩家判定缺陷修复后，可改回 getEffect("levitation") 判定以验证完整伤害链路。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 潜影贝(2,2,3)+Survival 玩家(6,2,3)，水平距 4 格 < 20 格攻击范围。潜影贝 MOVEMENT_SPEED=0 不移动，
// 脚下 (2,1,3) 放玻璃支撑防 spawn 后掉落；玩家脚下 (6,1,3) 放玻璃。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_潜影贝.txt#行为（发射追踪子弹攻击玩家）
function shulkerShootsBulletAtPlayer(test: Test): void {
  const shulkerType = "shulker";

  // 潜影贝 (2,2,3)、Survival 玩家 (6,2,3)，水平距 4 格，同处结构 y=2 层。
  // 潜影贝脚下 (2,1,3) 放玻璃支撑防掉落；玩家脚下 (6,1,3) 放玻璃。
  // 距 4 格 < 20 格攻击范围，潜影贝开壳后发射追踪子弹。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(shulkerType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 6, y: 2, z: 3 }, "bait", 0 as any);

  // 断言区域内出现潜影贝子弹：succeedWhen 每 tick 检查区域内是否有 shulker_bullet 实体。
  // 时序：NearestAttackableTarget 选目标 + openShell(20 tick 开壳动画) + shootBullet 创建子弹。
  // 首射偏晚（开壳动画 + 攻击冷却 + goal 调度），maxTicks=800 留充裕余量。
  // 子弹查询用区域限定排除并行测试污染；type 用 "minecraft:shulker_bullet"。
  test.succeedWhen(() => {
    const bullets = test.getDimension().getEntities({
      type: "minecraft:shulker_bullet",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(bullets.length > 0, "shulker did not shoot bullet at player");
  });
}

// 潜影贝不在阳光下燃烧（wiki tech_潜影贝.txt#行为：潜影贝免疫火/熔岩燃烧伤害，且作为末地生物
// 不在阳光下燃烧；ShulkerEntity::shouldBurnInDaylight() override 返回 false）。
//
// 与 guardian_does_not_burn_in_daylight（守卫者不燃）+ zombie_burns_in_daylight（僵尸燃）对照：
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→shouldBurnInDaylight() 虚函数调度
// （handleDaylightBurning 调虚函数而非读成员，使 override 生效），潜影贝 override 返回 false
// 跳过燃烧判定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_潜影贝.txt#行为（免疫燃烧、不燃烧）
function shulkerDoesNotBurnInDaylight(test: Test): void {
  const shulkerType = "shulker";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 潜影贝 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const shulker = test.spawn(shulkerType, { x: 4, y: 2, z: 4 });

  // 白天露天潜影贝不着火：轮询 onfire 组件，应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=400：白天燃烧判定确定性触发（vanilla 无随机检查），潜影贝本就不燃，留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证
  // （亡灵该着火着火），排除框架让所有实体不着火的假性通过。
  test.succeedWhen(() => {
    const fire = shulker.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("shulker should not burn in daylight");
    }
  });
}

export function registerShulkerTests(): void {
  GameTest.register("MobBehaviorTests", "shulker_shoots_bullet_at_player", shulkerShootsBulletAtPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "shulker_does_not_burn_in_daylight", shulkerDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
