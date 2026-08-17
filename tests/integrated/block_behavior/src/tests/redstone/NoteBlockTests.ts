// 音符盒右键升调行为 GameTest。
//
// wiki tech_音符盒.txt#奏乐/音符：每次使用音符盒都会让其下一个发出音符的音高提高半音。按十二平均律，
//   每个乐器共两个完整八度（25 个音，note 0-24）。升至最高（note=24）后再次点击复位至 note=0（F♯3）。
//   即 note 循环 0→1→...→24→0。右键（使用键）触发升调；红石信号触发播放（不升调）；左键（attack）
//   仅播放不升调。
//
// C++ 链路：NoteBlock（redstone/NoteBlock.cpp）有 NOTE_0_24 + INSTRUMENT + POWERED state。
//   - onBlockActivated（本次补全）：cycleNote(note+1)%25
//     升半音 → setBlockState 写回 + triggerNote 播放新音高 → return Success。不检查手持物（空手/任意
//     物品右键均升调），不检查 mayBuild（useWithoutItem 无建造权限守卫）。
//     此前 NoteBlock 缺 onBlockActivated override（基类返 Pass），右键无法升调——生产缺失行为，已补全。
//   - cycleNote（:182）：(getNote+1) % NOTE_RANGE(25) 循环升半音。
//   - NOTE state 名 "note"（Properties.hpp NOTE_0_24 = IntegerProperty("note",0,24)）。
//   - triggerNote：根据下方方块材质决定乐器 + 播放音效/粒子（音效不可测，本组只断言 note state）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。音符盒 onBlockActivated 始终返 Success（不检查手持物/mayBuild），短路不 fallback。
//   用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，仅占位 useItemOnBlock 的
//   ItemStack 形参；useWithoutItem 是空手路径，Cubium useItemOnBlock 强制要 ItemStack，用 stick
//   等价模拟空手右键升调）。
//
// 测试覆盖（2 个场景，覆盖 wiki 右键升调 + 循环复位核心确定行为）：
//   1. 右键升调一次：放音符盒（note=0）+ stick useItemOnBlock → note=1，返 true。
//   2. 循环复位到 0：连续 25 次 useItemOnBlock（note 0→1→...→24→0）→ 第 25 次后 note=0（24 后复位）。
//
// 关键约束：
// 1. 音符盒完整方块（Material::ROCK），放 (3,2,1)（minecraft:note_block 默认 note=0, instrument=harp,
//    powered=false）。下方 (3,1,1) stone 支撑（非必需，音符盒无自毁，但贴近真实放置 + 与惯例一致）。
// 2. 读 note state 用 getState("note" as any) 绕过 BlockStateSuperset 白名单。
// 3. onBlockActivated 不检查手持物/mayBuild，用 stick 触发（stick 不被消耗，可重复使用）。
// 4. 升调 setBlockState(flags=3) 触发 neighborChanged，但无红石信号 isPowered 不变，note state 稳定可断言。
// 5. 循环场景 25 次 useItemOnBlock：note 0→1→...→24→0。第 25 次 (24+1)%25=0 复位。每次 new ItemStack
//    重新设入选中槽（防选中槽漂移，与中继器/重生锚范式一致）。
//
// 不测「乐器根据下方方块材质变化」：triggerNote 乐器选择依赖下方方块，但 instrument state 在放置时
//   由 getStateForPlacement/setInstrument 决定（Cubium 简化为默认 harp，未实现 setInstrument 上下文），
//   且音效不可测。跳过。TODO: 待 instrument state 上下文放置 + 音效可观测后补。
// 不测「红石信号触发播放」：涉红石传导 + triggerNote 音效（不可测），跳过。
// 不测「左键 attack 播放不升调」：SimulatedPlayer.attack 为 stub（MethodNotImplemented），不可测。
//   TODO: 待 attack API 就绪后补。
// 不测「上方方块决定乐器（NOTE_BLOCK_TOP_INSTRUMENTS）」：Cubium onBlockActivated 简化统一升调，
//   未实现 useItemOn 的 TOP_INSTRUMENTS+顶面 PASS 分支，跳过。TODO: 待标签/朝向守卫完善后补。
//
// 跨服务端：音符盒 note_block 方块名两端一致，note state 名两端一致，右键升调 0→1→...→24→0 循环
//   行为两端一致。基岩无 setBlockWithStates，本测试用 setBlockType 放默认 state（note=0），
//   两端均可放；右键升 note 行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_音符盒.txt#奏乐（每次使用提高半音，25 音循环复位）
// Ref: NoteBlock.cpp（onBlockActivated cycleNote+triggerNote→Success；cycleNote (note+1)%25）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 音符盒 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取音符盒 note state（number 0-24）。返回 null 表示读取失败或非音符盒。
// note state 名 "note"（Java 命名，见 Properties.hpp NOTE_0_24 = IntegerProperty("note",0,24)）。
function getNoteBlockNote(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("note" as any);
    return typeof value === "number" ? value : null;
}

// 放支撑 + 音符盒：(3,1,1) stone 支撑，(3,2,1) 音符盒（minecraft:note_block 默认 note=0, instrument=harp）。
function placeNoteBlock(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:note_block", { x: 3, y: 2, z: 1 }); // 音符盒 note=0
}

// 场景 1：右键升调一次——放音符盒（note=0）+ stick useItemOnBlock → note=1，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 音符盒 note=0。
// onBlockActivated：cycleNote (0+1)%25=1 → setBlockState 写回 + triggerNote 播放 → Success。
//
// 判定：useItemOnBlock 返 true（Success），note === 1（升调一次）。
function noteBlockPitchRaisesOnUse(test: Test): void {
    placeNoteBlock(test);
    test.assert(getNoteBlockNote(test, 3, 2, 1) === 0, `noteblock note should be 0 before, got ${getNoteBlockNote(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对音符盒 useItemOnBlock stick → onBlockActivated cycleNote (0+1)%25=1 → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when raising noteblock pitch");

    // 判定：note === 1（升调一次，提高半音）。
    test.assert(getNoteBlockNote(test, 3, 2, 1) === 1, `noteblock note should be 1 after one use, got ${getNoteBlockNote(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：循环复位到 0——连续 25 次 useItemOnBlock（note 0→1→...→24→0）→ 第 25 次后 note=0。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 音符盒 note=0（已放）。
// 每次右键 cycleNote (cur+1)%25：0→1→...→24→0。第 25 次 (24+1)%25=0 复位至起点（25 音循环）。
//
// 判定：25 次点击后 note === 0（24 后循环复位回 0）。
function noteBlockPitchCyclesBackToZeroOnUse(test: Test): void {
    placeNoteBlock(test);
    test.assert(getNoteBlockNote(test, 3, 2, 1) === 0, `noteblock note should be 0 before, got ${getNoteBlockNote(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // 连续 25 次右键升调：note 0→1→...→24→0。每次 new ItemStack 重新设入选中槽（防选中槽漂移）。
    // 第 i 次（1-based）后 note = i % 25：1→1, 2→2, ..., 24→24, 25→0（循环复位）。
    for (let i = 1; i <= 25; ++i) {
        const stick = new ItemStack("minecraft:stick", 1);
        const used = farmer.useItemOnBlock(
            stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on use #${i} (pitch cycle)`);
        const expected = i % 25; // 1..24, 25→0
        test.assert(getNoteBlockNote(test, 3, 2, 1) === expected, `noteblock note should be ${expected} after use #${i}, got ${getNoteBlockNote(test, 3, 2, 1)}`);
    }

    // 判定：note === 0（25 次循环后复位回起点 F♯3）。
    test.assert(getNoteBlockNote(test, 3, 2, 1) === 0, `noteblock note should cycle back to 0 after 25 uses, got ${getNoteBlockNote(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerNoteBlockTests(): void {
    GameTest.register("BlockBehaviorTests", "noteblock_pitch_raises_on_use", noteBlockPitchRaisesOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "noteblock_pitch_cycles_back_to_zero_on_use", noteBlockPitchCyclesBackToZeroOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
