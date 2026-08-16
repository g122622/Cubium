// 讲台放书与阅读行为 GameTest。
//
// wiki tech_讲台.txt#用途：讲台可存放书与笔或成书供玩家阅读。
//   - 拿着书与笔/成书对空的讲台按下使用键 → 把书放在讲台上（HAS_BOOK=true）。
//   - 已有书的讲台再按使用键 → 打开并阅读这部书（openContainer），不会重复放书。
//   - 非书物品（如木棍）对讲台使用 → 不触发放书（onBlockActivated 返 Pass）。
//   - 讲台被破坏后会掉落自身和内容物（已放的书随破坏掉落为物品实体）。
//
// C++ 链路：LecternBlock（LecternBlock.cpp）有 HAS_BOOK/POWERED/HORIZONTAL_FACING state。
//   - onBlockActivated（LecternBlock.cpp:214-260）：
//     · HAS_BOOK==true → 取 LecternEntity 调 openContainer + 统计 → return Success（打开阅读，不消耗手持物）。
//     · HAS_BOOK==false → 取手持物，isLecternBookItem 判定 → tryPlaceBook 成功则 shrink(1)+音效 → Success；
//       非书物品或 tryPlaceBook 失败 → return Pass。
//   - tryPlaceBook（:296-328）：HAS_BOOK 已 true 返 false；否则 LecternEntity.setBook + setHasBook(true)。
//   - onBlockRemoved（:262-275）：HAS_BOOK 时 _dropBook（removeBook + ItemDropHelper 上方掉落）。
//   - isLecternBookItem 接受 book/written_book/writable_book/enchanted_book（Cubium 扩展，vanilla 仅书与笔/成书）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。讲台 onBlockActivated 处理放书/阅读返回 Success 短路；非书物品返 Pass 后
//   fallback（stick 非 BlockItem，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//   useItemOnBlock 调 onBlockActivated 前把 stack 设到主手选中槽，使 onBlockActivated 的
//   player.getHeldItem(hand) 读到书。
//
// 测试覆盖（4 个场景，覆盖 wiki 放书/已有书阅读/非书不触发/破坏掉书核心行为，可跨服务端对比）：
//   1. 成书放讲台：空讲台 + written_book useItemOnBlock → has_book=true。
//   2. 非书不触发：空讲台 + 木棍 useItemOnBlock → has_book=false，useItemOnBlock 返 false。
//   3. 已有书右键阅读：讲台已有书 + 书 useItemOnBlock → openContainer → 返 true，has_book 仍 true。
//   4. 破坏掉书：放书后破坏讲台 → 书物品实体掉落（onBlockRemoved _dropBook）。
//
// 关键约束：
// 1. 讲台需放在固体方块上方（与多数功能性方块一致）——(3,1,1) 放 stone 支撑，(3,2,1) 放讲台。
// 2. 读 HAS_BOOK state 用 getState("has_book" as any) 绕过 BlockStateSuperset 白名单。
// 3. 用 written_book（vanilla 讲台接受的成书）放书，对齐 wiki「拿着成书对空讲台按使用键」。
// 4. 判定「非书不触发」用木棍（普通 Item，非 BlockItem）——避免 BlockItem onItemUse 放置方块返 Success 误判。
// 5. SimulatedPlayer 默认创造模式：放书 onBlockActivated 内部 shrink(1) 作用于选中槽，创造模式是否
//    消耗不影响 has_book 判定（has_book 翻转由 tryPlaceBook→setHasBook 决定，与消耗无关）。
// 6. 场景 4 用 test.setBlockType("minecraft:air", pos) 破坏讲台 → 触发 onBlockRemoved → _dropBook
//    掉落书物品实体，用 assertItemEntityPresent 判定。
//
// 不测「翻页红石脉冲/比较器信号」：翻页需 GUI 交互（SimulatedPlayer 无翻页 API），跳过。
//   TODO: 待 SimulatedPlayer 补 lectern nextPage 脚本绑定后补翻页红石脉冲测试。
// 不测「村民认领工作站转职图书管理员」：涉村民 AI 寻路/认领链路，跳过。
//
// 跨服务端：讲台 lectern 方块名两端一致，放书 has_book state 行为与 vanilla 一致。注意基岩 BDS
//   has_book state 名可能为 has_book（与 JE 一致），放书行为两端可对比。基岩无 setBlockWithStates，
//   本测试用 setBlockType 放空讲台（默认 has_book=false，无需设 state），两端均可放。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_讲台.txt#用途（放书/已有书阅读/破坏掉落）
// Ref: LecternBlock.cpp（onBlockActivated 放书/阅读；tryPlaceBook；onBlockRemoved _dropBook）
// Ref: LecternEntity.cpp（setBook/hasBook；getTotalPages 回退 100 页；getComparatorSignal）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 讲台 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取讲台 has_book state（boolean）。返回 null 表示读取失败或非讲台。
function getLecternHasBook(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("has_book" as any);
    return typeof value === "boolean" ? value : null;
}

// 放支撑 + 空讲台：(3,1,1) stone 支撑，(3,2,1) 空讲台（minecraft:lectern 默认 has_book=false）。
function placeLectern(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:lectern", { x: 3, y: 2, z: 1 }); // 空讲台
}

// 场景 1：成书放讲台——空讲台 + written_book useItemOnBlock → has_book=true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空讲台。
// onBlockActivated HAS_BOOK==false → 取手持 written_book → isLecternBookItem true → tryPlaceBook
// （LecternEntity.setBook + setHasBook(true)）→ shrink(1) + 音效 → return Success。
//
// 判定：useItemOnBlock 返 true（Success），has_book === true（tryPlaceBook setHasBook(true)）。
function lecternPlacesWrittenBook(test: Test): void {
    placeLectern(test);
    test.assert(getLecternHasBook(test, 3, 2, 1) === false, `lectern should have no book before, got ${getLecternHasBook(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const book = new ItemStack("minecraft:written_book", 1);

    // 对空讲台 useItemOnBlock 成书 → onBlockActivated tryPlaceBook → setHasBook(true) → Success。
    const used = farmer.useItemOnBlock(
        book as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing written_book on lectern");

    // 判定：has_book === true（讲台已放入成书）。
    test.assert(getLecternHasBook(test, 3, 2, 1) === true, `lectern has_book should be true after placing book, got ${getLecternHasBook(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：非书不触发——空讲台 + 木棍 useItemOnBlock → has_book=false，useItemOnBlock 返 false。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空讲台。
// onBlockActivated HAS_BOOK==false → 取手持木棍 → isLecternBookItem false → return Pass。
// Pass → fallback Item.useOn（木棍普通 Item，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//
// 注意：不能用石头等 BlockItem 测「非书不触发」——BlockItem::onItemUse 会尝试在讲台上方放置该方块
// 返回 Success，与「放书」无关，会误判为"触发了"。木棍是普通 Item（非 BlockItem），onItemUse 默认 Pass。
//
// 判定：useItemOnBlock 返 false（未触发放书），has_book === false（讲台仍空）。
function lecternIgnoresNonBookItem(test: Test): void {
    placeLectern(test);
    test.assert(getLecternHasBook(test, 3, 2, 1) === false, `lectern should have no book before, got ${getLecternHasBook(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对空讲台 useItemOnBlock 木棍 → isLecternBookItem false → Pass → fallback（木棍无 onItemUse 行为）→ false。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false for non-book item (stick), got ${used}`);

    // 判定：has_book === false（木棍非书，讲台仍空）。
    test.assert(getLecternHasBook(test, 3, 2, 1) === false, `lectern has_book should remain false for non-book item, got ${getLecternHasBook(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：已有书右键阅读——讲台已有书 + 书 useItemOnBlock → openContainer → 返 true，has_book 仍 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 讲台（先放成书使 has_book=true）。
// onBlockActivated HAS_BOOK==true → 取 LecternEntity openContainer + 统计 → return Success（打开阅读，
// 不消耗手持物，不重复放书）。tryPlaceBook 不会被调用（HAS_BOOK 分支已 return）。
//
// 判定：useItemOnBlock 返 true（openContainer Success），has_book 仍 true（未取书）。
function lecternOpensBookWhenAlreadyHasBook(test: Test): void {
    placeLectern(test);

    // 先放一本成书使讲台 has_book=true（模拟场景 1 的放书过程）。
    const placer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "placer");
    const firstBook = new ItemStack("minecraft:written_book", 1);
    const placeResult = placer.useItemOnBlock(
        firstBook as unknown as Parameters<typeof placer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(placeResult, "first book placement should succeed");
    test.assert(getLecternHasBook(test, 3, 2, 1) === true, `lectern should have book after first placement, got ${getLecternHasBook(test, 3, 2, 1)}`);

    // 已有书的讲台再 useItemOnBlock 成书 → onBlockActivated HAS_BOOK 分支 openContainer → Success。
    const secondBook = new ItemStack("minecraft:written_book", 1);
    const used = placer.useItemOnBlock(
        secondBook as unknown as Parameters<typeof placer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when opening already-placed book (openContainer Success)");

    // 判定：has_book 仍 true（打开阅读不取书，原书仍在讲台）。
    test.assert(getLecternHasBook(test, 3, 2, 1) === true, `lectern has_book should remain true after opening (book not taken), got ${getLecternHasBook(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：破坏掉书——放书后破坏讲台 → 书物品实体掉落（onBlockRemoved _dropBook）。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 讲台（先放成书使 has_book=true）。
// 放书后 setBlockType("minecraft:air", (3,2,1)) 破坏讲台 → LecternBlock::onBlockRemoved → HAS_BOOK
// 时 _dropBook（LecternEntity.removeBook + ItemDropHelper 上方掉落书物品实体）。
//
// 判定：破坏后讲台位置为 air，且 written_book 物品实体存在（_dropBook 在讲台上方掉落书）。
function lecternDropsBookWhenBroken(test: Test): void {
    placeLectern(test);

    // 先放成书使讲台 has_book=true。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const book = new ItemStack("minecraft:written_book", 1);
    const placeResult = farmer.useItemOnBlock(
        book as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(placeResult, "book placement should succeed");
    test.assert(getLecternHasBook(test, 3, 2, 1) === true, `lectern should have book before breaking, got ${getLecternHasBook(test, 3, 2, 1)}`);

    // 破坏讲台：setBlockType air 触发 onBlockRemoved → _dropBook 掉落书物品实体。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 判定 1：讲台位置变为 air（已破坏）。
    const block = test.getBlock({ x: 3, y: 2, z: 1 }) as unknown as { typeId?: string } | undefined;
    test.assert(block?.typeId === "minecraft:air", `lectern pos should be air after breaking, got ${block?.typeId}`);

    // 判定 2：written_book 物品实体存在（_dropBook 在讲台上方掉落书，searchRadius 覆盖讲台区域）。
    // 留 2 tick 让物品实体生成并注册。
    test.runAtTickTime(2, () => {
        test.assertItemEntityPresent("minecraft:written_book", { x: 3, y: 2, z: 1 }, 1.5, true);
        test.succeed();
    });
}

export function registerLecternTests(): void {
    GameTest.register("BlockBehaviorTests", "lectern_places_written_book", lecternPlacesWrittenBook)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "lectern_ignores_non_book_item", lecternIgnoresNonBookItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "lectern_opens_book_when_already_has_book", lecternOpensBookWhenAlreadyHasBook)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "lectern_drops_book_when_broken", lecternDropsBookWhenBroken)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
