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

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::valueprovider {

/**
 * @brief 整数值提供器（MC 1.21 IntProvider）
 *
 * 提供可控随机性的整数采样。参考 MC 1.21.11:
 * net.minecraft.util.valueproviders.IntProvider
 *
 * 使用方法：
 * @code
 * auto provider = UniformInt::create(0, 16);
 * i32 value = provider->sample(rng);
 * i32 min = provider->getMinValue();
 * i32 max = provider->getMaxValue();
 * @endcode
 */
class IntProvider {
public:
    virtual ~IntProvider() = default;

    /**
     * @brief 采样一个整数值
     * @param rng 随机数生成器
     * @return 采样结果
     */
    [[nodiscard]] virtual i32 sample(math::IRandom& rng) const = 0;

    /**
     * @brief 获取最小可能值
     */
    [[nodiscard]] virtual i32 getMinValue() const = 0;

    /**
     * @brief 获取最大可能值
     */
    [[nodiscard]] virtual i32 getMaxValue() const = 0;

    /**
     * @brief 获取提供器类型名称（用于调试/序列化）
     */
    [[nodiscard]] virtual const char* getTypeName() const = 0;

    /**
     * @brief 深拷贝当前 IntProvider 对象
     * @return 新的 IntProvider 实例，与当前对象完全独立
     */
    [[nodiscard]] virtual std::unique_ptr<IntProvider> clone() const = 0;
};

// ============================================================================
// ConstantInt — 固定值提供器
// ============================================================================

/**
 * @brief 固定整数值提供器（MC 1.21 ConstantInt）
 *
 * 始终返回相同的值。用于不需要随机性的场景。
 */
class ConstantInt final : public IntProvider {
public:
    static std::unique_ptr<ConstantInt> create(i32 value) { return std::make_unique<ConstantInt>(value); }

    explicit ConstantInt(i32 value)
        : m_value(value)
    {}

    [[nodiscard]] i32 sample(math::IRandom& /*rng*/) const override { return m_value; }
    [[nodiscard]] i32 getMinValue() const override { return m_value; }
    [[nodiscard]] i32 getMaxValue() const override { return m_value; }
    [[nodiscard]] const char* getTypeName() const override { return "constant"; }
    [[nodiscard]] std::unique_ptr<IntProvider> clone() const override { return std::make_unique<ConstantInt>(m_value); }

    [[nodiscard]] i32 getValue() const { return m_value; }

private:
    i32 m_value;
};

// ============================================================================
// UniformInt — 均匀分布整数值提供器
// ============================================================================

/**
 * @brief 均匀分布整数值提供器（MC 1.21 UniformInt）
 *
 * 在 [minInclusive, maxInclusive] 范围内均匀采样。
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.UniformInt
 */
class UniformInt final : public IntProvider {
public:
    static std::unique_ptr<UniformInt> create(i32 minInclusive, i32 maxInclusive)
    {
        return std::make_unique<UniformInt>(minInclusive, maxInclusive);
    }

    UniformInt(i32 minInclusive, i32 maxInclusive)
        : m_min(minInclusive)
        , m_max(maxInclusive)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng) const override
    {
        if (m_min >= m_max) {
            return m_min;
        }
        return m_min + rng.nextInt(m_max - m_min + 1);
    }

    [[nodiscard]] i32 getMinValue() const override { return m_min; }
    [[nodiscard]] i32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "uniform"; }
    [[nodiscard]] std::unique_ptr<IntProvider> clone() const override
    {
        return std::make_unique<UniformInt>(m_min, m_max);
    }

private:
    i32 m_min;
    i32 m_max;
};

// ============================================================================
// BiasedToBottomInt — 偏向底部的整数值提供器
// ============================================================================

/**
 * @brief 偏向底部的整数值提供器（MC 1.21 BiasedToBottomInt）
 *
 * 在 [minInclusive, maxInclusive] 范围内采样，偏向较低值。
 * 算法：两次随机取最小值，使低值概率更高。
 *
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.BiasedToBottomInt
 */
class BiasedToBottomInt final : public IntProvider {
public:
    static std::unique_ptr<BiasedToBottomInt> create(i32 minInclusive, i32 maxInclusive)
    {
        return std::make_unique<BiasedToBottomInt>(minInclusive, maxInclusive);
    }

    BiasedToBottomInt(i32 minInclusive, i32 maxInclusive)
        : m_min(minInclusive)
        , m_max(maxInclusive)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng) const override
    {
        // MC: min + rng.nextInt(rng.nextInt(max - min + 1) + 1)
        if (m_min >= m_max) {
            return m_min;
        }
        const i32 range = m_max - m_min + 1;
        return m_min + rng.nextInt(rng.nextInt(range) + 1);
    }

    [[nodiscard]] i32 getMinValue() const override { return m_min; }
    [[nodiscard]] i32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "biased_to_bottom"; }
    [[nodiscard]] std::unique_ptr<IntProvider> clone() const override
    {
        return std::make_unique<BiasedToBottomInt>(m_min, m_max);
    }

private:
    i32 m_min;
    i32 m_max;
};

// ============================================================================
// ClampedInt — 钳位整数值提供器
// ============================================================================

/**
 * @brief 钳位整数值提供器（MC 1.21 ClampedInt）
 *
 * 将另一个 IntProvider 的采样结果钳位到 [minInclusive, maxInclusive] 范围。
 *
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.ClampedInt
 */
class ClampedInt final : public IntProvider {
public:
    static std::unique_ptr<ClampedInt> create(std::unique_ptr<IntProvider> source, i32 minInclusive, i32 maxInclusive)
    {
        return std::make_unique<ClampedInt>(std::move(source), minInclusive, maxInclusive);
    }

    ClampedInt(std::unique_ptr<IntProvider> source, i32 minInclusive, i32 maxInclusive)
        : m_source(std::move(source))
        , m_min(minInclusive)
        , m_max(maxInclusive)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng) const override
    {
        return std::clamp(m_source->sample(rng), m_min, m_max);
    }

    [[nodiscard]] i32 getMinValue() const override { return m_min; }
    [[nodiscard]] i32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "clamped"; }
    [[nodiscard]] std::unique_ptr<IntProvider> clone() const override
    {
        return std::make_unique<ClampedInt>(m_source ? m_source->clone() : nullptr, m_min, m_max);
    }

private:
    std::unique_ptr<IntProvider> m_source;
    i32 m_min;
    i32 m_max;
};

// ============================================================================
// ClampedNormalInt — 正态分布钳位整数值提供器
// ============================================================================

/**
 * @brief 正态分布钳位整数值提供器（MC 1.21 ClampedNormalInt）
 *
 * 以 mean 为均值、deviation 为标准差生成正态分布随机数，
 * 然后钳位到 [minInclusive, maxInclusive] 范围。
 *
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.ClampedNormalInt
 */
class ClampedNormalInt final : public IntProvider {
public:
    static std::unique_ptr<ClampedNormalInt> create(f64 mean, f64 deviation, i32 minInclusive, i32 maxInclusive)
    {
        return std::make_unique<ClampedNormalInt>(mean, deviation, minInclusive, maxInclusive);
    }

    ClampedNormalInt(f64 mean, f64 deviation, i32 minInclusive, i32 maxInclusive)
        : m_mean(mean)
        , m_deviation(deviation)
        , m_min(minInclusive)
        , m_max(maxInclusive)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng) const override
    {
        // MC: Mth.clamp(Mth.floor(mean + rng.nextGaussian() * deviation), min, max)
        const f64 gaussian = static_cast<f64>(rng.nextGaussian(static_cast<f32>(0.0f), static_cast<f32>(1.0f)));
        const i32 value = static_cast<i32>(std::floor(m_mean + gaussian * m_deviation));
        return std::clamp(value, m_min, m_max);
    }

    [[nodiscard]] i32 getMinValue() const override { return m_min; }
    [[nodiscard]] i32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "clamped_normal"; }

    [[nodiscard]] f64 getMean() const { return m_mean; }
    [[nodiscard]] f64 getDeviation() const { return m_deviation; }

    [[nodiscard]] std::unique_ptr<IntProvider> clone() const override
    {
        return std::make_unique<ClampedNormalInt>(m_mean, m_deviation, m_min, m_max);
    }

private:
    f64 m_mean;
    f64 m_deviation;
    i32 m_min;
    i32 m_max;
};

// ============================================================================
// WeightedListInt — 加权列表整数值提供器
// ============================================================================

/**
 * @brief 加权列表整数值提供器（MC 1.21 WeightedListInt）
 *
 * 从带权重的 IntProvider 列表中选择一个，然后采样。
 *
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.WeightedListInt
 */
class WeightedListInt final : public IntProvider {
public:
    struct WeightedEntry {
        std::unique_ptr<IntProvider> provider;
        i32 weight;
    };

    static std::unique_ptr<WeightedListInt> create(std::vector<WeightedEntry> entries)
    {
        return std::make_unique<WeightedListInt>(std::move(entries));
    }

    explicit WeightedListInt(std::vector<WeightedEntry> entries)
        : m_entries(std::move(entries))
    {
        m_totalWeight = 0;
        for (const auto& entry : m_entries) {
            m_totalWeight += entry.weight;
        }
    }

    [[nodiscard]] i32 sample(math::IRandom& rng) const override
    {
        if (m_entries.empty()) {
            return 0;
        }
        i32 remaining = rng.nextInt(m_totalWeight);
        for (const auto& entry : m_entries) {
            remaining -= entry.weight;
            if (remaining < 0) {
                return entry.provider->sample(rng);
            }
        }
        return m_entries.back().provider->sample(rng);
    }

    [[nodiscard]] i32 getMinValue() const override
    {
        if (m_entries.empty()) {
            return 0;
        }
        i32 minVal = m_entries[0].provider->getMinValue();
        for (size_t i = 1; i < m_entries.size(); ++i) {
            minVal = std::min(minVal, m_entries[i].provider->getMinValue());
        }
        return minVal;
    }

    [[nodiscard]] i32 getMaxValue() const override
    {
        if (m_entries.empty()) {
            return 0;
        }
        i32 maxVal = m_entries[0].provider->getMaxValue();
        for (size_t i = 1; i < m_entries.size(); ++i) {
            maxVal = std::max(maxVal, m_entries[i].provider->getMaxValue());
        }
        return maxVal;
    }

    [[nodiscard]] const char* getTypeName() const override { return "weighted_list"; }

    [[nodiscard]] std::unique_ptr<IntProvider> clone() const override
    {
        std::vector<WeightedEntry> clonedEntries;
        clonedEntries.reserve(m_entries.size());
        for (const auto& entry : m_entries) {
            WeightedEntry clonedEntry;
            clonedEntry.weight = entry.weight;
            if (entry.provider) {
                clonedEntry.provider = entry.provider->clone();
            }
            clonedEntries.push_back(std::move(clonedEntry));
        }
        return std::make_unique<WeightedListInt>(std::move(clonedEntries));
    }

private:
    std::vector<WeightedEntry> m_entries;
    i32 m_totalWeight;
};

} // namespace mc::world::gen::valueprovider
