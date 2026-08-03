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

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <string>

namespace mc {

/**
 * @brief 竹子特征配置
 *
 * 参考 MC BambooFeature 使用的 ProbabilityFeatureConfiguration。
 * probability 控制是否在竹子基部周围放置灰化土圆盘。
 */
struct BambooFeatureConfig : public IFeatureConfig {
    /// 竹子方块状态（AGE=1, STAGE=0, LEAVES=None 的粗竹竿）
    const BlockState* bambooState = nullptr;

    /// 竹子顶部方块状态（LEAVES=Large, STAGE=1，停止生长的顶部）
    const BlockState* topFinalState = nullptr;

    /// 竹子顶部下方第一格方块状态（LEAVES=Large, STAGE=0）
    const BlockState* topLargeState = nullptr;

    /// 竹子顶部下方第二格方块状态（LEAVES=Small, STAGE=0）
    const BlockState* topSmallState = nullptr;

    /// 灰化土放置概率（0.0 = 永不放置，0.2 = 竹子丛林，1.0 = 总是放置）
    f32 podzolProbability = 0.0f;

    BambooFeatureConfig() = default;
};

/**
 * @brief 竹子特征
 *
 * 在丛林生物群系中生成竹子。
 * 竹子高度为 5-16 格，可选在基部周围放置灰化土圆盘。
 * 参考 MC BambooFeature
 */
class BambooFeature {
public:
    /**
     * @brief 放置竹子特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 竹子配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BambooFeatureConfig& config);

private:
    /**
     * @brief 检查竹子是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查下方方块是否支持竹子生长
     */
    [[nodiscard]] bool _isValidGround(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化竹子特征
 */
class ConfiguredBambooFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBambooFeature(std::unique_ptr<BambooFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const noexcept override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const noexcept override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const BambooFeatureConfig& getConfig() const noexcept { return *m_config; }

private:
    std::unique_ptr<BambooFeatureConfig> m_config;
    std::string m_name;
    // BambooFeature::place() 算法重载非 const（工具类无状态），但 ConfiguredBambooFeature::place() 语义不变
    // feature 对象本身在放置时不可变。标记 mutable 使 const override 可调用算法。
    mutable BambooFeature m_feature;
};

} // namespace mc
