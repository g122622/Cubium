#include "world/blockentity/BlockEntity.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"

namespace mc {

const BlockState* BlockEntity::getBlockState() const
{
    if (m_world == nullptr) {
        return nullptr;
    }
    return m_world->getBlockState(m_pos);
}

void BlockEntity::setChanged()
{
    m_changed = true;
    // 子类如 ContainerBlockEntity 会在需要时更新红石比较器
    // 当前基类无需额外操作
}

} // namespace mc
