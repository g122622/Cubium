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
#pragma once

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/core/Block.hpp"

#include <functional>

namespace mc {

class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 菌丝方块
 *
 * 可蔓延的菌丝方块，在光照充足时向周围泥土蔓延，
 * 在光照不足时退化成泥土。
 *
 * MC ID: minecraft:mycelium
 */
class MyceliumBlock : public SpreadableSnowyDirtBlock {
public:
    explicit MyceliumBlock(BlockProperties properties);

    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;
};

} // namespace blocks
} // namespace mc
