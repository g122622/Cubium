// 河豚行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 河豚在玩家/敌对生物接近 2 格内时膨胀（wiki other_河豚.txt#防御：当不处于创造/旁观模式的玩家、
// 盔甲架、美西螈或任何其他非水生生物在河豚周围 5×5×5 区域时，河豚会膨胀，从未膨胀到半膨胀，
// 再到完全膨胀）。
//
// C++ 链路：PufferfishEntity : AbstractFishEntity : WaterMobEntity。registerGoals（PufferfishEntity.cpp:102-108）
// 优先级1注册 PuffGoal（无 GoalFlag，每 tick 检查 shouldExecute）。PuffGoal::shouldExecute（SpecialGoals.cpp:284-290）
// 调 _findNearbyEnemy（:345-371）：取 boundingBox().grow(DETECTION_RANGE=2.0) 范围内 LivingEntity，
// _isEnemy（:315-343）判定——玩家非创造/旁观即 scary 返 true；水生生物（COD/SALMON/PUFFERFISH/
// TROPICAL_FISH/SQUID/DOLPHIN/TURTLE）返 false；其他生物返 true。玩家满足 → shouldExecute true →
// startExecuting 调 startPuffTimer()（m_puffTimer=1）。PufferfishEntity::tick（:110-143）膨胀分支：
// m_puffTimer>0 时，若 m_puffState==Deflated && m_puffTimer==1 → setPuffState(SemiPuffed)（即触发后
// 下一 tick 立即升到半膨胀）；若 m_puffTimer>40 && SemiPuffed → setPuffState(FullyPuffed)（约 40 tick 后）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block + y=1..4 air，陆地无水）。关键约束：
//   1. 河豚是水生生物，离水会窒息（AbstractFishEntity maxAir=480 ticks=24秒）。但膨胀测试窗口
//      （~50 tick）远小于 480 tick 窒息线，河豚不会窒息，干扰排除。
//   2. 河豚陆地扑腾（updateFlopping）每 100 tick 跳一次（0.4 上 + 0.05 水平），漂移极小，测试窗口
//      内位置基本稳定，PuffGoal 检测范围 grow(2.0) 容纳漂移。
//   3. 结构放置 +1 抬升：helper-y=N → 结构内 y=N-1。河豚 (3,2,3)、玩家 (4,2,3)，水平距 1 格
//      < 2.0 检测范围，PuffGoal 立即检测玩家触发膨胀。
//
// 判定手段：读 minecraft:pufferfish_puff_state 组件（Cubium 自定义组件绑定，MinecraftModuleFactory.cpp
// getComponent 对 PufferfishEntity 返 PufferfishPuffStateComponent，readonly value=getPuffState() 即
// 0/1/2）。用 spawn 返回的河豚引用读组件（膨胀是实体自身状态，不依赖坐标查询，引用稳定——同
// glow_squid_darkens_when_hurt 用 glowSquid 引用读 dark_ticks 组件模式）。玩家用 Survival（gameMode=0，
// 0 as any 绕过 TS 枚举校验）——创造/旁观玩家被 _isEnemy 滤掉（返 false），不触发膨胀。必须 Survival。
// **关键时序**：PuffGoal 触发 tick 设 m_puffTimer=1，下一 tick（tick+2）setPuffState(SemiPuffed)，
// 故 succeedWhen 检查 value≥1。maxTicks=100 留足 2 tick 升级 + 并行负载时序偏移余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_河豚.txt#防御（玩家接近膨胀）
function pufferfishPuffsWhenPlayerNear(test: Test): void {
  const pufferfishType = "pufferfish";

  // 河豚 (3,2,3)、Survival 玩家 (4,2,3)，紧邻 1 格 < 2.0 检测范围。helper-y=2 → 结构内 y=1 空气，
  // 脚踩结构内 y=0 grass_block。玩家用 Survival（gameMode=0）：创造/旁观被 _isEnemy 滤掉不触发膨胀。
  const pufferfish = test.spawn(pufferfishType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 断言河豚膨胀到 SemiPuffed(1) 或 FullyPuffed(2)：succeedWhen 每 tick 检查 puff_state 组件 value≥1。
  // 时序：PuffGoal shouldExecute 检测玩家(每 tick) → startPuffTimer(m_puffTimer=1) → 下一 tick
  // setPuffState(SemiPuffed)。用 spawn 返回引用读组件（膨胀是自身状态，引用稳定，不依赖坐标查询）。
  // (puffState as any).value 绕过 TS 类型（自定义组件无类型定义）。
  test.succeedWhen(() => {
    const puffState = pufferfish.getComponent("minecraft:pufferfish_puff_state" as any);
    test.assert(puffState !== undefined,
      "pufferfish has no puff_state component (binding missing)");
    const value = (puffState as any).value as number;
    test.assert(value >= 1,
      `pufferfish did not puff when player near, puff_state=${value}`);
  });
}

// 河豚膨胀后接触非水生生物应施加中毒。牛不免疫中毒，蜘蛛免疫，不能作为正向靶。
// 两者中心相隔 0.4 格，使膨胀后的接触范围确定覆盖靶；使用实体句柄读取效果。
function pufferfishPoisonsMobWhenPuffed(test: Test): void {
  test.spawn("minecraft:pufferfish", { x: 3, y: 2, z: 3 });
  const target = test.spawnAtLocation("minecraft:cow", test.worldLocation({ x: 3.9, y: 2, z: 3.5 }));
  test.succeedWhen(() => {
    test.assert((target as any).getEffect("poison") !== undefined,
      "cow was not poisoned by puffed pufferfish");
  });
}

// 河豚在守卫者旁不膨胀（wiki other_河豚.txt#防御：河豚对水生生物不膨胀——守卫者属于
// EntityTypeTags.NOT_SCARY_FOR_PUFFERFISH 标签，河豚检测到守卫者不触发 PuffGoal）。
//
// 本测试专项验证 PuffGoal::_isEnemy 白名单含 GUARDIAN（对齐缺陷修复）。Cubium _isEnemy 此前仅列 7 个
// 水生生物白名单，漏 GUARDIAN/ELDER_GUARDIAN/GLOW_SQUID/NAUTILUS/ZOMBIE_NAUTILUS 致河豚在这些水生
// 生物旁错误膨胀（vanilla NOT_SCARY_FOR_PUFFERFISH 标签 14 个成员）。本次修复显式补全 12 个已实现成员
// （SpecialGoals.cpp:340-347），sulfur_cube/tadpole 未实现留 TODO。
//
// 负向测试设计（防假通过）：
//   - 负向断言"河豚不膨胀"若用 succeedWhen 每 tick 检查 value==0，第 1 tick 即满足立即 PASSED——
//     修复前后都立即通过，测不出回归。故须用"持续窗口内恒==0"断言：预注册多个检查点，任一检查点
//     发现 puff_state!=0 即 assert(false) 抛异常经 wrapJsCallback 转 FAIL；全程==0 则最后一个检查点
//     test.succeed()。
//   - 假通过风险（框架 bug 让河豚永不膨胀）：由正向对照测试 pufferfish_puffs_when_player_near 兜底
//     （河豚在玩家旁确实膨胀=0→1），两者互补——若 PuffGoal 整体失效，正向测试会失败暴露。
//
// 环境选择：creeper_pit 陆地无水（同膨胀测试约束：480 tick 窒息线 >> 测试窗口 120 tick）。河豚 (3,2,3)、
// 守卫者 (4,2,3) 紧邻 1 格 < 2.0 检测范围。守卫者陆地不窒息（守卫者离开水扑腾但不窒息，见 GuardianTests）。
// 守卫者可能激光攻击河豚？——不会，GuardianAttackGoal 谓词只放行 Player/Squid/GlowSquid/Axolotl，
// 河豚不在攻击列表，守卫者不攻击河豚，河豚不受干扰。
//
// 检查点时序：PuffGoal shouldExecute 每 tick 检测，触发后下一 tick setPuffState(SemiPuffed=1)。
// 若白名单漏 GUARDIAN（回归），河豚应在 ~2 tick 内 puff_state 升到 1。检查点 [10,30,60,90,120] 覆盖
// 全窗口，任一时刻 puff_state!=0 即 FAIL。maxTicks=120 留足膨胀触发余量（若会膨胀早膨胀了）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_河豚.txt#防御（对水生生物不膨胀）
function pufferfishDoesNotPuffNearGuardian(test: Test): void {
  const pufferfishType = "pufferfish";
  const guardianType = "guardian";

  // 河豚 (3,2,3)、守卫者 (4,2,3) 紧邻 1 格 < 2.0 检测范围。helper-y=2 → 结构内 y=1 空气，脚踩 y=0 grass_block。
  // 守卫者受重力下落，脚下 (4,1,3) 放玻璃支撑；河豚陆地扑腾漂移极小，无需围栏（2.0 检测范围容漂移）。
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  const pufferfish = test.spawn(pufferfishType, { x: 3, y: 2, z: 3 });
  test.spawn(guardianType, { x: 4, y: 2, z: 3 });

  // 负向断言：窗口内多个检查点 puff_state 必须恒==0。任一检查点 !=0 即 assert(false) FAIL（捕获白名单
  // 回归——河豚错误膨胀）。全程==0 则末检查点 succeed。
  // 用 spawn 返回引用读 puff_state 组件（膨胀是自身状态，引用稳定，不依赖坐标查询）。
  const checkTicks = [10, 30, 60, 90, 120];
  for (let i = 0; i < checkTicks.length; i++) {
    const tick = checkTicks[i];
    const isLast = i === checkTicks.length - 1;
    test.runAtTickTime(tick, () => {
      const puffState = pufferfish.getComponent("minecraft:pufferfish_puff_state" as any);
      test.assert(puffState !== undefined,
        "pufferfish has no puff_state component (binding missing)");
      const value = (puffState as any).value as number;
      if (value !== 0) {
        // 河豚错误膨胀——白名单漏 GUARDIAN 回归，FAIL。
        test.assert(false,
          `pufferfish should not puff near guardian, but puff_state=${value} at tick ${tick}`);
        return;
      }
      if (isLast) {
        // 全程未膨胀，验证 NOT_SCARY_FOR_PUFFERFISH 白名单含 GUARDIAN。
        test.succeed();
      }
    });
  }
}

export function registerPufferfishTests(): void {
  GameTest.register("MobBehaviorTests", "pufferfish_puffs_when_player_near", pufferfishPuffsWhenPlayerNear)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);

  GameTest.register("MobBehaviorTests", "pufferfish_poisons_mob_when_puffed", pufferfishPoisonsMobWhenPuffed)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(150);

  GameTest.register("MobBehaviorTests", "pufferfish_does_not_puff_near_guardian", pufferfishDoesNotPuffNearGuardian)
    .structureName("gametests:creeper_pit")
    .maxTicks(120);
}
