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

#include "CactusBlock.hpp"
#include "../../../../entity/core/LivingEntity.hpp"
#include "../../../../entity/damage/DamageSource.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

CactusBlock::CactusBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 仙人掌形状：稍小的方块
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充（详见其它方块注释）：
    // createBlockState 触发 _cacheProperties→propagatesSkylightDown→getOcclusionShape→getShape，
    // 构造期回调 getShape 需 m_shapesByAge 已就绪，否则依赖空 shape 的脆弱巧合。
    CollisionShape cactusShape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
    for (std::size_t i = 0; i < m_shapesByAge.size(); ++i) {
        m_shapesByAge[i] = cactusShape;
    }

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_15())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_15(), 0));
}

// ========== 状态属性 ==========

i32 CactusBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_15());
}

const BlockState& CactusBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_15(), std::clamp(age, 0, 15));
}

// ========== 放置逻辑 ==========

BlockState CactusBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool CactusBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    const bool supported = (VanillaBlocks::SAND != nullptr && belowState->is(VanillaBlocks::SAND)) ||
        (VanillaBlocks::RED_SAND != nullptr && belowState->is(VanillaBlocks::RED_SAND)) || belowState->is(this);

    if (!supported) {
        return false;
    }

    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        const BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);
        if (adjState != nullptr && !adjState->isAir() && adjState->getMaterial().isSolid()) {
            return false;
        }
    }

    return true;
}

BlockState CactusBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 检查周围是否有固体方块
    if (facing != Direction::Up && facing != Direction::Down) {
        if (facingState.getMaterial().isSolid()) {
            // 周围有固体方块，仙人掌应该被破坏 - 返回空气状态
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

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

// ========== 生长逻辑 ==========

void CactusBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }

    // 检查高度限制（最高3格）
    i32 height = 1;
    for (i32 i = 1; i < 3; ++i) {
        BlockPos checkPos(pos.x, pos.y - i, pos.z);
        const BlockState* checkState = world.getBlockState(checkPos);
        if (checkState == nullptr || !checkState->is(this)) {
            break;
        }
        height++;
    }

    if (height >= 3) {
        return; // 已达到最高高度
    }

    // 随机生长
    if (random.nextInt(16) == 0) {
        i32 age = getAge(state);
        if (age >= 15) {
            // 生长新的仙人掌
            const BlockState& newTopState = defaultState();
            world.setBlockState(abovePos, &newTopState, 2);

            const BlockState& resetState = withAge(0);
            world.setBlockState(pos, &resetState, 2);
        } else {
            // 增加年龄
            const BlockState& agedState = withAge(age + 1);
            world.setBlockState(pos, &agedState, 2);
        }
    }
}

// ========== 形状 ==========

const CollisionShape& CactusBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    return m_shapesByAge[static_cast<std::size_t>(std::min(age, 15))];
}

const CollisionShape& CactusBlock::getCollisionShape(const BlockState& state) const
{
    return getShape(state);
}

// ========== 实体交互 ==========

void CactusBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    auto* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity == nullptr) {
        return;
    }

    auto damageSource = DamageSources::cactus();
    livingEntity->hurt(damageSource, 1.0f);
}

// ========== IPlantable 接口 ==========

PlantType CactusBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Desert;
}

const BlockState& CactusBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
