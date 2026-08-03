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

#include "WeightedPressurePlateBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/AbstractPressurePlateBlock.hpp"
#include <algorithm>
#include <vector>

namespace mc {
namespace blocks {

// 测重压力板tick延迟
static constexpr i32 WEIGHTED_PLATE_DELAY = 10;

WeightedPressurePlateBlock::WeightedPressurePlateBlock(const BlockProperties& properties, Sensitivity sensitivity)
    : AbstractPressurePlateBlock(properties)
    , m_sensitivity(sensitivity)
{}

i32 WeightedPressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const
{
    i32 count = _getEntityCount(world, pos);

    if (count <= 0) {
        return 0;
    }

    switch (m_sensitivity) {
        case Sensitivity::Light:
            // 轻质：每物品+1信号强度，最大15
            return std::min(count, 15);
        case Sensitivity::Heavy:
            // 重质：每10物品+1信号强度，最大15
            return std::min(count / 10, 15);
        default:
            return 0;
    }
}

i32 WeightedPressurePlateBlock::getTickDelay(bool oldPowered, bool newPowered) const
{
    MC_UNUSED(oldPowered);
    MC_UNUSED(newPowered);
    return WEIGHTED_PLATE_DELAY;
}

void WeightedPressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const
{
    // 测重压力板（金属材质）点击音效
    if (!world.isClientSide()) {
        world.playSound(pressed ? SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_ON
                                : SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_OFF,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.3f,
            0.6f);
    }
}

i32 WeightedPressurePlateBlock::_getEntityCount(IWorld& world, const BlockPos& pos) const
{
    // 创建压力板上方的碰撞箱
    AxisAlignedBB detectionBox(static_cast<f32>(pos.x) + 0.125f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f,
        static_cast<f32>(pos.z) + 0.875f);

    // 查询碰撞箱内的实体
    // 对应 MC Java 的 getEntityCount(Level, AABB, Entity.class)
    // 测重压力板检测所有实体，但排除不触发压力板的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 计算实体数量（排除不触发压力板的实体）
    i32 count = 0;
    for (Entity* entity : entities) {
        if (entity != nullptr && !entity->doesEntityNotTriggerPressurePlate()) {
            count++;
        }
    }

    return count;
}

} // namespace blocks
} // namespace mc
