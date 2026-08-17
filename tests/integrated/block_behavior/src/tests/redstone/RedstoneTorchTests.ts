// 红石火把（redstone_torch）非门逻辑 GameTest。
//
// wiki tech_红石火把.txt#用途（:69-101）：红石火把是逻辑类似非门的红石传输元件：
//   - 未激活时亮起并输出信号（强度 15）；激活时熄灭并取消输出信号（:72）。
//   - 输入条件（:75-76）：自身的附着（下方支撑方块）被充能时激活（熄灭）。红石火把从接收到输入到改变
//     输出有 2 游戏刻延迟（:79）。
//   - 输出（:97-101）：未激活（亮起）时激活毗邻（除附着外）的红石线/中继器/比较器/机械元件 15 信号，
//     强充能正上方的红石导体至 15。
//   - 烧毁（:104-130）：JE 60 tick 内熄灭 8 次烧毁；BE 输入只有自身时燃烧值递增烧毁。烧毁涉频繁翻转
//     时序，GameTest 非确定，本文件不测。
//   - 放置（:51-59）：必须放置于支撑形状上表面中心完整的方块上方，支撑变动/水流/活塞推动时掉落。
// 红石火把默认点亮（lit=true），破坏掉落自身（:46）。
//
// C++ 链路：RedstoneTorchBlock（redstone/RedstoneTorchBlock.cpp）单 state LIT（bool，默认 true）：
//   - shouldBeOff（:70-76）：检查 belowPos（火把下方支撑方块）是否从 Down 方向被强充能
//     （RedstonePower::isSidePowered(belowPos, Down)）。被强充能 → 应熄灭（shouldBeOff=true）。
//   - onBlockAdded（:83-102）：放置时通知六方向邻居 + 检查初始 shouldBeLit=!shouldBeOff，不一致则
//     scheduleBlockTick(REDSTONE_DELAY)。
//   - neighborChanged（:120-132）：邻居变化时 updateState。
//   - updateState（:200-211）：shouldBeLit != isCurrentlyLit 时 scheduleBlockTick(REDSTONE_DELAY,
//     ExtremelyHigh)。
//   - tick（:134-159）：执行时 shouldBeLit != isLit →（检查烧毁）→ setBlockState(lit=shouldBeLit) +
//     updateNeighborsExcept(Down)。
//   - getWeakPower（:161-184）：lit 且非烧毁且 side!=Down 时返回 15；熄灭返回 0。
//   - getStrongPower（:186-191）：仅 Down 方向返回 weakPower（强充能下方方块）。
//   - 红石块（RedstoneBlock.cpp）getStrongPower 全向 15，在火把支撑方块下方放置即强充能支撑方块，
//     适合作测试电源（同 RedstoneBlockTests 场景 2 范式）。
//
// 派发链路：setBlockType("minecraft:redstone_torch", pos) → setBlockState flags=3 → onBlockAdded
//   （检查初始状态）。setBlockType("minecraft:redstone_block", belowPos) → flags=3 → 邻居（支撑方块）
//   neighborChanged + 红石系统更新传播到火把。红石火把注册 BlockItemRegistry.cpp:1117
//   registerSimpleBlock(REDSTONE_TORCH, "redstone_torch")。方块注册 RedstoneBlocks.cpp:127。
//
// 测试覆盖（4 个场景，覆盖 wiki 非门逻辑核心确定行为，可跨服务端对比）：
//   1. 红石火把默认点亮：放置（支撑方块上，无电源）→ lit=true（非门默认输出，shouldBeOff=false）。
//   2. 支撑方块被强充能时熄灭：火把支撑方块下方放红石块强充能支撑方块 → lit=false（非门「附着被充能
//      则熄灭」+ 2 tick 延迟）。
//   3. 移除充能后复燃：火把已熄灭（支撑方块被强充能），移除红石块 → lit=true（非门恢复输出 +
//      2 tick 延迟）。
//   4. 红石火把破坏不崩溃：放火把 → setBlockType air → 位置变 air。
//
// 关键约束：
// 1. 场景 1 火把布局：支撑 stone A=(3,2,1)，火把=(3,3,1)（A 顶面）。setBlockType 放火把绕过
//    isValidPosition 直接放置。onBlockAdded 检查 shouldBeOff（belowPos=A，A 下方 (3,1,1) 为 air 无电源
//    → isSidePowered(A, Down)=false → shouldBeOff=false → shouldBeLit=true=isLit(true) → 无需翻转，
//    保持 lit=true）。pollUntilSucceed 轮询 lit=true 留 REDSTONE_DELAY 余量。
// 2. 场景 2 强充能：火把 lit=true 稳定后，(3,1,1) 放红石块 → 对 A 输出 strongPower(Down)=15 → A 从
//    Down 强充能 → shouldBeOff（isSidePowered(A, Down)=true）=true → shouldBeLit=false ≠ isLit(true) →
//    scheduleBlockTick(REDSTONE_DELAY) → tick setBlockState(lit=false)。分阶段：先确认 lit=true，再放
//    红石块，再轮询 lit=false。REDSTONE_DELAY 通常为 2 tick，startTick 留余量。
// 3. 场景 3 复燃：承接场景 2 终态（火把 lit=false，支撑方块被强充能）。移除红石块 (3,1,1)→air → A
//    不再被强充能 → shouldBeOff=false → shouldBeLit=true ≠ isLit(false) → scheduleBlockTick
//    (REDSTONE_DELAY) → tick setBlockState(lit=true) 复燃。分阶段：先确认 lit=false，再移除红石块，
//    再轮询 lit=true。
// 4. 场景 4 放火把后 setBlockType air 破坏：RedstoneTorchBlock::onBlockRemoved（:104-118）清理烧毁记录
//    + 通知邻居，位置变 air。断言变 air（链路不崩溃）。破坏掉落物非确定，仅测变 air。
// 5. 读 lit 用 getState("lit" as any)（BooleanProperty，C++ 属性名 "lit"，返 bool）。
// 6. glass_pit 结构 7×5×7（x,z∈[0,6], y∈[0,4]），y=0 glass 底，y=1..4 air 空腔。场景 1/2/3 用
//    y=1（红石块）/y=2（支撑 stone）/y=3（火把）三层，均在空腔内。
// 7. 注意 wiki :81-84「附着在红石块上的红石火把刚放置亮一段再熄灭」是 BE 特殊时序（火把直接附着红石块）。
//    本测试红石块在火把支撑方块下方（火把附着 stone，非红石块），测稳定的非门逻辑，避开该特殊时序。
//
// 不测「红石火把输出 15 信号激活毗邻红石线/机械元件」：需布置红石线/红石灯在火把毗邻（除下方附着），
//   且火把 lit 时供电，链路可测但属输出验证，本文件聚焦非门输入逻辑（亮/灭/复燃）。TODO: 可补
//   redstone_torch_powers_adjacent_components_when_lit。
// 不测「强充能正上方红石导体 15」：需布置红石导体在火把正上方 + 比较器探测，链路复杂，跳过。
// 不测「烧毁/复燃」：JE 60 tick 8 次翻转 / BE 燃烧值递增，需高频脉冲触发，GameTest 非确定，跳过。
//   TODO: 待高频脉冲测试范式完善后补 redstone_torch_burnout_after_rapid_flips。
// 不测「2 游戏刻延迟精确值」：REDSTONE_DELAY 常量值需对齐 vanilla 2 tick，但 GameTest tick 粒度 + 时序
//   偏差使精确断言不可靠，仅测翻转最终态（lit=false/true），不测精确延迟 tick。
// 不测「墙红石火把（redstone_wall_torch）」：墙火把 facing + 附着侧面，放置形态选择链路复杂，本文件
//   聚焦落地红石火把非门逻辑。TODO: 可补 redstone_wall_torch_placement_facing。
// 不测「含水 waterlogged」：涉流体，跳过。
// 不测「支撑变动掉落」：与 TorchTests 支撑自毁范式同构，跳过。
//
// 跨服务端：redstone_torch 方块名两端一致。lit state 名两端一致（C++ 内部名 "lit"）。非门逻辑（附着被
//   充能则熄灭、移除充能复燃）+ 2 tick 延迟两端与 vanilla 一致。烧毁机制 JE/BE 实现不同（不测）。
//   非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石火把.txt#用途（非门：未激活亮输出15/激活熄灭）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石火把.txt#输入（附着被充能时激活，2 tick 延迟）
// Ref: RedstoneTorchBlock.cpp（shouldBeOff 检查 belowPos 从 Down 强充能 / onBlockAdded / neighborChanged
//   / updateState / tick 翻转 lit / getWeakPower/getStrongPower）
// Ref: RedstoneBlock.cpp（getStrongPower 全向 15，强充能支撑方块作测试电源）
// Ref: RedstoneBlockTests.ts（红石块强充能火把支撑方块使其熄灭范式，本文件扩展为含复燃）
// Ref: RedstoneLampTests.ts / CopperBulbTests.ts（分阶段 runAtTickTime + pollUntilSucceed 红石块电源范式）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=0 为 glass 底，y=1..4 为 air 空腔。
// 场景 1/2/3：支撑 stone A=(3,2,1) + 红石火把 (3,3,1)（A 顶面）+ 红石块 (3,1,1)（A 下方强充能 A）。
// 场景 4：红石火把 (3,2,1)（下方 (3,1,1) stone 支撑，破坏不崩溃）。

// 读取 (x,y,z) 红石火把 lit state（bool）。返回 null 表示读取失败或非红石火把。
// LIT() 的 C++ 属性名为 "lit"（BooleanProperty，默认 true）。
function getTorchLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 放置火把测试基础结构：支撑 stone A=(3,2,1) + 红石火把 (3,3,1)（A 顶面）+ 确保 (3,1,1) 为 air（无电源）。
function placeTorchOnSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:redstone_torch", { x: 3, y: 3, z: 1 });
}

// 场景 1：红石火把默认点亮——放置（支撑方块上，无电源）→ lit=true。
//
// 布局：支撑 stone A=(3,2,1) + 红石火把 (3,3,1)（A 顶面，setBlockType 绕过 isValidPosition 直接放置）。
//   onBlockAdded 检查 shouldBeOff（belowPos=A，A 下方 (3,1,1)=air 无电源 → isSidePowered(A, Down)=false
//   → shouldBeOff=false → shouldBeLit=true=isLit(true) → 无需翻转，保持 lit=true）。
//
// 判定：pollUntilSucceed 轮询 (3,3,1) lit===true（非门默认输出，无电源点亮）。
//
// 此场景验证 wiki「红石火把未激活时亮起并输出信号」+ 默认 lit=true：放置无电源时火把点亮（非门默认
//   输出 15），与 vanilla RedstoneTorchBlock 默认 LIT=true 一致。
function redstoneTorchLitByDefault(test: Test): void {
    placeTorchOnSupport(test);
    test.assert(getBlockTypeId(test, 3, 3, 1) === "minecraft:redstone_torch", `redstone_torch should be at (3,3,1), got ${getBlockTypeId(test, 3, 3, 1)}`);

    // 轮询断言 lit===true（非门默认点亮，onBlockAdded 检查后保持 lit=true，留 REDSTONE_DELAY 余量）。
    pollUntilSucceed(
        test,
        () => getTorchLit(test, 3, 3, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should be true by default (no power), got ${getTorchLit(test, 3, 3, 1)}`);
            },
        },
    );
}

// 场景 2：支撑方块被强充能时熄灭——火把支撑方块下方放红石块强充能支撑方块 → lit=false。
//
// 布局：承接场景 1——支撑 stone A=(3,2,1) + 火把 (3,3,1) lit=true。火把 lit=true 稳定后，(3,1,1) 放
//   红石块 → 对 A 输出 strongPower(Down)=15 → A 从 Down 强充能 → shouldBeOff（isSidePowered(A, Down)
//   =true）=true → shouldBeLit=false ≠ isLit(true) → scheduleBlockTick(REDSTONE_DELAY) → tick
//   setBlockState(lit=false)。
//
// 判定：分阶段——先 pollUntilSucceed 确认火把初始 lit=true，再 runAtTickTime 放红石块强充能，再
//   pollUntilSucceed 轮询 lit===false（非门「附着被充能则熄灭」+ 2 tick 延迟）。
//
// 此场景验证 wiki「红石火把自身附着被充能时激活（熄灭）」+ 2 tick 延迟：红石块强充能支撑方块 A，
//   火把检测到 A 被充能而熄灭（非门翻转），经 REDSTONE_DELAY tick 延迟落地。
function redstoneTorchExtinguishesWhenSupportPowered(test: Test): void {
    placeTorchOnSupport(test);

    // 阶段 1：等火把初始 lit=true 稳定（无电源点亮）。
    pollUntilSucceed(
        test,
        () => getTorchLit(test, 3, 3, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should be true initially, got ${getTorchLit(test, 3, 3, 1)}`);
            },
        },
    );

    // 阶段 2：tick 8 放红石块强充能支撑方块 A。确认 lit=true 后放红石块。
    test.runAtTickTime(8, () => {
        if (getTorchLit(test, 3, 3, 1) !== true) {
            test.assert(false, `redstone_torch should be lit=true before powering support, got ${getTorchLit(test, 3, 3, 1)}`);
            return;
        }
        // (3,1,1) 放红石块 → 对 A=(3,2,1) strongPower(Down)=15 → A 从 Down 强充能 → 火把应熄灭。
        test.setBlockType("minecraft:redstone_block", { x: 3, y: 1, z: 1 });
    });

    // 阶段 3：轮询火把 lit===false（A 被强充能 → 火把非门熄灭，经 REDSTONE_DELAY tick 延迟）。
    //   startTick=12 留红石块放置 + REDSTONE_DELAY 延迟余量。
    pollUntilSucceed(
        test,
        () => getTorchLit(test, 3, 3, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should be false when support block powered, got ${getTorchLit(test, 3, 3, 1)}`);
            },
        },
    );
}

// 场景 3：移除充能后复燃——火把已熄灭（支撑方块被强充能），移除红石块 → lit=true。
//
// 布局：承接场景 2——火把 (3,3,1) lit=false，(3,1,1) 红石块强充能支撑方块 A。火把 lit=false 稳定后，
//   (3,1,1) 设 air 移除红石块 → A 不再被强充能 → shouldBeOff=false → shouldBeLit=true ≠ isLit(false)
//   → scheduleBlockTick(REDSTONE_DELAY) → tick setBlockState(lit=true) 复燃。
//
// 判定：分阶段——先 pollUntilSucceed 确认火把 lit=false（已熄灭），再 runAtTickTime 移除红石块，再
//   pollUntilSucceed 轮询 lit===true（非门恢复输出 + 2 tick 延迟）。
//
// 此场景验证 wiki「红石火把未激活时亮起」的恢复路径：移除支撑方块的充能后，shouldBeOff 恢复 false，
//   火把经 REDSTONE_DELAY tick 延迟复燃 lit=true，恢复非门默认输出。与场景 2 配对验证非门双向翻转。
function redstoneTorchRelightsWhenPowerRemoved(test: Test): void {
    placeTorchOnSupport(test);

    // 阶段 1：等火把初始 lit=true 稳定。
    pollUntilSucceed(
        test,
        () => getTorchLit(test, 3, 3, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should be true initially, got ${getTorchLit(test, 3, 3, 1)}`);
            },
        },
    );

    // 阶段 2：tick 8 放红石块强充能支撑方块 A（火把应熄灭）。
    test.runAtTickTime(8, () => {
        if (getTorchLit(test, 3, 3, 1) !== true) {
            test.assert(false, `redstone_torch should be lit=true before powering support, got ${getTorchLit(test, 3, 3, 1)}`);
            return;
        }
        test.setBlockType("minecraft:redstone_block", { x: 3, y: 1, z: 1 });
    });

    // 阶段 3：tick 16 确认火把已熄灭 lit=false 后移除红石块。留 REDSTONE_DELAY 延迟余量（8+8=16）。
    test.runAtTickTime(16, () => {
        if (getTorchLit(test, 3, 3, 1) !== false) {
            test.assert(false, `redstone_torch should be lit=false (extinguished) before removing power, got ${getTorchLit(test, 3, 3, 1)}`);
            return;
        }
        // (3,1,1) 设 air 移除红石块（红石块→air 真实状态变化，派发邻居更新）。A 不再被强充能 →
        //   火把 shouldBeOff=false → shouldBeLit=true ≠ isLit(false) → scheduleBlockTick 复燃。
        test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });
    });

    // 阶段 4：轮询火把 lit===true（移除充能后非门复燃，经 REDSTONE_DELAY tick 延迟）。
    //   startTick=20 留移除红石块 + REDSTONE_DELAY 延迟余量（16+4=20）。
    pollUntilSucceed(
        test,
        () => getTorchLit(test, 3, 3, 1) === true,
        {
            startTick: 20,
            interval: 4,
            maxTick: 70,
            onTimeout: () => {
                test.assert(false, `redstone_torch lit: should relight true after power removed, got ${getTorchLit(test, 3, 3, 1)}`);
            },
        },
    );
}

// 场景 4：红石火把破坏不崩溃——放火把 → setBlockType air 破坏 → 位置变 air。
//
// 布局：支撑 stone (3,1,1) + 红石火把 (3,2,1)（stone 顶面，setBlockType 直接放置）。
// setBlockType("minecraft:air", (3,2,1)) 破坏火把 → RedstoneTorchBlock::onBlockRemoved（清理烧毁记录 +
//   通知邻居），位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（火把已破坏，链路不崩溃）。
//
// 此场景验证红石火把破坏链路安全性：放火把后破坏，onBlockRemoved 清理 + 通知邻居不崩溃，位置正确变 air。
//   破坏掉落物（wiki :46 破坏掉落自身）非确定（项目范式不验证掉落物实体），故仅测变 air。
function redstoneTorchBreaksWhenRemoved(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:redstone_torch", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:redstone_torch", `redstone_torch should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏火把 → onBlockRemoved 清理烧毁记录 + 通知邻居 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言火把 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `redstone_torch pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerRedstoneTorchTests(): void {
    GameTest.register("BlockBehaviorTests", "redstone_torch_lit_by_default", redstoneTorchLitByDefault)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "redstone_torch_extinguishes_when_support_powered", redstoneTorchExtinguishesWhenSupportPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "redstone_torch_relights_when_power_removed", redstoneTorchRelightsWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(140);
    GameTest.register("BlockBehaviorTests", "redstone_torch_breaks_when_removed", redstoneTorchBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
