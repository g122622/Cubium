#include "WeightedPressurePlateBlock.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// 测重压力板tick延迟
static constexpr i32 WEIGHTED_PLATE_DELAY = 10;

WeightedPressurePlateBlock::WeightedPressurePlateBlock(
    const BlockProperties& properties,
    Sensitivity sensitivity)
    : AbstractPressurePlateBlock(properties)
    , m_sensitivity(sensitivity) {
}

i32 WeightedPressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const {
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

i32 WeightedPressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const {
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return WEIGHTED_PLATE_DELAY;
}

void WeightedPressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(pressed);
    // TODO: 播放测重压力板音效
    // world.playSound(pos, pressed ? SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_ON
    //                              : SoundEvents::BLOCK_METAL_PRESSURE_PLATE_CLICK_OFF,
    //                 0.3f, 0.6f);
}

i32 WeightedPressurePlateBlock::getEntityCount(IWorld& world, const BlockPos& pos) const {
    // TODO: 实现实体计数
    // 需要检测压力板上方的所有实体
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return 0;
}

} // namespace blocks
} // namespace mc
