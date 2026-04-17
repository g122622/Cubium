#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

FurnaceEntity::FurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Furnace, pos) {
}

std::unique_ptr<BlockEntity> FurnaceEntity::clone() const {
    auto cloned = std::make_unique<FurnaceEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "FurnaceEntity clone load failed");

    return cloned;

}

} // namespace blockentity
} // namespace mc
