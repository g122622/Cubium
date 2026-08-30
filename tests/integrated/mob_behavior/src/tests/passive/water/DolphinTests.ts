// 海豚行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";
import { fillBlock } from "../../../utils/block/build.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
// grass_pen 结构尺寸（9×5×9），helper 相对坐标。retaliates 测试专用查询范围。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 海豚受击后反击玩家（wiki tech_海豚.txt#攻击：海豚是中性生物，被攻击后会反击攻击者）。
//
// C++ 链路：DolphinEntity : WaterMobEntity（非 AnimalEntity）。registerGoals（DolphinEntity.cpp:267-320）：
//   targetSelector 优先级1：HurtByTargetGoal(this, true, predicate)——受击后设 attackTarget=攻击者
//     并呼叫附近同类（alertAllies=true，DolphinEntity.cpp:313-319）。谓词只排除 Guardian/ElderGuardian，
//     玩家攻击者不被排除，会反击（HurtByTargetGoal::shouldExecute 读 getLastHurtBy 判 isSuitableTarget，
//     Survival 玩家可通过，Creative/Spectator 被滤掉）。
//   goalSelector 优先级6：MeleeAttackGoal(this, 1.2, true)——speed=1.2，longMemory=true，
//     shouldExecute 读 attackTarget，寻路接近到攻击距离内 _attackTarget 读 ATTACK_DAMAGE(3.0)+hurt(玩家)。
//   注意：海豚未注册 NearestAttackableTargetGoal<Player>（targetSelector 仅 HurtByTargetGoal 一个），
//   故海豚不会主动攻击玩家，仅在受击后才反击——这正是本测试要验证的"受击后敌对攻击玩家"。
// registerAttributes（DolphinEntity.cpp:322-334）：MAX_HEALTH=10.0, ATTACK_DAMAGE=3.0。
//   MeleeAttackGoal::getAttackReachSqr=(attacker.width*2)²+target.width=(0.9*2)²+0.6=3.84，
//   开方约 1.96 格——玩家需在 1.96 格内。命中后 _attackTarget 读 ATTACK_DAMAGE(3.0) 并 hurt 玩家。
//   冷却 ATTACK_COOLDOWN_TICKS=20，经 adjustedTickDelay 减半约 10 tick。
//
// 陆地可行性（关键约束排除）：DolphinEntity : WaterMobEntity，陆地上每 tick 调 updateAirSupply 消耗
//   空气（WaterMobEntity::updateAirSupply，水生反逻辑：水中 setAir(maxAir) 回满，陆地 air-1，
//   air<=-20 时 hurt(drown,2.0)）。海豚 MAX_AIR=4800（DolphinEntity.cpp:99 setAir(4800)），
//   air 4800→0 耗 4800 tick + 0→-20 再 20 tick，首次窒息伤害在第 ~4820 tick。maxTicks=800 远小于此，
//   测试窗口内海豚完全不窒息掉血——窒息不干扰断言，掉血只能来自玩家攻击海豚（不影响海豚存活）。
//   注：海豚 DATA_MOISTNESS_LEVEL_PARAM 默认 2400，但 moistness 离水递减业务联动 TODO 未实现
//   （DolphinEntity.hpp:297），陆地上不因湿润度掉血。窒息是唯一陆地掉血源。
//
// 寻路可行性：海豚 navigator 是 MobEntity 默认 GroundPathNavigation（WalkNodeProcessor），陆地可寻路
//   （未覆盖 _createDefaultNavigator，与美西螈陆地测试同范式，见 AxolotlTests.ts）。
// FindWaterGoal（优先级0，mutex={Move}）shouldExecute 需 16 格内有水，grass_pen 无水→返回 false，
//   不锁 Move flag，MeleeAttackGoal 可正常运行寻路。RandomSwimmingGoal（优先级4，mutex={Move}）陆地
//   无水 getPathWeight=0 不主动执行，不抢占 MeleeAttackGoal。
//
// 环境选择：grass_pen（9×5×9 玻璃围墙草地）。
// 海豚是水生友好生物（非亡灵/怪物），不在阳光下燃烧，白天即可反击（不 batch night）。
// 海豚(3,2,3)+Survival 玩家(5,2,3)，水平距 2 格 < 攻击半径 20。
// 玩家 tick 8/16/24/32 后多次攻击海豚，触发 HurtByTargetGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 WolfTests/PolarBearTests 同款注释）。
// 海豚被攻击后设 attackTarget=玩家，MeleeAttackGoal 寻路接近 2 格 + 攻击冷却后 hurt(玩家, 3.0)。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 3.0，玩家满血 20 → 17。
// 玩家在陆地不溺水，HP 掉血只能来自海豚攻击，断言干净（maxTicks=450 < 玩家陆地无溺水问题，
//   玩家陆地不消耗 air）。
// 时序：玩家攻击(8/16/24/32 多次) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 2 格 + 攻击冷却 + hurt(3.0)。
//   海豚 MOVEMENT_SPEED=1.2（属性值，moveController 调制后较快），2 格接近 + 冷却约需 20-50 tick，
//   maxTicks=450 留充裕余量吸收并行环境 tick 抖动。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海豚.txt#攻击（被攻击后反击攻击者）
function dolphinRetaliatesWhenAttacked(test: Test): void {
  const dolphinType = "dolphin";

  // 海豚 (3,2,3)、Survival 玩家 (5,2,3)，水平距 2 格，同处结构 y=2 层。
  // grass_pen 玻璃围墙防逃逸（玻璃透明，canSee 射线通畅）。
  const dolphin = test.spawn(dolphinType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8/16/24/32 后玩家多次攻击海豚：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定，之后每隔 8 tick
  // 攻击一次共 4 次。多次攻击确保 HurtByTargetGoal 被触发设 attackTarget=玩家（attackEntity 远程命中，
  // 基岩语义 attack can be performed at any distance，见 WolfTests/PolarBearTests 同款注释）。单次攻击在并行
  // 环境下偶发未及时触发 HurtByTargetGoal（tick 抖动），多次攻击提高可靠性。
  test.runAtTickTime(8, () => {
    player.attackEntity(dolphin);
  });
  test.runAtTickTime(16, () => {
    player.attackEntity(dolphin);
  });
  test.runAtTickTime(24, () => {
    player.attackEntity(dolphin);
  });
  test.runAtTickTime(32, () => {
    player.attackEntity(dolphin);
  });

  // 断言玩家掉血：pollUntilSucceed 轮询玩家 HP<20（海豚反击命中）。
  // 时序：玩家攻击(8/16/24/32) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(3.0)。
  // 海豚 1.2 速度接近 3 格约需 30-60 tick，maxTicks 留充裕余量（且远小于海豚陆地窒息线 4820）。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  // onTimeout 诊断：超时时打印海豚/玩家位置、距离、HP，定位是"未设目标"还是"寻路不动"还是"未命中"。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (players.length === 0) return false;
    const health = players[0].getComponent("minecraft:health") as any;
    if (health === undefined) return false;
    return health.currentValue < 20;
  }, {
    maxTick: 400,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const dolphins = test.getDimension().getEntities({
        type: "minecraft:dolphin",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const playerHp = players.length > 0
        ? (players[0].getComponent("minecraft:health") as any)?.currentValue
        : "gone";
      const dolphinHp = dolphins.length > 0
        ? (dolphins[0].getComponent("minecraft:health") as any)?.currentValue
        : "gone";
      const dist = (players.length > 0 && dolphins.length > 0)
        ? Math.hypot(players[0].location.x - dolphins[0].location.x,
            players[0].location.z - dolphins[0].location.z)
        : -1;
      const dPos = dolphins.length > 0
        ? `(${dolphins[0].location.x.toFixed(1)},${dolphins[0].location.y.toFixed(1)},${dolphins[0].location.z.toFixed(1)})`
        : "gone";
      const pPos = players.length > 0
        ? `(${players[0].location.x.toFixed(1)},${players[0].location.y.toFixed(1)},${players[0].location.z.toFixed(1)})`
        : "gone";
      test.assert(false,
        `dolphin did not retaliate (dolphin=${dolphins.length} hp=${dolphinHp} pos=${dPos}; player=${players.length} hp=${playerHp} pos=${pPos}; dist=${dist.toFixed(2)})`);
    },
  });
}

// 玩家手持生鳕鱼右键海豚喂食，海豚进入"得到鱼"状态（wiki tech_海豚.txt#行为：喂食生鳕鱼/
// 生鲑鱼/河豚/热带鱼后，海豚会游向最近的沉船或海底废墟引导玩家寻宝）。
//
// C++ 链路（对齐 Java 1.21.11 Dolphin.mobInteract）：
//   1) Survival 玩家主手持生鳕鱼 + interactWithEntity(dolphin) → Player::interactOn
//      → dolphin.processInitialInteract → MobEntity::interactMob（基类返 Pass）
//      → DolphinEntity::interactMob override（本次新增）。
//   2) DolphinEntity::interactMob：heldItem 非空且 isFoodItem（COD/SALMON/PUFFERFISH/
//      TROPICAL_FISH）→ 播 DOLPHIN_EAT 音效 + setGotFish(true)（同步 DATA_GOT_FISH_PARAM id17，
//      触发 SwimToTreasureGoal 寻宝引导）+ shrink(1) 消耗生鳕鱼（创造跳过）+ 返 Success。
//
// 此前 Cubium DolphinEntity 无 interactMob override（继承 WaterMobEntity→MobEntity 默认返 Pass），
// 玩家持鱼右键海豚无任何反应——setGotFish 永不置位，寻宝引导链路断裂（对齐缺陷）。本次补全。
// 注：Java 幼体喂鱼走 ageUp 加速成长分支，但 Cubium 海豚幼体语义未实现（BABY 占位恒 false，
//   WaterMobEntity 不经 AgeableEntity 无 isChild/ageUp，见 DolphinEntity.hpp registerData 注释），
//   海豚永远是成体，喂鱼恒走 setGotFish 分支。幼体加速成长待 AgeableWaterCreature 体系补全。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ Survival 玩家。海豚是水生友好生物（非亡灵/怪物）不在阳光下
// 燃烧，白天即可喂食（不 batch night）。海豚是中性生物（HurtByTargetGoal 仅受击反击），主动不攻击
// Survival 玩家，喂鱼环境干净。海豚陆地会缓慢消耗空气（MAX_AIR=4800，4820 tick 后才窒息），maxTicks=200
// 远小于此，海豚存活不干扰判定。interactWithEntity 远程触发无距离门控，海豚乱跑不影响喂食。
//
// 判定手段：Survival 玩家持 5 个生鳕鱼 interactWithEntity(dolphin) 后，主手槽生鳕鱼数量减少（5→4）。
// setGotFish 是 C++ 内部状态（DATA_GOT_FISH_PARAM），脚本侧无 API 直接读取；SwimToTreasureGoal 寻宝
// 需附近有沉船/海底废墟结构（creeper_pit 无），寻宝行为不可观测。故以"物品消耗"作为喂食成功的直接证据
// （对齐 Java itemstack.consume(1, player)）。创造模式不消耗，故必须 Survival 玩家。
// 读取主手槽用 getComponent("minecraft:inventory").container.getItem(0)（slot 0=主手，对齐 setItem(...,0,true)）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海豚.txt#行为（喂食鱼后引导寻宝）
function dolphinFedFishSetsGotFish(test: Test): void {
  const dolphinType = "dolphin";
  const COD_ITEM = "minecraft:cod";
  const COD_COUNT = 5;

  // 海豚 (3,2,3)（creeper_pit y=0 石头地板，helper y=2→结构 y=1 空气，脚踩 y=0 石头，无需玻璃支撑）。
  const dolphin = test.spawn(dolphinType, { x: 3, y: 2, z: 3 });
  // Survival 玩家 (1,2,3) 持 5 个生鳕鱼（slot 0 主手）。距海豚 2 格（interactWithEntity 远程触发无距离门控）。
  // Survival 模式喂食会消耗生鳕鱼（创造跳过），消耗是喂食成功的判定证据。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "dolphinFeeder", 0 as any);
  const cod = new ItemStack(COD_ITEM, COD_COUNT);
  player.setItem(cod as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持生鳕鱼 interactWithEntity(dolphin) → DolphinEntity::interactMob → setGotFish + shrink(1)。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(dolphin);
  });

  // 轮询：主手槽生鳕鱼数量从 5 减至 4（消耗 1 个）。喂食 tick 5 后下一 tick 即可查（shrink 同步生效）。
  pollUntilSucceed(test, () => {
    const inv = player.getComponent("minecraft:inventory") as any;
    if (inv === undefined || inv.container === undefined) return false;
    const mainHand = inv.container.getItem(0);
    if (mainHand === undefined) return false;
    return mainHand.typeId === COD_ITEM && mainHand.amount === COD_COUNT - 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const inv = player.getComponent("minecraft:inventory") as any;
      const mainHand = (inv?.container?.getItem?.(0)) as any;
      const amt = mainHand?.amount;
      const typeId = mainHand?.typeId;
      const dolphins = test.getDimension().getEntities({
        type: "minecraft:dolphin",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `dolphin feeding did not consume cod: mainHand={typeId:${typeId}, amount:${amt}} (expected ${COD_ITEM} x${COD_COUNT - 1}); dolphins=${dolphins.length}`);
    },
  });
}

// 海豚跟随游泳玩家并给予"海豚的恩惠"效果（wiki tech_海豚.txt#行为：玩家在水中游泳时，
// 海豚会游到玩家身边并给予海豚的恩惠效果，提升玩家游泳速度）。
//
// C++ 链路（对齐 Java 1.21.11 Dolphin SwimWithPlayerGoal，DolphinGoals.cpp:397-520）：
//   1) 玩家在水中眼睛浸没 → Player::isActualSwimming（Player.cpp:2118-2123）
//      areEyesInWater() && isInWater() && !flying 返回 true → updateSwimming（Player.cpp:2125-2141，
//      每 tick 调）setSwimming(isActualSwimming()) 设 m_isSwimming=true + Swimming flag + Pose。
//      **Cubium 简化**：vanilla Java isSwimming 还需 isSprinting && !isPassenger，Cubium 仅查
//      眼睛浸水（无需冲刺键），测试利好——SimulatedPlayer 不需模拟冲刺输入。
//   2) 海豚 SwimWithPlayerGoal::shouldExecute（DolphinGoals.cpp:405-409）调 _findSwimmingPlayer
//      （:477-520）：getEntitiesInRange(SEARCH_RADIUS=10) 内找 Player && isSwimming() && 非攻击目标。
//      玩家 m_isSwimming=true → 命中 → shouldExecute 返回 true。
//   3) startExecuting（:425-437）：m_targetPlayer->addEffect(DolphinsGrace, EFFECT_DURATION=100, 0, ...)。
//      DolphinsGrace 持续 100 tick（5 秒）。tick（:445-475）每 EFFECT_INTERVAL=6 tick 刷新一次效果。
//
// goal 抢占核查（DolphinEntity.cpp:315-345 registerGoals）：
//   优先级0 SwimGoal flag={Jump}（SwimGoal.cpp:39）——不占 Move；shouldExecute 需 fluidHeight>eyeHeight
//     （完全淹没才上浮），不持续抢占。
//   优先级0 FindWaterGoal 无 flag（FindWaterGoal.cpp:61 注释"不 setFlags"）+ shouldContinueExecuting 恒 false
//     （一次性）——不占 Move。
//   优先级1 SwimToTreasureGoal shouldExecute 需 hasGotFish()——海豚未喂鱼不触发，不抢占。
//   故 SwimWithPlayerGoal（优先级2，flag={Move,Look}）的 Move flag 空闲，可正常启动。✓
//
// 环境选择：creeper_pit（7×5×7）。构造水池让玩家眼睛浸没触发 isSwimming：
//   玻璃围栏（2..4, 1..4, 2..4）四面墙围 3×3 内空，内填水 (2..4, 2..4, 2..4) 三层水源（y=2,3,4）。
//   水柱四面玻璃封闭 + 底部 grass_block（helper-y=1）支撑，水源稳定不流动。
//   海豚 (2,2,3) 水中、Survival 玩家 (4,2,3) 水中，水平距 2 格 < SEARCH_RADIUS=10。
//   玩家脚踩 helper-y=1 grass_block，身体 y=2..3.8，眼睛 y≈3.62 在 helper-y=3 水层中 → areEyesInWater=true
//   → isActualSwimming=true → m_isSwimming=true。
//   水柱高 3 层（y=2,3,4），玩家即使受浮力上浮，眼睛仍在 y=3/4 水层中保持浸没（上浮 ~1.6 格才让眼睛
//   到 y=4 水面，需数十 tick），首 tick（tick 1）updateSwimming 即设 isSwimming=true，goal 启动加效果。
//
// 玩家溺水时序：Survival 玩家眼睛浸没消耗 air（maxAir=300），air 耗尽后溺水伤害。但 startExecuting
//   在 goal 首 tick（约 tick 1-2）即加 DolphinsGrace 100 tick，判定 getEffect 在 tick 2-10 抓到，
//   maxTicks=100 << 300 溺水线，玩家不溺水死亡。判定用持久效果状态（duration 100 tick），不依赖
//   玩家持续游泳——即使玩家后续上浮 isSwimming 变 false，效果仍在。
//
// 判定手段：succeedWhen 每 tick 检查玩家 getEffect("dolphins_grace") 非空（对齐 PufferfishTests/AxolotlTests
//   getEffect 范式）。区域限定查玩家排除并行测试污染。DolphinsGrace duration 100 tick，succeedWhen 必抓到。
//   非确定性：依赖玩家 isSwimming 持续到 goal 启动（tick 1-2），水柱 3 层保证眼睛浸没窗口足够。
//   玩家用 Survival（gameMode=0，0 as any；toInt32 返回 optional，if(gm) 检查 optional 有值非值=0，
//   传 0 正确得 Survival——区别 Creative 不影响本测试但 Survival 更贴合 vanilla 语义）。
// Ref: DolphinGoals.cpp:397-437（SwimWithPlayerGoal shouldExecute/startExecuting addEffect）
// Ref: Player.cpp:2118-2141（isActualSwimming + updateSwimming）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海豚.txt#行为（游泳玩家获海豚的恩惠）
function dolphinGrantsDolphinsGraceToSwimmingPlayer(test: Test): void {
  const dolphinType = "dolphin";

  // 5×5 玻璃围栏（x=1..5, z=1..5）四面墙围 3×3 内空（2..4,*,2..4）：困住海豚+玩家于水池内，防游走出 SEARCH_RADIUS。
  // 围栏高 4 格（y=1..4）防跳出。先填玻璃墙（外围 5×5），再填水（内圈 3×3）。
  for (let y = 1; y <= 4; y++) {
    // z=1 与 z=5 端面：x=1..5 全填玻璃。
    for (let x = 1; x <= 5; x++) {
      test.setBlockType("minecraft:glass", { x, y, z: 1 });
      test.setBlockType("minecraft:glass", { x, y, z: 5 });
    }
    // x=1 与 x=5 侧面：z=2..4 填玻璃（角柱已由端面填）。
    for (let z = 2; z <= 4; z++) {
      test.setBlockType("minecraft:glass", { x: 1, y, z });
      test.setBlockType("minecraft:glass", { x: 5, y, z });
    }
  }
  // 围栏内 (2..4, 2..4, 2..4) 填水（3×3 三层水源 y=2,3,4），底部 helper-y=1 grass_block 支撑。
  // 水池四面玻璃封闭，水源稳定不流动。玩家眼睛 y≈3.62 在 y=3 水层浸没触发 isSwimming。
  fillBlock(test, "minecraft:water", 2, 2, 2, 4, 4, 4);

  // 海豚 (2,2,3) 水中、Survival 玩家 (4,2,3) 水中，水平距 2 格 < SEARCH_RADIUS=10。
  // 玩家眼睛浸没 y=3 水层 → isActualSwimming=true → m_isSwimming=true。
  test.spawn(dolphinType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "swimmer", 0 as any);

  // 断言玩家获得海豚的恩惠：succeedWhen 每 tick 检查 getEffect("dolphins_grace") 非空。
  // 时序：玩家 spawn 水中(tick 0) → tick 1 updateSwimming 设 isSwimming=true →
  //   SwimWithPlayerGoal::shouldExecute 找到游泳玩家 → startExecuting addEffect(DolphinsGrace,100)。
  //   判定 getEffect 非空约 tick 2-5。DolphinsGrace duration 100 tick，succeedWhen 必抓到。
  // 区域限定查玩家排除并行测试污染；type 用 "minecraft:player"。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared before gaining dolphins_grace");
    const grace = (players[0] as any).getEffect("dolphins_grace");
    test.assert(grace !== undefined,
      `dolphin did not grant dolphins_grace to swimming player, grace=${grace}`);
  });
}

export function registerDolphinTests(): void {
  GameTest.register("MobBehaviorTests", "dolphin_retaliates_when_attacked", dolphinRetaliatesWhenAttacked)
    .structureName("gametests:grass_pen")
    .maxTicks(450);

  GameTest.register("MobBehaviorTests", "dolphin_fed_fish_sets_got_fish", dolphinFedFishSetsGotFish)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "dolphin_grants_dolphins_grace_to_swimming_player", dolphinGrantsDolphinsGraceToSwimmingPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);
}
