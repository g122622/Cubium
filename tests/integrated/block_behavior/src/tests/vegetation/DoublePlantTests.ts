// 双高植物上半自毁行为 GameTest（移除下半方块时上半同步自毁）。
//
// 双高植物（向日葵/高草丛/大型蕨等）占两格，下半（lower）+上半（upper，half state）。wiki 高草丛
// :15 "两格高的非固体植物方块"，:53 "被挖掘时会立刻被破坏"。vanilla DoublePlantBlock.updateShape：
// 当另一半消失时当前半自毁（变 air）。Cubium DoublePlantBlock::updatePostPlacement 对齐此行为。
//
// C++ 链路：DoublePlantBlock::updatePostPlacement（DoublePlantBlock.cpp:134-173）仅处理 Y 轴邻居变化
// （:146 Directions::getAxis(facing)==Y）。当 isLower == isUpDirection（即下半收到 Up 变化、上半收到
// Down 变化——连接另一半的方向）时，若 facingState 不再是同类型另一半（is(this) && half!=当前半），
// 则返回 air 自毁（:157-159）。故移除下半后，上半（收到 Down 变化，facingState=air 非 this）自毁；
// 移除上半后，下半（收到 Up 变化，facingState=air 非 this）自毁。反应同 tick 同步。
//
// 本测试覆盖「移除下半 → 上半自毁」（:151 上半 isLower(false)==isUpDirection(false) 命中，facingState
// =air 非 this → 自毁）。另测「移除上半 → 下半自毁」对称分支。
//
// 关键约束（同 SnowTests）：
// 1. setBlockType 走 _resolveBlock 取 defaultState（half=Lower），不经 isValidPosition，强放下半即使
//    下方非草也能存活（放置不向自身派发 updatePostPlacement）。但为贴近 vanilla 语义且避免下半因
//    下方非草在后续链路中意外自毁，先显式铺 grass_block 作下半支撑。
// 2. 上半需用 setBlockWithStates("minecraft:sunflower", pos, "half=upper") 放置（setBlockType 只放
//    defaultState=lower）。上半放置向下半派发 Up 更新（下半 isLower==isUpDirection 命中，facingState
//    =upper is(this)&&half!=lower → 保持，下半不自毁）。
// 3. 移除下半/上半必须是真实状态变化（非 no-op）以派发更新：下半原本是 sunflower(lower)，设 air 是
//    真实变化；上半原本是 sunflower(upper)，设 air 真实变化。
//
// 不测 randomTick/骨粉生长：双高植物不随机生长（高草丛骨粉生成是另一机制，概率性，跳过）。
// 不测 BushBlock 下方支撑失效自毁：与 SnowTests 模式重复，且 canSustain 依赖下方方块 canSustainPlant，
// 本文件聚焦双高植物的「另一半消失自毁」独特行为。
//
// 跨服务端：half state 名两端一致（half=upper/lower，Java 式），自毁行为与 vanilla 一致（另一半消失
// 即自毁，同步），可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_高草丛.txt（两格高植物，破坏即消失）
// Ref: DoublePlantBlock.cpp（updatePostPlacement Y 轴另一半自毁分支）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除下半方块时上半自毁。
//
// 布局：(3,1,1) 铺 grass_block 作下半支撑，(3,2,1) 放向日葵下半（lower，setBlockType 默认），
// (3,3,1) 放向日葵上半（upper，setBlockWithStates "half=upper"）。再 (3,2,1) 设 air 移除下半。
// air 放置向 Up 邻居上半派发 updatePostPlacement(Down) → 上半 isLower(false)==isUpDirection(false)
// 命中 → facingState=air 非 this → 返回 air，上半自毁。
//
// 判定：succeedWhenBlockPresent 断言上半格 (3,3,1) 向日葵消失（同 tick 同步）。
function doublePlantUpperBreaksWhenLowerRemoved(test: Test): void {
    // (3,1,1) 铺 grass_block 作下半支撑（canSustain 满足，下半放置稳定）。
    test.setBlockType("minecraft:grass_block", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放向日葵下半（lower，setBlockType 取 defaultState half=Lower）。
    test.setBlockType("minecraft:sunflower", { x: 3, y: 2, z: 1 });

    // (3,3,1) 放向日葵上半（upper，setBlockWithStates 指定 half=upper）。上半放置向下半派发 Up 更新，
    // 下半 isLower==isUpDirection 命中且 facingState=upper is(this)&&half!=lower → 保持不自毁。
    test.setBlockWithStates("minecraft:sunflower", { x: 3, y: 3, z: 1 }, "half=upper");

    // (3,2,1) 设 air 移除下半（sunflower→air 真实状态变化，派发邻居更新）。air 放置向 Up 邻居上半
    // 派发 updatePostPlacement(Down) → 上半命中自毁分支 → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言上半格 (3,3,1) 向日葵已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:sunflower", { x: 3, y: 3, z: 1 }, false);
}

// 移除上半方块时下半自毁（对称分支，验证 Up 方向自毁）。
//
// 布局同上，但移除 (3,3,1) 上半。air 放置向 Down 邻居下半派发 updatePostPlacement(Up) → 下半
// isLower(true)==isUpDirection(true) 命中 → facingState=air 非 this → 返回 air，下半自毁。
function doublePlantLowerBreaksWhenUpperRemoved(test: Test): void {
    test.setBlockType("minecraft:grass_block", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:sunflower", { x: 3, y: 2, z: 1 });
    test.setBlockWithStates("minecraft:sunflower", { x: 3, y: 3, z: 1 }, "half=upper");

    // (3,3,1) 设 air 移除上半（sunflower→air 真实变化，派发邻居更新）。air 放置向 Down 邻居下半
    // 派发 updatePostPlacement(Up) → 下半命中自毁分支 → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 3, z: 1 });

    // 断言下半格 (3,2,1) 向日葵已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:sunflower", { x: 3, y: 2, z: 1 }, false);
}

export function registerDoublePlantTests(): void {
    GameTest.register("BlockBehaviorTests", "double_plant_upper_breaks_when_lower_removed", doublePlantUpperBreaksWhenLowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "double_plant_lower_breaks_when_upper_removed", doublePlantLowerBreaksWhenUpperRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
