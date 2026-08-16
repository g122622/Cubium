// 灯笼支撑自毁行为 GameTest（站立/悬挂两种形态的支撑失效自毁）。
//
// wiki block_灵魂灯笼.txt#放置（:42）：灯笼必须放在「方块中心完整上表面的上方」（站立，支撑在下方）
// 或「中心完整下表面的下方」（悬挂，支撑在上方）。依附的方块发生变化且不再能够依附时，灯笼会被
// 破坏并掉落。普通灯笼与灵魂灯笼放置/支撑行为一致（仅光照/交互差异），本文件测普通灯笼。
//
// C++ 链路：LanternBlock 有 hanging state（bool，true=悬挂/支撑在上方，false=站立/支撑在下方）。
//   - isValidPosition（LanternBlock.cpp:102-113）：hanging=true 检查 pos.up() 的 canSupportCenter(Down)；
//     hanging=false 检查 pos.down() 的 canSupportCenter(Up)。对齐 vanilla LanternBlock.canSurvive
//     （Block.canSupportCenter(world, pos.relative(supportDir), opposite(supportDir))）。
//   - updatePostPlacement（:115-141）：facing==supportDir（hanging=true→Up，false→Down）且
//     canSupportCenter(supportPos, opposite(supportDir)) 失败时返回 air 自毁。同 tick 同步。
// canSupportCenter（Block.cpp:822-836）→ isFaceSturdy(direction, SupportType::Center)：固体不透明方块
// （如 stone）的完整面返回 true；air/glass（透明）返回 false。故 stone 作支撑满足，air 不满足。
//
// 放置语义：setBlockType/setBlockWithStates 走 _resolveBlock 取 defaultState（hanging=false，
// waterlogged=false），不经 isValidPosition，故即使支撑缺失也能强放。LanternBlock 无 onBlockAdded
// 重写，放置不向自身派发 updatePostPlacement，强放不立即自毁。需「第二步移除支撑」触发灯笼
// supportDir 方向 updatePostPlacement 才自毁。
//
// 测试覆盖（2 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   1. 站立灯笼（hanging=false）下方支撑移除 → 自毁。
//   2. 悬挂灯笼（hanging=true）上方支撑移除 → 自毁。
//
// 关键约束（同支撑自毁范式，见 KelpTests/SugarCaneTests）：
// 1. 先放支撑 stone 再放灯笼，保证灯笼强放时支撑已存在（贴近 vanilla 放置语义）。放置不向自身派发
//    updatePostPlacement，hanging state 被保留。
// 2. 移除支撑必须是非 no-op 写入——先显式铺 stone 支撑再放灯笼，再设 air 移除支撑，保证 stone→air
//    真实状态变化派发更新。air 放置向灯笼派发 updatePostPlacement(supportDir) → canSupportCenter(air)
//    false → 返回 air，灯笼自毁。
//
// 不测「灯笼含水（waterlogged）」：依赖水流动/含水体系，且支撑自毁核心行为点与含水无关，跳过。
// 不测「铁链视觉连接」：纯视觉，无方块状态变化，不可测。
//
// 跨服务端：灯笼 hanging state 名两端一致（Java 式 bool），支撑自毁行为与 vanilla 一致（canSupportCenter
// 失败即破坏，同步），可跨服务端对比。getState("hanging") 用 as any 绕过 BlockStateSuperset 白名单
// （同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_灵魂灯笼.txt#放置（灯笼中心完整面支撑+失效掉落）
// Ref: LanternBlock.cpp（isValidPosition/updatePostPlacement/canSupportCenter 支撑判定）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (3,1,1)/(3,2,1) 作灯笼与支撑位，均在 air 空腔内。

// 站立灯笼（hanging=false）下方支撑移除时自毁变 air。
//
// 布局：(3,1,1) 铺 stone 作灯笼下方支撑（canSupportCenter(Up) true），(3,2,1) 用 setBlockWithStates
// 放 hanging=false 灯笼（站立，支撑在下方 stone），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居灯笼派发 updatePostPlacement(Down) → hanging=false→supportDir=Down →
// canSupportCenter((3,1,1)=air, Up) false → 返回 air，灯笼自毁。
//
// 判定：succeedWhenBlockPresent 断言灯笼格 (3,2,1) 灯笼消失（同 tick 同步）。
function lanternStandingBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 stone 作灯笼下方支撑（isFaceSturdy(Up, Center) true，canSupportCenter 满足）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 用 setBlockWithStates 放 hanging=false 灯笼（站立，支撑在下方 stone）。setBlockType 取
    // defaultState（hanging=false），此处显式 setBlockWithStates 以文档化 hanging=false 语义。
    test.setBlockWithStates("minecraft:lantern", { x: 3, y: 2, z: 1 }, "hanging=false");

    // (3,1,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 灯笼派发 updatePostPlacement(Down) → supportDir=Down → canSupportCenter((3,1,1)=air, Up) false →
    // 返回 air，灯笼自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言灯笼格 (3,2,1) 灯笼已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:lantern", { x: 3, y: 2, z: 1 }, false);
}

// 悬挂灯笼（hanging=true）上方支撑移除时自毁变 air。
//
// 布局：(3,2,1) 铺 stone 作灯笼上方支撑（canSupportCenter(Down) true），(3,1,1) 用 setBlockWithStates
// 放 hanging=true 灯笼（悬挂，支撑在上方 stone），再 (3,2,1) 设 air 移除支撑。
// air 放置向 Down 邻居灯笼派发 updatePostPlacement(Up) → hanging=true→supportDir=Up →
// canSupportCenter((3,2,1)=air, Down) false → 返回 air，灯笼自毁。
//
// 判定：succeedWhenBlockPresent 断言灯笼格 (3,1,1) 灯笼消失（同 tick 同步）。
function lanternHangingBreaksWhenSupportAboveRemoved(test: Test): void {
    // (3,2,1) 铺 stone 作灯笼上方支撑（isFaceSturdy(Down, Center) true，canSupportCenter 满足）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });

    // (3,1,1) 用 setBlockWithStates 放 hanging=true 灯笼（悬挂，支撑在上方 stone）。
    test.setBlockWithStates("minecraft:lantern", { x: 3, y: 1, z: 1 }, "hanging=true");

    // (3,2,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Down 邻居
    // 灯笼派发 updatePostPlacement(Up) → supportDir=Up → canSupportCenter((3,2,1)=air, Down) false →
    // 返回 air，灯笼自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言灯笼格 (3,1,1) 灯笼已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:lantern", { x: 3, y: 1, z: 1 }, false);
}

export function registerLanternTests(): void {
    GameTest.register("BlockBehaviorTests", "lantern_standing_breaks_when_support_below_removed", lanternStandingBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "lantern_hanging_breaks_when_support_above_removed", lanternHangingBreaksWhenSupportAboveRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
