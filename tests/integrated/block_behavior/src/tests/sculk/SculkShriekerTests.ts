// 幽匿尖啸体实体踩踏触发尖啸行为 GameTest。
//
// wiki tech_幽匿尖啸体.txt#激活机制：当非潜行实体踩在幽匿尖啸体上时，尖啸体会发出尖啸游戏事件，
//   触发附近的幽匿感测体/尖啸体。尖啸体被激活后进入尖啸状态（90 tick），尖啸期间不再重复激活。
//   自然生成的尖啸体（can_summon=true）在尖啸结束时会尝试召唤监守者。
//
// C++ 链路：SculkShriekerBlock（SculkBlocks.cpp:864-1019）有 SHRIEKING/CAN_SUMMON/WATERLOGGED state。
//   - onEntityWalk（SculkBlocks.cpp:949-964）：非潜行实体踩上时发 SHRIEK 游戏事件。
//     调用点 Entity::doBlockCollisions() 末尾（m_onGround && !isSteppingCarefully），
//     每帧触发（对齐 vanilla Entity.move 末尾 if(onGround) block.stepOn(...) 语义）。
//   - tick（SculkBlocks.cpp:966-985）：SHRIEKING 状态到期后转回非 SHRIEKING。
//   - shriek（SculkBlocks.cpp:1002-1019）：设 SHRIEKING=true，调度 SHRIEKING_TICKS(90) 后 tick，
//     播放粒子 + 发 SHRIEK 事件。
//   - tryShriek（SculkShriekerHelper.cpp:59-99）：解析玩家→检查 SHRIEKING 状态→重置 warningLevel→
//     _canRespond 时 _tryWarn→无条件 shriek()（_canRespond || warningIncreased）。
//   - canReceiveVibration（SculkVibrationSystem.cpp:131-150）：只响应 SHRIEK 事件 + 潜行门控。
//     不排除自身，故同一 shrieker 自己发的 SHRIEK 会传回给自己。
//
// 振动传播时序（VibrationSystemServer.cpp）：
//   onEntityWalk 发 SHRIEK 事件 → GameEventDispatcher::post → visitInRangeListeners →
//   Listener::handleGameEvent → scheduleVibration → addCandidate(info, gameTick)。
//   下一 tick 的 tickAll() → Ticker::tick → trySelectAndScheduleVibration（travelTime=floor(distance)）
//   → 递减 → 归零时 receiveVibration → onReceiveVibration → SculkShriekerHelper::tryShriek →
//   shriek() → SHRIEKING=true。
//
// 测试覆盖（1 个场景，覆盖 wiki 实体踩踏触发尖啸核心行为）：
//   1. sculk_shrieker_shrieks_when_walked_on：玩家落到 shrieker 上 → onEntityWalk → SHRIEK 事件 →
//      shrieker 自己接收 → tryShriek → shriek() → SHRIEKING=true。
//
// 关键约束：
// 1. sculk_shrieker 是半格高方块（8 像素），直接放 (3,2,1)（glass_pit 内部 air 腔 helper y=2）。
//    setBlockType 走默认 state（shrieking=false, can_summon=false, waterlogged=false），放置时创建
//    SculkShriekerBlockEntity + 注册振动监听器（ServerWorld::setBlockEntity → registerSculkShrieker）。
// 2. can_summon=false（默认）：_canRespond 返回 false，tryShriek 走 !canRespond 分支 → shriek()。
//    不召唤监守者（避免复杂生成链路），但仍触发 SHRIEKING 状态。这验证 wiki「踩踏触发尖啸」核心行为。
// 3. onEntityWalk 需要 m_onGround（实体落地）+ !isSteppingCarefully（非潜行）。
//    SimulatedPlayer 在 shrieker 上方 (3,3,1) 生成，自由落体落到 shrieker 顶部（半格高 y=2.5）。
//    落地时 m_onGround=true，doBlockCollisions（每帧调用）检测 belowPos=shrieker 位置 → onEntityWalk。
// 4. 读 shrieking 用 getState("shrieking" as any)，值域 true/false。
// 5. 振动传播有时序不确定性（依赖 tickAll 调度），用 pollUntilSucceed 轮询读 shrieking。
//    玩家从 y=3 落到 y=2.5 约需 5-10 tick（重力加速），SHRIEK 事件传播延迟约 1-3 tick。
//
// 不测「can_summon=true 召唤监守者」：依赖 _trySummonWarden + 难度 + 游戏规则 + 附近监守者检查 +
//   实体注册 + 生成位置搜索，链路过长且部分依赖未就绪。TODO: 待监守者召唤链路完善后补测试。
// 不测「警告等级递增」：tryShriek 内 _tryWarn 递增 wardenWarningEffect，但 Block JS 无 getComponent
//   访问方块实体数据。TODO: 待脚本侧方块实体数据读取链路打通后补 warningLevel 测试。
// 不测「黑暗效果应用」：_applyDarknessAround 对半径 40 格内玩家应用黑暗效果，需读玩家效果列表，
//   脚本侧 API 限制跳过。TODO: 待玩家效果读取链路打通后补 darkness 测试。
// 不测「尖啸体间连锁激活」：一个 shrieker 尖啸发 SHRIEK 事件 → 附近 shrieker 接收 → 连锁尖啸。
//   需双 shrieker 布局 + 精确振动传播时序，复杂跳过。TODO: 待连锁激活时序稳定后补测试。
//
// 跨服务端：sculk_shrieker 方块名两端一致。shrieking state 名两端一致（布尔）。
//   实体踩踏触发尖啸行为与 vanilla 一致，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_幽匿尖啸体.txt#激活机制（踩踏触发尖啸）
// Ref: SculkBlocks.cpp:949-1019（onEntityWalk/shriek/tick 尖啸触发与状态转换）
// Ref: SculkShriekerHelper.cpp:59-99（tryShriek 尖啸体激活逻辑）
// Ref: SculkVibrationSystem.cpp:131-167（canReceiveVibration/onReceiveVibration 振动接收）
// Ref: Entity.cpp doBlockCollisions()（onEntityWalk 调用点：每帧触发，Player 经 updatePhysics 调入）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 布局：
//   (3,1,1) stone 支撑（shrieker 半格高，需下方固体方块支撑放置）
//   (3,2,1) sculk_shrieker（半格高，振动监听器位置；顶部 y=2.5）
//   (3,3,1) SimulatedPlayer 生成位置（shrieker 上方 1 格，自由落体落到 shrieker 顶部）

const SHRIEKER_POS = { x: 3, y: 2, z: 1 };
const SUPPORT_POS = { x: 3, y: 1, z: 1 };
const SPAWN_POS = { x: 3, y: 3, z: 1 };

// 读取 shrieking（布尔）。返回 null 表示失败或非尖啸体。
function getShrieking(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("shrieking" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：玩家落到 shrieker 上 → SHRIEKING=true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) sculk_shrieker（can_summon=false 默认）。
// SimulatedPlayer 在 (3,3,1) 生成（shrieker 上方），自由落体落到 shrieker 顶部（y=2.5）。
// 落地时 m_onGround=true + !isSteppingCarefully → doBlockCollisionsAfterMove →
//   blockPos=shrieker 位置 → SculkShriekerBlock::onEntityWalk → 发 SHRIEK 事件。
// SHRIEK 事件 → shrieker 自己接收（canReceiveVibration 不排除自身，distance=0, travelTime=0）→
//   下一 tick tickAll → onReceiveVibration → SculkShriekerHelper::tryShriek →
//   _canRespond（false，can_summon=false）→ !canRespond 分支 → shriek() → SHRIEKING=true。
//
// 判定：pollUntilSucceed 轮询读 shrieking === true。
function sculkShriekerShrieksWhenWalkedOn(test: Test): void {
    // 支撑 + shrieker。
    test.setBlockType("minecraft:stone", SUPPORT_POS);
    test.setBlockType("minecraft:sculk_shrieker", SHRIEKER_POS);

    // 在 shrieker 上方生成 SimulatedPlayer，让其自由落体落到 shrieker 顶部。
    test.spawnSimulatedPlayer(SPAWN_POS, "faller");

    // 轮询读 shrieking=true。玩家从 y=3 落到 y=2.5 约需 5-10 tick（重力加速），
    // SHRIEK 事件传播延迟约 1-3 tick。
    pollUntilSucceed(
        test,
        () => getShrieking(test, 3, 2, 1) === true,
        {
            startTick: 5,
            interval: 2,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `shrieker should be shrieking after player landed on it, got shrieking=${getShrieking(test, 3, 2, 1)}`,
                );
            },
        },
    );
}

export function registerSculkShriekerTests(): void {
    GameTest.register("BlockBehaviorTests", "sculk_shrieker_shrieks_when_walked_on", sculkShriekerShrieksWhenWalkedOn)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
