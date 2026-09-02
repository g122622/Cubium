// 标靶方块投射物命中红石信号行为 GameTest。
//
// 验证 Cubium 标靶方块在箭矢命中时输出红石信号（power state），并在持续时间后归零，
// 对齐 MC Java 1.21.11 TargetBlock + AbstractArrow.onProjectileHit 链路。
//
// C++ 链路：
//   test.spawn("minecraft:arrow", pos) + entity.setVelocity({x,y,z})
//     → AbstractArrowEntity::tick（AbstractArrowEntity.cpp）performRayTrace 用 m_velocity 做射线终点
//     → 命中方块 onBlockHit（AbstractArrowEntity.cpp:582）→ 末尾补发 block.onProjectileHit(...)
//       （本修复新增：此前 onBlockHit 重写未调 onProjectileHit，致标靶链路断裂）
//     → TargetBlock::onProjectileHit（TargetBlock.cpp）：
//       strength = getRedstoneStrength(hitPos, hitFace)  // 命中面平面内两轴偏移最大值
//       duration = isArrow ? 20 : 8
//       若无已调度 tick：setBlockState(power=strength) + scheduleBlockTick(duration)
//     → tick 回调（TargetBlock::tick）：power != 0 时重置为 0
//
// vanilla 对齐（TargetBlock.java:42-89 + AbstractArrow）：
//   - onProjectileHit（:42-49）调 updateRedstoneOutput
//   - updateRedstoneOutput（:51-59）：j = instanceof AbstractArrow ? 20 : 8；
//     !hasScheduledTick 时 setOutputPower（设 state + scheduleTick(j)）
//   - getRedstoneStrength（:61-77）：取命中面 axis，d3 = max(面内两轴偏移)，
//     return ceil(15 * clamp((0.5-d3)/0.5, 0, 1))，至少 1
//   - tick（:85-89）：power != 0 时 setBlock(power=0)
//
// 前置能力（任务 #324）：Entity.setVelocity({x,y,z}) 绑定。投射物下一 tick 用此速度做 raytrace 命中目标。
// 静止投射物 performRayTrace delta≈0 必 miss，永不触发 onBlockHit，致命中链路端到端测试不可构造。
//
// 防假通过设计（正反对照）：
//   - target_emits_power_when_hit_by_arrow：箭矢 setVelocity 朝标靶飞 → 命中 → power > 0。
//     断言：标靶 power state ∈ [1, 15]（命中链路：onBlockHit→onProjectileHit→setBlockState）。
//     若 onProjectileHit 链路断裂（onBlockHit 未补发通知）：power 恒 0 → FAIL。
//     若 getRedstoneStrength 计算错误返回 0/负数：power ∉ [1,15] → FAIL。
//   - target_power_resets_after_duration：箭矢命中后 power > 0，等 ACTIVATION_TICKS_ARROWS(20)+tick 后
//     power 归零（tick 回调重置）。断言：①命中后 power > 0；②等待后 power === 0。
//     若 tick 回调未重置：power 恒 > 0 → ②失败 → FAIL。
//     若 scheduleBlockTick 未调度：tick 永不执行，power 恒 > 0 → ②失败 → FAIL。
//
// 时序：
//   - tick 0：放标靶 (3,2,5)；spawn 箭矢 (3,2,2) + setVelocity({0,0,3.0})（朝 +Z 飞，1 tick 跨 3 格）。
//   - tick 1：箭矢 tick，performRayTrace 射线 z∈[2,5] 覆盖标靶 z=5 → onBlockHit → onProjectileHit
//     → setBlockState(power=strength) + scheduleBlockTick(20)。
//   - tick 2~21：标靶 power > 0（scheduleBlockTick(20) 在 tick 21 触发 tick 回调重置 power=0）。
//   - tick 5：断言 power ∈ [1,15]（命中后稳定）。
//   - tick 25：断言 power === 0（tick 回调已重置）。
//
// 坐标设计（glass_pit 7×5×7，helper 相对坐标 x,z∈[0,6], y∈[0,4]）：
//   - 标靶 (3,2,5)：air 空腔内，箭矢从 -Z 方向命中其 north 面。
//   - 箭矢 spawn (3,2,2)：距标靶 3 格，setVelocity({0,0,3.0}) 1 tick 命中。
//   - 注：glass_pit y=1..2 是 air 空腔，箭矢在 y=2 飞行无遮挡。
//
// className 恒为 BlockBehaviorTests（对齐 block_behavior 包约定）。
// Ref: TargetBlock.java:42-89（vanilla onProjectileHit/updateRedstoneOutput/getRedstoneStrength/tick）
// Ref: TargetBlock.cpp（onProjectileHit/getRedstoneStrength/tick 实现）
// Ref: AbstractArrowEntity.cpp:582（onBlockHit 末尾补发 onProjectileHit，本修复新增）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed, waitForCondition } from "../../utils/test/poll.js";

const ARROW_TYPE = "minecraft:arrow";
const TARGET_BLOCK = "minecraft:target";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 玻璃底，y=1..2 air 空腔（玻璃墙）。箭矢在 y=2 飞行无遮挡。
const TARGET_POS = { x: 3, y: 2, z: 5 };
const ARROW_SPAWN_POS = { x: 3, y: 2, z: 2 };

// 读取标靶方块 power state（int 0-15）。返回 null 表示读取失败或非标靶。
function getTargetPower(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("power" as any);
    return typeof value === "number" ? value : null;
}

// 生成朝标靶飞行的箭矢：spawn 箭矢 + setVelocity 朝 +Z 飞（1 tick 跨 3 格命中标靶 north 面）。
// 先放标靶方块，再 spawn 箭矢并设速度。
function spawnArrowAimingTarget(test: Test): void {
    test.setBlockType(TARGET_BLOCK, TARGET_POS);
    const arrow = test.spawn(ARROW_TYPE, ARROW_SPAWN_POS);
    // setVelocity 朝 +Z 3.0/tick，1 tick 跨 3 格命中标靶 z=5。
    (arrow as any).setVelocity({ x: 0, y: 0, z: 3.0 });
}

// 箭矢命中标靶后输出红石信号（power ∈ [1, 15]）。
//
// 箭矢 (3,2,2) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 跨 3 格命中标靶 (3,2,5) north 面。
// onBlockHit → onProjectileHit → setBlockState(power=strength) + scheduleBlockTick(20)。
//
// 判定（tick 5，命中后稳定）：
//   标靶 power state ∈ [1, 15]（命中链路：onBlockHit→onProjectileHit→setBlockState）。
//   - 若 onProjectileHit 链路断裂（onBlockHit 未补发通知）：power 恒 0 ∉ [1,15] → FAIL。
//   - 若 getRedstoneStrength 计算错误返回 0/负数：power ∉ [1,15] → FAIL。
function targetEmitsPowerWhenHitByArrow(test: Test): void {
    spawnArrowAimingTarget(test);

    pollUntilSucceed(
        test,
        () => {
            const power = getTargetPower(test, TARGET_POS.x, TARGET_POS.y, TARGET_POS.z);
            return power !== null && power >= 1 && power <= 15;
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                const power = getTargetPower(test, TARGET_POS.x, TARGET_POS.y, TARGET_POS.z);
                test.assert(
                    false,
                    `target_emits_power_when_hit_by_arrow: power should be in [1,15], got ${power} `
                        + `(if 0/null: onProjectileHit link broken [onBlockHit did not call onProjectileHit] `
                        + `or getRedstoneStrength returned 0; if >15: getRedstoneStrength overflow)`,
                );
            },
        },
    );
}

// 箭矢命中标靶后 power > 0，等 ACTIVATION_TICKS_ARROWS(20)+tick 后 power 归零。
//
// 阶段 1（tick 5）：断言命中后 power > 0（同 target_emits_power_when_hit_by_arrow）。
// 阶段 2（tick 25）：等 scheduleBlockTick(20) 触发 tick 回调重置 power=0 后，断言 power === 0。
//
// 判定：
//   ① 命中后 power > 0（命中链路正常）。
//   ② 等待后 power === 0（tick 回调重置 + scheduleBlockTick 调度正常）。
//   - 若 tick 回调未重置：power 恒 > 0 → ②失败 → FAIL。
//   - 若 scheduleBlockTick 未调度：tick 永不执行，power 恒 > 0 → ②失败 → FAIL。
function targetPowerResetsAfterDuration(test: Test): void {
    spawnArrowAimingTarget(test);

    // 阶段 1：等命中后 power > 0。
    waitForCondition(
        test,
        () => {
            const power = getTargetPower(test, TARGET_POS.x, TARGET_POS.y, TARGET_POS.z);
            return power !== null && power >= 1;
        },
        () => {
            // 阶段 2：power > 0 满足后，等 scheduleBlockTick(20) 触发 tick 回调重置 power=0。
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                const power = getTargetPower(test, TARGET_POS.x, TARGET_POS.y, TARGET_POS.z);
                test.assert(
                    false,
                    `target_power_resets_after_duration phase1: power should be > 0 after hit, got ${power} `
                        + `(if 0/null: onProjectileHit link broken or arrow did not hit target)`,
                );
            },
        },
    );

    // 阶段 2：等 tick 回调重置后 power === 0。
    // scheduleBlockTick(20) 在命中后 20 tick 触发 tick 回调，重置 power=0。
    // 命中约在 tick 1，故 tick 21 触发重置。startTick=25 留足余量。
    waitForCondition(
        test,
        () => getTargetPower(test, TARGET_POS.x, TARGET_POS.y, TARGET_POS.z) === 0,
        () => {
            test.succeed();
        },
        {
            startTick: 25,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                const power = getTargetPower(test, TARGET_POS.x, TARGET_POS.y, TARGET_POS.z);
                test.assert(
                    false,
                    `target_power_resets_after_duration phase2: power should be 0 after duration, got ${power} `
                        + `(if >0: tick callback did not reset [scheduleBlockTick not scheduled or tick not executed])`,
                );
            },
        },
    );
}

export function registerTargetBlockTests(): void {
    GameTest.register("BlockBehaviorTests", "target_emits_power_when_hit_by_arrow", targetEmitsPowerWhenHitByArrow)
        .structureName("gametests:glass_pit")
        .maxTicks(80);

    GameTest.register("BlockBehaviorTests", "target_power_resets_after_duration", targetPowerResetsAfterDuration)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
