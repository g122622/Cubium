// 幽匿感测体振动激活/相位转换行为 GameTest。
//
// wiki tech_幽匿感测体.txt#振动检测：幽匿感测体可以检测半径 8 格内的振动（游戏事件）。
//   检测到振动后进入活跃阶段（30 tick），输出红石信号（power，基于振动距离计算）。
//   活跃阶段结束后进入冷却阶段（10 tick），冷却结束后回到不活跃阶段。
//   活跃期间不再接收新振动（canActivate 仅 Inactive 返回 true）。
//
// C++ 链路：SculkSensorBlock（SculkBlocks.cpp:253-487）有 SCULK_SENSOR_PHASE/POWER/WATERLOGGED state。
//   - activate（SculkBlocks.cpp:387-434）：设 Phase=Active + POWER=redstoneStrength，调度 ACTIVE_TICKS(30)
//     后 tick，通知邻居红石更新，触发共振事件，发 SCULK_SENSOR_TENDRILS_CLICKING 声音。
//   - deactivate（SculkBlocks.cpp:436-449）：设 Phase=Cooldown, Power=0，调度 COOLDOWN_TICKS(10) 后 tick。
//   - tick（SculkBlocks.cpp:451-476）：Active→deactivate()→Cooldown；Cooldown→Inactive（播放停止声音）。
//   - canReceiveVibration（SculkVibrationSystem.cpp:62-99）：frequency≠0 + 潜行门控 +
//     canActivate(state)（Phase==Inactive）+ 拒绝来自自身位置的 BLOCK_DESTROY/BLOCK_PLACE。
//   - onReceiveVibration（SculkVibrationSystem.cpp:101-125）：更新 lastVibrationFrequency，
//     计算 redstoneStrength（getRedstoneStrengthForDistance），调 SculkSensorBlock::activate。
//
// 振动传播时序（VibrationSystemServer.cpp）：
//   1. world.gameEvent(FLUID_PLACE, cauldronPos, waterCauldronState) → GameEventDispatcher::post →
//      visitInRangeListeners → Listener::handleGameEvent
//   2. handleGameEvent 检查 isValidVibration + canReceiveVibration + _isOccluded →
//      scheduleVibration → selectionStrategy().addCandidate(info, gameTick)
//   3. 下一 tick 的 tickAll() → Ticker::tick → trySelectAndScheduleVibration（从候选选最优，
//      设 travelTimeInTicks = floor(distance)）→ 递减 → 归零时 receiveVibration →
//      onReceiveVibration → activate
//
// 触发源：空炼药锅 + 水桶 → 水炼药锅，发 FLUID_PLACE 事件（频率 5）。
//   CauldronBlock::_handleBucketInteraction（CauldronBlock.cpp:240-260）：手持 water_bucket 右键空炼药锅 →
//   setBlockState(水炼药锅) + gameEvent(FLUID_PLACE)。
//   onBlockActivated 不依赖 openContainer（不打开 GUI），在 GameTestServer 中可正常工作。
//   SimulatedPlayer::useItemOnBlock(water_bucket, cauldronPos) → useItemOnBlock 内部把 stack 设到主手
//   选中槽 → onBlockActivated 通过 player.getHeldItem(hand) 读到 water_bucket → 加水路径。
//
// 测试覆盖（3 个场景，覆盖 wiki 振动检测 + 相位转换 Inactive→Active→Cooldown→Inactive）：
//   1. sculk_sensor_activates_on_fluid_place：炼药锅加水 → sensor 激活（Phase=active, power>0）。
//   2. sculk_sensor_deactivates_after_active_period：触发激活 → 等 30+ tick → Phase=cooldown。
//   3. sculk_sensor_returns_to_inactive_after_cooldown：触发激活 → 等 30+10+ tick → Phase=inactive。
//
// 关键约束：
// 1. sculk_sensor 是半格高方块（8 像素），直接放 (3,2,1)（glass_pit 内部 air 腔 helper y=2）。
//    setBlockType 走默认 state（inactive, power=0），放置时创建 SculkSensorBlockEntity +
//    注册振动监听器（ServerWorld::setBlockEntity → registerSculkSensor）。
// 2. 触发源炼药锅放 (4,2,1)（sensor 旁边，水平距离 1 格）。加水发 FLUID_PLACE 事件（频率 5），
//    事件位置 = 炼药锅位置 (4,2,1)，listener 位置 = sensor 位置 (3,2,1)，距离 = 1.0 格。
//    振动传播延迟 = floor(1.0) = 1 tick。从加水到 sensor 激活总延迟约 2-3 tick。
// 3. 读 sculk_sensor_phase 用 getState("sculk_sensor_phase" as any)，值域 inactive/active/cooldown。
//    读 power 用 getState("power" as any)，值域 0-15。
// 4. 振动传播有时序不确定性（依赖 tickAll 调度），用 pollUntilSucceed 轮询读 phase。
// 5. SimulatedPlayer 默认创造模式。useItemOnBlock 把 water_bucket 设到主手选中槽，
//    onBlockActivated 内部 !creativeMode 分支跳过消耗，加水路径正常执行。
//
// 不测「校准感测体频率过滤」：canReceiveVibration 未实现按红石信号过滤频率，行为与 vanilla 不一致。
//   TODO: 待校准感测体频率过滤实现后补测试。
// 不测「水浸状态下不检测振动」：canReceiveVibration 未检查 waterlogged 状态，行为与 vanilla 不一致。
//   TODO: 待水浸检测链路打通后补测试。
// 不测「比较器输出振动频率」：脚本侧无直接读比较器输出 API，需比较器方块+红石线链路，复杂跳过。
//   TODO: 待比较器读取链路打通后补比较器信号测试。
//
// 跨服务端：sculk_sensor 方块名两端一致。sculk_sensor_phase state 名两端一致（Java 式
//   sculk_sensor_phase enum，值 inactive/active/cooldown），power state 名两端一致（power 0-15）。
//   振动检测 + 相位转换行为与 vanilla 一致，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_幽匿感测体.txt#振动检测（活跃/冷却/不活跃相位）
// Ref: SculkBlocks.cpp:387-476（activate/deactivate/tick 相位转换）
// Ref: SculkVibrationSystem.cpp:62-125（canReceiveVibration/onReceiveVibration 振动接收）
// Ref: VibrationSystemServer.cpp:233-345（handleGameEvent/scheduleVibration/Ticker::tick 传播时序）
// Ref: CauldronBlock.cpp:240-260（_handleBucketInteraction 水桶加水发 FLUID_PLACE 事件）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 布局：
//   (3,1,1) stone 支撑（sensor 半格高，需下方固体方块支撑放置）
//   (3,2,1) sculk_sensor（半格高，振动监听器位置）
//   (4,2,1) cauldron（空炼药锅，振动源；FLUID_PLACE 事件位置）
// sensor (3,2,1) ↔ cauldron (4,2,1) 水平距离 1 格。

const SENSOR_POS = { x: 3, y: 2, z: 1 };
const CAULDRON_POS = { x: 4, y: 2, z: 1 };
const SUPPORT_POS = { x: 3, y: 1, z: 1 };

// 读取 sculk_sensor_phase（字符串：inactive/active/cooldown）。返回 null 表示失败或非感测体。
function getSensorPhase(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("sculk_sensor_phase" as any);
    return typeof value === "string" ? value : null;
}

// 读取 power（数字 0-15）。返回 null 表示失败或非感测体。
function getSensorPower(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("power" as any);
    return typeof value === "number" ? value : null;
}

// 放置 sculk_sensor + 炼药锅：stone 支撑 + sensor + 空炼药锅。
function placeSensorAndCauldron(test: Test): void {
    test.setBlockType("minecraft:stone", SUPPORT_POS);
    test.setBlockType("minecraft:sculk_sensor", SENSOR_POS);
    test.setBlockType("minecraft:cauldron", CAULDRON_POS);
}

// 触发振动：SimulatedPlayer 用水桶右键空炼药锅 → 加水 → FLUID_PLACE 事件 → sensor 激活。
// useItemOnBlock 把 water_bucket 设到主手选中槽 → onBlockActivated 读到 water_bucket → 加水路径。
function triggerVibration(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "opener");
    const waterBucket = new ItemStack("minecraft:water_bucket", 1);
    player.useItemOnBlock(
        waterBucket as unknown as Parameters<typeof player.useItemOnBlock>[0],
        CAULDRON_POS,
        Direction.Up,
    );
}

// 场景 1：炼药锅加水 → sensor 激活（Phase=active, power>0）。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) sculk_sensor + (4,2,1) cauldron。
// SimulatedPlayer useItemOnBlock(water_bucket, cauldron) → CauldronBlock::onBlockActivated →
//   _handleBucketInteraction → setBlockState(水炼药锅) + gameEvent(FLUID_PLACE)。
// FLUID_PLACE 事件 → sensor listener 接收（distance=1.0, travelTime=1 tick）→
//   onReceiveVibration → activate（Phase=Active, Power=redstoneStrength>0）。
//
// 判定：pollUntilSucceed 轮询读 sculk_sensor_phase === "active" 且 power > 0。
function sculkSensorActivatesOnFluidPlace(test: Test): void {
    placeSensorAndCauldron(test);
    triggerVibration(test);

    pollUntilSucceed(
        test,
        () => {
            const phase = getSensorPhase(test, 3, 2, 1);
            const power = getSensorPower(test, 3, 2, 1);
            return phase === "active" && power !== null && power > 0;
        },
        {
            startTick: 5,
            interval: 2,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `sensor should be active after cauldron fill, got phase=${getSensorPhase(test, 3, 2, 1)} power=${getSensorPower(test, 3, 2, 1)}`,
                );
            },
        },
    );
}

// 场景 2：触发激活 → 等 30+ tick → Phase=cooldown。
//
// 布局：同场景 1。
// 加水 → sensor 激活（Active, 30 tick）。ACTIVE_TICKS=30 后 tick → deactivate() →
//   Phase=Cooldown, Power=0，调度 COOLDOWN_TICKS(10) 后 tick。
//
// 判定：pollUntilSucceed 轮询读 phase=cooldown。
function sculkSensorDeactivatesAfterActivePeriod(test: Test): void {
    placeSensorAndCauldron(test);
    triggerVibration(test);

    pollUntilSucceed(
        test,
        () => getSensorPhase(test, 3, 2, 1) === "cooldown",
        {
            startTick: 31,
            interval: 2,
            maxTick: 50,
            onTimeout: () => {
                test.assert(
                    false,
                    `sensor should enter cooldown after active period, got phase=${getSensorPhase(test, 3, 2, 1)}`,
                );
            },
        },
    );
}

// 场景 3：触发激活 → 等 30+10+ tick → Phase=inactive。
//
// 布局：同场景 1。
// 加水 → sensor 激活（Active, 30 tick）→ deactivate → Cooldown（10 tick）→
//   tick → Inactive（播放停止声音）。
//
// 判定：pollUntilSucceed 轮询读 phase=inactive。
function sculkSensorReturnsToInactiveAfterCooldown(test: Test): void {
    placeSensorAndCauldron(test);
    triggerVibration(test);

    pollUntilSucceed(
        test,
        () => getSensorPhase(test, 3, 2, 1) === "inactive",
        {
            startTick: 41,
            interval: 2,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `sensor should return to inactive after cooldown, got phase=${getSensorPhase(test, 3, 2, 1)}`,
                );
            },
        },
    );
}

export function registerSculkSensorTests(): void {
    GameTest.register("BlockBehaviorTests", "sculk_sensor_activates_on_fluid_place", sculkSensorActivatesOnFluidPlace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "sculk_sensor_deactivates_after_active_period", sculkSensorDeactivatesAfterActivePeriod)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "sculk_sensor_returns_to_inactive_after_cooldown", sculkSensorReturnsToInactiveAfterCooldown)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
