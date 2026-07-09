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
    {}
};

/**
 * @brief 加权方块状态提供者
 *
 * 根据权重随机选择方块状态。用于植被特征中随机选择植物类型，
 * 也可用于树叶提供器（如杜鹃树混合杜鹃叶与开花杜鹃叶）。
 *
 * 内部使用线性扫描减权重法选择，适用于条目数较少（< 64）的场景。
 * 当 totalWeight >= 64 时，MC 原版切换到紧凑 selector，本实现暂未做此优化。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.stateproviders.WeightedStateProvider
 *       net.minecraft.util.random.WeightedList
 */
class WeightedBlockStateProvider {
public:
    WeightedBlockStateProvider() = default;

    /// 拷贝构造函数（深拷贝条目列表，权重与总权重一并复制）
    WeightedBlockStateProvider(const WeightedBlockStateProvider& other)
        : m_entries(other.m_entries)
        , m_totalWeight(other.m_totalWeight)
    {}

    /// 拷贝赋值运算符
    WeightedBlockStateProvider& operator=(const WeightedBlockStateProvider& other)
    {
        if (this != &other) {
            m_entries = other.m_entries;
            m_totalWeight = other.m_totalWeight;
        }
        return *this;
    }

    WeightedBlockStateProvider(WeightedBlockStateProvider&&) noexcept = default;
    WeightedBlockStateProvider& operator=(WeightedBlockStateProvider&&) noexcept = default;

    /**
     * @brief 深拷贝当前提供者
     * @return 新的 WeightedBlockStateProvider 实例
     */
    [[nodiscard]] std::unique_ptr<WeightedBlockStateProvider> clone() const
    {
        return std::make_unique<WeightedBlockStateProvider>(*this);
    }

    /**
     * @brief 添加加权方块状态
     * @param state 方块状态
     * @param weight 权重（必须 >= 0）
     */
    void add(const BlockState* state, i32 weight)
    {
        m_entries.emplace_back(state, weight);
        m_totalWeight += weight;
    }

    /**
     * @brief 获取随机方块状态
     * @param rng 随机数生成器
     * @return 随机选择的方块状态；若条目为空返回 nullptr
     */
    [[nodiscard]] const BlockState* getState(math::IRandom& rng) const
    {
        if (m_entries.empty() || m_totalWeight <= 0) {
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

    /**
     * @brief 获取总权重
     */
    [[nodiscard]] i32 totalWeight() const noexcept { return m_totalWeight; }

    /**
     * @brief 只读访问全部加权条目（用于解析期枚举，如把 weighted_state_provider
     *        的所有状态平铺到 flowers 列表）
     */
    [[nodiscard]] const std::vector<WeightedStateEntry>& entries() const noexcept { return m_entries; }

private:
    std::vector<WeightedStateEntry> m_entries;
    i32 m_totalWeight = 0;
};

} // namespace mc::world::gen::feature::state
