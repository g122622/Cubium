// 火把（torch/wall_torch）放置形态选择、墙火把朝向与下方支撑自毁行为 GameTest。
//
// wiki block_火把（燃尽）.txt：火把可放置在地面上（minecraft:torch，落地火把，无 facing）或贴在墙面上
//   （minecraft:wall_torch，墙火把，HORIZONTAL_FACING 朝向被点击墙的方向）。同一火把物品（WallOrFloorItem）
//   根据玩家点击的面自动选择形态：点击顶面 Up → 落地火把（Down 分支），点击侧面 → 墙火把（水平分支）。
//   墙火把 facing = 点击面（朝向被点击墙）。火把需坚固面支撑：落地火把需下方中心支撑，墙火把需贴墙侧
//   sturdy 面；支撑失效自毁掉落。
//   （注：1.21.6+ 火把放置后随机刻变「燃尽」为实验特性且概率性，Cubium 未实现，本文件不涉及。）
//
// C++ 链路：TorchBlock（decorative/TorchBlock.cpp）单态无 state（落地火把）；WallTorchBlock（继承
//   TorchBlock）有 HORIZONTAL_FACING（C++ 属性名 "facing"，默认 North）。
//   - TorchBlock 不 override getStateForPlacement（基类返 defaultState，落地火把无 facing 正确）。
//   - WallTorchBlock::getStateForPlacement（WallTorchBlock.cpp:150-176）：遍历 getNearestLookingDirections，
//     水平方向 facing=opposite(direction)，attachPos=pos.offset(direction)（被点击方块），校验
//     isFaceSturdy(attachPos, facing, Full) 通过即返回。getNearestLookingDirections 首位 =
//     opposite(clickedFace)，故 facing=opposite(opposite(clickedFace))=clickedFace（朝向被点击墙）。
//   - TorchBlock::isValidPosition（:52-58）：canSupportCenter(pos.down(), Up)（下方中心支撑）。
//   - WallTorchBlock::isValidPosition（:100-112）：attachPos=pos.offset(opposite(facing))，校验
//     isFaceSturdy(attachPos, facing, Full)（贴墙侧 sturdy 面）。
//   - TorchBlock::updatePostPlacement（:60-79）：facing==Down 且 !_canSurvive → 返 air 自毁。
//   - WallTorchBlock::updatePostPlacement（:125-148）：facing==opposite(torchFacing) 且贴墙侧非 sturdy →
//     返 air 自毁。
//
// 物品链路：torch 物品是 WallOrFloorItem（Items.cpp:3542，floorBlock=torch, wallBlock=wall_torch）。
//   WallOrFloorItem::getStateForPlacement（WallOrFloorItem.cpp，已修复对齐 vanilla
//   StandingAndWallBlockItem.getPlacementState）：遍历 getNearestLookingDirections，跳过 Up，
//   Down→委托 TorchBlock.getStateForPlacement（落地形态），水平→委托 WallTorchBlock.getStateForPlacement
//   （墙形态），isValidPosition 通过即返回。修复前该函数硬编码 defaultState()（wall facing 恒 North），
//   从不委托方块侧，导致墙火把物品放置朝向错误。修复后委托方块侧，facing=clickedFace 正确。
//   BlockItem::canPlace（已修复）用 state.getBlock()（state owner）校验，非 m_block——对 wall_torch state
//   用 WallTorchBlock.isValidPosition 正确校验贴墙侧（修复前用 TorchBlock.isValidPosition 查下方误判）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock(stack, blockLocation, face, faceLocation)（SimulatedPlayer.cpp
//   :265-346）。手持 torch 物品点击 stone 侧面/顶面 → onBlockActivated(stone) 基类 Pass → fallback
//   Item.useOn → BlockItem::tryPlace → WallOrFloorItem::getStateForPlacement（修复后委托对应方块）→
//   setBlockState(placementPos)。placementPos = 被点击方块.relative(face)。SimulatedPlayer 默认创造模式。
//
// 测试覆盖（3 个场景，覆盖 wiki 形态选择 + 墙朝向 + 支撑自毁核心确定行为）：
//   1. 墙火把放置朝向：点击 stone 南面 → wall_torch 落 (3,2,2)，facing=south（修复后委托方块侧正确算朝向）。
//   2. 落地火把形态选择：点击 stone 顶面 Up → torch 落 (3,2,1)（Down 分支选 floor 变体，非 wall_torch）。
//   3. 落地火把下方支撑自毁：放 torch 在 stone 上 → 移除下方 stone → 自毁为 air。
//
// 关键约束：
// 1. 场景 1 被点击方块用 stone（stone onBlockActivated 基类 Pass，不短路放置）。点击 (3,2,1) stone
//    南面 South，新墙火把落 (3,2,2)（placementPos=(3,2,1).relative(South)）。WallOrFloorItem 委托
//    WallTorchBlock.getStateForPlacement：首位 direction=opposite(South)=North（水平），facing=opposite(North)
//    =South。attachPos=(3,2,2).offset(North)=(3,2,1)=stone，isFaceSturdy(stone, (3,2,1), South, Full) ✓
//    → 返 facing=south。isValidPosition 双保险通过。
// 2. 场景 2 点击 (3,1,1) stone 顶面 Up，新落地火把落 (3,2,1)（placementPos=(3,1,1).relative(Up)）。
//    WallOrFloorItem：首位 direction=opposite(Up)=Down（非 Up 不跳过），targetBlock=block()=TorchBlock
//    （floor 变体）。TorchBlock.getStateForPlacement 基类返 defaultState（单态无 facing）。isValidPosition
//    （TorchBlock）：canSupportCenter((3,2,1).down()=(3,1,1), Up) → stone 顶面 sturdy ✓ → 落地火把。
//    断言 typeId=minecraft:torch（非 wall_torch），验证 Down 分支选 floor 变体。
// 3. 场景 3 用 setBlockType 强放 torch（绕过 isValidPosition），先放 stone 支撑再放 torch。移除 (3,1,1)
//    stone → Down 方向 updatePostPlacement → _canSurvive 失败 → 返 air 自毁。
// 4. 读 wall torch facing 用 getState("facing" as any)（C++ 属性名 "facing"，返小写方向字符串）。
//    落地火把无 facing state，不读 facing（仅判 typeId）。
// 5. stone 任意面 isFaceSturdy(Full/Center) 均 sturdy（完整方块），测试不受限。
// 6. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后即可
//    读 state。留 maxTicks 余量防时序。
//
// 不测「墙火把侧面支撑自毁」：与落地火把支撑自毁范式同构（updatePostPlacement opposite(facing) 方向），
//   本文件场景 3 已覆盖落地火把支撑自毁核心行为。TODO: 可补 wall_torch_breaks_when_support_wall_removed。
// 不测「火把发光等级 14」：光照测试属 lighting 包范畴，跳过。
// 不测「火把燃尽」：1.21.6+ 实验特性，概率性随机刻，Cubium 未实现，跳过。
// 不测「火把含水」：waterlogged 涉流体，本文件聚焦形态选择/朝向/自毁，跳过。
//
// 跨服务端：torch/wall_torch 方块名两端一致。墙火把 facing state 名两端一致（C++ 内部名 "facing"）。
//   形态选择（Down→floor/水平→wall）+ 墙朝向（facing=clickedFace）+ 支撑自毁行为两端与 vanilla 一致。
//   非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_火把（燃尽）.txt（地面/墙面放置，坚固面支撑+失效掉落）
// Ref: WallTorchBlock.cpp（getStateForPlacement：facing=opposite(direction)=clickedFace，isFaceSturdy 校验）
// Ref: TorchBlock.cpp（updatePostPlacement Down _canSurvive canSupportCenter 支撑自毁）
// Ref: WallOrFloorItem.cpp（getStateForPlacement：修复后委托方块侧，Down→floor/水平→wall 形态选择）
// Ref: BlockItemUseContext.cpp（getNearestLookingDirections：opposite(clickedFace) 提首位）
// Ref: BannerTests.ts（useItemOnBlock 放置范式 + getState 读 facing，wall 变体 facing=clickedFace 同构）
// Ref: CarpetTests.ts（支撑自毁范式：setBlockType 支撑+方块 → 移除支撑 → succeedWhenBlockPresent 断言消失）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1：墙火把 (3,2,2)，被点击 stone (3,2,1)（火把 North 侧）。
// 场景 2：落地火把 (3,2,1)，被点击 stone (3,1,1)（火把下方）。
// 场景 3：落地火把 (3,2,1)，下方支撑 (3,1,1) stone。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 墙火把 facing state（小写方向字符串：north/south/east/west）。返回 null 表示失败或非墙火把。
// 注意：WallTorchBlock 用 HORIZONTAL_FACING()，其 C++ 属性名为 "facing"（非 "horizontal_facing"），
// getState 按 entry.property->name() 匹配 C++ 内部名，故读 "facing"。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 场景 1：墙火把放置朝向——点击 stone 南面 → wall_torch 落 (3,2,2)，facing=south。
//
// 布局：(3,2,1) 放 stone（被点击方块，火把贴墙参照）。手持 torch useItemOnBlock 点击 (3,2,1)
//   南面（face=South），新墙火把落 (3,2,2)（placementPos=(3,2,1).relative(South)）。
// WallOrFloorItem::getStateForPlacement（修复后委托 WallTorchBlock）：getNearestLookingDirections 首位
//   direction=opposite(South)=North（水平），facing=opposite(North)=South。attachPos=(3,2,2).offset(North)
//   =(3,2,1)=stone，isFaceSturdy(stone, (3,2,1), South, Full) ✓ → 返 facing=south。
//
// 判定：(3,2,2) typeId === "minecraft:wall_torch" 且 facing==="south"。
//
// 此场景验证 WallOrFloorItem 修复对 torch 生效：修复前该路径硬编码 facing=north（恒默认），修复后委托
//   方块侧正确算出 facing=south。若修复回退，本场景 facing 断言会失败（实际 north）。
function torchWallFacingWhenPlacedOnSouthFace(test: Test): void {
    // (3,2,1) 放 stone（被点击方块，火把贴墙参照）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone", `stone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 2 }, "farmer");
    const torch = new ItemStack("minecraft:torch", 1);

    // 手持 torch 点击 (3,2,1) 南面 South → 新墙火把落 (3,2,2)。stone onBlockActivated Pass → fallback 放置。
    // WallOrFloorItem 委托 WallTorchBlock.getStateForPlacement → facing=south。
    const used = farmer.useItemOnBlock(
        torch as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.South,
    );
    test.assert(used, "useItemOnBlock should return true when placing wall torch on south face");

    // 判定：新墙火把 (3,2,2) 是 wall_torch，facing=south（朝向被点击墙 South 面）。
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:wall_torch", `new wall torch should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getFacing(test, 3, 2, 2) === "south", `wall torch facing should be south (clicked face), got ${getFacing(test, 3, 2, 2)}`);

    test.succeed();
}

// 场景 2：落地火把形态选择——点击 stone 顶面 Up → torch 落 (3,2,1)（Down 分支选 floor 变体）。
//
// 布局：(3,1,1) 放 stone（被点击方块，火把下方支撑）。手持 torch useItemOnBlock 点击 (3,1,1)
//   顶面（face=Up），新落地火把落 (3,2,1)（placementPos=(3,1,1).relative(Up)）。
// WallOrFloorItem::getStateForPlacement：getNearestLookingDirections 首位 direction=opposite(Up)=Down
//   （非 Up 不跳过），targetBlock=block()=TorchBlock（floor 变体）。TorchBlock.getStateForPlacement 基类返
//   defaultState（单态无 facing）。isValidPosition（TorchBlock）：canSupportCenter((3,2,1).down()=(3,1,1), Up)
//   → stone 顶面 sturdy ✓ → 落地火把。
//
// 判定：(3,2,1) typeId === "minecraft:torch"（非 wall_torch，验证 Down 分支选 floor 变体）。
//
// 此场景验证 WallOrFloorItem 的 Down 分支正确选 floor 变体（落地火把）：修复前 Down 分支返
//   block().defaultState()（TorchBlock 单态，typeId 恰为 torch），形态选择"碰巧"正确但朝向委托缺失。
//   修复后 Down 分支委托 TorchBlock.getStateForPlacement（基类 defaultState），结果一致但走正确委托链。
//   本场景核心验证形态选择（Down→floor 非 wall），与场景 1（水平→wall）对照。
function torchFloorWhenPlacedOnTop(test: Test): void {
    // (3,1,1) 放 stone（被点击方块，火把下方支撑，顶面 Up sturdy）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const torch = new ItemStack("minecraft:torch", 1);

    // 手持 torch 点击 (3,1,1) 顶面 Up → 新落地火把落 (3,2,1)。WallOrFloorItem Down 分支选 TorchBlock
    // （floor 变体），canSupportCenter(下方 stone, Up) ✓ → 落地火把（typeId=torch 非 wall_torch）。
    const used = farmer.useItemOnBlock(
        torch as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing floor torch on top face");

    // 判定：新落地火把 (3,2,1) 是 torch（非 wall_torch，Down 分支选 floor 变体）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:torch", `new floor torch should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：落地火把下方支撑自毁——放 torch 在 stone 上 → 移除下方 stone → 自毁为 air。
//
// 布局：(3,1,1) 铺 stone 作火把下方支撑（canSupportCenter(Up) true），(3,2,1) 放火把（在 stone 上，
//   强放绕过 isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居火把派发 updatePostPlacement(Down) → 下方 air canSupportCenter(Up) false →
// _canSurvive 失败 → 返回 air，火把自毁。同 tick 同步。
//
// 判定：succeedWhenBlockPresent 断言火把格 (3,2,1) 火把消失（同 tick 同步）。
function torchBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 stone 作火把下方支撑（isFaceSturdy(Up, Center) true，canSupportCenter 满足）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放火把（在 stone 上，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:torch", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 火把派发 updatePostPlacement(Down) → 下方 air canSupportCenter(Up) false → _canSurvive 失败 →
    // 返回 air，火把自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言火把格 (3,2,1) 火把已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:torch", { x: 3, y: 2, z: 1 }, false);
}

export function registerTorchTests(): void {
    GameTest.register("BlockBehaviorTests", "torch_wall_facing_when_placed_on_south_face", torchWallFacingWhenPlacedOnSouthFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "torch_floor_when_placed_on_top", torchFloorWhenPlacedOnTop)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "torch_breaks_when_support_below_removed", torchBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
