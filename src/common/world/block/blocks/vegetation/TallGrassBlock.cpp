#include "TallGrassBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../Material.hpp"

namespace mc {
namespace blocks {

// ========== TallGrassBlock ==========

TallGrassBlock::TallGrassBlock(const BlockProperties& properties)
    : BushBlock(properties) {
    // 高草形状：薄的一层
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

const CollisionShape& TallGrassBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

bool TallGrassBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 高草可以放置在草方块、泥土、耕地等上
    const Material& material = groundState.getMaterial();

    // 检查是否为植物可生长的材质
    // TODO: 使用更精确的方块检查（草方块、泥土、耕地等）
    return material.isSolid();
}

// ========== FernBlock ==========

FernBlock::FernBlock(const BlockProperties& properties)
    : TallGrassBlock(properties) {
    // 蕨类使用与高草相同的形状
}

} // namespace blocks
} // namespace mc
