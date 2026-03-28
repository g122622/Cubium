#include "FlowerBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../Material.hpp"

namespace mc {
namespace blocks {

// ========== FlowerBlock ==========

FlowerBlock::FlowerBlock(
    const BlockProperties& properties,
    u32 suspiciousStewEffect,
    i32 effectDuration)
    : BushBlock(properties)
    , m_suspiciousStewEffect(suspiciousStewEffect)
    , m_effectDuration(effectDuration) {

    // 花朵形状：小型，偏移中心
    m_shape = CollisionShape::box(0.3125f, 0.0f, 0.3125f, 0.6875f, 0.375f, 0.6875f);
}

const CollisionShape& FlowerBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

bool FlowerBlock::canSustain(
    const BlockState& groundState,
    IWorld& world,
    const BlockPos& groundPos) const {

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 花朵可以放置在草方块、泥土、耕地等上
    const Material& material = groundState.getMaterial();
    return material.isSolid();
}

// ========== LilacBlock ==========

LilacBlock::LilacBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties) {
}

// ========== RoseBushBlock ==========

RoseBushBlock::RoseBushBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties) {
}

// ========== PeonyBlock ==========

PeonyBlock::PeonyBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties) {
}

// ========== SunflowerBlock ==========

SunflowerBlock::SunflowerBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties) {
}

} // namespace blocks
} // namespace mc
