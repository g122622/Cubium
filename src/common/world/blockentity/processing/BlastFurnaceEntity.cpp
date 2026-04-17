#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

BlastFurnaceEntity::BlastFurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::BlastFurnace, pos) {
}

std::unique_ptr<BlockEntity> BlastFurnaceEntity::clone() const {
    auto cloned = std::make_unique<BlastFurnaceEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "BlastFurnaceEntity clone load failed");

    return cloned;
}

bool BlastFurnaceEntity::canSmelt(IWorld& world) const {
    return AbstractFurnaceEntity::canSmelt(world);
}

} // namespace blockentity
} // namespace mc