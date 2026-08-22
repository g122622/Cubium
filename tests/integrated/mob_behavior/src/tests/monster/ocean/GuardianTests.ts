// 守卫者行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// open_grass_hall 结构尺寸（41×7×9，y=0..6），helper 相对坐标。四壁 glass 墙 + 内部全 air 露天草地长廊。
// 美西螈垂直布局测试用：垂距 4 格需 y 范围 ≥5（守卫者 y=2 + 美西螈 y=6），creeper_pit y=0..4 不够。
const HALL_FROM = { x: 0, y: 0, z: 0 };
const HALL_VOLUME = { x: 41, y: 7, z: 9 };

// 守卫者激光攻击玩家致掉血（wiki tech_守卫者.txt#攻击：守卫者会主动攻击约 16 格激光射程内的
// 玩家、鱿鱼、发光鱿鱼、美西螈。激光需几秒充能，充满后突然消失并对目标造成连续两次伤害：
// 一次魔法伤害（不被护甲降低），一次生物攻击伤害（被护甲降低））。
//
// C++ 链路：GuardianEntity : MonsterEntity，registerGoals 注册
// NearestAttackableTargetGoal<LivingEntity>(checkSight=true，谓词只放行 Player/Squid，距离平方>9)（优先级1）+
// GuardianAttackGoal（激光攻击，优先级4）。GuardianAttackGoal::tick：m_tickCounter 从 -10 递增到 80，
// 到 80 时结算伤害——先 magic() 魔法伤害 LASER_DAMAGE(4.0)，再 mobAttack() 物理伤害 ATTACK_DAMAGE(4.0)。
// 完整周期约 90 tick（10 准备 + 80 充能）。激光是确定性命中（无散布），不像烈焰人火球。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 守卫者(2,2,3)+玩家(6,2,3)，水平距 4 格，distSq=16，>9 走激光分支（C++ 无 15 格硬上限，靠 FOLLOW_RANGE=16 搜索）。
// 守卫者陆地不窒息（wiki#行为：在空气中不会窒息，陆地可一直存活），C++ LivingEntity::updateAirSupply
// 不在水中时恢复空气，故陆地测试守卫者不会溺水死亡。
//
// 判定手段：断言玩家 HP 下降（<20）。激光确定性命中（无 nextGaussian 散布），双段伤害魔法4+物理4，
// 玩家初始满血 20，被命中即掉至 <20。与烈焰人火球不同，激光无需散布概率担忧，掉血断言稳定。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 谓词滤掉不可被攻击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_守卫者.txt#攻击（激光充能后双段伤害）
function guardianLaserDamagesPlayer(test: Test): void {
  const guardianType = "guardian";

  // 守卫者 (2,2,3)、Survival 玩家 (6,2,3)，水平距 4 格，同处结构 y=2 层。
  // 守卫者受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (6,1,3) 放玻璃。
  // 距离 4 格 >3（避开 shouldContinueExecuting 的 distSq<=9 近距离停止判定）且 <15（激光射程内）。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(guardianType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 6, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + GuardianAttackGoal 准备 10 tick + 充能 80 tick + 结算伤害。
  // 完整周期约 90 tick，maxTicks=500 留充裕余量（守卫者攻击前可能先游荡几 tick 选目标）。
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
      `guardian did not damage player with laser, hp=${(health as any).currentValue}`);
  });
}

// 守卫者不在阳光下燃烧（wiki tech_守卫者.txt#行为：守卫者离开水会扑腾但不窒息，陆地可一直存活；
// 守卫者虽是 MonsterEntity 子类，但 shouldBurnInDaylight() override 返回 false）。
//
// 与 zombie_burns_in_daylight（僵尸燃）+ blaze_does_not_burn_in_daylight（烈焰人不燃）对照：
// 僵尸验证 MonsterEntity 默认 shouldBurnInDaylight=true 对基础亡灵生效，守卫者验证水生怪物
// override false 跳过燃烧。C++ 链路：MonsterEntity::tick→handleDaylightBurning→shouldBurnInDaylight()
// 虚函数调度（handleDaylightBurning 调虚函数而非读成员，使 override 生效），守卫者 override 返回
// false 跳过燃烧判定。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_守卫者.txt#行为（陆地不窒息、不燃烧）
function guardianDoesNotBurnInDaylight(test: Test): void {
  const guardianType = "guardian";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 守卫者 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const guardian = test.spawn(guardianType, { x: 4, y: 2, z: 4 });

  // 白天露天守卫者不着火：轮询 onfire 组件，应恒 undefined（shouldBurnInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，守卫者本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），若有框架 bug 让所有实体不着火测试也过——但有
  // zombie_burns_in_daylight 正向断言对照（亡灵该着火着火），互补验证。
  test.succeedWhen(() => {
    const fire = guardian.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("guardian should not burn in daylight");
    }
  });
}

// 守卫者激光攻击鱿鱼（wiki tech_守卫者.txt#攻击：守卫者会主动攻击约 16 格激光射程内的玩家、
// 鱿鱼、发光鱿鱼和美西螈）。
//
// C++ 链路：GuardianEntity registerGoals（GuardianEntity.cpp:130-163）targetSelector 优先级1
//   NearestAttackableTargetGoal<LivingEntity>(checkSight=true, chance=10)，谓词（:135-162）：
//     类型筛选 isPlayer||isSquid（:142-146）——**鱿鱼分支 isSquid（:143）放行鱿鱼，无 Creative/
//     Spectator 检查（鱿鱼非玩家不走 :149-154 玩家特殊分支）**；距离筛选 distSq>9.0（>3 格，:157-160）。
//   GuardianAttackGoal::tick（GuardianAttackGoal.cpp）m_tickCounter 从 -10 递增到 80（ATTACK_DURATION），
//   到 80 时结算：先 magic() LASER_DAMAGE(4.0) 魔法伤害，再 mobAttack() ATTACK_DAMAGE(4.0) 物理伤害。
//   完整周期约 90 tick（10 准备 + 80 充能），激光确定性命中无散布。
//   注：谓词只放行 SQUID，**未放行 GLOW_SQUID/AXOLOTL**（与 wiki 列举的发光鱿鱼/美西螈不符，对齐缺陷，
//   留待后续对齐任务）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
//   守卫者(2,2,3) 脚下 (2,1,3) 玻璃支撑（MonsterEntity 受重力下落）；鱿鱼(6,2,3) 脚下 (6,1,3) 玻璃。
//   水平距 4 格，distSq=16 > 9（走激光分支）且 < FOLLOW_RANGE=16（搜索范围内）。鱿鱼陆地"无法移动"
//   位置固定，守卫者激光远射程无需寻路接近。
//
// 鱿鱼陆地窒息时序（关键约束，照搬 axolotlAttacksSquid 范式）：鱿鱼 : WaterMobEntity，
//   updateAirSupply 陆地 air-1，air<=-20 时 hurt(drown,2.0)。鱿鱼 maxAir=300，首窒息在 ~320 tick
//   （air 300→0 耗 300 tick + 0→-20 再 20 tick，见 SquidTests 同款链路）。激光周期 ~90 tick + 选目标
//   chance=10 tick，首击应在 ~100-120 tick。**maxTicks=300 < 320**——确保测试窗口内鱿鱼窒息尚未触发，
//   掉血只能来自守卫者激光，断言纯粹验证 isSquid 谓词分支 + 激光链路。
//
// 判定手段：succeedWhen 每 tick 检查鱿鱼 HP<10（满血 10，激光双段 4+4=8 命中后 10→2）或鱿鱼已死亡消失
//   （length==0，被多击杀死）。激光确定性命中（无散布），掉血断言稳定。区域限定查鱿鱼排除并行测试污染。
//   不需 Survival 玩家（鱿鱼非玩家，谓词不过滤游戏模式），无需 spawn 玩家。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_守卫者.txt#攻击（激光攻击鱿鱼/发光鱿鱼/美西螈）
function guardianLaserDamagesSquid(test: Test): void {
  const guardianType = "guardian";
  const squidType = "squid";

  // 守卫者 (2,2,3)、鱿鱼 (6,2,3)，水平距 4 格，同处结构 y=2 层。
  // 守卫者脚下 (2,1,3) 玻璃支撑（受重力下落）；鱿鱼脚下 (6,1,3) 玻璃。
  // 距离 4 格 >3（满足谓词 distSq=16>9）且 <16（FOLLOW_RANGE 搜索范围内）。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(guardianType, { x: 2, y: 2, z: 3 });
  test.spawn(squidType, { x: 6, y: 2, z: 3 });

  // 断言鱿鱼掉血：succeedWhen 每 tick 检查鱿鱼 HP<10 或已死亡消失。
  // 时序：NearestAttackableTarget isSquid 谓词选鱿鱼(chance=10tick) + GuardianAttackGoal 准备 10 tick
  //   + 充能 80 tick + 结算双段伤害(4+4=8)。完整周期约 90 tick，首击应在 ~100-120 tick。
  //   maxTicks=300 < 320 窒息线——鱿鱼窒息尚未触发，掉血必来自守卫者激光（排除窒息干扰）。
  //   鱿鱼查询用区域限定排除并行测试的鱿鱼污染；type 用 "minecraft:squid"。
  test.succeedWhen(() => {
    const squids = test.getDimension().getEntities({
      type: squidType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 鱿鱼已被守卫者激光打死消失——攻击行为生效。
    if (squids.length === 0) {
      return;
    }
    const health = squids[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "squid has no health component");
    test.assert((health as any).currentValue < 10,
      `guardian did not damage squid with laser, hp=${(health as any).currentValue}`);
  });
}

// 守卫者激光攻击发光鱿鱼（wiki tech_守卫者.txt#攻击：守卫者会主动攻击约 16 格激光射程内的玩家、
// 鱿鱼、发光鱿鱼和美西螈）。
//
// 本测试专项验证谓词的 GLOW_SQUID 分支。Cubium 谓词此前用 entityType()==SQUID 精确匹配，
// 排除了 GlowSquid（其 entityType 是 GLOW_SQUID）——对齐缺陷（vanilla Java instanceof Squid
// 涵盖 GlowSquid extends Squid）。本次修复谓词显式放行 GLOW_SQUID（GuardianEntity.cpp:148-149）。
//
// 链路同 guardian_laser_damages_squid：NearestAttackableTargetGoal 谓词 isSquid 分支（含 GLOW_SQUID）
// 选目标 + GuardianAttackGoal 激光双段伤害 magic 4 + 物理 4 = 8。
//
// 环境与约束同 guardian_laser_damages_squid：creeper_pit，守卫者(2,2,3)+发光鱿鱼(6,2,3) 水平距 4 格，
// 脚下玻璃支撑。发光鱿鱼 : WaterMobEntity，maxAir=300（同鱿鱼），陆地窒息首伤 ~320 tick，
// maxTicks=300 < 320 规避窒息。发光鱿鱼满血 10，激光 8 伤害命中后 10→2 < 10。
// 判定 glow_squid HP<10 或 length==0，区域限定排除并行污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_守卫者.txt#攻击（激光攻击发光鱿鱼）
function guardianLaserDamagesGlowSquid(test: Test): void {
  const guardianType = "guardian";
  const glowSquidType = "glow_squid";

  // 守卫者 (2,2,3)、发光鱿鱼 (6,2,3)，水平距 4 格（distSq=16>9 满足谓词，<FOLLOW_RANGE=16）。
  // 脚下玻璃支撑（守卫者受重力下落；发光鱿鱼陆地静止）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 3 });
  test.spawn(guardianType, { x: 2, y: 2, z: 3 });
  test.spawn(glowSquidType, { x: 6, y: 2, z: 3 });

  // 断言发光鱿鱼掉血：HP<10 或已死亡消失。激光周期 ~90 tick，首击 ~100-120 tick。
  // maxTicks=300 < 320 窒息线——掉血必来自守卫者激光（排除窒息干扰）。
  test.succeedWhen(() => {
    const glowSquids = test.getDimension().getEntities({
      type: glowSquidType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (glowSquids.length === 0) {
      return;
    }
    const health = glowSquids[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "glow_squid has no health component");
    test.assert((health as any).currentValue < 10,
      `guardian did not damage glow_squid with laser, hp=${(health as any).currentValue}`);
  });
}

// 守卫者激光攻击美西螈（wiki tech_守卫者.txt#攻击：守卫者攻击玩家、鱿鱼、发光鱿鱼和美西螈）。
//
// 本测试验证谓词的 AXOLOTL 分支。Cubium 谓词此前完全缺 AXOLOTL（对齐缺陷，vanilla instanceof Axolotl）。
// 本次修复显式放行 AXOLOTL（GuardianEntity.cpp:150）。
//
// 链路同 guardian_laser_damages_squid：谓词 isAxolotl 选目标 + GuardianAttackGoal 激光双段 8 伤害。
// 美西螈满血 14，激光 8 命中后 14→6；两击致死（6→-2 消失）。
//
// 关键设计——垂直+水平偏移布局规避两类失败（前期水平布局失败：美西螈与守卫者 RandomWalking
//   碰近 distSq<9 触发谓词"距离太近不攻击"全程不掉血；正上方垂直布局偶发失败：美西螈脚下支撑
//   玻璃挡垂直射线致 canSee 失败锁不到目标）：
//   - 美西螈与守卫者垂直分层 + 水平偏移：垂距 4 格（distSq=16>9 满足谓词）+ 水平偏 4 格，3D 距 √32≈5.66>3。
//     美西螈水平 RandomWalking 改变 x/z 不改变 y，垂距恒定，距离恒满足谓词。
//   - 守卫者 (20,2,4) 站玻璃顶（下方 (20,1,4) 玻璃支撑防重力下落），美西螈 (24,6,4) 站玻璃顶
//     （下方 (24,5,4) 玻璃支撑）。垂距 4 格，水平偏 4 格。射线 (20,2.6,4)→(24,6.8,4) 斜向上
//     穿 (21,3,4)(22,4,4)(23,5,4) 全 air，视线无阻挡（canSee raycastBlocks 命中 air 不挡）。
//     **水平偏移避免美西螈支撑玻璃挡视线**——正上方时垂直射线穿 (20,5,4) 玻璃致 canSee false。
//   - 守卫者 GuardianAttackGoal flag={Move,Look}，shouldExecute 锁定目标后占 Move flag，RandomWalking
//     被抢占不移动，激光连续充能 90 tick 命中（解决水平布局守卫者游走充能中断问题）。
//   - open_grass_hall（41×7×9，y=0..6）提供足够 y 范围（creeper_pit y=0..4 不够 4 格垂距）。
//     x=20/24 在 41 长度中部，美西螈水平游走不撞墙（墙在 x=0/40）。
//
// 环境约束：
//   - 美西螈陆地 maxAir=6000，560 tick 远不窒息（3000 tick 才窒息），无窒息干扰。
//   - 守卫者陆地不窒息（GuardianTests 同款验证）。
//   - 美西螈水平游走可能在 x 方向移动，但垂距不变，攻击持续；若美西螈游走出查询区域则 length==0，
//     但 open_grass_hall 玻璃墙在 x=0/40 限制美西螈在 41 长度内，HALL_VOLUME 41×7×9 覆盖全结构，
//     美西螈不会出查询区域。
//
// 判定手段：pollUntilSucceed 每 tick 检查美西螈 HP<14（受击 14→6）或 length==0（被打死消失）。
//   激光周期 ~90 tick，首击 ~100-120，两击致死 ~200 tick。maxTick=560 留余量。
//   区域限定 HALL_VOLUME 查美西螈排除并行污染。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_守卫者.txt#攻击（激光攻击美西螈）
function guardianLaserDamagesAxolotl(test: Test): void {
  const guardianType = "guardian";
  const axolotlType = "axolotl";

  // 垂直+水平偏移布局：守卫者 (20,2,4) 站 (20,1,4) 玻璃顶；美西螈 (24,6,4) 站 (24,5,4) 玻璃顶。
  // 垂距 4 格（y=2 到 y=6）+ 水平偏 4 格（x=20 到 x=24），3D 距 √32≈5.66>3 满足谓词。
  // **关键：水平偏移避免美西螈支撑玻璃挡视线**——若美西螈正上方（同 x,z）于守卫者，垂直射线穿过
  //   美西螈脚下玻璃 (20,5,4) 致 canSee raycast 命中玻璃返回 false，守卫者锁不到目标全程不掉血
  //   （早期正上方布局 8 次 1 失败，诊断 hp=14 视线被挡）。水平偏移后射线 (20,2.6,4)→(24,6.8,4)
  //   斜向上穿 (21,3,4)(22,4,4)(23,5,4) 全 air，视线通畅。
  // 守卫者 GuardianAttackGoal flag={Move,Look}，锁定目标后占 Move flag 不移动（RandomWalking 被抢占），
  //   激光连续充能 90 tick 命中。
  test.setBlockType("minecraft:glass", { x: 20, y: 1, z: 4 });
  test.setBlockType("minecraft:glass", { x: 24, y: 5, z: 4 });
  test.spawn(guardianType, { x: 20, y: 2, z: 4 });
  test.spawn(axolotlType, { x: 24, y: 6, z: 4 });

  // 断言美西螈掉血或被打死：HP<14（受击 14→6）或 length==0（被打死消失）。
  // 垂距恒 4 格，激光持续命中。美西螈 maxAir=6000 不窒息。首击 ~100-120 tick，两击致死 ~200 tick。
  pollUntilSucceed(test, () => {
    const axolotls = test.getDimension().getEntities({
      type: axolotlType,
      location: test.worldLocation(HALL_FROM),
      volume: HALL_VOLUME,
    });
    if (axolotls.length === 0) return true;
    const health = axolotls[0].getComponent("minecraft:health") as any;
    if (health === undefined) return false;
    return health.currentValue < 14;
  }, {
    maxTick: 560,
    onTimeout: () => {
      const axolotls = test.getDimension().getEntities({
        type: axolotlType,
        location: test.worldLocation(HALL_FROM),
        volume: HALL_VOLUME,
      });
      const guardians = test.getDimension().getEntities({
        type: guardianType,
        location: test.worldLocation(HALL_FROM),
        volume: HALL_VOLUME,
      });
      const aHp = axolotls.length > 0
        ? (axolotls[0].getComponent("minecraft:health") as any)?.currentValue : "gone";
      const aPos = axolotls.length > 0
        ? `(${axolotls[0].location.x.toFixed(1)},${axolotls[0].location.y.toFixed(1)},${axolotls[0].location.z.toFixed(1)})`
        : "gone";
      const gPos = guardians.length > 0
        ? `(${guardians[0].location.x.toFixed(1)},${guardians[0].location.y.toFixed(1)},${guardians[0].location.z.toFixed(1)})`
        : "gone";
      const dist = (axolotls.length > 0 && guardians.length > 0)
        ? Math.hypot(axolotls[0].location.x - guardians[0].location.x,
            axolotls[0].location.y - guardians[0].location.y,
            axolotls[0].location.z - guardians[0].location.z) : -1;
      test.assert(false,
        `guardian did not damage axolotl (axolotl=${axolotls.length} hp=${aHp} pos=${aPos}; guardian=${guardians.length} pos=${gPos}; dist3d=${dist.toFixed(2)})`);
    },
  });
}

export function registerGuardianTests(): void {
  GameTest.register("MobBehaviorTests", "guardian_laser_damages_player", guardianLaserDamagesPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "guardian_does_not_burn_in_daylight", guardianDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "guardian_laser_damages_squid", guardianLaserDamagesSquid)
    .structureName("gametests:creeper_pit")
    .maxTicks(300);

  GameTest.register("MobBehaviorTests", "guardian_laser_damages_glow_squid", guardianLaserDamagesGlowSquid)
    .structureName("gametests:creeper_pit")
    .maxTicks(300);

  GameTest.register("MobBehaviorTests", "guardian_laser_damages_axolotl", guardianLaserDamagesAxolotl)
    .structureName("gametests:open_grass_hall")
    .maxTicks(600);
}
