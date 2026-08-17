// 旗帜（banner）墙挂朝向与站立/墙挂支撑自毁行为 GameTest。
//
// wiki tech_旗帜.txt：旗帜可面对不同方向放置在方块顶面（站立 white_banner，rotation 0-15）和侧面
//   （墙挂 white_wall_banner，facing 水平 4 向）。旗帜会在附着方块移动、移除或被破坏时被破坏并掉落自身
//   ——站立旗帜下方支撑移除自毁，墙挂旗帜所贴墙移除自毁。旗帜于 1.14 重做（统一为 wall/floor 双形态），
//   1.21.11 已包含，属 vanilla 正式特性。图案/NBT 是方块实体层面，本测试不测。
//
// C++ 链路：BannerBlock（decorative/BannerBlock.cpp）两个类：
//   - StandingBannerBlock（white_banner）：仅 ROTATION state（IntegerProperty 0-15，C++ 属性名 "rotation"，
//     默认 0）。
//   - WallBannerBlock（white_wall_banner）：仅 HORIZONTAL_FACING state（C++ 属性名 "facing"，4 水平方向，
//     默认 North）。
//   - getStateForPlacement（StandingBannerBlock:95-101）：rotation = floor((180+yaw)*16/360+0.5) & 15
//     （由玩家 yaw，SimulatedPlayer 默认朝向不可控，故站立放置朝向跳过）。
//   - getStateForPlacement（WallBannerBlock:189-211）：遍历 getNearestLookingDirections()，水平方向
//     facing = opposite(direction)，用 isValidPosition 校验。getNearestLookingDirections 把
//     opposite(clickedFace) 提到首位（BlockItemUseContext.cpp:217-237），故首位 direction =
//     opposite(clickedFace)，facing = opposite(direction) = clickedFace。即墙挂旗帜 facing = 点击面
//     （朝向被点击墙的方向）。isValidPosition 查 opposite(facing) 方向（被点击方块）是否 solid。
//   - isValidPosition（StandingBannerBlock:103-109）：下方 belowState.isSolid()。
//   - isValidPosition（WallBannerBlock:213-219）：opposite(facing) 方向 supportState.isSolid()。
//   - updatePostPlacement（StandingBannerBlock:111-131）：facing==Down 且 below 非 solid → 返 air 自毁。
//   - updatePostPlacement（WallBannerBlock:221-242）：facing==opposite(bannerFacing) 且 supportState
//     非 solid → 返 air 自毁（仅支撑侧方向触发，非支撑侧不触发）。
//
// 物品链路：BannerItem（继承 WallOrFloorItem）物品放置走 WallOrFloorItem::getStateForPlacement
//   （WallOrFloorItem.cpp:41-105，已修复对齐 vanilla StandingAndWallBlockItem.getPlacementState）：
//   遍历 getNearestLookingDirections，跳过 Up，Down→委托 StandingBannerBlock.getStateForPlacement，
//   水平→委托 WallBannerBlock.getStateForPlacement，isValidPosition 通过即返回。修复前该函数硬编码
//   defaultState()（wall facing 恒 North、standing rotation 恒 0），从不委托方块侧，导致墙挂旗帜
//   物品放置朝向错误。修复后委托方块侧，facing=clickedFace 正确。本测试场景 2 验证修复正确性。
//
// 派发链路：SimulatedPlayer::useItemOnBlock(stack, blockLocation, face, faceLocation)（SimulatedPlayer.cpp
//   :265-346）。手持 white_banner 物品（BannerItem，Items.cpp:3350）点击 stone 侧面 →
//   onBlockActivated(stone) 基类 Pass → fallback Item.useOn → BlockItem::tryPlace →
//   WallOrFloorItem::getStateForPlacement（修复后委托 WallBannerBlock）→ setBlockState(placementPos)。
//   placementPos = 被点击方块.relative(face)。SimulatedPlayer 默认创造模式（物品不消耗）。
//
// 测试覆盖（4 个场景，覆盖 wiki 墙挂朝向 + 站立/墙挂支撑自毁核心确定行为）：
//   1. 墙挂旗帜放置朝向：点击 stone 南面 → 墙挂旗帜 facing=south（修复后委托方块侧正确算朝向）。
//   2. 墙挂旗帜支撑自毁：预置墙挂旗帜贴 South 侧 stone → 移除 South 侧 stone → 自毁为 air。
//   3. 站立旗帜支撑自毁：预置站立旗帜下方 stone → 移除下方 stone → 自毁为 air。
//   4. 墙挂旗帜移除非支撑侧不自毁：预置墙挂旗帜贴 South 侧 stone → 移除 North 侧（非支撑侧）→ 不自毁。
//
// 关键约束：
// 1. 场景 1 被点击方块用 stone（stone onBlockActivated 基类 Pass，不短路放置）。点击 (3,2,1) stone
//    南面 South，新墙挂旗帜落 (3,2,2)（placementPos=(3,2,1).relative(South)）。WallOrFloorItem 委托
//    WallBannerBlock.getStateForPlacement：首位 direction=opposite(South)=North，facing=opposite(North)
//    =South，isValidPosition 查 opposite(South)=North 方向 (3,2,1) stone solid ✓ → 返 facing=south。
// 2. 场景 2/4 用 setBlockWithStates 预置墙挂旗帜（不走物品放置，直接写 state，绕过 WallOrFloorItem）。
//    facing 的 C++ 属性名是 "facing"（HORIZONTAL_FACING 创建名为 "facing"），setBlockWithStates 按 C++
//    内部名查属性。墙挂旗帜 (3,2,2) facing=south，支撑在 South 侧 (3,2,3)（opposite(south)=north，
//    故 facing=south 的支撑在 north 侧……需仔细：isValidPosition 查 opposite(facing) 方向。facing=south
//    → opposite(south)=north → 支撑在 north 侧 (3,2,1)）。场景 2 移除 north 侧 (3,2,1) 触发自毁；
//    场景 4 移除 south 侧 (3,2,3) 不触发（非支撑侧）。
//    ——注意：facing=south 时旗帜"朝南挂"，其贴墙侧是 north（opposite(facing)=north），故支撑墙在 north
//    侧 (3,2,1)。这与直觉相反（朝南挂的旗帜贴北墙），需在布局中正确放置支撑墙。
// 3. 场景 3 用 setBlockWithStates 预置站立旗帜 rotation=0（绕过物品放置，避免 yaw 不可控）。下方支撑
//    (3,1,1) stone。移除 (3,1,1) → Down 方向 updatePostPlacement → below 非 solid → 自毁。
// 4. 支撑自毁是 updatePostPlacement 同步触发（移除支撑方块 setBlockState air 派发邻居更新 → 旗帜
//    updatePostPlacement 返 air → ServerWorld 立即 setBlockState）。用 succeedWhenBlockPresent 直接断言，
//    或同 tick 同步读 typeId。留 maxTicks 余量防时序。
// 5. 读 wall banner facing 用 getState("facing" as any)（C++ 属性名 "facing"，返小写方向字符串）。
//    读 standing banner rotation 用 getState("rotation" as any)（返 number）。
// 6. 支撑方块用 stone（isSolid=true）。旗帜自身 notSolid（notSolid()=true），不影响支撑判定。
//
// 不测「站立旗帜放置朝向」：rotation 由玩家 yaw 映射 0-15（StandingBannerBlock:98），SimulatedPlayer
//   默认朝向不可控，且物品放置走 WallOrFloorItem floor 分支委托 StandingBannerBlock.getStateForPlacement
//   （修复后），rotation 取决于 yaw。跳过。TODO: 待 SimulatedPlayer yaw 可控后补 banner_standing_rotation。
// 不测「旗帜图案/NBT」：涉方块实体 Patterns NBT，脚本侧不可断言图案，跳过。
// 不测「炼药锅洗旗帜图案」：涉容器交互 + 图案 NBT，跳过。
// 不测「盾牌/地图印图」：非方块放置行为，跳过。
//
// 跨服务端：white_banner/white_wall_banner 方块名两端一致。墙挂 facing/站立 rotation state 名两端一致
//   （C++ 内部名 "facing"/"rotation"，基岩对外经 TemplateLoader 映射，脚本侧 getState 用 C++ 名）。
//   墙挂旗帜放置朝向（facing=clickedFace）+ 支撑自毁行为两端与 vanilla 一致。setBlockWithStates 预置
//   旗帜是 Cubium 专有写入（基岩侧用物品放置），但放置朝向 + 自毁行为本身两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_旗帜.txt（顶面/侧面放置，支撑失效自毁掉落）
// Ref: BannerBlock.cpp（WallBannerBlock.getStateForPlacement：facing=opposite(direction)=clickedFace，isValidPosition 校验）
// Ref: BannerBlock.cpp（updatePostPlacement：站立 Down/墙挂 opposite(facing) 支撑失效返 air 自毁）
// Ref: WallOrFloorItem.cpp（getStateForPlacement：修复后委托方块侧，对齐 vanilla StandingAndWallBlockItem）
// Ref: BlockItemUseContext.cpp（getNearestLookingDirections：opposite(clickedFace) 提首位）
// Ref: BellTests.ts（useItemOnBlock 放置范式 + setBlockWithStates 预置 + getState 读 facing）
// Ref: CarpetTests.ts（支撑自毁范式：setBlockType 支撑+方块 → 移除支撑 → succeedWhenBlockPresent 断言消失）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1：墙挂旗帜 (3,2,2)，被点击 stone (3,2,1)（旗帜 North 侧）。
// 场景 2/4：墙挂旗帜 (3,2,2) facing=south（贴 North 侧 (3,2,1) 墙）；场景 2 移除 (3,2,1)，场景 4 移除 (3,2,3)。
// 场景 3：站立旗帜 (3,2,1)，下方支撑 (3,1,1) stone。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BellTests/EndRodTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 墙挂旗帜 facing state（小写方向字符串：north/south/east/west）。返回 null 表示失败或非墙挂旗帜。
// 注意：WallBannerBlock 用 HORIZONTAL_FACING()，其 C++ 属性名为 "facing"（非 "horizontal_facing"），
// getState 按 entry.property->name() 匹配 C++ 内部名，故读 "facing"。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 场景 1：墙挂旗帜放置朝向——点击 stone 南面 → 墙挂旗帜 facing=south。
//
// 布局：(3,2,1) 放 stone（被点击方块，旗帜贴墙参照）。手持 white_banner useItemOnBlock 点击 (3,2,1)
//   南面（face=South），新墙挂旗帜落 (3,2,2)（placementPos=(3,2,1).relative(South)）。
// WallOrFloorItem::getStateForPlacement（修复后委托 WallBannerBlock）：getNearestLookingDirections 首位
//   direction=opposite(South)=North（水平），facing=opposite(North)=South。isValidPosition 查
//   opposite(facing)=opposite(South)=North 方向 (3,2,1) stone solid ✓ → 返 facing=south。
//
// 判定：(3,2,2) typeId === "minecraft:white_wall_banner" 且 facing==="south"。
//
// 此场景验证 WallOrFloorItem 修复正确性：修复前该路径硬编码 facing=north（恒默认），修复后委托方块侧
//   正确算出 facing=south。若修复回退，本场景 facing 断言会失败（实际 north）。
function bannerWallFacingWhenPlacedOnSouthFace(test: Test): void {
    // (3,2,1) 放 stone（被点击方块，旗帜贴墙参照）。对侧 (3,2,3) 保持 air（单面墙，不影响墙挂放置）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone", `stone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 2 }, "farmer");
    const banner = new ItemStack("minecraft:white_banner", 1);

    // 手持 white_banner 点击 (3,2,1) 南面 South → 新墙挂旗帜落 (3,2,2)。stone onBlockActivated Pass →
    // fallback 放置。WallOrFloorItem 委托 WallBannerBlock.getStateForPlacement → facing=south。
    const used = farmer.useItemOnBlock(
        banner as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.South,
    );
    test.assert(used, "useItemOnBlock should return true when placing wall banner on south face");

    // 判定：新墙挂旗帜 (3,2,2) 是 white_wall_banner，facing=south（朝向被点击墙 South 面）。
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:white_wall_banner", `new wall banner should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getFacing(test, 3, 2, 2) === "south", `wall banner facing should be south (clicked face), got ${getFacing(test, 3, 2, 2)}`);

    test.succeed();
}

// 场景 2：墙挂旗帜支撑自毁——预置墙挂旗帜贴 North 侧 stone → 移除 North 侧 stone → 自毁为 air。
//
// 布局：墙挂旗帜 (3,2,2) 预置 facing=south（朝南挂，贴 North 侧 (3,2,1) 墙——isValidPosition 查
//   opposite(facing)=opposite(south)=north 方向 (3,2,1) 是否 solid）。North 侧 (3,2,1) 放 stone 支撑。
//   移除 (3,2,1) stone（设 air）→ 触发 (3,2,2) 旗帜 North 方向 updatePostPlacement（facing=North=
//   opposite(bannerFacing=south)）→ 查 North 侧 air 非 solid → 返 air 自毁。
//
// 判定：succeedWhenBlockPresent 断言墙挂旗帜 (3,2,2) 已消失（自毁为 air）。
function bannerWallBreaksWhenSupportWallRemoved(test: Test): void {
    // North 侧 (3,2,1) 放 stone 支撑（facing=south 的墙挂旗帜贴 North 侧墙）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone", `stone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 预置墙挂旗帜 (3,2,2) facing=south（朝南挂，贴 North 侧 (3,2,1) stone 墙）。
    // 注意：facing 的 C++ 属性名是 "facing"（HORIZONTAL_FACING 创建名为 "facing"）。
    (test as TestWithStates).setBlockWithStates("minecraft:white_wall_banner", { x: 3, y: 2, z: 2 }, "facing=south");
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:white_wall_banner", `wall banner should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getFacing(test, 3, 2, 2) === "south", `wall banner facing should be south before support removal, got ${getFacing(test, 3, 2, 2)}`);

    // (3,2,1) 设 air 移除支撑墙（stone→air 真实变化，派发邻居更新）。air 放置向 South 邻居 (3,2,2) 旗帜
    // 派发 updatePostPlacement(South 方向？)——需仔细：移除 (3,2,1) 时，其 South 邻居 (3,2,2) 收到
    // updatePostPlacement(facing=South？)。实际：被改方块 (3,2,1) 通知其邻居，邻居 (3,2,2) 收到的
    // facing 是「从 (3,2,2) 指向 (3,2,1)」的方向 = North。故旗帜 updatePostPlacement(facing=North)，
    // North==opposite(bannerFacing=south) ✓ → 查 North 侧 (3,2,1) air 非 solid → 返 air 自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言墙挂旗帜 (3,2,2) 已自毁消失（支撑墙移除，同 tick 同步自毁）。
    test.succeedWhenBlockPresent("minecraft:white_wall_banner", { x: 3, y: 2, z: 2 }, false);
}

// 场景 3：站立旗帜支撑自毁——预置站立旗帜下方 stone → 移除下方 stone → 自毁为 air。
//
// 布局：站立旗帜 (3,2,1) 预置 rotation=0（绕过物品放置，避免 yaw 不可控）。下方支撑 (3,1,1) stone。
//   移除 (3,1,1) stone（设 air）→ 触发 (3,2,1) 旗帜 Down 方向 updatePostPlacement（被改方块 (3,1,1)
//   通知其 Up 邻居 (3,2,1)，邻居收到 facing=Down）→ 查 below (3,1,1) air 非 solid → 返 air 自毁。
//
// 判定：succeedWhenBlockPresent 断言站立旗帜 (3,2,1) 已消失（自毁为 air）。
function bannerStandingBreaksWhenSupportBelowRemoved(test: Test): void {
    // 下方 (3,1,1) 放 stone 支撑（站立旗帜 isValidPosition 要求 belowState.isSolid）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    // 预置站立旗帜 (3,2,1) rotation=0（绕过物品放置，直接写 state，避免 yaw 不可控）。
    // rotation 的 C++ 属性名是 "rotation"（ROTATION_0_15 创建名为 "rotation"）。
    (test as TestWithStates).setBlockWithStates("minecraft:white_banner", { x: 3, y: 2, z: 1 }, "rotation=0");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:white_banner", `standing banner should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // (3,1,1) 设 air 移除下方支撑（stone→air 真实变化，派发邻居更新）。air 放置向 Up 邻居 (3,2,1) 旗帜
    // 派发 updatePostPlacement(facing=Down) → 查 below (3,1,1) air 非 solid → 返 air 自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言站立旗帜 (3,2,1) 已自毁消失（下方支撑失效，同 tick 同步自毁）。
    test.succeedWhenBlockPresent("minecraft:white_banner", { x: 3, y: 2, z: 1 }, false);
}

// 场景 4：墙挂旗帜移除非支撑侧不自毁——移除 South 侧（非支撑侧）→ 旗帜不自毁。
//
// 布局：墙挂旗帜 (3,2,2) facing=south（贴 North 侧 (3,2,1) stone 墙支撑）。South 侧 (3,2,3) 保持 air
//   （非支撑侧）。移除 South 侧 (3,2,3)——但 (3,2,3) 本就是 air，设 air 是 no-op 不派发更新。故本场景
//   改为：South 侧 (3,2,3) 先放 stone 再移除（制造真实变化），验证移除非支撑侧不触发自毁。
//   移除 (3,2,3) stone → 触发 (3,2,2) 旗帜 North 方向 updatePostPlacement（被改方块 (3,2,3) 通知其
//   North 邻居 (3,2,2)，邻居收到 facing=South）。South != opposite(bannerFacing=south)=North →
//   updatePostPlacement 直接返回原 state，不自毁。
//
// 判定：succeedWhenBlockPresent 断言墙挂旗帜 (3,2,2) 仍在（移除非支撑侧不自毁）。
//
// 对照场景 2：同 facing=south 墙挂旗帜，场景 2 移除支撑侧（North (3,2,1)）→ 自毁；场景 4 移除非支撑侧
//   （South (3,2,3)）→ 不自毁。验证 updatePostPlacement 仅对 opposite(bannerFacing) 方向响应。
function bannerWallSurvivesWhenNonSupportSideRemoved(test: Test): void {
    // North 侧 (3,2,1) 放 stone 支撑（facing=south 的墙挂旗帜贴 North 侧墙）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    // South 侧 (3,2,3) 放 stone（非支撑侧，先放以便后续移除制造真实状态变化派发更新）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone", `stone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getBlockTypeId(test, 3, 2, 3) === "minecraft:stone", `stone should be at (3,2,3), got ${getBlockTypeId(test, 3, 2, 3)}`);

    // 预置墙挂旗帜 (3,2,2) facing=south（贴 North 侧 (3,2,1) stone 墙支撑，South 侧 (3,2,3) 非支撑）。
    (test as TestWithStates).setBlockWithStates("minecraft:white_wall_banner", { x: 3, y: 2, z: 2 }, "facing=south");
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:white_wall_banner", `wall banner should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getFacing(test, 3, 2, 2) === "south", `wall banner facing should be south, got ${getFacing(test, 3, 2, 2)}`);

    // 移除非支撑侧 South (3,2,3) stone→air（真实变化，派发邻居更新）。被改方块 (3,2,3) 通知其 North
    // 邻居 (3,2,2) 旗帜，邻居收到 facing=South（从 (3,2,2) 指向 (3,2,3)）。South != opposite(south)=North
    // → updatePostPlacement 不响应，旗帜保持原 state 不自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 3 });

    // 断言墙挂旗帜 (3,2,2) 仍在（移除非支撑侧 South 不自毁，支撑侧 North (3,2,1) 仍 stone）。
    test.succeedWhenBlockPresent("minecraft:white_wall_banner", { x: 3, y: 2, z: 2 }, true);
}

export function registerBannerTests(): void {
    GameTest.register("BlockBehaviorTests", "banner_wall_facing_when_placed_on_south_face", bannerWallFacingWhenPlacedOnSouthFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "banner_wall_breaks_when_support_wall_removed", bannerWallBreaksWhenSupportWallRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "banner_standing_breaks_when_support_below_removed", bannerStandingBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "banner_wall_survives_when_non_support_side_removed", bannerWallSurvivesWhenNonSupportSideRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
