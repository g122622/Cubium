#include "IInventory.hpp"
#include "../entities/player/Player.hpp"

namespace mc {

// ============================================================================
// IInventory 默认实现
// ============================================================================

bool IInventory::isUsableByPlayer(const Player& player) const
{
    // 默认实现：始终返回 true
    // 子类应重写此方法以检查距离
    (void)player;
    return true;
}

void IInventory::openInventory(Player& player)
{
    // 默认实现：空操作
    // 子类可以重写以实现打开计数、音效等功能
    (void)player;
}

void IInventory::closeInventory(Player& player)
{
    // 默认实现：空操作
    // 子类可以重写以实现关闭计数、物品返还等功能
    (void)player;
}

} // namespace mc
