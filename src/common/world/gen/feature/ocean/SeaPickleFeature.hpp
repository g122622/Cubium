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
#include <memory>

namespace mc {

/**
 * @brief 海泡菜特征配置
 */
struct SeaPickleFeatureConfig : public IFeatureConfig {
    /// 海泡菜方块状态
    const BlockState* seaPickleState = nullptr;

    /// 尝试放置次数
    i32 tries = 10;

    /// 最大数量 (1-4)
    i32 maxCount = 4;

    SeaPickleFeatureConfig() = default;

    explicit SeaPickleFeatureConfig(const BlockState* state, i32 t, i32 maxC) noexcept
        : seaPickleState(state)
        , tries(t)
        , maxCount(maxC)
    {}
};

/**
 * @brief 海泡菜特征
 *
 * 在水下生成海泡菜，通常在暖水海洋。
 * 海泡菜可以堆叠1-4个。
 */
class SeaPickleFeature {
public:
    /**
     * @brief 放置海泡菜特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 海泡菜配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SeaPickleFeatureConfig& config);

private:
    /**
     * @brief 检查海泡菜是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& pickleState) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化海泡菜特征
 */
class ConfiguredSeaPickleFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSeaPickleFeature(std::unique_ptr<SeaPickleFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const noexcept override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const noexcept override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const SeaPickleFeatureConfig& getConfig() const noexcept { return *m_config; }

private:
    std::unique_ptr<SeaPickleFeatureConfig> m_config;
    std::string m_name;
    mutable SeaPickleFeature m_feature;
};

} // namespace mc
