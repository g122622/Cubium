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
