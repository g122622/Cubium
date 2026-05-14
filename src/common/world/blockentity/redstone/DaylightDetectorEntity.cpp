#include "DaylightDetectorEntity.hpp"

namespace mc {
namespace blockentity {

DaylightDetectorEntity::DaylightDetectorEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::DaylightDetector, pos)
{}

std::unique_ptr<BlockEntity> DaylightDetectorEntity::clone() const
{
    return std::make_unique<DaylightDetectorEntity>(m_pos);
}

} // namespace blockentity
} // namespace mc