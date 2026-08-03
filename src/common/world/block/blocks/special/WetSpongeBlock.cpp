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

#include "WetSpongeBlock.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ========== WetSpongeBlock ==========

WetSpongeBlock::WetSpongeBlock(const BlockProperties& properties)
    : Block(properties)
{}

void WetSpongeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 在下界（超热维度）放置时变干
    if (world.isUltraWarm()) {
        // 变为干海绵
        const BlockState& spongeState = VanillaBlocks::SPONGE->defaultState();
        world.setBlockState(pos, &spongeState, 3);

        // 播放蒸汽效果（事件 2009）
        world.playEvent(world::WorldEvents::WET_SPONGE_DRY, pos, 0);

        // 播放火焰熄灭音效
        world.playSound(ResourceLocation("minecraft", "block.fire.extinguish"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);
    }
}

} // namespace blocks
} // namespace mc
