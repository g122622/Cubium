// 末地烛（end_rod）放置朝向行为 GameTest。
//
// wiki world_末地烛.txt#用途：末地烛可放在任何方块表面，不会因支撑方块被破坏而掉落。它具有朝向
//   （FACING state，6 向），放置时朝向玩家点击的面。当玩家尝试在「另一个末地烛的灯柱顶面」上放置
//   末地烛时，游戏放置「方向相反」的末地烛，使二者灯柱对接（背靠背）。这是末地烛区别于普通定向
//   方块的核心放置交互行为。
//   - 末地烛于 1.9 加入，1.21.11 已包含，属 vanilla 正式特性。
//
// C++ 链路：EndRodBlock（end/EndRodBlock.cpp）继承 Block，FACING（6 向）一个 state，默认 facing=up
//   （:60 setDefaultState(FACING, Up)）。
//   - getStateForPlacement（:69-89，对齐 MC 1.21.11 EndRodBlock.getStateForPlacement）：
//     direction = context.getClickedFace()（玩家点击的面，从被点击方块指向新格）。
//     neighborPos = context.placementPos().offset(opposite(direction))——放置目标格再反向回指，
//       即回指被点击的现有方块（vanilla getClickedPos() 是放置目标格语义）。
//     若 neighborState 是 end_rod 且其 FACING == direction → facing = opposite(direction)
//       （背靠背反向，灯柱对接）；否则 facing = direction（正向，朝向点击面）。
//   - placementPos() 返回放置目标格（replaceClicked ? 被点击方块 : 被点击方块朝向那侧的相邻格），
//     对齐 vanilla BlockPlaceContext.getClickedPos() 的重写语义。曾误用 blockPos()（被点击方块）
//     导致背靠背判定查错邻居，已修复为 placementPos()（见 EndRodBlock.cpp 修复注释）。
//   - EndRodBlock 不 override onBlockActivated（基类返 Pass），故 useItemOnBlock 走 Block.use 前置
//     分支后 fallback 到 Item.useOn（BlockItem::onItemUse → tryPlace → getStateForPlacement）放置新块。
//   - EndRodBlock 不 override isValidPosition（基类默认返 true），可在任意位置放置，无需支撑方块。
//
// 派发链路：SimulatedPlayer::useItemOnBlock(stack, blockLocation, face, faceLocation)（SimulatedPlayer.cpp
//   :265-346）。face 是被点击现有方块的哪一面（BlockRaycastResult face，从现有方块指向新格/外侧），
//   原样作为 getClickedFace() 传入 BlockItemUseContext。点击 (3,2,1) 顶面传 face=Up → 新块放 (3,3,1)。
//   end_rod 物品是 BlockItem（Items.cpp:3775 registerBlockBackedItem + BlockItemRegistry.cpp:857
//   registerSimpleBlock），可被 useItemOnBlock 放置。end_rod 放置朝向只由 getClickedFace 决定，不读
//   yaw/pitch/horizontalDirection，故 SimulatedPlayer 默认朝向不干扰。
//
// 测试覆盖（2 个场景，覆盖 wiki 放置朝向核心行为，双向闭合）：
//   1. 背靠背反向放置：(3,2,1) end_rod facing=up + 手持 end_rod 点 (3,2,1) 顶面 Up → (3,3,1) facing=down。
//   2. 正向放置对照：(3,1,1) stone + 手持 end_rod 点 (3,1,1) 顶面 Up → (3,2,1) facing=up（无同向邻居 fallback）。
//
// 关键约束：
// 1. end_rod 无需支撑（isValidPosition 默认返 true），直接放置存活。但场景 1 需先放一根 facing=up 的
//    end_rod 作为「被点击的现有方块」——用 setBlockWithStates 设 facing=up（C++ 属性名 "facing"，
//    值 "up"，setBlockWithStates 格式 "facing=up"）。setBlockWithStates 是 Cubium 专有写入（基岩无），
//    通过 cast Test & { setBlockWithStates } 访问（同 TripWireHookTests/BedTests 范式）。
// 2. useItemOnBlock 第二参数是被点击的现有方块坐标，第三参数 face 是点击的面。点击 (3,2,1) 顶面传
//    face=Direction.Up，新块落在 (3,3,1)（placementPos = (3,2,1).relative(Up)）。faceLocation 默认
//    (0.5,0.5,0.5) 即方块中心，不影响 end_rod 放置朝向（朝向只由 face 决定）。
// 3. 手持物品用 new ItemStack("minecraft:end_rod", 1)，cast 后传 useItemOnBlock（同 FlowerPotTests 范式）。
//    end_rod 是 BlockItem，onItemUse → tryPlace → getStateForPlacement → setBlockState(placementPos)。
// 4. 放置朝向判定用 getState("facing" as any)——Cubium FACING state 的 C++ 属性名为 "facing"
//    （DirectionProperty::create("facing")），getState 对 DirectionProperty 走 fallback 返小写方向名
//    字符串（"up"/"down" 等，Directions::toString 表）。用 as any 绕过 BlockStateSuperset 白名单。
// 5. end_rod 放置是 useItemOnBlock 同步触发（BlockItem::onItemUse → tryPlace 同步 setBlockState），
//    useItemOnBlock 返回后即可读 state。无需轮询。但留 small maxTicks 余量防时序。
//
// 不测「末地烛被水/熔岩流动破坏」：涉流体 tick 非确定 + 流体扩散时序，跳过。TODO: 可补 end_rod_broken_by_fluid。
// 不测「下落方块砸到水平末地烛被破坏」：涉 FallingBlock 实体 + 朝向组合 + 非确定时序，跳过。TODO: 可补
//   end_rod_breaks_falling_block_when_horizontal。
// 不测「末地烛发光等级 14」：光照测量涉 light engine，非放置行为，跳过。
// 不测「末地烛含水（BE）」：含水是 BE 特性且涉流体，跳过。
//
// 跨服务端：end_rod 方块名两端一致（minecraft:end_rod），facing state 名两端一致（C++ 内部名 "facing"，
//   基岩对外经 TemplateLoader 映射，脚本侧 getState 用 C++ 名）。放置朝向行为两端一致：点击面方向 fallback
//   为正向，点击同向 end_rod 顶面则反向（背靠背）。setBlockWithStates 设初始 facing 是 Cubium 专有写入
//   （基岩侧用物品放置朝向），但放置朝向行为本身两端可对比。注意：本测试核心场景 1 依赖刚修复的
//   placementPos() 修正——修复前 Cubium 给 facing=up（与 vanilla down 不符），修复后两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_末地烛.txt#用途（放任何表面，点击同向末地烛顶面放反向末地烛）
// Ref: EndRodBlock.cpp（getStateForPlacement：placementPos().offset(opposite(direction)) 查邻居，同向 end_rod → 反向）
// Ref: BlockItemUseContext.cpp（placementPos = replaceClicked ? blockPos : adjacentPos，对齐 vanilla getClickedPos 重写）
// Ref: SimulatedPlayer.cpp（useItemOnBlock：face 原样作 getClickedFace，Block.use 前置 + Item.useOn fallback 放置）
// Ref: FlowerPotTests.ts（useItemOnBlock 放置范式：new ItemStack + cast + face 参数）
// Ref: TripWireHookTests.ts（setBlockWithStates 设 facing 范式 + getState("facing") 读 facing 范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1：被点击 end_rod (3,2,1) facing=up，新块落 (3,3,1)。
// 场景 2：被点击 stone (3,1,1)，新块落 (3,2,1)。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 TripWireHookTests/BedTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 方块 FACING state（小写方向名字符串：up/down/north/south/west/east）。
// 返回 null 表示读取失败或该方块无 facing state。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 场景 1：背靠背反向放置——(3,2,1) end_rod facing=up + 手持 end_rod 点其顶面 Up → (3,3,1) facing=down。
//
// 布局：(3,2,1) 放 end_rod facing=up（被点击的现有方块）。手持 end_rod useItemOnBlock 点击 (3,2,1) 顶面
//   （face=Up），新块落 (3,3,1)。
// getStateForPlacement：direction=Up（getClickedFace）。placementPos=(3,3,1)。
//   neighborPos = (3,3,1).offset(opposite(Up)) = (3,3,1).offset(Down) = (3,2,1)（回指被点击的现有 end_rod）。
//   neighborState 是 end_rod 且 FACING==up==direction → 条件成立 → facing=opposite(Up)=Down。
//   新块 (3,3,1) facing=down（与 (3,2,1) facing=up 灯柱对接，背靠背）。
//
// 判定：(3,3,1) typeId === "minecraft:end_rod" 且 getState("facing") === "down"（背靠背反向）。
function endRodReversesFacingWhenPlacedOnSameFacingRod(test: Test): void {
    // (3,2,1) 放 end_rod facing=up（被点击的现有方块，用 setBlockWithStates 设 facing=up）。
    (test as TestWithStates).setBlockWithStates("minecraft:end_rod", { x: 3, y: 2, z: 1 }, "facing=up");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:end_rod", `base end_rod should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "up", `base end_rod facing should be up, got ${getFacing(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const endRod = new ItemStack("minecraft:end_rod", 1);

    // 手持 end_rod 点击 (3,2,1) 顶面 Up → useItemOnBlock Block.use Pass fallback Item.useOn 放置新块 (3,3,1)。
    // getStateForPlacement：direction=Up，neighborPos=(3,2,1) 现有 end_rod FACING=up==direction → facing=down。
    const used = farmer.useItemOnBlock(
        endRod as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing end_rod on top of another end_rod");

    // 判定：新块 (3,3,1) 是 end_rod 且 facing=down（背靠背反向，灯柱对接）。
    test.assert(getBlockTypeId(test, 3, 3, 1) === "minecraft:end_rod", `new end_rod should be at (3,3,1), got ${getBlockTypeId(test, 3, 3, 1)}`);
    test.assert(getFacing(test, 3, 3, 1) === "down", `new end_rod facing should be down (back-to-back reverse), got ${getFacing(test, 3, 3, 1)}`);

    test.succeed();
}

// 场景 2：正向放置对照——(3,1,1) stone + 手持 end_rod 点 (3,1,1) 顶面 Up → (3,2,1) facing=up（无同向邻居 fallback）。
//
// 布局：(3,1,1) 放 stone（被点击的现有方块，非 end_rod）。手持 end_rod useItemOnBlock 点击 (3,1,1) 顶面
//   （face=Up），新块落 (3,2,1)。
// getStateForPlacement：direction=Up。placementPos=(3,2,1)。
//   neighborPos = (3,2,1).offset(opposite(Up)) = (3,1,1)（回指被点击的 stone）。
//   neighborState 是 stone（!is(end_rod)）→ 条件不成立 → facing=direction=Up（正向，朝向点击面）。
//   新块 (3,2,1) facing=up（默认朝向，朝向被点击面方向）。
//
// 判定：(3,2,1) typeId === "minecraft:end_rod" 且 getState("facing") === "up"（正向，非反向）。
//
// 此场景对照场景 1：点击非 end_rod 方块时走 fallback 正向朝向，验证「反向仅对同向 end_rod 触发」的边界，
//   排除「永远反向」或「永远正向」的错误实现。
function endRodFacesClickedDirectionWhenPlacedOnNormalBlock(test: Test): void {
    // (3,1,1) 放 stone（被点击的现有方块，非 end_rod，触发 fallback 正向朝向）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const endRod = new ItemStack("minecraft:end_rod", 1);

    // 手持 end_rod 点击 (3,1,1) 顶面 Up → 新块落 (3,2,1)。
    // getStateForPlacement：direction=Up，neighborPos=(3,1,1) stone !is(end_rod) → facing=Up（正向）。
    const used = farmer.useItemOnBlock(
        endRod as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing end_rod on stone");

    // 判定：新块 (3,2,1) 是 end_rod 且 facing=up（正向朝向点击面，非反向）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:end_rod", `new end_rod should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "up", `new end_rod facing should be up (forward, clicked face), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerEndRodTests(): void {
    GameTest.register("BlockBehaviorTests", "end_rod_reverses_facing_when_placed_on_same_facing_rod", endRodReversesFacingWhenPlacedOnSameFacingRod)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "end_rod_faces_clicked_direction_when_placed_on_normal_block", endRodFacesClickedDirectionWhenPlacedOnNormalBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
