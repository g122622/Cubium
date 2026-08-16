// 楼梯角形状重算行为 GameTest（毗邻楼梯形成内角/外角形状）。
//
// wiki tech_楼梯.txt（:167-171）："楼梯会更改它们的形状以与毗邻的楼梯（包括不同类型的）连接：
// 当楼梯的半格面与另一个楼梯的面毗邻时，楼梯的整格面会变成 L 形，以连接另一个楼梯的半格面
// （这形成了一个内角）。当楼梯的整格面与另一个楼梯的面毗邻时，楼梯的半格面会变成 L 形，以连接
// 另一个楼梯的整格面（这形成了一个外角）。" shape state: straight/inner_left/inner_right/
// outer_left/outer_right。
//
// C++ 链路：StairsBlock::updatePostPlacement（StairsBlock.cpp:236-264）仅在水平方向邻居变化时调
// _calculateShape（:362-396）重算 shape。_calculateShape：
//   1. 前方（facing 方向）邻居若是同 half、垂直朝向楼梯，且第三位置（opposite(neighborFacing)）
//      是"不同楼梯"（_isDifferentStairs，:427-443，非同 facing 同 half）→ OuterLeft/OuterRight。
//   2. 后方（opposite(facing) 方向）邻居若是同 half、垂直朝向楼梯，且第三位置（neighborFacing）
//      是"不同楼梯" → InnerLeft/InnerRight。
//   3. 否则 Straight。
// _neighborIsStairs（:398-425）：邻居须是楼梯、同 half、facing 与当前楼梯在不同轴（垂直）。
// 邻居方块变化（setBlockType/setBlockWithStates flags=3）同 tick 同步派发 neighborChanged →
// updatePostPlacement → _calculateShape 返回新 shape，ServerWorld 同步写入。纯同步，无 tick 调度。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState（facing=North, half=bottom, shape=straight），
// 不经 getStateForPlacement。故单放一个楼梯 shape=straight；需在邻位放第二个垂直朝向楼梯触发第一个
// 楼梯的 updatePostPlacement 重算角形状。
//
// 测试覆盖（2 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   1. 内角：A facing=East + 后方（West 邻位）B facing=North（垂直）→ A shape=inner_right。
//   2. 外角：A facing=East + 前方（East 邻位）B facing=North（垂直）→ A shape=outer_left。
//
// 内角场景推演（A=(3,1,1) facing=East, half=bottom）：
//   - 后方邻居 = opposite(East)=West 邻位 = (2,1,1) 放 B facing=North（North 是 Z 轴，East 是 X 轴，
//     不同轴 ✓，同 half=bottom ✓）。
//   - 第三位置 = backwardFacing=North = (3,1,0)... 但 (3,1,0) 可能在 glass_pit 玻璃墙边界。
//   为避免边界，改用 A facing=North + 后方 South 邻位 B facing=East：
//   - A=(3,1,1) facing=North（setBlockType 默认即 North），half=bottom。
//   - 后方邻居 = opposite(North)=South 邻位 = (3,1,2) 放 B facing=East（East 是 X 轴，North 是 Z 轴，
//     垂直 ✓，同 half ✓）。
//   - 第三位置 = backwardFacing=East = (3,1,1)+East=(4,1,1)。若 (4,1,1) air → _isDifferentStairs
//     返回 true（非楼梯）→ 形成内角。
//   - backwardFacing=East, facing=North. East == rotateYCCW(North)=West? 否 → InnerRight。
//
// 外角场景推演（A=(3,1,1) facing=East, half=bottom）：
//   - A 用 setBlockWithStates "facing=east" 放置（默认 North，需显式设 East）。
//   - 前方邻居 = East 邻位 = (4,1,1) 放 B facing=North（setBlockType 默认 North，垂直 ✓，同 half ✓）。
//   - 第三位置 = opposite(forwardFacing)=opposite(North)=South = (3,1,1)+South=(3,1,2)。若 (3,1,2) air
//     → _isDifferentStairs true → 形成外角。
//   - forwardFacing=North, facing=East. North == rotateYCCW(East)=North? 是 → OuterLeft。
//
// 关键约束：
// 1. 先放 A 再放 B：B 放置（flags=3）向 A 派发 neighborChanged → A updatePostPlacement 重算 shape。
//    放置不向自身派发 updatePostPlacement，A 放置时保持 straight，待 B 放置触发重算。
// 2. 第三位置须为 air（或非同朝向楼梯）以使 _isDifferentStairs 返回 true 形成角形状。glass_pit 内部
//    air 空腔满足（(4,1,1)/(3,1,2) 默认 air）。
//
// 不测倒置楼梯（half=top）角形状：与 bottom 对称，行为点相同，按「单一职责」本文件聚焦 bottom
// 半格的 Inner/Outer 判定。TODO: 可补 stairs_top_half_corner 测试覆盖 half=top 分支。
// 不测「正面朝上与倒置楼梯不互连」（wiki :171）：需混合 half，且 _neighborIsStairs 同 half 检查
// 已隐含覆盖，跳过。
//
// 跨服务端：facing/half/shape state 名两端一致（Java 式），角形状重算规则两端一致（前方 Outer/
// 后方 Inner + 垂直朝向 + 第三位置判定），可跨服务端对比。getState("shape") 用 as any 绕过
// BlockStateSuperset 白名单（同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_楼梯.txt（楼梯与毗邻楼梯形成内角/外角）
// Ref: StairsBlock.cpp（_calculateShape/updatePostPlacement/_neighborIsStairs/_isDifferentStairs）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (3,1,1) 为主楼梯 A，邻位 (3,1,2) South / (4,1,1) East 作邻居楼梯 B，均在 air 空腔内。

// 取楼梯 shape state（"straight"/"inner_left"/"inner_right"/"outer_left"/"outer_right"）。返回 null
// 表示读取失败。
function getStairsShape(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("shape" as any);
    return typeof value === "string" ? value : null;
}

// 内角：A facing=North + 后方 South 邻位 B facing=East → A shape=inner_right。
//
// 布局：(3,1,1) 放 A（oak_stairs 默认 facing=North half=bottom shape=straight），(3,1,2) 放 B
// （setBlockWithStates "facing=east" half=bottom）。B 放置向 North 邻位 A 派发 updatePostPlacement(South)
// → A 后方邻居 B facing=East 垂直 + 第三位置 East (4,1,1) air 非楼梯 → InnerRight。
//
// 判定：pollUntilSucceed 轮询 A (3,1,1) shape === "inner_right"。
function stairsFormsInnerCorner(test: Test): void {
    // (3,1,1) 放主楼梯 A（oak_stairs 默认 facing=North half=bottom shape=straight）。
    test.setBlockType("minecraft:oak_stairs", { x: 3, y: 1, z: 1 });

    // (3,1,2) 放邻居楼梯 B（setBlockWithStates "facing=east"，East 是 X 轴，与 A 的 North Z 轴垂直，
    // 同 half=bottom）。B 放置向 North 邻位 A 派发 updatePostPlacement(South) → A 重算 shape。
    test.setBlockWithStates("minecraft:oak_stairs", { x: 3, y: 1, z: 2 }, "facing=east");

    // 轮询断言 A (3,1,1) shape === "inner_right"。updatePostPlacement 同步，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => getStairsShape(test, 3, 1, 1) === "inner_right",
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `stairs inner: A shape should be inner_right, got ${getStairsShape(test, 3, 1, 1)}`,
                );
            },
        },
    );
}

// 外角：A facing=East + 前方 East 邻位 B facing=North → A shape=outer_left。
//
// 布局：(3,1,1) 用 setBlockWithStates "facing=east" 放 A（half=bottom），(4,1,1) 放 B（oak_stairs 默认
// facing=North half=bottom）。B 放置向 West 邻位 A 派发 updatePostPlacement(East) → A 前方邻居 B
// facing=North 垂直 + 第三位置 opposite(North)=South (3,1,2) air 非楼梯 → OuterLeft。
//
// 判定：pollUntilSucceed 轮询 A (3,1,1) shape === "outer_left"。
function stairsFormsOuterCorner(test: Test): void {
    // (3,1,1) 用 setBlockWithStates 放主楼梯 A（facing=east half=bottom shape=straight）。默认 facing=
    // North，需显式设 facing=east 使前方为 East 邻位。
    test.setBlockWithStates("minecraft:oak_stairs", { x: 3, y: 1, z: 1 }, "facing=east");

    // (4,1,1) 放邻居楼梯 B（oak_stairs 默认 facing=North half=bottom，North 是 Z 轴，与 A 的 East X 轴
    // 垂直，同 half=bottom）。B 放置向 West 邻位 A 派发 updatePostPlacement(East) → A 重算 shape。
    test.setBlockType("minecraft:oak_stairs", { x: 4, y: 1, z: 1 });

    // 轮询断言 A (3,1,1) shape === "outer_left"。
    pollUntilSucceed(
        test,
        () => getStairsShape(test, 3, 1, 1) === "outer_left",
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `stairs outer: A shape should be outer_left, got ${getStairsShape(test, 3, 1, 1)}`,
                );
            },
        },
    );
}

export function registerStairsTests(): void {
    GameTest.register("BlockBehaviorTests", "stairs_forms_inner_corner", stairsFormsInnerCorner)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "stairs_forms_outer_corner", stairsFormsOuterCorner)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
