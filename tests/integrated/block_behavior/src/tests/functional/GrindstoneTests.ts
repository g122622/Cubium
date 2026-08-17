// 砂轮（grindstone）放置朝向、附着面与破坏行为 GameTest。
//
// wiki tech_砂轮.txt#用途/破坏：
//   - 砂轮是可用于修补损坏物品并移除魔咒的方块，按使用键打开界面（GUI 容器操作，SimulatedPlayer 无 API，不测）。
//   - 砂轮可附着在地面（floor）、墙面（wall）或天花板（ceiling），由玩家点击的面决定附着方式。
//   - 砂轮被破坏后掉落自身（破坏掉落物非确定，本组只测方块自身被移除变 air 不崩溃）。
//   - 砂轮不能被活塞移动（跨子系统，不测）。
//   注：wiki「BE 砂轮会在附着方块移除时被破坏」是基岩独有特性，Java 版砂轮 canSurvive 恒 true 不自毁。
//   Cubium GrindstoneBlock.updatePostPlacement 实现了附着支撑移除自毁（偏离 Java，对齐 BE），本组不测自毁
//   （避免 JE/BE 不一致行为），留 TODO 标注此偏差。
//
// C++ 链路：GrindstoneBlock（functional/GrindstoneBlock.cpp）两个 state：
//   - HORIZONTAL_FACING（C++ 属性名 "facing"，水平 4 向，默认 North）：砂轮朝向。
//   - ATTACH_FACE（C++ 属性名 "face"，枚举 floor/wall/ceiling，默认 wall）：附着面。
//   - getStateForPlacement（:262-285）：三分支判定附着面 + 朝向：
//     · clickedFace==Up → face=floor, facing=horizontalDirection（玩家水平朝向，同向）。
//     · clickedFace==Down → face=ceiling, facing=horizontalDirection（同向）。
//     · 水平面点击 → face=wall, facing=clickedFace（点击面本身）。
//     常规点击场景与 vanilla FaceAttachedHorizontalDirectionalBlock.getStateForPlacement 一致：
//     vanilla 遍历 getNearestLookingDirections（非替换时 [0]=opposite(clickedFace)），Y 轴分支 facing=
//     horizontalDirection（同向），水平分支 facing=opposite(opposite(clickedFace))=clickedFace。
//   - isValidPosition（:287-316）：按 face 检查支撑方块 isSolid（floor→下方、ceiling→上方、wall→背面
//     opposite(facing)）。canPlace 调此判定，支撑缺失则放置失败。
//   - updatePostPlacement（:318-366）：支撑方块变非 solid 时自毁掉落（BE 行为，本组不测）。
//   - 无 BlockEntity（GrindstoneBlock.hpp 未重写 hasBlockEntity/createBlockEntity，与 barrel 不同）。
//   - 物品注册：BlockItemRegistry.cpp registerSimpleBlock(VanillaBlocks::GRINDSTONE, "grindstone")。
//     本提交前砂轮方块/物品均未注册（类已实现但未实例化进方块表），本提交补全注册。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 grindstone 物品点击
//   stone → stone onBlockActivated 基类 Pass → fallback Item.useOn → BlockItem::tryPlace →
//   getStateForPlacement → setBlockState 放砂轮。yaw()/pitch() 来自 SimulatedPlayer（:325）。
//
// 朝向控制（复用 BellTests 的 lookAtLocation 范式）：
//   horizontalDirection（BlockItemUseContext.cpp:111-125）仅读 yaw：yaw∈[315,360)∪[0,45)→South，
//   [45,135)→West，[135,225)→North，[225,315)→East。lookAtLocation yaw=atan2(-dx,dz)
//   （SimulatedPlayer.cpp:99）。砂轮 floor/ceiling 用 horizontalDirection（仅 yaw），故 lookAt.y 不影响
//   facing（pitch 独立），用 lookAt.y=playerPos.y+1 使 pitch≈0（同 BellTests 范式，不影响 horizontalDirection）。
//   wall 分支 facing=clickedFace（点击面），与玩家朝向无关。
//
// 测试覆盖（5 个场景，覆盖 wiki 放置朝向 + 附着面 + state 读写 + 破坏核心确定行为）：
//   1. Floor 4 朝向放置：点击 stone 顶面 Up → face=floor, facing=玩家朝向（同向，4 向 South/West/North/East）。
//   2. Ceiling 朝向放置：点击 stone 底面 Down → face=ceiling, facing=玩家朝向（同向 South）。
//   3. Wall 2 朝向放置：点击 stone 侧面（South/East）→ face=wall, facing=点击面本身。
//   4. face/facing state 读写：setBlockWithStates 预置 face=floor,facing=east → getState 可读。
//   5. 砂轮破坏不崩溃：放砂轮 → setBlockType air 破坏 → 位置变 air。
//
// 关键约束：
// 1. 砂轮 isValidPosition 要求支撑 isSolid：floor 需下方 solid、ceiling 需上方 solid、wall 需背面
//    opposite(facing) solid。canPlace 调此判定，支撑缺失放置失败。所有场景均先放 stone 支撑。
// 2. 场景 1 floor：stone (3,1,1) 顶面 Up → 砂轮落 (3,2,1)，isValidPosition floor 查 (3,1,1) solid ✓。
// 3. 场景 2 ceiling：stone (3,3,1) 底面 Down → 砂轮落 (3,2,1)，isValidPosition ceiling 查 (3,3,1) solid ✓。
// 4. 场景 3 wall：stone 侧面 → 砂轮落侧面外侧，isValidPosition wall 查 opposite(facing)=背面 stone solid ✓。
//    facing=clickedFace，故背面=opposite(clickedFace)=stone 本身（被点击方块），满足支撑。
// 5. 实体碰撞检查（BlockItem.cpp:262-274）：砂轮碰撞箱非空，玩家不能站在 placementPos。玩家位置远离落点。
// 6. 场景 1/2 lookAt.y=playerPos.y+1 使 pitch≈0（horizontalDirection 仅 yaw 故 pitch 不影响 facing，但
//    保持与 BellTests 范式一致）。每朝向用新 player 避免 yaw 残留；每次清理落点避免残留阻断放置。
// 7. 读 face 用 getState("face" as any)（ATTACH_FACE 属性名 "face"，返 "floor"/"wall"/"ceiling"）。
//    读 facing 用 getState("facing" as any)（HORIZONTAL_FACING 属性名 "facing"，返 "north" 等）。
// 8. 砂轮无 BlockEntity，破坏不涉容器遍历，setBlockType air 直接变 air（onBlockRemoved 基类）。
//
// 不测「GUI 修复祛魔」：SimulatedPlayer 无 GUI 放物品/取产物 API，openContainer 链路虽触发但无法验证
//   修复/祛魔产物。TODO: 待脚本侧 BlockEntity 容器操作 API 补全后补 grindstone_disenchant_repair。
// 不测「附着支撑移除自毁」：Cubium updatePostPlacement 实现自毁是 BE 行为，Java 版 canSurvive 恒 true
//   不自毁（JE/BE 不一致）。本组不为 JE/BE 不一致行为写测试。TODO: 待明确 Cubium 取 JE 还是 BE 语义后补
//   grindstone_support_removal_behavior（若取 JE 应删除 updatePostPlacement 自毁，若取 BE 应测自毁）。
// 不测「砂轮不能被活塞推动」：跨活塞子系统，且活塞推动链路复杂，跳过。
// 不测「村民认领砂轮转职武器匠」：涉 AI 寻路/认领，非确定，跳过。
//
// 跨服务端：grindstone 方块名两端一致（minecraft:grindstone），face/facing state 名两端一致
//   （C++ 内部名 "face"/"facing"）。放置朝向（floor/ceiling facing=horizontalDirection 同向，wall
//   facing=clickedFace）+ state 读写 + 破坏行为与 vanilla Java 一致，可跨服务端对比。lookAtLocation 是
//   Cubium 专有朝向控制，但放置 facing 语义两端可对比（基岩用真实玩家视线放置），非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_砂轮.txt#用途（floor/wall/ceiling 放置，GUI 修复祛魔）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_砂轮.txt#破坏（破坏掉落自身）
// Ref: GrindstoneBlock.cpp（getStateForPlacement 三分支 face+facing / isValidPosition 支撑判定）
// Ref: FaceAttachedHorizontalDirectionalBlock.java:38-55（vanilla getStateForPlacement Y轴同向/水平clickedFace）
// Ref: GrindstoneBlock.java:70-72（vanilla canSurvive 恒 true，JE 不自毁——Cubium updatePostPlacement 偏差）
// Ref: BlockItemUseContext.cpp:111-125（horizontalDirection 仅 yaw 映射 South/West/North/East）
// Ref: BlockItem.cpp:219-277（canPlace 调 isValidPosition + 实体碰撞检查）
// Ref: BuildingBlocks.cpp（GRINDSTONE 注册：Material::ROCK, hardness3.0, resistance6.0, requiresTool, Pickaxe）
// Ref: BellTests.ts（Floor/Ceiling 朝向 lookAtLocation 范式：facing=horizontalDirection 同向，lookAt.y=+1）
// Ref: BarrelTests.ts（useItemOnBlock 放置 + state 读写 + 破坏不崩溃范式）
// Ref: LanternTests.ts（支撑自毁范式——砂轮自毁 BE 偏差不测，仅参考 isSolid 支撑语义）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1 floor：stone (3,1,1)，砂轮落 (3,2,1)。
// 场景 2 ceiling：stone (3,3,1)，砂轮落 (3,2,1)。
// 场景 3 wall：stone 侧面，砂轮落侧面外侧（见 WALL_CASES）。
// 场景 4/5：砂轮 (3,2,1)（setBlockWithStates 预置 / setBlockType 放置）。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BarrelTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 砂轮 facing state（小写方向字符串：north/south/east/west）。返回 null 表示失败或非砂轮。
// HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（DirectionProperty 水平 4 向）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 砂轮 face state（附着面枚举名：floor/wall/ceiling）。返回 null 表示失败或非砂轮。
// ATTACH_FACE() 的 C++ 属性名为 "face"（EnumProperty<AttachFace>，返 "floor"/"wall"/"ceiling"）。
function getFace(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("face" as any);
    return typeof value === "string" ? value : null;
}

// Floor/Ceiling 朝向放置映射表（Y 轴点击，facing=horizontalDirection(玩家朝向)，同向非 opposite）。
// GrindstoneBlock.getStateForPlacement Y 轴分支（GrindstoneBlock.cpp:270-280）：
//   clickedFace==Up → face=floor, facing=horizontalDirection；clickedFace==Down → face=ceiling, facing=horizontalDirection。
// horizontalDirection（BlockItemUseContext.cpp:111-125）：yaw∈[315,360)∪[0,45)→South，[45,135)→West，
//   [135,225)→North，[225,315)→East。lookAtLocation yaw=atan2(-dx,dz)（SimulatedPlayer.cpp:99）。
// 注：horizontalDirection 仅读 yaw 不读 pitch，故 lookAt.y 不影响 facing。取 lookAt.y=playerPos.y+1 使
//   pitch≈0（同 BellTests 范式，不影响 horizontalDirection），水平距离≥5 放大 horizDist 使 pitch 微小。
interface YAxisFacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 stone/砂轮落点重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 使 pitch≈0）
    expectedFacing: string; // 砂轮 facing=horizontalDirection(玩家朝向)，同向非 opposite
}

// 4 朝向推算（playerPos.y=2，lookAt.y=3 使 pitch≈0；horizontalDirection 仅 yaw 故 pitch 不影响）：
//   South（yaw[315,360)∪[0,45)→facing=South）：玩家(1,2,1)，lookAt(3,3,6)，dx=2,dz=5→338°→South→facing=south。
//   West（yaw[45,135)→facing=West）：玩家(5,2,1)，lookAt(0,3,1)，dx=-5,dz=0→90°→West→facing=west。
//   North（yaw[135,225)→facing=North）：玩家(1,2,5)，lookAt(3,3,0)，dx=2,dz=-5→158°→North→facing=north。
//   East（yaw[225,315)→facing=East）：玩家(1,2,1)，lookAt(6,3,1)，dx=5,dz=0→270°→East→facing=east。
// facing=玩家朝向（同向），与 furnace/barrel 的 facing=opposite(朝向) 相反——这是砂轮 Y 轴分支用
//   horizontalDirection（非 getNearestLookingDirection().getOpposite()）的特点，同 BellBlock Y 轴分支。
const Y_AXIS_FACING_CASES: YAxisFacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 3, z: 6 }, expectedFacing: "south" },
    { name: "west", playerPos: { x: 5, y: 2, z: 1 }, lookAt: { x: 0, y: 3, z: 1 }, expectedFacing: "west" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 3, z: 0 }, expectedFacing: "north" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 3, z: 1 }, expectedFacing: "east" },
];

// Wall 朝向放置映射表（水平面点击，facing=clickedFace，点击面本身）。
// GrindstoneBlock.getStateForPlacement 水平分支（GrindstoneBlock.cpp:276-280）：
//   face=wall, facing=clickedFace。常规场景与 vanilla 一致（vanilla [0]=opposite(clickedFace)→
//   facing=opposite(opposite(clickedFace))=clickedFace）。facing 与玩家朝向无关，仅由点击面决定。
// 每 case 独立 stone 位置 + 点击面 + 落点，玩家站远离落点处（避免实体碰撞阻断放置）。
interface WallFacingCase {
    name: string; // 点击面名（facing=该面）
    stonePos: { x: number; y: number; z: number }; // 被点击 stone 位置（也是 wall 背面支撑）
    clickedFace: Direction; // 点击面（facing=此面）
    grindstonePos: { x: number; y: number; z: number }; // 砂轮落点（stone.relative(clickedFace)）
    playerPos: { x: number; y: number; z: number }; // 玩家位置（远离落点，避免碰撞；朝向不影响 wall facing）
    expectedFacing: string; // 砂轮 facing=clickedFace
}

// 2 朝向（覆盖 Z 轴 South + X 轴 East，验证 wall 分支两轴判定）：
//   South（Z 轴）：stone(3,2,1) 点击 South → 砂轮落(3,2,2), facing=south。背面 opposite(south)=north=(3,2,1) stone ✓。
//   East（X 轴）：stone(1,2,2) 点击 East → 砂轮落(2,2,2), facing=east。背面 opposite(east)=west=(1,2,2) stone ✓。
// 玩家站远离落点：South case 玩家(3,2,4)（落点(3,2,2) 北侧 2 格外）；East case 玩家(5,2,2)（落点(2,2,2) 东侧 3 格外）。
const WALL_FACING_CASES: WallFacingCase[] = [
    {
        name: "south",
        stonePos: { x: 3, y: 2, z: 1 },
        clickedFace: Direction.South,
        grindstonePos: { x: 3, y: 2, z: 2 },
        playerPos: { x: 3, y: 2, z: 4 },
        expectedFacing: "south",
    },
    {
        name: "east",
        stonePos: { x: 1, y: 2, z: 2 },
        clickedFace: Direction.East,
        grindstonePos: { x: 2, y: 2, z: 2 },
        playerPos: { x: 5, y: 2, z: 2 },
        expectedFacing: "east",
    },
];

// 场景 1：Floor 4 朝向放置——点击 stone 顶面 Up → face=floor, facing=玩家朝向（同向）。
//
// 布局：(3,1,1) stone（被点击方块，砂轮下方支撑）。每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt)
//   设朝向 → 清理 (3,2,1) → 手持 grindstone useItemOnBlock 点击 (3,1,1) 顶面 Up → placementPos=(3,2,1) →
//   getStateForPlacement Y 轴分支：clickedFace=Up→floor, facing=horizontalDirection(玩家朝向) →
//   isValidPosition floor 查 (3,1,1)=stone isSolid ✓ → setBlockState 放砂轮 (3,2,1)。断言 face=floor, facing=expectedFacing。
//
// 判定：4 朝向放置后 (3,2,1) face===floor 且 facing===expectedFacing（玩家朝向，同向非 opposite）。
//
// 此场景验证 wiki「砂轮放上表面（floor）」+ GrindstoneBlock Y 轴 Up 分支 facing=horizontalDirection（玩家朝向
//   同向）。与 furnace/barrel 的 facing=opposite(朝向) 相反——砂轮 Y 轴用 horizontalDirection 而非
//   getNearestLookingDirection().getOpposite()，同 BellBlock Y 轴分支。每朝向用新 player 避免 yaw 残留；
//   每次清理 (3,2,1) 避免砂轮残留阻断放置。
function grindstoneFloorFacingEqualsPlayerDirection(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of Y_AXIS_FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 砂轮 facing=玩家朝向（同向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向砂轮残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 grindstone 点击 (3,1,1) stone 顶面 Up → 砂轮落 (3,2,1)。
        // getStateForPlacement Y 轴分支：clickedFace=Up→floor, facing=horizontalDirection(玩家朝向)。
        const grindstoneItem = new ItemStack("minecraft:grindstone", 1);
        const used = player.useItemOnBlock(
            grindstoneItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing floor grindstone facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言砂轮 (3,2,1) 已放置且 face=floor, facing=玩家朝向（同向非 opposite）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:grindstone", `grindstone should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        test.assert(getFace(test, 3, 2, 1) === "floor", `grindstone face should be floor, got ${getFace(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `grindstone facing should be ${c.expectedFacing} (player direction, not opposite), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：Ceiling 朝向放置——点击 stone 底面 Down → face=ceiling, facing=玩家朝向（同向 South）。
//
// 布局：(3,3,1) stone（被点击方块，砂轮上方天花板支撑）。玩家 (1,2,1) 朝南 lookAtLocation({3,3,6})
//   （yaw≈338°→South）。手持 grindstone useItemOnBlock 点击 (3,3,1) 底面 Down → placementPos=(3,2,1)
//   （stone 下方）→ getStateForPlacement Y 轴分支：clickedFace=Down→ceiling, facing=horizontalDirection(South) →
//   isValidPosition ceiling 查 (3,3,1)=stone isSolid ✓ → setBlockState 放砂轮 (3,2,1)。
//
// 判定：(3,2,1) face===ceiling 且 facing===south（玩家朝向 South，同向）。
//
// 此场景验证 wiki「砂轮放底面下方（ceiling）」+ GrindstoneBlock Y 轴 Down 分支 facing=horizontalDirection。
//   仅测 1 朝向（South）聚焦 ceiling 分支判定 + facing 同向；4 朝向已在场景 1 floor 验证（ceiling 同逻辑）。
function grindstoneCeilingFacingEqualsPlayerDirection(test: Test): void {
    // (3,3,1) stone（砂轮在其下方，被点击底面的方块，ceiling 支撑须 isSolid）。
    test.setBlockType("minecraft:stone", { x: 3, y: 3, z: 1 });
    test.assert(getBlockTypeId(test, 3, 3, 1) === "minecraft:stone", `stone should be at (3,3,1), got ${getBlockTypeId(test, 3, 3, 1)}`);
    // 清理砂轮落点 (3,2,1)。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_south");
    // 朝南：lookAt (3,3,6)，dx=2,dz=5→338°→South。
    player.lookAtLocation({ x: 3, y: 3, z: 6 });

    // 手持 grindstone 点击 (3,3,1) 底面 Down → 砂轮落 (3,2,1)（stone 下方）。
    // getStateForPlacement Y 轴分支：clickedFace=Down→ceiling, facing=horizontalDirection(South)。
    const grindstoneItem = new ItemStack("minecraft:grindstone", 1);
    const used = player.useItemOnBlock(
        grindstoneItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 3, z: 1 },
        Direction.Down,
    );
    test.assert(used, "useItemOnBlock should return true when placing ceiling grindstone");

    // 断言砂轮 (3,2,1) 已放置且 face=ceiling, facing=south（玩家朝向，同向）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:grindstone", `grindstone should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getFace(test, 3, 2, 1) === "ceiling", `grindstone face should be ceiling, got ${getFace(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "south", `grindstone facing should be south (player direction, not opposite), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：Wall 2 朝向放置——点击 stone 侧面 → face=wall, facing=点击面本身。
//
// 布局：每 case 独立 stone 位置 + 点击面。手持 grindstone useItemOnBlock 点击 stone 侧面（clickedFace）→
//   placementPos=stone.relative(clickedFace) → getStateForPlacement 水平分支：face=wall, facing=clickedFace →
//   isValidPosition wall 查 opposite(facing)=opposite(clickedFace)=stone 本身 isSolid ✓ → setBlockState 放砂轮。
//
// 判定：每 case 砂轮落点 face===wall 且 facing===expectedFacing（点击面本身）。
//
// 此场景验证 wiki「砂轮放墙面（wall）」+ GrindstoneBlock 水平分支 facing=clickedFace（点击面本身，非
//   opposite）。与 Y 轴分支 facing=horizontalDirection(玩家朝向) 对照——水平点击朝向取点击面，垂直点击
//   朝向取玩家朝向同向。2 朝向覆盖 Z 轴(South) + X 轴(East) 验证 wall 分支两轴判定。玩家朝向不影响 wall
//   facing（facing=clickedFace），玩家位置仅须远离落点避免实体碰撞。
function grindstoneWallFacingEqualsClickedFace(test: Test): void {
    for (const c of WALL_FACING_CASES) {
        // 放被点击 stone（也是 wall 背面支撑，opposite(clickedFace)=stone 本身须 isSolid）。
        test.setBlockType("minecraft:stone", c.stonePos);
        test.assert(getBlockTypeId(test, c.stonePos.x, c.stonePos.y, c.stonePos.z) === "minecraft:stone", `stone should be at (${c.stonePos.x},${c.stonePos.y},${c.stonePos.z}), got ${getBlockTypeId(test, c.stonePos.x, c.stonePos.y, c.stonePos.z)}`);
        // 清理砂轮落点。
        test.setBlockType("minecraft:air", c.grindstonePos);

        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // 玩家朝向不影响 wall facing（facing=clickedFace），lookAtLocation 仅自然朝向（避免 spawn 默认朝向）。
        player.lookAtLocation(c.stonePos);

        // 手持 grindstone 点击 stone 侧面 clickedFace → 砂轮落 grindstonePos。
        // getStateForPlacement 水平分支：face=wall, facing=clickedFace。
        const grindstoneItem = new ItemStack("minecraft:grindstone", 1);
        const used = player.useItemOnBlock(
            grindstoneItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            c.stonePos,
            c.clickedFace,
        );
        test.assert(used, `useItemOnBlock should return true when placing wall grindstone facing ${c.expectedFacing} (clicked face ${c.name})`);

        // 断言砂轮落点 face=wall, facing=点击面本身（非 opposite）。
        test.assert(getBlockTypeId(test, c.grindstonePos.x, c.grindstonePos.y, c.grindstonePos.z) === "minecraft:grindstone", `grindstone should be placed at (${c.grindstonePos.x},${c.grindstonePos.y},${c.grindstonePos.z}) for ${c.name}, got ${getBlockTypeId(test, c.grindstonePos.x, c.grindstonePos.y, c.grindstonePos.z)}`);
        test.assert(getFace(test, c.grindstonePos.x, c.grindstonePos.y, c.grindstonePos.z) === "wall", `grindstone face should be wall for ${c.name}, got ${getFace(test, c.grindstonePos.x, c.grindstonePos.y, c.grindstonePos.z)}`);
        const facing = getFacing(test, c.grindstonePos.x, c.grindstonePos.y, c.grindstonePos.z);
        test.assert(facing === c.expectedFacing, `grindstone facing should be ${c.expectedFacing} (clicked face, not opposite) for ${c.name}, got ${facing}`);
    }

    test.succeed();
}

// 场景 4：face/facing state 读写——setBlockWithStates 预置 face=floor,facing=east → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 砂轮（setBlockWithStates 预置 face=floor,facing=east，绕过物品放置
//   直接写 state）。
//
// 判定：getState("face")==="floor" 且 getState("facing")==="east"（验证 face/facing 双 state 经
//   setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「砂轮 face（floor/wall/ceiling）/ facing 双 state」可读写：setBlockWithStates 预置
//   face=floor,facing=east（非默认 wall/north 组合）后 getState 双 state 均可读。不测「GUI 修复祛魔」
//   （onBlockActivated openContainer，SimulatedPlayer 无 GUI 放物品 API，跳过）。
function grindstoneStateReadable(test: Test): void {
    // (3,1,1) stone 支撑（砂轮 floor 下方支撑，仅贴近真实放置语义，setBlockWithStates 不经 isValidPosition）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    // setBlockWithStates 预置 face=floor,facing=east（非默认 wall/north 组合，验证双 state 可写）。
    (test as TestWithStates).setBlockWithStates("minecraft:grindstone", { x: 3, y: 2, z: 1 }, "face=floor,facing=east");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:grindstone", `grindstone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言 face===floor 且 facing===east（双 state 可读写）。
    test.assert(getFace(test, 3, 2, 1) === "floor", `face should be floor after setBlockWithStates, got ${getFace(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "east", `facing should be east after setBlockWithStates, got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 5：砂轮破坏不崩溃——放砂轮 → setBlockType air 破坏 → 位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 砂轮（setBlockType 放置，无 BlockEntity）。
// setBlockType("minecraft:air", (3,2,1)) 破坏砂轮 → GrindstoneBlock 无 onBlockRemoved 重写（基类）→
//   位置变 air。砂轮无 BlockEntity，不涉容器遍历，破坏链路安全。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（砂轮已破坏，无 BlockEntity 链路崩溃）。
//
// 此场景验证砂轮破坏链路安全性：放砂轮后破坏，位置正确变 air。砂轮无 BlockEntity（与 barrel 不同），
//   破坏不涉容器遍历。空砂轮无内容物（SimulatedPlayer 无 GUI 放物品 API，无法构造有内容物场景）。
function grindstoneBreaksWhenRemoved(test: Test): void {
    // (3,1,1) stone 支撑 + (3,2,1) 砂轮（setBlockType 放置）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:grindstone", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:grindstone", `grindstone should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏砂轮 → 基类 onBlockRemoved → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言砂轮 (3,2,1) 已破坏变 air。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `grindstone pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerGrindstoneTests(): void {
    GameTest.register("BlockBehaviorTests", "grindstone_floor_facing_equals_player_direction", grindstoneFloorFacingEqualsPlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "grindstone_ceiling_facing_equals_player_direction", grindstoneCeilingFacingEqualsPlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "grindstone_wall_facing_equals_clicked_face", grindstoneWallFacingEqualsClickedFace)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "grindstone_state_readable", grindstoneStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "grindstone_breaks_when_removed", grindstoneBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
