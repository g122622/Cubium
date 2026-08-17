/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "WallOrFloorItem.hpp"

#include <vector>

#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"

namespace mc {

WallOrFloorItem::WallOrFloorItem(const Block& floorBlock, const Block& wallBlock, ItemProperties properties)
    : BlockItem(floorBlock, properties)
    , m_wallBlock(&wallBlock)
{}

const BlockState* WallOrFloorItem::getStateForPlacement(const BlockItemUseContext& context) const
{
    // 对齐 vanilla StandingAndWallBlockItem.getPlacementState（net.minecraft.world.item.
    // StandingAndWallBlockItem.java:27-45）：
    //   attachmentDirection == DOWN（火把/旗帜/告示牌等"贴墙或落地"类）。遍历
    //   getNearestLookingDirections()，跳过 attachmentDirection.getOpposite()（即 Up）：
    //     direction == DOWN      → 委托 floorBlock.getStateForPlacement（落地形态）
    //     其他（水平方向）        → 委托 wallBlock.getStateForPlacement（贴墙形态）
    //   首个 canPlace（此处用方块侧 isValidPosition 校验）通过的方向即返回其 state。
    //
    // 【关键修复】旧实现直接返回 m_wallBlock->defaultState() / block().defaultState()，
    //   朝向硬编码为默认值（wall 变体 facing 恒为 North、floor 变体 rotation 恒为 0），
    //   从不委托方块侧 getStateForPlacement，导致 banner/sign/hanging_sign/torch/
    //   soul_torch/redstone_torch 等所有 WallOrFloorItem 子类物品放置朝向全部错误。
    //   现改为委托方块侧，由各方块（WallBannerBlock/WallSignBlock/WallHangingSignBlock/
    //   WallTorchBlock/RedstoneWallTorchBlock 等）override 的 getStateForPlacement 依据
    //   点击面/视线方向计算正确朝向。方块侧返回 BlockState 值，此处复用基类
    //   BlockItem::getStateForPlacement（BlockItem.cpp:200-217）的 stateId→canonical
    //   稳定化范式转为注册表 canonical 指针后返回。

    const std::vector<Direction> nearestDirections = context.getNearestLookingDirections();

    const IWorld& world = context.getWorld();
    const BlockPos& pos = context.placementPos();

    if (!world.isWithinWorldBounds(pos)) {
        return nullptr;
    }

    IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));

    const BlockState* resultState = nullptr;

    // 遍历方向，寻找可放置的方向（attachmentDirection.getOpposite() == Up 被跳过）。
    for (Direction direction : nearestDirections) {
        if (direction == Direction::Up) {
            // 跳过 attachmentDirection.getOpposite()：贴墙类物品不放天花板（Up 反向 = Down 的对面）。
            continue;
        }

        // 选择目标方块：Down 走 floor 变体，水平方向走 wall 变体。
        const Block& targetBlock = (direction == Direction::Down) ? block() : *m_wallBlock;

        // 委托方块侧 getStateForPlacement 计算朝向（值）。const_cast 仅为满足非 const 签名。
        BlockState placed =
            const_cast<Block&>(targetBlock).getStateForPlacement(const_cast<BlockItemUseContext&>(context));

        // 规范化为注册表 canonical 指针（对齐基类 BlockItem::getStateForPlacement 范式）。
        BlockState* canonical = Block::getBlockState(placed.stateId());
        if (canonical == nullptr) {
            // 理论上不应发生（方块侧返回的 state 必来自 defaultState().with(...)，stateId 有效）。
            continue;
        }

        // canPlace：校验该 state 在 placementPos 是否可存活。方块侧 getStateForPlacement
        // 内部通常已自带 isValidPosition 校验（WallBannerBlock/WallTorchBlock 等），
        // 此处再校验一次作双保险（floor 变体如 TorchBlock 未 override，依赖此处校验下方支撑）。
        if (targetBlock.isValidPosition(*canonical, blockReader, pos)) {
            resultState = canonical;
            break;
        }
    }

    return resultState;
}

} // namespace mc
