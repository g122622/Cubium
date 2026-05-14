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

#include "GrindstoneBlock.hpp"
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

// ========== GrindstoneBlock 实现 ==========

GrindstoneBlock::GrindstoneBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::HORIZONTAL_FACING())
                         .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), id);
                         });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // 创建砂轮形状
    // 底座 + 侧柱 + 砂轮
    constexpr f32 P = 1.0f / 16.0f;

    // 简化形状：侧柱 + 砂轮
    CollisionShape postLeft = CollisionShape::box(0.0f, 0.0f, 0.0f, 2.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape postRight = CollisionShape::box(14.0f * P, 0.0f, 0.0f, 16.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape wheel = CollisionShape::box(2.0f * P, 4.0f * P, 0.0f, 14.0f * P, 12.0f * P, 2.0f * P);

    CollisionShape baseShape = CollisionShape::combine(CollisionShape::combine(postLeft, postRight), wheel);

    // 各朝向旋转
    // 北朝向
    m_shapesByFacing[static_cast<size_t>(Direction::North)] = baseShape;

    // 南朝向
    m_shapesByFacing[static_cast<size_t>(Direction::South)] = baseShape;

    // 西朝向 - 旋转90度
    CollisionShape postLeftW = CollisionShape::box(0.0f, 0.0f, 0.0f, 2.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape postRightW = CollisionShape::box(0.0f, 0.0f, 14.0f * P, 2.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wheelW = CollisionShape::box(0.0f, 4.0f * P, 2.0f * P, 2.0f * P, 12.0f * P, 14.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::West)] =
        CollisionShape::combine(CollisionShape::combine(postLeftW, postRightW), wheelW);

    // 东朝向
    CollisionShape postLeftE = CollisionShape::box(14.0f * P, 0.0f, 0.0f, 16.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape postRightE = CollisionShape::box(14.0f * P, 0.0f, 14.0f * P, 16.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wheelE = CollisionShape::box(14.0f * P, 4.0f * P, 2.0f * P, 16.0f * P, 12.0f * P, 14.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::East)] =
        CollisionShape::combine(CollisionShape::combine(postLeftE, postRightE), wheelE);

    m_collisionShape = baseShape;
}

BlockState GrindstoneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
}

bool GrindstoneBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 砂轮需要附着在墙上
    // 检查后方是否有支撑
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockPos behindPos(pos.x + Directions::xOffset(facing), pos.y, pos.z + Directions::zOffset(facing));
    const BlockState* behindState = world.getBlockState(behindPos);

    if (behindState == nullptr) {
        return false;
    }

    return behindState->isSolid();
}

BlockState GrindstoneBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    Direction grindstoneFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 检查附着的墙是否还存在
    // 参考 MC 1.16.5: GrindstoneBlock.updatePostPlacement
    if (facing == grindstoneFacing) {
        if (!facingState.isSolid()) {
            // 墙被移除，掉落砂轮物品
            const Block* block = &state.getBlock();
            if (block != nullptr) {
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
    }

    return state;
}

const BlockState& GrindstoneBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& GrindstoneBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& GrindstoneBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

const CollisionShape& GrindstoneBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_collisionShape;
}

} // namespace blocks
} // namespace mc
