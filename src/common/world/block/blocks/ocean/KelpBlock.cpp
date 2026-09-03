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

#include "KelpBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

KelpBlock::KelpBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    // vanilla 1.21.11 kelp 仅有 age（始终在水下，无 waterlogged 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_25())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_25(), 0));

    // 海带形状：细长
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f);
}

i32 KelpBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_25());
}

BlockState KelpBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_25(), std::min(age, 25));
}

BlockState KelpBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    // 海带必须在水中
    return defaultState().with(BlockStateProperties::AGE_0_25(), 0);
}

bool KelpBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 可以放置在海带上方或固体方块上
    if (belowState->is(this)) {
        return true;
    }

    if (VanillaBlocks::KELP_PLANT != nullptr && belowState->is(VanillaBlocks::KELP_PLANT)) {
        return true;
    }

    return belowState->isSolid();
}

BlockState KelpBlock::updatePostPlacement(const BlockState& state,
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

void KelpBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return; // 上方被占用
    }

    // 检查高度限制（基于年龄）
    i32 age = getAge(state);
    if (age >= 25) {
        return; // 已达到最大高度
    }

    // 随机生长
    if (random.nextFloat() < 0.14f) { // 约14%概率
        // 增加上方海带
        const BlockState& kelpState = defaultState();
        world.setBlockState(abovePos, &kelpState, 2);
        const BlockState& agedState = withAge(age + 1);
        world.setBlockState(pos, &agedState, 2);
    }
}

// ========== IGrowable 接口实现 ==========

bool KelpBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(isClientSide);
    // wiki tech_海带.txt#生长（:63）："对海带使用骨粉可使其生长一格。"
    // 条件：age < 25（未达最大年龄）且上方为空气（可生长）。
    const i32 age = getAge(state);
    if (age >= 25) {
        return false;
    }

    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    return aboveState != nullptr && aboveState->isAir();
}

bool KelpBlock::canUseBonemeal(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // wiki :63 骨粉使海带生长一格，必定成功。
    return true;
}

void KelpBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);
    // wiki tech_海带.txt#生长（:63）："对海带使用骨粉可使其生长一格。"
    // 在上方放置新的海带方块（defaultState, age=0），与 randomTick 生长逻辑一致。
    const i32 age = getAge(state);
    if (age >= 25) {
        return;
    }

    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr || !aboveState->isAir()) {
        return;
    }

    const BlockState& kelpState = defaultState();
    world.setBlockState(abovePos, &kelpState, 2);
    const BlockState& agedState = withAge(age + 1);
    world.setBlockState(pos, &agedState, 2);
}

const CollisionShape& KelpBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& KelpBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== IPlantable 接口实现 ==========

PlantType KelpBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Water;
}

const BlockState& KelpBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
