#include "StonePressurePlateBlock.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// 石头压力板tick延迟
static constexpr i32 STONE_PLATE_DELAY = 10;

StonePressurePlateBlock::StonePressurePlateBlock(const BlockProperties& properties)
    : AbstractPressurePlateBlock(properties) {
}

i32 StonePressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const {
    // TODO: 实现实体检测
    // 石头压力板只能被生物触发，物品不会触发
    // 有生物时输出15，无生物时输出0
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return 0;
}

i32 StonePressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const {
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return STONE_PLATE_DELAY;
}

void StonePressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(pressed);
    // TODO: 播放石头压力板音效
    // world.playSound(pos, pressed ? SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_ON
    //                              : SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_OFF,
    //                 0.3f, 0.6f);
}

} // namespace blocks
} // namespace mc
