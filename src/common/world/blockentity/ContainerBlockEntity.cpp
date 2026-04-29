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

} // namespace mc
