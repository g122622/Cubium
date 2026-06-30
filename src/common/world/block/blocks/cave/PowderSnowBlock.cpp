/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, WHETHER
 * ARISING FROM, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PowderSnowBlock.hpp"

#include "common/item/Items.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace blocks {

PowderSnowBlock::PowderSnowBlock(const BlockProperties& properties)
    : Block(properties)
{}

const CollisionShape& PowderSnowBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return VoxelShapes::empty();
}

fluid::Fluid* PowderSnowBlock::pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 细雪不是流体，使用 pickupItem 代替
    return nullptr;
}

const Item* PowderSnowBlock::pickupItem(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 将细雪方块替换为空气
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    if (airState != nullptr) {
        world.setBlockState(pos, airState, 3);
    }

    // 返回细雪桶物品
    return Items::POWDER_SNOW_BUCKET;
}

const ResourceLocation* PowderSnowBlock::getPickupSound(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    return &SoundEvents::ITEM_BUCKET_FILL_POWDER_SNOW;
}

} // namespace blocks
} // namespace mc
