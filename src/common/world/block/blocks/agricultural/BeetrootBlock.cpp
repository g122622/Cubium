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

#include "BeetrootBlock.hpp"
#include "../../../../core/Constants.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include <algorithm>

namespace mc {
namespace blocks {

BeetrootBlock::BeetrootBlock(const BlockProperties& properties)
    : CropBlock(properties, BlockStateProperties::AGE_0_3())
{

    // 预计算甜菜根各生长阶段的形状
    // 只有 4 个阶段，高度：2, 4, 6, 8 像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 4.0f, 6.0f, 8.0f};

    for (int i = 0; i < 4; ++i) {
        m_beetrootShapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

const IntegerProperty& BeetrootBlock::getAgeProperty() const
{
    // 甜菜根使用 AGE_0_3 属性
    return BlockStateProperties::AGE_0_3();
}

void BeetrootBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 甜菜根有 1/3 概率跳过生长检查

    // 如果已经成熟，不需要生长
    if (isMaxAge(state)) {
        return;
    }

    // 甜菜根有 1/3 概率跳过
    if (random.nextInt(3) == 0) {
        return;
    }

    // 光照检查
    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    // 计算生长概率
    const f32 growthChance = std::max(1.0f, getGrowthChance(*this, static_cast<IBlockReader&>(world), pos));
    const i32 randomBound = static_cast<i32>(25.0f / growthChance) + 1;
    if (random.nextInt(randomBound) == 0) {
        world.setBlockState(pos, &withAge(getAge(state) + 1), 2);
    }
}

i32 BeetrootBlock::getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const
{
    // 甜菜根骨粉增加的生长阶段较少
    // 返回父类的 1/3（约 0-1，因为父类返回 2-5）
    // 实际上，甜菜根骨粉只增加 1 个阶段
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return 1;
}

const CollisionShape& BeetrootBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_beetrootShapesByAge[age];
}

u32 BeetrootBlock::getCropItem() const
{
    // 返回甜菜根物品ID
    return Items::BEETROOT->itemId();
}

u32 BeetrootBlock::getSeedItem() const
{
    // 返回甜菜根种子物品ID
    return Items::BEETROOT_SEEDS->itemId();
}

} // namespace blocks
} // namespace mc
