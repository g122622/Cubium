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

#include "AnvilBlock.hpp"

#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"

namespace mc {
namespace blocks {

// ========== 铁砧伤害参数（参考 MC 原版 AnvilBlock） ==========

/// 铁砧每格下落伤害系数
static constexpr f32 ANVIL_FALL_DAMAGE_PER_DISTANCE = 2.0f;

/// 铁砧最大伤害值
static constexpr i32 ANVIL_FALL_DAMAGE_MAX = 40;

AnvilBlock::AnvilBlock(const BlockProperties& properties)
    : FallingBlock(properties)
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认朝向为北
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

void AnvilBlock::onStartFalling(IWorld& /*world*/, const BlockPos& /*pos*/, entity::FallingBlockEntity& entity)
{
    // 铁砧下落时设置伤害参数
    entity.setHurtEntities(true);
    entity.setFallDamagePerDistance(ANVIL_FALL_DAMAGE_PER_DISTANCE);
    entity.setFallDamageMax(ANVIL_FALL_DAMAGE_MAX);
}

void AnvilBlock::onEndFalling(IWorld& world,
    const BlockPos& pos,
    const BlockState& /*fallingState*/,
    const BlockState& /*hitState*/,
    entity::FallingBlockEntity& entity)
{
    MC_UNUSED(entity);
    // 播放铁砧落地音效
    world.playEvent(world::WorldEvents::ANVIL_LAND_SOUND, pos, 0);
}

void AnvilBlock::onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity)
{
    MC_UNUSED(entity);
    // 播放铁砧破碎音效
    world.playEvent(world::WorldEvents::ANVIL_DESTROYED_SOUND, pos, 0);
}

BlockState AnvilBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家水平朝向设置方块方向
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

const BlockState& AnvilBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = currentFacing;

    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(currentFacing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(currentFacing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(currentFacing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& AnvilBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = currentFacing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (currentFacing == Direction::East) {
                newFacing = Direction::West;
            } else if (currentFacing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (currentFacing == Direction::North) {
                newFacing = Direction::South;
            } else if (currentFacing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState* AnvilBlock::damageAnvil(const BlockState& state)
{
    const Block& block = state.getBlock();
    const ResourceLocation& loc = block.blockLocation();

    // 获取当前铁砧的朝向，用于设置损坏后的状态
    Direction facing = Direction::North;
    if (state.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    }

    // anvil → chipped_anvil
    if (loc == ResourceLocation("minecraft", "anvil")) {
        const Block* nextBlock = block_registry::BuildingBlocks::CHIPPED_ANVIL;
        if (nextBlock != nullptr) {
            return &nextBlock->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        }
        return nullptr;
    }

    // chipped_anvil → damaged_anvil
    if (loc == ResourceLocation("minecraft", "chipped_anvil")) {
        const Block* nextBlock = block_registry::BuildingBlocks::DAMAGED_ANVIL;
        if (nextBlock != nullptr) {
            return &nextBlock->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        }
        return nullptr;
    }

    // damaged_anvil → 完全损坏（返回 nullptr）
    if (loc == ResourceLocation("minecraft", "damaged_anvil")) {
        return nullptr;
    }

    // 非铁砧方块，不进行损坏
    return nullptr;
}

} // namespace blocks
} // namespace mc
