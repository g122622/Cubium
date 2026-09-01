// 苦力怕行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 苦力怕接近玩家后膨胀爆炸：CreeperSwellGoal 在 attackTarget 距离<3 格时驱动膨胀，
// tick 累加 m_timeSinceIgnited，达 fuseTime(30 tick=1.5s) 后 explode() 并 remove()。
// C++ 链路：NearestAttackableTargetGoal(优先级1,checkSight,限PLAYER) 设 attackTarget →
// CreeperSwellGoal(优先级2, distSq<9 膨胀/distSq>49 取消) →
// CreeperEntity::tick 检 fuse → explode() → remove()。
// JS 读不到膨胀状态（无 DataParameter 同步、无 creeper 组件绑定），故断言走"爆炸后实体消失"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#引爆
function creeperSwellExplodes(test: Test): void {
  const creeperType = "creeper";

  // 结构 creeper_pit（7×5×7 开放坑）：y=0 满铺 grass_block 地板，y=1..4 全 air（无围墙）。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 苦力怕 spawn 于 (3,2,3)（helper-y=2 → 结构内 y=1 空气，脚踩结构内 y=0 grass_block），
  // 玩家于 (4,2,3)，直线距离 1 格：
  // - 在 CreeperSwellGoal 触发距离内（SWELL_TRIGGER_DISTANCE_SQ=9，即<3 格），无需寻路接近
  // - 绕过 MeleeAttackGoal 寻路环节，纯测 setCreeperState 膨胀 + fuse 累加 + explode 链路
  // - 开放坑无围墙遮挡，NearestAttackableTargetGoal checkSight=true 的 canSee 射线不被玻璃阻挡
  //   （grass_pen 外圈玻璃墙会挡视线致 attackTarget 恒 null，creeper_pit 无墙规避此问题）
  // 玩家用 Survival 模式（gameMode=0）：
  //   默认创造的 SimulatedPlayer 会被 TargetGoal::isSuitableTarget 滤掉（创造/旁观不可被攻击），
  //   苦力怕不会选其为目标，CreeperSwellGoal 永不触发。必须显式传 Survival。
  //   运行时 C++ 绑定 ScriptTestHelper.cpp 期望第三参为数字（isNumber→toInt32，
  //   mc::GameMode{Survival=0,Creative=1,...}），而非 TS 类型定义里的字符串枚举 GameMode；
  //   故传数字 0 并用 as any 绕过 TS 字符串枚举类型校验（类型定义与运行时不符）。
  //   注意：不可 `import { GameMode } from "@minecraft/server"`——Cubium 运行时该模块未导出
  //   GameMode，导入会使整个行为包 entry 加载失败（SyntaxError: Could not find export 'GameMode'）。
  test.spawn(creeperType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 时序：CreeperSwellGoal 首次 tick(优先级2,抢占) 设 state=1 → CreeperEntity::tick 每 tick
  // m_timeSinceIgnited +=1，达 fuseTime(30 tick=1.5s) 后 explode() → remove()。maxTicks=200 余量充足。
  // 断言爆炸后苦力怕实体消失：explode() 末尾调 remove()，assertEntityPresentInArea 扫描
  // 结构 bounds 查实体（非查方块），故爆炸破坏地板草地不影响该断言。
  // 爆炸需 mobGriefing=true（默认开）；即便关闭爆炸仅伤实体不破坏方块，苦力怕仍 remove()。
  test.succeedWhen(() => {
    test.assertEntityPresentInArea(creeperType, false);
  });
}

// 苦力怕被闪电击中后充能为闪电苦力怕（wiki tech_苦力怕.txt#闪电苦力怕：被闪电击中后变成
// 闪电苦力怕，其爆炸威力更强，借此可获得生物头颅）。
//
// C++ 链路：LightningBoltEntity::_damageEntities 对命中范围(±3 XZ)内实体先 hurt(5.0) 再调
// entity->onStruckByLightning()（EffectEntities.cpp:509-511）。CreeperEntity::onStruckByLightning
// 仅 setPowered(true)，不转化实体、不移除自身、不检查难度（对齐 vanilla Creeper#thunderHit，
// 区别于 PigEntity 转化需非 Peaceful——充能产物仍是苦力怕本身、无和平消失问题）。
// 苦力怕 20 血，闪电伤害 5，存活 15 血，充能后实体仍在。
//
// 判定手段：读 minecraft:is_charged 组件（对齐基岩 EntityIsChargedComponent，componentId=
// "minecraft:is_charged"，"组件存在即 charged"）。Cubium 绑定（MinecraftModuleFactory.cpp
// getComponent）对 CreeperEntity 查 isPowered()，true 返回 IsChargedComponent，否则 undefined。
// 闪电劈中前苦力怕未充能（组件 undefined），劈中后充能（组件非 undefined）即证明
// onStruckByLightning override 被调用并设了 powered。此为充能链路的精确端到端验证。
// 充能本身不引爆（vanilla thunderHit 不触发 swell），苦力怕仅标记 powered，故存活可读组件。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#闪电苦力怕
function creeperChargedByLightning(test: Test): void {
  const creeperType = "creeper";
  const lightningType = "lightning_bolt";

  // 结构 creeper_pit（7×5×7 开放坑）：y=0 grass_block，y=1..4 air，无围墙（与 creeper_swell_explodes
  // 同结构）。苦力怕与闪电同格 spawn：闪电 ±3 XZ 命中范围必覆盖苦力怕。
  // 苦力怕 (3,2,3)（脚踩结构内 y=0 grass_block，helper-y=2→结构内 y=1 空气），闪电同位 (3,2,3)。
  const creeper = test.spawn(creeperType, { x: 3, y: 2, z: 3 });
  test.spawn(lightningType, { x: 3, y: 2, z: 3 });

  // 时序：闪电首 tick 即 _damageEntities → hurt(5) + onStruckByLightning → setPowered(true)。
  // 充能当 tick 完成。苦力怕 20-5=15 血存活，充能不引爆。
  // 断言：苦力怕仍存活（未消失，闪电伤害未致死）且已充能（is_charged 组件非 undefined）。
  // 用 spawn 返回的 creeper 引用读组件——充能是苦力怕自身状态，不依赖坐标查询，引用稳定。
  // maxTicks=200 留闪电生成 + 首 tick 触发 + 余量。
  test.succeedWhen(() => {
    test.assertEntityPresentInArea(creeperType, true);
    const charged = creeper.getComponent("minecraft:is_charged");
    test.assert(charged !== undefined,
      "creeper was not charged by lightning (is_charged component missing)");
  });
}

// 苦力怕逃离猫（wiki tech_苦力怕.txt#行为：当苦力怕与猫或豹猫距离6格或更近时，苦力怕会
// 开始忽略附近生物并逃离到与其相距至少16格。苦力怕在逃离时速度比追赶玩家要快）。
//
// C++ 链路：CreeperEntity::registerGoals 注册 AvoidEntityGoal<CAT/OCELOT>（优先级3，
// avoidDistance=6.0 检测距离，farSpeed=1.0 nearSpeed=1.2）。苦力怕检测到 6 格内猫时主动逃离。
// 注意：与 skeleton_flees_from_wolf（狼双向追击骷髅）不同，猫不主动攻击苦力怕，仅苦力怕单向逃离。
//
// 环境选择：grass_pen（9×5×9，玻璃墙围栏）。关键：不用 creeper_pit（开放坑无围墙）——苦力怕
// AvoidEntityGoal 会朝远离猫方向逃离，开放坑无墙约束苦力怕可能游荡出查询区致 getEntities 查不到
// 被误判。grass_pen 玻璃墙把苦力怕限制在内部空气腔。猫对苦力怕无攻击行为，玻璃墙不影响。
// 苦力怕无 RestrictSun/FleeSun goal，day/night 均可，用默认 day 批次。
//
// 判定手段：苦力怕从猫旁(2,2,3)逃离，断言苦力怕距猫水平距离 > 4 格（初始 1 格→逃离后 >4 格）。
// 用 getEntities 区域限定取苦力怕世界坐标计算与猫世界坐标的水平距离。逃离方向虽随机（朝远离猫
// 的任意方向），但只要距猫 >4 格即证明 AvoidEntityGoal 驱动了位移。grass_pen 9×9 对角 ~11 格，
// 苦力怕逃离到 >4 格容易满足。判定距离阈值（而非固定区域）规避逃离方向随机导致的 flaky。
// maxTicks=1000 留逃离寻路 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#行为（逃离猫）
function creeperFleesFromCat(test: Test): void {
  const creeperType = "creeper";
  const catType = "cat";

  // 猫于 (2,2,2)（grass_pen 内部空气腔一角），苦力怕于 (2,2,3)（紧邻猫，距 1 格 < 6 检测距离）。
  // 苦力怕立即检测到猫并朝远离方向逃离。grass_pen 玻璃墙防苦力怕逃出查询区。
  // grass_pen y=0 满铺 grass_block，苦力怕 helper-y=2→结构内 y=1 空气，脚踩 y=0 草地，无需补支撑。
  test.spawn(catType, { x: 2, y: 2, z: 2 });
  test.spawn(creeperType, { x: 2, y: 2, z: 3 });

  // 猫世界坐标（helper (2,2,2) 经 worldLocation 转换）。
  const catWorld = test.worldLocation({ x: 2, y: 2, z: 2 });

  // 断言苦力怕逃离：距猫水平距离 > 4 格（初始 1 格，逃离后应 >4 格）。
  // 区域限定用 PEN（grass_pen 9×5×9）排除并行测试污染。
  // 逃离速度 1.0+，拉开 4 格约需 20-40 tick，maxTicks=1000 留充裕余量吸收非确定性。
  test.succeedWhen(() => {
    const creepers = test.getDimension().getEntities({
      type: creeperType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(creepers.length > 0, "creeper disappeared");
    const c = creepers[0];
    const dx = c.location.x - catWorld.x;
    const dz = c.location.z - catWorld.z;
    test.assert(dx * dx + dz * dz > 4 * 4,
      `creeper did not flee from cat, distSq=${dx * dx + dz * dz}`);
  });
}

// 苦力怕逃离豹猫（wiki tech_苦力怕.txt#行为：当苦力怕与猫或豹猫距离6格或更近时，苦力怕会
// 开始忽略附近生物并逃离到与其相距至少16格）。
//
// 本测试与 creeper_flees_from_cat 形成对照：两者共用同一 AvoidEntityGoal（CreeperEntity.cpp:328-338），
// 该 goal 用单个 lambda 谓词 `type == CAT || type == OCELOT` 同时匹配猫和豹猫。cat 测试已验证 CAT 分支，
// 本测试验证 OCELOT 分支——若谓词误只写 `type == CAT`（漏 OCELOT），cat 测试仍通过但本测试会失败
// 暴露缺陷。两条分支独立覆盖，确证 AvoidEntityGoal 对猫科两种实体均生效。
//
// C++ 链路：CreeperEntity::registerGoals 注册 AvoidEntityGoal（优先级3，avoidDistance=6.0 检测距离，
// farSpeed=1.0 nearSpeed=1.2）。谓词同时匹配 CAT 和 OCELOT 两种实体类型。苦力怕检测到 6 格内豹猫时
// 主动逃离。豹猫对苦力怕无攻击行为（OcelotEntity registerGoals 无针对苦力怕的 goal）。
//
// 环境选择：grass_pen（9×5×9，玻璃墙围栏）。与 creeper_flees_from_cat 同结构同布局——
// 豹猫于 (2,2,2)，苦力怕于 (2,2,3)，距 1 格 < 6 检测距离。grass_pen 玻璃墙把苦力怕限制在内部空气腔，
// 防逃离出查询区。苦力怕无 RestrictSun/FleeSun goal，day/night 均可，用默认 day 批次。
// 豹猫 spawn 默认未驯服；本测试无需玩家（苦力怕 AvoidEntityGoal 是苦力怕主动检测豹猫并逃离，
// 不依赖玩家触发），故不 spawn 玩家，排除豹猫 AvoidPlayerGoal 干扰。
//
// 判定手段：苦力怕从豹猫旁(2,2,3)逃离，断言苦力怕距豹猫水平距离 > 4 格（初始 1 格→逃离后 >4 格）。
// 用 getEntities 区域限定取苦力怕世界坐标计算与豹猫世界坐标的水平距离。逃离方向虽随机（朝远离豹猫
// 的任意方向），但只要距豹猫 >4 格即证明 AvoidEntityGoal 的 OCELOT 分支驱动了位移。
// grass_pen 9×9 对角 ~11 格，苦力怕逃离到 >4 格容易满足。判定距离阈值（而非固定区域）规避逃离方向
// 随机导致的 flaky。逃离速度 1.0+，拉开 4 格约需 20-40 tick，maxTicks=1000 留充裕余量吸收非确定性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#行为（逃离豹猫）
function creeperFleesFromOcelot(test: Test): void {
  const creeperType = "creeper";
  const ocelotType = "ocelot";

  // 豹猫于 (2,2,2)（grass_pen 内部空气腔一角），苦力怕于 (2,2,3)（紧邻豹猫，距 1 格 < 6 检测距离）。
  // 苦力怕立即检测到豹猫并朝远离方向逃离。grass_pen 玻璃墙防苦力怕逃出查询区。
  // grass_pen y=0 满铺 grass_block，苦力怕 helper-y=2→结构内 y=1 空气，脚踩 y=0 草地，无需补支撑。
  test.spawn(ocelotType, { x: 2, y: 2, z: 2 });
  test.spawn(creeperType, { x: 2, y: 2, z: 3 });

  // 豹猫世界坐标（helper (2,2,2) 经 worldLocation 转换）。
  const ocelotWorld = test.worldLocation({ x: 2, y: 2, z: 2 });

  // 断言苦力怕逃离：距豹猫水平距离 > 4 格（初始 1 格，逃离后应 >4 格）。
  // 区域限定用 PEN（grass_pen 9×5×9）排除并行测试污染。
  // 逃离速度 1.0+，拉开 4 格约需 20-40 tick，maxTicks=1000 留充裕余量吸收非确定性。
  test.succeedWhen(() => {
    const creepers = test.getDimension().getEntities({
      type: creeperType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(creepers.length > 0, "creeper disappeared");
    const c = creepers[0];
    const dx = c.location.x - ocelotWorld.x;
    const dz = c.location.z - ocelotWorld.z;
    test.assert(dx * dx + dz * dz > 4 * 4,
      `creeper did not flee from ocelot, distSq=${dx * dx + dz * dz}`);
  });
}

// 苦力怕不攻击非玩家生物（wiki tech_苦力怕.txt#行为：苦力怕追逐玩家；受击时反击但优先玩家）。
// 苦力怕 NearestAttackableTargetGoal 仅匹配 PLAYER（CreeperEntity.cpp:299-303 谓词），不主动选
// 其他生物为 attackTarget，故对非玩家生物不膨胀不爆炸。
//
// 与 creeper_swell_explodes 形成对照：同结构 creeper_pit，玩家近身(<3格)会膨胀爆炸，
// 而非玩家生物(牛)近身不膨胀——验证 targetSelector 的 Player-only 约束。
//
// 判定手段：苦力怕与牛紧邻(<3格)，若干 tick 后苦力怕仍存活（未爆炸 remove）即证明未对牛膨胀。
// 苦力怕无阳光燃烧（shouldBurnInDaylight=false）、牛不攻击苦力怕，环境安全，存活确定。
// 注意：苦力怕 HurtByTargetGoal(优先级2) 仅在受击时触发反击，牛不攻击苦力怕故不触发；
// 且即便触发也只设 attackTarget，CreeperSwellGoal 仍需 distSq<9，牛在旁虽<9 但 HurtByTarget
// 未激活（牛未攻击），attackTarget 仍为 null（NearestAttackableTargetGoal 不选牛），苦力怕不膨胀。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#行为（追逐玩家）
function creeperDoesNotSwellWithoutPlayerTarget(test: Test): void {
  const creeperType = "creeper";
  const cowType = "cow";

  // 苦力怕 (3,2,3)、牛 (4,2,3)，紧邻 1 格 < 3 膨胀触发距离。但牛非玩家，苦力怕不选其为 attackTarget。
  // creeper_pit 开放坑 y=0 草地，苦力怕/牛脚踩草地（helper-y=2→结构内 y=1 空气）。
  test.spawn(creeperType, { x: 3, y: 2, z: 3 });
  test.spawn(cowType, { x: 4, y: 2, z: 3 });

  // 等待足够 tick（远超 fuseTime 30 tick + 余量），确认苦力怕未爆炸（仍存活）。
  // 用 runAtTickTime 在 tick 100（>> fuseTime 30）断言苦力怕存活后 succeed。
  // 若苦力怕误对牛膨胀，tick 30 前后即爆炸 remove，tick 100 时 assertEntityPresentInArea(false) 失败。
  test.runAtTickTime(100, () => {
    test.assertEntityPresentInArea(creeperType, true);
    test.succeed();
  });
}

// 打火石右键点燃苦力怕（wiki tech_苦力怕.txt#引爆：玩家可使用打火石或火焰弹点燃苦力怕，
// 点燃后苦力怕会进入引爆倒计时并爆炸）。
//
// C++ 链路（对齐 Java 1.21.11 Creeper.mobInteract）：
//   玩家主手持 flint_and_steel + interactWithEntity(creeper)（ScriptSimulatedPlayer 扩展绑定）
//   → Player::interactOn(creeper, MainHand)（Player.cpp:2843）
//   → creeper.processInitialInteract → MobEntity::processInitialInteract（MobEntity.cpp:639）
//     → 命名牌/刷怪蛋/拴绳/剪刀装备分支均不命中 → interactMob(player, hand)（MobEntity.cpp:753）
//   → CreeperEntity::interactMob override（对齐 Java Creeper.mobInteract）：
//     heldItem.getItem()=flint_and_steel ∈ ItemTags::CREEPER_IGNITERS → 播 FLINTANDSTEEL_USE 音效 +
//     ignite()（置 m_ignited=true）+ 可损坏物品 hurtAndBreak(1, player, MainHand) 扣耐久。
//   → CreeperEntity::tick：hasIgnited()=true → setCreeperState(1) → m_timeSinceIgnited 每 tick +1，
//     达 m_fuseTime(30 tick) 后 explode() → remove()。
//
// 此前 Cubium CreeperEntity 无 interactMob override（基类 MobEntity::interactMob 返 Pass），
// Player::interactOn 第3步返 Pass 后第4步走 Item::itemInteractionForEntity——而 FlintAndSteelItem
// 未 override itemInteractionForEntity（只有 onItemUse 处理方块点燃），故打火石右键苦力怕完全不点燃
// （对齐缺陷）。本次新增 CREEPER_IGNITERS 标签 + CreeperEntity::interactMob override 补全此链路。
//
// 环境选择：grass_pen（9×5×9 玻璃墙围栏）。
//   - 关键：玩家须远离苦力怕 > 7 格（CreeperSwellGoal 取消距离 distSq>49），避免苦力怕因玩家靠近
//     而 CreeperSwellGoal 自发膨胀，干扰"打火石点燃"的归因。grass_pen 9×9 对角 ~11 格，
//     苦力怕 (1,2,1) + 玩家 (7,2,7)，水平距 ~8.5 格 > 7，CreeperSwellGoal 不触发自发膨胀。
//   - 玻璃墙挡 NearestAttackableTargetGoal checkSight 射线（grass_pen 外圈玻璃），苦力怕看不到玩家
//     → 不选玩家为 attackTarget → 进一步保证不自发膨胀。只有打火石点燃（m_ignited=true）才引爆。
//   - interactWithEntity 无距离门控（远程转发 interactOn），玩家可在 8.5 格外右键苦力怕。
//   - 玩家用创造模式（默认）：点燃不依赖苦力怕选玩家为目标（玩家主动右键触发 interactMob），
//     创造模式跳过打火石耐久消耗（interactMob 内 creativeMode 守卫），同一打火石可重复测试。
//
// 判定手段：点燃后苦力怕引信 30 tick 爆炸 remove，succeedWhen 断言苦力怕实体消失。
//   assertEntityPresentInArea(creeper, false) 扫描结构 bounds 查实体，爆炸破坏草地/玻璃不影响该断言。
//   爆炸半径 3，玩家距 8.5 格不受伤害。
// 时序：tick 5 interactWithEntity（留 5 tick 让 spawn 注册稳定）→ tick 6 起 m_timeSinceIgnited 累加
//   → tick ~36 explode() remove()。maxTicks=200 余量充足。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#引爆（打火石/火焰弹点燃）
function creeperIgnitedByFlintAndSteel(test: Test): void {
  const creeperType = "creeper";

  // 苦力怕 (1,2,1)（grass_pen 内角，脚踩 y=0 草地，helper-y=2→结构内 y=1 空气）。
  // 玩家 (7,2,7)（对角，距苦力怕水平 ~8.5 格 > 7 CreeperSwellGoal 取消距离，不自发膨胀）。
  const creeper = test.spawn(creeperType, { x: 1, y: 2, z: 1 });
  const player = test.spawnSimulatedPlayer({ x: 7, y: 2, z: 7 }, "igniter");

  // 创造玩家主手持打火石。setItem(slot=0 主手, true 强制覆盖)。
  // ItemStack 类型分裂（顶层 vs server-gametest 嵌套），as unknown 强转绕过编译期（见 CowBreedTests 同款）。
  const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);
  player.setItem(flintAndSteel as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5：持打火石右键苦力怕 → interactMob → ignite()。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(creeper);
  });

  // 断言苦力怕被点燃后爆炸消失：引信 30 tick 后 explode()→remove()。
  // 若 interactMob 链路断裂（打火石未点燃苦力怕），苦力怕不爆炸，succeedWhen 超时失败暴露缺陷。
  test.succeedWhen(() => {
    test.assertEntityPresentInArea(creeperType, false);
  });
}

// 火焰弹右键点燃苦力怕（wiki tech_苦力怕.txt#引爆：火焰弹亦可点燃苦力怕）。
//
// 与 creeper_ignited_by_flint_and_steel 形成对照：同 CREEPER_IGNITERS 标签的另一成员，
// 验证火焰弹分支（不可损坏物品 shrink 消耗，而非打火石的 hurtAndBreak 耐久）也能点燃苦力怕。
// C++ 链路同上，区别仅 interactMob 内 item->isDamageable()=false（火焰弹 maxDamage=0）→ 走 shrink(1)
// 而非 hurtAndBreak（创造模式均跳过消耗，不影响点燃逻辑）。
//
// 此测试覆盖 CREEPER_IGNITERS 标签完整性：打火石 + 火焰弹两成员均能点燃苦力怕。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_苦力怕.txt#引爆（火焰弹点燃）
function creeperIgnitedByFireCharge(test: Test): void {
  const creeperType = "creeper";

  const creeper = test.spawn(creeperType, { x: 1, y: 2, z: 1 });
  const player = test.spawnSimulatedPlayer({ x: 7, y: 2, z: 7 }, "igniter2");

  const fireCharge = new ItemStack("minecraft:fire_charge", 1);
  player.setItem(fireCharge as unknown as Parameters<typeof player.setItem>[0], 0, true);

  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(creeper);
  });

  test.succeedWhen(() => {
    test.assertEntityPresentInArea(creeperType, false);
  });
}

export function registerCreeperTests(): void {
  GameTest.register("MobBehaviorTests", "creeper_swell_explodes", creeperSwellExplodes)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "creeper_charged_by_lightning", creeperChargedByLightning)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "creeper_flees_from_cat", creeperFleesFromCat)
    .structureName("gametests:grass_pen")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "creeper_flees_from_ocelot", creeperFleesFromOcelot)
    .structureName("gametests:grass_pen")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "creeper_does_not_swell_without_player_target", creeperDoesNotSwellWithoutPlayerTarget)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "creeper_ignited_by_flint_and_steel", creeperIgnitedByFlintAndSteel)
    .structureName("gametests:grass_pen")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "creeper_ignited_by_fire_charge", creeperIgnitedByFireCharge)
    .structureName("gametests:grass_pen")
    .maxTicks(200);
}
