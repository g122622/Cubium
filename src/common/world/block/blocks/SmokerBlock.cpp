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

#include "SmokerBlock.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/stats/Stats.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/AbstractFurnaceBlock.hpp"
#include "common/world/blockentity/processing/SmokerEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

SmokerBlock::SmokerBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties)
{}

std::unique_ptr<BlockEntity> SmokerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SmokerEntity>(pos);
}

bool SmokerBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player)
{
    if (world.openContainer(ContainerType::Furnace, pos, player)) {
        player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_SMOKER), 1);
        return true;
    }
    return false;
}

} // namespace blocks
} // namespace mc
