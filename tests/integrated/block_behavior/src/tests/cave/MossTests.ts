// 苔藓块骨粉传播行为 GameTest（骨粉将相邻可替换方块转变为苔藓）。
//
// wiki other_苔藓块.txt#骨粉（:45）：上方一格为空气的苔藓块被使用骨粉后，以苔藓块为中心，
// 水平切比雪夫半径 2-3 格、垂直切比雪夫半径 5 格范围内，最上方且上方一格为空气的
// 草方块/泥土/砂土/灰化土/缠根泥土/菌丝体/苍白苔藓块/石头/深板岩/凝灰岩/花岗岩/闪长岩/安山岩/
// (JE:泥巴/沾泥的红树根) 和洞穴藤蔓 有概率转变为苔藓块。
//
// C++ 链路：MossBlock::grow（MossBlock.cpp:57-95）骨粉后扫描 pos+(-1,-1,-1) 到 pos+(1,5,1)
// 的 3x7x3 范围，将 MOSS_REPLACEABLE 标签内且上方为 air 的方块替换为 moss_block（flags=3 同步）。
// canUseBonemeal 恒 true（100% 即时生效）。_placeMossVegetation 概率性在上方放植被（不测）。
// MOSS_REPLACEABLE 标签 = base_stone_overworld + cave_vines + dirt；stone ∈ base_stone_overworld。
//
// 已知 Cubium 偏差（本测试不覆盖范围/概率差异，仅测两端一致的核心行为点）：
//   - Cubium grow 范围固定 3x7x3（水平 ±1，垂直 -1..+5）且确定性全替换；vanilla 范围 5x5~7x7
//     （水平切比雪夫半径 2-3）概率性替换（边缘 75%、角落 0%、内部 100%）。范围与概率两端不一致。
//   - 本测试只断言"距离 1 的内部位置可替换方块（上方 air）被传播替换为苔藓"——此行为点两端
//     均 100% 成立（距离 1 永远是 vanilla 内部位置 100% 选中 + 向下搜寻命中），不涉及范围/概率差异。
//   TODO: 待 Cubium grow 范围/概率对齐 vanilla 后，补充"边缘/角落概率替换""远距离传播"等测试。
//
// 测试布局：(3,1,1) 放 moss_block（中心，被骨粉，上方 (3,2,1) air 满足"上方一格为空气"），
// (4,1,1) 放 stone（East 邻位，距离 1 内部位置，stone ∈ base_stone_overworld ∈ MOSS_REPLACEABLE，
// 上方 (4,2,1) air 满足"上方一格为空气"）。SimulatedPlayer 对 (3,1,1) 苔藓块用骨粉 → grow →
// (4,1,1) stone 被替换为 moss_block。
//
// 判定：pollUntilSucceed 轮询 (4,1,1) === minecraft:moss_block（grow 同步 setBlockState，留余量）。
//
// 跨服务端：核心行为点（相邻可替换方块被传播替换）两端一致，可跨服务端对比。stone ∈ MOSS_REPLACEABLE
// 两端标签一致（base_stone_overworld 含 stone）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_苔藓块.txt#骨粉（骨粉传播范围与可替换方块）
// Ref: MossBlock.cpp（grow 3x7x3 扫描替换 MOSS_REPLACEABLE）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 苔藓块骨粉传播：相邻石头（MOSS_REPLACEABLE，上方 air）被替换为苔藓块。
//
// 布局：(3,1,1) moss_block（中心，上方 air），(4,1,1) stone（East 邻位，上方 air）。
// SimulatedPlayer 对 (3,1,1) 苔藓块用骨粉 → MossBlock::grow 3x7x3 扫描 → (4,1,1) stone ∈
// MOSS_REPLACEABLE 且上方 air → 替换为 moss_block。
function mossBonemealSpreadsToAdjacentStone(test: Test): void {
    const mossPos = { x: 3, y: 1, z: 1 };
    const stonePos = { x: 4, y: 1, z: 1 };

    // (3,1,1) 放 moss_block（被骨粉的中心，上方 (3,2,1) air 满足骨粉前置条件）。
    test.setBlockType("minecraft:moss_block", mossPos);

    // (4,1,1) 放 stone（East 邻位，距离 1。stone ∈ base_stone_overworld ∈ MOSS_REPLACEABLE，
    // 上方 (4,2,1) air 满足 grow 的"上方为空气"条件）。
    test.setBlockType("minecraft:stone", stonePos);

    // SimulatedPlayer 持骨粉对苔藓块使用（direction=Up，从上方使用，同 CropBoneMealTests 范式）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        mossPos,
        Direction.Up,
    );
    test.assert(used, "moss bonemeal: useItemOnBlock should return true");

    // 判定：(4,1,1) 被替换为 moss_block。grow 同步 setBlockState，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => {
            const block = test.getBlock(stonePos);
            return block !== undefined && block.typeId === "minecraft:moss_block";
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                const block = test.getBlock(stonePos);
                test.assert(
                    false,
                    `moss bonemeal: (4,1,1) should be minecraft:moss_block, got ${block?.typeId}`,
                );
            },
        },
    );
}

export function registerMossTests(): void {
    GameTest.register("BlockBehaviorTests", "moss_bonemeal_spreads_to_adjacent_stone", mossBonemealSpreadsToAdjacentStone)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
