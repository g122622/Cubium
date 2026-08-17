// 锁链（iron_chain）放置轴向（AXIS）行为 GameTest。
//
// wiki tech_铁链.txt#用途：锁链不需要任何支撑方块即可放置，即使周围没有方块也可完全独立存在。
//   锁链能水平或垂直放置，但不会像铁栏杆那样在不同水平方向相交时形成转角。1.16.2 pre1 起可沿
//   任意坐标轴方向放置。放置时锁链的轴向（AXIS state：x/y/z）由玩家点击的面决定——点击顶面/底面
//   （Up/Down）→ 竖直 y 轴；点击北面/南面（North/South）→ 水平 z 轴；点击东面/西面（East/West）
//   → 水平 x 轴。这是锁链区别于普通方块的定向放置行为（轴向跟随点击面）。
//   - 锁链可放置特性于 1.16.2 pre1 加入，1.21.11 已包含，属 vanilla 正式特性。
//   - 1.21.9（25w32a 重命名为 Iron Chain，25w35a 将 ID 由 chain 改为 iron_chain），Cubium 对齐用
//     minecraft:iron_chain 注册名。
//
// C++ 链路：ChainBlock（decorative/ChainBlock.cpp）继承 Block + IWaterLoggable，AXIS + WATERLOGGED
//   两个 state，默认 axis=y（:60-61）。
//   - getStateForPlacement（:72-86，对齐 MC 1.21.11 ChainBlock.getStateForPlacement →
//     RotatedPillarBlock.getStateForPlacement）：
//     axis = Directions::getAxis(context.getClickedFace())——点击面方向映射到轴。
//     映射表（Direction.hpp:238-251）：Down/Up→Y, North/South→Z, West/East→X，与 vanilla
//     Direction.getAxis() 一致。AXIS 纯由点击面决定，不查邻居（与 EndRodBlock 背靠背逻辑不同，
//     无相邻同方块衔接判定）。waterlogged = shouldWaterlogAt(placementPos)（放置目标格水状态）。
//   - ChainBlock 不 override isValidPosition（基类默认返 true），可任意放置，无需支撑（对齐 wiki
//     「不需要任何支撑方块」）。
//   - ChainBlock 不 override onBlockActivated（基类返 Pass），故 useItemOnBlock 走 Block.use 前置
//     分支后 fallback 到 Item.useOn（BlockItem::onItemUse → tryPlace → getStateForPlacement）放置新块。
//
// 派发链路：SimulatedPlayer::useItemOnBlock(stack, blockLocation, face, faceLocation)（SimulatedPlayer.cpp
//   :265-346）。face 是被点击现有方块的哪一面（BlockRaycastResult face，从现有方块指向新格/外侧），
//   原样作为 getClickedFace() 传入 BlockItemUseContext。点击某方块某面 → 新块落在该面外侧相邻格
//   （placementPos = 被点击方块.relative(face)）。AXIS 由 face 映射决定。
//   iron_chain 物品是 BlockItem（Items.cpp:3768 registerBlockBackedItem + BlockItemRegistry.cpp:859
//   registerSimpleBlock），可被 useItemOnBlock 放置。AXIS 计算只由 getClickedFace 决定，不读
//   yaw/pitch/horizontalDirection，故 SimulatedPlayer 默认朝向不干扰。
//
// 测试覆盖（3 个场景，覆盖 wiki 三轴向放置核心确定行为，每轴一个代表面）：
//   1. 点击顶面 Up → axis=y（竖直）：(3,1,1) stone + 手持 iron_chain 点其顶面 Up → (3,2,1) axis=y。
//   2. 点击北面 North → axis=z（水平 z 轴）：(3,2,2) stone + 手持 iron_chain 点其北面 North → (3,2,1) axis=z。
//   3. 点击东面 East → axis=x（水平 x 轴）：(2,2,1) stone + 手持 iron_chain 点其东面 East → (3,2,1) axis=x。
//
// 关键约束：
// 1. 锁链无需支撑（isValidPosition 默认返 true），但需要一个「被点击的现有方块」作为放置参照——
//    用 stone 作为被点击方块（stone 不 override onBlockActivated 基类返 Pass，不短路放置）。
//    点击 stone 的某面，新 iron_chain 落在该面外侧相邻格。
// 2. useItemOnBlock 第二参数是被点击的现有方块坐标，第三参数 face 是点击的面。新块落在
//    placementPos = 被点击方块.relative(face)。例如点击 (3,1,1) 顶面 Up → 新块 (3,2,1)。
//    faceLocation 默认 (0.5,0.5,0.5) 即方块中心，不影响 AXIS（AXIS 只由 face 决定）。
// 3. 手持物品用 new ItemStack("minecraft:iron_chain", 1)，cast 后传 useItemOnBlock（同 FlowerPotTests/
//    EndRodTests 范式）。iron_chain 是 BlockItem，onItemUse → tryPlace → getStateForPlacement →
//    setBlockState(placementPos)。
// 4. 轴向判定用 getState("axis" as any)——Cubium AXIS state 的 C++ 属性名为 "axis"
//    （AxisProperty::create("axis")），getState 对 EnumProperty<Axis> 走 fallback 返小写轴名
//    字符串（"x"/"y"/"z"，Axes::toString 表）。用 as any 绕过 BlockStateSuperset 白名单。
// 5. 锁链放置是 useItemOnBlock 同步触发（BlockItem::onItemUse → tryPlace 同步 setBlockState），
//    useItemOnBlock 返回后即可读 state。无需轮询。但留 small maxTicks 余量防时序。
// 6. 三场景覆盖三轴向（y/z/x），每轴取一个代表面（Up/North/East）。不测 Down/South/West 三个
//    反面——它们与对应正面同轴（Down→y, South→z, West→x），行为对称冗余，避免测试膨胀。
//
// 不测「锁链含水」：waterlogged 涉流体放置，本文件聚焦 AXIS 放置朝向。TODO: 可补 chain_waterlogged_when_placed_in_water。
// 不测「锁链旋转（rotate）」：rotate 是物品框架旋转（X↔Z 互换），非放置行为，且需 rotate API，
//   跳过。TODO: 可补 chain_rotate_swaps_x_z_axis。
// 不测「锁链灯笼悬挂」：锁链可悬挂灯笼等是灯笼附着判定，非锁链自身放置行为，跳过。
//
// 跨服务端：iron_chain 方块名两端一致（minecraft:iron_chain，1.21.9+ 重命名两端对齐），axis state
//   名两端一致（C++ 内部名 "axis"，基岩对外经 TemplateLoader 映射，脚本侧 getState 用 C++ 名）。
//   AXIS 放置朝向行为两端一致：点击面方向映射到轴。两端均可放 stone + iron_chain，AXIS 行为两端
//   可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_铁链.txt#用途（不需支撑，可水平/垂直放置，沿任意轴）
// Ref: ChainBlock.cpp（getStateForPlacement：axis = getAxis(getClickedFace)，不查邻居，与 vanilla 一致）
// Ref: Direction.hpp（getAxis：Down/Up→Y, North/South→Z, West/East→X，对齐 vanilla Direction.getAxis）
// Ref: SimulatedPlayer.cpp（useItemOnBlock：face 原样作 getClickedFace，Block.use 前置 + Item.useOn fallback 放置）
// Ref: FlowerPotTests.ts / EndRodTests.ts（useItemOnBlock 放置范式：new ItemStack + cast + face 参数）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 三个场景的新块统一落在 (3,2,1)，被点击 stone 分别在其不同方向相邻格。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 方块 AXIS state（小写轴名字符串：x/y/z）。返回 null 表示读取失败或该方块无 axis state。
function getAxis(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("axis" as any);
    return typeof value === "string" ? value : null;
}

// 场景 1：点击顶面 Up → axis=y（竖直）。
//
// 布局：(3,1,1) 放 stone（被点击的现有方块）。手持 iron_chain useItemOnBlock 点击 (3,1,1) 顶面
//   （face=Up），新块落 (3,2,1)。
// getStateForPlacement：face=Up → getAxis(Up)=Y → axis=y。新块 (3,2,1) axis=y（竖直）。
//
// 判定：(3,2,1) typeId === "minecraft:iron_chain" 且 getState("axis") === "y"（竖直）。
function chainAxisYWhenPlacedOnTopFace(test: Test): void {
    // (3,1,1) 放 stone（被点击方块，放置参照，不短路）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const chain = new ItemStack("minecraft:iron_chain", 1);

    // 手持 iron_chain 点击 (3,1,1) 顶面 Up → 新块落 (3,2,1)，getAxis(Up)=Y → axis=y。
    const used = farmer.useItemOnBlock(
        chain as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing iron_chain on top face");

    // 判定：新块 (3,2,1) 是 iron_chain 且 axis=y（点击顶面 → 竖直）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:iron_chain", `new chain should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getAxis(test, 3, 2, 1) === "y", `new chain axis should be y (vertical, top face), got ${getAxis(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：点击北面 North → axis=z（水平 z 轴）。
//
// 布局：(3,2,2) 放 stone（被点击的现有方块，在目标格 (3,2,1) 的南边 z+1）。手持 iron_chain
//   useItemOnBlock 点击 (3,2,2) 北面（face=North），新块落 (3,2,1)（stone 北侧相邻格）。
// getStateForPlacement：face=North → getAxis(North)=Z → axis=z。新块 (3,2,1) axis=z（水平 z 轴）。
//
// 判定：(3,2,1) typeId === "minecraft:iron_chain" 且 getState("axis") === "z"（水平 z 轴）。
function chainAxisZWhenPlacedOnNorthFace(test: Test): void {
    // (3,2,2) 放 stone（被点击方块，在目标格南边 z+1，点击其北面新块落 (3,2,1)）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 2 });
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:stone", `stone should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const chain = new ItemStack("minecraft:iron_chain", 1);

    // 手持 iron_chain 点击 (3,2,2) 北面 North → 新块落 (3,2,1)，getAxis(North)=Z → axis=z。
    const used = farmer.useItemOnBlock(
        chain as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 2 },
        Direction.North,
    );
    test.assert(used, "useItemOnBlock should return true when placing iron_chain on north face");

    // 判定：新块 (3,2,1) 是 iron_chain 且 axis=z（点击北面 → 水平 z 轴）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:iron_chain", `new chain should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getAxis(test, 3, 2, 1) === "z", `new chain axis should be z (horizontal, north face), got ${getAxis(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：点击东面 East → axis=x（水平 x 轴）。
//
// 布局：(2,2,1) 放 stone（被点击的现有方块，在目标格 (3,2,1) 的西边 x-1）。手持 iron_chain
//   useItemOnBlock 点击 (2,2,1) 东面（face=East），新块落 (3,2,1)（stone 东侧相邻格）。
// getStateForPlacement：face=East → getAxis(East)=X → axis=x。新块 (3,2,1) axis=x（水平 x 轴）。
//
// 判定：(3,2,1) typeId === "minecraft:iron_chain" 且 getState("axis") === "x"（水平 x 轴）。
function chainAxisXWhenPlacedOnEastFace(test: Test): void {
    // (2,2,1) 放 stone（被点击方块，在目标格西边 x-1，点击其东面新块落 (3,2,1)）。
    test.setBlockType("minecraft:stone", { x: 2, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 2, 2, 1) === "minecraft:stone", `stone should be at (2,2,1), got ${getBlockTypeId(test, 2, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const chain = new ItemStack("minecraft:iron_chain", 1);

    // 手持 iron_chain 点击 (2,2,1) 东面 East → 新块落 (3,2,1)，getAxis(East)=X → axis=x。
    const used = farmer.useItemOnBlock(
        chain as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 2, y: 2, z: 1 },
        Direction.East,
    );
    test.assert(used, "useItemOnBlock should return true when placing iron_chain on east face");

    // 判定：新块 (3,2,1) 是 iron_chain 且 axis=x（点击东面 → 水平 x 轴）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:iron_chain", `new chain should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getAxis(test, 3, 2, 1) === "x", `new chain axis should be x (horizontal, east face), got ${getAxis(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerChainTests(): void {
    GameTest.register("BlockBehaviorTests", "chain_axis_y_when_placed_on_top_face", chainAxisYWhenPlacedOnTopFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "chain_axis_z_when_placed_on_north_face", chainAxisZWhenPlacedOnNorthFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "chain_axis_x_when_placed_on_east_face", chainAxisXWhenPlacedOnEastFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
