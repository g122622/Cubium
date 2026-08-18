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
 */

#include "CactusFlowerBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../Block.hpp"
#include "../../BlockTags.hpp"
#include "../../SupportType.hpp"
#include "../../registry/VanillaBlocks.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/blocks/vegetation/FlowerBlock.hpp"

namespace mc {
namespace blocks {

CactusFlowerBlock::CactusFlowerBlock(const BlockProperties& properties)
    : FlowerBlock(properties, 0, 0)
{
    // 仙人掌花形状：14像素宽、12像素高的柱形，比普通花朵（6x6）更大
    m_shape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.75f, 0.9375f);
}

bool CactusFlowerBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    // 仙人掌花可放置在仙人掌、耕地，或任意顶面提供中心支撑（isFaceSturdy(Up, Center)）的方块上。
    if (VanillaBlocks::CACTUS != nullptr && groundState.is(VanillaBlocks::CACTUS)) {
        return true;
    }
    if (VanillaBlocks::FARMLAND != nullptr && groundState.is(VanillaBlocks::FARMLAND)) {
        return true;
    }
    return groundState.isFaceSturdy(world, groundPos, Direction::Up, SupportType::Center);
}

} // namespace blocks
} // namespace mc
