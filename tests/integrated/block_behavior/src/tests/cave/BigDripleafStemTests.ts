// 大型垂滴叶茎（big_dripleaf_stem）集成测试：验证茎的转茎构造、失支撑/失叶片自毁、有效支撑存活行为
// （对齐 wiki 大型垂滴叶#用途/#破坏 + 历史 21w17a 茎自毁修复）。
//
// 这是 BigDripleafTests.ts 显式留的 TODO 缺口（BigDripleafTests.ts:65-66「不测『上方叠放转茎』…
// TODO: 可补 big_dripleaf_converts_to_stem_when_stacked」+「stem 延迟销毁…时序复杂，跳过」），本文件补全。
//
// wiki tech_大型垂滴叶.txt：
//   #用途（:52）："在空间充足的情况下，对着已放置的大型垂滴叶放置大型垂滴叶会使其增高一格"——
//     叶片叠放时下方叶片转为茎（big_dripleaf_stem）。
//   #破坏（:54）："破坏大型垂滴叶的任意部分会导致整个植株被破坏，且每一格会掉落一个大型垂滴叶物品。"
//   #历史 21w17a（:369）："现在大型垂滴叶茎在其上方任意部分被破坏时也会被一同破坏。{{Bug|MC-214838}}"
//     ——明确茎在上方叶片被破坏时自毁的行为锚点，1.21.11 已含。
//   #破坏（:49）："大型垂滴叶被破坏后会掉落自身。"——茎自毁掉落大型垂滴叶物品（spawnAfterBreak 处理，
//     本测试仅断言方块消失，不断言物品实体）。
//
// ============================ Cubium 实现链路 ============================
// BigDripleafStemBlock（cave/BigDripleafStemBlock.cpp）：
//   - isValidPosition（:88-111）：下方须是茎 或 BIG_DRIPLEAF_PLACEABLE 标签方块（clay 在标签内），
//     且上方须是茎 或 BIG_DRIPLEAF 叶片。**两端同时满足才存活**。
//   - updatePostPlacement（:113-131）：facing==Down||Up 且 !isValidPosition → scheduleBlockTick(currentPos, 1)
//     （固定 1 tick 延迟，非 randomTick）。
//   - tick（:133-152）：1 tick 后重检 !isValidPosition → spawnAfterBreak + setBlockState(air, 3) 自毁。
//
// BigDripleafBlock（cave/BigDripleafBlock.cpp）转茎逻辑（:131-140）：facing==Up 且上方 facingState.is(this)
//   （上方也是叶片）→ 返回 BIG_DRIPLEAF_STEM state（继承原叶片 facing + waterlogged）。同步，同 tick。
//
// 触发链路全由 updatePostPlacement（同步邻居更新）+ scheduleBlockTick(1)（1 tick 延迟）驱动，无 randomTick、
// 无光照、无玩家交互、无跨维度依赖。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。同 BigDripleafTests.ts 坐标范式。
//
// 布局列 (3,*,1)：clay 支撑 (3,1,1)，叶片/茎 (3,2,1)，上方叶片 (3,3,1)。clay 在 BIG_DRIPLEAF_PLACEABLE
// 标签内（BlockTags.cpp:2953-2957），isValidPosition 下方支撑判定通过。
//
// 测试1 big_dripleaf_converts_to_stem_when_leaf_placed_above（叶片叠放转茎，wiki :52）：
//   clay (3,1,1) + 叶片 (3,2,1) tilt=none。再放叶片 (3,3,1) tilt=none。(3,3,1) 放置派发邻居更新 →
//   (3,2,1) 叶片 updatePostPlacement(Up) 检测上方变叶片 → 返回 stem state（同步转茎）。
//   断言 (3,2,1) 变 big_dripleaf_stem 且 facing=north（茎继承原叶片朝向）。
//
// 测试2 big_dripleaf_stem_breaks_when_leaf_above_removed（上方叶片移除茎自毁，wiki :54/:369）：
//   clay (3,1,1) + 叶片 (3,2,1) + 叶片 (3,3,1) → (3,2,1) 转茎（同测试1）。移除上方叶片 (3,3,1)→air。
//   茎 (3,2,1) updatePostPlacement(Up) 检测上方 air → isValidPosition 失败（上方非茎/叶）→
//   scheduleBlockTick(1) → 1 tick 后 tick → setBlockState(air) 自毁。
//   断言 (3,2,1) 变 air。
//
// 测试3 big_dripleaf_stem_breaks_when_support_below_removed（下方支撑移除茎自毁，wiki :54 整株破坏）：
//   clay (3,1,1) + 茎 (3,2,1)（setBlockWithStates 直接预置 stem）+ 叶片 (3,3,1)（满足茎 isValidPosition
//   上方须叶/茎）。移除 clay (3,1,1)→air。茎 updatePostPlacement(Down) 检测下方 air 非
//   BIG_DRIPLEAF_PLACEABLE → isValidPosition 失败 → scheduleBlockTick(1) → 自毁。
//   断言 (3,2,1) 变 air。
//
// 测试4 big_dripleaf_stem_survives_with_valid_support_and_leaf_above（有效支撑+上方叶片茎存活，正向防误判）：
//   clay (3,1,1) + 茎 (3,2,1) + 叶片 (3,3,1)。不做任何破坏。茎 isValidPosition 两端满足（下方 clay 标签 +
//   上方叶片）不自毁。断言 (3,2,1) 仍是 big_dripleaf_stem（防 updatePostPlacement 误触发自毁）。
//
// ============================ 排除项（不写测试）============================
// - 茎自毁掉落大型垂滴叶物品（spawnAfterBreak）：掉落物实体断言非确定（物品实体生成位置/拾取时序），
//   wiki :49/:54 掉落行为由 loot/spawnAfterBreak 处理，本测试仅断言方块 typeId 变化，不测物品。TODO: 待
//   物品实体断言体系完善后补。
// - 茎级联自毁（多段茎链）：clay+茎A+茎B+叶片，移除叶片→B自毁→A上方air→A自毁。Cubium setBlockState(air,3)
//   派发邻居更新理论上能级联，但多段时序+边界复杂，本测试用单段茎验证核心自毁，级联留 TODO。
// - 茎含水（waterlogged state）：与叶片含水同范式（BigDripleafTests 场景3已测 waterlog 读写），茎 waterlog
//   逻辑复用 waterloggable::shouldWaterlogAt，不重复测。
// - 茎红石信号无效（wiki :71「对茎部施加红石信号则无效」）：茎无 tilt 状态机，红石对茎无效果是「无行为」，
//   难以正向断言「无效果」，跳过。
// - 骨粉对茎（wiki 21w11a :366）：茎可骨粉，依赖骨粉 useItem 链路，BoneMealTests 已覆盖骨粉范式，跳过。
//
// ============================ 跨服务端对比 ============================
// - big_dripleaf_stem typeId 两端一致（1.21.11 JE/BE 统一）。facing/waterlogged state 名两端一致。
// - 叶片叠放转茎、上方叶片移除茎自毁、下方支撑移除茎自毁、有效支撑存活，均为 1.21.11 JE/BE 一致特性
//   （21w17a 茎自毁修复 MC-214838，1.21.11 已含）。
// - 测试用 setBlockType/setBlockWithStates 放置 clay/叶片/茎/air，setBlockWithStates 预置 state 是 Cubium
//   专有写入（基岩侧用物品放置），但转茎/自毁/存活行为本身两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_大型垂滴叶.txt#用途（:52 叶片叠放增高，下方转茎）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_大型垂滴叶.txt#破坏（:54 破坏任意部分整株破坏；:49 掉落自身）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_大型垂滴叶.txt#历史（:369 21w17a 茎上方部分破坏时一同破坏 MC-214838）
// Ref: BigDripleafStemBlock.cpp:88-111（isValidPosition 下方茎/标签+上方茎/叶两端满足）
// Ref: BigDripleafStemBlock.cpp:113-131（updatePostPlacement Down/Up 失效→scheduleBlockTick 1 延迟）
// Ref: BigDripleafStemBlock.cpp:133-152（tick 重检 isValidPosition 失败→spawnAfterBreak+setBlockState air）
// Ref: BigDripleafBlock.cpp:131-140（facing==Up&&上方叶片→返回 stem state，继承 facing/waterlogged）
// Ref: BigDripleafTests.ts（glass_pit 坐标范式 + setBlockWithStates cast + clay 支撑 + pollUntilSucceed）
// Ref: BlockTags.cpp:2953-2957（BIG_DRIPLEAF_PLACEABLE 标签含 clay/moss_block 等）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。clay 支撑 (3,1,1)，茎/叶片 (3,2,1)，上方叶片 (3,3,1)。
const CLAY = { x: 3, y: 1, z: 1 };
const STEM_POS = { x: 3, y: 2, z: 1 }; // 茎/下方叶片位置
const LEAF_ABOVE = { x: 3, y: 3, z: 1 }; // 上方叶片位置

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露，用 cast 访问，同 BigDripleafTests/BannerTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 读取方块 facing state（字符串：north/south/east/west）。返回 null 表示失败或无 facing 属性。
// Cubium HORIZONTAL_FACING C++ 属性名为 "facing"。
function getFacing(test: Test, pos: { x: number; y: number; z: number }): string | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 放置 clay 支撑 + 叶片（facing=north，tilt=none）。
function placeClayAndLeaf(test: Test, pos: { x: number; y: number; z: number }): void {
    test.setBlockType("minecraft:clay", CLAY);
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", pos, "facing=north,tilt=none");
}

// 叶片叠放转茎——下方叶片上方再放叶片 → 下方叶片转 big_dripleaf_stem（wiki :52 增高）。
// clay (3,1,1) + 叶片 (3,2,1) + 叶片 (3,3,1)。(3,3,1) 放置派发邻居更新 → (3,2,1) updatePostPlacement(Up)
// 检测上方变叶片 → 返回 stem state（同步转茎，继承 facing=north）。
function bigDripleafConvertsToStemWhenLeafPlacedAbove(test: Test): void {
    placeClayAndLeaf(test, STEM_POS);
    // 上方放叶片 (3,3,1) tilt=none。setBlockWithStates flags=3 派发邻居更新 → (3,2,1) 叶片
    // updatePostPlacement(facing=Up, facingState=叶片) → 返回 stem state（BigDripleafBlock.cpp:131-140）。
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", LEAF_ABOVE, "facing=north,tilt=none");

    // 转茎是 updatePostPlacement 同步触发，但留余量防时序（邻居更新派发到 state 写入可能跨 tick）。
    pollUntilSucceed(
        test,
        () => {
            return getTypeId(test, STEM_POS) === "minecraft:big_dripleaf_stem" && getFacing(test, STEM_POS) === "north";
        },
        {
            startTick: 2,
            interval: 2,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `stem convert: expected big_dripleaf_stem[facing=north] at ${JSON.stringify(STEM_POS)}, ` +
                        `got ${getTypeId(test, STEM_POS)}[facing=${getFacing(test, STEM_POS)}] ` +
                        `(above=${getTypeId(test, LEAF_ABOVE)}; ` +
                        `if still big_dripleaf, BigDripleafBlock updatePostPlacement Up->stem may be missing)`,
                );
            },
        },
    );
}

// 上方叶片移除 → 茎自毁（wiki :54/:369 茎随上方部分破坏一同破坏）。
// 先构造转茎（叶片+叶片→下方转茎），再移除上方叶片 → 茎 updatePostPlacement(Up) 失效 → 1 tick 后自毁。
function bigDripleafStemBreaksWhenLeafAboveRemoved(test: Test): void {
    placeClayAndLeaf(test, STEM_POS);
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", LEAF_ABOVE, "facing=north,tilt=none");

    // 阶段1：等转茎完成（同测试1链路）。
    // 阶段2：t=30 移除上方叶片 (3,3,1)→air，茎 updatePostPlacement(Up) 检测上方 air → isValidPosition
    // 失败 → scheduleBlockTick(1) → 1 tick 后 tick → setBlockState(air) 自毁。
    // 用 runAtTickTime 在 t=30 移除（此时转茎早已完成），再 pollUntilSucceed 从 t=35 起断言茎变 air。
    test.runAtTickTime(30, () => {
        // 仅在已是茎时移除上方叶片（转茎成功）；否则保留现场供 onTimeout 诊断。
        if (getTypeId(test, STEM_POS) === "minecraft:big_dripleaf_stem") {
            test.setBlockType("minecraft:air", LEAF_ABOVE); // 移除上方叶片，派发邻居更新触发茎 updatePostPlacement(Up)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, STEM_POS) === "minecraft:air",
        {
            startTick: 35,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `stem break on leaf removed: expected air at ${JSON.stringify(STEM_POS)} after removing above leaf, ` +
                        `got ${getTypeId(test, STEM_POS)} ` +
                        `(above=${getTypeId(test, LEAF_ABOVE)} should be air; ` +
                        `if still stem, BigDripleafStemBlock updatePostPlacement Up-fail->tick->air may be missing)`,
                );
            },
        },
    );
}

// 下方支撑移除 → 茎自毁（wiki :54 整株破坏）。
// clay (3,1,1) + 茎 (3,2,1)（直接预置 stem）+ 叶片 (3,3,1)（满足茎 isValidPosition 上方须叶/茎）。
// 移除 clay → 茎 updatePostPlacement(Down) 检测下方 air 非 BIG_DRIPLEAF_PLACEABLE → isValidPosition 失败 →
// scheduleBlockTick(1) → 1 tick 后自毁。
function bigDripleafStemBreaksWhenSupportBelowRemoved(test: Test): void {
    test.setBlockType("minecraft:clay", CLAY);
    // 直接预置 stem（绕过转茎时序，聚焦下方支撑失效自毁）。facing=north，上方须叶片满足 isValidPosition。
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf_stem", STEM_POS, "facing=north");
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", LEAF_ABOVE, "facing=north,tilt=none");

    // t=20 移除 clay 支撑（→air），茎 updatePostPlacement(Down) 检测下方 air → isValidPosition 失败 →
    // scheduleBlockTick(1) → 1 tick 后 tick → setBlockState(air) 自毁。
    test.runAtTickTime(20, () => {
        // 仅在茎仍存在时移除支撑（防预置失败误操作）。
        if (getTypeId(test, STEM_POS) === "minecraft:big_dripleaf_stem") {
            test.setBlockType("minecraft:air", CLAY); // 移除 clay，派发邻居更新触发茎 updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, STEM_POS) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `stem break on support removed: expected air at ${JSON.stringify(STEM_POS)} after removing clay, ` +
                        `got ${getTypeId(test, STEM_POS)} ` +
                        `(clay=${getTypeId(test, CLAY)} should be air; ` +
                        `if still stem, BigDripleafStemBlock updatePostPlacement Down-fail->tick->air may be missing)`,
                );
            },
        },
    );
}

// 有效支撑+上方叶片 → 茎存活不自毁（正向防误判，验证 isValidPosition 两端满足时不触发自毁）。
// clay (3,1,1) + 茎 (3,2,1) + 叶片 (3,3,1)。不做任何破坏，等待后断言茎仍存在。
function bigDripleafStemSurvivesWithValidSupportAndLeafAbove(test: Test): void {
    test.setBlockType("minecraft:clay", CLAY);
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf_stem", STEM_POS, "facing=north");
    (test as TestWithStates).setBlockWithStates("minecraft:big_dripleaf", LEAF_ABOVE, "facing=north,tilt=none");

    // 等待足够 tick（超过自毁的 1 tick 延迟窗口），断言茎仍存在（isValidPosition 两端满足，不自毁）。
    // 若 updatePostPlacement 误触发自毁，茎会在 1 tick 内消失，此测试会捕获。
    pollUntilSucceed(
        test,
        () => getTypeId(test, STEM_POS) === "minecraft:big_dripleaf_stem",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `stem survive: expected big_dripleaf_stem to remain at ${JSON.stringify(STEM_POS)} with valid support, ` +
                        `got ${getTypeId(test, STEM_POS)} ` +
                        `(clay=${getTypeId(test, CLAY)} above=${getTypeId(test, LEAF_ABOVE)}; ` +
                        `if air, isValidPosition may falsely fail or updatePostPlacement may over-trigger self-destruct)`,
                );
            },
        },
    );
}

export function registerBigDripleafStemTests(): void {
    GameTest.register("BlockBehaviorTests", "big_dripleaf_converts_to_stem_when_leaf_placed_above", bigDripleafConvertsToStemWhenLeafPlacedAbove)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "big_dripleaf_stem_breaks_when_leaf_above_removed", bigDripleafStemBreaksWhenLeafAboveRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "big_dripleaf_stem_breaks_when_support_below_removed", bigDripleafStemBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "big_dripleaf_stem_survives_with_valid_support_and_leaf_above", bigDripleafStemSurvivesWithValidSupportAndLeafAbove)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
