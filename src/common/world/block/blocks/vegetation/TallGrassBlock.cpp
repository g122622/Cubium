#include "TallGrassBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../Material.hpp"
#include "../../VanillaBlocks.hpp"

namespace {

[[nodiscard]] bool isVegetationGround(const mc::BlockState& groundState)
{
    return (mc::VanillaBlocks::GRASS_BLOCK != nullptr && groundState.is(mc::VanillaBlocks::GRASS_BLOCK)) ||
        (mc::VanillaBlocks::DIRT != nullptr && groundState.is(mc::VanillaBlocks::DIRT)) ||
        (mc::VanillaBlocks::COARSE_DIRT != nullptr && groundState.is(mc::VanillaBlocks::COARSE_DIRT)) ||
        (mc::VanillaBlocks::PODZOL != nullptr && groundState.is(mc::VanillaBlocks::PODZOL)) ||
        (mc::VanillaBlocks::FARMLAND != nullptr && groundState.is(mc::VanillaBlocks::FARMLAND));
}

} // namespace

namespace mc {
namespace blocks {

// ========== TallGrassBlock ==========

TallGrassBlock::TallGrassBlock(const BlockProperties& properties)
    : BushBlock(properties)
{
    // 高草形状：薄的一层
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

const CollisionShape& TallGrassBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool TallGrassBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    return isVegetationGround(groundState);
}

// ========== FernBlock ==========

FernBlock::FernBlock(const BlockProperties& properties)
    : TallGrassBlock(properties)
{
    // 蕨类使用与高草相同的形状
}

} // namespace blocks
} // namespace mc
