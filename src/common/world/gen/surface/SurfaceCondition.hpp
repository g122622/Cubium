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

#include "common/world/biome/Biomes.hpp"
#include "common/world/gen/surface/CaveSurface.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::noise {
class NormalNoise;
}

namespace mc::math {
class PositionalRandomFactory;
}

namespace mc::world::gen::surface {

class SurfaceRuleContext;

// ============================================================================
// SurfaceCondition — MC 1.21 SurfaceRules 条件接口
// ============================================================================

/**
 * @brief SurfaceRules 条件基类
 *
 * 条件用于判断在当前位置是否满足特定规则的前提。
 * MC 1.21 中 ConditionSource.apply(Context) 创建绑定 Context 的 Condition，
 * 本项目简化为直接在 Condition 中持有 const Context 引用。
 */
class SurfaceCondition {
public:
    virtual ~SurfaceCondition() = default;

    /** 在当前 context 下评估条件 */
    [[nodiscard]] virtual bool test(const SurfaceRuleContext& ctx) const = 0;
};

// ============================================================================
// LazyXZCondition / LazyYCondition — MC 1.21 SurfaceRules.LazyCondition
// ============================================================================
// MC 1.21: XZ-only 条件（NoiseThresholdCondition/Hole/Steep）每列只求值一次，
// Y 依赖条件（StoneDepth/Y/Water/VerticalGradient/Biome/Temperature）每 Y 步只求值一次。
// 原版将缓存放进 per-call 的 LazyCondition 实例；本项目规则树跨线程共享，
// 故缓存由 SurfaceRuleContext 持有（以 this 指针为 key），见 cachedXZ/cachedY。
// 子类实现 compute()；test() 为 final，负责命中/求值/缓存。

/**
 * @brief XZ-only 条件基类：结果在当前列内缓存（updateXZ 失效）。
 * 对应原版 SurfaceRules.LazyXZCondition。
 */
class LazyXZCondition : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const final;
    [[nodiscard]] virtual bool compute(const SurfaceRuleContext& ctx) const = 0;
};

/**
 * @brief Y 依赖条件基类：结果在当前 Y 步内缓存（updateY/updateXZ 失效）。
 * 对应原版 SurfaceRules.LazyYCondition。
 */
class LazyYCondition : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const final;
    [[nodiscard]] virtual bool compute(const SurfaceRuleContext& ctx) const = 0;
};

/** 石块深度检查（MC: StoneDepthCheck） */
class StoneDepthCondition final : public LazyYCondition {
public:
    StoneDepthCondition(i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface)
        : m_offset(offset)
        , m_addSurfaceDepth(addSurfaceDepth)
        , m_secondaryDepthRange(secondaryDepthRange)
        , m_surface(surface)
    {}

    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;

private:
    i32 m_offset;
    bool m_addSurfaceDepth;
    i32 m_secondaryDepthRange;
    CaveSurface m_surface;
};

/** Y 高度检查（MC: YCondition） */
class YCondition final : public LazyYCondition {
public:
    YCondition(VerticalAnchor anchor, i32 surfaceDepthMultiplier, bool addStoneDepth)
        : m_anchor(anchor)
        , m_surfaceDepthMultiplier(surfaceDepthMultiplier)
        , m_addStoneDepth(addStoneDepth)
    {}

    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;

private:
    VerticalAnchor m_anchor;
    i32 m_surfaceDepthMultiplier;
    bool m_addStoneDepth;
};

/** 水面检查（MC: WaterCondition） */
class WaterCondition final : public LazyYCondition {
public:
    WaterCondition(i32 offset, i32 surfaceDepthMultiplier, bool addStoneDepth)
        : m_offset(offset)
        , m_surfaceDepthMultiplier(surfaceDepthMultiplier)
        , m_addStoneDepth(addStoneDepth)
    {}

    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;

private:
    i32 m_offset;
    i32 m_surfaceDepthMultiplier;
    bool m_addStoneDepth;
};

/** 生物群系检查 */
class BiomeCondition final : public LazyYCondition {
public:
    explicit BiomeCondition(std::vector<BiomeId> biomes)
        : m_biomes(std::move(biomes))
    {}

    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;

private:
    std::vector<BiomeId> m_biomes;
};

/** NOT 条件 — 普通（非 lazy），委托给内层条件；内层若是 lazy 则自行命中其缓存 */
class NotCondition final : public SurfaceCondition {
public:
    explicit NotCondition(std::unique_ptr<SurfaceCondition> condition)
        : m_condition(std::move(condition))
    {}

    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override { return !m_condition->test(ctx); }

private:
    std::unique_ptr<SurfaceCondition> m_condition;
};

/**
 * @brief 噪声阈值条件（MC: NoiseThresholdConditionSource）
 *
 * MC 1.21: 存储噪声名称，原版在 apply(context) 时一次性解析 NormalNoise，且 Condition
 * 生命周期绑定 Context（随 RandomState 一起销毁）。本项目规则树跨 RandomState/线程共享，
 * 不能在 Condition 上缓存解析后的 NormalNoise*（RandomState 销毁后即悬垂，见下），
 * 故每次 compute() 通过 ctx.randomState()->getOrCreateNoise() 现解析。布尔结果已由
 * SurfaceRuleContext::cachedXZ 按 XZ 戳缓存（每列只 compute 一次），解析仅在缓存未命中
 * 时发生，getOrCreateNoise 内部走 shared_lock 哈希命中快路径，性能无忧。
 * 属 XZ-only 条件（getValue(blockX, 0, blockZ) 不依赖 Y），继承 LazyXZCondition。
 */
class NoiseThresholdCondition final : public LazyXZCondition {
public:
    NoiseThresholdCondition(std::string noiseName, f64 minThreshold, f64 maxThreshold)
        : m_noiseName(std::move(noiseName))
        , m_minThreshold(minThreshold)
        , m_maxThreshold(maxThreshold)
    {}

    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;

private:
    std::string m_noiseName;
    f64 m_minThreshold;
    f64 m_maxThreshold;
};

/**
 * @brief 垂直梯度条件（MC: VerticalGradientConditionSource）
 *
 * 用于基岩层等（随机梯度过渡），MC 1.21 中存储随机工厂名称。与 NoiseThresholdCondition
 * 同理：不在共享 Condition 上缓存 PositionalRandomFactory*（会随首个 RandomState 销毁而
 * 悬垂），每次 compute() 通过 ctx.randomState()->getOrCreateRandomFactory() 现解析。
 * 依赖 blockY（每 Y 步不同），继承 LazyYCondition。
 */
class VerticalGradientCondition final : public LazyYCondition {
public:
    VerticalGradientCondition(std::string randomName, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove)
        : m_randomName(std::move(randomName))
        , m_trueAtAndBelow(trueAtAndBelow)
        , m_falseAtAndAbove(falseAtAndAbove)
    {}

    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;

private:
    std::string m_randomName;
    VerticalAnchor m_trueAtAndBelow;
    VerticalAnchor m_falseAtAndAbove;
};

/** 陡峭条件 — XZ-only（MC: SteepMaterialCondition） */
class SteepCondition final : public LazyXZCondition {
public:
    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;
};

/** 温度条件（是否冷到可以降雪）— Y 依赖（MC: TemperatureHelperCondition） */
class TemperatureCondition final : public LazyYCondition {
public:
    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;
};

/** Hole 条件（surfaceDepth <= 0）— XZ-only（MC: HoleCondition） */
class HoleCondition final : public LazyXZCondition {
public:
    [[nodiscard]] bool compute(const SurfaceRuleContext& ctx) const override;
};

/** 预备表面以上条件 — 普通（非 lazy）；ctx.abovePreliminarySurface() 内部已有 XZ 缓存 */
class AbovePreliminarySurfaceCondition final : public SurfaceCondition {
public:
    [[nodiscard]] bool test(const SurfaceRuleContext& ctx) const override;
};

} // namespace mc::world::gen::surface
