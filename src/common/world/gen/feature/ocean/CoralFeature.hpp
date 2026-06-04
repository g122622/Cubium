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

#include "../../../block/blocks/coral/CoralBlock.hpp"
#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <memory>

namespace mc {

/**
 * @brief 珊瑚特征配置
 */
struct CoralFeatureConfig : public IFeatureConfig {
    /// 珊瑚颜色
    blocks::CoralColor color;

    /// 是否生成墙上的珊瑚扇
    bool includeWallFan = true;

    /// 是否生成失活珊瑚
    bool isDead = false;

    CoralFeatureConfig() = default;

    explicit CoralFeatureConfig(blocks::CoralColor coralColor, bool wallFan = true, bool dead = false)
        : color(coralColor)
        , includeWallFan(wallFan)
        , isDead(dead)
    {}
};

/**
 * @brief 珊瑚特征基类
 *
 * 在水下生成珊瑚结构。
 * 有多种变体：树形、蘑菇形、爪形。
 */
class CoralFeature {
public:
    /**
     * @brief 放置珊瑚特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 珊瑚配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config);

protected:
    /**
     * @brief 检查珊瑚是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 放置珊瑚方块
     */
    void _placeCoralBlock(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color) const;

    /**
     * @brief 放置珊瑚扇
     */
    void _placeCoralFan(
        WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, Direction direction) const;
};

/**
 * @brief 配置化珊瑚特征
 */
class ConfiguredCoralFeature : public ConfiguredFeatureBase {
public:
    ConfiguredCoralFeature(std::unique_ptr<CoralFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const CoralFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<CoralFeatureConfig> m_config;
    std::string m_name;
    CoralFeature m_feature;
};

/**
 * @brief 珊瑚树特征
 *
 * 生成垂直向上的珊瑚结构。
 */
class CoralTreeFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config);

private:
    void _generateBranch(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        blocks::CoralColor color,
        bool isDead,
        Direction direction,
        i32 length,
        bool includeDecorations);
};

/**
 * @brief 珊瑚蘑菇特征
 *
 * 生成蘑菇形状的珊瑚结构。
 */
class CoralMushroomFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config);

private:
    void _generateCap(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        blocks::CoralColor color,
        bool isDead,
        i32 radius,
        bool includeDecorations);
};

/**
 * @brief 珊瑚爪特征
 *
 * 生成爪形的珊瑚结构。
 */
class CoralClawFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config);

private:
    void _generateClaw(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        blocks::CoralColor color,
        bool isDead,
        Direction direction,
        bool includeDecorations);
};

/**
 * @brief 预定义珊瑚配置
 */
struct CoralFeatures {
    /// 初始化所有珊瑚特征
    static void initialize();

    /// 获取所有珊瑚特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredCoralFeature>>& getAllFeatures();

    /// 获取所有珊瑚特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredCoralFeature>> getAllFeaturesAndClear();

    /// 创建管状珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createTubeCoral();

    /// 创建脑珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createBrainCoral();

    /// 创建气泡珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createBubbleCoral();

    /// 创建火焰珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createFireCoral();

    /// 创建角珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createHornCoral();

    /// 创建失活管状珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createDeadTubeCoral();

    /// 创建失活脑珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createDeadBrainCoral();

    /// 创建失活气泡珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createDeadBubbleCoral();

    /// 创建失活火焰珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createDeadFireCoral();

    /// 创建失活角珊瑚
    static std::unique_ptr<ConfiguredCoralFeature> createDeadHornCoral();

private:
    static std::vector<std::unique_ptr<ConfiguredCoralFeature>> s_features;
};

} // namespace mc
