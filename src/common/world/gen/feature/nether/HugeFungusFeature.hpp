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
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/Feature.hpp"

#include <memory>
#include <string>

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
 * 参考 MC 1.21.11: HugeFungusConfiguration
 */
struct HugeFungusFeatureConfig : public IFeatureConfig {
    /// 真菌类型
    FungusType fungusType = FungusType::Crimson;

    /// 是否种植在菌岩上（种植的不需要空间检查）
    bool planted = false;

    HugeFungusFeatureConfig() = default;

    explicit HugeFungusFeatureConfig(FungusType type, bool isPlanted = false)
        : fungusType(type)
        , planted(isPlanted)
    {}
};

/**
 * @brief 巨型真菌特征
 *
 * 在下界生成巨型真菌（绯红和诡异）。
 *
 * 绯红真菌：
 * - 绯红菌柄 + 下界疣块菌盖 + 垂泪藤 + 菌光体
 * - 1/12 概率双倍高度
 * - 6% 概率粗壮菌柄（3x3）
 *
 * 诡异真菌：
 * - 诡异菌柄 + 诡异疣块菌盖 + 扭曲藤 + 菌光体
 * - 1/12 概率双倍高度
 * - 6% 概率粗壮菌柄（3x3）
 *
 * 参考 MC 1.21.11: HugeFungusFeature
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
    [[nodiscard]] bool _canPlaceAt(
        WorldGenRegion& world, const BlockPos& pos, const HugeFungusFeatureConfig& config) const;

    /**
     * @brief 获取菌柄方块状态
     */
    [[nodiscard]] const BlockState* _getStemState(FungusType type) const;

    /**
     * @brief 获取菌盖方块状态
     */
    [[nodiscard]] const BlockState* _getCapState(FungusType type) const;

    /**
     * @brief 生成菌柄
     */
    void _generateStem(
        WorldGenRegion& world, const BlockPos& pos, i32 height, const BlockState* stemState, bool thickStem);

    /**
     * @brief 生成菌盖
     */
    void _generateCap(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& topPos,
        i32 capHeight,
        const BlockState* capState,
        const BlockState* shroomlightState,
        const BlockState* airState,
        bool thickStem);

    /**
     * @brief 生成藤蔓（垂泪藤或扭曲藤）
     */
    void _generateVines(
        WorldGenRegion& world, math::Random& random, const BlockPos& stemBase, i32 stemHeight, FungusType type);

    /**
     * @brief 生成菌岩基座
     */
    void _generateBase(WorldGenRegion& world, const BlockPos& pos, const HugeFungusFeatureConfig& config);
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
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const HugeFungusFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<HugeFungusFeatureConfig> m_config;
    std::string m_name;
    mutable HugeFungusFeature m_feature;
};

} // namespace mc
