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

#include "WoodPressurePlateBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/AbstractPressurePlateBlock.hpp"
#include <vector>

namespace mc {
namespace blocks {

// 木压力板tick延迟
static constexpr i32 WOOD_PLATE_DELAY = 10;

WoodPressurePlateBlock::WoodPressurePlateBlock(const BlockProperties& properties)
    : AbstractPressurePlateBlock(properties)
{}

i32 WoodPressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const
{
    // 木压力板可以被所有实体触发（玩家、怪物、物品等）
    // 对应 MC Java 的 PressurePlateSensitivity.EVERYTHING：使用 Entity.class 过滤
    // 有实体时输出15，无实体时输出0

    // 创建压力板上方的碰撞箱
    AxisAlignedBB detectionBox(static_cast<f32>(pos.x) + 0.125f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f,
        static_cast<f32>(pos.z) + 0.875f);

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 木压力板被任何实体触发，但排除不触发压力板的实体
    // （蝙蝠、标记模式盔甲架、不祥物品生成器等）
    for (Entity* entity : entities) {
        if (entity != nullptr && !entity->doesEntityNotTriggerPressurePlate()) {
            return 15; // 有实体就输出最大信号
        }
    }

    return 0;
}

i32 WoodPressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const
{
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return WOOD_PLATE_DELAY;
}

void WoodPressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const
{
    if (!world.isClientSide()) {
        world.playSound(pressed ? SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_ON
                                : SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_OFF,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.3f,
            0.6f);
    }
}

} // namespace blocks
} // namespace mc
