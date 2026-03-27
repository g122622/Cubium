#include "world/blockentity/processing/FurnaceEntity.hpp"

namespace mc {
namespace blockentity {

FurnaceEntity::FurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Furnace, pos) {
}

std::unique_ptr<BlockEntity> FurnaceEntity::clone() const {
    auto cloned = std::make_unique<FurnaceEntity>(m_pos);
    // TODO: 复制熔炉状态（燃烧时间、熔炼进度等）
    return cloned;
}

} // namespace blockentity
} // namespace mc
