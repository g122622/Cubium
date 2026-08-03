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

#include "BambooBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// BambooBlock 实现
// ============================================================================

BambooBlock::BambooBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_1())
            .add(BlockStateProperties::STAGE_0_1())
            .add(BlockStateProperties::BAMBOO_LEAVES_PROP())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：年龄0，阶段0，无叶子
    setDefaultState(defaultState()
            .with(BlockStateProperties::AGE_0_1(), 0)
            .with(BlockStateProperties::STAGE_0_1(), 0)
            .with(BlockStateProperties::BAMBOO_LEAVES_PROP(), BlockStateProperties::BambooLeaves::None));

    // 创建形状
    // 正常形状：稍小的方块（类似甘蔗）
    m_normalShape = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 1.0f, 0.8125f);

    // 大叶子形状：带叶子的竹子有更大的碰撞箱
    m_largeLeavesShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);

    // 碰撞形状：与其他方块相同，用于碰撞检测
    m_collisionShape = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 1.0f, 0.8125f);
}

const EnumProperty<BlockStateProperties::BambooLeaves>& BambooBlock::BAMBOO_LEAVES_PROP()
{
    return BlockStateProperties::BAMBOO_LEAVES_PROP();
}

BlockState BambooBlock::getStateForPlacement(BlockItemUseContext& context)
{
    IBlockReader& world = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(context.getWorld()));
    BlockPos pos = context.placementPos();

    // 检查下方是否有竹子
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState != nullptr && belowState->is(this)) {
        // 继承下方竹子的叶子类型
        BlockStateProperties::BambooLeaves leaves = belowState->get(BlockStateProperties::BAMBOO_LEAVES_PROP());
        return defaultState().with(BlockStateProperties::BAMBOO_LEAVES_PROP(), leaves);
    }

    return defaultState();
}

bool BambooBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 竹子可以放在竹子上
    if (belowState->is(this)) {
        return true;
    }

    // 或者放在竹子可种植的方块上
    return BlockTags::BAMBOO_PLANTABLE_ON().contains(*belowState);
}

BlockState BambooBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查下方支撑
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

void BambooBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查上方是否有空间（最高生长到 MAX_BUILD_HEIGHT - 1）
    if (pos.y >= world::MAX_BUILD_HEIGHT - 1) {
        return;
    }

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }

    // 获取竹子高度
    i32 bambooHeight = _getNumBambooBlocksBelow(static_cast<IBlockReader&>(world), pos) + 1;

    // 检查高度限制
    if (bambooHeight >= BAMBOO_MAX_HEIGHT) {
        return;
    }

    // 获取阶段
    i32 stage = state.get(BlockStateProperties::STAGE_0_1());

    // 阶段0时，随机增加年龄或生长
    if (stage == 0) {
        if (random.nextInt(3) == 0) {
            _growBamboo(state, world, pos, random, bambooHeight);
        }
    } else {
        // 阶段1时，必定生长（来自骨粉）
        _growBamboo(state, world, pos, random, bambooHeight);
    }
}

bool BambooBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // 竹子总是可以生长（如果高度未达到限制）
    return true;
}

bool BambooBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // 骨粉有概率成功
    return random.nextFloat() < 0.45f;
}

void BambooBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    // 获取高度
    i32 bambooHeight = _getNumBambooBlocksBelow(static_cast<IBlockReader&>(world), pos) + 1;

    // 生长1-2格
    i32 growthAmount = 1 + random.nextInt(2);

    for (i32 i = 0; i < growthAmount && bambooHeight < BAMBOO_MAX_HEIGHT; ++i) {
        BlockPos abovePos(pos.x, pos.y + i + 1, pos.z);
        const BlockState* aboveState = world.getBlockState(abovePos);

        if (aboveState == nullptr || aboveState->isAir()) {
            _growBamboo(state, world, pos.offset(Direction::Up, i), random, bambooHeight + i);
        } else {
            break;
        }
    }
}

const CollisionShape& BambooBlock::getShape(const BlockState& state) const noexcept
{
    BlockStateProperties::BambooLeaves leaves = state.get(BlockStateProperties::BAMBOO_LEAVES_PROP());

    if (leaves == BlockStateProperties::BambooLeaves::Large) {
        return m_largeLeavesShape;
    }

    return m_normalShape;
}

const CollisionShape& BambooBlock::getCollisionShape(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return m_collisionShape;
}

bool BambooBlock::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const noexcept
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return true;
}

i32 BambooBlock::_getNumBambooBlocksBelow(IBlockReader& world, const BlockPos& pos) const
{
    i32 count = 0;
    BlockPos checkPos = pos;

    for (i32 i = 0; i < BAMBOO_MAX_HEIGHT; ++i) {
        checkPos = checkPos.down();
        const BlockState* checkState = world.getBlockState(checkPos);

        if (checkState == nullptr || !checkState->is(this)) {
            break;
        }

        count++;
    }

    return count;
}

void BambooBlock::_growBamboo(
    const BlockState& currentState, IWorld& world, const BlockPos& pos, math::IRandom& random, i32 bambooHeight)
{
    // 检查高度限制
    if (bambooHeight >= BAMBOO_MAX_HEIGHT) {
        return;
    }

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);

    // 计算新的叶子类型
    BlockStateProperties::BambooLeaves newLeaves = BlockStateProperties::BambooLeaves::None;

    if (bambooHeight >= 1) {
        // 随机选择小叶子或大叶子
        if (random.nextInt(3) == 0) {
            newLeaves = BlockStateProperties::BambooLeaves::Large;
        } else {
            newLeaves = BlockStateProperties::BambooLeaves::Small;
        }
    }

    // 更新当前竹子的叶子类型
    BlockState updatedState = currentState.with(BlockStateProperties::BAMBOO_LEAVES_PROP(), newLeaves);
    world.setBlockState(pos, &updatedState, 2);

    // 在上方放置新的竹子
    BlockState newState =
        defaultState()
            .with(BlockStateProperties::AGE_0_1(), random.nextInt(2))
            .with(BlockStateProperties::STAGE_0_1(), 0)
            .with(BlockStateProperties::BAMBOO_LEAVES_PROP(), BlockStateProperties::BambooLeaves::None);

    world.setBlockState(abovePos, &newState, 3);
}

// ========== IPlantable 接口实现 ==========

PlantType BambooBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Beach;
}

const BlockState& BambooBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return defaultState();
}

// ============================================================================
// BambooSaplingBlock 实现
// ============================================================================

BambooSaplingBlock::BambooSaplingBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 竹子幼苗没有状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [this](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 幼苗形状
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.3125f, 0.75f);
}

bool BambooSaplingBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 幼苗只能放在竹子可种植的方块上（不能放在竹子上）
    if (VanillaBlocks::BAMBOO != nullptr && belowState->is(VanillaBlocks::BAMBOO)) {
        return false;
    }

    return BlockTags::BAMBOO_PLANTABLE_ON().contains(*belowState);
}

BlockState BambooSaplingBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查下方支撑
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

void BambooSaplingBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 随机生长（1/8概率）
    if (random.nextInt(8) == 0) {
        _growBamboo(world, pos);
    }
}

bool BambooSaplingBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    return true;
}

bool BambooSaplingBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    return random.nextFloat() < 0.45f;
}

void BambooSaplingBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);
    MC_UNUSED(state);

    _growBamboo(world, pos);
}

const CollisionShape& BambooSaplingBlock::getShape(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return m_shape;
}

void BambooSaplingBlock::_growBamboo(IWorld& world, const BlockPos& pos)
{
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }

    // 将幼苗替换为竹子
    if (VanillaBlocks::BAMBOO != nullptr) {
        const BlockState& bambooState = VanillaBlocks::BAMBOO->defaultState();
        world.setBlockState(pos, &bambooState, 3);
    }
}

// ========== IPlantable 接口实现 ==========

PlantType BambooSaplingBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Beach;
}

const BlockState& BambooSaplingBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return defaultState();
}

} // namespace blocks
} // namespace mc
