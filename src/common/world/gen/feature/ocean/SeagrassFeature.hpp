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
 * @brief 海草特征配置
 */
struct SeagrassFeatureConfig : public IFeatureConfig {
    /// 海草方块状态
    const BlockState* seagrassState = nullptr;

    /// 高海草下方方块状态
    const BlockState* tallSeagrassLowerState = nullptr;

    /// 高海草上方方块状态
    const BlockState* tallSeagrassUpperState = nullptr;

    /// 高海草概率 (0.0 - 1.0)
    f32 tallSeagrassChance = 0.0f;

    /// 放置尝试次数
    i32 tries = 48;

    /// 水平扩散半径（使用 nextInt(spread)-nextInt(spread)）
    i32 horizontalSpread = 8;

    SeagrassFeatureConfig() = default;

    explicit SeagrassFeatureConfig(const BlockState* seagrass)
        : seagrassState(seagrass)
        , tallSeagrassChance(0.0f)
        , tries(48)
        , horizontalSpread(8)
    {}

    SeagrassFeatureConfig(const BlockState* seagrass,
        const BlockState* tallLower,
        const BlockState* tallUpper,
        f32 tallChance = 0.3f,
        i32 t = 48,
        i32 spread = 8)
        : seagrassState(seagrass)
        , tallSeagrassLowerState(tallLower)
        , tallSeagrassUpperState(tallUpper)
        , tallSeagrassChance(tallChance)
        , tries(t)
        , horizontalSpread(spread)
    {}
};

/**
 * @brief 海草特征
 *
 * 在水下生成海草。
 * 可以生成普通海草和高海草。
 */
class SeagrassFeature {
public:
    /**
     * @brief 放置海草特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 海草配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SeagrassFeatureConfig& config);

private:
    /**
     * @brief 检查海草是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& seagrassState) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 放置高海草
     */
    bool _placeTallSeagrass(WorldGenRegion& world, const BlockPos& pos, const SeagrassFeatureConfig& config) const;
};

/**
 * @brief 配置化海草特征
 */
class ConfiguredSeagrassFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSeagrassFeature(std::unique_ptr<SeagrassFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const noexcept override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const SeagrassFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<SeagrassFeatureConfig> m_config;
    std::string m_name;
    mutable SeagrassFeature m_feature;
};

} // namespace mc
