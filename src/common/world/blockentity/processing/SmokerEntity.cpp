#include "world/blockentity/processing/SmokerEntity.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

SmokerEntity::SmokerEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Smoker, pos) {
}

std::unique_ptr<BlockEntity> SmokerEntity::clone() const {
    auto cloned = std::make_unique<SmokerEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "SmokerEntity clone load failed");

    return cloned;
}

bool SmokerEntity::canSmelt(IWorld& world) const {
    return AbstractFurnaceEntity::canSmelt(world);
}

} // namespace blockentity
} // namespace mc