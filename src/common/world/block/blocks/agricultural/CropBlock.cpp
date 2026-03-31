#include "CropBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"

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

BlockState CropBlock::withAge(int age) const {
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

    // TODO: 检查光照等级
    // if (world.getLightSubtracted(pos, 0) >= 9) {
    //     float growthChance = getGrowthChance(*this, world, pos);
    //     if (random.nextInt(static_cast<int>(25.0f / growthChance) + 1) == 0) {
    //         world.setBlockState(pos, withAge(getAge(state) + 1), 2);
    //     }
    // }

    // 简化实现：随机生长
    int age = getAge(state);
    if (age < getMaxAge()) {
        // 基础生长概率约为 1/25
        if (random.nextInt(25) == 0) {
            world.setBlockState(pos.x, pos.y, pos.z, &withAge(age + 1), 2);
        }
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

    // 农作物需要耕地
    // TODO: 检查是否为耕地方块
    // return groundState.is(Blocks::FARMLAND);

    // 简化实现：检查材料
    const Material& material = groundState.getMaterial();
    return material.isSolid();
}

float CropBlock::getGrowthChance(
    const Block& block,
    IBlockReader& world,
    const BlockPos& pos) {

    MC_UNUSED(block);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // TODO: 实现完整的生长速度计算
    // 包括：
    // 1. 周围耕地（湿润耕地提供更高速度）
    // 2. 相邻同种作物（降低速度）
    // 3. 行/列布局优化

    float growthChance = 1.0f;

    // 简化版本
    return growthChance;
}

} // namespace blocks
} // namespace mc
