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
#include <memory>
#include <optional>

namespace mc {

/**
 * @brief 树木最小尺寸约束类型枚举
 *
 * 对应 MC 1.21.11 net.minecraft.world.level.levelgen.feature.featuresize.FeatureSizeType
 * 通过 type 字符串（如 "minecraft:two_layers_feature_size"）在 JSON 中分派到对应子类。
 */
enum class FeatureSizeType : u8 {
    /// 两层特征尺寸（TwoLayersFeatureSize）：以 limit 为界分下层/上层
    TwoLayers,
    /// 三层特征尺寸（ThreeLayersFeatureSize）：以 limit / upperLimit 分下/中/上三层
    ThreeLayers,
};

/**
 * @brief 树木最小尺寸约束基类
 *
 * 定义树木生成时的最小空间占用规则。TreeFeature 在 getMaxFreeTreeHeight 阶段
 * 调用 getSizeAtHeight(trunkHeight, y) 获取每一层 y 的水平检查半径，
 * 用于判断树干周围是否有足够空间放置。
 *
 * 子类通过 limit / upperLimit 等阈值切换不同层的尺寸值（lowerSize / middleSize / upperSize），
 * 以模拟原版树木"底部细、顶部粗"或"三层渐变"的空间需求。
 *
 * 对应 MC 1.21.11 net.minecraft.world.level.levelgen.feature.featuresize.FeatureSize
 *
 * minClippedHeight：可选的最小裁剪高度。当 getMaxFreeTreeHeight 返回的实际可用高度
 * 不足 trunkHeight 时，若 minClippedHeight 有值且实际高度 >= minClippedHeight，
 * 仍然允许树木生成（用裁剪后的高度）。用于 fancy_oak 等需要容忍较矮空间的配置。
 */
class FeatureSize {
public:
    FeatureSize() = default;
    explicit FeatureSize(std::optional<i32> minClippedHeight)
        : m_minClippedHeight(minClippedHeight)
    {}

    virtual ~FeatureSize() = default;

    /**
     * @brief 获取本约束的类型
     */
    [[nodiscard]] virtual FeatureSizeType type() const = 0;

    /**
     * @brief 获取指定 y 层（相对树干底部）的水平检查半径
     *
     * @param trunkHeight 树干总高度
     * @param y 相对树干底部的 y 偏移（0 = 树干底部）
     * @return 该层的水平检查半径
     */
    [[nodiscard]] virtual i32 getSizeAtHeight(i32 trunkHeight, i32 y) const = 0;

    /**
     * @brief 最小裁剪高度（可选）
     *
     * 若返回非空值，当实际可用高度不足 trunkHeight 但 >= 该值时，仍允许生成。
     */
    [[nodiscard]] std::optional<i32> minClippedHeight() const noexcept { return m_minClippedHeight; }

    /**
     * @brief 克隆本对象（深拷贝）
     */
    [[nodiscard]] virtual std::unique_ptr<FeatureSize> clone() const = 0;

protected:
    std::optional<i32> m_minClippedHeight;
};

/**
 * @brief 两层特征尺寸
 *
 * 以 limit 为界：
 * - y < limit：返回 lowerSize
 * - y >= limit：返回 upperSize
 *
 * 对应 MC 1.21.11 net.minecraft.world.level.levelgen.feature.featuresize.TwoLayersFeatureSize
 * 数据包示例（acacia.json）：
 *   "minimum_size": {
 *     "type": "minecraft:two_layers_feature_size",
 *     "limit": 1,
 *     "lower_size": 0,
 *     "upper_size": 2
 *   }
 */
class TwoLayersFeatureSize final : public FeatureSize {
public:
    /**
     * @brief 构造两层特征尺寸
     * @param limit 下层到上层的 y 阈值
     * @param lowerSize y < limit 时的水平检查半径
     * @param upperSize y >= limit 时的水平检查半径
     * @param minClippedHeight 可选的最小裁剪高度
     */
    TwoLayersFeatureSize(i32 limit, i32 lowerSize, i32 upperSize, std::optional<i32> minClippedHeight = std::nullopt)
        : FeatureSize(minClippedHeight)
        , m_limit(limit)
        , m_lowerSize(lowerSize)
        , m_upperSize(upperSize)
    {}

    [[nodiscard]] FeatureSizeType type() const noexcept override { return FeatureSizeType::TwoLayers; }

    [[nodiscard]] i32 getSizeAtHeight(i32 /*trunkHeight*/, i32 y) const noexcept override
    {
        return y < m_limit ? m_lowerSize : m_upperSize;
    }

    [[nodiscard]] i32 limit() const noexcept { return m_limit; }
    [[nodiscard]] i32 lowerSize() const noexcept { return m_lowerSize; }
    [[nodiscard]] i32 upperSize() const noexcept { return m_upperSize; }

    [[nodiscard]] std::unique_ptr<FeatureSize> clone() const override
    {
        return std::make_unique<TwoLayersFeatureSize>(m_limit, m_lowerSize, m_upperSize, m_minClippedHeight);
    }

private:
    i32 m_limit;
    i32 m_lowerSize;
    i32 m_upperSize;
};

/**
 * @brief 三层特征尺寸
 *
 * 以 limit / (trunkHeight - upperLimit) 为界分三层：
 * - y < limit：返回 lowerSize
 * - limit <= y < trunkHeight - upperLimit：返回 middleSize
 * - y >= trunkHeight - upperLimit：返回 upperSize
 *
 * 对应 MC 1.21.11 net.minecraft.world.level.levelgen.feature.featuresize.ThreeLayersFeatureSize
 * 数据包示例（dark_oak.json）：
 *   "minimum_size": {
 *     "type": "minecraft:three_layers_feature_size",
 *     "limit": 1,
 *     "lower_size": 0,
 *     "middle_size": 1,
 *     "upper_limit": 1,
 *     "upper_size": 2
 *   }
 */
class ThreeLayersFeatureSize final : public FeatureSize {
public:
    /**
     * @brief 构造三层特征尺寸
     * @param limit 下层到中层的 y 阈值
     * @param upperLimit 中层到上层的 y 阈值（相对于 trunkHeight 的偏移）
     * @param lowerSize y < limit 时的水平检查半径
     * @param middleSize limit <= y < trunkHeight - upperLimit 时的水平检查半径
     * @param upperSize y >= trunkHeight - upperLimit 时的水平检查半径
     * @param minClippedHeight 可选的最小裁剪高度
     */
    ThreeLayersFeatureSize(i32 limit,
        i32 upperLimit,
        i32 lowerSize,
        i32 middleSize,
        i32 upperSize,
        std::optional<i32> minClippedHeight = std::nullopt)
        : FeatureSize(minClippedHeight)
        , m_limit(limit)
        , m_upperLimit(upperLimit)
        , m_lowerSize(lowerSize)
        , m_middleSize(middleSize)
        , m_upperSize(upperSize)
    {}

    [[nodiscard]] FeatureSizeType type() const noexcept override { return FeatureSizeType::ThreeLayers; }

    [[nodiscard]] i32 getSizeAtHeight(i32 trunkHeight, i32 y) const noexcept override
    {
        if (y < m_limit) {
            return m_lowerSize;
        }
        return y >= trunkHeight - m_upperLimit ? m_upperSize : m_middleSize;
    }

    [[nodiscard]] i32 limit() const noexcept { return m_limit; }
    [[nodiscard]] i32 upperLimit() const noexcept { return m_upperLimit; }
    [[nodiscard]] i32 lowerSize() const noexcept { return m_lowerSize; }
    [[nodiscard]] i32 middleSize() const noexcept { return m_middleSize; }
    [[nodiscard]] i32 upperSize() const noexcept { return m_upperSize; }

    [[nodiscard]] std::unique_ptr<FeatureSize> clone() const override
    {
        return std::make_unique<ThreeLayersFeatureSize>(
            m_limit, m_upperLimit, m_lowerSize, m_middleSize, m_upperSize, m_minClippedHeight);
    }

private:
    i32 m_limit;
    i32 m_upperLimit;
    i32 m_lowerSize;
    i32 m_middleSize;
    i32 m_upperSize;
};

} // namespace mc
