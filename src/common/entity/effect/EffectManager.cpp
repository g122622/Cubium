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

#include "EffectManager.hpp"
#include "../core/LivingEntity.hpp"
#include "EffectType.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include <cstddef>
#include <utility>

namespace mc {
namespace entity {
namespace effect {

// ============================================================================
// EffectManager 实现
// ============================================================================

bool EffectManager::addEffect(EffectInstance effect, LivingEntity& entity)
{
    // 瞬间效果始终立即执行，不参与合并逻辑
    if (isInstantEffect(effect.type())) {
        // MC 原版行为: InstantenousMobEffect 在添加时立即触发一次，然后效果结束
        // 瞬间效果没有属性修改器，apply() 为空操作
        // applyInstantly() 直接调用 _applyEffect() 执行效果逻辑
        effect.apply(entity);
        effect.applyInstantly(entity);
        return true;
    }

    // 查找是否已存在相同类型的效果
    i32 index = _findEffectIndex(effect.type());

    if (index >= 0) {
        // 已存在，尝试合并
        EffectInstance& existing = m_effects[index];

        // 只有当新效果更强（amplifier更高）时才需要移除旧的属性修改器并重新应用
        // 如果新效果更弱或同级，merge() 不会修改 amplifier，无需重新应用
        bool needsReapply = effect.amplifier() > existing.amplifier();

        if (needsReapply) {
            // 新效果更强，先移除旧的属性修改器
            existing.remove(entity);
        }

        bool merged = existing.merge(effect);

        if (merged && needsReapply && !existing.isApplied()) {
            // merge() 在 amplifier 变化时将 m_applied 设为 false
            // 此时需要用新的 amplifier 重新应用属性修改器
            existing.apply(entity);
        }

        return merged;
    } else {
        // 新效果，添加并应用
        effect.apply(entity);
        m_effects.push_back(std::move(effect));
        return true;
    }
}

void EffectManager::removeEffect(EffectType type, LivingEntity& entity)
{
    i32 index = _findEffectIndex(type);
    if (index >= 0) {
        m_effects[index].remove(entity);
        m_effects.erase(m_effects.begin() + index);
    }
}

void EffectManager::removeAllEffects(LivingEntity& entity)
{
    for (auto& effect : m_effects) {
        effect.remove(entity);
    }
    m_effects.clear();
}

const EffectInstance* EffectManager::getEffect(EffectType type) const
{
    i32 index = _findEffectIndex(type);
    return index >= 0 ? &m_effects[index] : nullptr;
}

EffectInstance* EffectManager::getEffect(EffectType type)
{
    i32 index = _findEffectIndex(type);
    return index >= 0 ? &m_effects[index] : nullptr;
}

bool EffectManager::hasEffect(EffectType type) const
{
    return _findEffectIndex(type) >= 0;
}

i32 EffectManager::getEffectLevel(EffectType type) const
{
    const EffectInstance* effect = getEffect(type);
    return effect ? effect->getEffectLevel() : 0;
}

void EffectManager::tick(LivingEntity& entity)
{
    // 从后向前遍历，以便安全移除过期效果
    for (i32 i = static_cast<i32>(m_effects.size()) - 1; i >= 0; --i) {
        if (!m_effects[i].tick(entity)) {
            // 效果过期，移除
            m_effects.erase(m_effects.begin() + i);
        }
    }
}

bool EffectManager::hasBeneficialEffect() const
{
    for (const auto& effect : m_effects) {
        if (isBeneficialEffect(effect.type())) {
            return true;
        }
    }
    return false;
}

bool EffectManager::hasHarmfulEffect() const
{
    for (const auto& effect : m_effects) {
        if (!isBeneficialEffect(effect.type())) {
            return true;
        }
    }
    return false;
}

i32 EffectManager::_findEffectIndex(EffectType type) const
{
    for (size_t i = 0; i < m_effects.size(); ++i) {
        if (m_effects[i].type() == type) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

} // namespace effect
} // namespace entity
} // namespace mc
