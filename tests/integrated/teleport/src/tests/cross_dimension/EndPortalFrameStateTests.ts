// 末地传送门框架方块状态测试：验证 EYE/FACING 状态属性、放置朝向。
//
// wiki 机制（tech_末地传送门框架.txt）：
//   - 状态属性：EYE（是否有末影之眼，布尔）、FACING（朝向，水平四方向）。
//   - 放置朝向："放置末地传送门框架时，被放置的末地传送门框架会朝向玩家面向方向的相反方向。"
//     即玩家面朝南放置，框架 FACING=北（朝向玩家相反方向）。
//   - 红石元件：红石比较器检测框架顶部是否有末影之眼。有眼→强度15；无眼→强度0。
//   - 光源：末地传送门框架发出亮度等级为1的光。
//   - 默认状态：FACING=North，EYE=false。
//
// Cubium 实现（EndPortalFrameBlock.cpp）：
//   - 状态容器：EYE（BooleanProperty "eye"）+ HORIZONTAL_FACING（DirectionProperty "facing"，createHorizontal）。
//   - setDefaultState：EYE=false, FACING=North。
//   - getStateForPlacement：return defaultState().with(HORIZONTAL_FACING(), Directions::opposite(facing))。
//     对齐 Java 版 EndPortalFrameBlock.java:56 的 p_53052_.getHorizontalDirection().getOpposite()
//     （框架朝向玩家相反方向）。此前 Cubium 漏 opposite（同向），本期修复加 Directions::opposite。
//   - horizontalDirection 由 playerYaw 计算（BlockItemUseContext.cpp:117-124）：
//     yaw∈[315,360)∪[0,45)→South，[45,135)→West，[135,225)→North，[225,315)→East。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 end_portal_frame
//   物品点击 stone 顶面 → onBlockActivated 基类 Pass（stone 非末地传送门框架，targetBlock 走基类 Pass）→
//   fallback Item.useOn → BlockItem::onItemUse → tryPlace → 构造 BlockItemUseContext（playerYaw 来自
//   SimulatedPlayer yaw()，:325）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)) →
//   setBlockState 放末地传送门框架。创造模式不消耗物品。
//
// 朝向控制（复用 GlazedTerracottaTests/FurnaceTests opposite 范式）：
//   末地传送门框架 facing=opposite(horizontalDirection)（仅 yaw），与熔炉/带釉陶瓦同语义。
//   lookAtLocation(blockPos) 瞬时设 yaw=atan2(-dx,dz)（0→South,90→West,180→North,270→East，
//   SimulatedPlayer.cpp:99）。
//   4 朝向映射（facing=opposite(玩家朝向)）：
//     - yaw=0(South)→facing=North；yaw=90(West)→facing=East；
//     - yaw=180(North)→facing=South；yaw=270(East)→facing=West。
//
// 测试覆盖（4 个场景，覆盖 wiki 放置朝向 + state 读写 + EYE 状态核心确定行为）：
//   1. facing=opposite(玩家朝向) 放置（4 朝向）：玩家 lookAtLocation 控制朝向 → useItemOnBlock
//      end_portal_frame → 断言 facing=opposite(朝向)。4 朝向逐一验证（验证修复 getStateForPlacement 加 opposite）。
//   2. EYE 状态可读写：setBlockWithStates 预置 eye=true → getState 可读。
//   3. FACING 四方向可读写：setBlockWithStates 预置 facing=south/east/west/north → getState 可读。
//   4. 默认状态：setBlockWithStates 预置 eye=false,facing=north（默认）→ getState 可读。
//
// 关键约束：
// 1. 场景 1 朝向控制：每朝向独立 spawn 玩家（不与 (3,1,1)/(3,2,1) 重叠），lookAtLocation 传入 [0,6] 内
//    目标坐标产生目标 yaw，再 useItemOnBlock end_portal_frame 点击 (3,1,1) stone Up → 落 (3,2,1)。
// 2. 场景 2/3/4 用 setBlockWithStates 预置 state（绕过物品放置，直接写 state）。getState 读 "eye"/"facing"
//    （C++ 内部属性名）。验证 state 直接写入路径。
// 3. 读 facing/eye 用 getState（DirectionProperty 序列化为方向名字符串 "north"/"south"/"east"/"west"，
//    BooleanProperty 序列化为 true/false）。
// 4. 末地传送门框架有碰撞箱（框架高度 13/16=0.8125，有眼时 1.0），但无 canSurvive（基类 true 无支撑要求）。
//    stone 支撑仅为贴近真实放置。
// 5. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后
//    即可读 state。留 maxTicks 余量防时序。
//
// 不测「红石比较器检测 EYE」：涉比较器模拟信号输出（analogOutputSignal），需比较器方块实体 + 红石信号链路，
//   非本组聚焦，跳过。TODO: 待红石比较器模拟信号测试范式完善后补 end_portal_frame_comparator_signal。
// 不测「亮度等级 1」：末地传送门框架发光亮度为 1（lightEmission=1），但 GameTest 无法直接读亮度等级
//   （需光照系统测试范式），跳过。TODO: 待光照亮度读取测试范式完善后补 end_portal_frame_light_emission。
//
// 跨服务端：end_portal_frame 方块名两端一致。facing/eye state 名两端一致（C++ 内部名 "facing"/"eye"，
//   Java 命名）。朝向放置（facing=opposite(玩家朝向)）+ state 读写行为两端与 vanilla 一致。
//   lookAtLocation 是 Cubium 专有朝向控制（基岩 GameTest 无此 API），但 facing=opposite(朝向) 放置行为
//   本身两端可对比（基岩用真实玩家朝向放置），非 one-sided。setBlockWithStates 预置 state 是 Cubium
//   专有写入，但 state 行为本身两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末地传送门框架.txt#方块状态（EYE/FACING 属性）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末地传送门框架.txt#放置（朝向玩家相反方向）
// Ref: EndPortalFrameBlock.cpp（getStateForPlacement facing=opposite(horizontalDirection)，对齐 vanilla）
// Ref: EndPortalFrameBlock.java:56（getHorizontalDirection().getOpposite()，框架朝向玩家相反方向）
// Ref: BlockItemUseContext.cpp:117-124（horizontalDirection 由 playerYaw 计算：0=South/90=West/180=North/270=East）
// Ref: SimulatedPlayer.cpp:99（lookAtLocation yaw=atan2(-dx,dz)，瞬时 setRotation）
// Ref: GlazedTerracottaTests.ts（facing=opposite(horizontalDirection) 4 朝向坐标配方，本组复用）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 末地传送门框架 facing state（方向名字符串 "north"/"south"/"east"/"west"）。
// 注意：HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（DirectionProperty::createHorizontal），
// getState 按内部名匹配，返回方向名字符串（Java 命名，小写）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 末地传送门框架 eye state（布尔 true/false）。
// 注意：EYE() 的 C++ 属性名为 "eye"（BooleanProperty），getState 返回 boolean。
function getEye(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("eye" as any);
    return typeof value === "boolean" ? value : null;
}

// 朝向放置映射表：玩家朝向 → 末地传送门框架 facing（facing=opposite(玩家朝向)，同 GlazedTerracottaTests 语义）。
// horizontalDirection（BlockItemUseContext.cpp:117-124）：yaw∈[315,360)∪[0,45)→South，
// [45,135)→West，[135,225)→North，[225,315)→East。末地传送门框架 facing=opposite(horizontalDirection)。
// lookAtLocation yaw=atan2(-dx,dz)（SimulatedPlayer.cpp:99）：0→South,90→West,180→North,270→East。
interface FacingCase {
    name: string; // 玩家朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（结构相对，不与 (3,1,1)/(3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（产生目标 yaw，坐标在 [0,6] 内）
    expectedFacing: string; // 末地传送门框架 facing=opposite(玩家朝向)
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
// 坐标配方同 GlazedTerracottaTests FACING_CASES（末地传送门框架与带釉陶瓦 facing 语义一致：
// opposite(horizontalDirection)，仅 yaw，lookAt.y=playerPos.y）。
const FACING_CASES: FacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 2, z: 5 }, expectedFacing: "north" },
    { name: "west", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 0, y: 2, z: 1 }, expectedFacing: "east" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 2, z: 1 }, expectedFacing: "south" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 2, z: 1 }, expectedFacing: "west" },
];

// 放置测试基础结构：(3,1,1) stone 支撑（被点击方块），(3,2,1) 待放末地传送门框架位（air）。
function placeStoneSupport(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });
}

// 场景 1：facing=opposite(玩家朝向) 放置——4 朝向逐一验证（验证修复 getStateForPlacement 加 opposite）。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持
//   end_portal_frame useItemOnBlock 点击 (3,1,1) stone 顶面 Up → placementPos=(3,2,1)（stone 不可
//   替换 → 相邻位置上方 air）→ getStateForPlacement facing=opposite(horizontalDirection(yaw)) →
//   setBlockState 放末地传送门框架 (3,2,1)。断言 facing=expectedFacing。
//
// 判定：4 朝向放置后 facing 分别为 north/east/south/west（facing=opposite(玩家朝向)）。
//
// 此场景验证 wiki「放置时朝向玩家相反方向」+ getStateForPlacement facing=opposite(horizontalDirection)：
//   玩家朝 South→末地传送门框架 facing=North，朝 West→facing=East，朝 North→facing=South，朝 East→facing=West。
//   用 lookAtLocation 控制 yaw（复用 GlazedTerracottaTests opposite 范式坐标）。每朝向用新 player 避免 yaw 残留；
//   每次清理 (3,2,1) 避免末地传送门框架残留阻断放置。本场景是修复 getStateForPlacement 加 opposite 的回归验证
//   （修复前 facing=horizontalDirection 同向，会与 expectedFacing 相反导致断言失败）。
function endPortalFramePlacementFacingOppositePlayer(test: Test): void {
    placeStoneSupport(test);
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FACING_CASES) {
        // 每朝向独立 spawn 玩家（避免 yaw 残留）。
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 末地传送门框架 facing=opposite。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向末地传送门框架残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 end_portal_frame 点击 (3,1,1) stone 顶面 Up → 末地传送门框架落 (3,2,1)。
        // getStateForPlacement facing=opposite(horizontalDirection(yaw))。
        const frameItem = new ItemStack("minecraft:end_portal_frame", 1);
        const used = player.useItemOnBlock(
            frameItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing end_portal_frame facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言末地传送门框架 (3,2,1) 已放置且 facing=expectedFacing（opposite(玩家朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:end_portal_frame", `end_portal_frame should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `end_portal_frame facing should be ${c.expectedFacing} (opposite of player facing ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 2：EYE 状态可读写——setBlockWithStates 预置 eye=true → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 末地传送门框架（setBlockWithStates 预置 eye=true，绕过物品放置直接写 state）。
//
// 判定：getState("eye")===true（验证 eye state 经 setBlockWithStates 写入后可读）。
//
// 此场景验证 wiki「末地传送门框架 EYE（是否有末影之眼）state」可读写：setBlockWithStates 预置
//   eye=true 后 getState 可读。与场景 1（物品放置 facing=opposite(朝向)）互补，验证 state 直接写入路径。
function endPortalFrameEyeStateTrue(test: Test): void {
    placeStoneSupport(test);
    (test as TestWithStates).setBlockWithStates("minecraft:end_portal_frame", { x: 3, y: 2, z: 1 }, "eye=true");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:end_portal_frame", `end_portal_frame should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.assert(getEye(test, 3, 2, 1) === true, `eye should be true, got ${getEye(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：FACING 四方向可读写——setBlockWithStates 预置 facing=south/east/west/north → getState 可读。
//
// 布局：(3,1,1) stone 支撑。依次 setBlockWithStates 预置 facing=south/east/west/north，读回验证方向字符串。
//
// 判定：4 方向后 facing 分别为 south/east/west/north（state 可读写）。
//
// 此场景验证 wiki「末地传送门框架 FACING（水平朝向）state」可读写：setBlockWithStates 预置
//   facing=各方向 后 getState 可读。与场景 1（物品放置 facing=opposite(朝向)）互补，验证 state 直接写入路径。
function endPortalFrameFacingStates(test: Test): void {
    const facings: string[] = ["south", "east", "west", "north"];
    let idx = 0;

    const checkNext = (): void => {
        if (idx >= facings.length) {
            test.succeed();
            return;
        }
        const facing = facings[idx];
        (test as TestWithStates).setBlockWithStates("minecraft:end_portal_frame", { x: 3, y: 2, z: 1 }, `facing=${facing}`);
        test.assert(getFacing(test, 3, 2, 1) === facing, `facing should be ${facing}, got ${getFacing(test, 3, 2, 1)}`);
        idx++;
        checkNext();
    };

    checkNext();
}

// 场景 4：默认状态——setBlockWithStates 预置 eye=false,facing=north（默认）→ getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 末地传送门框架（setBlockWithStates 预置 eye=false,facing=north）。
//
// 判定：getState("eye")===false 且 getState("facing")==="north"（默认状态可读写）。
//
// 此场景验证 wiki「末地传送门框架默认状态：FACING=North，EYE=false」可读写。
function endPortalFrameDefaultState(test: Test): void {
    placeStoneSupport(test);
    (test as TestWithStates).setBlockWithStates("minecraft:end_portal_frame", { x: 3, y: 2, z: 1 }, "eye=false,facing=north");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:end_portal_frame", `end_portal_frame should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.assert(getEye(test, 3, 2, 1) === false, `eye should be false, got ${getEye(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "north", `facing should be north, got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerEndPortalFrameStateTests(): void {
    GameTest.register("TeleportTests", "end_portal_frame_placement_facing_opposite_player", endPortalFramePlacementFacingOppositePlayer)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("TeleportTests", "end_portal_frame_eye_state_true", endPortalFrameEyeStateTrue)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("TeleportTests", "end_portal_frame_facing_states", endPortalFrameFacingStates)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("TeleportTests", "end_portal_frame_default_state", endPortalFrameDefaultState)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
