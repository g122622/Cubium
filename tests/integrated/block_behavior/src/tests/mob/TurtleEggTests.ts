// 海龟蛋（turtle_egg）堆叠、沙子支撑判定与 state 读写行为 GameTest。
//
// wiki tech_海龟蛋.txt#用途/破坏/历史：
//   - 用途：海龟蛋可像普通固体方块放置，一个方块空间最多 4 个蛋（EGGS 1-4）。无需依附面，可悬空。
//   - 放置堆叠：手持海龟蛋物品对已有海龟蛋（非潜行 + eggs<4）右键 → 蛋数量递增（eggs+1），满 4 不再增
//     （wiki 历史 18w22a「玩家处在单个海龟蛋的碰撞箱内，尝试放置第二个蛋会失败。更多的蛋放置在同一
//     方块中时，会发出放置的音效。指向侧面的方块放置时，也可以使同一方块的蛋的数量增加」）。
//   - 孵化：须放在 sand/red_sand/suspicious_sand 上（BlockTags::SAND）；孵化经 3 阶段（HATCH 0-2），
//     随机刻 + 天体角度概率推进（非确定，不测）。
//   - 踩破：足够大生物（海龟除外）站立/落在蛋上每刻 1/100、1/3 概率破坏，逐个破碎（随机概率，不测）。
//   - 僵尸/尸壳/溺尸等主动寻路踩碎（AI 非确定，不测）。
//
// C++ 链路：TurtleEggBlock（mob/TurtleEggBlock.cpp）两个 state：
//   - EGGS_1_4（C++ 属性名 "eggs"，1-4，默认 1，Properties.hpp:940-943）。
//   - HATCH_0_2（C++ 属性名 "hatch"，0-2，默认 0，Properties.hpp:971-974）。
//   - getStateForPlacement（:107-123）：placementPos 已有同类型蛋且 eggs<4 → eggs+1；否则 defaultState()。
//   - isValidPosition（:125-139）：下方须 BlockTags::SAND()（含 sand/red_sand/suspicious_sand，
//     BlockTags.cpp:1236-1239，对齐 wiki「沙子/红沙/可疑的沙子」）。
//   - isReplaceable（本提交新增，对齐 vanilla TurtleEggBlock.canBeReplaced TurtleEggBlock.java:147-152）：
//     手持海龟蛋物品 + 非潜行 + eggs<4 → true（已有蛋可被「替换」即堆叠）。修复前缺此 override，
//     基类 isReplaceable 返 m_isReplaceable（海龟蛋注册无 .replaceable()，故 false），点击已有蛋顶面
//     _canReplace=false → placementPos=上方 air → getStateForPlacement 检测 air → 新蛋落上方不堆叠，
//     与 vanilla「一个方块空间最多 4 个蛋」偏差。修复后同 CandleBlock::isReplaceable 同构。
//   - 未 override updatePostPlacement（支撑失效不自毁，走基类返 state）——故沙子上放蛋后移除沙子，
//     蛋不会自毁（与火把/大滴叶支撑自毁范式不同，海龟蛋 wiki 明言「无需依附面，可悬空」）。
//   - randomTick 孵化（:160-237，随机+天体角度，不测）、onFallenUpon/onEntityWalk 踩破（:239-336，
//     随机概率，不测）。
//   - 物品注册：Items.cpp:3780-3781 registerBlockBackedItem(TURTLE_EGG, "turtle_egg")，
//     BlockItemRegistry.cpp:1186 registerSimpleBlock(TURTLE_EGG, "turtle_egg")。物品已注册，可放置。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 turtle_egg 物品点击
//   方块 → onBlockActivated 基类 Pass → fallback Item.useOn → BlockItem::onItemUse → tryPlace →
//   构造 BlockItemUseContext（placementPos 由 _canReplace 决定：被点击方块可替换→被点击位置，否则
//   相邻位置）。对已有海龟蛋（eggs<4，手持 turtle_egg，非潜行），isReplaceable=true → placementPos=
//   已有蛋格 → getStateForPlacement eggs+1 → setBlockState 写回（堆叠）。创造模式海龟蛋不消耗。
//
// 测试覆盖（3 个场景，覆盖 wiki 堆叠 + 沙子支撑判定 + state 读写核心确定行为）：
//   1. 蛋堆叠递增：sand 支撑 + 1 蛋 → 连续 3 次 useItemOnBlock turtle_egg → eggs 1→2→3→4（验证
//      isReplaceable 修复 + getStateForPlacement 堆叠）。
//   2. 沙子支撑判定：sand 下方放蛋成功 / stone 下方放蛋失败（isValidPosition BlockTags::SAND）。
//   3. eggs/hatch 双 state 读写：setBlockWithStates 预置 eggs=3,hatch=1 → getState 可读。
//
// 关键约束：
// 1. 支撑用 sand（BlockTags::SAND 成员，isValidPosition 通过）。glass_pit 底层 y=1 是 glass，需先
//    setBlockType sand 覆盖 (3,1,1) 作支撑。海龟蛋放 (3,2,1)（sand 上方）。
// 2. 场景 1 堆叠用 useItemOnBlock(turtle_egg_item, {3,2,1}, Direction.Up) 点击已有蛋顶面。SimulatedPlayer
//    创造模式默认非潜行（isSneaking=false），isReplaceable 满足 → placementPos=(3,2,1) 已有蛋格 →
//    getStateForPlacement eggs+1。创造模式海龟蛋不消耗，可连续 3 次堆叠到 eggs=4。
// 3. 场景 2 用 useItemOnBlock 放置判定：sand 支撑 → isValidPosition true → 放置成功返 true，(3,2,1)
//    是 turtle_egg；stone 支撑 → isValidPosition false → 放置失败返 false，(3,2,1) 仍 air。用新
//    SimulatedPlayer（避免选中槽残留 turtle_egg 影响）。
// 4. 场景 3 用 setBlockWithStates 预置 eggs=3,hatch=1（绕过物品放置，直接写 state）。getState 读
//    "eggs"/"hatch"（C++ 内部属性名）。验证双 state 可读写。
// 5. 读 eggs/hatch 用 getState("eggs"/"hatch" as any)（C++ 内部属性名，IntegerProperty 序列化为数字）。
// 6. 海龟蛋注册 noCollision().notSolid()（NaturalBlocks.cpp:262-264），但 isValidPosition 不依赖碰撞，
//    仅查下方 BlockTags::SAND。stone 下方放蛋失败是 isValidPosition 拒绝，与碰撞无关。
//
// 不测「孵化」：randomTick 随机刻 + 天体角度概率（_canGrow 1/500 或黎明 100%），非确定，跳过。
//   TODO: 可补 turtle_egg_hatches_on_sand_at_dawn（需控制 dayTime 到黎明窗口 + 多次重试，非确定）。
// 不测「踩破」：onFallenUpon/onEntityWalk 随机概率（1/3、1/100），非确定，跳过。
//   TODO: 可补 turtle_egg_trampled_by_mob（需多次重试取统计，非确定）。
// 不测「僵尸寻路踩碎」：AI 寻路非确定，跳过。
// 不测「支撑失效自毁」：海龟蛋未 override updatePostPlacement，wiki 明言「无需依附面，可悬空」，
//   移除下方沙子蛋不自毁（与火把/大滴叶范式不同），无自毁行为可测。
//
// 跨服务端：turtle_egg 方块名两端一致。eggs/hatch state 名两端一致（C++ 内部名）。
//   蛋堆叠（isReplaceable + getStateForPlacement eggs+1）+ 沙子支撑判定（BlockTags::SAND）+
//   state 读写行为两端与 vanilla 一致。isReplaceable 是本提交修复的 Cubium 偏差（修复前海龟蛋无法堆叠，
//   基岩侧海龟蛋堆叠行为正常），修复后两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海龟蛋.txt#用途（一个方块空间最多 4 个蛋，悬空放置）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海龟蛋.txt#孵化（沙子/红沙/可疑的沙子上孵化）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海龟蛋.txt#历史 18w22a（堆叠 + 侧面放置使蛋数量增加）
// Ref: TurtleEggBlock.cpp（getStateForPlacement 堆叠 / isValidPosition BlockTags::SAND / isReplaceable 修复）
// Ref: TurtleEggBlock.java:147-152（vanilla canBeReplaced：手持蛋物品+非潜行+eggs<4 → true 堆叠）
// Ref: Properties.hpp:940-974（EGGS_1_4 "eggs" 1-4 / HATCH_0_2 "hatch" 0-2）
// Ref: BlockTags.cpp:1236-1239（SAND 标签含 sand/red_sand/suspicious_sand）
// Ref: CandleTests.ts（堆叠范式：useItemOnBlock 物品点击已有方块顶面，isReplaceable+getStateForPlacement 递增）
// Ref: TorchTests.ts（useItemOnBlock 放置 + isValidPosition 支撑判定范式）
// Ref: BigDripleafTests.ts（setBlockWithStates 预置 state + getState 读 C++ 内部属性名范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/3：海龟蛋 (3,2,1)，下方支撑 (3,1,1)（sand 或 stone）。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BigDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 海龟蛋 eggs state（数字 1-4）。返回 null 表示失败或非海龟蛋。
// 注意：EGGS_1_4() 的 C++ 属性名为 "eggs"（IntegerProperty::create("eggs", 1, 4)），getState 按内部名匹配。
function getEggs(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("eggs" as any);
    return typeof value === "number" ? value : null;
}

// 读取 (x,y,z) 海龟蛋 hatch state（数字 0-2）。返回 null 表示失败或非海龟蛋。
// 注意：HATCH_0_2() 的 C++ 属性名为 "hatch"（IntegerProperty::create("hatch", 0, 2)）。
function getHatch(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("hatch" as any);
    return typeof value === "number" ? value : null;
}

// 放置测试基础结构：sand 支撑 + turtle_egg 海龟蛋（eggs=1，hatch=0 默认）。
// sand 在 BlockTags::SAND 标签内，isValidPosition 通过。先支撑后蛋（避免蛋放置时 isValidPosition 失败）。
function placeEggOnSand(test: Test): void {
    test.setBlockType("minecraft:sand", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:turtle_egg", { x: 3, y: 2, z: 1 });
}

// 场景 1：蛋堆叠递增——sand 支撑 + 1 蛋 → 连续 3 次 useItemOnBlock turtle_egg → eggs 1→2→3→4。
//
// 布局：sand 支撑 (3,1,1) + turtle_egg (3,2,1) eggs=1（已放）。
// 每次对 (3,2,1) useItemOnBlock turtle_egg 物品（BlockItem.onItemUse 放置）→ BlockItemUseContext:
//   _canReplace((3,2,1)) 调 TurtleEggBlock::isReplaceable（修复后）：手持 turtle_egg 物品 + 非潜行
//   （SimulatedPlayer 创造默认非潜行）+ eggs<4 → true → placementPos=(3,2,1) 已有蛋格 →
//   getStateForPlacement 检测已有蛋 eggs+1 → setBlockState 写回（堆叠）。创造模式海龟蛋不消耗，连续 3
//   次堆叠到 eggs=4。
//
// 判定：3 次堆叠后 eggs === 4（满堆叠）。
//
// 此场景验证 TurtleEggBlock::isReplaceable 修复生效：修复前缺 override，基类 isReplaceable 返 false，
//   点击已有蛋顶面 placementPos=上方 air → getStateForPlacement 检测 air → 新蛋落 (3,3,1) 上方而非
//   堆叠，(3,2,1) eggs 恒 1，本场景 eggs===4 断言失败。修复后 isReplaceable=true → placementPos=
//   (3,2,1) → 堆叠生效，eggs 递增到 4。与 CandleTests 堆叠范式同构。
function turtleEggStacksUpToFour(test: Test): void {
    placeEggOnSand(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:turtle_egg", `turtle_egg should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getEggs(test, 3, 2, 1) === 1, `eggs should be 1 before, got ${getEggs(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // 连续 3 次 useItemOnBlock turtle_egg 物品堆叠：eggs 1→2→3→4。
    for (let i = 0; i < 3; ++i) {
        const eggItem = new ItemStack("minecraft:turtle_egg", 1);
        const used = farmer.useItemOnBlock(
            eggItem as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on stack #${i + 1} (turtle egg placement)`);
        const expected = i + 2; // 1→2→3→4
        test.assert(getEggs(test, 3, 2, 1) === expected, `eggs should be ${expected} after stack #${i + 1}, got ${getEggs(test, 3, 2, 1)}`);
    }

    // 判定：eggs === 4（满堆叠）。
    test.assert(getEggs(test, 3, 2, 1) === 4, `eggs should be 4 (max) after stacking, got ${getEggs(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：沙子支撑判定——sand 下方放蛋成功 / stone 下方放蛋失败（isValidPosition BlockTags::SAND）。
//
// 布局 A（成功）：(3,1,1) sand 支撑。手持 turtle_egg useItemOnBlock 点击 (3,1,1) 顶面 Up → 新蛋落
//   (3,2,1)（placementPos=(3,1,1).relative(Up)，(3,2,1) 是 air 可替换）。getStateForPlacement 返
//   defaultState()（eggs=1）。canPlace→isValidPosition(state, (3,2,1))：下方 (3,1,1) sand ∈
//   BlockTags::SAND → true → 放置成功，(3,2,1) 是 turtle_egg。
// 布局 B（失败）：(3,1,1) stone 支撑（stone 不在 BlockTags::SAND）。手持 turtle_egg useItemOnBlock
//   点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) air。getStateForPlacement defaultState()。canPlace→
//   isValidPosition(state, (3,2,1))：下方 (3,1,1) stone 不在 BlockTags::SAND → false → 放置失败，
//   (3,2,1) 仍 air。
//
// 判定：A 布局 useItemOnBlock 返 true 且 (3,2,1) === turtle_egg；B 布局 useItemOnBlock 返 false 且
//   (3,2,1) === air（非 turtle_egg）。
//
// 此场景验证 wiki「海龟蛋须放在沙子类方块上」+ isValidPosition BlockTags::SAND 判定：sand 支撑放成功，
//   stone 支撑放失败。两端与 vanilla 一致。
function turtleEggPlacementRequiresSandBelow(test: Test): void {
    // —— 布局 A：sand 支撑，放蛋成功 ——
    test.setBlockType("minecraft:sand", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:sand", `sand should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmerA = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmerA");
    const eggA = new ItemStack("minecraft:turtle_egg", 1);
    // 手持 turtle_egg 点击 (3,1,1) 顶面 Up → 新蛋落 (3,2,1)。isValidPosition(下方 sand) true → 放成功。
    const usedA = farmerA.useItemOnBlock(
        eggA as unknown as Parameters<typeof farmerA.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(usedA, "useItemOnBlock should return true when placing turtle egg on sand");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:turtle_egg", `turtle egg should be placed at (3,2,1) on sand, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 清理 (3,2,1) 蛋 + (3,1,1) sand，为布局 B 腾位。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // —— 布局 B：stone 支撑，放蛋失败 ——
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmerB = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmerB");
    const eggB = new ItemStack("minecraft:turtle_egg", 1);
    // 手持 turtle_egg 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1)。isValidPosition(下方 stone 非 SAND)
    //   false → 放失败，(3,2,1) 仍 air。
    const usedB = farmerB.useItemOnBlock(
        eggB as unknown as Parameters<typeof farmerB.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(!usedB, "useItemOnBlock should return false when placing turtle egg on stone (not sand)");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `no turtle egg should be at (3,2,1) on stone, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：eggs/hatch 双 state 读写——预置 eggs=3,hatch=1 → getState 可读。
//
// 布局：sand 支撑 (3,1,1) + turtle_egg (3,2,1) eggs=3,hatch=0（setBlockType 默认 eggs=1,hatch=0），
//   再 setBlockWithStates 覆盖为 eggs=3,hatch=1（绕过物品放置，直接写 state）。
//
// 判定：getState("eggs")===3 且 getState("hatch")===1（验证 eggs/hatch 双 state 经 setBlockWithStates
//   写入后可读）。
//
// 此场景验证 wiki「海龟蛋 EGGS 1-4 / HATCH 0-2 双 state」可读写：setBlockWithStates 预置 eggs=3,hatch=1
//   后 getState 双 state 均可读。不测「孵化推进 hatch+1」（randomTick 随机，跳过）。
function turtleEggStateReadable(test: Test): void {
    test.setBlockType("minecraft:sand", { x: 3, y: 1, z: 1 });
    // setBlockWithStates 预置 eggs=3,hatch=1（从默认 state 出发逐属性应用）。
    (test as TestWithStates).setBlockWithStates("minecraft:turtle_egg", { x: 3, y: 2, z: 1 }, "eggs=3,hatch=1");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:turtle_egg", `turtle_egg should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 eggs===3 且 hatch===1（双 state 可读写）。
    test.assert(getEggs(test, 3, 2, 1) === 3, `eggs should be 3 after setBlockWithStates, got ${getEggs(test, 3, 2, 1)}`);
    test.assert(getHatch(test, 3, 2, 1) === 1, `hatch should be 1 after setBlockWithStates, got ${getHatch(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerTurtleEggTests(): void {
    GameTest.register("BlockBehaviorTests", "turtle_egg_stacks_up_to_four", turtleEggStacksUpToFour)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "turtle_egg_placement_requires_sand_below", turtleEggPlacementRequiresSandBelow)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "turtle_egg_state_readable", turtleEggStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
