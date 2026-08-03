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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "JukeboxBlock.hpp"
#include "../../../../entity/inventory/IInventory.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/Item.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntityType.hpp"
#include "../../../blockentity/interactive/JukeboxEntity.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== JukeboxBlock 实现 ==========

JukeboxBlock::JukeboxBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HAS_RECORD())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HAS_RECORD(), false));

    // 唱片机形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState JukeboxBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

const CollisionShape& JukeboxBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

std::unique_ptr<BlockEntity> JukeboxBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::JukeboxEntity>(pos);
}

int JukeboxBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 从唱片机方块实体获取比较器信号
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Jukebox) {
        auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);
        return jukebox->getComparatorSignal();
    }

    return 0;
}

BlockActionResult JukeboxBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::Jukebox) {
        return ActionResultType::Pass;
    }

    auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);

    // 如果有唱片，取出唱片
    if (hasRecord(state)) {
        ItemStack record = jukebox->getRecord();
        if (!record.isEmpty()) {
            // 停止播放
            jukebox->stopPlaying(world);

            // 清空唱片（内部不再更新方块状态，由 Block 层负责）
            jukebox->setRecord(ItemStack::EMPTY, world);

            // 更新方块状态为无唱片
            BlockState newState = state.with(BlockStateProperties::HAS_RECORD(), false);
            world.setBlockState(pos, &newState, 3);

            // 掉落唱片到方块上方
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world, record, pos.x + 0.5, pos.y + 1.01, pos.z + 0.5, rng);

            return ActionResultType::Consume;
        }
    }

    // 如果没有唱片，检查玩家手中是否有唱片
    ItemStack& heldItem = player.getHeldItem(hand);
    if (!heldItem.isEmpty() && heldItem.getItem() != nullptr && heldItem.getItem()->isMusicDisc()) {
        // 放入唱片
        ItemStack recordToInsert = heldItem.copy();
        recordToInsert.setCount(1);

        // 设置唱片到方块实体（内部会开始播放）
        jukebox->setRecord(recordToInsert, world);

        // 更新方块状态为有唱片
        BlockState newState = state.with(BlockStateProperties::HAS_RECORD(), true);
        world.setBlockState(pos, &newState, 3);

        // 消耗玩家手中的唱片（非创造模式）
        if (!player.isCreative()) {
            heldItem.shrink(1);
        }

        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

void JukeboxBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 方块移除时掉落唱片机内的唱片
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Jukebox) {
        auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);

        // 停止播放
        jukebox->stopPlaying(world);

        // 获取并掉落唱片
        ItemStack record = jukebox->getRecord();
        if (!record.isEmpty()) {
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world, record, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, rng);
        }
    }

    Block::onBlockRemoved(world, pos, state);
}

} // namespace blocks
} // namespace mc
