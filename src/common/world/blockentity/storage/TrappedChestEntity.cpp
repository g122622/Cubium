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

#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "common/core/Types.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include <algorithm>
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

TrappedChestEntity::TrappedChestEntity(const BlockPos& pos)
    : ChestEntity(BlockEntityType::TrappedChest, pos)
{}

std::unique_ptr<BlockEntity> TrappedChestEntity::clone() const
{
    auto cloned = std::make_unique<TrappedChestEntity>(getPos());

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "TrappedChestEntity clone load failed");

    return cloned;
}

i32 TrappedChestEntity::getRedstoneSignal(IWorld& world) const
{
    // 信号强度等于打开玩家数，但不超过15
    i32 playerCount = getOpenCount();

    // 如果是双箱，需要计算两个箱子的打开数
    ChestEntity* connected = getConnectedChest(world);
    if (connected) {
        playerCount += connected->getOpenCount();
    }

    return std::min(playerCount, 15);
}

void TrappedChestEntity::openContainer(Player* player)
{
    // 先增加计数
    ChestEntity::openContainer(player);

    if (m_world != nullptr) {
        _notifyNeighbors(*m_world);

        // 双箱时，同步通知连接箱子的邻居（红石信号变化也会影响其邻居）
        ChestEntity* connected = getConnectedChest(*m_world);
        if (connected != nullptr) {
            const BlockState* connectedState = m_world->getBlockState(connected->getPos());
            if (connectedState != nullptr) {
                world::redstone::RedstoneSystem::instance().updateNeighbors(
                    *m_world, connected->getPos(), connectedState->getBlockMutable());
                world::redstone::RedstoneSystem::instance().updateComparators(*m_world, connected->getPos());
            }
        }
    }
}

void TrappedChestEntity::closeContainer(Player* player)
{
    // 先减少计数
    ChestEntity::closeContainer(player);

    if (m_world != nullptr) {
        _notifyNeighbors(*m_world);

        // 双箱时，同步通知连接箱子的邻居
        ChestEntity* connected = getConnectedChest(*m_world);
        if (connected != nullptr) {
            const BlockState* connectedState = m_world->getBlockState(connected->getPos());
            if (connectedState != nullptr) {
                world::redstone::RedstoneSystem::instance().updateNeighbors(
                    *m_world, connected->getPos(), connectedState->getBlockMutable());
                world::redstone::RedstoneSystem::instance().updateComparators(*m_world, connected->getPos());
            }
        }
    }
}

void TrappedChestEntity::_notifyNeighbors(IWorld& world)
{
    const BlockState* state = world.getBlockState(getPos());
    if (state == nullptr) {
        return;
    }

    world::redstone::RedstoneSystem::instance().updateNeighbors(world, getPos(), state->getBlockMutable());
    world::redstone::RedstoneSystem::instance().updateComparators(world, getPos());
}

} // namespace blockentity
} // namespace mc
