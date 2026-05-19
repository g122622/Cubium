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

#include "StonePressurePlateBlock.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/core/EntityTypeIdNumber.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/AxisAlignedBB.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// 石头压力板tick延迟
static constexpr i32 STONE_PLATE_DELAY = 10;

StonePressurePlateBlock::StonePressurePlateBlock(const BlockProperties& properties)
    : AbstractPressurePlateBlock(properties)
{}

i32 StonePressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const
{
    // 石头压力板只能被生物触发，物品不会触发
    // 有生物时输出15，无生物时输出0

    // 创建压力板上方的碰撞箱
    AxisAlignedBB detectionBox(static_cast<f32>(pos.x) + 0.125f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f,
        static_cast<f32>(pos.z) + 0.875f);

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 石头压力板只被生物触发（不包括物品）
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            entity::EntityTypeId type = entity->typeId();
            // 石头压力板只检测玩家（后续可添加 Mob 类型）
            if (type == entity::EntityTypeIdNumber::PLAYER) {
                return 15; // 有生物就输出最大信号
            }
            // Item 类型不触发石头压力板
        }
    }

    return 0;
}

i32 StonePressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const
{
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return STONE_PLATE_DELAY;
}

void StonePressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const
{
    // 参考 MC 1.16.5: StonePressurePlateBlock.playClickSound
    world.playSound(
        pressed ? SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_ON : SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_OFF,
        sound::SoundCategory::Blocks,
        pos.center(),
        0.3f,
        0.6f);
}

} // namespace blocks
} // namespace mc
