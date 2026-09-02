// 测重压力板信号强度随实体数变化行为 GameTest。
//
// wiki block_测重压力板.txt#红石元件：测重压力板根据其上方实体数量输出不同信号强度。
//   - 轻质测重压力板（金）：信号强度 = min(实体数, 15)（每实体+1）
//   - 重质测重压力板（铁）：信号强度 = min(实体数 / 10, 15)（每10实体+1）
//   与木/石压力板（固定 15）不同，测重压力板持久化 power 0-15（POWER_0_15 state，属性名 power）。
//
// vanilla 对齐（WeightedPressurePlateBlock.java + PressurePlateBlock）：
//   - calculateSignalStrength：轻质 min(count, 15)，重质 min(count/10, 15)
//   - getRedstoneStrength：实体数 → 信号强度映射
//   - 持久化 POWER 0-15（非木/石 POWERED bool）
//
// C++ 链路（WeightedPressurePlateBlock.cpp）：
//   - calculateSignalStrength（:70-88）：_getEntityCount 统计检测框内实体数，
//     轻质返回 min(count, 15)，重质返回 min(count/10, 15)
//   - _getEntityCount（:121-145）：检测框 (0.125,0,0.125)-(0.875,0.25,0.875)，
//     getEntitiesInAABB 统计实体，排除 doesEntityNotTriggerPressurePlate() 的实体
//   - getStoredSignal（:90-94）：读 POWER_0_15（信号强度 0-15）
//   - withStoredSignal（:96-99）：写 POWER_0_15
//   - 基类 AbstractPressurePlateBlock::tick 调 calculateSignalStrength 更新 power + 通知邻居
//
// 测试覆盖（2 个场景，覆盖 wiki 测重压力板信号强度=实体数核心行为）：
//   1. light_weighted_pressure_plate_signal_one_with_single_entity：
//      1 个猪站到轻质测重压力板上 → power=1（非 15，证明测重信号强度=实体数）
//   2. light_weighted_pressure_plate_signal_two_with_two_entities：
//      2 个不同 mob（猪+牛）站到轻质测重压力板上 → power=2（轻质每实体+1）
//
// 关键约束：
// 1. 测重压力板放 (3,2,1)，下方 (3,1,1) 放 stone 支撑（压力板须放在支撑形状完整的方块上方）。
// 2. 实体 spawn (3,3,1) 落到压力板上方（自由落体 1 格，脚进入检测框触发）。
//    2 个实体场景用不同 typeId（pig + cow）避免合并，spawn 在 (3,3,1) 和 (3,3.5,1) 错开高度。
// 3. 读 power state 用 getState("power" as any)，返回 number（0-15）。
// 4. 测重压力板每 10gt 更新状态，实体站上去后最多 10gt 延迟才更新 power。
//
// 不测「重质测重压力板」：重质需 10 个实体才 power=1，spawn 10 个 mob 不现实且时序非确定，跳过。
//   TODO: 待多实体精确控制链路稳定后补 heavy_weighted_pressure_plate_signal 测试。
//
// 跨服务端：light_weighted_pressure_plate 方块名两端一致。power state 名两端一致。
//   测重压力板信号强度=实体数行为与 vanilla 一致，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_测重压力板.txt#红石元件
// Ref: WeightedPressurePlateBlock.cpp:70-145（calculateSignalStrength/_getEntityCount）
// Ref: AbstractPressurePlateBlock.cpp（tick 调 calculateSignalStrength 更新 power）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 压力板放 (3,2,1)，下方 (3,1,1) 放 stone 支撑。
// 实体 spawn (3,3,1) 落到压力板上方。

// 读取测重压力板 power state（int 0-15）。返回 null 表示失败或非压力板。
function getPlatePower(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("power" as any);
    return typeof value === "number" ? value : null;
}

// 在 (3,1,1) 放 stone 支撑 + (3,2,1) 放测重压力板。
function placeWeightedPlateOnStone(test: Test, plateType: string): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType(plateType, { x: 3, y: 2, z: 1 });
}

// 场景 1：1 个猪站到轻质测重压力板上 → power=1。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) light_weighted_pressure_plate，猪 spawn (3,3,1) 落到压力板上方。
// 猪碰撞箱脚部进入检测框 → onEntityCollision 调度 tick → calculateSignalStrength（轻质 min(count,15)=1）
// → power=1。
//
// 判定：pollUntilSucceed 轮询 power===1（测重压力板每 10gt 更新，留余量）。
//   - 若 power=0：猪未进入检测框或 calculateSignalStrength 链路断裂。
//   - 若 power=15：误用木/石压力板逻辑（固定 15）而非测重逻辑（实体数）。
function lightWeightedPlateSignalOneWithSingleEntity(test: Test): void {
    placeWeightedPlateOnStone(test, "minecraft:light_weighted_pressure_plate");
    test.spawn("pig", { x: 3, y: 3, z: 1 });

    pollUntilSucceed(
        test,
        () => getPlatePower(test, 3, 2, 1) === 1,
        {
            startTick: 10,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `light_weighted_plate: power should be 1 with single entity, got ${getPlatePower(test, 3, 2, 1)} `
                        + `(if 0: entity not in detection box or calculateSignalStrength broken; `
                        + `if 15: wrong plate logic [fixed 15] not weighted [entity count])`,
                );
            },
        },
    );
}

// 场景 2：2 个不同 mob（猪+牛）站到轻质测重压力板上 → power=2。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) light_weighted_pressure_plate。
// spawn pig (3,3,1) + cow (3,3,1) 同位置错开高度落下，两者碰撞箱都进入检测框 →
// calculateSignalStrength（轻质 min(count,15)=2）→ power=2。
//
// 判定：pollUntilSucceed 轮询 power===2（2 个实体均进入检测框后更新）。
//   - 若 power=1：仅 1 个实体进入检测框（另一实体被推挤出框或未落地）。
//   - 若 power=0：两实体均未进入检测框。
function lightWeightedPlateSignalTwoWithTwoEntities(test: Test): void {
    placeWeightedPlateOnStone(test, "minecraft:light_weighted_pressure_plate");
    test.spawn("pig", { x: 3, y: 3, z: 1 });
    test.spawn("cow", { x: 3, y: 3, z: 1 });

    pollUntilSucceed(
        test,
        () => getPlatePower(test, 3, 2, 1) === 2,
        {
            startTick: 10,
            interval: 4,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `light_weighted_plate_two_entities: power should be 2 with two entities, got ${getPlatePower(test, 3, 2, 1)} `
                        + `(if 1: only one entity in detection box [other pushed out or not landed]; `
                        + `if 0: both entities not in detection box)`,
                );
            },
        },
    );
}

export function registerWeightedPressurePlateTests(): void {
    GameTest.register(
        "BlockBehaviorTests",
        "light_weighted_pressure_plate_signal_one_with_single_entity",
        lightWeightedPlateSignalOneWithSingleEntity,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(80);

    GameTest.register(
        "BlockBehaviorTests",
        "light_weighted_pressure_plate_signal_two_with_two_entities",
        lightWeightedPlateSignalTwoWithTwoEntities,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
