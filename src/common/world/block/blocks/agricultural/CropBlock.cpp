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

#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

CropBlock::CropBlock(const BlockProperties& properties, const IntegerProperty& ageProperty)
    : BushBlock(properties)
    , m_ageProperty(ageProperty)
{
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充。
    // 原因：createBlockState 会为每个 BlockState 调用 _cacheProperties，其中
    // getOpacity→propagatesSkylightDown→getOcclusionShape→getShape 会在 BlockState 构造期
    // 回调 getShape。std::array::operator[] 虽不抛异常，但构造期取到的是默认构造的空
    // CollisionShape，使光照判定依赖"空 shape 恰好非 full block"的脆弱巧合。先填 shape
    // 再 createBlockState，保证构造期 getShape 返回真实形状。对齐 PointedDripstoneBlock /
    // AmethystClusterBlock 的正确构造顺序。

    // 预计算各生长阶段的形状
    // 年龄0-7对应高度2/16到16/16
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f};

    for (i32 i = 0; i < 8; ++i) {
        m_shapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }

    // 创建状态容器，注册年龄属性
    // 注意：不能在构造函数中调用虚方法 getAgeProperty()，因为 C++ 基类构造期间
    // 虚分派会解析到基类而非派生类。因此通过构造函数参数传入年龄属性。
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(m_ageProperty)
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态为 age=0
    setDefaultState(defaultState().with(m_ageProperty, 0));
}

// ========== 状态属性 ==========

const IntegerProperty& CropBlock::getAgeProperty() const
{
    return m_ageProperty;
}

i32 CropBlock::getAge(const BlockState& state) const
{
    return state.get(getAgeProperty());
}

const BlockState& CropBlock::withAge(i32 age) const
{
    return defaultState().with(getAgeProperty(), std::min(age, getMaxAge()));
}

bool CropBlock::isMaxAge(const BlockState& state) const
{
    return getAge(state) >= getMaxAge();
}

// ========== 放置逻辑 ==========

BlockState CropBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(getAgeProperty(), 0);
}

bool CropBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方是否为耕地
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr || !canSustain(*belowState, world, belowPos)) {
        return false;
    }

    // 检查光照：getLightSubtracted(pos, 0) >= 8 或 canSeeSky(pos)
    // 由于 IBlockReader 没有 getLightSubtracted 方法，使用传统方式
    const i32 blockLight = static_cast<i32>(world.getBlockLight(pos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(pos));
    // 光照 >= CROP_SURVIVAL_LIGHT_THRESHOLD 或能看见天空
    return std::max(blockLight, skyLight) >= game::CROP_SURVIVAL_LIGHT_THRESHOLD;
}

// ========== 生长逻辑 ==========

void CropBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 如果已经成熟，不需要生长
    if (isMaxAge(state)) {
        return;
    }

    // 光照检查：作物生长需要足够光照
    if (world.getLightSubtracted(pos, 0) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    // 计算生长概率
    const f32 growthChance = std::max(1.0f, getGrowthChance(*this, static_cast<IBlockReader&>(world), pos));
    const i32 randomBound = static_cast<i32>(25.0f / growthChance) + 1;
    if (random.nextInt(randomBound) == 0) {
        world.setBlockState(pos, &withAge(getAge(state) + 1), 2);
    }
}

// ========== IGrowable 接口实现 ==========

bool CropBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);

    // 只有未成熟时才能生长
    return !isMaxAge(state);
}

bool CropBlock::canUseBonemeal(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);

    // 骨粉总是有效
    return true;
}

void CropBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{

    MC_UNUSED(random);

    i32 newAge = getAge(state) + getBonemealAgeIncrease(world, pos);
    i32 maxAge = getMaxAge();

    if (newAge > maxAge) {
        newAge = maxAge;
    }

    world.setBlockState(pos, &withAge(newAge), 2);
}

void CropBlock::grow(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    math::Random random(world.seed());
    grow(world, random, pos, state);
}

i32 CropBlock::getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const
{
    // 使用世界种子和方块位置派生确定性随机数
    // 这确保同一位置多次使用骨粉结果一致
    const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos));
    math::Random random(seed);
    // 返回 2-5 的随机数（骨粉增加 2-5 个生长阶段）
    return 2 + random.nextInt(4);
}

// ========== 形状 ==========

const CollisionShape& CropBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    MC_ASSERT_RELEASE(age >= 0 && age <= 7);
    return m_shapesByAge[age];
}

// ========== 保护方法 ==========

bool CropBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    // 委托给下方方块的 canSustainPlant 方法
    // CropBlock::getPlantType() 返回 PlantType::Crop，
    // Block::canSustainPlant 的 Crop 分支只接受 Farmland
    const Block& groundBlock = groundState.getBlock();
    return groundBlock.canSustainPlant(groundState, static_cast<IBlockReader&>(world), groundPos, Direction::Up, *this);
}

PlantType CropBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Crop;
}

f32 CropBlock::getGrowthChance(const Block& block, IBlockReader& world, const BlockPos& pos)
{
    f32 growthChance = 1.0f;

    const auto& moistureProp = BlockStateProperties::MOISTURE_0_7();
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            const BlockPos groundPos(pos.x + dx, pos.y - 1, pos.z + dz);
            const BlockState* groundState = world.getBlockState(groundPos);
            if (groundState == nullptr || VanillaBlocks::FARMLAND == nullptr ||
                !groundState->is(VanillaBlocks::FARMLAND)) {
                continue;
            }

            // 湿润耕地增加 3 倍生长速度
            f32 bonus = 1.0f;
            if (groundState->hasProperty(moistureProp) && groundState->get(moistureProp) > 0) {
                bonus = 3.0f;
            }

            // 周围耕地减半贡献
            if (dx != 0 || dz != 0) {
                bonus *= 0.25f;
            }

            growthChance += bonus;
        }
    }

    // 检查周围是否有同类作物（降低生长速度）
    const auto isSameCrop = [&](i32 x, i32 z) {
        const BlockState* check = world.getBlockState(x, pos.y, z);
        return check != nullptr && check->is(&block);
    };

    const bool north = isSameCrop(pos.x, pos.z - 1);
    const bool south = isSameCrop(pos.x, pos.z + 1);
    const bool west = isSameCrop(pos.x - 1, pos.z);
    const bool east = isSameCrop(pos.x + 1, pos.z);
    const bool axisCrowded = (north || south) && (west || east);

    const bool diagonalCrowded = isSameCrop(pos.x - 1, pos.z - 1) || isSameCrop(pos.x + 1, pos.z - 1) ||
        isSameCrop(pos.x - 1, pos.z + 1) || isSameCrop(pos.x + 1, pos.z + 1);

    if (axisCrowded || diagonalCrowded) {
        growthChance *= 0.5f;
    }

    return std::max(growthChance, 1.0f);
}

} // namespace blocks
} // namespace mc
