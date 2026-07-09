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
 * @brief 甘蔗特征配置
 *
 * 参考 MC BlockStateFeatureConfig
 */
struct SugarCaneFeatureConfig : public IFeatureConfig {
    /// 甘蔗方块状态
    const BlockState* state = nullptr;

    /// 最大高度
    i32 maxHeight = 3;

    /// 尝试次数
    i32 tries = 20;

    /// X/Z扩散范围
    i32 xzSpread = 8;

    SugarCaneFeatureConfig() = default;

    explicit SugarCaneFeatureConfig(const BlockState* sugarCaneState, i32 maxH = 3)
        : state(sugarCaneState)
        , maxHeight(maxH)
    {}
};

/**
 * @brief 甘蔗特征
 *
 * 在水源附近生成甘蔗。
 * 参考 MC SugarCaneFeature / ReedsFeature
 */
class SugarCaneFeature {
public:
    /**
     * @brief 放置甘蔗特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 甘蔗配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SugarCaneFeatureConfig& config);

private:
    /**
     * @brief 检查甘蔗是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查周围是否有水
     * 甘蔗需要相邻的水源才能生长
     */
    [[nodiscard]] bool _hasWaterNearby(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查下方方块是否支持甘蔗生长
     */
    [[nodiscard]] bool _isValidGround(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化甘蔗特征
 */
class ConfiguredSugarCaneFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSugarCaneFeature(std::unique_ptr<SugarCaneFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const noexcept override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const noexcept override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const SugarCaneFeatureConfig& getConfig() const noexcept { return *m_config; }

private:
    std::unique_ptr<SugarCaneFeatureConfig> m_config;
    std::string m_name;
    // SugarCaneFeature::place() 算法重载非 const（工具类无状态），但 ConfiguredSugarCaneFeature::place() 语义不变
    // feature 对象本身在放置时不可变。标记 mutable 使 const override 可调用算法。
    mutable SugarCaneFeature m_feature;
};

} // namespace mc
