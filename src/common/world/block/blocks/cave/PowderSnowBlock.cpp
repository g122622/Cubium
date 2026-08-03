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

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <algorithm>

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

void PowderSnowBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 对应 MC Java 的 InsideBlockEffectType.FREEZE 效果

    // 1. 设置实体处于细雪中的标志
    // MC Java: entity.setIsInPowderSnow(true)
    entity.setIsInPowderSnow(true);

    // 2. 如果实体可以冰冻，增加冰冻计时器
    // MC Java: if (entity.canFreeze()) { entity.setTicksFrozen(Math.min(entity.getTicksRequiredToFreeze(),
    // entity.getTicksFrozen() + 1)); }
    if (entity.canFreeze()) {
        const i32 current = entity.getTicksFrozen();
        const i32 max = entity.getTicksRequiredToFreeze();
        entity.setTicksFrozen(std::min(max, current + 1));
    }

    // 3. 设置运动减速乘数，使实体在细雪中缓慢移动
    // 细雪的减速效果：XZ 轴 0.9，Y 轴 0.9
    // 参考 MC Java 的 PowderSnowBlock.canEntityWalkOnTop() 中的运动乘数
    entity.setMotionMultiplier(Vector3(0.9, 0.9, 0.9));
}

} // namespace blocks
} // namespace mc
