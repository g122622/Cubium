// 气泡柱集成测试：验证 BubbleColumnBlock 由灵魂沙/岩浆块触发生成、向上传播、失支撑变水的行为
// （对齐 wiki 气泡柱章节「人工制造」「涡流」「涌流」）。
//
// wiki tech_气泡柱.txt：
//   #人工制造（:30-35）："当在水源下放置灵魂沙时，会产生涌流气泡柱。"（:31）
//     "当在水源下放置岩浆块时，会产生涡流气泡柱。"（:33）
//     "气泡柱在向上延伸的过程中会在抵达非水源方块后停止延伸。"（:35）
//   #涡流（:56-61）："将岩浆块放置在水源下方时，会产生垂直下降的气泡，这就是涡流。
//     涡流会将实体向下推动。"（:58-59）
//   #涌流（:63-67）："将灵魂沙放置在水源下方时，会产生垂直上升的气泡，这就是涌流。
//     涌流会将实体向上推动。"（:64-65）
//
// ============================ Cubium 实现链路 ============================
// 气泡柱（BubbleColumnBlock，ocean/BubbleColumnBlock.cpp）本身不主动生成，而是由下方灵魂沙/岩浆块
// 触发：
// - 岩浆块（MagmaBlock，nether/MagmaBlock.cpp）：onBlockAdded（:47-52）调度 20 tick 后 tick；
//   neighborChanged（:54-72）上方变水时调度 20 tick；tick（:74-84）调 placeBubbleColumn(above, true)
//   生成涡流气泡柱（DRAG=true，下拖）。
// - 灵魂沙（SoulSandBlock，nether/SoulSandBlock.cpp，本次新增）：镜像 MagmaBlock，但 tick 调
//   placeBubbleColumn(above, false) 生成涌流气泡柱（DRAG=false，上推）。
//   此前灵魂沙注册为 SimpleBlock（NetherBlocks.cpp）无任何气泡柱触发逻辑——这是 Cubium 实现缺陷
//   （vanilla SoulSandBlock 有 onPlace/neighborChanged 触发 placeBubbleColumn），本次修复为 SoulSandBlock。
//
// BubbleColumnBlock::placeBubbleColumn（:77-84）：canHoldBubbleColumn（:86-107，要求当前格是水源）
// 为真时 setBlockState 为 bubble_column + DRAG。
// BubbleColumnBlock::onBlockAdded（:136-142）：气泡柱被添加时 getDrag(pos.down()) 决定 DRAG，并在
// 上方 placeBubbleColumn 传播（向上延伸，遇非水源停止，因 canHoldBubbleColumn 返 false）。
// BubbleColumnBlock::updatePostPlacement（:185-219）：isValidPosition 失败（下方非 magma/soul_sand/
// bubble_column，或当前非水/气泡柱）→ 变水；上方变水 → 调度 5 tick 后传播。
// BubbleColumnBlock::tick（:242-251）：5 tick 后在上方 placeBubbleColumn 继承 DRAG。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。
//
// 布局（水源放 y=2，触发源放 y=1）：
//   - 触发源（灵魂沙/岩浆块）(3,1,3)：内部 air 层第一层，下方 y=0 结构底支撑。
//   - 水源 (3,2,3)：触发源正上方，内部 air 层第二层。setBlockType("minecraft:water") 放水方块默认
//     状态（LiquidBlock，流体为源 level=8，满足 canHoldBubbleColumn 的 isSource 要求）。
//   - 水源水平四周 (2,2,3)(4,2,3)(3,2,2)(3,2,4) 预放 stone 防止水源流动蔓延（玻璃墙空腔内若已是
//     玻璃则 setBlockType no-op，若是 air 则补 stone 围挡，确保水源静止不流失）。
//
// 测试1 soul_sand_water_creates_upward_bubble_column（灵魂沙+水源→涌流气泡柱 DRAG=false）：
//   先放水源 (3,2,3) + 四周围挡，再放灵魂沙 (3,1,3)。灵魂沙 onBlockAdded 调度 20 tick 后 tick 生成
//   涌流气泡柱。轮询断言 (3,2,3) 变 bubble_column 且 drag=false。
//
// 测试2 magma_water_creates_downward_bubble_column（岩浆块+水源→涡流气泡柱 DRAG=true）：
//   先放水源 (3,2,3) + 四周围挡，再放岩浆块 (3,1,3)。岩浆块 onBlockAdded 调度 20 tick 后 tick 生成
//   涡流气泡柱。轮询断言 (3,2,3) 变 bubble_column 且 drag=true。
//
// 测试3 upward_bubble_column_propagates_upward（涌流气泡柱向上传播）：
//   灵魂沙 (3,1,3) + 水源柱 (3,2,3)(3,3,3)（两格水源）。灵魂沙生成 (3,2,3) 气泡柱后，气泡柱
//   onBlockAdded 在上方 (3,3,3) placeBubbleColumn 传播（继承 DRAG=false）。轮询断言 (3,3,3) 也变
//   bubble_column 且 drag=false。
//
// 测试4 bubble_column_without_source_becomes_water（气泡柱失去下方支撑→变水）：
//   灵魂沙 (3,1,3) + 水源 (3,2,3) 生成气泡柱后，移除灵魂沙（变 air）。气泡柱 updatePostPlacement(Down)
//   检测下方变 air，isValidPosition 失败（下方非 magma/soul_sand/bubble_column）→ 变水。轮询断言
//   (3,2,3) 变 water。
//
// ============================ 排除项（不写测试）============================
// - 实体推动（涌流上推/涡流下拖）：onEntityCollision（:221-240）需实体进入气泡柱，依赖物理模拟，
//   速度断言非确定，跳过（wiki :60/:66 平衡速度数值两端不一致，JE 4.9/11、BE 6/8）。
// - 船在涡流中摇晃沉没（3秒）：wiki :61，依赖船实体物理，跳过。
// - 活塞推动气泡柱消失：wiki :50，依赖活塞推动方块，活塞测试体系另立，跳过。
// - 呼吸系统（空气中呼吸生物可在气泡柱呼吸）：wiki :52-54，依赖生物窒息计时器，跳过。
// - 自然生成（废弃传送门/海底废墟岩浆块）：wiki :26-28，世界生成层面，跳过。
// - 涡流影响弹射物（1.16.2 修复）：wiki :225，弹射物物理，跳过。
// - 华丽气泡外观/粒子/音效：纯视觉，跳过。
//
// ============================ 跨服务端对比 ============================
// - 灵魂沙+水源→涌流气泡柱、岩浆块+水源→涡流气泡柱、向上传播、失支撑变水，均为 1.21.11 JE/BE 一致
//   特性（气泡柱 1.13 加入，1.21.11 已含）。bubble_column typeId 与 drag state 名两端一致。
// - 测试用 setBlockType 放水源/灵魂沙/岩浆块/stone，均为两端通用 API，非 one-sided。
//   注：基岩 BDS 灵魂沙气泡柱触发由基岩原生逻辑保证（灵魂沙方块行为），Cubium 修复后两端均可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_气泡柱.txt#人工制造（:31灵魂沙涌流/:33岩浆块涡流/:35向上延伸至非水源停）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_气泡柱.txt#涡流（:58岩浆块水源下方→涡流下拖）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_气泡柱.txt#涌流（:64灵魂沙水源下方→涌流上推）
// Ref: BubbleColumnBlock.cpp:77-84（placeBubbleColumn canHoldBubbleColumn 守卫水源后 setBlockState）
// Ref: BubbleColumnBlock.cpp:109-129（getDrag 灵魂沙→false/岩浆块→true/气泡柱→继承）
// Ref: BubbleColumnBlock.cpp:136-142（onBlockAdded 上方传播）/185-219（updatePostPlacement 失支撑变水）/242-251（tick 5tick 传播）
// Ref: MagmaBlock.cpp:47-84（onBlockAdded/neighborChanged 调度20tick，tick placeBubbleColumn above DRAG=true）
// Ref: SoulSandBlock.cpp（镜像 MagmaBlock，tick placeBubbleColumn above DRAG=false，本次新增修复 SimpleBlock 缺陷）
// Ref: WaterLavaInteractionTests.ts（glass_pit 布局 + 水源放置范式 + pollUntilSucceed）
// Ref: LiquidTests.ts（setBlockType water 放水源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。触发源（灵魂沙/岩浆块）放 y=1，水源放 y=2。
const SOURCE_BLOCK = { x: 3, y: 1, z: 3 }; // 灵魂沙/岩浆块位置
const WATER_POS = { x: 3, y: 2, z: 3 }; // 水源/气泡柱位置
const ABOVE_POS = { x: 3, y: 3, z: 3 }; // 水源上方（传播测试用）
// 水源水平四周（防流动蔓延围挡）。glass_pit 玻璃墙内若已是玻璃则 setBlockType no-op。
const WATER_SURROUNDINGS = [
    { x: 2, y: 2, z: 3 },
    { x: 4, y: 2, z: 3 },
    { x: 3, y: 2, z: 2 },
    { x: 3, y: 2, z: 4 },
];

// 读取 (x,y,z) 方块 typeId。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 读取 (x,y,z) bubble_column 的 drag state（boolean）。返回 null 表示非气泡柱或读取失败。
// Cubium DRAG state C++ 属性名为 "drag"（BooleanProperty::create("drag")），getState 返 boolean。
function getDrag(test: Test, pos: { x: number; y: number; z: number }): boolean | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("drag" as any);
    return typeof value === "boolean" ? value : null;
}

// 在水源水平四周放 stone 围挡，防止水源流动蔓延（glass_pit 玻璃墙内若已是玻璃则 no-op）。
function sealWaterSurroundings(test: Test): void {
    for (const pos of WATER_SURROUNDINGS) {
        test.setBlockType("minecraft:stone", pos);
    }
}

// 灵魂沙放水源下方 → 生成涌流气泡柱（DRAG=false，向上推动）。
// wiki 人工制造/涌流：水源下放灵魂沙产生涌流气泡柱。
// 先放水源 (3,2,3) + 围挡，再放灵魂沙 (3,1,3)。灵魂沙 onBlockAdded 调度 20 tick 后 tick 生成气泡柱。
function soulSandWaterCreatesUpwardBubbleColumn(test: Test): void {
    test.setBlockType("minecraft:water", WATER_POS);
    sealWaterSurroundings(test);
    test.setBlockType("minecraft:soul_sand", SOURCE_BLOCK);

    pollUntilSucceed(
        test,
        () => {
            return getTypeId(test, WATER_POS) === "minecraft:bubble_column" && getDrag(test, WATER_POS) === false;
        },
        {
            startTick: 30,
            interval: 10,
            maxTick: 120,
            onTimeout: () => {
                const type = getTypeId(test, WATER_POS);
                const drag = getDrag(test, WATER_POS);
                test.assert(
                    false,
                    `soul_sand water bubble: expected bubble_column[drag=false] at ${JSON.stringify(WATER_POS)}, ` +
                        `got ${type}[drag=${drag}] ` +
                        `(soul_sand=${getTypeId(test, SOURCE_BLOCK)}; ` +
                        `if still water, SoulSandBlock tick/placeBubbleColumn may not trigger)`,
                );
            },
        },
    );
}

// 岩浆块放水源下方 → 生成涡流气泡柱（DRAG=true，向下推动）。
// wiki 人工制造/涡流：水源下放岩浆块产生涡流气泡柱。
// 先放水源 (3,2,3) + 围挡，再放岩浆块 (3,1,3)。岩浆块 onBlockAdded 调度 20 tick 后 tick 生成气泡柱。
// 注：岩浆块 typeId 为 minecraft:magma_block（vanilla 扁平化命名，非 magma）。
function magmaWaterCreatesDownwardBubbleColumn(test: Test): void {
    test.setBlockType("minecraft:water", WATER_POS);
    sealWaterSurroundings(test);
    test.setBlockType("minecraft:magma_block", SOURCE_BLOCK);

    pollUntilSucceed(
        test,
        () => {
            return getTypeId(test, WATER_POS) === "minecraft:bubble_column" && getDrag(test, WATER_POS) === true;
        },
        {
            startTick: 30,
            interval: 10,
            maxTick: 120,
            onTimeout: () => {
                const type = getTypeId(test, WATER_POS);
                const drag = getDrag(test, WATER_POS);
                test.assert(
                    false,
                    `magma water bubble: expected bubble_column[drag=true] at ${JSON.stringify(WATER_POS)}, ` +
                        `got ${type}[drag=${drag}] ` +
                        `(magma=${getTypeId(test, SOURCE_BLOCK)}; ` +
                        `if still water, MagmaBlock tick/placeBubbleColumn may not trigger)`,
                );
            },
        },
    );
}

// 涌流气泡柱向上传播：灵魂沙生成 (3,2,3) 气泡柱后，气泡柱 onBlockAdded 在上方 (3,3,3) 传播。
// wiki 人工制造：气泡柱向上延伸，遇非水源停止。
// 灵魂沙 (3,1,3) + 水源柱 (3,2,3)(3,3,3)。灵魂沙生成 (3,2,3) 气泡柱 → onBlockAdded 上方 (3,3,3) 传播。
function upwardBubbleColumnPropagatesUpward(test: Test): void {
    // 两格水源柱：下方 (3,2,3) 紧贴灵魂沙，上方 (3,3,3) 待传播。
    test.setBlockType("minecraft:water", WATER_POS);
    test.setBlockType("minecraft:water", ABOVE_POS);
    // 围挡下方水源四周（上方水源 (3,3,3) 四周在 y=3 玻璃墙空腔外，结构已是 air/玻璃，水会向下流但
    // 下方是水源柱不触发额外流动；保守起见上方水源四周也围挡）。
    sealWaterSurroundings(test);
    for (const pos of [
        { x: 2, y: 3, z: 3 },
        { x: 4, y: 3, z: 3 },
        { x: 3, y: 3, z: 2 },
        { x: 3, y: 3, z: 4 },
    ]) {
        test.setBlockType("minecraft:stone", pos);
    }
    test.setBlockType("minecraft:soul_sand", SOURCE_BLOCK);

    pollUntilSucceed(
        test,
        () => {
            // 上方 (3,3,3) 也应变 bubble_column[drag=false]（继承下方涌流 DRAG）。
            return getTypeId(test, ABOVE_POS) === "minecraft:bubble_column" && getDrag(test, ABOVE_POS) === false;
        },
        {
            startTick: 30,
            interval: 10,
            maxTick: 140,
            onTimeout: () => {
                const below = getTypeId(test, WATER_POS);
                const above = getTypeId(test, ABOVE_POS);
                const aboveDrag = getDrag(test, ABOVE_POS);
                test.assert(
                    false,
                    `bubble propagate: expected bubble_column[drag=false] at ${JSON.stringify(ABOVE_POS)}, ` +
                        `got ${above}[drag=${aboveDrag}] ` +
                        `(below ${JSON.stringify(WATER_POS)}=${below}; ` +
                        `if below is bubble but above is water, onBlockAdded upward propagation may be missing)`,
                );
            },
        },
    );
}

// 气泡柱失去下方支撑 → 变水（updatePostPlacement isValidPosition 失败）。
// 灵魂沙 (3,1,3) + 水源 (3,2,3) 生成气泡柱后，移除灵魂沙 → 气泡柱下方变 air → isValidPosition 失败 → 变水。
// 时序：灵魂沙 onBlockAdded 在放置时（t≈0）调度 20 tick 后生成气泡柱（t≈20）。t=130 时气泡柱已稳定
// 生成，此时移除灵魂沙（变 air）。气泡柱 updatePostPlacement(Down) 检测下方变 air → isValidPosition
// 失败 → 变水。从 t=140 起轮询断言 (3,2,3) 变 water。
function bubbleColumnWithoutSourceBecomesWater(test: Test): void {
    test.setBlockType("minecraft:water", WATER_POS);
    sealWaterSurroundings(test);
    test.setBlockType("minecraft:soul_sand", SOURCE_BLOCK);

    // t=130 移除灵魂沙（此时气泡柱早已在 t≈20 生成）。仅当已是气泡柱时移除，否则保留现场供 onTimeout 诊断。
    test.runAtTickTime(130, () => {
        if (getTypeId(test, WATER_POS) === "minecraft:bubble_column") {
            test.setBlockType("minecraft:air", SOURCE_BLOCK); // 移除灵魂沙，触发气泡柱 updatePostPlacement(Down)
        }
    });

    // 从 t=140 起轮询断言气泡柱变水。
    pollUntilSucceed(
        test,
        () => {
            // 移除灵魂沙后，气泡柱 updatePostPlacement(Down) 检测下方 air → isValidPosition 失败 → 变水。
            return getTypeId(test, WATER_POS) === "minecraft:water";
        },
        {
            startTick: 140,
            interval: 10,
            maxTick: 240,
            onTimeout: () => {
                const type = getTypeId(test, WATER_POS);
                const source = getTypeId(test, SOURCE_BLOCK);
                test.assert(
                    false,
                    `bubble without source: expected water at ${JSON.stringify(WATER_POS)} after removing soul_sand, ` +
                        `got ${type} ` +
                        `(source_block=${source} should be air; ` +
                        `if still bubble_column, updatePostPlacement isValidPosition-fail-to-water may be missing)`,
                );
            },
        },
    );
}

export function registerBubbleColumnTests(): void {
    GameTest.register("BlockBehaviorTests", "soul_sand_water_creates_upward_bubble_column", soulSandWaterCreatesUpwardBubbleColumn)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
    GameTest.register("BlockBehaviorTests", "magma_water_creates_downward_bubble_column", magmaWaterCreatesDownwardBubbleColumn)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
    GameTest.register("BlockBehaviorTests", "upward_bubble_column_propagates_upward", upwardBubbleColumnPropagatesUpward)
        .structureName("gametests:glass_pit")
        .maxTicks(220);
    GameTest.register("BlockBehaviorTests", "bubble_column_without_source_becomes_water", bubbleColumnWithoutSourceBecomesWater)
        .structureName("gametests:glass_pit")
        .maxTicks(260);
}
