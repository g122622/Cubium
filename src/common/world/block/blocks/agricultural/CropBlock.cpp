#include "CropBlock.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"
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
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 检查光照等级
    // TODO: 实现光照检查
    // return (world.getLightSubtracted(pos, 0) >= 8 || world.canSeeSky(pos)) && belowState->is(Blocks::FARMLAND);

    return canSustain(*belowState, world, belowPos);
}

// ========== 生长逻辑 ==========

void CropBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 如果已经成熟，不需要生长
    if (isMaxAge(state)) {
        return;
    }

    const i32 blockLight = static_cast<i32>(world.getBlockLight(pos.x, pos.y + 1, pos.z));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(pos.x, pos.y + 1, pos.z));
    if (std::max(blockLight, skyLight) < 9) {
        return;
    }

    const f32 growthChance = std::max(1.0f, getGrowthChance(*this, static_cast<IBlockReader&>(world), pos));
    const i32 randomBound = std::max(1, static_cast<i32>(25.0f / growthChance) + 1);
    if (random.nextInt(randomBound) == 0) {
        world.setBlockState(pos.x, pos.y, pos.z, &withAge(getAge(state) + 1), 2);
    }
}

bool CropBlock::ticksRandomly() const {
    return true;  // 农作物总是需要随机 tick
}

void CropBlock::grow(IWorld& world, const BlockPos& pos, const BlockState& state) {
    int newAge = getAge(state) + getBonemealAgeIncrease();
    int maxAge = getMaxAge();

    if (newAge > maxAge) {
        newAge = maxAge;
    }

    world.setBlockState(pos.x, pos.y, pos.z, &withAge(newAge), 2);
}

int CropBlock::getBonemealAgeIncrease() const {
    // 骨粉增加 2-5 年龄
    // TODO: 使用世界随机数
    return 2 + (rand() % 4);  // NOLINT(runtime/int)
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

    float growthChance = 1.0f;

    const auto& moistureProp = BlockStateProperties::MOISTURE_0_7();
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            const BlockPos groundPos(pos.x + dx, pos.y - 1, pos.z + dz);
            const BlockState* groundState = world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
            if (groundState == nullptr || VanillaBlocks::FARMLAND == nullptr ||
                !groundState->is(VanillaBlocks::FARMLAND)) {
                continue;
            }

            f32 bonus = 1.0f;
            if (groundState->hasProperty(moistureProp) && groundState->get(moistureProp) > 0) {
                bonus = 3.0f;
            }

            if (dx != 0 || dz != 0) {
                bonus *= 0.25f;
            }

            growthChance += bonus;
        }
    }

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
