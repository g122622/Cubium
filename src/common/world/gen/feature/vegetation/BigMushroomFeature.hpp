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
 * @brief 巨型蘑菇特征配置
 */
struct BigMushroomFeatureConfig : public IFeatureConfig {
    /// 蘑菇盖方块状态
    const BlockState* capState = nullptr;

    /// 蘑菇柄方块状态
    const BlockState* stemState = nullptr;

    /// 蘑菇盖半径
    i32 capRadius = 2;

    BigMushroomFeatureConfig() = default;

    BigMushroomFeatureConfig(const BlockState* cap, const BlockState* stem, i32 radius)
        : capState(cap)
        , stemState(stem)
        , capRadius(radius)
    {}
};

/**
 * @brief 巨型蘑菇特征基类
 */
class BigMushroomFeature {
public:
    virtual ~BigMushroomFeature() = default;

    /**
     * @brief 放置巨型蘑菇特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 蘑菇配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BigMushroomFeatureConfig& config);

protected:
    /**
     * @brief 计算给定高度的蘑菇盖半径
     * @param baseRadius 基础半径
     * @param totalHeight 总高度
     * @param capRadius 配置的盖半径
     * @param currentHeight 当前高度
     * @return 当前高度的半径
     */
    [[nodiscard]] virtual i32 getCapRadius(i32 baseRadius, i32 totalHeight, i32 capRadius, i32 currentHeight) const = 0;

    /**
     * @brief 生成蘑菇盖
     */
    virtual void generateCap(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) = 0;

    /**
     * @brief 生成蘑菇柄
     */
    void generateStem(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const BigMushroomFeatureConfig& config,
        i32 height);

    /**
     * @brief 计算蘑菇高度
     */
    [[nodiscard]] i32 calculateHeight(math::Random& random) const;

    /**
     * @brief 检查是否可以放置蘑菇
     */
    [[nodiscard]] bool canPlaceAt(
        WorldGenRegion& world, const BlockPos& pos, i32 height, const BigMushroomFeatureConfig& config) const;
};

/**
 * @brief 巨型棕色蘑菇特征
 *
 * 生成平顶的棕色巨型蘑菇
 */
class BigBrownMushroomFeature : public BigMushroomFeature {
public:
    ~BigBrownMushroomFeature() override = default;

protected:
    [[nodiscard]] i32 getCapRadius(i32 baseRadius, i32 totalHeight, i32 capRadius, i32 currentHeight) const override;

    void generateCap(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) override;
};

/**
 * @brief 巨型红色蘑菇特征
 *
 * 生成圆顶的红色巨型蘑菇
 */
class BigRedMushroomFeature : public BigMushroomFeature {
public:
    ~BigRedMushroomFeature() override = default;

protected:
    [[nodiscard]] i32 getCapRadius(i32 baseRadius, i32 totalHeight, i32 capRadius, i32 currentHeight) const override;

    void generateCap(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) override;
};

/**
 * @brief 配置化巨型蘑菇特征
 */
class ConfiguredBigMushroomFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBigMushroomFeature(
        std::unique_ptr<BigMushroomFeatureConfig> config, const char* featureName, bool isBrown);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const BigMushroomFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BigMushroomFeatureConfig> m_config;
    std::string m_name;
    std::unique_ptr<BigMushroomFeature> m_feature;
};

} // namespace mc
