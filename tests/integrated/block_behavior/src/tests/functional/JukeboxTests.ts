// 唱片机放唱片/取唱片/破坏掉落行为 GameTest。
//
// wiki tech_唱片机.txt#播放唱片：对空的唱片机使用音乐唱片会将该音乐唱片插入唱片机并播放；
//   对着已经装有音乐唱片的唱片机交互将会弹出其中的音乐唱片并停止播放。
//   唱片机被破坏后会掉落自身和内容物（已插入的唱片随破坏掉落）。
//   非音乐唱片物品对唱片机使用不触发放入（onBlockActivated 返 Pass）。
//
// C++ 链路：JukeboxBlock（JukeboxBlock.cpp）有 HAS_RECORD state（默认 false）。
//   - onBlockActivated（JukeboxBlock.cpp:107-172）：
//     · HAS_RECORD==true（hasRecord）→ 取出唱片：stopPlaying + setRecord(EMPTY) +
//       setBlockState HAS_RECORD=false + 上方掉落唱片实体 → return Consume（不检查手持物）。
//     · HAS_RECORD==false → 取手持物，heldItem.getItem()->isMusicDisc() 判定 → setRecord(拷贝count=1) +
//       setBlockState HAS_RECORD=true + 非创造模式 shrink(1) → return Consume。
//     · 非唱片物品 → return Pass。
//   - onBlockRemoved（:174-195）：stopPlaying + getRecord 非空则掉落唱片实体（方块中心）。
//   - getComparatorInputOverride：从 JukeboxEntity 取 getComparatorSignal（唱片信号强度，13→1）。
//   - MusicDiscItem::isMusicDisc() 返 true；music_disc_13 构造信号强度=1（Items.cpp:5011-5014）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。唱片机 onBlockActivated 放/取唱片返回 Consume，useItemOnBlock 判定
//   isSuccess||isConsume 短路返 true；非唱片物品返 Pass 后 fallback（stick 非 BlockItem，onItemUse
//   默认 Pass）→ useItemOnBlock 返 false。useItemOnBlock 调 onBlockActivated 前把 stack 设到主手
//   选中槽，使 onBlockActivated 的 player.getHeldItem(hand) 读到唱片。
//
// 测试覆盖（4 个场景，覆盖 wiki 放唱片/取唱片/非唱片不触发/破坏掉落核心行为，可跨服务端对比）：
//   1. 放唱片：空唱片机 + music_disc_13 useItemOnBlock → has_record=true，返 true（Consume）。
//   2. 非唱片不触发：空唱片机 + 木棍 useItemOnBlock → has_record=false，返 false。
//   3. 取唱片：已有唱片 + 木棍 useItemOnBlock → has_record=false + 唱片物品实体掉落。
//   4. 破坏掉落：放唱片后破坏唱片机 → onBlockRemoved 掉落 music_disc_13 物品实体。
//
// 关键约束：
// 1. 唱片机是完整方块，无需支撑——直接 (3,2,1) 放唱片机（minecraft:jukebox 默认 has_record=false）。
// 2. 读 HAS_RECORD state 用 getState("has_record" as any) 绕过 BlockStateSuperset 白名单。
// 3. 用 music_disc_13（首张唱片，信号强度 1，两端一致）测放/取/破坏，对齐 wiki「使用音乐唱片」。
// 4. 判定「非唱片不触发」用木棍（普通 Item，非 BlockItem）——避免 BlockItem onItemUse 放置方块返
//    Success/Consume 误判。
// 5. SimulatedPlayer 默认创造模式：放唱片 onBlockActivated 内部 `if(!isCreative()) shrink(1)`，创造
//    跳过消耗，但 has_record 翻转由 setRecord+setBlockState 决定，与消耗无关。
// 6. 取唱片场景手持木棍：onBlockActivated hasRecord 分支优先取出（不检查手持物），返 Consume。
//    取出无消耗检查，唱片实体必掉落（spawnItemEntity）。
// 7. 破坏场景用 test.setBlockType("minecraft:air", pos) → onBlockRemoved → 掉落唱片实体。
//
// 不测「比较器信号强度」：脚本侧无直接读比较器输出 API，需比较器方块+红石线链路，复杂跳过。
//   TODO: 待比较器读取链路打通后补 music_disc_13 比较器信号=1 测试。
// 不测「播放音乐/停止」：音频播放无法在无头 GameTest 判定，跳过。
//
// 跨服务端：唱片机 jukebox 方块名两端一致，has_record state 行为与 vanilla 一致。基岩无
//   setBlockWithStates，本测试用 setBlockType 放空唱片机（默认 has_record=false），两端均可放。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_唱片机.txt#播放唱片（放入/弹出/破坏掉落）
// Ref: JukeboxBlock.cpp（onBlockActivated 放/取唱片返 Consume；onBlockRemoved 掉落唱片）
// Ref: Items.cpp:5011-5014（music_disc_13 信号强度=1，isMusicDisc=true）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 唱片机 (3,2,1)（完整方块，无需支撑）。

// 读取唱片机 has_record state（boolean）。返回 null 表示读取失败或非唱片机。
function getJukeboxHasRecord(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("has_record" as any);
    return typeof value === "boolean" ? value : null;
}

// 放空唱片机：(3,2,1) 唱片机（minecraft:jukebox 默认 has_record=false）。
function placeJukebox(test: Test): void {
    test.setBlockType("minecraft:jukebox", { x: 3, y: 2, z: 1 }); // 空唱片机
}

// 场景 1：放唱片——空唱片机 + music_disc_13 useItemOnBlock → has_record=true，返 true。
//
// 布局：(3,2,1) 空唱片机。
// onBlockActivated HAS_RECORD==false → 取手持 music_disc_13 → isMusicDisc true → setRecord +
// setBlockState HAS_RECORD=true + 创造跳过 shrink → return Consume。
//
// 判定：useItemOnBlock 返 true（Consume），has_record === true（setBlockState 翻转）。
function jukeboxInsertsMusicDisc(test: Test): void {
    placeJukebox(test);
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === false, `jukebox should have no record before, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const disc = new ItemStack("minecraft:music_disc_13", 1);

    // 对空唱片机 useItemOnBlock music_disc_13 → onBlockActivated isMusicDisc → setRecord + HAS_RECORD=true → Consume。
    const used = farmer.useItemOnBlock(
        disc as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when inserting music_disc_13 into jukebox");

    // 判定：has_record === true（唱片机已插入唱片）。
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === true, `jukebox has_record should be true after inserting disc, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：非唱片不触发——空唱片机 + 木棍 useItemOnBlock → has_record=false，返 false。
//
// 布局：(3,2,1) 空唱片机。
// onBlockActivated HAS_RECORD==false → 取手持木棍 → isMusicDisc false → return Pass。
// Pass → fallback Item.useOn（木棍普通 Item，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//
// 注意：不能用石头等 BlockItem 测「非唱片不触发」——BlockItem::onItemUse 会尝试在唱片机上方放置
// 该方块返回 Success，与「放入唱片」无关，会误判为"触发了"。木棍是普通 Item（非 BlockItem）。
//
// 判定：useItemOnBlock 返 false（未触发放入），has_record === false（唱片机仍空）。
function jukeboxIgnoresNonDiscItem(test: Test): void {
    placeJukebox(test);
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === false, `jukebox should have no record before, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对空唱片机 useItemOnBlock 木棍 → isMusicDisc false → Pass → fallback（木棍无 onItemUse 行为）→ false。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false for non-disc item (stick), got ${used}`);

    // 判定：has_record === false（木棍非唱片，唱片机仍空）。
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === false, `jukebox has_record should remain false for non-disc item, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：取唱片——已有唱片 + 木棍 useItemOnBlock → has_record=false + 唱片物品实体掉落。
//
// 布局：(3,2,1) 唱片机（先放 music_disc_13 使 has_record=true）。
// onBlockActivated HAS_RECORD==true（hasRecord）→ 取出：stopPlaying + setRecord(EMPTY) +
// setBlockState HAS_RECORD=false + 上方掉落唱片实体 → return Consume（不检查手持物，手持木棍亦可触发）。
//
// 判定：useItemOnBlock 返 true（Consume），has_record === false（取出后翻转），music_disc_13 物品实体存在。
function jukeboxEjectsDiscWhenAlreadyHasRecord(test: Test): void {
    placeJukebox(test);

    // 先放 music_disc_13 使唱片机 has_record=true。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const disc = new ItemStack("minecraft:music_disc_13", 1);
    const insertResult = farmer.useItemOnBlock(
        disc as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(insertResult, "disc insertion should succeed");
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === true, `jukebox should have record before ejection, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    // 已有唱片的唱片机 useItemOnBlock 木棍 → onBlockActivated hasRecord 分支取出 + 掉落 → Consume。
    // 手持木棍（非唱片）：hasRecord 分支优先取出，不检查手持物是否为唱片。
    const stick = new ItemStack("minecraft:stick", 1);
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when ejecting disc (Consume)");

    // 判定 1：has_record === false（取出后翻转）。
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === false, `jukebox has_record should be false after ejection, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    // 判定 2：music_disc_13 物品实体存在（onBlockActivated 上方掉落唱片）。留 2 tick 让实体注册。
    test.runAtTickTime(2, () => {
        test.assertItemEntityPresent("minecraft:music_disc_13", { x: 3, y: 2, z: 1 }, 1.5, true);
        test.succeed();
    });
}

// 场景 4：破坏掉落——放唱片后破坏唱片机 → onBlockRemoved 掉落 music_disc_13 物品实体。
//
// 布局：(3,2,1) 唱片机（先放 music_disc_13 使 has_record=true）。
// 放唱片后 setBlockType("minecraft:air", (3,2,1)) 破坏唱片机 → JukeboxBlock::onBlockRemoved →
// stopPlaying + getRecord 非空则掉落唱片实体（方块中心）。
//
// 判定：破坏后唱片机位置为 air，且 music_disc_13 物品实体存在（onBlockRemoved 掉落）。
function jukeboxDropsDiscWhenBroken(test: Test): void {
    placeJukebox(test);

    // 先放 music_disc_13 使唱片机 has_record=true。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const disc = new ItemStack("minecraft:music_disc_13", 1);
    const insertResult = farmer.useItemOnBlock(
        disc as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(insertResult, "disc insertion should succeed");
    test.assert(getJukeboxHasRecord(test, 3, 2, 1) === true, `jukebox should have record before breaking, got ${getJukeboxHasRecord(test, 3, 2, 1)}`);

    // 破坏唱片机：setBlockType air 触发 onBlockRemoved → 掉落唱片实体。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 判定 1：唱片机位置变为 air（已破坏）。
    const block = test.getBlock({ x: 3, y: 2, z: 1 }) as unknown as { typeId?: string } | undefined;
    test.assert(block?.typeId === "minecraft:air", `jukebox pos should be air after breaking, got ${block?.typeId}`);

    // 判定 2：music_disc_13 物品实体存在（onBlockRemoved 掉落唱片）。留 2 tick 让实体注册。
    test.runAtTickTime(2, () => {
        test.assertItemEntityPresent("minecraft:music_disc_13", { x: 3, y: 2, z: 1 }, 1.5, true);
        test.succeed();
    });
}

export function registerJukeboxTests(): void {
    GameTest.register("BlockBehaviorTests", "jukebox_inserts_music_disc", jukeboxInsertsMusicDisc)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "jukebox_ignores_non_disc_item", jukeboxIgnoresNonDiscItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "jukebox_ejects_disc_when_already_has_record", jukeboxEjectsDiscWhenAlreadyHasRecord)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "jukebox_drops_disc_when_broken", jukeboxDropsDiscWhenBroken)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
