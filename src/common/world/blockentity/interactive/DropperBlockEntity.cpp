#include "DropperBlockEntity.hpp"

namespace mc {
namespace blockentity {

DropperBlockEntity::DropperBlockEntity(const BlockPos& pos)
    : DispenserBlockEntity(BlockEntityType::Dropper, pos) {
}

std::unique_ptr<BlockEntity> DropperBlockEntity::clone() const {
    auto cloned = std::make_unique<DropperBlockEntity>(m_pos);
    // 复制库存内容
    for (i32 slot = 0; slot < INVENTORY_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(slot, stack.copy());
        }
    }
    // 复制锁定状态
    if (isLocked()) {
        cloned->setLocked(true);
        cloned->setLockKey(getLockKey());
    }
    return cloned;
}

} // namespace blockentity
} // namespace mc
