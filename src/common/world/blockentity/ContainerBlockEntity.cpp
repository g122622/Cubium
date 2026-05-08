#include "world/blockentity/ContainerBlockEntity.hpp"
#include "entity/entities/player/Player.hpp"

namespace mc {

void ContainerBlockEntity::openContainer(Player* player) {
    // MC 1.16.5: 观察者模式玩家不计入打开数
    if (player != nullptr && player->isSpectator()) {
        return;
    }
    // MC 1.16.5: 负数保护（防止数据损坏）
    if (m_openCount < 0) {
        m_openCount = 0;
    }
    ++m_openCount;
}

void ContainerBlockEntity::closeContainer(Player* player) {
    // MC 1.16.5: 观察者模式玩家不计入打开数
    if (player != nullptr && player->isSpectator()) {
        return;
    }
    if (m_openCount > 0) {
        --m_openCount;
    }
}

bool ContainerBlockEntity::isUsableByPlayer(const Player& player, f32 maxDistanceSq) const {
    // 参考 MC 1.16.5: net.minecraft.tileentity.LockableTileEntity.isUsableByPlayer
    // 检查：
    // 1. 方块实体仍然存在于世界中（m_world != nullptr 且未被移除）
    // 2. 玩家在指定距离范围内

    // 如果方块实体已被移除，返回 false
    if (isRemoved()) {
        return false;
    }

    // 计算玩家与方块中心的距离平方
    const BlockPos pos = getPos();
    return player.distanceSqTo(
               static_cast<f32>(pos.x) + 0.5f,
               static_cast<f32>(pos.y) + 0.5f,
               static_cast<f32>(pos.z) + 0.5f) <= maxDistanceSq;
}

} // namespace mc
