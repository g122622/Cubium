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

#include "WoodType.hpp"
#include "common/sound/SoundEvents.hpp"

namespace mc::WoodTypes {
const WoodType OAK{"oak",
    BlockSetTypes::OAK,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType SPRUCE{"spruce",
    BlockSetTypes::SPRUCE,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType BIRCH{"birch",
    BlockSetTypes::BIRCH,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType JUNGLE{"jungle",
    BlockSetTypes::JUNGLE,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType ACACIA{"acacia",
    BlockSetTypes::ACACIA,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType DARK_OAK{"dark_oak",
    BlockSetTypes::DARK_OAK,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType PALE_OAK{"pale_oak",
    BlockSetTypes::PALE_OAK,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType CRIMSON{"crimson",
    BlockSetTypes::CRIMSON,
    BlockSoundTypes::NETHER_WOOD,
    BlockSoundTypes::NETHER_WOOD_HANGING_SIGN,
    SoundEvents::BLOCK_NETHER_WOOD_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_NETHER_WOOD_FENCE_GATE_OPEN};
const WoodType WARPED{"warped",
    BlockSetTypes::WARPED,
    BlockSoundTypes::NETHER_WOOD,
    BlockSoundTypes::NETHER_WOOD_HANGING_SIGN,
    SoundEvents::BLOCK_NETHER_WOOD_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_NETHER_WOOD_FENCE_GATE_OPEN};
const WoodType MANGROVE{"mangrove",
    BlockSetTypes::MANGROVE,
    BlockSoundTypes::WOOD,
    BlockSoundTypes::HANGING_SIGN,
    SoundEvents::BLOCK_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_FENCE_GATE_OPEN};
const WoodType CHERRY{"cherry",
    BlockSetTypes::CHERRY,
    BlockSoundTypes::CHERRY_WOOD,
    BlockSoundTypes::CHERRY_WOOD_HANGING_SIGN,
    SoundEvents::BLOCK_CHERRY_WOOD_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_CHERRY_WOOD_FENCE_GATE_OPEN};
const WoodType BAMBOO{"bamboo",
    BlockSetTypes::BAMBOO,
    BlockSoundTypes::BAMBOO_WOOD,
    BlockSoundTypes::BAMBOO_WOOD_HANGING_SIGN,
    SoundEvents::BLOCK_BAMBOO_WOOD_FENCE_GATE_CLOSE,
    SoundEvents::BLOCK_BAMBOO_WOOD_FENCE_GATE_OPEN};
} // namespace mc::WoodTypes
