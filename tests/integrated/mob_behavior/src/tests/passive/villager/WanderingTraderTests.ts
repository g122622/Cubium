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
}
