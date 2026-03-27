#include "WoodPressurePlateBlock.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// 木压力板tick延迟
static constexpr i32 WOOD_PLATE_DELAY = 10;

WoodPressurePlateBlock::WoodPressurePlateBlock(const BlockProperties& properties)
    : AbstractPressurePlateBlock(properties) {
}

i32 WoodPressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const {
    // TODO: 实现实体检测
    // 木压力板可以被所有实体触发（玩家、怪物、物品等）
    // 有实体时输出15，无实体时输出0
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return 0;
}

i32 WoodPressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const {
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return WOOD_PLATE_DELAY;
}

void WoodPressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(pressed);
    // TODO: 播放木压力板音效
    // world.playSound(pos, pressed ? SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_ON
    //                              : SoundEvents::BLOCK_WOODEN_PRESSURE_PLATE_CLICK_OFF,
    //                 0.3f, 0.6f);
}

} // namespace blocks
} // namespace mc
