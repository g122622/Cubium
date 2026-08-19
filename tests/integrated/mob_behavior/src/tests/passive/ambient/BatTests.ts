// 蝙蝠行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// grass_pen 结构尺寸（9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 蝙蝠夜晚飞行（wiki tech_蝙蝠.txt#行为：蝙蝠在夜晚飞行，白天倒挂休息）。
//
// C++ 链路：BatRestGoal::shouldExecute 校验 dayTimeOfDay()<12000（白天）才尝试休息，夜间直接 return false
// （BatGoals.cpp:262-268）。夜间 RestGoal 不执行，BatRandomFlyGoal（优先级0，shouldExecute 仅判 !isResting()）
// 持续驱动蝙蝠随机飞行（选 ±7/-2~+4 目标点，平滑转向，BatGoals.cpp:103-180）。蝙蝠默认 m_flying=true，
// tick 中 flying 且非 resting 时 Y 速度阻尼 0.6（BatEntity.cpp:79-82）。
//
// 判定手段：蝙蝠夜间持续飞行必有显著位移。记录 tick 20（待 spawn + 首次 goal tick 就绪）的初始位置，
// succeedWhen 每 tick 断言当前位置距初始位置水平距离 >2 格（飞行位移）。飞行速度 0.1，2 格约需 40+ tick，
// maxTicks=600 留充裕余量吸收飞行目标点随机性。
//
// 环境选择：grass_pen（9×5×9），蝙蝠 spawn 于 (4,3,4)（结构内 y=2 空气腔，y=1..3 内部空气可飞）。
// night 批设 dayTime=18000（≥12000 夜间，RestGoal 不执行）。蝙蝠飞行不需视线/露天，grass_pen 玻璃墙
// 不影响（BatRandomFlyGoal 选目标点只查空气方块，玻璃墙外是 worldgen 但飞行在结构内空气腔）。
// 不 spawn 玩家：蝙蝠对玩家无反应（无 AvoidEntity/FleeGoal），且避免 RestGoal 玩家 4 格唤醒干扰。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蝙蝠.txt#行为（夜晚飞行）
function batFliesAtNight(test: Test): void {
  const batType = "bat";

  // 蝙蝠 spawn 于 (4,3,4)（结构内 y=2 空气腔，helper-y=3→结构内 y=2）。grass_pen 中心，飞行空间充足。
  test.spawn(batType, { x: 4, y: 3, z: 4 });

  // 记录 tick 20 初始位置（待 spawn + 首次 BatRandomFlyGoal tick 就绪）。
  // BatRandomFlyGoal 优先级0，shouldExecute 仅判 !isResting()（夜间不休息恒 true），spawn 即开始飞行。
  const startPos = test.worldLocation({ x: 4, y: 3, z: 4 });
  let initPos: { x: number; y: number; z: number } | null = null;
  test.runAtTickTime(20, () => {
    const bats = test.getDimension().getEntities({
      type: batType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (bats.length > 0) {
      initPos = { x: bats[0].location.x, y: bats[0].location.y, z: bats[0].location.z };
    }
  });

  // 断言蝙蝠飞行位移：距初始位置水平距离 >2 格。initPos 在 tick 20 采样，succeedWhen 持续检查。
  // 飞行速度 0.1，2 格约需 40+ tick（tick 20 后再飞 40 tick ≈ tick 60），maxTicks=600 留充裕余量。
  test.succeedWhen(() => {
    const bats = test.getDimension().getEntities({
      type: batType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(bats.length > 0, "bat disappeared");
    test.assert(initPos !== null, "bat initial position not sampled");
    const b = bats[0];
    const dx = b.location.x - initPos!.x;
    const dz = b.location.z - initPos!.z;
    test.assert(dx * dx + dz * dz > 2 * 2,
      `bat did not fly at night, distSq=${dx * dx + dz * dz}`);
  });
}

// 蝙蝠白天倒挂休息（wiki tech_蝙蝠.txt#行为：白天蝙蝠倒挂在方块下方休息，玩家靠近 4 格内唤醒）。
//
// C++ 链路：BatRestGoal::shouldExecute（BatGoals.cpp:255-282）白天(dayTime<12000) + 非休息 + 1/100 概率 +
// _canRestAtCurrentPosition（上方 floor(pos.y+height+0.1)=floor(pos.y+1.0) 处有固体方块，蝙蝠 height=0.9）。
// startExecuting（BatGoals.cpp:299-320）：setResting(true)、setFlying(false)、清零速度、对齐到
// blockY - height + 0.1（挂在上方方块底面）。tick 持续 setVelocity(0,0,0) 保持静止。
// _shouldStopResting：夜间/玩家4格内/失去支撑→唤醒。day 批默认 dayTime=6000<12000 白天，RestGoal 可触发。
//
// 概率性处理：1/100 每 tick 触发，但需 _canRestAtCurrentPosition 通过（蝙蝠当前格上方有固体方块）。
// 蝙蝠飞行中 pos.y 会变化（BatRandomFlyGoal targetY=currentY+randint(-2,4)），飞低时上方可能是空气致
// canRest 失败，故实际倒挂触发率低于理论 1/100。用 6 只蝙蝠 + 较长采样窗口（tick 200→400，200 tick 窗口）
// 提高累计触发概率：6 只 × 200 tick × 1/100 ≈ 12 次尝试，即便 canRest 通过率仅 ~30%，至少一次成功概率
// >1-0.7^12≈99.1%。满铺石头天花板保证蝙蝠飞到高 y 位置时上方有石头可挂（canRest 随处可满足）。
//
// 判定手段：倒挂后蝙蝠速度恒零、位置稳定。用 succeedWhen 每 tick 持续检测"是否有蝙蝠相对上一 tick
// 位移 <0.1"（静止）。相比固定两 tick 采样，持续检测能捕捉任意 tick 的倒挂瞬间，不依赖采样窗口恰好
// 落在倒挂时段。闭包存上一 tick 各蝙蝠位置，每 tick 比对：任一蝙蝠位移 <0.1 即判定倒挂静止 succeed。
// 飞行蝙蝠每 tick 位移 >0.1（飞行速度 0.1+），倒挂蝙蝠位移 ≈0，区分明确。startTick=200 留 1/100
// 倒挂触发时间，maxTicks=800 留充裕余量吸收 1/100 概率 + canRest 失败重试。
//
// 环境选择：grass_pen（9×5×9）。helper-y=3 满铺石头天花板（结构内 y=2，9×9 共 81 块），6 只蝙蝠 spawn
// 于 helper-y=2（结构内 y=1 空气，上方 y=2 石头，canRest 查 floor(pos.y+1.0)=2 命中石头）。
// 蝙蝠在 y=1 飞行层（高 1 格，蝙蝠高 0.9 刚好），上方满石头限制飞行高度，倒挂随处可触发。
// day 批（默认）dayTime=6000 白天。不 spawn 玩家避免 4 格唤醒。
// 不用 skyAccess：倒挂不需露天，grass_pen 玻璃墙+石头天花板封闭环境即可。
// maxTicks=800：200 tick 等待倒挂触发 + 每 tick 持续检测窗口 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蝙蝠.txt#行为（白天倒挂休息）
function batRestsInDaytime(test: Test): void {
  const batType = "bat";
  const ceilingY = 3; // helper-y=3 满铺石头天花板（结构内 y=2）
  const batY = 2; // 蝙蝠 spawn helper-y=2（结构内 y=1，上方 y=2 石头）

  // 满铺石头天花板：grass_pen 9×9（x,z ∈ [0,8]），helper-y=3。保证蝙蝠飞到任意位置上方有固体方块可挂。
  for (let x = 0; x < 9; x++) {
    for (let z = 0; z < 9; z++) {
      test.setBlockType("minecraft:stone", { x, y: ceilingY, z });
    }
  }

  // 6 只蝙蝠分散 spawn 于 y=1 飞行层，提高 1/100 倒挂触发率（6 只 × 600 tick ≈ 36 次尝试）。
  test.spawn(batType, { x: 2, y: batY, z: 2 });
  test.spawn(batType, { x: 2, y: batY, z: 6 });
  test.spawn(batType, { x: 4, y: batY, z: 4 });
  test.spawn(batType, { x: 6, y: batY, z: 2 });
  test.spawn(batType, { x: 6, y: batY, z: 6 });
  test.spawn(batType, { x: 4, y: batY, z: 7 });

  // 上一 tick 各蝙蝠位置数组（getEntities 返回顺序可能不稳定，用最近邻匹配而非索引对齐）。null 表示首帧。
  let prevPositions: { x: number; y: number; z: number }[] | null = null;

  // 每 tick 持续检测：任一蝙蝠与上一 tick 某蝙蝠位置位移 <0.1 即倒挂静止 succeed。
  // startTick=200 留 1/100 倒挂触发时间。用 runAtTickTime 每 tick 注册检测（预注册 200..800 共 601 个检查点）。
  for (let tick = 200; tick <= 800; tick++) {
    test.runAtTickTime(tick, () => {
      const bats = test.getDimension().getEntities({
        type: batType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      if (bats.length === 0) {
        return; // 蝙蝠消失（不应发生），本 tick 不判定
      }
      const currentPositions = bats.map(b => ({ x: b.location.x, y: b.location.y, z: b.location.z }));
      if (prevPositions !== null) {
        // 最近邻匹配：任一当前蝙蝠与上一 tick 某蝙蝠位移 <0.1 即静止。
        for (const cur of currentPositions) {
          for (const prev of prevPositions) {
            const dx = cur.x - prev.x;
            const dy = cur.y - prev.y;
            const dz = cur.z - prev.z;
            if (dx * dx + dy * dy + dz * dz < 0.1 * 0.1) {
              // 该蝙蝠相对上一 tick 位移 <0.1，判定倒挂静止，succeed。
              test.succeed();
              return;
            }
          }
        }
      }
      prevPositions = currentPositions;
      // 本 tick 未检测到静止，更新 prevPositions 等下一 tick。最后一个 tick（800）未 succeed 则超时。
    });
  }
}

export function registerBatTests(): void {
  GameTest.register("MobBehaviorTests", "bat_flies_at_night", batFliesAtNight)
    .batch("night")
    .structureName("gametests:grass_pen")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "bat_rests_in_daytime", batRestsInDaytime)
    .structureName("gametests:grass_pen")
    .maxTicks(900);
}
