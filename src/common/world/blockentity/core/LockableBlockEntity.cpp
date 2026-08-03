/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/core/ItemStack.hpp"
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

bool LockableBlockEntity::canOpen(const Player* player, const ItemStack& heldItem) const
{
    // 未锁定的容器：所有人可打开
    if (!m_locked || m_lockKey.empty()) {
        return true;
    }

    // 创造模式玩家：可以打开任何容器
    if (player != nullptr && player->gameMode() == GameMode::Creative) {
        return true;
    }

    // 检查手持物品是否是正确的钥匙
    if (!heldItem.isEmpty() && heldItem.hasCustomName()) {
        if (heldItem.getCustomName() == m_lockKey) {
            return true;
        }
    }

    // 检查失败，不能打开
    return false;
}

void LockableBlockEntity::setCustomName(const std::string& name)
{
    if (m_customName != name) {
        m_customName = name;
        setChanged();
    }
}

std::string LockableBlockEntity::getDisplayName() const
{
    if (!m_customName.empty()) {
        return m_customName;
    }
    return getDefaultName();
}

bool LockableBlockEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载锁定状态
    if (data.contains("Lock") && data["Lock"].is_string()) {
        m_lockKey = data["Lock"].get<std::string>();
        m_locked = !m_lockKey.empty();
    }

    // 加载自定义名称
    if (data.contains("CustomName") && data["CustomName"].is_string()) {
        m_customName = data["CustomName"].get<std::string>();
    }

    return true;
}

void LockableBlockEntity::save(nlohmann::json& data) const
{
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
