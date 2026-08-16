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

#include "ScaffoldingBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 形状常量 (像素坐标) ==========

// 顶部平台边框 (0-16, 14-16, 0-16)
static constexpr f32 TOP_Y_MIN = 14.0f / 16.0f;
static constexpr f32 TOP_Y_MAX = 1.0f;

// 四个角的支柱 (每根 2x16x2 像素)
static constexpr f32 CORNER_SIZE = 2.0f / 16.0f;

// 底部平台 (0-16, 0-2, 0-16)
static constexpr f32 BOTTOM_Y_MIN = 0.0f;
static constexpr f32 BOTTOM_Y_MAX = 2.0f / 16.0f;

ScaffoldingBlock::ScaffoldingBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充（详见其它方块注释）：
    // createBlockState 触发 _cacheProperties→propagatesSkylightDown→getOcclusionShape→getShape，
    // 构造期回调 getShape 需 m_topShape/m_fullShape 等已就绪，否则依赖空 shape 的脆弱巧合。
    // 创建形状
    // 顶部平台：边框形状 (field_220121_d)
    // 四个角的支柱 + 顶部边缘
    m_topShape = CollisionShape::box(0.0f, TOP_Y_MIN, 0.0f, 1.0f, TOP_Y_MAX, 1.0f);

    // 底部平台 (field_220123_f)
    m_baseShape = CollisionShape::box(0.0f, BOTTOM_Y_MIN, 0.0f, 1.0f, BOTTOM_Y_MAX, 1.0f);

    // 完整形状 (field_220122_e)
    // 包含顶部平台、四个角支柱、底部平台
    m_fullShape = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::box(0.0f, TOP_Y_MIN, 0.0f, 1.0f, TOP_Y_MAX, 1.0f), // 顶部平台
            CollisionShape::box(0.0f, 0.0f, 0.0f, CORNER_SIZE, 1.0f, CORNER_SIZE)                  // 西北角
            ),
        CollisionShape::combine(CollisionShape::box(1.0f - CORNER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, CORNER_SIZE), // 东北角
            CollisionShape::combine(
                CollisionShape::box(0.0f, 0.0f, 1.0f - CORNER_SIZE, CORNER_SIZE, 1.0f, 1.0f),       // 西南角
                CollisionShape::box(1.0f - CORNER_SIZE, 0.0f, 1.0f - CORNER_SIZE, 1.0f, 1.0f, 1.0f) // 东南角
                )));

    // 空形状用于碰撞检测
    m_emptyShape = CollisionShape::empty();

    // 创建状态容器
    // DISTANCE 属性范围是 0-7
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DISTANCE_0_7())
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::BOTTOM())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态: distance=7, waterlogged=false, bottom=false
    setDefaultState(defaultState()
            .with(BlockStateProperties::DISTANCE_0_7(), 7)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::BOTTOM(), false));
}

BlockState ScaffoldingBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockPos pos = context.placementPos();
    IWorld& world = const_cast<IWorld&>(context.getWorld());

    // 计算距离支撑点的距离
    i32 distance = calculateDistance(world, pos);

    // 检查是否应该显示底部支撑柱
    bool bottom = shouldShowBottom(world, pos, distance);

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(context.getWorld(), pos);

    return defaultState()
        .with(BlockStateProperties::DISTANCE_0_7(), distance)
        .with(BlockStateProperties::BOTTOM(), bottom)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState ScaffoldingBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 调度 tick 以更新距离和底部状态
    world.tickManager().scheduleBlockTick(currentPos, *this, 1, world::tick::TickPriority::Normal);

    return state;
}

void ScaffoldingBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 方块添加时调度 tick
    world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::Normal);
}

void ScaffoldingBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 检查当前位置的方块是否还是脚手架
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || !isScaffolding(currentState)) {
        return;
    }

    // 重新计算距离
    i32 distance = calculateDistance(world, pos);
    bool bottom = shouldShowBottom(world, pos, distance);

    // 创建新状态
    BlockState newState =
        state.with(BlockStateProperties::DISTANCE_0_7(), distance).with(BlockStateProperties::BOTTOM(), bottom);

    // distance == 7 表示需要掉落
    if (distance == 7) {
        i32 previousDistance = state.get(BlockStateProperties::DISTANCE_0_7());

        if (previousDistance == 7) {
            // 已经是 distance=7，破坏方块并掉落物品

            // 移除方块
            const BlockState* airState = BlockRegistry::instance().airState();
            world.setBlockState(pos, airState, 3);

            // 掉落脚手架物品
            const Block* block = &currentState->getBlock();
            const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
            if (blockItem != nullptr) {
                // 创建物品堆
                ItemStack stack(*blockItem, 1);

                // 在方块中心位置生成物品实体
                math::Random& rng = world.getRandom();
                ItemDropHelper::spawnItemEntity(&world,
                    stack,
                    static_cast<f64>(pos.x) + 0.5,
                    static_cast<f64>(pos.y) + 0.5,
                    static_cast<f64>(pos.z) + 0.5,
                    rng);
            }
        } else {
            // 新的 distance=7，生成下落方块实体

            // 移除原方块
            const BlockState* airState = BlockRegistry::instance().airState();
            if (airState != nullptr && world.setBlockState(pos, airState, 3)) {
                // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
                auto* registry = world.entityRegistry();
                if (registry == nullptr) {
                    return;
                }

                // 创建下落实体
                auto fallingEntity = std::make_unique<entity::FallingBlockEntity>(*registry);
                fallingEntity->setTypeId(entity::EntityTypeKeys::FALLING_BLOCK);
                fallingEntity->setPosition(
                    static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
                fallingEntity->setVelocity(0.0f, 0.0f, 0.0f);
                fallingEntity->setBlockId(currentState->blockId());
                fallingEntity->setFallStartPos(static_cast<f64>(pos.y));
                // 脚手架下落时不伤害实体
                fallingEntity->setHurtEntities(false);

                const EntityInstanceId entityId = world.spawnEntity(std::move(fallingEntity));
                if (entityId == 0) {
                    // 生成失败，恢复方块
                    world.setBlockState(pos, currentState, 3);
                }
            }
        }
    } else if (state != newState) {
        // 状态改变，更新方块
        world.setBlockState(pos, &newState, 3);
    }
}

bool ScaffoldingBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 只有当距离 < 7 时才能放置
    return calculateDistance(world, pos) < 7;
}

const CollisionShape& ScaffoldingBlock::getShape(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    // 当 bottom=true 时返回完整形状，否则返回顶部平台
    if (state.get(BlockStateProperties::BOTTOM())) {
        return m_fullShape;
    }
    return m_topShape;
}

const CollisionShape& ScaffoldingBlock::getCollisionShape(const BlockState& state) const noexcept
{
    // 脚手架的碰撞形状比较特殊：
    // - 当 distance != 0 且 bottom=true 时，玩家可以站在底部平台上
    // - 其他情况下，玩家可以穿过脚手架
    i32 distance = state.get(BlockStateProperties::DISTANCE_0_7());
    bool bottom = state.get(BlockStateProperties::BOTTOM());

    if (distance != 0 && bottom) {
        // 返回底部平台形状，允许玩家站在上面
        return m_baseShape;
    }

    // 无碰撞，玩家可以穿过
    return m_emptyShape;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* ScaffoldingBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== 静态方法实现 ==========

i32 ScaffoldingBlock::calculateDistance(IWorld& world, const BlockPos& pos)
{
    // 从 pos 向下移动，然后遍历水平方向查找支撑
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);

    i32 minDistance = 7;

    if (belowState != nullptr) {
        // 检查下方方块
        if (isScaffolding(belowState)) {
            // 下方是脚手架，继承其距离
            minDistance = belowState->get(BlockStateProperties::DISTANCE_0_7());
        } else if (belowState->isSolidSide(world, belowPos, Direction::Up)) {
            // 下方方块的顶部是实体面，直接支撑
            return 0;
        }
    }

    // 检查下方位置的水平邻居脚手架
    static const Direction horizontalDirections[] = {
        Direction::North, Direction::East, Direction::South, Direction::West};

    for (Direction dir : horizontalDirections) {
        BlockPos neighborBelowPos = belowPos.offset(dir);
        const BlockState* neighborBelowState = world.getBlockState(neighborBelowPos);

        if (neighborBelowState != nullptr && isScaffolding(neighborBelowState)) {
            i32 neighborDistance = neighborBelowState->get(BlockStateProperties::DISTANCE_0_7());
            minDistance = std::min(minDistance, neighborDistance + 1);
            if (minDistance == 1) {
                break; // 已经找到最小距离，提前退出
            }
        }
    }

    return minDistance;
}

bool ScaffoldingBlock::shouldShowBottom(IWorld& world, const BlockPos& pos, i32 distance) noexcept
{
    // 如果 distance > 0 且下方不是脚手架，则显示底部支撑柱
    if (distance > 0) {
        BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        return !isScaffolding(belowState);
    }
    return false;
}

bool ScaffoldingBlock::isScaffolding(const BlockState* state) noexcept
{
    if (state == nullptr) {
        return false;
    }
    // 检查方块是否为脚手架类型
    // 使用 dynamic_cast 检查类型
    return dynamic_cast<const ScaffoldingBlock*>(&state->getBlock()) != nullptr;
}

} // namespace blocks
} // namespace mc
