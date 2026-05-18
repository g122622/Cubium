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

#include "BellBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ========== BellBlock 实现 ==========

BellBlock::BellBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::HORIZONTAL_FACING())
                         .add(BlockStateProperties::BELL_ATTACHMENT())
                         .add(BlockStateProperties::POWERED())
                         .create([](const Block& block, std::vector<size_t> values, const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
                         });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor)
            .with(BlockStateProperties::POWERED(), false));

    // 创建钟的形状
    constexpr f32 P = 1.0f / 16.0f;

    // 地面附着：钟身 + 支架
    m_floorShape = CollisionShape::box(4.0f * P, 0.0f, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P);

    // 天花板附着
    m_ceilingShape = CollisionShape::box(4.0f * P, 0.0f, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P);

    // 墙面附着
    m_wallShape = CollisionShape::box(4.0f * P, 4.0f * P, 0.0f, 12.0f * P, 12.0f * P, 16.0f * P);

    // 初始化形状缓存
    for (int i = 0; i < 16; ++i) {
        m_shapesByState[i] = m_floorShape;
    }
}

BlockState BellBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    Direction clickedFace = context.getClickedFace();

    // 根据点击的面确定附着类型
    BlockStateProperties::BellAttachment attachment;
    if (clickedFace == Direction::Up) {
        attachment = BlockStateProperties::BellAttachment::Floor;
    } else if (clickedFace == Direction::Down) {
        attachment = BlockStateProperties::BellAttachment::Ceiling;
    } else {
        attachment = BlockStateProperties::BellAttachment::SingleWall;
        facing = clickedFace;
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::BELL_ATTACHMENT(), attachment);
}

BlockState BellBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());
    Direction bellFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 检查支撑是否仍然有效
    bool valid = true;
    switch (attachment) {
        case BlockStateProperties::BellAttachment::Floor:
            if (facing == Direction::Down) {
                valid = facingState.isSolid();
            }
            break;
        case BlockStateProperties::BellAttachment::Ceiling:
            if (facing == Direction::Up) {
                valid = facingState.isSolid();
            }
            break;
        case BlockStateProperties::BellAttachment::SingleWall:
            if (facing == bellFacing) {
                valid = facingState.isSolid();
            }
            break;
        case BlockStateProperties::BellAttachment::DoubleWall:
            // 双面墙需要两侧都有支撑
            if (facing == bellFacing || facing == Directions::opposite(bellFacing)) {
                valid = facingState.isSolid();
            }
            break;
    }

    if (!valid) {
        // 参考 MC 1.16.5: BellBlock.updatePostPlacement
        // 支撑失效时，掉落钟物品并移除方块
        // 生成物品掉落
        const Block* block = &state.getBlock();
        if (block != nullptr) {
            // 获取方块对应的物品
            const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
            if (blockItem != nullptr) {
                ItemStack dropStack(blockItem, 1);
                math::Random rng;
                ItemDropHelper::spawnItemEntity(&world,
                    dropStack,
                    static_cast<f64>(currentPos.x) + 0.5,
                    static_cast<f64>(currentPos.y) + 0.5,
                    static_cast<f64>(currentPos.z) + 0.5,
                    rng);
            }
        }
        return VanillaBlocks::AIR->defaultState();
    }

    return state;
}

const BlockState& BellBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BellBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& BellBlock::getShape(const BlockState& state) const
{
    BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());

    switch (attachment) {
        case BlockStateProperties::BellAttachment::Floor:
            return m_floorShape;
        case BlockStateProperties::BellAttachment::Ceiling:
            return m_ceilingShape;
        case BlockStateProperties::BellAttachment::SingleWall:
        case BlockStateProperties::BellAttachment::DoubleWall:
            return m_wallShape;
        default:
            return m_floorShape;
    }
}

} // namespace blocks
} // namespace mc
