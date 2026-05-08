#include "CooldownTracker.hpp"
#include "../../item/core/Item.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>

namespace mc {
namespace entity::player {

void CooldownTracker::tick() {
    ++m_ticks;

    if (m_cooldowns.empty()) {
        return;
    }

    // 移除已过期的冷却
    auto it = m_cooldowns.begin();
    while (it != m_cooldowns.end()) {
        if (it->second.expireTicks <= m_ticks) {
            const Item* item = it->first;
            it = m_cooldowns.erase(it);
            notifyOnRemove(item);
        } else {
            ++it;
        }
    }
}

void CooldownTracker::setCooldown(const Item* item, i32 ticks) {
    if (item == nullptr || ticks <= 0) {
        return;
    }

    i32 createTicks = m_ticks;
    i32 expireTicks = m_ticks + ticks;

    // 如果已有冷却，先移除
    auto existing = m_cooldowns.find(item);
    if (existing != m_cooldowns.end()) {
        m_cooldowns.erase(existing);
    }

    m_cooldowns.emplace(item, Cooldown(createTicks, expireTicks));
    notifyOnSet(item, ticks);
}

void CooldownTracker::removeCooldown(const Item* item) {
    if (item == nullptr) {
        return;
    }

    auto it = m_cooldowns.find(item);
    if (it != m_cooldowns.end()) {
        m_cooldowns.erase(it);
        notifyOnRemove(item);
    }
}

f32 CooldownTracker::getCooldownProgress(const Item* item, f32 partialTicks) const {
    if (item == nullptr) {
        return 0.0f;
    }

    auto it = m_cooldowns.find(item);
    if (it == m_cooldowns.end()) {
        return 0.0f;
    }

    const Cooldown& cooldown = it->second;
    i32 totalDuration = cooldown.expireTicks - cooldown.createTicks;

    if (totalDuration <= 0) {
        return 0.0f;
    }

    // 剩余时间 = 过期时间 - (当前时间 + 插值)
    f32 remaining = static_cast<f32>(cooldown.expireTicks) -
                    (static_cast<f32>(m_ticks) + partialTicks);

    // 进度 = 剩余时间 / 总时长
    f32 progress = remaining / static_cast<f32>(totalDuration);

    // Clamp 到 [0, 1]
    return std::clamp(progress, 0.0f, 1.0f);
}

bool CooldownTracker::hasCooldown(const Item* item) const {
    if (item == nullptr) {
        return false;
    }

    auto it = m_cooldowns.find(item);
    if (it == m_cooldowns.end()) {
        return false;
    }

    // 检查是否已过期
    return it->second.expireTicks > m_ticks;
}

i32 CooldownTracker::getCooldownTicks(const Item* item) const {
    if (item == nullptr) {
        return 0;
    }

    auto it = m_cooldowns.find(item);
    if (it == m_cooldowns.end()) {
        return 0;
    }

    i32 remaining = it->second.expireTicks - m_ticks;
    return remaining > 0 ? remaining : 0;
}

void CooldownTracker::notifyOnSet(const Item* item, i32 ticks) {
    // 基类空实现，子类可重写以实现网络同步等
    (void)item;
    (void)ticks;
}

void CooldownTracker::notifyOnRemove(const Item* item) {
    // 基类空实现，子类可重写以实现网络同步等
    (void)item;
}

} // namespace entity::player
} // namespace mc
