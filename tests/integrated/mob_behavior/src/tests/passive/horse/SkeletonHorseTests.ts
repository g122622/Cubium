// 骷髅马行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// grass_pen 结构尺寸（9×5×9），helper 相对坐标 x,z∈[0,8], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 骷髅马陷阱激活（wiki tech_骷髅马.txt#生成：雷暴天气中闪电有概率生成骷髅陷阱马，玩家接近 10 格内
// 时触发，陷阱马变成骷髅骑手——骷髅骑骷髅马，困难难度额外生成 3 只。"在天气晴朗时，骷髅陷阱马
// 仍然能被正常激活"——激活本身不要求雷暴，雷暴只决定陷阱马的生成）。
//
// C++ 链路：SkeletonHorseEntity : AbstractHorseEntity。陷阱马状态用私有成员 m_trap（非 DataParameter），
// setTrap(true)（SkeletonHorseEntity.cpp:74-92）向 m_goalSelector 优先级1添加 TriggerSkeletonTrapGoal。
// TriggerSkeletonTrapGoal（SpecialGoals.cpp:669-732）flag=Move，shouldExecute 守卫 isTrap()+alive+world，
// 取 boundingBox().expand(PLAYER_DETECTION_RANGE=10) 范围内 PLAYER，dynamic_cast<Player*> 成功且存活，
// 跳过 isSpectator()/isCreative()，distanceSqTo<=PLAYER_DETECTION_RANGE_SQ(100) 返 true。tick() 直接调
// m_horse->triggerTrap()。triggerTrap（SkeletonHorseEntity.cpp:94-299）守卫 m_trap，清 m_trap=false，
// setTame(true)，取难度 extraHorses=(Hard)?3:0，创建 SKELETON 骷髅骑手（位置=马位置，enablePersistence，
// finalizeSpawn，装备铁头盔+弓，setHurtResistantTime(60)），spawnEntity 后 startRiding(*this) 骑上原马，
// 末尾生成 setEffectOnly(true) 纯视觉闪电。**不检查天气/雷暴**（对齐 wiki"晴天仍能激活"）。
//
// 陷阱马生成方式（关键）：GameTest test.spawn("skeleton_horse", pos) 生成普通骷髅马（m_trap=false），
// 不会触发陷阱。需用 spawn 事件后缀 test.spawn("skeleton_horse<minecraft:set_trap>", pos) 生成陷阱马——
// GameTestHelper::applySpawnEvent（GameTestHelper.cpp applySpawnEvent）对 skeleton_horse + minecraft:set_trap
// 分支调 horse->setTrap(true) 注册 TriggerSkeletonTrapGoal（本提交新增该派发分支，仿 slime/rabbit 模式）。
//
// 环境选择：grass_pen（9×5×9 露天平地，y=0 grass_block 满铺 + y=1..4 air）。陷阱马 (2,2,2)、Survival
// 玩家 (7,2,7)，水平距 √(5²+5²)≈7.07 格 < 10 检测范围（PLAYER_DETECTION_RANGE_SQ=100），TriggerSkeletonTrapGoal
// shouldExecute 命中。结构放置 +1 抬升：helper-y=N → 结构内 y=N-1，脚踩结构内 y=0 grass_block。
// 用 batch("night")：骷髅骑手是亡灵白天会燃烧，night 避免生成后燃烧死亡干扰（触发是同 tick 即时生成，
// succeedWhen 立即满足，但 night 更稳）。激活不要求天气/雷暴，day/night 不影响触发。
//
// 判定手段：succeedWhen 区域限定 getEntities 取 skeleton 实体，断言 length>=1（触发前 0→触发后 1）。
// GameTestServer 默认难度 Normal（GameTestServer.hpp:45），triggerTrap extraHorses=(Hard)?3:0=0，
// 确定性生成 1 只骷髅骑手（Normal，非困难不额外生马）。区域限定用 PEN（grass_pen 9×5×9）排除并行
// 测试污染。SimulatedPlayer 用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验）——创造/旁观玩家被
// TriggerSkeletonTrapGoal::shouldExecute 滤掉（isCreative()/isSpectator() 跳过），不触发陷阱。必须 Survival。
// maxTicks=300：shouldExecute 每 tick 检测（flag=Move 不互斥）+ triggerTrap 同 tick 生成，留调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骷髅马.txt#生成（陷阱马玩家接近激活）
function skeletonHorseTrapActivates(test: Test): void {
  const skeletonType = "skeleton";

  // 陷阱马 (2,2,2)（spawn 事件后缀 set_trap 派发 setTrap(true) 注册 TriggerSkeletonTrapGoal）。
  // Survival 玩家 (7,2,7)，水平距 √(5²+5²)≈7.07 格 < 10 检测范围。helper-y=2 → 结构内 y=1 空气，
  // 脚踩结构内 y=0 grass_block。玩家 Survival：创造/旁观被 shouldExecute 滤掉不触发陷阱。
  test.spawn("skeleton_horse<minecraft:set_trap>", { x: 2, y: 2, z: 2 });
  test.spawnSimulatedPlayer({ x: 7, y: 2, z: 7 }, "bait", 0 as any);

  // 断言骷髅骑手生成：succeedWhen 每 tick 检查区域内 skeleton 实体 length>=1。
  // 时序：TriggerSkeletonTrapGoal shouldExecute 检测玩家(每 tick) → tick() 调 triggerTrap 同 tick
  // 生成骷髅骑手 + startRiding 骑上原马 + 纯视觉闪电。Normal 难度确定性 1 只（extraHorses=0）。
  // 区域限定用 PEN（grass_pen 9×5×9）排除并行测试污染。
  test.succeedWhen(() => {
    const skeletons = test.getDimension().getEntities({
      type: skeletonType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(skeletons.length >= 1,
      `skeleton trap did not activate, skeleton count=${skeletons.length}`);
  });
}

export function registerSkeletonHorseTests(): void {
  GameTest.register("MobBehaviorTests", "skeleton_horse_trap_activates", skeletonHorseTrapActivates)
    .batch("night")
    .structureName("gametests:grass_pen")
    .maxTicks(300);
}
