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

#include "WoodButtonBlock.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/AbstractButtonBlock.hpp"

namespace mc {
namespace blocks {

// 木按钮按压持续时间（30 tick = 1.5秒）
static constexpr i32 WOOD_BUTTON_PRESS_TIME = 30;

WoodButtonBlock::WoodButtonBlock(const BlockProperties& properties)
    : AbstractButtonBlock(properties, WOOD_BUTTON_PRESS_TIME)
{}

void WoodButtonBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const
{
    world.playSound(pressed ? SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_ON : SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_OFF,
        sound::SoundCategory::Blocks,
        pos.center(),
        0.3f,
        0.6f);
}

} // namespace blocks
} // namespace mc
