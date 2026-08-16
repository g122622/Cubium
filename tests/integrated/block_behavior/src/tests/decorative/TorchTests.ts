// 火把下方支撑自毁行为 GameTest（移除下方支撑方块时火把自毁）。
//
// wiki block_火把（燃尽）.txt：火把放置需有坚固上表面的方块支撑（地面火把站立在下方方块上）。
// wiki block_灵魂火把.txt 同理：火把必须放在方块中心完整上表面的上方，依附方块失效时破坏掉落。
// 下方支撑方块被移除时，火把失去支撑自毁（vanilla 经 neighborChanged→updateShape 链）。
// （注：1.21.6+ 火把放置后随机刻变「燃尽」为实验特性且概率性，Cubium 未实现，本文件不涉及，
// 仅测稳定的支撑自毁行为点。）
//
// C++ 链路：TorchBlock::updatePostPlacement（TorchBlock.cpp:60-79）当 facing==Down 时检查
// _canSurvive（:81-86，Block::canSupportCenter(world, pos.down(), Up)）。移除下方支撑（→air）后，
// 下方 air canSupportCenter false → _canSurvive 失败 → 返回 air 自毁。反应同 tick 同步。
// isValidPosition（:52-58）同样判 canSupportCenter(pos.down(), Up)，强放绕过之。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState，不经 isValidPosition，故即使下方非坚固
// 面也能强放火把。TorchBlock 无 onBlockAdded 重写，放置不向自身派发 updatePostPlacement，强放不
// 立即自毁。需「第二步移除下方支撑」触发火把 Down 方向 updatePostPlacement 才自毁。
//
// 关键约束（同支撑自毁范式，见 KelpTests/LanternTests）：
// 1. 先放 stone 支撑再放火把，保证火把强放时下方有支撑（贴近 vanilla 放置语义）。放置不向自身
//    派发 updatePostPlacement，火把保留。
// 2. 移除支撑必须是非 no-op 写入——先显式铺 stone 支撑再放火把，再设 air 移除支撑，保证 stone→air
//    真实状态变化派发更新。air 放置向 Up 邻居火把派发 updatePostPlacement(Down) → 下方 air
//    canSupportCenter(Up) false → _canSurvive 失败 → 返回 air，火把自毁。
//
// 不测「火把发光等级 14」：光照测试属 lighting 包范畴，本文件聚焦支撑自毁行为点。
// 不测「墙火把（WallTorchBlock）侧面支撑自毁」：墙火把 typeId 不同（minecraft:wall_torch），且侧面
// 支撑判定独立，本文件聚焦地面火把（minecraft:torch）的下方支撑自毁核心行为点。
//   TODO: 可补 wall_torch_breaks_when_support_removed 覆盖 WallTorchBlock 侧面支撑。
// 不测「火把燃尽」：1.21.6+ 实验特性，概率性随机刻，Cubium 未实现，按准则跳过。
//
// 跨服务端：火把无 state（地面火把单态），支撑自毁行为与 vanilla 一致（canSupportCenter 失败即
// 破坏，同步），可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_灵魂火把.txt（火把中心完整上表面支撑+失效掉落）
// Ref: TorchBlock.cpp（updatePostPlacement Down _canSurvive canSupportCenter 支撑自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除火把下方支撑方块（石头）时火把自毁变 air。
//
// 布局：(3,1,1) 铺 stone 作火把下方支撑（canSupportCenter(Up) true），(3,2,1) 放火把（在 stone 上，
// 强放绕过 isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居火把派发 updatePostPlacement(Down) → 下方 air canSupportCenter(Up) false →
// _canSurvive 失败 → 返回 air。
//
// 判定：succeedWhenBlockPresent 断言火把格 (3,2,1) 火把消失（同 tick 同步）。
function torchBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 stone 作火把下方支撑（isFaceSturdy(Up, Center) true，canSupportCenter 满足）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放火把（在 stone 上，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:torch", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 火把派发 updatePostPlacement(Down) → 下方 air canSupportCenter(Up) false → _canSurvive 失败 →
    // 返回 air，火把自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言火把格 (3,2,1) 火把已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:torch", { x: 3, y: 2, z: 1 }, false);
}

export function registerTorchTests(): void {
    GameTest.register("BlockBehaviorTests", "torch_breaks_when_support_below_removed", torchBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
