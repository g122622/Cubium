// 流浪商人（Wandering Trader）昼夜隐身/显形行为 GameTest。
//
// 覆盖流浪商人 UseItemGoal 双 goal 链路（对齐 MC Java 1.21.11 WanderingTrader）：
//   - 夜间（isBrightOutside()==false）喝隐身药水：条件 !isBrightOutside() && !hasEffect(Invisibility)。
//   - 白天（isBrightOutside()==true）喝牛奶解除隐身：条件 isBrightOutside() && hasEffect(Invisibility)。
// 流浪商人是 mob_behavior 包零覆盖实体类别，本测试补全其最具辨识度的 vanilla 行为。
//
// C++ 链路（VillagerEntity.cpp:1419-1449 WanderingTraderEntity::registerGoals）：
//   优先级 0 注册两个 UseItemGoal：
//     1. 夜间隐身药水（INVISIBILITY 药水）—— condition: world!=null && !isBrightOutside() && !hasEffect(Invisibility)。
//     2. 白天牛奶（MILK_BUCKET）—— condition: world!=null && isBrightOutside() && hasEffect(Invisibility)。
//   UseItemGoal（WanderingTraderGoals.cpp:64-97）：shouldExecute 读 condition；startExecuting 调
//   setMainHandItem(stack)+setActiveHand(MainHand)；shouldContinueExecuting 读 isUsingItem()；
//   resetTask 清主手+stopActiveHand+音效。喝药水经 LivingEntity::updateUsingItem（LivingEntity.cpp:2387-2418）
//   递减 useCount 到 0 调 onItemUseFinish → PotionItem::onItemUseFinish（PotionItem.cpp:74）→ _applyEffects
//   对 LivingEntity 施加 Invisibility；喝牛奶经 MilkBucketItem::onItemUseFinish 清除所有效果。
//
// isBrightOutside（IWorld.hpp:1037-1044）：!hasSkyLight() 返 false（下界/末地）；否则 getSkyDarkening()<4。
//   getSkyDarkening（IWorld.hpp:640-645）= calculateSkyDarkening(dayTimeOfDay, raining, thundering)。
//   主世界 dayTimeOfDay 6000（正午）skyDarkening=0 → bright=true；13000（夜晚）skyDarkening≈11 → bright=false。
//
// 框架依赖（均已验证可用）：
//   - getEffect("invisibility")（MinecraftModuleFactory.cpp:1709）：返 {typeId,amplifier,duration} 或 undefined。
//     IllusionerTests illusionerCastsMirror 已验证此绑定（getEffect("invisibility") !== undefined 判定）。
//   - chat("/time set night|day")（TimeCommand.cpp:105-115）：set night→setDayTime(13000)，set day→setDayTime(1000)。
//     permLevel≥2（创造玩家满足）。DespawnTests 已验证 chat 改世界级状态 + runOnFinish 恢复范式。
//
// GameTestServer 时间（GameTestServer.cpp:329-365）：batch 名 "night"→TimeOfDayEnvironment(18000)，
//   "day"/其他→TimeOfDayEnvironment(6000) 批初始环境。本测试不依赖 batch 环境（运行期 chat 主动切时间），
//   故用非 night/day 前缀的独占 batch 避免初始环境干扰 + 隔离时间状态污染。
//
// 【并行污染隔离】/time set 是世界级命令，跨测试持久化不自动重置。夜间测试会污染同批依赖白天的测试。
// 故每个改时间的测试用独占 batch 串行 + runOnFinish 恢复 /time set day（1000），防污染后续批次。
// DespawnTests 已验证此范式（difficulty 世界级状态 + 独占 batch + runOnFinish 恢复）。
//
// 结构选择：grass_pen（9×5×9 露天草地）。isBrightOutside 是世界级时间判断（getSkyDarkening），
//   与实体是否在露天无关，故任何结构均可；grass_pen 与驴/马测试同构，流浪商人脚踩草地存活。
//   流浪商人 AvoidEntityGoal 仅躲僵尸/掠夺者/唤魔者等（不躲玩家），测试不 spawn 这些故不乱跑。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: VillagerEntity.cpp:1419-1449（registerGoals 双 UseItemGoal）
// Ref: WanderingTraderGoals.cpp:64-97（UseItemGoal）/ PotionItem.cpp:74（_applyEffects）
// Ref: IWorld.hpp:1037-1044（isBrightOutside）/ TimeCommand.cpp:105-115（set night/day）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

/** 取区域内流浪商人的首个（区域限定避免批内并行污染）。 */
function firstTrader(test: Test): any | null {
  const arr = test.getDimension().getEntities({
    type: "minecraft:wandering_trader",
    location: test.worldLocation(PEN_FROM),
    volume: PEN_VOLUME,
  });
  return arr.length > 0 ? arr[0] : null;
}

// 夜间流浪商人喝隐身药水获得 Invisibility（验证夜间 UseItemGoal + 药水 _applyEffects 链路）。
//
// 流浪商人 spawn（默认白天不隐身）→ chat("/time set night") 切夜 → isBrightOutside()==false &&
// !hasEffect(Invisibility) → 夜间 UseItemGoal shouldExecute=true → setActiveHand 喝药水 →
// updateUsingItem 递减 → onItemUseFinish → _applyEffects 施加 Invisibility。
//
// 判定：getEffect("invisibility") !== undefined。
//
// 时序：命令生效（1-2 tick）+ UseItemGoal shouldExecute（下个 goal tick）+ 药水 useDuration
//   （隐身药水 useDuration=32 tick）+ _applyEffects。合计约 40-60 tick，maxTick=200 留充裕余量。
function traderDrinksInvisibilityAtNight(test: Test): void {
  test.spawn("wandering_trader", { x: 4, y: 2, z: 4 });
  // 创造玩家执行命令（permLevel=2 满足 /time 权限）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "traderNightOp");

  // tick 2 切夜（给 spawn 完成 + 玩家就绪留余量）。
  test.runAtTickTime(2, () => {
    player.chat("/time set night");
  });
  // 恢复白天，防污染后续批次。
  test.runOnFinish(() => {
    player.chat("/time set day");
  });

  // 用 succeedWhen 每 tick 检查（比 pollUntilSucceed 检查点更密，精确捕获隐身生效时刻）。
  // Cubium succeedWhen 语义：assert 失败（抛异常）当"继续轮询"而非立即 FAIL（BaseGameTestInstance.cpp:88-90）。
  test.succeedWhen(() => {
    const trader = firstTrader(test);
    test.assert(trader != null, "wandering trader disappeared");
    const invis = (trader as any).getEffect("invisibility");
    test.assert(invis !== undefined,
      `wandering trader did not gain invisibility at night `
      + `(night UseItemGoal/PotionItem._applyEffects broken)`);
  });
}

// 白天流浪商人喝牛奶解除隐身（验证白天牛奶 UseItemGoal + MilkBucket 清除效果链路）。
//
// 两阶段：tick 2 chat("/time set night") 切夜 → 等隐身生效（pollUntilSucceed 阶段一）→
// tick 切回白天 chat("/time set day") → isBrightOutside()==true && hasEffect(Invisibility) →
// 白天牛奶 UseItemGoal shouldExecute=true → 喝牛奶 → MilkBucket::onItemUseFinish 清除所有效果 →
// Invisibility 消失。
//
// 判定（阶段二）：getEffect("invisibility") === undefined（隐身被牛奶清除）。
//
// 时序：切夜（2 tick）+ 夜间隐身生效（约 40 tick）+ tick 55 断言隐身已生效（防假通过）+
//   切白天（60 tick）+ 牛奶 UseItemGoal（约 40 tick）。合计约 100 tick，maxTick=400 留充裕余量。
//
// 这是双 goal 完整验证：夜间隐身 goal + 白天牛奶 goal 都触发，最具对齐价值。
// tick 55 的隐身断言确保"隐身先生效"——若隐身未生效，此断言 FAIL 而非阶段二假通过。
function traderDrinksMilkToReappearAtDay(test: Test): void {
  test.spawn("wandering_trader", { x: 4, y: 2, z: 4 });
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "traderCycleOp");

  // 阶段一：切夜，等隐身生效。
  test.runAtTickTime(2, () => {
    player.chat("/time set night");
  });

  // tick 55 断言隐身已生效（防阶段二假通过：若隐身未生效，切白天后本就无隐身会误判牛奶清除成功）。
  test.runAtTickTime(55, () => {
    const trader = firstTrader(test);
    const invis = trader != null ? (trader as any).getEffect("invisibility") : null;
    test.assert(invis !== undefined,
      `phase1: wandering trader should be invisible at night before switching to day `
      + `(night UseItemGoal broken, cannot test milk clear), `
      + `invisibility=${invis === undefined ? "undefined" : "present"}`);
  });

  // 阶段二：tick 60（隐身已生效）切回白天，触发牛奶 goal。
  test.runAtTickTime(60, () => {
    player.chat("/time set day");
  });
  // 恢复白天（已在 tick 60 切 day，但 runOnFinish 兜底防异常退出时残留夜晚）。
  test.runOnFinish(() => {
    player.chat("/time set day");
  });

  // 阶段二判定：tick 70 后（切白天后留牛奶 goal 触发时间）轮询 invisibility 消失。
  pollUntilSucceed(test, () => {
    const trader = firstTrader(test);
    if (trader == null) return false;
    return (trader as any).getEffect("invisibility") === undefined;
  }, {
    startTick: 70,
    interval: 2,
    maxTick: 400,
    onTimeout: () => {
      const trader = firstTrader(test);
      const invis = trader != null ? (trader as any).getEffect("invisibility") : null;
      test.assert(false,
        `wandering trader did not drink milk to remove invisibility at day `
        + `(day milk UseItemGoal/MilkBucket clear broken), `
        + `invisibility=${invis === undefined ? "undefined" : "present"}`);
    },
  });
}

// 白天流浪商人不喝隐身药水（对照组：验证白天不触发夜间隐身 goal）。
//
// 流浪商人 spawn（默认白天 isBrightOutside()==true）→ 不切时间 → 夜间 UseItemGoal condition
// !isBrightOutside()=false → shouldExecute=false 不喝药水 → 始终无 Invisibility。
//
// 判定：等 100 tick 后断言 getEffect("invisibility") === undefined（确认白天不会乱喝隐身药水）。
//
// 对照组区别于 traderDrinksInvisibilityAtNight：验证 condition 的 isBrightOutside 门控，
// 排除"流浪商人无条件喝药水"的假阳性。
function traderNoInvisibilityAtDay(test: Test): void {
  test.spawn("wandering_trader", { x: 4, y: 2, z: 4 });
  test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "traderDayOp");
  // 不切时间，依赖默认白天环境（day 批初始 6000）。

  // 等 100 tick（覆盖若干 UseItemGoal tick + 药水 useDuration）后断言无隐身。
  test.runAtTickTime(100, () => {
    const trader = firstTrader(test);
    const invis = trader != null ? (trader as any).getEffect("invisibility") : null;
    test.assert(invis === undefined,
      `wandering trader should not be invisible at day `
      + `(night UseItemGoal condition isBrightOutside gate broken), `
      + `invisibility=${invis === undefined ? "undefined" : "present"}`);
    test.succeed();
  });
}

// 流浪商人躲避劫掠兽（wiki tech_流浪商人.txt + 对齐 Java 1.21.11 VillagerHostilesSensor：
//   ravager 在敌对白名单内，距离 12 格触发逃离）。
//
// 本测试专项验证 WanderingTraderEntity::registerGoals 的 AvoidEntityGoal(RAVAGER, 12格) 谓词（缺陷修复）。
// C++ 链路（VillagerEntity.cpp:1473-1480）：流浪商人优先级1注册 AvoidEntityGoal(this, 12.0f, ...)，
//   谓词 `entityType()==RAVAGER`。AvoidEntityGoal::shouldExecute（AvoidEntityGoal.cpp:61-84）：
//   _findEntityToAvoid（:134-143）用 EntityUtils::findClosestEntity(world, pos, avoidDistance+3.0=15, predicate)
//   搜索范围内 RAVAGER；找到后 _findEscapePosition（:145-168）用 RandomPositionGenerator.findRandomTargetBlockAwayFrom
//   选远离目标的逃跑点，_isEscapePositionValid（:170-180）要求逃跑点比当前位置更远离目标；nav.moveTo 逃离。
//   flag={Move}，优先级1 抢占 RandomWalkingGoal（优先级8）。
//
// 此前缺陷：流浪商人 AvoidEntityGoal 列表无 RAVAGER 谓词（vanilla ravager=12 在白名单），致流浪商人
//   在劫掠兽旁不逃离（原地 RandomWalking）。本次修复补 RAVAGER AvoidEntityGoal（:1473-1480）。
//
// 关键设计——双侧玻璃围栏（防实体逃出查询区域 + 防劫掠兽追杀）：
//   - 流浪商人 WanderingTraderEntity : AbstractVillagerEntity（VillagerEntity.cpp:1334），故劫掠兽
//     RavagerAttackGoal targetSelector 优先级4 NearestAttackableTargetGoal<AbstractVillagerEntity>
//     （RavagerEntity.cpp:392-397）**会选流浪商人为目标**追击。须用玻璃墙物理隔开防追杀。
//   - 玻璃墙 x=4（y=1,2,3，z=0..8 全长）分隔东西：流浪商人 (2,2,4) 西侧，劫掠兽 (6,2,4) 东侧，
//     初始距 4 格 < 12 触发 AvoidEntity RAVAGER。劫掠兽只破坏树叶（_breakLeavesOnCollision，RavagerEntity.cpp:305
//     仅 BlockTags::LEAVES），不破坏玻璃，墙有效隔开。
//   - AvoidEntityGoal._findEntityToAvoid 用 findClosestEntity **不查视线**（仅 predicate 类型匹配），
//     故玻璃墙不阻挡流浪商人感知劫掠兽——墙隔开仅防物理追击，不防感知。
//   - **流浪商人西侧围栏**（z=1 和 z=7，y=1,2,3，x=0..4）：限制流浪商人在 x=0..4, z=2..6 西侧带内逃离，
//     防止逃离方向随机（RandomPositionGenerator 选远离劫掠兽的随机方向，可能朝 -x-z 偏离）逃出结构
//     查询区域 PEN_VOLUME（9×5×9）。早期诊断发现流浪商人 t=20 即逃离到 x≈0.9 z≈6.3，无围栏时会继续
//     游走出区域致 trader=0 查询失败。围栏把流浪商人困在 4×5 区域内，逃离到 x≈0 围栏边停下。
//   - **劫掠兽东侧围栏**（z=1 和 z=7，y=1,2,3，x=4..8）：限制劫掠兽在东侧带，防 RandomWalking 游出区域。
//
// 判定手段：pollUntilSucceed 轮询流浪商人到劫掠兽的 3D 距离 > 5.5（初始 4，逃离到 x≈0 后距≈6）。
//   AvoidEntityGoal._isEscapePositionValid 保证逃跑点比当前位置更远，距离单调增大；逃离到 x≈0 围栏边
//   后距离稳定 ≈6 > 5.5。**startTick=20 早期轮询**——流浪商人 t=20 已开始逃离（早期诊断实测），早期
//   捕获避免流浪商人逃离后被围栏反弹回近处（极小概率）或时序偏移。maxTick=400 留充裕余量。
//
// 环境约束：
//   - grass_pen 9×5×9 露天。skyAccess(true)+setupTicks(20) 隔离 NaturalSpawner 自然生成（露天 skyLight=15
//     拒绝怪物生成），避免自然生成的怪物干扰流浪商人位置判定。
//   - 独占 batch（trader_flee_solo）串行——劫掠兽/流浪商人实体查询须独占区域避免同批其他测试的同类
//     实体污染计数。
//   - 白天默认环境：劫掠兽 setBurnsInDaylight(false) 不燃；流浪商人白天不喝隐身（UseItemGoal 条件
//     isBrightOutside 不满足），无 UseItemGoal 干扰 Move flag。
//   - 劫掠兽体积大（width 1.95），脚下 (6,1,4) 放玻璃支撑防重力下落；流浪商人脚下 grass_block 地板天然支撑。
//   - 流浪商人 MOVEMENT_SPEED=0.5（VillagerEntity.cpp:1576），陆地 GroundPathNavigation 可寻路逃离。
// Ref: VillagerEntity.cpp:1473-1480（AvoidEntityGoal RAVAGER 修复）/ :1334（WanderingTrader: AbstractVillager）
// Ref: AvoidEntityGoal.cpp:61-84（shouldExecute）/ :134-143（_findEntityToAvoid 不查视线）
// Ref: RavagerEntity.cpp:305-344（_breakLeavesOnCollision 仅破坏树叶不破坏玻璃）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_劫掠兽.txt#行为（灾厄村民敌对，流浪商人逃离）
function wanderingTraderFleesFromRavager(test: Test): void {
  const traderType = "wandering_trader";
  const ravagerType = "ravager";

  // 中间隔墙 x=4（y=1,2,3，z=0..8 全长）分隔东西两侧。
  for (let z = 0; z <= 8; z++) {
    test.setBlockType("minecraft:glass", { x: 4, y: 1, z });
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z });
    test.setBlockType("minecraft:glass", { x: 4, y: 3, z });
  }
  // 西侧围栏 z=1 和 z=7（y=1,2,3，x=0..4）：限制流浪商人在 z=2..6 西侧带内逃离，防逃出查询区域。
  // 东侧围栏 z=1 和 z=7（y=1,2,3，x=4..8）：限制劫掠兽在 z=2..6 东侧带，防游出区域。
  // x=0 和 x=8 是 grass_pen 结构自有玻璃墙（边界），无需补。
  for (let x = 0; x <= 8; x++) {
    if (x === 4) continue; // x=4 已由隔墙填充
    test.setBlockType("minecraft:glass", { x, y: 1, z: 1 });
    test.setBlockType("minecraft:glass", { x, y: 2, z: 1 });
    test.setBlockType("minecraft:glass", { x, y: 3, z: 1 });
    test.setBlockType("minecraft:glass", { x, y: 1, z: 7 });
    test.setBlockType("minecraft:glass", { x, y: 2, z: 7 });
    test.setBlockType("minecraft:glass", { x, y: 3, z: 7 });
  }
  // 劫掠兽脚下 (6,1,4) 玻璃支撑防重力下落（劫掠兽受 MonsterEntity 重力）。
  test.setBlockType("minecraft:glass", { x: 6, y: 1, z: 4 });

  // 流浪商人 (2,2,4) 西侧、劫掠兽 (6,2,4) 东侧，初始距 4 格 < 12 触发 AvoidEntity RAVAGER。
  // helper-y=2 → 结构内 y=1 空气腔，脚踩结构内 y=0 grass_block。
  test.spawn(traderType, { x: 2, y: 2, z: 4 });
  test.spawn(ravagerType, { x: 6, y: 2, z: 4 });

  // 轮询：流浪商人逃离使到劫掠兽距离 > 5.5（初始 4 → 逃离到 x≈0 围栏边后 ≈6）。
  // AvoidEntityGoal._isEscapePositionValid 保证逃跑点更远，距离单调增大。startTick=20 早期捕获。
  pollUntilSucceed(test, () => {
    const traders = test.getDimension().getEntities({
      type: "minecraft:wandering_trader",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    const ravagers = test.getDimension().getEntities({
      type: ravagerType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (traders.length === 0 || ravagers.length === 0) return false;
    const t = traders[0].location;
    const r = ravagers[0].location;
    const dist = Math.hypot(t.x - r.x, t.y - r.y, t.z - r.z);
    return dist > 5.5;
  }, {
    startTick: 20,
    interval: 10,
    maxTick: 400,
    onTimeout: () => {
      const traders = test.getDimension().getEntities({
        type: "minecraft:wandering_trader",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const ravagers = test.getDimension().getEntities({
        type: ravagerType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const tPos = traders.length > 0
        ? `(${traders[0].location.x.toFixed(1)},${traders[0].location.y.toFixed(1)},${traders[0].location.z.toFixed(1)})`
        : "gone";
      const rPos = ravagers.length > 0
        ? `(${ravagers[0].location.x.toFixed(1)},${ravagers[0].location.y.toFixed(1)},${ravagers[0].location.z.toFixed(1)})`
        : "gone";
      const dist = (traders.length > 0 && ravagers.length > 0)
        ? Math.hypot(traders[0].location.x - ravagers[0].location.x,
            traders[0].location.y - ravagers[0].location.y,
            traders[0].location.z - ravagers[0].location.z) : -1;
      // dist≈4（初始）说明流浪商人未逃离（AvoidEntity RAVAGER 谓词缺失/失效）；
      // dist>4 但<5.5 说明逃离中或被墙角限制（边界情况，可能需调阈值）。
      test.assert(false,
        `wandering trader did not flee from ravager: trader=${traders.length} pos=${tPos}; `
        + `ravager=${ravagers.length} pos=${rPos}; dist=${dist.toFixed(2)} `
        + `(dist≈4: AvoidEntity RAVAGER predicate missing/broken - trader not fleeing)`);
    },
  });
}

export function registerWanderingTraderTests(): void {
  GameTest.register("MobBehaviorTests", "wandering_trader_drinks_invisibility_at_night", traderDrinksInvisibilityAtNight)
    .batch("trader_night_solo")
    .structureName("gametests:grass_pen")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "wandering_trader_drinks_milk_to_reappear_at_day", traderDrinksMilkToReappearAtDay)
    .batch("trader_cycle_solo")
    .structureName("gametests:grass_pen")
    .maxTicks(450);

  GameTest.register("MobBehaviorTests", "wandering_trader_no_invisibility_at_day", traderNoInvisibilityAtDay)
    .batch("trader_day_solo")
    .structureName("gametests:grass_pen")
    .maxTicks(150);

  GameTest.register("MobBehaviorTests", "wandering_trader_flees_from_ravager", wanderingTraderFleesFromRavager)
    .batch("trader_flee_solo")
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(420);
}
