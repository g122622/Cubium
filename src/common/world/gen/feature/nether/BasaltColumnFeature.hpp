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
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <memory>

namespace mc {

/**
 * @brief 玄武岩柱特征配置
 */
struct BasaltColumnFeatureConfig : public IFeatureConfig {
    /// 最小柱高
    i32 minHeight = 0;

    /// 最大柱高
    i32 maxHeight = 5;

    /// 是否尝试达到天花板
    bool reachCeiling = false;

    BasaltColumnFeatureConfig() = default;

    explicit BasaltColumnFeatureConfig(i32 minH, i32 maxH, bool reach = false)
        : minHeight(minH)
        , maxHeight(maxH)
        , reachCeiling(reach)
    {}
};

/**
 * @brief 玄武岩柱特征
 *
 * 使用聚类算法生成从地板向上延伸的玄武岩柱。
 * 90% 概率使用聚类模式（reach=5, size=50），
 * 10% 概率使用非聚类模式（reach=8, size=15）。
 */
class BasaltColumnFeature {
public:
    /**
     * @brief 放置玄武岩柱
     */
    bool place(
        WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BasaltColumnFeatureConfig& config);

private:
    /**
     * @brief 在指定位置放置一根柱子
     * @param world 世界区域
     * @param pos 柱子底部位置
     * @param height 柱子高度
     */
    void _placeColumn(WorldGenRegion& world, const BlockPos& pos, i32 height);

    /**
     * @brief 检查位置是否可以放置玄武岩（空气或熔岩海洋，且下方不在 CANNOT_PLACE_ON 中）
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel) const;

    /**
     * @brief 向下搜索，找到可以放置玄武岩的表面位置
     */
    [[nodiscard]] BlockPos _findSurface(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel) const;

    /**
     * @brief 向上搜索，找到空气位置
     */
    [[nodiscard]] BlockPos _findAir(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel) const;
};

/**
 * @brief 配置化玄武岩柱特征
 */
class ConfiguredBasaltColumnFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBasaltColumnFeature(std::unique_ptr<BasaltColumnFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const BasaltColumnFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BasaltColumnFeatureConfig> m_config;
    std::string m_name;
    mutable BasaltColumnFeature m_feature;
};

} // namespace mc
