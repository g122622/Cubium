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

#include "PoolAliasBinding.hpp"

#include "../../../util/math/random/Random.hpp"

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

// ========== RandomGroupPoolAliasBinding ==========

ResourceLocation RandomGroupPoolAliasBinding::resolve(math::Random& rng) const
{
    // 先选一个组，再从组内解析
    auto resolved = resolveGroup(rng);
    if (resolved.empty()) {
        return m_alias;
    }
    return resolved.front().second;
}

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

std::vector<std::pair<ResourceLocation, ResourceLocation>> RandomGroupPoolAliasBinding::resolveGroup(
    math::Random& rng) const
{
    if (m_groups.empty()) {
        return {};
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& group : m_groups) {
        totalWeight += group.weight;
    }

    if (totalWeight <= 0) {
        return {};
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
    std::vector<std::pair<ResourceLocation, ResourceLocation>> result;
    result.reserve(selectedGroup->bindings.size());
    for (const auto& binding : selectedGroup->bindings) {
        ResourceLocation resolved = binding->resolve(rng);
        result.emplace_back(binding->alias(), std::move(resolved));
    }
    return result;
}

// ========== PoolAliasBindings ==========

void PoolAliasBindings::addBinding(std::unique_ptr<PoolAliasBinding> binding)
{
    if (binding) {
        m_bindings.push_back(std::move(binding));
    }
}

ResourceLocation PoolAliasBindings::resolve(const ResourceLocation& alias, math::Random& rng) const
{
    for (const auto& binding : m_bindings) {
        if (binding->alias() == alias) {
            return binding->resolve(rng);
        }
    }
    return alias;
}

std::vector<std::pair<ResourceLocation, ResourceLocation>> PoolAliasBindings::resolveAllGroups(math::Random& rng) const
{
    std::vector<std::pair<ResourceLocation, ResourceLocation>> result;
    for (const auto& binding : m_bindings) {
        if (auto* groupBinding = dynamic_cast<const RandomGroupPoolAliasBinding*>(binding.get())) {
            auto groupResult = groupBinding->resolveGroup(rng);
            for (auto& pair : groupResult) {
                result.push_back(std::move(pair));
            }
        } else {
            ResourceLocation resolved = binding->resolve(rng);
            result.emplace_back(binding->alias(), std::move(resolved));
        }
    }
    return result;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
