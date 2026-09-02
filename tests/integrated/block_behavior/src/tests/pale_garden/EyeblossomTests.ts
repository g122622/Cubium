// 眼眸花昼夜节律状态切换行为 GameTest。
//
// wiki block_眼眸花.txt#行为：眼眸花有两种状态——开放（open_eyeblossom）和闭合（closed_eyeblossom）。
//   主世界昼夜节律下，眼眸花会在开/合状态间切换：
//   - 夜晚（dayTimeOfDay ∈ [12600, 23401)）→ 应开放（Open）
//   - 白天 → 应闭合（Closed）
//   切换由 randomTick 触发：检查 EYEBLOSSOM_OPEN 环境属性，若与当前状态不一致则切换为反状态方块。
//   开放状态发光等级 1，闭合状态不发光。
//
// vanilla 对齐（EyeblossomBlock.java + EyeblossomEnvironment）：
//   - randomTick（:81）：调 tryChangingState，若切换则连锁触发周围 3×2×3 范围同种眼眸花
//   - tryChangingState（:264）：读 EYEBLOSSOM_OPEN 环境属性（主世界按 dayTimeOfDay 判断昼夜）
//     → 若 targetOpen != currentOpen → transform() 取反状态方块 → setBlockState 切换
//   - 主世界 EYEBLOSSOM_OPEN 关键帧：tod ∈ [12600, 23401) 返回 True（夜晚开放），其他返回 False
//
// C++ 链路（EyeblossomBlock.cpp + EyeblossomEnvironment.hpp）：
//   - ticksRandomly() 返回 true（hpp:110）
//   - randomTick（:81）→ tryChangingState（:264）→ getEyeblossomOpen（EyeblossomEnvironment.hpp:66）
//   - getEyeblossomOpen：主世界按 dayTimeOfDay 判断，tod ∈ [12600, 23401) 返回 True，其他 False
//   - 切换：transform()（:186）取反状态方块 → setBlockState（:283）
//   - 连锁：周围 3×2×3 范围内同种眼眸花方块调度延迟 tick（:296-316）
//
// 测试覆盖（2 个场景，覆盖 wiki 昼夜节律状态切换核心行为）：
//   1. open_eyeblossom_closes_in_daytime：白天放 open_eyeblossom + 调高 randomTickSpeed
//      → randomTick 触发 tryChangingState → 白天应闭合 → 切换为 closed_eyeblossom
//   2. closed_eyeblossom_stays_closed_in_daytime：白天放 closed_eyeblossom + 调高 randomTickSpeed
//      → 白天应闭合（与当前一致）→ 不切换（保持 closed_eyeblossom）
//
// 关键约束：
// 1. GameTest 默认批次为白天（tod=6000），getEyeblossomOpen 返回 False（白天应闭合）。
// 2. randomTick 触发概率性（ticksRandomly=true），默认 randomTickSpeed=3 时单格每 tick 命中概率
//    仅 3/4096≈0.073%，短时间命中概率极低。测试开头用 SimulatedPlayer.chat("/gamerule randomTickSpeed 1000")
//    调高 randomTickSpeed，使单格每 tick 命中概率≈24.4%，约 5 tick 内确定性命中。
// 3. 读 typeId 验证状态切换：open_eyeblossom → closed_eyeblossom（白天应闭合）。
// 4. 测试运行时间短（约 100 tick），tod 推进有限，不会从白天推进到夜晚。
//
// 不测「夜晚开放切换」：需 night 批次（tod=18000）放 closed_eyeblossom → 切换为 open_eyeblossom。
//   night 批次 tod=18000 ∈ [12600, 23401) → getEyeblossomOpen 返回 True（夜晚应开放）。
//   TODO: 待 night 批次测试稳定后补 closed_eyeblossom_opens_at_night 测试。
// 不测「连锁触发」：需双 eyeblossom 布局 + 精确连锁时序，复杂跳过。
//   TODO: 待连锁激活时序稳定后补 eyeblossom_chain_switches 测试。
// 不测「蜜蜂中毒」：onEntityCollision → BeeEntity 站入开放眼眸花 → addEffect(Poison, 25 ticks)。
//   需读 bee 实体的 poison effect，复杂跳过。TODO: 待蜜蜂效果读取链路打通后补 bee_poison 测试。
//
// 跨服务端：open_eyeblossom/closed_eyeblossom 方块名两端一致。
//   昼夜节律状态切换行为与 vanilla 一致，可跨服务端对比。
//   注：randomTickSpeed 调高用 SimulatedPlayer.chat 执行 /gamerule，基岩 BDS chat 是发消息语义
//   （不执行命令），故本组用 chat 调高 randomTickSpeed 的测试基岩侧 one-sided（同 IceMeltTests）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_眼眸花.txt#行为（昼夜节律切换）
// Ref: EyeblossomBlock.cpp:81-316（randomTick/tryChangingState/transform 切换链路）
// Ref: EyeblossomEnvironment.hpp:66-90（getEyeblossomOpen 主世界昼夜判断）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 眼眸花放 (3,2,1)（glass_pit 内部 air 腔 helper y=2），下方 (3,1,1) 是 glass_pit 玻璃底支撑。
const EYEBLOSSOM_POS = { x: 3, y: 2, z: 1 };
// SimulatedPlayer 生成位置（air 区域，用于执行 /gamerule randomTickSpeed 1000）。
const PLAYER_POS = { x: 1, y: 2, z: 1 };

// 调高 randomTickSpeed 使眼眸花方块在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%，约 5 tick 内确定性命中。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId，缺失返回 undefined。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string | undefined {
    const block = test.getBlock(pos);
    return block?.typeId;
}

// 调高 randomTickSpeed（SimulatedPlayer 执行 /gamerule）。不 assert chat 返回值。
function raiseRandomTickSpeed(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);
}

// 场景 1：白天放 open_eyeblossom + 调高 randomTickSpeed → randomTick 触发 tryChangingState
// → 白天应闭合 → 切换为 closed_eyeblossom。
//
// 判定：pollUntilSucceed 轮询 typeId === "minecraft:closed_eyeblossom"。
//   - 若仍为 open_eyeblossom：randomTick 未触发（randomTickSpeed 未调高）或 tryChangingState 链路断裂。
//   - 若切换为 closed_eyeblossom：昼夜节律状态切换核心行为验证通过。
function openEyeblossomClosesInDaytime(test: Test): void {
    test.setBlockType("minecraft:open_eyeblossom", EYEBLOSSOM_POS);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => getTypeId(test, EYEBLOSSOM_POS) === "minecraft:closed_eyeblossom",
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                test.assert(
                    false,
                    `open_eyeblossom_closes_in_daytime: expected open->closed, got ${getTypeId(test, EYEBLOSSOM_POS)} `
                        + `(if still open_eyeblossom: randomTick not triggered [randomTickSpeed not raised] `
                        + `or tryChangingState link broken; daytime tod=6000 should close open eyeblossom)`,
                );
            },
        },
    );
}

// 场景 2：白天放 closed_eyeblossom + 调高 randomTickSpeed → 白天应闭合（与当前一致）→ 不切换
// → 保持 closed_eyeblossom。
//
// 判定：pollUntilSucceed 轮询 typeId === "minecraft:closed_eyeblossom"（保持不变）。
//   - 若切换为 open_eyeblossom：tryChangingState 逻辑错误（白天不应开放）。
//   - 若保持 closed_eyeblossom：白天应闭合与当前一致，不切换，行为正确。
function closedEyeblossomStaysClosedInDaytime(test: Test): void {
    test.setBlockType("minecraft:closed_eyeblossom", EYEBLOSSOM_POS);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => getTypeId(test, EYEBLOSSOM_POS) === "minecraft:closed_eyeblossom",
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                test.assert(
                    false,
                    `closed_eyeblossom_stays_closed_in_daytime: expected stay closed, got ${getTypeId(test, EYEBLOSSOM_POS)} `
                        + `(if switched to open_eyeblossom: tryChangingState logic wrong [daytime should stay closed])`,
                );
            },
        },
    );
}

export function registerEyeblossomTests(): void {
    GameTest.register("BlockBehaviorTests", "open_eyeblossom_closes_in_daytime", openEyeblossomClosesInDaytime)
        .structureName("gametests:glass_pit")
        .maxTicks(180);

    GameTest.register(
        "BlockBehaviorTests",
        "closed_eyeblossom_stays_closed_in_daytime",
        closedEyeblossomStaysClosedInDaytime,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(180);
}
