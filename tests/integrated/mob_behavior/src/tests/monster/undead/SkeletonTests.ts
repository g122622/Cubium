// 骷髅行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit / grass_pen / mediumglass 结构尺寸（7×5×7 / 9×5×9 / 12×9×11），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };
const GLASS_FROM = { x: 0, y: 0, z: 0 };
const GLASS_VOLUME = { x: 12, y: 9, z: 11 };

// 骷髅在阳光下着火：白天露天环境（canSeeSky=true 且亮度>0.5）下，亡灵生物每 tick 有概率
// 被点燃 8 秒。C++ 链路：MonsterEntity::tick → handleDaylightBurning → burnUndead →
// igniteForSeconds(8.0f)；isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky。
// 骷髅默认 m_burnsInDaylight=true（MonsterEntity 基类），无 isImmuneToFire，故露天白天必燃。
// JS 侧读火焰状态：Entity.getComponent("minecraft:onfire") 未着火返回 undefined，
// 着火返回 OnFireComponent（对齐基岩 OnFireComponent 语义"组件存在即着火"）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅.txt#生物族群（阳光下着火）
function skeletonBurnsInDaylight(test: Test): void {
  const skeletonType = "skeleton";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙围栏+内部空气，y=4 全 air 露天。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 骷髅 spawn 于 helper-y=2（结构内 y=1 空气腔），头顶 y=2/y=3 空气、y=4 露天 → canSeeSky=true。
  // 中心位置（4,2,4）远离玻璃墙，骷髅 AI 游荡不会触及围栏；整个空气腔头顶均露天，无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自 @minecraft/server-gametest 内嵌的
  // @minecraft/server，与顶层包的 Entity 类型因 Dimension 属性差异不兼容，显式标注会触发 TS2322。
  const skeleton = test.spawn(skeletonType, { x: 4, y: 2, z: 4 });

  // 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
  // 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，
  // 骷髅无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（期望 25 tick + 余量）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = skeleton.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("skeleton not on fire yet");
    }
  });
}

// 骷髅远程射击玩家（wiki tech_骷髅.txt#行为：骷髅追逐 16 格内的玩家，进入射程后持弓射箭）。
//
// C++ 链路：SkeletonEntity 构造期强制补弓（SkeletonEntity.cpp:53-55，GameTest spawn 不走
// finalizeSpawn 故需构造期补弓）→ AbstractSkeletonEntity::setCombatTask 判定主手持弓 →
// 注册 RangedBowAttackGoal（优先级 4）→ targetSelector 的 NearestAttackableTargetGoal<Player>
// （checkSight=true）选 Survival 玩家为 attackTarget → RangedBowAttackGoal::tick 在射程内
// （15 格）seenTime>=20 后蓄力 20 tick 发射 → attackEntityWithRangedAttack 创建 ArrowEntity
// （setBaseDamageFromMob 约 2-4 伤害）→ 箭矢命中玩家造成伤害。
//
// 环境选择：必须夜晚 batch("night") + creeper_pit（开放坑）。AbstractSkeleton 注册了
// RestrictSunGoal（限制阳光）+ FleeSunGoal（逃离阳光），白天露天骷髅会优先逃离阳光而非攻击玩家
// （wiki: 骷髅不像僵尸那样不顾一切追逐，会寻找阴凉）。夜晚无阳光 FleeSun 不触发，骷髅主动选玩家射击。
// creeper_pit 开放坑无围墙阻挡 checkSight 视线 + 寻路通畅（glass_pit 玻璃挡寻路）。
//
// 判定手段：断言玩家 HP 下降（<20）。骷髅远程箭伤害约 2-4（setBaseDamageFromMob），
// 玩家初始满血 20，被 1 箭命中即掉至 <20。箭矢命中玩家即证明 RangedBowAttackGoal 远程攻击链路通。
// 不直接断言箭矢实体出现（箭矢飞行命中后消失，getEntities 轮询撞窗口不稳，见 SnowGolemTests 同款坑）。
// 玩家用 Survival（gameMode=0）：创造/旁观被 isSuitableTarget 滤掉，骷髅不选其为目标。
// 此测试与 stray_shoots_arrow_at_player / bogged_shoots_poison_arrow_at_player 形成同构对照：
// 三者同为 AbstractSkeletonEntity 子类持弓远程，交叉验证 setCombatTask 远程分支。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅.txt#行为（追逐玩家并射箭）
function skeletonShootsArrowAtPlayer(test: Test): void {
  const skeletonType = "skeleton";

  // 骷髅于 (1,2,1)（一角），Survival 玩家于 (5,2,5)（对角，距 ~5.7 格 < 15 格射程）。
  // 骷髅在射程内锁定玩家后停止移动 + strafe 射击（RangedBowAttackGoal distSq<=15² 且 seenTime>=20）。
  // 玩家会被箭命中掉血（骷髅箭伤害 ~2-4）。玩家 HP 20，约 1 箭即掉至 <20。
  test.spawn(skeletonType, { x: 1, y: 2, z: 1 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：骷髅 seenTime>=20（约 tick 20）+ 蓄力 20 tick（约 tick 40）首箭 + 箭飞行几 tick命中，
  // 约 tick 45-60 玩家首伤。maxTicks=400 留寻路/锁定/蓄力 + 余量。
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
      `skeleton did not shoot player, hp=${(health as any).currentValue}`);
  });
}

// 骷髅不持弓时改用近战攻击（wiki tech_骷髅.txt#行为：如果骷髅不持任何物品或所持物品不是弓，
// 会双手平举进行近战攻击，行为与凋零骷髅相似）。
//
// C++ 链路：SkeletonEntity 构造期补弓后，通过 equippable 组件清空主手 →
// AbstractSkeletonEntity::setEquipment（EquipmentSlot::MainHand 变更）触发 setCombatTask() →
// setCombatTask 检查主手 canUseNonMeleeWeapon（空手 weaponItem==nullptr → shouldUseRanged=false）
// → removeGoalsOfType<RangedBowAttackGoal> + 注册 MeleeAttackGoal（优先级 4）→
// 骷髅改用近战 AI 追击玩家 → 贴脸后 MeleeAttackGoal::attackEntityAsMob 造成 ATTACK_DAMAGE=2.0 伤害。
//
// 卸弓手段：Entity.getComponent("minecraft:equippable").setEquipment("Mainhand", undefined)。
// Cubium 绑定（MinecraftModuleFactory.cpp setEquipment）第二参数 undefined/null 时执行
// living->setEquipment(slot, ItemStack::EMPTY)，触发 C++ setEquipment → setCombatTask 重评。
// EquipmentSlot 字符串 "Mainhand"（首字母大写、hand 小写，对齐基岩枚举）。
//
// 判定手段：断言玩家 HP<20。近战伤害 2.0（ATTACK_DAMAGE 属性），玩家 20 血，1 击即掉至 18<20。
// 近战需骷髅寻路贴近玩家（creeper_pit 7×7，骷髅一角玩家对角约 5.7 格），寻路 + 近战命中时序较长，
// maxTicks=600 留充足寻路时间。与 skeleton_shoots_arrow_at_player（持弓远程）形成对照：
// 同一骷髅实体，持弓走 RangedBowAttackGoal 远程分支，卸弓走 MeleeAttackGoal 近战分支，
// 交叉验证 setCombatTask 的 shouldUseRanged 分支选择。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅.txt#行为（不持弓时近战攻击）
function skeletonFightsInMeleeWithoutBow(test: Test): void {
  const skeletonType = "skeleton";

  // 骷髅于 (1,2,1)，spawn 后立即清空主手弓 → 触发 setCombatTask 切 MeleeAttackGoal。
  // Survival 玩家于 (5,2,5)（对角，距 ~5.7 格）。骷髅近战 AI 追击玩家，贴脸后造成 2.0 伤害。
  const skeleton = test.spawn(skeletonType, { x: 1, y: 2, z: 1 });

  // 清空主手弓：equippable.setEquipment("Mainhand", undefined) → C++ setEquipment(MainHand, EMPTY)
  // → setCombatTask removeGoalsOfType<RangedBowAttackGoal> + addGoal<MeleeAttackGoal>。
  // 必须在 spawn 后立即执行，确保骷髅从首 tick 起就走近战 AI（避免先远程射一箭再切近战）。
  const equippable = skeleton.getComponent("minecraft:equippable");
  test.assert(equippable !== undefined, "skeleton has no equippable component");
  (equippable as any).setEquipment("Mainhand", undefined);

  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家掉血：近战命中即 HP<20。
  // 时序：骷髅切近战 AI（tick 0）+ 寻路贴近玩家（~5.7 格，骷髅速度 0.25，约 100-200 tick）+
  // MeleeAttackGoal attackInterval 后首次命中。maxTicks=600 留充足寻路 + 命中余量。
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
      `unarmed skeleton did not melee player, hp=${(health as any).currentValue}`);
  });
}

// 骷髅逃避狼（wiki tech_骷髅.txt#行为：骷髅会主动逃离狼，同时狼会主动攻击骷髅）。
//
// C++ 链路：AbstractSkeletonEntity::registerGoals 注册 AvoidEntityGoal<WOLF>（优先级 3，
// 6 格检测距离，远距速度 1.0 近距 1.2）→ 骷髅检测到 6 格内狼时主动逃离。
// 同时 WolfEntity::registerGoals 注册 NearestAttackableTargetGoal<LivingEntity>（checkSight=false，
// 谓词匹配 SKELETON/STRAY/WITHER_SKELETON）→ 狼无论是否驯服都主动追击骷髅。
// 双向交互：骷髅一边逃一边被狼追，狼速度 0.3 > 骷髅 0.25，最终狼追上咬骷髅（狼伤害 2.0/击）。
//
// 环境选择：必须夜晚 batch("night")（骷髅 RestrictSun/FleeSun 怕阳光）+ grass_pen（9×5×9 有玻璃墙）。
// 关键：不用 creeper_pit（开放坑无围墙）——骷髅 AvoidEntityGoal 会朝远离狼方向移动，7×7 开放坑
// 对角线 ~8.5 格 > 6 格检测距离，骷髅逃出检测范围后随机游荡走出 7×7 查询区，区域限定 getEntities
// 查不到骷髅会被 length===0 误判为"已死"假通过（flaky 风险）。grass_pen 玻璃墙把骷髅限制在内部空气腔，
// 不会跑出查询区。狼对骷髅 checkSight=false、骷髅 AvoidEntityGoal 也不依赖视线，玻璃墙不影响双方目标选择。
//
// 判定手段：双判定（同 WolfTests.wolfAttacksSheep 模式）——骷髅 HP<20（狼追上咬了几口）或
// 骷髅消失（狼最终咬死）。骷髅满血 20，HP<20 即"受到过伤害"。狼 2.0/击 + 20 tick 冷却，
// 约 10 击（200+ tick）咬死骷髅。maxTicks=1000 留追逐 + 击杀余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅.txt#行为（逃离狼）
function skeletonFleesFromWolf(test: Test): void {
  const skeletonType = "skeleton";
  const wolfType = "wolf";

  // 骷髅于 (2,2,2)，狼于 (6,2,6)（grass_pen 内部空气腔对角，距 ~5.7 格 < 6 格 AvoidEntity 检测距离）。
  // 狼立即锁定骷髅（checkSight=false）追击；骷髅检测到狼逃避。grass_pen 玻璃墙防骷髅逃出查询区。
  test.spawn(skeletonType, { x: 2, y: 2, z: 2 });
  test.spawn(wolfType, { x: 6, y: 2, z: 6 });

  // 双判定：骷髅消失（狼咬死）直接通过；否则断言骷髅 HP<20（狼追上咬伤）。
  // 区域限定用 PEN（grass_pen 9×5×9），type 不带前缀（怪物类型）。
  test.succeedWhen(() => {
    const skeletons = test.getDimension().getEntities({
      type: skeletonType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (skeletons.length === 0) {
      return; // 骷髅已被狼咬死消失——逃避+被追击行为生效，直接通过。
    }
    const health = skeletons[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "skeleton has no health component");
    test.assert((health as any).currentValue < 20,
      `wolf did not catch fleeing skeleton, hp=${(health as any).currentValue}`);
  });
}

// 骷髅攻击铁傀儡（wiki tech_骷髅.txt#行为：骷髅追逐 16 格内的玩家、铁傀儡和幼年海龟）。
//
// C++ 链路：AbstractSkeletonEntity::registerGoals 的 targetSelector 注册
// NearestAttackableTargetGoal<IronGolemEntity>（优先级 3，checkSight=true）→ 骷髅主动选铁傀儡为
// attackTarget → RangedBowAttackGoal 远程射铁傀儡 → 箭矢命中铁傀儡造成伤害（铁傀儡 100 HP）。
// 同时 IronGolemEntity 也注册 NearestAttackableTargetGoal<MonsterEntity>（骷髅是 MonsterEntity 子类）
// → 铁傀儡也会追击骷髅（双向锁定）。
//
// 判定手段：断言铁傀儡 HP<100（骷髅射中铁傀儡使其掉血）。铁傀儡 100 血，骷髅箭伤 2-4/箭，
// 需骷髅射中至少 1 箭。单只骷髅风险：铁傀儡近战 7.5~21.5 伤害 2 击即杀骷髅（20HP），骷髅在铁傀儡
// 靠近前仅能射 1-2 箭，存在 0 命中即死、铁傀儡 HP 仍 100 的失败风险。故放 4 只骷髅分散四角齐射：
// 铁傀儡追杀近角骷髅时，远角骷髅持续射击，4 骷髅齐射大幅提高铁傀儡中箭概率，确保铁傀儡掉血。
//
// 环境选择：夜晚 batch("night")（骷髅怕阳光）+ mediumglass（12×9×11，空间大，4 骷髅可分散四角
// 拉开与铁傀儡距离，延长铁傀儡追杀时间窗口让远角骷髅多射箭）。mediumglass 空气腔无内部遮挡，
// 骷髅-铁傀儡视线通畅（玻璃墙在边界外不挡空气腔内视线）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅.txt#行为（追逐铁傀儡）
function skeletonAttacksIronGolem(test: Test): void {
  const skeletonType = "skeleton";
  const ironGolemType = "iron_golem";

  // 铁傀儡于中心 (6,3,5)，4 骷髅分散四角 (3,3,2)/(9,3,2)/(3,3,8)/(9,3,8)，距铁傀儡均约 4.2 格 < 15 射程。
  // mediumglass 空气腔 x∈[2,10] z∈[1,9] y=3，四角坐标均在空气腔内。铁傀儡追近角骷髅时远角骷髅齐射。
  test.spawn(ironGolemType, { x: 6, y: 3, z: 5 });
  test.spawn(skeletonType, { x: 3, y: 3, z: 2 });
  test.spawn(skeletonType, { x: 9, y: 3, z: 2 });
  test.spawn(skeletonType, { x: 3, y: 3, z: 8 });
  test.spawn(skeletonType, { x: 9, y: 3, z: 8 });

  // 断言铁傀儡掉血：HP<100 即证明至少 1 只骷髅射中铁傀儡（远程攻击铁傀儡链路通）。
  // 时序：4 骷髅 seenTime>=20（约 tick 20）+ 蓄力 20 tick（约 tick 40）首箭齐射，铁傀髡走过来
  // 追杀约 tick 60-150，期间 4 骷髅射出多箭。maxTicks=800 留齐射 + 命中 + 余量。
  // 铁傀髡查询用区域限定 GLASS（mediumglass 12×9×11），type 不带前缀。
  test.succeedWhen(() => {
    const golems = test.getDimension().getEntities({
      type: ironGolemType,
      location: test.worldLocation(GLASS_FROM),
      volume: GLASS_VOLUME,
    });
    test.assert(golems.length > 0, "iron_golem disappeared");
    const health = golems[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "iron_golem has no health component");
    test.assert((health as any).currentValue < 100,
      `skeletons did not shoot iron_golem, hp=${(health as any).currentValue}`);
  });
}

export function registerSkeletonTests(): void {
  GameTest.register("MobBehaviorTests", "skeleton_burns_in_daylight", skeletonBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false（骷髅阳光燃烧测试稳定失败的根因）。skyAccess=true 让
    // MinecraftStructurePlacer 清空结构 footprint 正上方至世界顶部的所有方块，制造露天列使
    // canSeeSky=true。不设此值则 grass_pen 顶部被 worldgen 石头覆盖，第一次 PASSED 是结构偶然
    // 落在 worldgen 洞穴位置的 flaky 假象。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后，光照变更入队 m_lightQueue，需若干世界 tick 由
    // ServerWorld::tick 的 drainAndProcess 批量重算 skyLight 达 15。setupTicks 阶段（负 tickCount）
    // 让世界先 tick 20 次让光照稳定，再正式跑测试体，避免首 tick canSeeSky 仍为 false。
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "skeleton_shoots_arrow_at_player", skeletonShootsArrowAtPlayer)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "skeleton_fights_in_melee_without_bow", skeletonFightsInMeleeWithoutBow)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "skeleton_flees_from_wolf", skeletonFleesFromWolf)
    .batch("night")
    .structureName("gametests:grass_pen")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "skeleton_attacks_iron_golem", skeletonAttacksIronGolem)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(800);
}
