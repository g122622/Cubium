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

#include "BlastFurnaceBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../../stats/Stats.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/processing/BlastFurnaceEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/AbstractFurnaceBlock.hpp"
#include <memory>

namespace mc {
namespace blocks {

BlastFurnaceBlock::BlastFurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties)
{}

std::unique_ptr<BlockEntity> BlastFurnaceBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::BlastFurnaceEntity>(pos);
}

bool BlastFurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player)
{
    if (world.openContainer(ContainerType::Furnace, pos, player)) {
        player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_BLAST_FURNACE), 1);
        return true;
    }
    return false;
}

} // namespace blocks
} // namespace mc
