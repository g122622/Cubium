/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the kind, express or implied, including but
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "TorchflowerCropBlock.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

TorchflowerCropBlock::TorchflowerCropBlock(const BlockProperties& properties)
    : CropBlock(properties, BlockStateProperties::AGE_0_1())
{
    // 预计算火把花各生长阶段的形状
    // MC Java: SHAPES = Block.boxes(1, p_394173_ -> Block.column(6.0, 0.0, 6 + p_394173_ * 4))
    // age=0: column(radius=6, yMin=0, yMax=6) -> 盒子 (5, 0, 5, 11, 6/16, 11)
    // age=1: column(radius=6, yMin=0, yMax=10) -> 盒子 (5, 0, 5, 11, 10/16, 11)
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 radius = 6.0f * P;
    constexpr f32 minXZ = 8.0f * P - radius; // 0.5 - 6/16 = 0.125
    constexpr f32 maxXZ = 8.0f * P + radius; // 0.5 + 6/16 = 0.875

    m_torchflowerShapesByAge[0] = CollisionShape::box(minXZ, 0.0f, minXZ, maxXZ, 6.0f * P, maxXZ);
    m_torchflowerShapesByAge[1] = CollisionShape::box(minXZ, 0.0f, minXZ, maxXZ, 10.0f * P, maxXZ);
}

// ========== 状态属性 ==========

const IntegerProperty& TorchflowerCropBlock::getAgeProperty() const
{
    return BlockStateProperties::AGE_0_1();
}

const BlockState& TorchflowerCropBlock::withAge(i32 age) const
{
    if (age >= getMaxAge()) {
        // age>=2: 替换为火把花方块状态
        return VanillaBlocks::TORCHFLOWER->defaultState();
    }
    // age=0 或 age=1: 返回自身的作物状态
    return defaultState().with(getAgeProperty(), age);
}

// ========== 生长逻辑 ==========

void TorchflowerCropBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 如果已经成熟，不需要生长
    if (isMaxAge(state)) {
        return;
    }

    // 火把花有 1/3 概率跳过生长检查
    if (random.nextInt(3) == 0) {
        return;
    }

    // 光照检查
    if (world.getLightSubtracted(pos, 0) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    // 计算生长概率
    const f32 growthChance = std::max(1.0f, getGrowthChance(*this, static_cast<IBlockReader&>(world), pos));
    const i32 randomBound = static_cast<i32>(25.0f / growthChance) + 1;
    if (random.nextInt(randomBound) == 0) {
        const i32 newAge = getAge(state) + 1;
        world.setBlockState(pos, &withAge(newAge), 2);
    }
}

i32 TorchflowerCropBlock::getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 火把花骨粉每次只增加 1 个生长阶段
    return 1;
}

// ========== 骨粉生长 ==========

void TorchflowerCropBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);

    const i32 newAge = getAge(state) + getBonemealAgeIncrease(world, pos);
    world.setBlockState(pos, &withAge(newAge), 2);
}

// ========== 形状 ==========

const CollisionShape& TorchflowerCropBlock::getShape(const BlockState& state) const
{
    const i32 age = getAge(state);
    MC_ASSERT_RELEASE(age >= 0 && age <= 1);
    return m_torchflowerShapesByAge[age];
}

// ========== 掉落物 ==========

u32 TorchflowerCropBlock::getCropItem() const
{
    // 成熟时掉落火把花物品
    return Items::TORCHFLOWER->itemId();
}

u32 TorchflowerCropBlock::getSeedItem() const
{
    // 种子为火把花种子
    return Items::TORCHFLOWER_SEEDS->itemId();
}

} // namespace blocks
} // namespace mc
