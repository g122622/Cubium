#include "SaplingBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

SaplingBlock::SaplingBlock(TreeGenerator treeGenerator, const BlockProperties& properties)
    : BushBlock(properties)
    , m_treeGenerator(std::move(treeGenerator)) {

    // 树苗形状：小型植物
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
}

// ========== 状态属性 ==========

i32 SaplingBlock::getStage(const BlockState& state) const {
    return state.get(BlockStateProperties::STAGE_0_1());
}

BlockState SaplingBlock::withStage(i32 stage) const {
    return defaultState().with(BlockStateProperties::STAGE_0_1(), std::min(stage, 1));
}

// ========== 放置逻辑 ==========

BlockState SaplingBlock::getStateForPlacement(BlockItemUseContext& context) {
    MC_UNUSED(context);
    return defaultState();
}

// ========== 生长逻辑 ==========

void SaplingBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 检查光照
    // TODO: 实现光照检查
    // if (world.getLightSubtracted(pos, 0) >= 9) {
    //     grow(world, pos, state);
    // }

    // 简化实现：随机生长
    if (random.nextFloat() < 0.1f) {
        grow(world, pos, state);
    }
}

bool SaplingBlock::grow(IWorld& world, const BlockPos& pos, BlockState& state) {
    i32 stage = getStage(state);

    if (stage < 1) {
        // 生长到下一阶段
        auto nextState = withStage(stage + 1);
        world.setBlockState(pos.x, pos.y, pos.z, &nextState, 2);
        return true;
    } else {
        // 已成熟，生成树
        if (m_treeGenerator) {
            // TODO: 使用世界随机数
            math::Random rng(static_cast<u64>(pos.x * 3129871) ^ static_cast<u64>(pos.z * 116129781) ^ static_cast<u64>(pos.y));
            m_treeGenerator(world, pos, rng);

            // 树生成后移除树苗
            world.setBlockState(pos.x, pos.y, pos.z, nullptr, 2);
            return true;
        }
    }

    return false;
}

// ========== 形状 ==========

const CollisionShape& SaplingBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

// ========== 保护方法 ==========

bool SaplingBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 树苗可以放置在草方块、泥土、耕地等上
    const Material& material = groundState.getMaterial();
    return material.isSolid();
}

} // namespace blocks
} // namespace mc
