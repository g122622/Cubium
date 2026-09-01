// 生物跳跃行为类 GameTest。
//
// 复现"所有生物很难跳上一格高方块"的异常。C++ 跳跃触发链路：
//   MobEntity::tick → LivingEntity::aiStep（LivingEntity.cpp:1897-1933）：m_isJumping && onGround &&
//   m_jumpTicks==0 → LivingEntity::jump() 设垂直速度=jumpPower(0.42)，m_jumpTicks=10 冷却。
//   m_isJumping 由 JumpController::tick 末尾 setJumping(m_isJumping) 写入（JumpController.cpp:38-46），
//   而 JumpController::setJumping() 由 MovementController::tick 的 MoveTo 分支在判定 shouldJump 时调用
//   （MovementController.cpp:215-220）。
//
// MovementController::tick 跳跃触发两条件（MovementController.cpp:155-220，满足其一 shouldJump=true）：
//   条件1（:163）：目标路径点比当前高 dy > stepHeight(0.6) 且水平距离近 < max(1,width)²。
//   条件2（:169-213）：实体当前所在方块（floorTo(x/y/z)）碰撞形状非空，且 entityY < 方块顶 Y，
//     且方块非门/栅栏 → shouldJump。
// 牛 stepHeight=0.6（继承 LivingEntity，CowEntity 未 override），1 格高方块(碰撞箱 0..1)无法步进上去，
// 必须跳跃。若跳跃触发或物理执行有缺陷，牛会卡在 1 格高墙前无法翻越。
//
// 寻路侧已核查无阻塞：WalkNodeProcessor::getNeighbors 对 1 格高 Blocked 方块的墙顶正确判定为 Walkable
// 邻居（WalkNodeProcessor.cpp:338-346），PathNavigator 能规划翻越路径。故牛会朝墙寻路，触发
// MovementController 跳跃判定——若跳跃异常，牛卡墙前 x 停在障碍前无法越过。
//
// 驱动方式：TemptGoal（牛被手持小麦玩家诱惑）。CowEntity registerGoals 注册 TemptGoal（诱惑物=小麦，
// 检测范围 10 格，scaredByMovement=false），tick 调 navigator()->moveTo(player) 朝玩家寻路。玩家静止
// 持小麦即可触发（对齐 cow_follows_wheat 范式）。
//
// 环境选择：mediumglass（12×9×11 走廊，helper y=2 z=5 x=4..10 共 7 格直线空气走廊，对齐 cow_follows_wheat）。
//   关键：经基线验证，牛在 mediumglass 走廊能从 x=10 朝玩家 x=2 正常移动到 x≈4.4（寻路+物理均正常），
//   故选用 mediumglass 而非 creeper_pit 自建场景（creeper_pit 自建开放场地牛不动——疑 worldgen 包围
//   致寻路/物理被困，且 setBlockType 自建方块与结构原生方块行为不一致）。mediumglass 走廊 helper-y=2
//   z=5 x=4..10 是空气走廊（牛实测可行走），在此段放障碍触发跳跃。
//
// 几何（helper 坐标）：
//   - 障碍：helper-y=2 的 (7,2,5) 放 stone，1 格高（占据牛脚部行走层，顶面比地板顶面高 1 格）。
//     牛 stepHeight=0.6 < 1.0 无法步进，须跳跃翻越。stone 实心方块碰撞箱 0..1。
//   - 牛 spawn (10,2,5)：走廊远端，距玩家(2,2,5) 8 格 < TemptRange 10，前方 x=7 障碍。
//   - 玩家 (2,2,5) 持小麦：走廊另一端当诱饵。玩家静止，TemptGoal 驱动牛朝玩家寻路。
//
// 坐标映射：结构放置 +1 抬升（helper-y=N 对应结构内 y=N-1），但 setBlockType/spawn 均用 helper 相对
// 坐标，框架统一处理映射，测试代码无需关心绝对 Y。
//
// 判定手段：牛被诱惑朝玩家(2,2,5)寻路，遇 x=7 障碍须跳跃翻越。pollUntilSucceed 轮询牛世界坐标 x
// 越过障碍（x < 6.5）即证明成功翻越（牛朝 -x 方向走，越过 x=7 后 x<6.5）；若跳跃异常，牛卡在 x≈7.5
// 无法越过，超时 onTimeout 报错。用 x 阈值判定吸收牛翻越后继续前冲的位置波动。
//
// 时序：TemptGoal 每 tick 评估 + 寻路起步 + 朝障碍移动(3 格) + 跳跃 + 落到墙顶 + 跳下 + 到玩家侧。
//   牛 MOVEMENT_SPEED=0.2，诱惑速度 1.0 倍=0.2/tick，3 格约 15 tick，但寻路起步+跳跃+落定有延迟，
//   startTick=40 留寻路稳定时间，maxTick=600 留充裕余量吸收非确定性。
// Ref: MovementController.cpp:155-220（跳跃触发条件）
// Ref: LivingEntity.cpp:1897-1933（aiStep 跳跃执行）
// Ref: CowEntity.cpp registerGoals（TemptGoal 小麦诱惑）/ registerAttributes（MOVEMENT_SPEED=0.2）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// mediumglass 结构尺寸 12×9×11（helper x∈[0,11], y∈[0,8], z∈[0,10]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 12, y: 9, z: 11 };

// 障碍墙位置：x=7，牛从 x=10 朝玩家 x=2 走，遇 x=7 障碍须跳。
const OBSTACLE_X = 7;
// 牛翻越障碍的判定阈值：牛朝 -x 走，越过 x=7 后 x < 6.5 即成功翻越。
const CROSS_THRESHOLD = OBSTACLE_X - 0.5;

// 牛被手持小麦玩家诱惑，遇 1 格高方块墙须跳跃翻越才能到达玩家侧。
//
// 复现"生物很难跳上一格高方块"：若 MovementController 跳跃触发或 LivingEntity::jump 物理执行有缺陷，
// 牛会卡在障碍墙前(x≈7.5)无法翻越，pollUntilSucceed 超时失败。正常情况下牛应跳过 1 格高墙到达玩家侧。
function cowJumpsOverOneBlockWall(test: Test): void {
  const cowType = "cow";

  // 1) 障碍墙：helper-y=2 的 (7,2,5) 放 stone，1 格高（占据牛脚部行走层，顶面比地板顶面高 1 格）。
  //    牛 stepHeight=0.6 < 1.0 无法步进，须跳跃翻越。stone 实心方块碰撞箱 0..1。
  //    牛从 x=10 朝玩家 x=2 走，遇 x=7 障碍须跳。x=4..10 经基线验证是空气可行走走廊。
  test.setBlockType("minecraft:stone", { x: OBSTACLE_X, y: 2, z: 5 });

  // 2) 玩家 (2,2,5) 持小麦：走廊一端当诱饵。玩家静止（SimulatedPlayer 默认不动），TemptGoal 检测
  //    到 10 格内持麦玩家即驱动牛寻路朝玩家移动。对齐 cow_follows_wheat 范式。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");
  const wheat = new ItemStack("minecraft:wheat", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(wheat as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 3) 牛 spawn (10,2,5)：走廊远端，距玩家 8 格 < TemptRange 10，前方 x=7 障碍。
  test.spawn(cowType, { x: 10, y: 2, z: 5 });

  // 4) 轮询断言：牛越过障碍墙（世界坐标 x < 6.5）即成功翻越。
  //    正常：牛被诱惑→朝玩家寻路→遇 x=7 障碍→跳跃翻越→落到玩家侧 x<6.5。
  //    异常（跳跃缺陷）：牛卡在 x≈7.5 无法翻越，超时 onTimeout 报错（复现"跳不上一格高方块"）。
  pollUntilSucceed(test, () => {
    const cows = test.getDimension().getEntities({
      type: cowType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (cows.length === 0) return false;
    return cows[0].location.x < CROSS_THRESHOLD;
  }, {
    startTick: 40,
    interval: 10,
    maxTick: 600,
    onTimeout: () => {
      const cows = test.getDimension().getEntities({
        type: cowType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const x = cows.length > 0 ? cows[0].location.x.toFixed(2) : "n/a";
      test.assert(false,
        `cow did not jump over 1-block wall: cowX=${x} (obstacle=${OBSTACLE_X}, threshold=${CROSS_THRESHOLD}, player=2)`);
    },
  });
}

export function registerJumpTests(): void {
  GameTest.register("MobBehaviorTests", "cow_jumps_over_one_block_wall", cowJumpsOverOneBlockWall)
    .structureName("gametests:mediumglass")
    .maxTicks(700);
}
