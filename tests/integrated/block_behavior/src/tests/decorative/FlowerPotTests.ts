// 花盆放花/取花行为 GameTest。
//
// wiki tech_花盆.txt#用途：手持花（或可盆栽植物）右键空花盆 → 花盆变为对应 potted_* 方块（盆栽）；
//   手持不可盆栽物品右键花盆 → 不触发放入（onBlockActivated 返 Pass）；
//   已有盆栽的花盆，手持花再右键 → 消费动作但不重复放入（返 Consume，方块不变）；
//   空手右键已盆栽花盆 → 取出内容物，花盆变回空花盆（vanilla useWithoutItem 路径）。
//
// C++ 链路：FlowerPotBlock（decorative/FlowerPotBlock.cpp）空花盆 m_potted=nullptr，potted_* 各自
//   m_potted 指向内容物方块，构造时注册到 s_pottedByContent 反查表。
//   - onBlockActivated（:112-197）：
//     · 分支1（手持非空）：dynamic_cast<BlockItem> 取 contentBlock → getByContent 反查 potted 方块；
//       不可盆栽（targetPot==nullptr）→ Pass；已有内容物（!isEmpty）→ Consume（不重复放）；
//       空花盆 → setBlockState 替换为 potted_* defaultState + shrink(1) → Success。
//     · 分支2（空手）：空花盆 → Consume（无操作）；已有内容物 → 取出内容物入背包 + setBlockState
//       变回 minecraft:flower_pot → Success。
//   - 放花是方块类型替换（flower_pot → potted_dandelion），非同方块 state 变化。
//   - isEmpty()：m_potted==nullptr 即空花盆。
//   - getByContent：s_pottedByContent[contentBlock] 反查（dandelion → POTTED_DANDELION）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。花盆 onBlockActivated 放花返回 Success 短路；非植物物品返 Pass 后 fallback
//   （stick 非 BlockItem，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//   useItemOnBlock 调 onBlockActivated 前把 stack 设到主手选中槽，使 onBlockActivated 的
//   player.getHeldItem(hand) 读到花。
//
// 测试覆盖（4 个场景，覆盖 wiki 放花/非植物不触发/已有内容物不重复放/空手取花回空花盆核心行为）：
//   1. 放蒲公英：空花盆 + dandelion useItemOnBlock → potted_dandelion，返 true（Success）。
//   2. 非植物不触发：空花盆 + 木棍 useItemOnBlock → 仍 flower_pot，返 false。
//   3. 已有内容物不重复放：potted_dandelion + dandelion useItemOnBlock → 返 true（Consume），仍 potted_dandelion。
//   4. 空手取花：potted_dandelion + interactWithBlock（空手右键）→ 返 true，花盆变回 flower_pot（one-sided，
//      依赖 Cubium 补全的 interactWithBlock 绑定，基岩 BDS 该 API 行为需另行验证）。
//
// 关键约束：
// 1. 花盆需放在固体方块上方（isValidPosition 检查 belowState.isSolid）——(3,1,1) 放 stone 支撑，
//    (3,2,1) 放空花盆（minecraft:flower_pot）。
// 2. 放花是方块类型替换：判定用 getBlock typeId 是否变为 potted_dandelion。
// 3. 用 dandelion（蒲公英，两端一致的盆栽植物）测放花，对齐 wiki「手持花右键花盆」。
//    dandelion 是 BlockItem（registerBlockBackedItem + registerSimpleBlock），onBlockActivated
//    dynamic_cast<BlockItem> 成功，getByContent 反查到 POTTED_DANDELION。
// 4. 判定「非植物不触发」用木棍（普通 Item，非 BlockItem）——dynamic_cast<BlockItem> 返 nullptr，
//    getByContent 不查 → Pass → fallback（木棍无 onItemUse）→ useItemOnBlock 返 false。
// 5. SimulatedPlayer 默认创造模式：放花 onBlockActivated 内部 shrink(1) 作用于选中槽，创造模式
//    仍 shrink（花盆放花不区分创造模式，与堆肥桶/唱片机不同——花盆始终消耗）。但放花判定看方块
//    类型变为 potted_dandelion，与消耗无关。
// 6. 场景 3 已有内容物：onBlockActivated 分支1 getByContent(dandelion)=POTTED_DANDELION，
//    !isEmpty()（已是 potted_dandelion）→ return Consume（不 setBlockState，方块不变）。
//
// 不测「空手取花」：onBlockActivated 取花走分支2（空手 useWithoutItem），但 SimulatedPlayer::
//   useItemOnBlock 强制要 ItemStack 形参（_unwrapItemStack nullptr 即报错），useItemInSlotOnBlock
//   空槽直接返 false 不调 useItemOnBlock，无法构造空手路径。已补全 interactWithBlock 脚本绑定
//   （ScriptSimulatedPlayer.cpp，空手右键复用 useItemOnBlock 空堆路径，仅走 Block.use 不走 Item.useOn），
//   见下方 flower_pot_ejects_flower_when_empty_hand 场景。
// 不测「下界/末地花盆行为」：花盆在下界/末地无特殊行为（不像床爆炸），跳过。
//
// 跨服务端：花盆 flower_pot/potted_dandelion 方块名两端一致，放花行为与 vanilla 一致。基岩无
//   setBlockWithStates，本测试用 setBlockType 放空花盆（默认 state），两端均可放；放花后判定方块
//   类型两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_花盆.txt#用途（放花/取花/不可盆栽不触发）
// Ref: FlowerPotBlock.cpp（onBlockActivated 分支1放花/分支2取花；getByContent 反查；isEmpty）
// Ref: FlowerPotBlocks.cpp:149（potted_dandelion 注册，内容物 DANDELION）
// Ref: Items.cpp:4179 / BlockItemRegistry.cpp:753（dandelion 为 BlockItem）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 花盆 (3,2,1)，下方 (3,1,1) stone 支撑（花盆需 solid 上方放置）。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性，非官方 type.id）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 放支撑 + 空花盆：(3,1,1) stone 支撑，(3,2,1) 空花盆（minecraft:flower_pot 默认 state）。
function placeFlowerPot(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:flower_pot", { x: 3, y: 2, z: 1 }); // 空花盆
}

// 场景 1：放蒲公英——空花盆 + dandelion useItemOnBlock → potted_dandelion，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空花盆。
// onBlockActivated 分支1：dandelion 是 BlockItem → getByContent(DANDELION)=POTTED_DANDELION →
// isEmpty()（空花盆）→ setBlockState 替换为 potted_dandelion defaultState + shrink(1) → Success。
//
// 判定：useItemOnBlock 返 true（Success），typeId === "minecraft:potted_dandelion"。
function flowerPotPlacesDandelion(test: Test): void {
    placeFlowerPot(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:flower_pot", `pot should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const dandelion = new ItemStack("minecraft:dandelion", 1);

    // 对空花盆 useItemOnBlock 蒲公英 → onBlockActivated getByContent → setBlockState potted_dandelion → Success。
    const used = farmer.useItemOnBlock(
        dandelion as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing dandelion in flower pot");

    // 判定：方块类型变为 potted_dandelion（放花是方块类型替换）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:potted_dandelion", `pot should become potted_dandelion, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：非植物不触发——空花盆 + 木棍 useItemOnBlock → 仍 flower_pot，返 false。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空花盆。
// onBlockActivated 分支1：木棍 dynamic_cast<BlockItem> 返 nullptr（木棍是普通 Item 非 BlockItem）→
// contentBlock=nullptr → getByContent 不查 → targetPot=nullptr → return Pass。
// Pass → fallback Item.useOn（木棍普通 Item，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//
// 注意：木棍非 BlockItem 是关键——若用 BlockItem（如石头），dynamic_cast 成功但 getByContent(stone)
// 返 nullptr（石头不可盆栽）也走 Pass。但木棍更干净（连 BlockItem 都不是），且避免石头 onItemUse
// 在花盆上方放置方块返 Success 的误判。
//
// 判定：useItemOnBlock 返 false（未触发放花），typeId === "minecraft:flower_pot"（花盆仍空）。
function flowerPotIgnoresNonFlowerItem(test: Test): void {
    placeFlowerPot(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:flower_pot", `pot should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对空花盆 useItemOnBlock 木棍 → dynamic_cast<BlockItem> nullptr → Pass → fallback（木棍无 onItemUse）→ false。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false for non-flower item (stick), got ${used}`);

    // 判定：花盆仍为空 flower_pot（木棍非可盆栽植物，未触发放花）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:flower_pot", `pot should remain empty flower_pot for non-flower item, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：已有内容物不重复放——potted_dandelion + dandelion useItemOnBlock → 返 true（Consume），仍 potted_dandelion。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) potted_dandelion（先放蒲公英使花盆有内容物）。
// onBlockActivated 分支1：dandelion 是 BlockItem → getByContent(DANDELION)=POTTED_DANDELION →
// !isEmpty()（已是 potted_dandelion，有内容物）→ return Consume（不 setBlockState，方块不变，不重复放）。
//
// 判定：useItemOnBlock 返 true（Consume），typeId 仍 === "minecraft:potted_dandelion"（未替换）。
function flowerPotDoesNotReplaceWhenAlreadyHasFlower(test: Test): void {
    placeFlowerPot(test);

    // 先放蒲公英使花盆变为 potted_dandelion（模拟场景 1 的放花过程）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const firstDandelion = new ItemStack("minecraft:dandelion", 1);
    const placeResult = farmer.useItemOnBlock(
        firstDandelion as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(placeResult, "first dandelion placement should succeed");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:potted_dandelion", `pot should be potted_dandelion before second use, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 已有内容物的花盆再 useItemOnBlock 蒲公英 → onBlockActivated !isEmpty() → Consume（不替换）。
    const secondDandelion = new ItemStack("minecraft:dandelion", 1);
    const used = farmer.useItemOnBlock(
        secondDandelion as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when pot already has flower (Consume)");

    // 判定：方块仍为 potted_dandelion（已有内容物不重复放入，未替换）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:potted_dandelion", `pot should remain potted_dandelion (not replaced), got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：空手取花——potted_dandelion + interactWithBlock（空手右键）→ 返 true，花盆变回 flower_pot。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) potted_dandelion（先放蒲公英使花盆有内容物）。
// interactWithBlock 走 SimulatedPlayer 空手右键路径（复用 useItemOnBlock 空堆，仅 Block.use 不走 Item.useOn）。
// onBlockActivated 分支2（空手 useWithoutItem）：heldStack.isEmpty()（空手）→ !isEmpty()（已是 potted_dandelion
//   有内容物）→ 取出内容物入背包 + setBlockState 变回 minecraft:flower_pot → Success。
//
// 关键：interactWithBlock 内部不 setItem 到选中槽，玩家真实选中槽为空（创造模式默认空手），故
//   onBlockActivated 的 getHeldItem(MainHand) 读到空堆，分支2 heldStack.isEmpty() 为 true 触发取花。
//   故用新 spawn 的 SimulatedPlayer（选中槽未被先前 useItemOnBlock 污染）确保空手。
//
// 判定：interactWithBlock 返 true（Success），typeId === "minecraft:flower_pot"（花盆变回空花盆）。
//
// one-sided：依赖 Cubium 补全的 interactWithBlock 绑定。基岩 BDS 官方 SimulatedPlayer.interactWithBlock
//   存在但本项目未与基岩对比验证其空手取花行为，故标 one-sided（仅 Cubium 跑）。
function flowerPotEjectsFlowerWhenEmptyHand(test: Test): void {
    placeFlowerPot(test);

    // 先放蒲公英使花盆变为 potted_dandelion（模拟场景 1 的放花过程）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const dandelion = new ItemStack("minecraft:dandelion", 1);
    const placeResult = farmer.useItemOnBlock(
        dandelion as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(placeResult, "first dandelion placement should succeed");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:potted_dandelion", `pot should be potted_dandelion before empty-hand interact, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // interactWithBlock 空手右键已盆栽花盆 → onBlockActivated 分支2 取花 + 变回 flower_pot → Success。
    // interactWithBlock 为 Cubium 补全的 SimulatedPlayer 方法（基岩官方 API 名一致，但
    // @minecraft/server-gametest 类型定义未声明），用 as any 绕过类型检查。
    const used = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean })
        .interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(used, "interactWithBlock should return true when ejecting flower with empty hand");

    // 判定：花盆变回空 flower_pot（空手取出内容物后回空花盆）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:flower_pot", `pot should become empty flower_pot after empty-hand eject, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerFlowerPotTests(): void {
    GameTest.register("BlockBehaviorTests", "flower_pot_places_dandelion", flowerPotPlacesDandelion)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "flower_pot_ignores_non_flower_item", flowerPotIgnoresNonFlowerItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "flower_pot_does_not_replace_when_already_has_flower", flowerPotDoesNotReplaceWhenAlreadyHasFlower)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "flower_pot_ejects_flower_when_empty_hand", flowerPotEjectsFlowerWhenEmptyHand)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
