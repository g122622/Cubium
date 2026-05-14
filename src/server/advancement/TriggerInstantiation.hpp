#pragma once

/**
 * @file TriggerInstantiation.hpp
 * @brief 触发器模板方法实例化
 *
 * 此文件提供 CriterionListener::grantCriterion() 和
 * AbstractCriterionTrigger::trigger() 模板方法的实现。
 *
 * 这些方法需要 mc::server::PlayerAdvancements 的完整定义，
 * 因此在 common 模块的 trigger .cpp 文件中包含此文件。
 *
 * 参考 CriterionTrigger.hpp 末尾的注释。
 */

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/player/ServerPlayer.hpp"

namespace mc::advancement {

// ============================================================================
// CriterionListener::grantCriterion() 实现
// ============================================================================

template <typename T>
void CriterionListener<T>::grantCriterion(::mc::server::PlayerAdvancements& advancements) const
{
    advancements.grantCriterion(m_advancement, m_criterion);
}

// ============================================================================
// AbstractCriterionTrigger::trigger() 实现
// ============================================================================

template <typename T>
template <typename PredicateT>
void AbstractCriterionTrigger<T>::trigger(::mc::server::PlayerAdvancements& advancements, PredicateT&& predicate)
{
    auto it = m_listeners.find(&advancements);
    if (it == m_listeners.end()) {
        return;
    }

    // 复制监听器列表，因为 predicate 可能修改原列表
    auto listeners = it->second;
    for (const auto& listener : listeners) {
        if (predicate(listener.getInstance())) {
            listener.grantCriterion(advancements);
        }
    }
}

} // namespace mc::advancement
