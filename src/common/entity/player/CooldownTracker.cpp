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

#include "CooldownTracker.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include <algorithm>
#include <limits>
#include <utility>

namespace mc::entity::player {

void CooldownTracker::setListener(std::function<void(const ResourceLocation&, i32)> listener)
{
    m_listener = std::move(listener);
}

void CooldownTracker::tick()
{
    ++m_ticks;
    auto it = m_cooldowns.begin();
    while (it != m_cooldowns.end()) {
        if (it->second.expireTicks <= m_ticks) {
            const ResourceLocation group = it->first;
            it = m_cooldowns.erase(it);
            if (m_listener) {
                m_listener(group, 0);
            }
        } else {
            ++it;
        }
    }
}

void CooldownTracker::setCooldown(const Item* item, i32 ticks)
{
    if (item != nullptr) {
        setCooldown(item->itemLocation(), ticks);
    }
}

void CooldownTracker::setCooldown(const ResourceLocation& group, i32 ticks)
{
    // 无条件写入，不对 ticks 做正负判断：原版 addCooldown 同样是直接 put。
    // 曾经这里对 ticks <= 0 特判成 removeCooldown，会让「设了 0 tick 冷却」这种输入
    // 得到与原版不同的一组可观察状态（原版此后 isOnCooldown 仍为 true、进度为 NaN，
    // 因为分母 endTime - startTime 为 0）。生产调用不会传非正值，特判只是多一条分支。
    m_cooldowns.insert_or_assign(group, Cooldown(m_ticks, m_ticks + ticks));
    if (m_listener) {
        m_listener(group, ticks);
    }
}

void CooldownTracker::removeCooldown(const Item* item)
{
    if (item != nullptr) {
        removeCooldown(item->itemLocation());
    }
}

void CooldownTracker::removeCooldown(const ResourceLocation& group)
{
    m_cooldowns.erase(group);
    if (m_listener) {
        m_listener(group, 0);
    }
}

f32 CooldownTracker::getCooldownProgress(const Item* item, f32 partialTicks) const
{
    return item != nullptr ? getCooldownProgress(item->itemLocation(), partialTicks) : 0.0f;
}

f32 CooldownTracker::getCooldownProgress(const ResourceLocation& group, f32 partialTicks) const
{
    const auto it = m_cooldowns.find(group);
    if (it == m_cooldowns.end()) {
        return 0.0f;
    }
    const auto& cooldown = it->second;
    const i64 duration = cooldown.expireTicks - cooldown.createTicks;
    const f32 remaining = static_cast<f32>(cooldown.expireTicks - m_ticks) - partialTicks;
    const f32 ratio = remaining / static_cast<f32>(duration);
    // 用比较而不是 std::clamp：原版 Mth.clamp(NaN, 0, 1) 返回 NaN（`value < min ? min :
    // value > max ? max : value` 的两条比较对 NaN 都为 false，原值返回），而
    // std::clamp 在 -ffast-math 下对 NaN 的行为不保证。冷却时长为 0 时分母为 0，
    // 这个分支才会被走到。
    if (ratio < 0.0f) {
        return 0.0f;
    }
    if (ratio > 1.0f) {
        return 1.0f;
    }
    return ratio;
}

bool CooldownTracker::hasCooldown(const Item* item) const
{
    return item != nullptr && hasCooldown(item->itemLocation());
}

bool CooldownTracker::hasCooldown(const ResourceLocation& group) const
{
    // 与原版 isOnCooldown 一致：看的是进度是否为正，而不是剩余 tick 是否为正。
    // 两者在正常冷却（时长为正）上等价；差别只在时长为负这种非常规输入上——
    // 那时剩余 tick 为负但进度已被 clamp 到 1，原版视为仍在冷却。
    return getCooldownProgress(group, 0.0f) > 0.0f;
}

i32 CooldownTracker::getCooldownTicks(const Item* item) const
{
    return item != nullptr ? getCooldownTicks(item->itemLocation()) : 0;
}

i32 CooldownTracker::getCooldownTicks(const ResourceLocation& group) const
{
    const auto it = m_cooldowns.find(group);
    return it != m_cooldowns.end()
        ? static_cast<i32>(std::clamp<i64>(it->second.expireTicks - m_ticks, 0, std::numeric_limits<i32>::max()))
        : 0;
}

} // namespace mc::entity::player
