#include "StoneButtonBlock.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// 石头按钮按压持续时间（20 tick = 1秒）
// MC 1.16.5: AbstractButtonBlock.java 第53-55行
// func_235471_c_() 返回 this.wooden ? 30 : 20
static constexpr i32 STONE_BUTTON_PRESS_TIME = 20;

StoneButtonBlock::StoneButtonBlock(const BlockProperties& properties)
    : AbstractButtonBlock(properties, STONE_BUTTON_PRESS_TIME) {
}

void StoneButtonBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 播放石头按钮音效
    // world.playSound(pos, pressed ? SoundEvents::BLOCK_STONE_BUTTON_CLICK_ON
    //                              : SoundEvents::BLOCK_STONE_BUTTON_CLICK_OFF,
    //                 0.3f, 0.6f);
}

} // namespace blocks
} // namespace mc
