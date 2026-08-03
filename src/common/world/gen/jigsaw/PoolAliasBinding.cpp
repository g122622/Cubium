/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to the permitted persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "PoolAliasBinding.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

// ========== RandomPoolAliasBinding ==========

ResourceLocation RandomPoolAliasBinding::resolve(math::Random& rng) const
{
    if (m_targets.empty()) {
        return m_alias;
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& target : m_targets) {
        totalWeight += target.weight;
    }

    if (totalWeight <= 0) {
        return m_alias;
    }

    // 按权重随机选择
    i32 roll = rng.nextInt(totalWeight);
    i32 accumulated = 0;
    for (const auto& target : m_targets) {
        accumulated += target.weight;
        if (roll < accumulated) {
            return target.pool;
        }
    }

    // 兜底返回最后一个
    return m_targets.back().pool;
}

void RandomPoolAliasBinding::forEachResolved(math::Random& rng, const Resolver& callback) const
{
    // 对应 MC 1.21 RandomPoolAlias.forEachResolved：按权重随机选一个 target，输出 (alias, target)
    callback(m_alias, resolve(rng));
}

// ========== RandomGroupPoolAliasBinding ==========

std::unique_ptr<PoolAliasBinding> RandomGroupPoolAliasBinding::clone() const
{
    std::vector<AliasGroup> clonedGroups;
    clonedGroups.reserve(m_groups.size());
    for (const auto& group : m_groups) {
        AliasGroup clonedGroup;
        clonedGroup.weight = group.weight;
        for (const auto& binding : group.bindings) {
            clonedGroup.bindings.push_back(binding->clone());
        }
        clonedGroups.push_back(std::move(clonedGroup));
    }
    return std::make_unique<RandomGroupPoolAliasBinding>(m_alias, std::move(clonedGroups));
}

void RandomGroupPoolAliasBinding::forEachResolved(math::Random& rng, const Resolver& callback) const
{
    // 对应 MC 1.21 RandomGroupPoolAlias.forEachResolved：
    // 按组权重随机选一个组，然后解析组内所有绑定，每个绑定通过 callback 输出 (alias, target)
    if (m_groups.empty()) {
        return;
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& group : m_groups) {
        totalWeight += group.weight;
    }

    if (totalWeight <= 0) {
        return;
    }

    // 按权重选择一个组
    i32 roll = rng.nextInt(totalWeight);
    i32 accumulated = 0;
    const AliasGroup* selectedGroup = &m_groups.front();
    for (const auto& group : m_groups) {
        accumulated += group.weight;
        if (roll < accumulated) {
            selectedGroup = &group;
            break;
        }
    }

    // 解析组内所有绑定
    for (const auto& binding : selectedGroup->bindings) {
        binding->forEachResolved(rng, callback);
    }
}

// ========== PoolAliasBindings ==========

void PoolAliasBindings::addBinding(std::unique_ptr<PoolAliasBinding> binding)
{
    if (binding) {
        m_bindings.push_back(std::move(binding));
    }
}

void PoolAliasBindings::forEachResolved(math::Random& rng, const PoolAliasBinding::Resolver& callback) const
{
    for (const auto& binding : m_bindings) {
        binding->forEachResolved(rng, callback);
    }
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
