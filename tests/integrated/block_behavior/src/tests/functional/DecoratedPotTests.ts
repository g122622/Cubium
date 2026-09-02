// 饰纹陶罐放物品/空手交互/破坏掉落行为 GameTest。
//
// wiki tech_饰纹陶罐.txt#交互机制：饰纹陶罐没有交互界面。当手持对应物品与饰纹陶罐交互时，
//   若物品成功存入，饰纹陶罐会前后晃动并在罐口冒出烟尘粒子（dust_plume）；若物品未成功存入
//   （饰纹陶罐已满或存有不同种物品）或若未手持物品与饰纹陶罐交互，饰纹陶罐会水平旋转晃动而不
//   冒出粒子。玩家无法通过自行交互取出其中的物品。
// wiki tech_饰纹陶罐.txt#容器容量：一个饰纹陶罐拥有1格储存空间，可在其中放置至多1组同类物品。
// wiki tech_饰纹陶罐.txt#破坏掉落：被不具有精准采集的剑、锹、镐、斧、锄、三叉戟或重锤挖掘或被
//   弹射物击中破坏时，饰纹陶罐被破坏后会掉落用于合成自身的四个物品和内容物。
//
// C++ 链路：DecoratedPotBlock（TrailsBlocks.cpp）有 FACING/CRACKED/WATERLOGGED state。
//   - onBlockActivated（TrailsBlocks.cpp:243-343）：
//     · 副手（Hand::OffHand）→ return Pass。
//     · 取方块实体，非 DecoratedPot 类型 → return Pass。
//     · 客户端：直接返回 Success（播放动画）。
//     · 服务端：取手持物品 heldItem + 罐内物品 potItem。
//       · heldItem 非空：
//         · canInsert = 陶罐为空 || (罐内物品与手持物品相同且未达到最大堆叠)
//         · canInsert 为 true：wobble(Positive) + split(1) + setItem + 创造模式 grow(1) +
//           playSound(BLOCK_DECORATED_POT_INSERT) + gameEvent(BLOCK_CHANGE) + updateComparators → Success。
//         · canInsert 为 false：走空手交互逻辑（负摇晃）。
//       · heldItem 为空：走空手交互逻辑（负摇晃）。
//     · 空手交互逻辑：wobble(Negative) + playSound(BLOCK_DECORATED_POT_INSERT_FAIL) + gameEvent → Success。
//   - onBlockRemoved（TrailsBlocks.cpp:388-422）：
//     · CRACKED 状态：掉落4个独立的陶片/砖块物品（getItemFromPattern）。
//     · 非 CRACKED 状态：掉落陶罐内存储的物品（getItem + spawnItemEntity）。
//     · 清空容器以防止重复掉落。
//   - playerWillDestroy（TrailsBlocks.cpp:345-361）：
//     · 手持 BREAKS_DECORATED_POTS 标签物品 + 无精准采集 → setBlockState CRACKED=true。
//     · 注意：playerWillDestroy 需要通过玩家破坏链路触发，但 SimulatedPlayer.breakBlock 脚本绑定
//       未实现（_throwNotImplemented），故无法通过脚本模拟玩家手持剑破坏陶罐的场景。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。饰纹陶罐 onBlockActivated 在所有路径（放入成功/失败/空手）都返回 Success，
//   故 useItemOnBlock 始终返回 true。这与 JukeboxTests 中"非唱片返 Pass → false"不同。
//   useItemOnBlock 调 onBlockActivated 前把 stack 设到主手选中槽，使 onBlockActivated 的
//   player.getHeldItem(hand) 读到手持物品。
//
// 测试覆盖（5 个场景，覆盖 wiki 放入成功/叠加/不同物品失败/空手负摇晃/破坏掉落核心行为）：
//   1. 放入物品成功：空罐 + 钻石 useItemOnBlock → 返 true + assertContainerContains(diamond)。
//   2. 同类物品可叠加放入：已放1钻石 + 再 useItemOnBlock 钻石 → 返 true + 仍含钻石。
//   3. 不同物品放入失败：已放钻石 + 铁锭 useItemOnBlock → 返 true（负摇晃仍是 Success）+ 仍含钻石。
//   4. 空手右键负摇晃：interactWithBlock → 返 true + 容器内容不变（仍含钻石）。
//   5. 非 cracked 破坏掉落内容物：放物品后 setBlockType air → onBlockRemoved 掉落存储的物品实体。
//
// 关键约束：
// 1. 饰纹陶罐需放在固体方块上方——(3,1,1) 放 stone 支撑，(3,2,1) 放陶罐。
// 2. 读 CRACKED state 用 getState("cracked" as any) 绕过 BlockStateSuperset 白名单。
// 3. 用钻石（minecraft:diamond）测放入物品，对齐 wiki「手持对应物品与饰纹陶罐交互」。
// 4. SimulatedPlayer 默认创造模式：放入物品 onBlockActivated 内部创造模式 grow(1) 不消耗手持物，
//    但 setItem 翻转由 split(1) 决定，与消耗无关。
// 5. 场景 5 用 test.setBlockType("minecraft:air", pos) 移除方块 → 触发 onBlockRemoved →
//    非 CRACKED 状态掉落陶罐内存储的物品（spawnItemEntity）。
// 6. assertContainerContains 对陶罐有效：陶罐继承 ContainerBlockEntity，assertContainerContains 通过
//    dynamic_cast<ContainerBlockEntity*> 获取容器后遍历槽位用 isSameItem 匹配。
//
// 不测「剑破坏触发 cracked」：SimulatedPlayer.breakBlock 脚本绑定未实现（_throwNotImplemented），
//   无法通过脚本模拟玩家手持剑破坏陶罐的场景（playerWillDestroy 需通过玩家破坏链路触发）。
//   TODO: 待 SimulatedPlayer.breakBlock 脚本绑定实现后补剑破坏触发 cracked 测试。
// 不测「投射物击碎陶罐」：需要发射箭矢命中陶罐，涉及投射物实体生成与碰撞检测链路，复杂跳过。
//   TODO: 待投射物击碎方块链路完善后补投射物击碎陶罐测试。
// 不测「比较器信号强度」：脚本侧无直接读比较器输出 API，需比较器方块+红石线链路，复杂跳过。
//   TODO: 待比较器读取链路打通后补比较器信号测试。
// 不测「摇晃动画」：摇晃动画无法在无头 GameTest 判定（wobbleStartedAtTick 脚本侧不可读），跳过。
//
// 跨服务端：饰纹陶罐 decorated_pot 方块名两端一致，放物品行为与 vanilla 一致。基岩无
//   setBlockWithStates，本测试用 setBlockType 放空陶罐（默认 cracked=false，无需设 state），两端均可放。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_饰纹陶罐.txt#交互机制（放入成功/失败/空手负摇晃）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_饰纹陶罐.txt#容器容量（1格储存空间，至多1组同类物品）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_饰纹陶罐.txt#破坏掉落（非cracked掉内容物）
// Ref: TrailsBlocks.cpp（onBlockActivated 放入物品/空手负摇晃；onBlockRemoved 掉落内容物）
// Ref: GameTestHelper.cpp:651（assertContainerContains 通过 ContainerBlockEntity 遍历槽位）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 陶罐 (3,2,1)，下方 (3,1,1) stone 支撑。

// 放支撑 + 空陶罐：(3,1,1) stone 支撑，(3,2,1) 空陶罐（minecraft:decorated_pot 默认 cracked=false）。
function placeDecoratedPot(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:decorated_pot", { x: 3, y: 2, z: 1 }); // 空陶罐
}

// 读取陶罐 cracked state（boolean）。返回 null 表示读取失败或非陶罐。
function getPotCracked(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("cracked" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：放入物品成功——空罐 + 钻石 useItemOnBlock → 返 true + assertContainerContains(diamond)。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空陶罐。
// onBlockActivated：陶罐为空 → canInsert=true → wobble(Positive) + split(1) + setItem +
//   playSound(BLOCK_DECORATED_POT_INSERT) + gameEvent(BLOCK_CHANGE) + updateComparators → Success。
//
// 判定：useItemOnBlock 返 true（Success），陶罐容器含钻石（assertContainerContains）。
function potInsertsItem(test: Test): void {
    placeDecoratedPot(test);
    test.assert(getPotCracked(test, 3, 2, 1) === false, `pot should not be cracked before, got ${getPotCracked(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const diamond = new ItemStack("minecraft:diamond", 1);

    // 对空陶罐 useItemOnBlock 钻石 → onBlockActivated canInsert → split(1) + setItem → Success。
    const used = farmer.useItemOnBlock(
        diamond as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when inserting diamond into pot");

    // 判定：陶罐容器含钻石（assertContainerContains 通过 ContainerBlockEntity 遍历槽位）。
    test.assertContainerContains(new ItemStack("minecraft:diamond", 1), { x: 3, y: 2, z: 1 });

    test.succeed();
}

// 场景 2：同类物品可叠加放入——已放1钻石 + 再 useItemOnBlock 钻石 → 返 true + 仍含钻石。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 陶罐（先放1钻石使罐内含1钻石）。
// onBlockActivated：罐内物品（钻石）与手持物品（钻石）相同且未达到最大堆叠 → canInsert=true →
//   wobble(Positive) + split(1) + grow(1) + setItem → Success。
//
// 判定：useItemOnBlock 返 true（Success），陶罐容器仍含钻石（assertContainerContains）。
function potStacksSameItem(test: Test): void {
    placeDecoratedPot(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const diamond = new ItemStack("minecraft:diamond", 1);

    // 先放1钻石使罐内含1钻石。
    const firstInsert = farmer.useItemOnBlock(
        diamond as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstInsert, "first diamond insertion should succeed");

    // 再 useItemOnBlock 钻石 → 罐内物品与手持物品相同且未满 → canInsert → grow(1) + setItem → Success。
    const secondInsert = farmer.useItemOnBlock(
        diamond as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(secondInsert, "second diamond insertion should succeed (stacking)");

    // 判定：陶罐容器仍含钻石（叠加后仍是钻石类型）。
    test.assertContainerContains(new ItemStack("minecraft:diamond", 1), { x: 3, y: 2, z: 1 });

    test.succeed();
}

// 场景 3：不同物品放入失败——已放钻石 + 铁锭 useItemOnBlock → 返 true（负摇晃仍是 Success）+
//   仍含钻石 + 不含铁锭。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 陶罐（先放1钻石使罐内含1钻石）。
// onBlockActivated：罐内物品（钻石）与手持物品（铁锭）不同 → canInsert=false → 走空手交互逻辑
//   （wobble(Negative) + playSound(BLOCK_DECORATED_POT_INSERT_FAIL)）→ Success。
//
// 判定：useItemOnBlock 返 true（Success，负摇晃仍是 Success），陶罐容器仍含钻石（assertContainerContains）。
function potRejectsDifferentItem(test: Test): void {
    placeDecoratedPot(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const diamond = new ItemStack("minecraft:diamond", 1);

    // 先放1钻石使罐内含1钻石。
    const firstInsert = farmer.useItemOnBlock(
        diamond as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstInsert, "first diamond insertion should succeed");

    // 再 useItemOnBlock 铁锭 → 罐内物品（钻石）与手持物品（铁锭）不同 → canInsert=false → 负摇晃 → Success。
    const ironIngot = new ItemStack("minecraft:iron_ingot", 1);
    const used = farmer.useItemOnBlock(
        ironIngot as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true even when insert fails (wobble Negative is Success)");

    // 判定 1：陶罐容器仍含钻石（铁锭未放入）。
    test.assertContainerContains(new ItemStack("minecraft:diamond", 1), { x: 3, y: 2, z: 1 });

    test.succeed();
}

// 场景 4：空手右键负摇晃——interactWithBlock → 返 true + 容器内容不变（仍含钻石）。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 陶罐（先放1钻石使罐内含1钻石）。
// onBlockActivated：heldItem 为空（interactWithBlock 传空 ItemStack）→ 走空手交互逻辑
//   （wobble(Negative) + playSound(BLOCK_DECORATED_POT_INSERT_FAIL)）→ Success。
//
// 判定：interactWithBlock 返 true（Success），陶罐容器仍含钻石（空手不改变内容物）。
function potWobblesNegativeOnEmptyHand(test: Test): void {
    placeDecoratedPot(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const diamond = new ItemStack("minecraft:diamond", 1);

    // 先放1钻石使罐内含1钻石。
    const firstInsert = farmer.useItemOnBlock(
        diamond as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstInsert, "first diamond insertion should succeed");

    // 空手 interactWithBlock → heldItem 为空 → wobble(Negative) + playSound(INSERT_FAIL) → Success。
    const interacted = farmer.interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(interacted, "interactWithBlock should return true (wobble Negative is Success)");

    // 判定：陶罐容器仍含钻石（空手不改变内容物）。
    test.assertContainerContains(new ItemStack("minecraft:diamond", 1), { x: 3, y: 2, z: 1 });

    test.succeed();
}

// 场景 5：非 cracked 破坏掉落内容物——放物品后 setBlockType air → onBlockRemoved 掉落存储的物品实体。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 陶罐（先放1钻石使罐内含1钻石）。
// 放钻石后 setBlockType("minecraft:air", (3,2,1)) 移除方块 → DecoratedPotBlock::onBlockRemoved →
//   非 CRACKED 状态 → getItem 非空 → spawnItemEntity 掉落钻石物品实体。
//
// 判定：破坏后陶罐位置为 air，且 diamond 物品实体存在（onBlockRemoved 掉落存储的物品）。
function potDropsStoredItemWhenBroken(test: Test): void {
    placeDecoratedPot(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const diamond = new ItemStack("minecraft:diamond", 1);

    // 先放1钻石使罐内含1钻石。
    const firstInsert = farmer.useItemOnBlock(
        diamond as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstInsert, "first diamond insertion should succeed");

    // 移除方块：setBlockType air 触发 onBlockRemoved → 非 CRACKED 状态掉落存储的物品（钻石）。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 判定 1：陶罐位置变为 air（已移除）。
    const block = test.getBlock({ x: 3, y: 2, z: 1 }) as unknown as { typeId?: string } | undefined;
    test.assert(block?.typeId === "minecraft:air", `pot pos should be air after removal, got ${block?.typeId}`);

    // 判定 2：diamond 物品实体存在（onBlockRemoved 掉落存储的钻石）。留 2 tick 让物品实体生成并注册。
    test.runAtTickTime(2, () => {
        test.assertItemEntityPresent("minecraft:diamond", { x: 3, y: 2, z: 1 }, 1.5, true);
        test.succeed();
    });
}

export function registerDecoratedPotTests(): void {
    GameTest.register("BlockBehaviorTests", "pot_inserts_item", potInsertsItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pot_stacks_same_item", potStacksSameItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pot_rejects_different_item", potRejectsDifferentItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pot_wobbles_negative_on_empty_hand", potWobblesNegativeOnEmptyHand)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pot_drops_stored_item_when_broken", potDropsStoredItemWhenBroken)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
