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

#include "JukeboxBlock.hpp"
#include "../../../../entity/inventory/IInventory.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntityType.hpp"
#include "../../../blockentity/interactive/JukeboxEntity.hpp"

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

    // 从唱片机方块实体获取比较器信号
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Jukebox) {
        auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);
        return jukebox->getComparatorSignal();
    }

    // 有唱片时输出1，无唱片时输出0
    return hasRecord(state) ? 1 : 0;
}

ActionResultType JukeboxBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hand);
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

            // 清空唱片
            jukebox->setRecord(ItemStack::EMPTY);

            // 掉落唱片
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world, record, pos.x + 0.5, pos.y + 1.0, pos.z + 0.5, rng);

            // 更新方块状态
            BlockState newState = state.with(BlockStateProperties::HAS_RECORD(), false);
            world.setBlockState(pos, &newState, 3);

            return ActionResultType::Consume;
        }
    }

    // TODO: 如果玩家手中有唱片，放入唱片

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

void JukeboxBlock::setRecord(IWorld& world, const BlockPos& pos, BlockState& state, bool hasRecord)
{
    BlockState newState = state.with(BlockStateProperties::HAS_RECORD(), hasRecord);
    world.setBlockState(pos, &newState, 3);

    // 更新方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Jukebox) {
        auto* jukebox = static_cast<blockentity::JukeboxEntity*>(entity);
        if (hasRecord) {
            jukebox->startPlaying(world);
        } else {
            jukebox->stopPlaying(world);
        }
    }
}

} // namespace blocks
} // namespace mc
