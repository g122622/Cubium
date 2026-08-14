// 苦力怕行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

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

export function registerCreeperTests(): void {
  GameTest.register("MobBehaviorTests", "creeper_swell_explodes", creeperSwellExplodes)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);
}
