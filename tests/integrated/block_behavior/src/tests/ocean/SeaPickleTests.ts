// 海泡菜（sea_pickle）堆叠增量行为 GameTest。
//
// wiki tech_海泡菜.txt#用途：海泡菜每个方块最多堆叠 4 个（类似海龟蛋、蜡烛）。手持海泡菜右键已有
//   海泡菜 → 数量 +1（PICKLES state 1→2→3→4），上限 4 不上溢。堆叠后保持原含水状态。海泡菜含水时
//   发光（亮度 1=6/2=9/3=12/4=15，仅作背景，脚本侧不可断言亮度）。海泡菜于 1.13 加入，1.21.11 已包含。
//
// C++ 链路：SeaPickleBlock（ocean/SeaPickleBlock.cpp）继承 Block + IWaterLoggable，两个 state：
//   - pickles（IntegerProperty，1-4，默认 1）：海泡菜数量。
//   - waterlogged（BooleanProperty，默认 true，但放置时由 shouldWaterlogAt 重算）。
//   - SeaPickleBlock **不 override onBlockActivated**（基类返 Pass），故堆叠不在交互路径，而在放置路径：
//     getStateForPlacement（:89-111）查 placementPos 已有方块，若 is(this) 则 pickles+1（count<4 时），
//     否则 pickles=1 + waterlogged=shouldWaterlogAt。用 context.placementPos()（正确，无 EndRodBlock 偏差）。
//   - 堆叠可达性：海泡菜材质 OCEAN_PLANT，makeOceanPlantMaterial 设 replaceable=true（Material.cpp:240-243），
//     BlockItemUseContext::_canReplace 对已有海泡菜返 true → placementPos 落在原海泡菜格（同格替换）→
//     getStateForPlacement 堆叠分支可达（与 CandleBlock 同范式）。count>=4 时 return *existingState
//     （pickles 保持 4 不上溢），但 tryPlace 仍 setBlockState（no-op，已是目标 state）+ 消耗物品。
//   - isValidPosition（:113-128）：下方 isSolid() 即可放置（Cubium 偏差：vanilla 陆地需珊瑚块，Cubium
//     统一为下方固体，测试用 stone 支撑即可，无需珊瑚块）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock(stack, blockLocation, face, faceLocation)（SimulatedPlayer.cpp
//   :265-346）。手持 sea_pickle 物品（BlockItem，Items.cpp:2662 registerBlockBackedItem）。点击已有海泡菜：
//   onBlockActivated 基类返 Pass → fallback Item.useOn → BlockItem::tryPlace → _canReplace(海泡菜)=true →
//   placementPos=原海泡菜格 → getStateForPlacement 堆叠分支 pickles+1 → setBlockState 同步写入。
//   useItemOnBlock 返回后即可 getState("pickles") 读到 +1 后的值（同步）。SimulatedPlayer 默认创造模式
//   （物品不消耗），不影响堆叠逻辑（堆叠靠 existingState 查询，不靠物品数量）。
//
// 测试覆盖（2 个场景，覆盖 wiki 堆叠增量核心确定行为）：
//   1. 堆叠增量：放置 1 → 右键 +1 到 2 → +1 到 3 → +1 到 4，每步断言 pickles 递增。
//   2. 满 4 不上溢：pickles=4 时再右键，断言 pickles 仍 4（不上溢到 5）。
//
// 关键约束：
// 1. 海泡菜需下方固体支撑（isValidPosition 检查 belowState.isSolid()）。下方放 stone 支撑。
//    海泡菜自身 notSolid()，不能作另一个海泡菜的支撑——堆叠靠 isReplaceable 同格替换，非下方支撑。
// 2. 放置/堆叠点击面传 Direction.Up：步骤 1 点击下方 stone 顶面（placementPos=stone 上方格=海泡菜格）；
//    步骤 2+ 点击已有海泡菜顶面（isReplaceable=true → placementPos=海泡菜格，同格替换堆叠）。
//    faceLocation 默认 (0.5,0.5,0.5) 方块中心，不影响堆叠（堆叠靠 placementPos 已有方块判定）。
// 3. 手持物品用 new ItemStack("minecraft:sea_pickle", 1)，cast 后传 useItemOnBlock（同 FlowerPotTests/
//    EndRodTests/ChainTests/BellTests 范式）。每次操作 new 一个新 ItemStack（创造模式不消耗，但语义清晰）。
// 4. pickles 判定用 getState("pickles" as any)——Cubium PICKLES state 的 C++ 属性名为 "pickles"
//    （IntegerProperty::create("pickles", 1, 4)），getState 对 IntegerProperty 返 number（i32）。用 as any
//    绕过 BlockStateSuperset 白名单。断言用 === 数值比较。
// 5. 堆叠是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后即可读。
//    无需轮询，但留 small maxTicks 余量防时序。
// 6. 场景 2 用 setBlockWithStates 预置 pickles=4（直接写满，不走 4 次堆叠），再右键断言不上溢。
//    预置须显式设 waterlogged（Cubium 默认 waterlogged=true，干燥环境设 false 避免水源干扰）。
//
// 不测「骨粉传播」：Cubium 未实现（SeaPickleBlock 不继承 IGrowable，BoneMealItem::onItemUse 对海泡菜返
//   Fail 无效果）。vanilla 骨粉会补到 4 + 扩散，Cubium 无此行为，跳过。TODO: 待骨粉传播实现后补。
// 不测「发光亮度 6/9/12/15」：亮度是光照引擎层面，无脚本 API 断言 getLightLevel，跳过。
// 不测「陆地放置需珊瑚块」：Cubium isValidPosition 偏差（统一为下方固体，不限制珊瑚块），该 vanilla
//   行为在 Cubium 不存在，跳过。TODO: 待陆地珊瑚块限制实现后补。
// 不测「破坏掉落=海泡菜数」：掉落是 loot table 层面，非 state 层面，跳过。
//
// 跨服务端：sea_pickle 方块名两端一致（minecraft:sea_pickle），pickles state 名两端一致（C++ 内部名
//   "pickles"，基岩对外经 TemplateLoader 映射，脚本侧 getState 用 C++ 名）。堆叠增量行为两端一致：
//   右键已有海泡菜 pickles+1，上限 4 不上溢。两端均可放 stone + sea_pickle，堆叠行为两端可对比，
//   非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海泡菜.txt#用途（最多 4 个，右键已有 +1 堆叠）
// Ref: SeaPickleBlock.cpp（getStateForPlacement 堆叠分支：placementPos 已有 is(this) 则 pickles+1，count<4）
// Ref: SeaPickleBlock.cpp（isValidPosition 下方 isSolid 即可放置，Cubium 偏差不限珊瑚块）
// Ref: Material.cpp（OCEAN_PLANT replaceable=true，堆叠靠 isReplaceable 同格替换）
// Ref: SimulatedPlayer.cpp（useItemOnBlock：Block.use Pass fallback Item.useOn 放置，同步 setBlockState）
// Ref: CandleTests.ts（同类堆叠范式：isReplaceable 同格替换 +1，getState 读数量）
// Ref: FlowerPotTests.ts（useItemOnBlock 放置范式：new ItemStack + cast + face 参数）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 海泡菜 (3,2,1)，下方支撑 (3,1,1) stone（isValidPosition 要求 belowState.isSolid）。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BellTests/EndRodTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 海泡菜 pickles state（number：1-4）。返回 null 表示读取失败或非海泡菜。
// Cubium PICKLES state C++ 属性名为 "pickles"（IntegerProperty::create("pickles", 1, 4)），
// getState 对 IntegerProperty 返 number。
function getPickles(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("pickles" as any);
    return typeof value === "number" ? value : null;
}

// 场景 1：堆叠增量——放置 1 → 右键 +1 到 2 → +1 到 3 → +1 到 4，每步断言 pickles 递增。
//
// 布局：(3,1,1) stone 支撑。手持 sea_pickle useItemOnBlock 点击 (3,1,1) 顶面 Up → 海泡菜落 (3,2,1)
//   （placementPos=stone 上方格），getStateForPlacement 无已有海泡菜 → pickles=1。
//   再手持 sea_pickle 点击 (3,2,1) 顶面 Up → onBlockActivated Pass → fallback 放置 →
//   _canReplace((3,2,1) 海泡菜)=true → placementPos=(3,2,1) → getStateForPlacement 已有海泡菜 count=1<4
//   → pickles=2。重复 +1 到 3、4。
//
// 判定：每步 useItemOnBlock 后 getState("pickles") === 期望值（1→2→3→4 递增）。
function seaPickleStacksUpToOneByOne(test: Test): void {
    // (3,1,1) 放 stone 支撑（isValidPosition 要求 belowState.isSolid）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");

    // 放置 1 个海泡菜：点击 (3,1,1) stone 顶面 Up → 海泡菜落 (3,2,1)，pickles=1。
    let used = farmer.useItemOnBlock(
        new ItemStack("minecraft:sea_pickle", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing first sea_pickle");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:sea_pickle", `sea_pickle should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getPickles(test, 3, 2, 1) === 1, `first sea_pickle pickles should be 1, got ${getPickles(test, 3, 2, 1)}`);

    // 堆叠 +1 到 2：点击 (3,2,1) 海泡菜顶面 Up → 同格替换堆叠，pickles=2。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:sea_pickle", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking sea_pickle to 2");
    test.assert(getPickles(test, 3, 2, 1) === 2, `sea_pickle pickles should be 2 after stack, got ${getPickles(test, 3, 2, 1)}`);

    // 堆叠 +1 到 3：点击 (3,2,1) 顶面 Up → pickles=3。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:sea_pickle", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking sea_pickle to 3");
    test.assert(getPickles(test, 3, 2, 1) === 3, `sea_pickle pickles should be 3 after stack, got ${getPickles(test, 3, 2, 1)}`);

    // 堆叠 +1 到 4：点击 (3,2,1) 顶面 Up → pickles=4（达上限）。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:sea_pickle", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking sea_pickle to 4");
    test.assert(getPickles(test, 3, 2, 1) === 4, `sea_pickle pickles should be 4 after stack, got ${getPickles(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：满 4 不上溢——pickles=4 时再右键，断言 pickles 仍 4（不上溢到 5）。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 海泡菜预置 pickles=4（setBlockWithStates 直接写满，显式设
//   waterlogged=false 避免水源干扰）。手持 sea_pickle useItemOnBlock 点击 (3,2,1) 顶面 Up →
//   getStateForPlacement 已有海泡菜 count=4，!count<4 → return *existingState（pickles 保持 4）。
//   tryPlace setBlockState（no-op，已是目标 state）+ 消耗物品，但 pickles 不变。
//
// 判定：useItemOnBlock 后 getState("pickles") === 4（满不上溢，不变成 5）。
function seaPickleDoesNotExceedFourWhenFull(test: Test): void {
    // (3,1,1) stone 支撑。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 预置海泡菜 pickles=4（直接写满，显式 waterlogged=false 干燥环境避免水源干扰）。
    (test as TestWithStates).setBlockWithStates("minecraft:sea_pickle", { x: 3, y: 2, z: 1 }, "pickles=4,waterlogged=false");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:sea_pickle", `sea_pickle should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getPickles(test, 3, 2, 1) === 4, `sea_pickle pickles should be 4 before overflow attempt, got ${getPickles(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");

    // 手持 sea_pickle 点击 (3,2,1) 顶面 Up → getStateForPlacement count=4 !<4 → return *existingState
    //   （pickles 保持 4，与当前 state 完全相同）。placeBlock 调 setBlockState 判定 no-op（已是目标
    //   state）返 false → tryPlace 返 Fail → useItemOnBlock 返 false（Cubium 行为：满 4 不上溢且不消耗，
    //   因 setBlockState no-op 返 false；与 vanilla 满 4 右键消耗物品但不上溢有细微差异，属 setBlockState
    //   no-op 语义特性，非 sea_pickle 特有）。核心断言是 pickles 不上溢，返回值宽松处理。
    const used = farmer.useItemOnBlock(
        new ItemStack("minecraft:sea_pickle", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    // 满 4 时 useItemOnBlock 返 false（Cubium no-op setBlockState），不强制 true。
    test.assert(used === false, `useItemOnBlock should return false when sea_pickle is full (no-op setBlockState), got ${used}`);

    // 判定：pickles 仍 4（满不上溢，不变成 5）。这是核心断言。
    test.assert(getPickles(test, 3, 2, 1) === 4, `sea_pickle pickles should stay 4 (no overflow to 5), got ${getPickles(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerSeaPickleTests(): void {
    GameTest.register("BlockBehaviorTests", "sea_pickle_stacks_up_one_by_one", seaPickleStacksUpToOneByOne)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "sea_pickle_does_not_exceed_four_when_full", seaPickleDoesNotExceedFourWhenFull)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
