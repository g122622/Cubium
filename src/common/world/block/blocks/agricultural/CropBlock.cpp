#include "CropBlock.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include <functional>
#include <algorithm>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

CropBlock::CropBlock(const BlockProperties& properties)
    : BushBlock(properties) {

    // 预计算各生长阶段的形状
    // 年龄0-7对应高度2/16到16/16
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f};

    for (int i = 0; i < 8; ++i) {
        m_shapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, heights[i] * P, 16.0f * P);
    }
}

// ========== 状态属性 ==========

const IntegerProperty& CropBlock::getAgeProperty() const {
    return BlockStateProperties::AGE_0_7();
}

int CropBlock::getAge(const BlockState& state) const {
    return state.get(getAgeProperty());
}

const BlockState& CropBlock::withAge(int age) const {
    return defaultState().with(getAgeProperty(), std::min(age, getMaxAge()));
}

bool CropBlock::isMaxAge(const BlockState& state) const {
    return getAge(state) >= getMaxAge();
}

// ========== 放置逻辑 ==========

BlockState CropBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState().with(getAgeProperty(), 0);
}

bool CropBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方是否为耕地
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr || !canSustain(*belowState, world, belowPos)) {
        return false;
    }

    // 参考 MC 1.16.5: CropsBlock.isValidPosition
    // 检查光照：getLightSubtracted(pos, 0) >= 8 或 canSeeSky(pos)
    // 由于 IBlockReader 没有 getLightSubtracted 方法，使用传统方式
    const i32 blockLight = static_cast<i32>(world.getBlockLight(pos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(pos));
    // 光照 >= 8 或能看见天空
    return std::max(blockLight, skyLight) >= 8;
}

// ========== 生长逻辑 ==========

void CropBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 参考 MC 1.16.5: CropsBlock.randomTick
    // 如果已经成熟，不需要生长
    if (isMaxAge(state)) {
        return;
    }

    // 参考 MC 1.16.5: 光照检查使用 getLightSubtracted(pos, 0)
    if (world.getLightSubtracted(pos, 0) < 9) {
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

bool CropBlock::canGrow(
    IBlockReader& world,
    const BlockPos& pos,
    const BlockState& state,
    bool isClient) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClient);

    // 只有未成熟时才能生长
    return !isMaxAge(state);
}

bool CropBlock::canUseBonemeal(
    IWorld& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState& state) const {

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);

    // 骨粉总是有效
    return true;
}

void CropBlock::grow(
    IWorld& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState& state) {

    MC_UNUSED(random);

    int newAge = getAge(state) + getBonemealAgeIncrease(world, pos);
    int maxAge = getMaxAge();

    if (newAge > maxAge) {
        newAge = maxAge;
    }

    world.setBlockState(pos, &withAge(newAge), 2);
}

void CropBlock::grow(IWorld& world, const BlockPos& pos, const BlockState& state) {
    math::Random random(world.seed());
    grow(world, random, pos, state);
}

int CropBlock::getBonemealAgeIncrease(IWorld& world, const BlockPos& pos) const {
    // 参考: net.minecraft.block.CropsBlock#getBonemealAgeIncrease
    // 使用世界种子和方块位置派生确定性随机数
    // 这确保同一位置多次使用骨粉结果一致
    const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos));
    math::Random random(seed);
    // 返回 2-5 的随机数（骨粉增加 2-5 个生长阶段）
    return 2 + random.nextInt(4);
}

// ========== 形状 ==========

const CollisionShape& CropBlock::getShape(const BlockState& state) const {
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 7);
    return m_shapesByAge[age];
}

// ========== 保护方法 ==========

bool CropBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 农作物只能种在耕地上
    return VanillaBlocks::FARMLAND != nullptr && groundState.is(VanillaBlocks::FARMLAND);
}

float CropBlock::getGrowthChance(
    const Block& block,
    IBlockReader& world,
    const BlockPos& pos) {

    // 参考: net.minecraft.block.CropsBlock#getGrowthChance
    float growthChance = 1.0f;

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

    const bool diagonalCrowded =
        isSameCrop(pos.x - 1, pos.z - 1) ||
        isSameCrop(pos.x + 1, pos.z - 1) ||
        isSameCrop(pos.x - 1, pos.z + 1) ||
        isSameCrop(pos.x + 1, pos.z + 1);

    if (axisCrowded || diagonalCrowded) {
        growthChance *= 0.5f;
    }

    return std::max(growthChance, 1.0f);
}

} // namespace blocks
} // namespace mc
