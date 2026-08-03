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
 * THE SOFTWARE IS PROVIDED "AS KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace mc::world::gen::valueprovider {

// 导入 VerticalAnchor 到此命名空间
using mc::world::gen::surface::VerticalAnchor;

/**
 * @brief 世界生成上下文（MC 1.21 WorldGenerationContext）
 *
 * 提供世界生成所需的高度信息，用于 HeightProvider 解析 VerticalAnchor。
 */
class WorldGenerationContext {
public:
    WorldGenerationContext(i32 minGenY, i32 genDepth)
        : m_minGenY(minGenY)
        , m_genDepth(genDepth)
    {}

    [[nodiscard]] i32 getMinGenY() const { return m_minGenY; }
    [[nodiscard]] i32 getGenDepth() const { return m_genDepth; }

private:
    i32 m_minGenY;
    i32 m_genDepth;
};

// ============================================================================
// HeightProvider — 高度提供器基类
// ============================================================================

/**
 * @brief 高度提供器（MC 1.21 HeightProvider）
 *
 * 提供可控随机性的 Y 坐标采样。与 IntProvider 不同，
 * HeightProvider 使用 VerticalAnchor 来定义高度范围，
 * 并通过 WorldGenerationContext 解析为实际 Y 坐标。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.heightproviders.HeightProvider
 */
class HeightProvider {
public:
    virtual ~HeightProvider() = default;

    /**
     * @brief 采样一个 Y 坐标
     * @param rng 随机数生成器
     * @param context 世界生成上下文
     * @return Y 坐标
     */
    [[nodiscard]] virtual i32 sample(math::IRandom& rng, const WorldGenerationContext& context) const = 0;

    /**
     * @brief 获取提供器类型名称（用于调试/序列化）
     */
    [[nodiscard]] virtual const char* getTypeName() const = 0;

    /**
     * @brief 深拷贝此高度提供器
     *
     * 与 IntProvider/PoolAliasBinding 一致：数据驱动工厂构造配置时需复制 def 中已解析的
     * HeightProvider（def 所有权留在 Loader），故基类提供虚 clone()，子类逐个实现。
     *
     * @return 新构造的等价 HeightProvider
     */
    [[nodiscard]] virtual std::unique_ptr<HeightProvider> clone() const = 0;
};

// ============================================================================
// ConstantHeight — 固定高度提供器
// ============================================================================

/**
 * @brief 固定高度提供器（MC 1.21 ConstantHeight）
 *
 * 始终返回 VerticalAnchor 解析后的固定高度。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.heightproviders.ConstantHeight
 */
class ConstantHeight final : public HeightProvider {
public:
    static std::unique_ptr<ConstantHeight> create(VerticalAnchor value)
    {
        return std::make_unique<ConstantHeight>(std::move(value));
    }

    explicit ConstantHeight(VerticalAnchor value)
        : m_value(std::move(value))
    {}

    [[nodiscard]] i32 sample(math::IRandom& /*rng*/, const WorldGenerationContext& context) const override
    {
        return m_value.resolveY(context.getMinGenY(), context.getGenDepth());
    }

    [[nodiscard]] const char* getTypeName() const override { return "constant"; }

    [[nodiscard]] std::unique_ptr<HeightProvider> clone() const override { return ConstantHeight::create(m_value); }

    [[nodiscard]] const VerticalAnchor& getValue() const { return m_value; }

private:
    VerticalAnchor m_value;
};

// ============================================================================
// UniformHeight — 均匀分布高度提供器
// ============================================================================

/**
 * @brief 均匀分布高度提供器（MC 1.21 UniformHeight）
 *
 * 在 [minInclusive, maxInclusive] 范围内均匀采样 Y 坐标。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.heightproviders.UniformHeight
 */
class UniformHeight final : public HeightProvider {
public:
    static std::unique_ptr<UniformHeight> create(VerticalAnchor minInclusive, VerticalAnchor maxInclusive)
    {
        return std::make_unique<UniformHeight>(std::move(minInclusive), std::move(maxInclusive));
    }

    UniformHeight(VerticalAnchor minInclusive, VerticalAnchor maxInclusive)
        : m_min(std::move(minInclusive))
        , m_max(std::move(maxInclusive))
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng, const WorldGenerationContext& context) const override
    {
        const i32 minY = m_min.resolveY(context.getMinGenY(), context.getGenDepth());
        const i32 maxY = m_max.resolveY(context.getMinGenY(), context.getGenDepth());
        if (minY >= maxY) {
            return minY;
        }
        return minY + rng.nextInt(maxY - minY + 1);
    }

    [[nodiscard]] const char* getTypeName() const override { return "uniform"; }

    [[nodiscard]] std::unique_ptr<HeightProvider> clone() const override { return UniformHeight::create(m_min, m_max); }

    [[nodiscard]] const VerticalAnchor& getMin() const { return m_min; }
    [[nodiscard]] const VerticalAnchor& getMax() const { return m_max; }

private:
    VerticalAnchor m_min;
    VerticalAnchor m_max;
};

// ============================================================================
// BiasedToBottomHeight — 偏向底部高度提供器
// ============================================================================

/**
 * @brief 偏向底部高度提供器（MC 1.21 BiasedToBottomHeight）
 *
 * 在 [minInclusive, maxInclusive] 范围内采样，偏向较低值。
 * inner 参数控制偏向底部的强度（默认为 1）。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.heightproviders.BiasedToBottomHeight
 */
class BiasedToBottomHeight final : public HeightProvider {
public:
    static std::unique_ptr<BiasedToBottomHeight> create(
        VerticalAnchor minInclusive, VerticalAnchor maxInclusive, i32 inner = 1)
    {
        return std::make_unique<BiasedToBottomHeight>(std::move(minInclusive), std::move(maxInclusive), inner);
    }

    BiasedToBottomHeight(VerticalAnchor minInclusive, VerticalAnchor maxInclusive, i32 inner)
        : m_min(std::move(minInclusive))
        , m_max(std::move(maxInclusive))
        , m_inner(inner)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng, const WorldGenerationContext& context) const override
    {
        const i32 minY = m_min.resolveY(context.getMinGenY(), context.getGenDepth());
        const i32 maxY = m_max.resolveY(context.getMinGenY(), context.getGenDepth());
        if (minY >= maxY) {
            return minY;
        }
        const i32 range = maxY - minY + 1;
        // MC: min + randomBetweenInclusive(rng, 0, range - inner) + rng.nextInt(inner)
        const i32 outerRange = range - m_inner;
        if (outerRange <= 0) {
            return minY + rng.nextInt(range);
        }
        return minY + rng.nextInt(outerRange + 1) + rng.nextInt(m_inner);
    }

    [[nodiscard]] const char* getTypeName() const override { return "biased_to_bottom"; }

    [[nodiscard]] std::unique_ptr<HeightProvider> clone() const override
    {
        return BiasedToBottomHeight::create(m_min, m_max, m_inner);
    }

    [[nodiscard]] const VerticalAnchor& getMin() const { return m_min; }
    [[nodiscard]] const VerticalAnchor& getMax() const { return m_max; }
    [[nodiscard]] i32 getInner() const { return m_inner; }

private:
    VerticalAnchor m_min;
    VerticalAnchor m_max;
    i32 m_inner;
};

// ============================================================================
// VeryBiasedToBottomHeight — 强烈偏向底部高度提供器
// ============================================================================

/**
 * @brief 强烈偏向底部高度提供器（MC 1.21 VeryBiasedToBottomHeight）
 *
 * 在 [minInclusive, maxInclusive] 范围内采样，非常强烈地偏向较低值。
 * 使用三层嵌套随机来强化偏向。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.heightproviders.VeryBiasedToBottomHeight
 */
class VeryBiasedToBottomHeight final : public HeightProvider {
public:
    static std::unique_ptr<VeryBiasedToBottomHeight> create(
        VerticalAnchor minInclusive, VerticalAnchor maxInclusive, i32 inner = 1)
    {
        return std::make_unique<VeryBiasedToBottomHeight>(std::move(minInclusive), std::move(maxInclusive), inner);
    }

    VeryBiasedToBottomHeight(VerticalAnchor minInclusive, VerticalAnchor maxInclusive, i32 inner)
        : m_min(std::move(minInclusive))
        , m_max(std::move(maxInclusive))
        , m_inner(inner)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng, const WorldGenerationContext& context) const override
    {
        const i32 minY = m_min.resolveY(context.getMinGenY(), context.getGenDepth());
        const i32 maxY = m_max.resolveY(context.getMinGenY(), context.getGenDepth());
        if (minY >= maxY) {
            return minY;
        }
        const i32 range = maxY - minY + 1;
        // MC: 三层嵌套随机，强烈偏向底部
        const i32 outerRange = range - m_inner;
        if (outerRange <= 0) {
            return minY + rng.nextInt(range);
        }
        const i32 biased = rng.nextInt(rng.nextInt(outerRange + 1) + 1);
        return minY + biased + rng.nextInt(m_inner);
    }

    [[nodiscard]] const char* getTypeName() const override { return "very_biased_to_bottom"; }

    [[nodiscard]] std::unique_ptr<HeightProvider> clone() const override
    {
        return VeryBiasedToBottomHeight::create(m_min, m_max, m_inner);
    }

    [[nodiscard]] const VerticalAnchor& getMin() const { return m_min; }
    [[nodiscard]] const VerticalAnchor& getMax() const { return m_max; }
    [[nodiscard]] i32 getInner() const { return m_inner; }

private:
    VerticalAnchor m_min;
    VerticalAnchor m_max;
    i32 m_inner;
};

// ============================================================================
// TrapezoidHeight — 梯形/三角形分布高度提供器
// ============================================================================

/**
 * @brief 梯形/三角形分布高度提供器（MC 1.21 TrapezoidHeight）
 *
 * 在 [minInclusive, maxInclusive] 范围内采样，使用梯形/三角形分布。
 * plateau 控制平顶宽度：0 为三角形分布，正值增加平顶区域。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.heightproviders.TrapezoidHeight
 */
class TrapezoidHeight final : public HeightProvider {
public:
    static std::unique_ptr<TrapezoidHeight> create(
        VerticalAnchor minInclusive, VerticalAnchor maxInclusive, i32 plateau = 0)
    {
        return std::make_unique<TrapezoidHeight>(std::move(minInclusive), std::move(maxInclusive), plateau);
    }

    TrapezoidHeight(VerticalAnchor minInclusive, VerticalAnchor maxInclusive, i32 plateau)
        : m_min(std::move(minInclusive))
        , m_max(std::move(maxInclusive))
        , m_plateau(plateau)
    {}

    [[nodiscard]] i32 sample(math::IRandom& rng, const WorldGenerationContext& context) const override
    {
        const i32 minY = m_min.resolveY(context.getMinGenY(), context.getGenDepth());
        const i32 maxY = m_max.resolveY(context.getMinGenY(), context.getGenDepth());
        if (minY >= maxY) {
            return minY;
        }
        const i32 range = maxY - minY;
        if (m_plateau >= range) {
            return minY + rng.nextInt(range + 1);
        }
        // MC 梯形分布算法
        const i32 bottomSlope = m_plateau + 1;
        const i32 topSlope = range - m_plateau;
        const i32 totalArea = bottomSlope + topSlope;
        const i32 randomValue = rng.nextInt(totalArea);
        if (randomValue < bottomSlope) {
            // 在上升斜坡部分
            const i32 slopeValue = randomValue;
            // 使用 sqrt 变换产生三角形分布
            const f32 t = static_cast<f32>(slopeValue) / static_cast<f32>(bottomSlope);
            const i32 result = static_cast<i32>(t * static_cast<f32>(bottomSlope));
            return minY + result;
        }
        // 在下降斜坡部分或平顶
        return minY + m_plateau + (randomValue - bottomSlope);
    }

    [[nodiscard]] const char* getTypeName() const override { return "trapezoid"; }

    [[nodiscard]] std::unique_ptr<HeightProvider> clone() const override
    {
        return TrapezoidHeight::create(m_min, m_max, m_plateau);
    }

    [[nodiscard]] const VerticalAnchor& getMin() const { return m_min; }
    [[nodiscard]] const VerticalAnchor& getMax() const { return m_max; }
    [[nodiscard]] i32 getPlateau() const { return m_plateau; }

private:
    VerticalAnchor m_min;
    VerticalAnchor m_max;
    i32 m_plateau;
};

} // namespace mc::world::gen::valueprovider
