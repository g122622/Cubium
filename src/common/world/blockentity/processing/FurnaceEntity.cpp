#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

FurnaceEntity::FurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Furnace, pos)
{}

const ResourceLocation& FurnaceEntity::getFireCrackleSound() const
{
    return SoundEvents::BLOCK_FURNACE_FIRE_CRACKLE;
}

std::unique_ptr<BlockEntity> FurnaceEntity::clone() const
{
    auto cloned = std::make_unique<FurnaceEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "FurnaceEntity clone load failed");

    return cloned;
}

} // namespace blockentity
} // namespace mc
