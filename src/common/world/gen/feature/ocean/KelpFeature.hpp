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
 * @brief 海带特征配置
 */
struct KelpFeatureConfig : public IFeatureConfig {
    /// 海带方块状态
    const BlockState* kelpState = nullptr;

    /// 海带顶部方块状态（用于顶端海带）
    const BlockState* kelpTopState = nullptr;

    /// 放置尝试次数
    i32 tries;

    /// 单株最大高度
    i32 maxHeight;

    KelpFeatureConfig() = default;

    explicit KelpFeatureConfig(const BlockState* kelp, const BlockState* kelpTop, i32 t, i32 maxH)
        : kelpState(kelp)
        , kelpTopState(kelpTop)
        , tries(t)
        , maxHeight(maxH)
    {}
};

/**
 * @brief 海带特征
 *
 * 在水下生成海带。
 * 海带可以从海底向上生长到水面。
 */
class KelpFeature {
public:
    /**
     * @brief 放置海带特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 海带配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const KelpFeatureConfig& config);

private:
    /**
     * @brief 检查海带是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化海带特征
 */
class ConfiguredKelpFeature : public ConfiguredFeatureBase {
public:
    ConfiguredKelpFeature(std::unique_ptr<KelpFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const KelpFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<KelpFeatureConfig> m_config;
    std::string m_name;
    mutable KelpFeature m_feature;
};

} // namespace mc
