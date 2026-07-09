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

#include <memory>
#include <vector>

#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"

namespace mc {

/**
 * @brief 草丛特征配置
 *
 * 用于配置草、蕨类等植被的生成参数。
 */
struct GrassFeatureConfig : public IFeatureConfig {
    /// 可放置的方块状态列表（随机选择）
    std::vector<const BlockState*> states;

    /// 尝试放置次数
    i32 tries = 64;

    /// X方向扩散范围
    i32 xSpread = 7;

    /// Y方向扩散范围
    i32 ySpread = 3;

    /// Z方向扩散范围
    i32 zSpread = 7;

    /// 是否可以替换现有方块
    bool canReplace = false;

    /// 是否需要水
    bool requiresWater = false;

    /// 是否投影到地面（从高度图获取Y坐标）
    bool project = true;

    GrassFeatureConfig() = default;

    /**
     * @brief 添加方块状态
     */
    void addState(const BlockState* state) { states.push_back(state); }

    /**
     * @brief 获取随机方块状态
     */
    [[nodiscard]] const BlockState* getRandomState(math::Random& random) const;
};

/**
 * @brief 草丛特征
 *
 * 在指定位置周围随机放置草、蕨类等植被。
 */
class GrassFeature {
public:
    /**
     * @brief 放置草丛特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 草丛配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const GrassFeatureConfig& config);

private:
    /**
     * @brief 检查草丛是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const GrassFeatureConfig& config) const;

    /**
     * @brief 检查下方方块是否支持草丛生长
     */
    [[nodiscard]] bool _isValidGround(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化草丛特征
 */
class ConfiguredGrassFeature : public ConfiguredFeatureBase {
public:
    ConfiguredGrassFeature(std::unique_ptr<GrassFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const GrassFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<GrassFeatureConfig> m_config;
    std::string m_name;
    // GrassFeature::place() 算法重载非 const（工具类无状态），但 ConfiguredGrassFeature::place() 语义不变
    // feature 对象本身在放置时不可变。标记 mutable 使 const override 可调用算法。
    mutable GrassFeature m_feature;
};

} // namespace mc
