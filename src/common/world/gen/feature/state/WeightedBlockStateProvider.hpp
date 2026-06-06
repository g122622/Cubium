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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature::state {

/**
 * @brief 加权方块状态条目
 */
struct WeightedStateEntry {
    const BlockState* state;
    i32 weight;

    WeightedStateEntry(const BlockState* s, i32 w)
        : state(s)
        , weight(w)
    {
    }
};

/**
 * @brief 加权方块状态提供者
 *
 * 根据权重随机选择方块状态。用于植被特征中随机选择植物类型。
 *
 * 参考: net.minecraft.world.level.levelgen.blockproviders.WeightedListBlockStateProvider
 */
class WeightedBlockStateProvider {
public:
    WeightedBlockStateProvider() = default;

    /**
     * @brief 添加加权方块状态
     * @param state 方块状态
     * @param weight 权重
     */
    void add(const BlockState* state, i32 weight)
    {
        m_entries.emplace_back(state, weight);
        m_totalWeight += weight;
    }

    /**
     * @brief 获取随机方块状态
     * @param rng 随机数生成器
     * @return 随机选择的方块状态
     */
    [[nodiscard]] const BlockState* getState(math::IRandom& rng) const
    {
        if (m_entries.empty()) {
            return nullptr;
        }
        i32 remaining = rng.nextInt(m_totalWeight);
        for (const auto& entry : m_entries) {
            remaining -= entry.weight;
            if (remaining < 0) {
                return entry.state;
            }
        }
        return m_entries.back().state;
    }

    /**
     * @brief 获取条目数量
     */
    [[nodiscard]] size_t size() const { return m_entries.size(); }

    /**
     * @brief 是否为空
     */
    [[nodiscard]] bool empty() const { return m_entries.empty(); }

private:
    std::vector<WeightedStateEntry> m_entries;
    i32 m_totalWeight = 0;
};

} // namespace mc::world::gen::feature::state
