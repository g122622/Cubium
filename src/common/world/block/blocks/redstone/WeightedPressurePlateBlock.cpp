#include "WeightedPressurePlateBlock.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/AxisAlignedBB.hpp"
#include "../../../IWorld.hpp"

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
    i32 count = getEntityCount(world, pos);

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

i32 WeightedPressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const
{
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return WEIGHTED_PLATE_DELAY;
}

void WeightedPressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const
{
    // MC 1.16.5: 测重压力板（金属）点击音效
    if (!world.isClientSide()) {
        world.playSound(pressed ? SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_ON
                                : SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_OFF,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.3f,
            0.6f);
    }
}

i32 WeightedPressurePlateBlock::getEntityCount(IWorld& world, const BlockPos& pos) const
{
    // 创建压力板上方的碰撞箱
    AxisAlignedBB detectionBox(static_cast<f32>(pos.x) + 0.125f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f,
        static_cast<f32>(pos.z) + 0.875f);

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 计算实体数量
    i32 count = 0;
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            count++;
        }
    }

    return count;
}

} // namespace blocks
} // namespace mc
