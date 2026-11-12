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

#include "common/world/block/blocks/agricultural/AttachedStemBlock.hpp"

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/base/types/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== AttachedStemBlock 实现 ==========

AttachedStemBlock::AttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties)
    : BushBlock(properties)
    , m_crop(crop)
{
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充。
    // 原因：createBlockState 会为每个 BlockState 调用 _cacheProperties，其中
    // getOpacity→propagatesSkylightDown→getOcclusionShape→getShape 会在 BlockState 构造期
    // （此时本构造函数体尚未执行到此处）回调 getShape。若 m_shapesByDirection 此时为空，
    // getShape 的 fallback（m_shapesByDirection.at(North)）会命中空 unordered_map 抛
    // std::out_of_range("invalid unordered_map<K, T> key")。对齐 PointedDripstoneBlock /
    // AmethystClusterBlock 的正确构造顺序（先填 shape 再 createBlockState）。

    // 预计算各方向的形状
    // 形状从中心 (6, 0, 6) 延伸到对应方向的边缘，高度 10 像素
    constexpr f32 P = 1.0f / 16.0f;

    // North: (6, 0, 0) -> (10, 10, 10)
    m_shapesByDirection[Direction::North] = CollisionShape::box(6.0f * P, 0.0f, 0.0f, 10.0f * P, 10.0f * P, 10.0f * P);

    // South: (6, 0, 6) -> (10, 10, 16)
    m_shapesByDirection[Direction::South] =
        CollisionShape::box(6.0f * P, 0.0f, 6.0f * P, 10.0f * P, 10.0f * P, 16.0f * P);

    // West: (0, 0, 6) -> (10, 10, 10)
    m_shapesByDirection[Direction::West] =
        CollisionShape::box(0.0f * P, 0.0f, 6.0f * P, 10.0f * P, 10.0f * P, 10.0f * P);

    // East: (6, 0, 6) -> (16, 10, 10)
    m_shapesByDirection[Direction::East] =
        CollisionShape::box(6.0f * P, 0.0f, 6.0f * P, 16.0f * P, 10.0f * P, 10.0f * P);

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

BlockState AttachedStemBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), context.horizontalDirection());
}

BlockState AttachedStemBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 获取茎指向的方向
    Direction stemFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 如果更新方向是茎指向的方向，检查该方向是否还是果实
    if (facing == stemFacing) {
        // 检查邻居是否是对应的果实方块
        if (m_crop != nullptr && !facingState.is(m_crop)) {
            // 果实不存在了，变回普通茎（AGE=7）
            const Block* stem = m_crop->getStem();
            if (stem != nullptr) {
                return stem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
            }
        }
    }

    // 其他情况调用父类处理（下方支撑检查等）
    return BushBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

const CollisionShape& AttachedStemBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    // 默认返回北方向的形状
    return m_shapesByDirection.at(Direction::North);
}

const BlockState& AttachedStemBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& AttachedStemBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

PlantType AttachedStemBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Crop;
}

} // namespace blocks
} // namespace mc
