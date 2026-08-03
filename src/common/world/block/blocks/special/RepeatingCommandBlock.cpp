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

#include "RepeatingCommandBlock.hpp"

#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/special/CommandBlock.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/redstone/CommandBlockEntity.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ========== RepeatingCommandBlock ==========

RepeatingCommandBlock::RepeatingCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties)
{}

void RepeatingCommandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 获取方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }

    auto* commandEntity = static_cast<blockentity::CommandBlockEntity*>(entity);

    // 循环模式（AUTO）：每 tick 执行
    if (commandEntity->getMode() == blockentity::CommandBlockMode::Auto) {
        // 检查条件
        bool conditional = isConditional(state);
        if (!commandEntity->checkCondition(world, getFacing(state), conditional)) {
            commandEntity->setSuccessCount(0);
        } else {
            // 执行命令
            execute(world, pos, state, commandEntity);
        }

        // 无条件通知比较器更新信号
        world::redstone::RedstoneSystem::instance().updateComparators(world, pos);

        // 如果仍然被供电或自动执行，重新调度下一 tick
        if (commandEntity->isPowered() || commandEntity->isAuto()) {
            world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
        }
    }
}

std::unique_ptr<BlockEntity> RepeatingCommandBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CommandBlockEntity>(pos, blockentity::CommandBlockMode::Auto);
}

} // namespace blocks
} // namespace mc
