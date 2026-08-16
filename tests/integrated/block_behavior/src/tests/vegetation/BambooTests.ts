// 竹子/竹笋支撑自毁行为 GameTest（移除下方支撑方块时竹子/竹笋被破坏）。
//
// wiki tech_竹子.txt#生长（:75）：竹子可种植在草方块、菌丝体、灰化土、泥土、缠根泥土、砂土、泥巴、
// 沾泥的红树根、苔藓块、苍白苔藓块、沙砾、可疑的沙砾、沙子、红沙和可疑的沙子上，也能种在竹子和竹笋上。
// wiki tech_竹子.txt#用途（:69）：「竹子被破坏时，自身及上方的所有竹子都会被破坏并掉落」，且竹子依附
// 下方种植面，下方支撑失效时竹子失去支撑自毁掉落（vanilla BambooStalkBlock.updateShape→canSurvive 失败
// → scheduleTick(1) → tick 调 destroyBlock）。
//
// C++ 链路：BambooBlock 有 AGE_0_1/STAGE_0_1/BAMBOO_LEAVES 三状态。
//   - isValidPosition（BambooBlock.cpp:112-131）：下方是竹子本身 → true；否则下方在
//     BlockTags::BAMBOO_PLANTABLE_ON 标签内 → true；否则 false。
//   - updatePostPlacement（:133-154）：仅当 facing==Down 时调 isValidPosition，失败返回 airState 自毁。
//     反应同 tick 同步（updatePostPlacement 直接返回 air，ServerWorld 立即 setBlockState），与
//     SugarCaneBlock/LanternBlock/TorchBlock 自毁链路一致。
//   - BambooSaplingBlock::isValidPosition（:350-368）：下方在 BAMBOO_PLANTABLE_ON 标签内 → true；
//     下方是竹子则 false（竹笋不能放竹子上）。
//   - BambooSaplingBlock::updatePostPlacement（:370-391）：facing==Down 且 isValidPosition 失败 → 返回 air。
// canSupportCenter/isSolidSide 不参与竹子支撑判定（与灯笼/梯子不同），竹子纯走 BAMBOO_PLANTABLE_ON 标签。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState，不经 isValidPosition，故即使下方非种植面也能
// 强放。BambooBlock/BambooSaplingBlock 无 onBlockAdded 重写，放置不向自身派发 updatePostPlacement，强放
// 不立即自毁。需「第二步移除下方支撑」触发 Down 方向 updatePostPlacement 才自毁。
//
// 测试覆盖（3 个场景，行为与 Cubium 现有自毁链路一致）：
//   1. 竹子（minecraft:bamboo）下方 grass_block 支撑移除 → 自毁。
//   2. 竹笋（minecraft:bamboo_sapling）下方 grass_block 支撑移除 → 自毁。
//   3. 竹子种在 moss_block 上不自毁（回归保护：BAMBOO_PLANTABLE_ON 标签须含 #dirt 展开的苔藓块）。
//
// 关键约束（同支撑自毁范式，见 SugarCaneTests/LanternTests）：
// 1. 先放 grass_block 支撑再放竹子/竹笋，保证强放时下方有支撑（贴近 vanilla 放置语义）。放置不向自身
//    派发 updatePostPlacement，竹子/竹笋保留。
// 2. 移除支撑必须是非 no-op 写入——先显式铺 grass_block 支撑再放竹子/竹笋，再设 air 移除支撑，保证
//    grass_block→air 真实状态变化派发更新。air 放置向 Up 邻居竹子/竹笋派发 updatePostPlacement(Down) →
//    下方 air 不在 BAMBOO_PLANTABLE_ON 标签（air 非标签成员）→ isValidPosition 失败 → 返回 air，自毁。
//
// 不测 randomTick 生长：概率性（random.nextInt(3)==0，wiki :77「1/3 概率长高一格」），非确定，按准则跳过。
// 不测骨粉催熟 grow：canUseBonemeal 概率性（0.45，:211），且 grow 内 1+random.nextInt(2) 长高格数随机，
//   非确定，按准则跳过。TODO: 待骨粉确定性路径稳定后可补 bamboo_bonemeal_grows 测试。
// 不测「竹子叠竹子（getStateForPlacement 继承 leaves）」：属放置语义而非支撑自毁行为点，且 Cubium
//   updatePostPlacement 未实现 vanilla Up 分支（cycle AGE），行为与 vanilla 不一致，按准则不为不一致写测试。
// 不测「竹子被破坏时上方竹子连锁掉落」：依赖连锁破坏链（vanilla 由 updateShape Up 分支 + canSurvive 链式
//   scheduleTick 实现），Cubium updatePostPlacement 仅处理 Down 不实现 Up 连锁，行为与 vanilla 不一致，跳过。
//
// 跨服务端注意：竹子 leaves/stage/age state 名两端一致（Java 式），但本测试只断言方块变 air（核心自毁
// 行为），不读 state。需注意 Cubium 端自毁是同步（updatePostPlacement 返回 air），vanilla 端是延迟
// （scheduleTick(1) 后 destroyBlock）——两端 tick 容忍度需放宽，但「最终是否自毁」结论一致可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_竹子.txt#生长（竹子可种植方块列表）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_竹子.txt#用途（竹子破坏时上方连锁掉落）
// Ref: BambooBlock.cpp（isValidPosition/updatePostPlacement Down 支撑自毁）
// Ref: BambooStalkBlock.java（vanilla canSurvive/updateShape/tick 延迟自毁链）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (3,1,1) 作支撑位、(3,2,1) 作竹子/竹笋位，均在 air 空腔内。

// 移除竹子下方支撑方块（草方块）时竹子自毁变 air。
//
// 布局：(3,1,1) 铺 grass_block 作竹子下方支撑（grass_block 在 BAMBOO_PLANTABLE_ON 标签内），
// (3,2,1) 放竹子（在 grass_block 上，强放绕过 isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居竹子派发 updatePostPlacement(Down) → 下方 air 不在 BAMBOO_PLANTABLE_ON 标签 →
// isValidPosition 失败 → 返回 air，竹子自毁。
//
// 判定：succeedWhenBlockPresent 断言竹子格 (3,2,1) 竹子消失（同 tick 同步）。
function bambooBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 grass_block 作竹子下方支撑（grass_block ∈ BAMBOO_PLANTABLE_ON，isValidPosition true）。
    test.setBlockType("minecraft:grass_block", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放竹子（在 grass_block 上，强放绕过 isValidPosition，BambooBlock 无 onBlockAdded 不立即自毁）。
    test.setBlockType("minecraft:bamboo", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（grass_block→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 竹子派发 updatePostPlacement(Down) → 下方 air ∉ BAMBOO_PLANTABLE_ON → isValidPosition false → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言竹子格 (3,2,1) 竹子已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:bamboo", { x: 3, y: 2, z: 1 }, false);
}

// 移除竹笋下方支撑方块（草方块）时竹笋自毁变 air。
//
// 布局：(3,1,1) 铺 grass_block 作竹笋下方支撑，(3,2,1) 放竹笋（在 grass_block 上，强放绕过 isValidPosition），
// 再 (3,1,1) 设 air 移除支撑。air 放置向 Up 邻居竹笋派发 updatePostPlacement(Down) → 下方 air 不在
// BAMBOO_PLANTABLE_ON 标签 → BambooSaplingBlock::isValidPosition 失败 → 返回 air，竹笋自毁。
//
// 判定：succeedWhenBlockPresent 断言竹笋格 (3,2,1) 竹笋消失（同 tick 同步）。
function bambooSaplingBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 grass_block 作竹笋下方支撑（grass_block ∈ BAMBOO_PLANTABLE_ON）。
    test.setBlockType("minecraft:grass_block", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放竹笋（在 grass_block 上，强放绕过 isValidPosition，BambooSaplingBlock 无 onBlockAdded 不立即自毁）。
    test.setBlockType("minecraft:bamboo_sapling", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（grass_block→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 竹笋派发 updatePostPlacement(Down) → 下方 air ∉ BAMBOO_PLANTABLE_ON → isValidPosition false → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言竹笋格 (3,2,1) 竹笋已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:bamboo_sapling", { x: 3, y: 2, z: 1 }, false);
}

// 竹子种在苔藓块（moss_block）上不自毁，验证 BAMBOO_PLANTABLE_ON 标签包含 wiki#生长 列举的苔藓块。
//
// 背景：wiki tech_竹子.txt#生长（:75）明确竹子可种植在「苔藓块」上。Cubium 早期 BAMBOO_PLANTABLE_ON
// 标签因不支持 #dirt 子标签引用而漏掉了 moss_block（moss_block 在 vanilla #dirt 标签内），导致竹子在
// 苔藓块上无法种植（isValidPosition 返回 false）。该缺陷已修复（标签展开 #dirt 全部成员），本测试为
// 回归保护：确保 moss_block 始终在 BAMBOO_PLANTABLE_ON 标签内，竹子在其上不被自毁判定破坏。
//
// 验证链路（利用 updatePostPlacement(Down) 同步触发 isValidPosition）：
// 1. 先在 (3,2,1) 强放竹子（下方 (3,1,1) 暂为 air，强放绕过 isValidPosition，BambooBlock 无
//    onBlockAdded 不立即自毁，悬空竹子存活到下一步）。
// 2. 再在 (3,1,1) 放 moss_block（air→moss_block 真实状态变化，派发邻居更新）。moss_block 放置向 Up
//    邻居竹子派发 updatePostPlacement(Down) → isValidPosition(下方=moss_block)：
//    - 修复前：moss_block ∉ BAMBOO_PLANTABLE_ON → false → 返回 air，竹子自毁（测试失败）。
//    - 修复后：moss_block ∈ BAMBOO_PLANTABLE_ON → true → 返回原 state，竹子存活（测试通过）。
//
// 判定：succeedWhenBlockPresent 断言竹子格 (3,2,1) 竹子仍在（true）。若标签回归漏掉 moss_block，
// 竹子会在第二步被自毁，断言失败，从而暴露标签缺陷。
//
// 注意：本测试只验证 moss_block 单一新增种植面（代表 #dirt 展开修复），不为每种新种植面（mycelium/
// rooted_dirt/mud/pale_moss_block/muddy_mangrove_roots/suspicious_gravel）各写一个测试——这些成员的
// 标签包含关系已由 BlockTagsTest.BambooPlantableOnContains* 单元测试覆盖，本集成测试聚焦「竹子实际
// 不自毁」的端到端行为，单一代表方块足够。
function bambooSurvivesOnMossBlock(test: Test): void {
    // (3,2,1) 强放竹子（下方 (3,1,1) 暂为 air，强放绕过 isValidPosition，悬空不立即自毁）。
    test.setBlockType("minecraft:bamboo", { x: 3, y: 2, z: 1 });

    // (3,1,1) 放 moss_block（air→moss_block 真实状态变化，派发邻居更新）。moss_block 放置向 Up 邻居竹子
    // 派发 updatePostPlacement(Down) → isValidPosition(下方=moss_block) → moss_block ∈ BAMBOO_PLANTABLE_ON
    // → true → 返回原 state，竹子存活（修复后行为）。
    test.setBlockType("minecraft:moss_block", { x: 3, y: 1, z: 1 });

    // 断言竹子格 (3,2,1) 竹子仍在（修复后 moss_block 是有效种植面，竹子不自毁）。
    test.succeedWhenBlockPresent("minecraft:bamboo", { x: 3, y: 2, z: 1 }, true);
}

export function registerBambooTests(): void {
    GameTest.register("BlockBehaviorTests", "bamboo_breaks_when_support_below_removed", bambooBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "bamboo_sapling_breaks_when_support_below_removed", bambooSaplingBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "bamboo_survives_on_moss_block", bambooSurvivesOnMossBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
