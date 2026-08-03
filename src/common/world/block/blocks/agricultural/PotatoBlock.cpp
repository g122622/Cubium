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

#include "common/world/block/blocks/agricultural/PotatoBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"

namespace mc {
namespace blocks {

PotatoBlock::PotatoBlock(const BlockProperties& properties)
    : CropBlock(properties)
{

    // 预计算马铃薯各生长阶段的形状
    // 高度与胡萝卜相同：2, 3, 4, 5, 6, 7, 8, 9 像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};

    for (int i = 0; i < 8; ++i) {
        m_potatoShapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

u32 PotatoBlock::getCropItem() const
{
    // 马铃薯的作物和种子是同一个物品
    return Items::POTATO->itemId();
}

u32 PotatoBlock::getSeedItem() const
{
    // 马铃薯的作物和种子是同一个物品
    return Items::POTATO->itemId();
}

const CollisionShape& PotatoBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_potatoShapesByAge[age];
}

} // namespace blocks
} // namespace mc
