// 带釉陶瓦（glazed_terracotta，16 色）放置朝向、state 读写与破坏行为 GameTest。
//
// wiki tech_带釉陶瓦.txt#放置/破坏/用途：
//   - 放置（:114-116）：放置时纹理根据玩家面对方向旋转（facing=opposite(玩家水平朝向)，箭头背离玩家）。
//     品红色带釉陶瓦含箭头纹理可指引方向，朝向表（:118-130）基于 facing=opposite(玩家朝向) 语义。
//   - 破坏（:107）：带釉陶瓦被破坏后掉落自身（基线掉落行为）。
//   - 破坏工具（:103）：合适工具为镐，需木镐及以上，否则不掉落（涉工具等级判定，复杂，本组不测）。
//   - 用途（:112）：能被活塞推动但不能被活塞拉动（wiki 未带 {{in}} 标签，但「推动不拉动」是 JE 行为；
//     BE 带釉陶瓦可被活塞拉动是 JE/BE 偏差，Cubium 活塞未实现，留 TODO 不测）。
//   - 16 色方块 ID 两端一致（淡灰色 1.13 扁平化后统一 light_gray_glazed_terracotta，:347-348）。
//
// C++ 链路：GlazedTerracottaBlock（blocks/decorative/GlazedTerracottaBlock.cpp）单 state：
//   - HORIZONTAL_FACING（C++ 属性名 "facing"，DirectionProperty::createHorizontal，默认 North，
//     构造函数 :56 setDefaultState facing=North）。
//   - getStateForPlacement（:59-64）：facing=opposite(context.horizontalDirection())——对齐 vanilla
//     GlazedTerracottaBlock.java:28 getHorizontalDirection().getOpposite()（箭头背离玩家）。
//     此前 Cubium 漏 opposite（同向），本期修复加 Directions::opposite 对齐 vanilla。
//   - horizontalDirection 由 playerYaw 计算（BlockItemUseContext.cpp:111-125）：
//     yaw∈[315,360)∪[0,45)→South，[45,135)→West，[135,225)→North，[225,315)→East。
//   - rotate（:66-71）/mirror（:73-79）用 Directions::rotateDirection/mirrorToRotation（结构旋转/镜像，
//     非 GameTest 范畴，本组不测）。
//   - 无 canSurvive/isValidPosition/updatePostPlacement/BlockEntity（纯装饰方块，无支撑要求/无容器）。
//   - 物品注册：BlockItemRegistry.cpp 在 terracotta 段后补 16 色 registerSimpleBlock
//     (VanillaBlocks::XXX_GLAZED_TERRACOTTA, "xxx_glazed_terracotta")。此前物品全部未注册，本期补全，
//     使 useItemOnBlock 放置链路与破坏掉落可测。方块本身早已注册（ColoredBlocks.cpp:431-467）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 white_glazed_terracotta
//   物品点击 stone 顶面 → onBlockActivated 基类 Pass（stone 非带釉陶瓦，targetBlock 走基类 Pass）→
//   fallback Item.useOn → BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext（playerYaw 来自
//   SimulatedPlayer yaw()，:325）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)) →
//   setBlockState 放带釉陶瓦。创造模式不消耗物品。
//
// 朝向控制（复用 FurnaceTests opposite 范式）：
//   带釉陶瓦 facing=opposite(horizontalDirection)（仅 yaw），与熔炉同语义（熔炉 facing=opposite(朝向)，
//   lookAt.y=playerPos.y 即可，pitch 不影响 facing）。故 4 朝向坐标配方与 FurnaceTests FACING_CASES
//   完全一致（玩家位置/lookAt 目标/yaw/facing 映射），仅替换被测方块为 white_glazed_terracotta。
//   lookAtLocation(blockPos) 瞬时设 yaw=atan2(-dx,dz)（0→South,90→West,180→North,270→East，
//   SimulatedPlayer.cpp:99）。
//   4 朝向映射（facing=opposite(玩家朝向)）：
//     - yaw=0(South)→facing=North；yaw=90(West)→facing=East；
//     - yaw=180(North)→facing=South；yaw=270(East)→facing=West。
//
// 测试覆盖（4 个场景，覆盖 wiki 放置朝向 + 物品放置链路 + state 读写 + 破坏核心确定行为）：
//   1. facing=opposite(玩家朝向) 放置（4 朝向，white_glazed_terracotta 代表色）：玩家 lookAtLocation
//      控制朝向 → useItemOnBlock white_glazed_terracotta → 断言 facing=opposite(朝向)。4 朝向逐一验证
//      South→North/West→East/North→South/East→West 映射（验证修复 getStateForPlacement 加 opposite）。
//   2. 物品放置链路（white_glazed_terracotta 代表色）：useItemOnBlock 放置 → typeId=white_glazed_terracotta
//      + facing 默认（默认 yaw=0→South→facing=North）。验证 16 色物品注册补全后放置可用。
//   3. facing state 读写：setBlockWithStates 预置 facing=east → getState 可读。
//   4. 破坏不崩溃：放带釉陶瓦 → setBlockType air 破坏 → 位置变 air（无 BlockEntity，链路安全）。
//
// 关键约束：
// 1. 场景 1 朝向控制：每朝向独立 spawn 玩家（不与 (3,1,1)/(3,2,1) 重叠），lookAtLocation 传入 [0,6] 内
//    目标坐标产生目标 yaw，再 useItemOnBlock white_glazed_terracotta 点击 (3,1,1) stone Up → 落 (3,2,1)。
//    4 朝向玩家位置/lookAt 目标/yaw/facing 映射见场景 1 注释（同 FurnaceTests FACING_CASES）。
// 2. 场景 2 用 useItemOnBlock 放置：手持 white_glazed_terracotta 点击 (3,1,1) stone 顶面 Up →
//    placementPos=(3,2,1)（stone 不可替换 → 相邻位置上方 air）→ getStateForPlacement facing=North
//    （默认 yaw=0→South→opposite=North）→ setBlockState 放置。断言 typeId + facing=North。
// 3. 场景 3 用 setBlockWithStates 预置 facing=east（绕过物品放置，直接写 state）。getState 读 "facing"
//    （C++ 内部属性名）。验证 facing state 可读写。
// 4. 场景 4 放带釉陶瓦后 setBlockType air 破坏：纯装饰方块无 BlockEntity，基类 onBlockRemoved 空操作，
//    位置变 air。断言位置变 air（破坏成功，链路不崩溃）。破坏掉落物非确定（项目范式不验证掉落物实体，
//    见 GrindstoneTests/BarrelTests），仅测变 air。
// 5. 读 facing 用 getState("facing" as any)（DirectionProperty 序列化为方向名字符串 "north"/"south"/
//    "east"/"west"，与 Java 命名一致，小写）。
// 6. 带釉陶瓦有碰撞箱（固体方块），但无 canSurvive（基类 true 无支撑要求）。stone 支撑仅为贴近真实放置。
// 7. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
// 8. 16 色共享同一 GlazedTerracottaBlock 类与同一 getStateForPlacement，4 朝向用 white 代表色覆盖即可，
//    无需 16 色×4 朝向（避免 64 测试过重且无额外覆盖价值）。
//
// 不测「活塞推动不拉动」：wiki :112「能被活塞推动但不能被活塞拉动」是 JE 行为，BE 可拉动是 JE/BE 偏差；
//   Cubium 活塞未实现，无法测。TODO: 待活塞实现且 JE/BE 推拉行为统一后补 glazed_terracotta_piston_push。
// 不测「破坏工具等级」：wiki :103 需木镐及以上否则不掉落，涉工具等级判定 + 徒手/错工具破坏掉落差异，
//   SimulatedPlayer 持工具破坏 API 受限且破坏掉落物非确定（项目范式不验证掉落物实体）。TODO: 待脚本侧
//   工具等级破坏掉落测试范式完善后补 glazed_terracotta_requires_pickaxe_to_drop。
// 不测「烧炼获取」：涉熔炉烧炼配方 + tick 计时，非确定且复杂，跳过。
// 不测「纹理拼接图案」：涉 2×2 摆放 + 纹理渲染，GameTest 无法读纹理，跳过。
//
// 跨服务端：white_glazed_terracotta 方块名两端一致。facing state 名两端一致（C++ 内部名 "facing"，
//   Java 命名）。朝向放置（facing=opposite(玩家朝向)）+ state 读写 + 破坏行为两端与 vanilla 一致。
//   lookAtLocation 是 Cubium 专有朝向控制（基岩 GameTest 无此 API），但 facing=opposite(朝向) 放置行为
//   本身两端可对比（基岩用真实玩家朝向放置），非 one-sided。setBlockWithStates 预置 state 是 Cubium
//   专有写入，但 state 行为本身两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_带釉陶瓦.txt#放置（纹理随玩家朝向旋转，facing=opposite(朝向)）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_带釉陶瓦.txt#破坏（破坏掉落自身）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_带釉陶瓦.txt#用途（活塞推动不拉动，JE 行为，Cubium 未实现留 TODO）
// Ref: GlazedTerracottaBlock.cpp:59-64（getStateForPlacement facing=opposite(horizontalDirection)，对齐 vanilla）
// Ref: GlazedTerracottaBlock.java:28（getHorizontalDirection().getOpposite()，箭头背离玩家）
// Ref: BlockItemUseContext.cpp:111-125（horizontalDirection 由 playerYaw 计算：0=South/90=West/180=North/270=East）
// Ref: SimulatedPlayer.cpp:90-106（lookAtLocation yaw=atan2(-dx,dz)，瞬时 setRotation）
// Ref: ColoredBlocks.cpp:431-467（16 色带釉陶瓦方块注册）
// Ref: BlockItemRegistry.cpp（16 色 glazed_terracotta 物品注册，本期补全）
// Ref: FurnaceTests.ts:175-180（facing=opposite(horizontalDirection) 4 朝向坐标配方，本组复用）
// Ref: GrindstoneTests.ts（破坏不崩溃范式：setBlockType air + 断言变 air，不验证掉落物实体）
// Ref: BigDripleafTests.ts（setBlockWithStates 预置 state + getState 读 C++ 内部属性名范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/4：带釉陶瓦 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 3：带釉陶瓦 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

// 被测代表色：白色带釉陶瓦。16 色共享同一 GlazedTerracottaBlock 类与 getStateForPlacement，一色覆盖即可。
const GLAZED_TYPE = "minecraft:white_glazed_terracotta";

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BigDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 带釉陶瓦 facing state（方向名字符串 "north"/"south"/"east"/"west"）。返回 null 表示失败或非带釉陶瓦。
// 注意：HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（DirectionProperty::createHorizontal("facing")），
// getState 按内部名匹配，返回方向名字符串（Java 命名，小写）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 朝向放置映射表：玩家朝向 → 带釉陶瓦 facing（facing=opposite(玩家朝向)，同 FurnaceTests 语义）。
// horizontalDirection（BlockItemUseContext.cpp:117-124）：yaw∈[315,360)∪[0,45)→South，
// [45,135)→West，[135,225)→North，[225,315)→East。带釉陶瓦 facing=opposite(horizontalDirection)。
// lookAtLocation yaw=atan2(-dx,dz)（SimulatedPlayer.cpp:99）：0→South,90→West,180→North,270→East。
interface FacingCase {
    name: string; // 玩家朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（结构相对，不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（产生目标 yaw，坐标在 [0,6] 内）
    expectedFacing: string; // 带釉陶瓦 facing=opposite(玩家朝向)
}

// 4 朝向逐一推算（玩家位置 + lookAt 目标 → yaw → horizontalDirection → facing=opposite）：
//   South（yaw∈[315,360)∪[0,45)→facing=North）：玩家 (1,2,1)，lookAt (3,2,5)，
//     dx=2,dz=4→atan2(-2,4)≈-26.6°→+360=333°∈[315,360)→South→facing=North。
//   West（yaw∈[45,135)→facing=East）：玩家 (1,2,1)，lookAt (0,2,1)，
//     dx=-1,dz=0→atan2(1,0)=90°∈[45,135)→West→facing=East。
//   North（yaw∈[135,225)→facing=South）：玩家 (1,2,5)，lookAt (3,2,1)，
//     dx=2,dz=-4→atan2(-2,-4)≈153°∈[135,225)→North→facing=South。
//   East（yaw∈[225,315)→facing=West）：玩家 (1,2,1)，lookAt (6,2,1)，
//     dx=5,dz=0→atan2(-5,0)=-90°→+360=270°∈[225,315)→East→facing=West。
// 玩家位置均不与 (3,1,1)/(3,2,1) 重叠；lookAt 目标均在 [0,6] 内不越界。
// 坐标配方同 FurnaceTests FACING_CASES（带釉陶瓦与熔炉 facing 语义一致：opposite(horizontalDirection)，
// 仅 yaw，lookAt.y=playerPos.y）。
const FACING_CASES: FacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 2, z: 5 }, expectedFacing: "north" },
    { name: "west", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 0, y: 2, z: 1 }, expectedFacing: "east" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 2, z: 1 }, expectedFacing: "south" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 2, z: 1 }, expectedFacing: "west" },
];

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放带釉陶瓦位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：facing=opposite(玩家朝向) 放置——4 朝向逐一验证（验证修复 getStateForPlacement 加 opposite）。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持
//   white_glazed_terracotta useItemOnBlock 点击 (3,1,1) stone 顶面 Up → placementPos=(3,2,1)（stone 不可
//   替换 → 相邻位置上方 air）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)) →
//   setBlockState 放带釉陶瓦 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家朝向)）。
//
// 此场景验证 wiki「放置时纹理随玩家朝向旋转」+ getStateForPlacement facing=opposite(horizontalDirection)：
//   玩家朝 South→带釉陶瓦 facing=North，朝 West→facing=East，朝 North→facing=South，朝 East→facing=West。
//   用 lookAtLocation 控制 yaw（复用 FurnaceTests opposite 范式坐标）。每朝向用新 player 避免 yaw 残留；
//   每次清理 (3,2,1) 避免带釉陶瓦残留阻断放置。本场景是修复 getStateForPlacement 加 opposite 的回归验证
//   （修复前 facing=horizontalDirection 同向，会与 expectedFacing 相反导致断言失败）。
function glazedTerracottaFacingOppositePlayerFacing(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        // 每朝向独立 spawn 玩家（避免 yaw 残留）。
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 带釉陶瓦 facing=opposite。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向带釉陶瓦残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 white_glazed_terracotta 点击 (3,1,1) stone 顶面 Up → 带釉陶瓦落 (3,2,1)。
        // getStateForPlacement facing=opposite(horizontalDirection(yaw))。
        const glazedItem = new ItemStack(GLAZED_TYPE, 1);
        const used = player.useItemOnBlock(
            glazedItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing glazed terracotta facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言带釉陶瓦 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === GLAZED_TYPE, `glazed terracotta should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `glazed terracotta facing should be ${c.expectedFacing} (opposite of player facing ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：物品放置链路——useItemOnBlock white_glazed_terracotta 放置 → typeId + facing 默认 North。
//
// 布局：(3,1,1) stone（被点击方块）。玩家 (1,2,1) 默认 yaw=0（朝 South，spawn 未设 yaw）。手持
//   white_glazed_terracotta useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement facing=opposite(South)=North → setBlockState 放带釉陶瓦。
//
// 判定：(3,2,1) typeId === "minecraft:white_glazed_terracotta" 且 facing==="north"（默认 yaw=0→South→
//   opposite=North）。验证 16 色物品注册补全后放置链路可用（修复前物品未注册，useItemOnBlock 无法放置）。
//
// 此场景验证带釉陶瓦物品放置链路 + getStateForPlacement 朝向（默认朝向 North）：物品已注册
//   （useItemOnBlock 成功放置）+ facing=North（默认 yaw=0→South→opposite=North）。场景 1 已详测 4 朝向，
//   此场景聚焦物品放置链路可用性（回归验证物品注册补全）。
function glazedTerracottaItemPlacement(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "placer");
    const glazedItem = new ItemStack(GLAZED_TYPE, 1);

    // 手持 white_glazed_terracotta 点击 (3,1,1) 顶面 Up → 带釉陶瓦落 (3,2,1)。
    // getStateForPlacement facing=opposite(South)=North（默认 yaw=0→South）。
    const used = player.useItemOnBlock(
        glazedItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing glazed terracotta");

    // 判定：带釉陶瓦 (3,2,1) 已放置，facing=north（默认 yaw=0→South→opposite=North）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === GLAZED_TYPE, `glazed terracotta should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "north", `facing should be north after placement (default yaw=0→South→opposite=North), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：facing state 读写——预置 facing=east → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 带釉陶瓦（setBlockWithStates 预置 facing=east，绕过物品放置直接写 state）。
//
// 判定：getState("facing")==="east"（验证 facing state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「带釉陶瓦 HORIZONTAL_FACING（水平朝向）state」可读写：setBlockWithStates 预置
//   facing=east 后 getState 可读。与场景 1/2（物品放置 facing=opposite(朝向)）互补，验证 state 直接写入路径。
function glazedTerracottaStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=east（从默认 state 出发逐属性应用）。
    (test as TestWithStates).setBlockWithStates(GLAZED_TYPE, { x: 3, y: 2, z: 1 }, "facing=east");
    test.assert(getBlockTypeId(test, 3, 2, 1) === GLAZED_TYPE, `glazed terracotta should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===east（state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "east", `facing should be east after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：破坏不崩溃——放带釉陶瓦 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 带釉陶瓦（setBlockType 放置）。
// setBlockType("minecraft:air", (3,2,1)) 破坏带釉陶瓦 → 纯装饰方块无 BlockEntity → 基类
//   Block::onBlockRemoved（Block.cpp:516-523）空操作 → 位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（带釉陶瓦已破坏，链路不崩溃）。
//
// 此场景验证带釉陶瓦破坏链路安全性：放带釉陶瓦后破坏，基类 onBlockRemoved 空操作不崩溃，位置正确变 air。
//   破坏掉落物（wiki :107 破坏掉落自身）非确定（项目范式不验证掉落物实体，见 GrindstoneTests/BarrelTests），
//   故仅测变 air，不验物品实体。TODO: 待脚本侧破坏掉落物测试范式完善后补 glazed_terracotta_drops_itself。
function glazedTerracottaBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType(GLAZED_TYPE, { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === GLAZED_TYPE, `glazed terracotta should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏带釉陶瓦 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言带釉陶瓦 (3,2,1) 已破坏变 air（链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `glazed terracotta pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerGlazedTerracottaTests(): void {
    GameTest.register("BlockBehaviorTests", "glazed_terracotta_facing_opposite_player_facing", glazedTerracottaFacingOppositePlayerFacing)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "glazed_terracotta_item_placement", glazedTerracottaItemPlacement)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "glazed_terracotta_state_readable", glazedTerracottaStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "glazed_terracotta_breaks_when_removed", glazedTerracottaBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
