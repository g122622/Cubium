#include "WoodButtonBlock.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// 木按钮按压持续时间（15 tick = 1.5秒）
static constexpr i32 WOOD_BUTTON_PRESS_TIME = 15;

WoodButtonBlock::WoodButtonBlock(const BlockProperties& properties)
    : AbstractButtonBlock(properties, WOOD_BUTTON_PRESS_TIME) {
}

void WoodButtonBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 播放木按钮音效
    // world.playSound(pos, pressed ? SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_ON
    //                              : SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_OFF,
    //                 0.3f, 0.6f);
}

} // namespace blocks
} // namespace mc
