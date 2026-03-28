#include "world/blockentity/BlockEntity.hpp"
#include "world/IWorld.hpp"

namespace mc {

const BlockState* BlockEntity::getBlockState() const {
    if (m_world == nullptr) {
        return nullptr;
    }
    return m_world->getBlockState(m_pos.x, m_pos.y, m_pos.z);
}

} // namespace mc
