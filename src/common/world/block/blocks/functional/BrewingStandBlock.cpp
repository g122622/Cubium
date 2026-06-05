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

#include "BrewingStandBlock.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/inventory/IInventory.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntityType.hpp"
#include "../../../blockentity/processing/BrewingStandEntity.hpp"

namespace mc {
namespace blocks {

// ========== BrewingStandBlock 实现 ==========

BrewingStandBlock::BrewingStandBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HAS_BOTTLE_0())
            .add(BlockStateProperties::HAS_BOTTLE_1())
            .add(BlockStateProperties::HAS_BOTTLE_2())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HAS_BOTTLE_0(), false)
            .with(BlockStateProperties::HAS_BOTTLE_1(), false)
            .with(BlockStateProperties::HAS_BOTTLE_2(), false));

    // 创建酿造台形状
    // 主体是底座 + 中间的柱子
    constexpr f32 P = 1.0f / 16.0f;
    CollisionShape base = CollisionShape::box(1.0f * P, 0.0f, 1.0f * P, 15.0f * P, 2.0f * P, 15.0f * P);
    CollisionShape pole = CollisionShape::box(7.0f * P, 0.0f, 7.0f * P, 9.0f * P, 14.0f * P, 9.0f * P);
    m_shape = CollisionShape::combine(base, pole);
}

BlockState BrewingStandBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

const CollisionShape& BrewingStandBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& BrewingStandBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

int BrewingStandBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 从酿造台方块实体获取比较器信号
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::BrewingStand) {
        auto* brewingStand = static_cast<blockentity::BrewingStandEntity*>(entity);
        return brewingStand->getComparatorSignal();
    }

    return 0;
}

std::unique_ptr<BlockEntity> BrewingStandBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BrewingStandEntity>(pos);
}

ActionResultType BrewingStandBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::BrewingStand) {
        auto* brewingStand = static_cast<blockentity::BrewingStandEntity*>(entity);
        // 打开酿造台GUI
        // TODO: 实现容器打开
        // player.openContainer(brewingStand);
        // player.addStat(Stats::INTERACT_WITH_BREWINGSTAND);
        MC_UNUSED(brewingStand);
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

void BrewingStandBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 方块实体在createBlockEntity中创建
    // 注意：自定义名称需要在放置时从物品获取，但当前项目架构不支持在onBlockPlacedBy中访问放置物品
    // TODO: 当架构支持后，实现从放置物品获取自定义名称
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

void BrewingStandBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 方块移除时掉落酿造台内的物品
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::BrewingStand) {
        auto* brewingStand = static_cast<blockentity::BrewingStandEntity*>(entity);
        IInventory* inventory = brewingStand->getInventory();

        // 掉落所有物品
        math::Random rng;
        for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
            ItemStack stack = inventory->removeItemNoUpdate(i);
            if (!stack.isEmpty()) {
                ItemDropHelper::spawnItemEntity(&world, stack, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, rng);
            }
        }
    }

    Block::onBlockRemoved(world, pos, state);
}

} // namespace blocks
} // namespace mc
