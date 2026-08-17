// 苍白垂须（pale_hanging_moss）集成测试：验证支撑放置、失支撑自毁、链式自毁、链式存活行为
// （对齐 wiki 苍白垂须#用途 :42 放置于下表面完整方块底部 / :44 破坏垂须一同摧毁其下方所有连接垂须）。
//
// wiki block_苍白垂须.txt：
//   #用途（:42）："苍白垂须可以放置在下表面[[方块支撑形状]]完整的方块底部。"——垂须悬挂于上方实心方块底部。
//   #用途（:44）："破坏苍白垂须会一同摧毁其下方所有连接着的苍白垂须。"——链式自毁：顶部支撑失效 →
//     该节垂须自毁 → 其下方垂须失去上方垂须支撑 → 逐级自毁，整条垂须链全部消失。
//   #骨粉（:50）："对苍白垂须使用骨粉会使其向下生长一格。"——骨粉行为依赖 useItem 链路，本测试不覆盖
//     （BoneMealTests 已覆盖骨粉范式），聚焦支撑/链式自毁。
//   #方块状态（:107）："{{Block states table}}"——wiki 方块状态表为空占位，TIP 状态无明文记录。
//     按「不为未记录行为写测试」准则，**不测 TIP 状态**（虽 Cubium 实现了 TIP 重算，但 wiki 未记录两端行为）。
//
// ============================ Cubium 实现链路 ============================
// PaleHangingMossBlock（pale_garden/PaleHangingMossBlock.cpp）：
//   - isValidPosition（:82-105）：上方方块须 isSolidSide(Down)（提供向下实心面，如石头），**或** 上方方块
//     本身是 PaleHangingMossBlock（dynamic_cast 判定，允许垂须链式悬挂——下方垂须的支撑是其上方的垂须）。
//   - updatePostPlacement（:107-129）：任意方向邻居变化时重检 isValidPosition，**失支撑则调度
//     scheduleBlockTick(currentPos, *this, 1, Normal)**（固定 1 tick 延迟）；同时重算 TIP（下方非垂须→tip=true，
//     下方是垂须→tip=false，:122-128）。
//   - tick（:131-143）：到点重检 isValidPosition，仍失支撑则 setBlockState(air, 3) 自毁。
//
// 链式自毁链路：移除顶部支撑 stone → A 节 updatePostPlacement 检测上方 air 非 solidSide 且非垂须 →
//   isValidPosition 失败 → scheduleBlockTick(A, 1) → 1 tick 后 A tick → setBlockState(air) 自毁。
//   A 自毁派发邻居更新 → B 节 updatePostPlacement 检测上方 A 变 air → isValidPosition 失败（上方非垂须）→
//   scheduleBlockTick(B, 1) → 1 tick 后 B tick → 自毁。逐级传播，每级 1 tick 延迟。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。
//
// 布局列 (3,*,1)（同 BigDripleafTests/BubbleColumnTests 坐标范式）：
//   - 支撑 stone (3,3,1)：垂须上方实心支撑。
//   - 垂须 A (3,2,1)：上方 stone 支撑。
//   - 垂须 B (3,1,1)：上方 A 支撑（链式，B 的支撑是垂须 A 而非 stone）。y=0 是结构底 cobblestone/glass，
//     B 悬挂于 A 下方，下方是否实心不影响垂须 isValidPosition（垂须只查上方）。
//
// 测试1 pale_hanging_moss_survives_with_solid_support_above（单格垂须有上方实心支撑存活，正向防误判）：
//   stone (3,3,1) + 垂须 A (3,2,1)。A 上方 stone isSolidSide(Down) 满足 isValidPosition。不做任何破坏。
//   断言 A 仍是 pale_hanging_moss（防 updatePostPlacement 误触发 scheduleBlockTick 自毁）。
//
// 测试2 pale_hanging_moss_chain_survives_with_top_support（垂须链有顶部支撑存活，链式正向）：
//   stone (3,3,1) + A (3,2,1) + B (3,1,1)。A 上方 stone、B 上方 A（垂须链）。两端 isValidPosition 满足。
//   断言 A、B 均仍是 pale_hanging_moss（防链式 isValidPosition 误判失支撑自毁）。
//
// 测试3 pale_hanging_moss_breaks_when_support_above_removed（移除上方支撑单格自毁，wiki :42/:44 支撑）：
//   stone (3,3,1) + A (3,2,1)。t=20 移除 stone(3,3,1)→air。A updatePostPlacement 检测上方 air 非 solidSide
//   且非垂须 → isValidPosition 失败 → scheduleBlockTick(1) → 1 tick 后 tick → setBlockState(air) 自毁。
//   断言 A 变 air。
//
// 测试4 pale_hanging_moss_chain_breaks_when_top_support_removed（移除顶部支撑链式自毁，wiki :44 链式）：
//   stone (3,3,1) + A (3,2,1) + B (3,1,1)。t=20 移除 stone(3,3,1)→air。A 失支撑自毁（同测试3），
//   A 自毁派发邻居更新 → B updatePostPlacement 检测上方 A 变 air → isValidPosition 失败 →
//   scheduleBlockTick(1) → 自毁。断言 A、B 均变 air（链式逐级自毁，每级 1 tick 延迟，给足轮询窗口）。
//
// ============================ 排除项（不写测试）============================
// - TIP 状态（末端/中间）：wiki 方块状态表为空占位（:107），TIP 行为无明文记录，按准则不测。
// - 骨粉向下生长（wiki :50）：依赖 useItem/骨粉链路，BoneMealTests 已覆盖骨粉范式，跳过。TODO: 待骨粉
//   useItem 链路对 pale_hanging_moss 验证后可补。
// - 环境音效（wiki :46-47，苍白橡木原木下方特殊音效）：纯音效，跳过。
// - 堆肥 30%（wiki :53）：依赖堆肥桶 useItem 链路 + 随机，跳过。
// - 剪子/精准采集掉落（wiki :37-39）：依赖物品工具判定 + 掉落物实体，跳过。
// - 熔岩引燃（wiki :118 历史 24w44a）：依赖火焰蔓延，跳过。
//
// ============================ 跨服务端对比 ============================
// - pale_hanging_moss typeId 两端一致（JE 1.21.2/1.21.4，BE 1.21.50，1.21.11 已含）。
// - 放置于下表面完整方块底部（:42）、破坏垂须一同摧毁下方连接垂须（:44 链式自毁），均为 wiki 明文记录
//   的两端一致行为。支撑自毁 + 链式自毁由 updatePostPlacement(同步邻居更新) + scheduleBlockTick(1)
//   (1 tick 延迟) 驱动，无光照/玩家/随机依赖，两端可对比。
// - 测试用 setBlockType 放 stone/pale_hanging_moss/air，均为两端通用 API，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_苍白垂须.txt#用途（:42 放置于下表面完整方块底部）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_苍白垂须.txt#用途（:44 破坏垂须一同摧毁下方连接垂须，链式自毁）
// Ref: PaleHangingMossBlock.cpp:82-105（isValidPosition 上方 isSolidSide(Down) 或上方垂须链）
// Ref: PaleHangingMossBlock.cpp:107-129（updatePostPlacement 失支撑→scheduleBlockTick(1)+TIP 重算）
// Ref: PaleHangingMossBlock.cpp:131-143（tick 重检 isValidPosition 失败→setBlockState air 自毁）
// Ref: BigDripleafTests.ts / BubbleColumnTests.ts（glass_pit 坐标范式 + pollUntilSucceed 轮询自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。支撑 stone (3,3,1)，垂须 A (3,2,1)，垂须 B (3,1,1)。
const SUPPORT = { x: 3, y: 3, z: 1 }; // 上方实心支撑 stone
const MOSS_A = { x: 3, y: 2, z: 1 }; // 垂须 A（上方 stone 支撑）
const MOSS_B = { x: 3, y: 1, z: 1 }; // 垂须 B（上方 A 支撑，链式）

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 放置支撑 stone + 单格垂须 A。A 上方 stone isSolidSide(Down) 满足 isValidPosition。
function placeSupportAndMoss(test: Test): void {
    test.setBlockType("minecraft:stone", SUPPORT);
    test.setBlockType("minecraft:pale_hanging_moss", MOSS_A);
}

// 单格垂须有上方实心支撑 → 存活不自毁（正向防误判，验证 isValidPosition 满足时不触发 scheduleBlockTick）。
// wiki :42 垂须放置于下表面完整方块底部。stone (3,3,1) + A (3,2,1)，不做破坏，等待后断言 A 仍存在。
function paleHangingMossSurvivesWithSolidSupportAbove(test: Test): void {
    placeSupportAndMoss(test);

    // 等待足够 tick（超过自毁的 1 tick 延迟窗口），断言 A 仍存在（isValidPosition 满足，不自毁）。
    // 若 updatePostPlacement 误触发 scheduleBlockTick，A 会在 1 tick 内消失，此测试会捕获。
    pollUntilSucceed(
        test,
        () => getTypeId(test, MOSS_A) === "minecraft:pale_hanging_moss",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `moss survive: expected pale_hanging_moss to remain at ${JSON.stringify(MOSS_A)} with stone above, ` +
                        `got ${getTypeId(test, MOSS_A)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, isValidPosition may falsely fail or updatePostPlacement may over-trigger self-destruct)`,
                );
            },
        },
    );
}

// 垂须链有顶部支撑 → A、B 均存活（链式正向，验证链式 isValidPosition 通过：B 的支撑是垂须 A）。
// wiki :44 垂须链式悬挂。stone (3,3,1) + A (3,2,1) + B (3,1,1)。不做破坏，等待后断言 A、B 均存在。
function paleHangingMossChainSurvivesWithTopSupport(test: Test): void {
    test.setBlockType("minecraft:stone", SUPPORT);
    test.setBlockType("minecraft:pale_hanging_moss", MOSS_A);
    test.setBlockType("minecraft:pale_hanging_moss", MOSS_B);

    pollUntilSucceed(
        test,
        () =>
            getTypeId(test, MOSS_A) === "minecraft:pale_hanging_moss" &&
            getTypeId(test, MOSS_B) === "minecraft:pale_hanging_moss",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `moss chain survive: expected both pale_hanging_moss at A=${JSON.stringify(MOSS_A)} and ` +
                        `B=${JSON.stringify(MOSS_B)} with top support, ` +
                        `got A=${getTypeId(test, MOSS_A)} B=${getTypeId(test, MOSS_B)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if B is air, chain isValidPosition (above is moss) may be missing)`,
                );
            },
        },
    );
}

// 移除上方支撑 → 单格垂须自毁（wiki :42/:44 支撑失效）。
// stone (3,3,1) + A (3,2,1)。t=20 移除 stone→air。A updatePostPlacement 检测上方 air → isValidPosition
// 失败 → scheduleBlockTick(1) → 1 tick 后 tick → setBlockState(air) 自毁。
function paleHangingMossBreaksWhenSupportAboveRemoved(test: Test): void {
    placeSupportAndMoss(test);

    // t=20 移除上方支撑 stone（→air），A updatePostPlacement 检测上方 air → isValidPosition 失败 →
    // scheduleBlockTick(1) → 1 tick 后 tick → setBlockState(air) 自毁。
    test.runAtTickTime(20, () => {
        // 仅在 A 仍存在时移除支撑（防预置失败误操作）。
        if (getTypeId(test, MOSS_A) === "minecraft:pale_hanging_moss") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 stone，派发邻居更新触发 A updatePostPlacement(Up)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, MOSS_A) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `moss break on support removed: expected air at ${JSON.stringify(MOSS_A)} after removing stone above, ` +
                        `got ${getTypeId(test, MOSS_A)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still pale_hanging_moss, updatePostPlacement support-fail->tick->air may be missing)`,
                );
            },
        },
    );
}

// 移除顶部支撑 → 链式自毁（wiki :44 破坏垂须一同摧毁其下方所有连接垂须）。
// stone (3,3,1) + A (3,2,1) + B (3,1,1)。t=20 移除 stone→air。A 失支撑自毁（同测试3），A 自毁派发
// 邻居更新 → B updatePostPlacement 检测上方 A 变 air → isValidPosition 失败 → scheduleBlockTick(1) →
// 自毁。逐级传播，每级 1 tick 延迟。
function paleHangingMossChainBreaksWhenTopSupportRemoved(test: Test): void {
    test.setBlockType("minecraft:stone", SUPPORT);
    test.setBlockType("minecraft:pale_hanging_moss", MOSS_A);
    test.setBlockType("minecraft:pale_hanging_moss", MOSS_B);

    // t=20 移除顶部支撑 stone（→air）。A 失支撑自毁（1 tick），A 自毁触发 B 失支撑自毁（再 1 tick）。
    test.runAtTickTime(20, () => {
        if (getTypeId(test, MOSS_A) === "minecraft:pale_hanging_moss") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 stone，触发链式自毁
        }
    });

    // 断言 A、B 均变 air（链式逐级自毁，B 比 A 晚 ~1 tick，给足轮询窗口）。
    pollUntilSucceed(
        test,
        () => getTypeId(test, MOSS_A) === "minecraft:air" && getTypeId(test, MOSS_B) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `moss chain break: expected both A=${JSON.stringify(MOSS_A)} and B=${JSON.stringify(MOSS_B)} ` +
                        `to become air after removing top support, ` +
                        `got A=${getTypeId(test, MOSS_A)} B=${getTypeId(test, MOSS_B)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if A is air but B is not, chain propagation (B updatePostPlacement on A-removed->self-destruct) may be missing)`,
                );
            },
        },
    );
}

export function registerPaleHangingMossTests(): void {
    GameTest.register("BlockBehaviorTests", "pale_hanging_moss_survives_with_solid_support_above", paleHangingMossSurvivesWithSolidSupportAbove)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "pale_hanging_moss_chain_survives_with_top_support", paleHangingMossChainSurvivesWithTopSupport)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "pale_hanging_moss_breaks_when_support_above_removed", paleHangingMossBreaksWhenSupportAboveRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "pale_hanging_moss_chain_breaks_when_top_support_removed", paleHangingMossChainBreaksWhenTopSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(140);
}
