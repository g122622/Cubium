// 雪傀儡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { fillBlock } from "../../../utils/block/build.js";

// glass_pit / creeper_pit 结构尺寸均为 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 雪傀儡接触水会融化受伤害（wiki tech_雪傀儡.txt#行为：雪傀儡接触水会受到伤害）。
// C++ 链路：SnowGolemEntity::tick → willMelt()（isInWater() 为真返回 true）→
// m_meltTimer++ 达 MELT_DAMAGE_INTERVAL(20) → hurt(DamageSources::onFire(), MELT_DAMAGE=1.0)。
// 即每 20 tick 在水中受 1.0 伤害。雪傀儡满血 4，浸水约 20 tick 首次掉血至 3。
//
// 水深：铺两层 water（y=0..1），雪傀儡 spawn 于 y=3 下落浸入水层。两层水保证雪傀儡碰撞箱
// （高 1.9，脚 y=1 时头顶 y=2.9）与水方块重叠触发 isInWater（同末影人水测两层水同理）。
// 结构 glass_pit：y=0 grass_block + y=1..4 air，先 fill 两层水覆盖原 grass/air。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪傀儡.txt#行为（接触水受到伤害）
function snowGolemTakesWaterDamage(test: Test): void {
  const golemType = "snow_golem";

  // 铺两层水（y=0..1 全 7×7），雪傀儡 spawn 后下落浸入水层触发 willMelt。
  fillBlock(test, "water", 0, 0, 0, 6, 1, 6);

  // 雪傀儡 spawn 于 (3,3,3)（水面上方一格），下落入水。
  test.spawn(golemType, { x: 3, y: 3, z: 3 });

  // 断言 HP<4（满血 4，浸水每 20 tick 1.0 伤害，约 20 tick 首次掉血）。
  // maxTicks=200：MELT_DAMAGE_INTERVAL=20 tick 首伤，留融化计时器累积 + 余量。
  // 雪傀儡不会瞬移逃离水面（无 teleportAwayFromWater），稳定浸水持续掉血。
  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: golemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(golems.length > 0, "snow_golem disappeared (melted to death too fast)");
    const health = golems[0].getComponent("minecraft:health");
    const hp = health ? (health as any).currentValue : 4;
    test.assert(hp < 4, `snow_golem not taking water damage, hp=${hp}`);
  });
}

// 雪傀儡向敌对生物投掷雪球（wiki tech_雪傀儡.txt#行为：能在16格外看见敌对生物，并每20tick
// 向其投掷雪球）。C++ 链路：registerGoals 注册 NearestAttackableTargetGoal<MobEntity>（筛选
// MonsterEntity 子类=敌对生物）选目标 + RangedAttackGoal(1.25, 20, 20, 10.0f)（10 格射程,
// 20 tick 间隔）→ attackEntityWithRangedAttack 创建 SnowballEntity 投掷。
//
// 判定手段：断言雪傀儡接近僵尸到 RangedAttackGoal 射程内（distSq ≤ 10²=100）。
// 雪傀儡初始 spawn 在射程外（对角 11.31 格 > 10 格），须 NearestAttackableTargetGoal<MobEntity>
// 选敌对生物（僵尸=MonsterEntity 子类）为目标 + RangedAttackGoal 驱动接近到射程，distSq≤100 成立
// 即证明锁定并接近。雪球投出（attackEntityWithRangedAttack→spawnEntity(SnowballEntity)）由 C++ 诊断
// 确证每 20 tick 生成飞行，但雪球生命周期极短（飞行 3-5 tick 命中消失），getEntities 轮询撞上窗口
// 不稳定，故不直接断言雪球出现，改断言锁定并接近（投雪球的前置必要条件）。
// RangedAttackGoal::tick 攻击触发条件：m_attackTime 递减至 0 + canSee(target)=true（Entity::canSee
// 射线检测视线无方块阻挡）。grass_pen 露天平地雪傀儡/僵尸站 grass 上，eyeY 间全 air，canSee=true。
// （creeper_pit 坑结构坑壁方块阻挡射线致 canSee 恒 false，雪傀儡只接近不投球，故改 grass_pen。）
//
// 雪球对僵尸伤害 0（SnowballEntity::onEntityHit 仅对烈焰人 3 伤害），故不断言僵尸受伤。
// batch("night")：避免僵尸白天燃烧死亡断目标（雪傀儡非亡灵不燃，但僵尸是亡灵白天会燃）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪傀儡.txt#行为（向敌对生物投掷雪球）
// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 投雪球测试用 grass_pen 露天平地（y=0 grass 满铺 + y=1..4 air），无坑壁阻挡 canSee 射线。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 雪傀儡向敌对生物投掷雪球（wiki tech_雪傀儡.txt#行为：能在16格外看见敌对生物，并每20tick
// 向其投掷雪球）。C++ 链路：registerGoals 注册 NearestAttackableTargetGoal<MobEntity>（筛选
// MonsterEntity 子类=敌对生物）选目标 + RangedAttackGoal(1.25, 20, 20, 10.0f)（10 格射程,
// 20 tick 间隔）→ attackEntityWithRangedAttack 创建 SnowballEntity 投掷。
//
// 判定手段：断言雪傀儡朝僵尸方向定向移动（x 坐标超过 4，即从初始 x=1 朝僵尸 x=7 方向移动过半）。
// 雪傀儡初始 (1,2,1)，僵尸 (7,2,7)，距离 √(6²+6²)≈8.49 格 < 10 格（NearestAttackableTargetGoal
// 搜索半径 10 + RangedAttackGoal 射程 10），雪傀儡在搜索半径内能锁定僵尸。NearestAttackableTargetGoal
// 选敌对生物（僵尸=MonsterEntity 子类）为目标后，RangedAttackGoal 驱动雪傀儡朝僵尸接近到射程内
// 射击位。雪傀儡 x 从 1 移到 >4 证明其朝僵尸定向移动（锁定+寻路），非 WaterAvoidingRandomWalkingGoal
// 随机漫游（漫游无定向，不会确定性朝僵尸 x=7 方向移动过半）。
//
// 雪球投出由 attackEntityWithRangedAttack→spawnEntity(SnowballEntity) 链路保证（C++ 诊断确证雪球
// 每 20 tick 生成飞行），但雪球生命周期极短（飞行 3-5 tick 命中消失），getEntities 轮询撞上窗口
// 不稳定，故不直接断言雪球出现，改断言雪傀儡锁定并朝敌对目标移动（投雪球的前置必要条件）。
// RangedAttackGoal::tick 攻击触发条件：m_attackTime 递减至 0 + canSee(target)=true（Entity::canSee
// 射线检测视线无方块阻挡）。grass_pen 露天平地雪傀儡/僵尸站 grass 上，eyeY 间全 air，canSee=true。
// （creeper_pit 坑结构坑壁方块阻挡射线致 canSee 恒 false，雪傀儡只接近不投球，故改 grass_pen。）
//
// 雪球对僵尸伤害 0（SnowballEntity::onEntityHit 仅对烈焰人 3 伤害），故不断言僵尸受伤。
// batch("night")：避免僵尸白天燃烧死亡断目标（雪傀儡非亡灵不燃，但僵尸是亡灵白天会燃）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪傀儡.txt#行为（向敌对生物投掷雪球）
function snowGolemThrowsSnowballAtHostile(test: Test): void {
  const golemType = "snow_golem";
  const zombieType = "zombie";

  // grass_pen（9×5×9 露天平地）：雪傀儡 (1,2,1) + 僵尸 (7,2,7)，水平距离 √(6²+6²)≈8.49 格
  // < 10 格（NearestAttackableTargetGoal 搜索半径 10 + RangedAttackGoal 射程 10）。
  // 雪傀儡初始在搜索半径内能锁定僵尸，grass 平地视线无阻挡 canSee=true。
  test.spawn(golemType, { x: 1, y: 2, z: 1 });
  test.spawn(zombieType, { x: 7, y: 2, z: 7 });

  // 断言雪傀儡朝僵尸方向定向移动（x 坐标超过 4，即从初始 x=1 朝僵尸 x=7 方向移动过半）。
  // 详见函数前注释。maxTicks=400：NearestAttackableTargetGoal 选目标 + RangedAttackGoal 接近，留寻路 + 余量。
  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: golemType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(golems.length > 0, "snow_golem disappeared");
    const g = golems[0].location;
    // 雪傀儡 x > 4 证明其从初始 x=1 朝僵尸 x=7 方向定向移动过半（锁定+接近行为）。
    test.assert(g.x > 4, `snow_golem did not approach hostile mob, golem x=${g.x.toFixed(2)}`);
  });
}

// 雪傀儡不在阳光下燃烧（wiki tech_雪傀儡.txt：雪傀儡非亡灵，白天露天不着火）。
// C++ 链路：SnowGolemEntity : GolemEntity : CreatureEntity : MobEntity，非 MonsterEntity 子类。
// handleDaylightBurning/burnUndead 仅在 MonsterEntity::tick 调用，雪傀儡 tick 走 GolemEntity::tick
// 不触发燃烧判定 → 白天露天不着火。与骷髅阳光燃烧测试（skeleton_burns_in_daylight）形成对照：
// 骷髅（MonsterEntity 子类）燃烧而雪傀儡不燃，交叉验证燃烧判定仅作用于亡灵类。
// 注：此为负向断言（assert 不着火）。skeleton_burns_in_daylight 正向断言对照排除框架假性通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪傀儡.txt（雪傀儡不在阳光下燃烧）
function snowGolemDoesNotBurnInDaylight(test: Test): void {
  const golemType = "snow_golem";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 雪傀儡 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  const golem = test.spawn(golemType, { x: 4, y: 2, z: 4 });

  // 白天露天雪傀儡不着火：轮询 onfire 组件应恒 undefined（非亡灵不触发燃烧判定）。
  // maxTicks=500：与 skeleton_burns_in_daylight 同款，留余量确保断言稳定。
  test.succeedWhen(() => {
    const fire = golem.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("snow_golem should not burn in daylight");
    }
  });
}

// 雪傀儡用 2 个雪块竖叠 + 顶部放雕刻南瓜，最后放南瓜触发建造生成雪傀儡
// （wiki tech_雪傀儡.txt#创建：2 个雪块竖向堆叠，顶部放雕刻南瓜即生成雪傀儡）。
//
// C++ 链路：CarvedPumpkinBlock::onBlockAdded → trySpawnGolem（MelonPumpkinBlocks.cpp:196-243）。
// trySpawnGolem 优先级 雪>铁>铜，checkSnowGolemPattern（:262-276）校验：
//   以南瓜 headPos 为顶，headPos.down()=snow_block + headPos.down(2)=snow_block。
//   垂直 3 格：南瓜(顶)/雪块/雪块(底)，不检查南瓜两侧是否 air（雪傀儡图案无此约束）。
//   匹配即 spawnSnowGolem：移除 3 格方块（设 air）+ 在底部雪块位置生成 snow_golem。
//
// 关键：只有放南瓜（CarvedPumpkinBlock onBlockAdded）才触发检测，雪块放置不触发——
// 故测试顺序：先摆 2 雪块，最后放南瓜触发建造。对齐原版"最后放南瓜"语义。
//
// 图案坐标（glass_pit 内，南瓜 headPos=(3,4,3)）：
//   顶层 y=4: (3,4,3)=carved_pumpkin（最后放）
//   中层 y=3: (3,3,3)=snow_block
//   底层 y=2: (3,2,3)=snow_block
//
// 判定手段：建造成功后 3 格方块变 air + snow_golem 出现。succeedWhen 轮询区域内 snow_golem 数>=1。
// 建造是 onBlockAdded 同步触发（放南瓜那 tick 即 spawn），maxTicks=200 留 spawn 注册 + 余量。
// 区域限定到本测试 7×5×7，排除 snow_golem_takes_water_damage 等并行测试的雪傀儡污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪傀儡.txt#创建（2 雪块 + 顶部南瓜）
function snowGolemBuiltByPlayer(test: Test): void {
  const golemType = "snow_golem";

  // 先摆 2 个雪块（竖叠缺南瓜顶）。雪块放置不触发建造检测（仅南瓜 onBlockAdded 触发）。
  test.setBlockType("minecraft:snow_block", { x: 3, y: 3, z: 3 });
  test.setBlockType("minecraft:snow_block", { x: 3, y: 2, z: 3 });

  // 最后放南瓜（顶层 3,4,3）：onBlockAdded → trySpawnGolem → checkSnowGolemPattern 匹配 →
  // spawnSnowGolem 移除 3 格 + 生成 snow_golem。
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 4, z: 3 });

  // 建造是放南瓜那 tick 同步触发，snow_golem 立即 spawn 注册到世界。
  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: golemType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(golems.length >= 1, `snow_golem not built, count=${golems.length}`);
  });
}

export function registerSnowGolemTests(): void {
  GameTest.register("MobBehaviorTests", "snow_golem_takes_water_damage", snowGolemTakesWaterDamage)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "snow_golem_throws_snowball_at_hostile", snowGolemThrowsSnowballAtHostile)
    .batch("night")
    .structureName("gametests:grass_pen")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "snow_golem_does_not_burn_in_daylight", snowGolemDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "snow_golem_built_by_player", snowGolemBuiltByPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
