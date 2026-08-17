// 熔炉（furnace）朝向放置、LIT 默认、state 读写与破坏行为 GameTest。
//
// wiki tech_熔炉.txt#用途/光源/破坏/历史：
//   - 历史java Alpha v1.2.0（:229）：「现在熔炉在放置时会朝向玩家，而不是朝向其他方向」——即熔炉
//     facing=opposite(玩家水平朝向)，熔炉正面朝向玩家（wiki 明言放置朝向玩家）。
//   - 光源（:20, :93）：燃烧中的熔炉发出亮度 13 的光（LIT=true→13，LIT=false→0）。发光行为已由
//     lighting 包 light_furnace_lit_emits_13 覆盖，本组不重复测发光，聚焦 facing 放置 + LIT 默认 false。
//   - 破坏（:64）：熔炉被破坏后掉落自身、内容物和储存的经验值（容器破坏掉落内容物）。
//   - 烧炼（:79）：200 tick 烧炼一个物品（涉燃料/烧炼物放入 + tick 计时，非确定，不测）。
//   - 红石比较器（:90）：检测熔炉内容物占空比（烧炼槽/燃料槽/成品槽，wiki :90）。
//
// C++ 链路：AbstractFurnaceBlock（blocks/AbstractFurnaceBlock.cpp）两个 state：
//   - HORIZONTAL_FACING（C++ 属性名 "facing"，DirectionProperty::createHorizontal("facing")，
//     Properties.hpp，默认 North，构造函数 :66-68 setDefaultState facing=North,lit=false）。
//   - LIT（C++ 属性名 "lit"，BooleanProperty，默认 false）。
//   - getStateForPlacement（:73-82）：facing=opposite(context.horizontalDirection())，LIT=false 强制。
//     即熔炉正面朝向玩家（horizontalDirection 是玩家朝向，opposite 即熔炉面朝玩家）。
//   - horizontalDirection 由 playerYaw 计算（BlockItemUseContext.cpp:111-125）：
//     yaw∈[315,360)∪[0,45)→South，[45,135)→West，[135,225)→North，[225,315)→East。
//   - getLightLevel（:86-90）：LIT?13:0（发光行为，lighting 包已测）。
//   - hasBlockEntity()=true（:103），FurnaceBlock::createBlockEntity（FurnaceBlock.cpp:43-46）返
//     FurnaceEntity。放置熔炉 setBlockState 触发 BlockEntity 创建。
//   - onBlockActivated（:94-111）：interactWith→openContainer（FurnaceBlock.cpp:48-55 openContainer
//     ContainerType::Furnace）。SimulatedPlayer 无 GUI 放物品 API，无法测烧炼/内容物。
//   - 未 override onBlockRemoved（走基类 Block::onBlockRemoved 空操作，Block.cpp:516-523）——
//     即 Cubium 熔炉破坏时不掉落内容物（与 vanilla 偏差，vanilla 经 playerWillDestroy/BlockEntity
//     掉落内容物）。但空熔炉无内容物，破坏走基类空操作 → 位置变 air，安全可测。内容物破坏掉落
//     偏差需有内容物的熔炉才能验证，SimulatedPlayer 无法放入，留 TODO。
//   - 物品注册：BlockItemRegistry.cpp:338 registerSimpleBlock(FURNACE, "furnace")，
//     BuildingBlocks.cpp:207 registerBlock<FurnaceBlock>。物品已注册，可 useItemOnBlock 放置。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 furnace 物品点击
//   stone 顶面 → onBlockActivated 基类 Pass（stone 非熔炉，targetBlock 非 furnace 走基类 Pass）→
//   fallback Item.useOn → BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext（playerYaw 来自
//   SimulatedPlayer yaw()，:325）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)),
//   LIT=false → setBlockState 放熔炉（hasBlockEntity 触发 BlockEntity 创建）。创造模式 furnace 不消耗。
//
// 朝向控制（block_behavior 包首个朝向控制测试，建立范式）：
//   SimulatedPlayer spawn 默认 yaw=0（朝 South，EntityRotationComponent 默认 Vector2{0,0}，
//   SimulatedPlayer.cpp:42-62 spawn 未调 setRotation）。脚本侧暴露 lookAtLocation(blockPos)（已实现
//   原生方法，非 stub，ScriptSimulatedPlayer.cpp:221-238），瞬时设 yaw=atan2(-dx,dz)（MC 约定
//   yaw=0→+Z/South, 90→-X/West, 180→-Z/North, 270→+X/East，SimulatedPlayer.cpp:99）。lookAtLocation
//   接收结构相对 BlockPos，经 worldBlockPosition 转世界绝对坐标算 dx/dz。
//   通过 lookAtLocation 传入合适相对坐标可精确控制 yaw 到 4 个朝向，进而控制熔炉 facing：
//     - yaw=0(South)→facing=North；yaw=90(West)→facing=East；
//     - yaw=180(North)→facing=South；yaw=270(East)→facing=West。
//   此前 block_behavior 包无任何测试调用朝向控制（BellTests/BannerTests/PumpkinTests 均因 yaw 不可控
//   跳过朝向测试留 TODO）。本组首次实际使用 lookAtLocation 控制朝向，为后续 Bell/Banner/Pumpkin 朝向
//   测试建立范式。
//
// 测试覆盖（4 个场景，覆盖 wiki 朝向放置 + LIT 默认 + state 读写 + 破坏核心确定行为）：
//   1. facing=opposite(玩家朝向) 放置（4 朝向）：玩家 lookAtLocation 控制朝向 → useItemOnBlock
//      furnace → 断言 facing=opposite(朝向)。4 朝向逐一验证 South→North/West→East/North→South/
//      East→West 映射（建立朝向控制范式）。
//   2. 放置 LIT=false 默认：useItemOnBlock furnace 放置 → LIT=false（getStateForPlacement 强制）。
//   3. facing/lit state 读写：setBlockWithStates 预置 facing=east,lit=false → getState 可读。
//   4. 熔炉破坏不崩溃：放熔炉（BlockEntity 创建）→ setBlockType air 破坏 → 位置变 air（基类
//      onBlockRemoved 空操作，链路安全）。
//
// 关键约束：
// 1. 场景 1 朝向控制：每个朝向独立 spawn 玩家在合适位置（不与 (3,1,1)/(3,2,1) 重叠），lookAtLocation
//    传入不越界（[0,6]）的目标坐标产生目标 yaw，再 useItemOnBlock furnace 点击 (3,1,1) stone Up →
//    熔炉落 (3,2,1)。4 朝向玩家位置/lookAt 目标/yaw/facing 映射见场景 1 注释。
// 2. 场景 2 用 useItemOnBlock 放置：手持 furnace 点击 (3,1,1) stone 顶面 Up → placementPos=(3,2,1)
//    （stone 不可替换 → 相邻位置上方 air）→ getStateForPlacement LIT=false → setBlockState 放熔炉。
//    断言 typeId=furnace + LIT=false。
// 3. 场景 3 用 setBlockWithStates 预置 facing=east,lit=false（绕过物品放置，直接写 state）。getState
//    读 "facing"/"lit"（C++ 内部属性名）。验证双 state 可读写。
// 4. 场景 4 放熔炉后 setBlockType air 破坏：基类 onBlockRemoved 空操作（无内容物掉落），位置变 air。
//    断言位置变 air（破坏成功，BlockEntity 创建/移除链路不崩溃）。空熔炉无内容物可掉，不验物品实体。
// 5. 读 facing 用 getState("facing" as any)（DirectionProperty 序列化为方向名字符串 "north"/"south"/
//    "east"/"west"，与 Java 命名一致）。读 lit 用 getState("lit" as any)（BooleanProperty 序列化为 bool）。
// 6. 熔炉有碰撞箱（固体方块），但 isValidPosition 基类 true 无支撑要求。stone 支撑仅为贴近真实放置。
// 7. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「烧炼」：涉燃料/烧炼物放入 + 200 tick 计时 + 烧炼配方，SimulatedPlayer 无 GUI 放物品 API，
//   非确定且复杂，跳过。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 furnace_smelts_item。
// 不测「发光 13」：已由 lighting 包 light_furnace_lit_emits_13 覆盖（DynamicEmissionTests.ts:133），
//   不重复。
// 不测「比较器信号」：需比较器贴熔炉 + 读输出信号，链路复杂且 GameTest 读红石信号 API 受限，跳过。
//   TODO: 待比较器读容器信号测试范式完善后补 furnace_comparator_signal。
// 不测「内容物破坏掉落」：SimulatedPlayer 无 GUI 放物品 API，无法向熔炉槽位放入物品（onBlockActivated
//   openContainer 仅打开 GUI，SimulatedPlayer 无后续放入操作），无法构造有内容物的熔炉。且 Cubium
//   AbstractFurnaceBlock 未 override onBlockRemoved（基类空操作），破坏不掉落内容物（与 vanilla 偏差）。
//   场景 4 仅测空熔炉破坏不崩溃。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补
//   furnace_drops_contents_when_broken，并修复 AbstractFurnaceBlock onBlockRemoved 内容物掉落偏差。
// 不测「猫坐熔炉」：涉猫 AI，非确定，跳过。
// 不测「活塞推动」：基岩可推动/Java 不可推动是 JE/BE 不一致（wiki :67 {{in|Java}}熔炉不能被活塞推动），
//   不为不一致行为写测试。
//
// 跨服务端：furnace 方块名两端一致。facing/lit state 名两端一致（C++ 内部名 "facing"/"lit"，Java 命名）。
//   朝向放置（facing=opposite(玩家朝向)）+ LIT 默认 false + state 读写 + 破坏行为两端与 vanilla 一致。
//   lookAtLocation 是 Cubium 专有朝向控制（基岩 GameTest 无此 API），但 facing=opposite(朝向) 放置行为
//   本身两端可对比（基岩用真实玩家朝向放置），非 one-sided。setBlockWithStates 预置 state 是 Cubium
//   专有写入，但 state 行为本身两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熔炉.txt#历史 java Alpha v1.2.0（放置朝向玩家）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熔炉.txt#光源（燃烧发光 13）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熔炉.txt#破坏（掉落自身/内容物/经验）
// Ref: AbstractFurnaceBlock.cpp（getStateForPlacement facing=opposite(朝向)+LIT=false / getLightLevel LIT?13:0）
// Ref: BlockItemUseContext.cpp:111-125（horizontalDirection 由 playerYaw 计算：0=South/90=West/180=North/270=East）
// Ref: SimulatedPlayer.cpp:90-106（lookAtLocation yaw=atan2(-dx,dz)，瞬时 setRotation）
// Ref: FurnaceBlock.cpp（createBlockEntity FurnaceEntity / interactWith openContainer）
// Ref: Block.cpp:516-523（基类 onBlockRemoved 空操作，AbstractFurnaceBlock 未 override）
// Ref: BlockItemRegistry.cpp:338（registerSimpleBlock FURNACE "furnace"）
// Ref: DynamicEmissionTests.ts:133（lighting 包 light_furnace_lit_emits_13 已覆盖发光）
// Ref: BrewingStandTests.ts（容器破坏不崩溃范式：setBlockType air + 断言变 air）
// Ref: BigDripleafTests.ts（setBlockWithStates 预置 state + getState 读 C++ 内部属性名范式）
// Ref: BellTests.ts:60 / BannerTests.ts:69（朝向控制 TODO 先例，本组首次实际使用 lookAtLocation）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2/4：熔炉 (3,2,1)，下方 (3,1,1) stone 支撑。
// 场景 3：熔炉 (3,2,1)（setBlockWithStates 预置 state），下方 (3,1,1) stone 支撑。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BigDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 熔炉 facing state（方向名字符串 "north"/"south"/"east"/"west"）。返回 null 表示失败或非熔炉。
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

// 读取 (x,y,z) 熔炉 lit state（bool）。返回 null 表示失败或非熔炉。
// 注意：LIT() 的 C++ 属性名为 "lit"（BooleanProperty::create("lit")）。
function getLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 朝向放置映射表：玩家朝向 → 熔炉 facing（facing=opposite(玩家朝向)）。
// horizontalDirection（BlockItemUseContext.cpp:117-124）：yaw∈[315,360)∪[0,45)→South，
// [45,135)→West，[135,225)→North，[225,315)→East。熔炉 facing=opposite(horizontalDirection)。
// lookAtLocation yaw=atan2(-dx,dz)（SimulatedPlayer.cpp:99）：0→South,90→West,180→North,270→East。
interface FacingCase {
    name: string; // 玩家朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（结构相对，不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（产生目标 yaw，坐标在 [0,6] 内）
    expectedFacing: string; // 熔炉 facing=opposite(玩家朝向)
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
const FACING_CASES: FacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 2, z: 5 }, expectedFacing: "north" },
    { name: "west", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 0, y: 2, z: 1 }, expectedFacing: "east" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 2, z: 1 }, expectedFacing: "south" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 2, z: 1 }, expectedFacing: "west" },
];

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放熔炉位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：facing=opposite(玩家朝向) 放置——4 朝向逐一验证（block_behavior 包首个朝向控制测试）。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持
//   furnace useItemOnBlock 点击 (3,1,1) stone 顶面 Up → placementPos=(3,2,1)（stone 不可替换 →
//   相邻位置上方 air）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)),LIT=false →
//   setBlockState 放熔炉 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家朝向)）。
//
// 此场景验证 wiki「熔炉放置朝向玩家」+ getStateForPlacement facing=opposite(horizontalDirection)：
//   玩家朝 South→熔炉 facing=North，朝 West→facing=East，朝 North→facing=South，朝 East→facing=West。
//   用 lookAtLocation 控制 yaw（block_behavior 包首次实际使用朝向控制，建立范式）。每朝向用新 player
//   避免 yaw 残留；每次清理 (3,2,1) 避免熔炉残留阻断放置。
function furnaceFacingOppositePlayerFacing(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        // 每朝向独立 spawn 玩家（避免 yaw 残留）。
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 熔炉 facing=opposite。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向熔炉残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 furnace 点击 (3,1,1) stone 顶面 Up → 熔炉落 (3,2,1)。
        // getStateForPlacement facing=opposite(horizontalDirection(yaw)), LIT=false。
        const furnaceItem = new ItemStack("minecraft:furnace", 1);
        const used = player.useItemOnBlock(
            furnaceItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing furnace facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言熔炉 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:furnace", `furnace should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `furnace facing should be ${c.expectedFacing} (opposite of player facing ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：放置 LIT=false 默认——useItemOnBlock furnace 放置 → LIT=false（getStateForPlacement 强制）。
//
// 布局：(3,1,1) stone（被点击方块）。玩家 (1,2,1) 默认 yaw=0（朝 South，spawn 未设 yaw）。手持 furnace
//   useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) → getStateForPlacement LIT=false →
//   setBlockState 放熔炉（hasBlockEntity 触发 BlockEntity 创建）。
//
// 判定：(3,2,1) typeId === "minecraft:furnace" 且 LIT===false（放置 state 确定，未燃烧）。
//
// 此场景验证熔炉物品放置链路 + getStateForPlacement 强制 LIT=false（放置时未燃烧）：物品已注册
//   （useItemOnBlock 成功放置）+ LIT=false（放置时无燃料，未燃烧）+ hasBlockEntity 触发 BlockEntity
//   创建不崩溃。facing 由默认 yaw=0→South→facing=North（场景 1 已详测 4 朝向，此场景聚焦 LIT=false）。
function furnacePlacedWithLitFalse(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const furnaceItem = new ItemStack("minecraft:furnace", 1);

    // 手持 furnace 点击 (3,1,1) 顶面 Up → 熔炉落 (3,2,1)。getStateForPlacement LIT=false。
    const used = farmer.useItemOnBlock(
        furnaceItem as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing furnace");

    // 判定：熔炉 (3,2,1) 已放置，LIT=false（getStateForPlacement 强制，放置时未燃烧）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:furnace", `furnace should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getLit(test, 3, 2, 1) === false, `lit should be false after placement, got ${getLit(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：facing/lit state 读写——预置 facing=east,lit=false → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 熔炉（setBlockWithStates 预置 facing=east,lit=false，绕过物品
//   放置直接写 state）。
//
// 判定：getState("facing")==="east" 且 getState("lit")===false（验证 facing/lit 双 state 经
//   setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「熔炉 FACING（水平朝向）/ LIT（点燃）双 state」可读写：setBlockWithStates 预置
//   facing=east,lit=false 后 getState 双 state 均可读。不测「燃烧 LIT 翻转」（涉燃料放入 + tick，跳过）。
function furnaceStateReadable(test: Test): void {
    placeStoneSupport(test);
    // setBlockWithStates 预置 facing=east,lit=false（从默认 state 出发逐属性应用）。
    (test as TestWithStates).setBlockWithStates("minecraft:furnace", { x: 3, y: 2, z: 1 }, "facing=east,lit=false");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:furnace", `furnace should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 facing===east 且 lit===false（双 state 可读写）。
    test.assert(getFacing(test, 3, 2, 1) === "east", `facing should be east after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);
    test.assert(getLit(test, 3, 2, 1) === false, `lit should be false after setBlockWithStates, got ${getLit(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：熔炉破坏不崩溃——放熔炉（BlockEntity 创建）→ setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 熔炉（setBlockType 放置，BlockEntity 创建）。
// setBlockType("minecraft:air", (3,2,1)) 破坏熔炉 → AbstractFurnaceBlock 未 override onBlockRemoved
//   → 基类 Block::onBlockRemoved（Block.cpp:516-523）空操作 → 位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（熔炉已破坏，BlockEntity 创建/移除链路不崩溃）。
//
// 此场景验证熔炉容器破坏链路安全性：放熔炉（BlockEntity 创建）后破坏，基类 onBlockRemoved 空操作
//   不崩溃，位置正确变 air。空熔炉无内容物可掉落（SimulatedPlayer 无 GUI 放物品 API，无法构造有内容物
//   的熔炉；且 Cubium AbstractFurnaceBlock 未 override onBlockRemoved 不掉落内容物，见文件头 TODO），
//   故仅测空破坏不崩溃，不验物品实体。
function furnaceBreaksWhenRemoved(test: Test): void {
    placeStoneSupport(test);
    test.setBlockType("minecraft:furnace", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:furnace", `furnace should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏熔炉 → 基类 onBlockRemoved 空操作 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言熔炉 (3,2,1) 已破坏变 air（BlockEntity 创建/移除链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `furnace pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerFurnaceTests(): void {
    GameTest.register("BlockBehaviorTests", "furnace_facing_opposite_player_facing", furnaceFacingOppositePlayerFacing)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "furnace_placed_with_lit_false", furnacePlacedWithLitFalse)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "furnace_state_readable", furnaceStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "furnace_breaks_when_removed", furnaceBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
