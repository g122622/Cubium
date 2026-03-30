#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/ItemStack.hpp"

namespace mc {
namespace blockentity {

bool LockableBlockEntity::canOpen(const Player* player, const ItemStack& heldItem) const {
    // 未锁定的容器：所有人可打开
    if (!m_locked || m_lockKey.empty()) {
        return true;
    }

    // 创造模式玩家：可以打开任何容器
    // TODO: 实现创造模式检测
    // if (player && player->isCreative()) {
    //     return true;
    // }

    // 检查手持物品是否是正确的钥匙
    if (!heldItem.isEmpty() && heldItem.hasCustomName()) {
        if (heldItem.getCustomName() == m_lockKey) {
            return true;
        }
    }

    // 检查失败，不能打开
    return false;
}

void LockableBlockEntity::setCustomName(const String& name) {
    if (m_customName != name) {
        m_customName = name;
        setChanged();
    }
}

String LockableBlockEntity::getDisplayName() const {
    if (!m_customName.empty()) {
        return m_customName;
    }
    return getDefaultName();
}

bool LockableBlockEntity::load(const nlohmann::json& data) {
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载锁定状态
    if (data.contains("Lock") && data["Lock"].is_string()) {
        m_lockKey = data["Lock"].get<String>();
        m_locked = !m_lockKey.empty();
    }

    // 加载自定义名称
    if (data.contains("CustomName") && data["CustomName"].is_string()) {
        m_customName = data["CustomName"].get<String>();
    }

    return true;
}

void LockableBlockEntity::save(nlohmann::json& data) const {
    ContainerBlockEntity::save(data);

    // 保存锁定状态
    if (m_locked && !m_lockKey.empty()) {
        data["Lock"] = m_lockKey;
    }

    // 保存自定义名称
    if (!m_customName.empty()) {
        data["CustomName"] = m_customName;
    }
}

} // namespace blockentity
} // namespace mc
