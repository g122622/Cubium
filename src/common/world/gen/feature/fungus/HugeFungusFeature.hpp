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

namespace mc {

/**
 * @brief 巨型真菌类型
 */
enum class FungusType : u8 {
    Crimson, ///< 绯红真菌（绯红森林）
    Warped   ///< 诡异真菌（诡异森林）
};

/**
 * @brief 巨型真菌特征配置
 *
 * 参考 MC HugeFungusFeatureConfig
 */
struct HugeFungusFeatureConfig : public IFeatureConfig {
    /// 真菌类型
    FungusType fungusType = FungusType::Crimson;

    /// 最小高度
    i32 minHeight = 4;

    /// 最大高度
    i32 maxHeight = 13;

    /// 菌盖半径
    i32 capRadius = 2;

    /// 是否种植在菌岩上
    bool planted = false;

    HugeFungusFeatureConfig() = default;

    explicit HugeFungusFeatureConfig(
        FungusType type, i32 minH = 4, i32 maxH = 13, i32 radius = 2, bool isPlanted = false)
        : fungusType(type)
        , minHeight(minH)
        , maxHeight(maxH)
        , capRadius(radius)
        , planted(isPlanted)
    {}
};

/**
 * @brief 巨型真菌特征
 *
 * 在下界生成巨型真菌（绯红和诡异）。
 * 参考 MC HugeFungusFeature
 *
 * 绯红真菌：
 * - 绯红菌柄（深红色）
 * - 绯红菌盖（红色菌块）
 * - 生成垂泪藤
 *
 * 诡异真菌：
 * - 诡异菌柄（青色）
 * - 诡异菌盖（青色菌块）
 * - 生成扭曲藤
 */
class HugeFungusFeature {
public:
    /**
     * @brief 放置巨型真菌
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 真菌配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const HugeFungusFeatureConfig& config);

private:
    /**
     * @brief 检查巨型真菌是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos, FungusType type) const;

    /**
     * @brief 生成菌柄
     */
    void generateStem(WorldGenRegion& world, math::Random& random, const BlockPos& pos, i32 height, FungusType type);

    /**
     * @brief 生成菌盖
     */
    void generateCap(WorldGenRegion& world, math::Random& random, const BlockPos& topPos, i32 radius, FungusType type);

    /**
     * @brief 生成藤蔓（垂泪藤或扭曲藤）
     */
    void generateVines(WorldGenRegion& world, math::Random& random, const BlockPos& capPos, FungusType type);

    /**
     * @brief 生成菌光体（发光装饰）
     */
    void generateShroomlights(WorldGenRegion& world, math::Random& random, const BlockPos& capPos, i32 radius);
};

/**
 * @brief 配置化巨型真菌特征
 */
class ConfiguredHugeFungusFeature : public ConfiguredFeatureBase {
public:
    ConfiguredHugeFungusFeature(std::unique_ptr<HugeFungusFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const HugeFungusFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<HugeFungusFeatureConfig> m_config;
    std::string m_name;
    HugeFungusFeature m_feature;
};

/**
 * @brief 预定义巨型真菌特征
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct HugeFungusFeatures {
    /// 初始化所有巨型真菌特征
    static void initialize();

    /// 获取所有巨型真菌特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredHugeFungusFeature>>& getAllFeatures();

    /// 获取所有巨型真菌特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredHugeFungusFeature>> getAllFeaturesAndClear();

    /// 创建绯红巨型真菌
    static std::unique_ptr<ConfiguredHugeFungusFeature> createCrimson();

    /// 创建诡异巨型真菌
    static std::unique_ptr<ConfiguredHugeFungusFeature> createWarped();

private:
    static std::vector<std::unique_ptr<ConfiguredHugeFungusFeature>> s_features;
};

} // namespace mc
