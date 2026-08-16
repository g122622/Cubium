// 梯子附着面自毁行为 GameTest（移除背面附着方块时梯子自毁）。
//
// wiki tech_梯子.txt（:52）：「梯子只能被放置在一个方块完整的侧面上。」即梯子须附在固体方块的
// 完整侧面（isSolidSide）。背面附着方块被移除时，梯子失去附着自毁掉落（vanilla 经 neighborChanged
// → updateShape 链）。
//
// C++ 链路：LadderBlock 有 facing state（HORIZONTAL_FACING，梯子朝向=背离附着面）。
//   - isValidPosition（LadderBlock.cpp:101-115）：facing 反方向（背面 attachPos）方块的
//     isSolidSide(world, attachPos, facing)。
//   - updatePostPlacement（:117-145）：facing==opposite(ladderFacing)（背面邻居变化）且
//     isSolidSide(attachState, attachPos, ladderFacing) 失败时返回 air 自毁。同 tick 同步。
// 梯子 facing=East 表示梯子朝东，背面在 West 邻位（附在 West 邻位方块的 East 面）。
// isSolidSide 对固体不透明方块（stone）的完整面返回 true；air/glass（透明）返回 false。
//
// 放置语义：setBlockWithStates 走 _resolveBlock 取 defaultState（facing=North），经 setBlockWithStates
// 显式设 facing=east，不经 isValidPosition，故即使背面非固体也能强放。LadderBlock 无 onBlockAdded
// 重写，放置不向自身派发 updatePostPlacement，强放不立即自毁。需「第二步移除背面附着方块」触发梯子
// opposite(facing) 方向 updatePostPlacement 才自毁。
//
// 测试覆盖（1 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   - 梯子 facing=east 附在 West 邻位 stone 的 East 面，移除 stone → 梯子自毁。
//
// 关键约束（同支撑自毁范式，见 VineTests/LanternTests）：
// 1. 先放背面 stone 再放梯子，保证梯子强放时背面有附着（贴近 vanilla 放置语义）。放置不向自身
//    派发 updatePostPlacement，facing=east 被保留。
// 2. 移除背面 stone 必须是非 no-op 写入——先显式铺 stone 再放梯子，再设 air 移除 stone，保证
//    stone→air 真实状态变化派发更新。air 放置向 East 邻位梯子派发 updatePostPlacement(West) →
//    facing=east→ladderFacing=East→背面 West=(2,1,1)=air → isSolidSide(air, east) false → 返回 air，
//    梯子自毁。
//
// 不测「梯子攀爬速度」：依赖实体碰撞箱 + isLadder，属实体行为，非方块状态行为点，跳过。
// 不测「梯子含水（waterlogged）」：依赖水流动/含水体系，且支撑自毁核心行为点与含水无关，跳过。
// 不测「四个朝向（north/south/west）」：与 east 对称，行为点相同（isSolidSide 判定），按「单一职责」
// 本文件聚焦 east 朝向。TODO: 可补 ladder_other_facings 覆盖其余朝向。
//
// 跨服务端：梯子 facing state 名两端一致（Java 式 north/south/east/west），附着面自毁行为与 vanilla
// 一致（isSolidSide 失败即破坏，同步），可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_梯子.txt（梯子放置在方块完整侧面）
// Ref: LadderBlock.cpp（isValidPosition/updatePostPlacement isSolidSide 附着面自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (2,1,1) 作背面附着 stone，(3,1,1) 作梯子，均在 air 空腔内。

// 移除梯子背面附着方块（石头）时梯子自毁变 air。
//
// 布局：(2,1,1) 铺 stone 作梯子背面附着（isSolidSide(east) true），(3,1,1) 用 setBlockWithStates
// 放 facing=east 梯子（背面 West=(2,1,1) stone），再 (2,1,1) 设 air 移除附着。
// air 放置向 East 邻位梯子派发 updatePostPlacement(West=opposite(east)) → 背面 (2,1,1)=air →
// isSolidSide(air, east) false → 返回 air，梯子自毁。
//
// 判定：succeedWhenBlockPresent 断言梯子格 (3,1,1) 梯子消失（同 tick 同步）。
function ladderBreaksWhenAttachedBlockRemoved(test: Test): void {
    // (2,1,1) 铺 stone 作梯子背面附着（isSolidSide(east) true，梯子 facing=east 附在其 East 面）。
    test.setBlockType("minecraft:stone", { x: 2, y: 1, z: 1 });

    // (3,1,1) 用 setBlockWithStates 放 facing=east 梯子（背面 West=(2,1,1) stone）。setBlockType 取
    // defaultState（facing=North），需 setBlockWithStates 显式设 facing=east 使背面为 West 邻位。
    test.setBlockWithStates("minecraft:ladder", { x: 3, y: 1, z: 1 }, "facing=east");

    // (2,1,1) 设 air 移除背面附着（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向
    // East 邻位梯子派发 updatePostPlacement(West) → ladderFacing=East→opposite=West→背面 (2,1,1)=air →
    // isSolidSide(air, east) false → 返回 air，梯子自毁。
    test.setBlockType("minecraft:air", { x: 2, y: 1, z: 1 });

    // 断言梯子格 (3,1,1) 梯子已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:ladder", { x: 3, y: 1, z: 1 }, false);
}

export function registerLadderTests(): void {
    GameTest.register("BlockBehaviorTests", "ladder_breaks_when_attached_block_removed", ladderBreaksWhenAttachedBlockRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
