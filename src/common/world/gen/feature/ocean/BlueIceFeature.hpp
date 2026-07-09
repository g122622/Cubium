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
 * @brief 蓝冰特征配置
 *
 * 冷海域蓝冰生成逻辑的配置参数。
 */
struct BlueIceFeatureConfig : public IFeatureConfig {
    const BlockState* blueIceState = nullptr;
    const BlockState* packedIceState = nullptr;

    /// 传播迭代次数
    i32 spreadAttempts = 200;
};

/**
 * @brief 蓝冰特征
 */
class BlueIceFeature {
public:
    bool place(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const BlueIceFeatureConfig& config,
        i32 seaLevel);

private:
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;

    [[nodiscard]] bool _isReplaceableForSpread(
        WorldGenRegion& world, const BlockPos& pos, const BlueIceFeatureConfig& config) const;

    [[nodiscard]] i32 _findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const;
};

/**
 * @brief 配置化蓝冰特征
 */
class ConfiguredBlueIceFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBlueIceFeature(std::unique_ptr<BlueIceFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const BlueIceFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BlueIceFeatureConfig> m_config;
    std::string m_name;
    mutable BlueIceFeature m_feature;
};

} // namespace mc
