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
#include <string>
#include <utility>
#include <vector>

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/Feature.hpp"

namespace mc {

/**
 * @brief 花卉特征配置
 *
 * 字段映射：tries -> tries, xz_spread -> xzSpread, y_spread -> ySpread。
 */
struct FlowerFeatureConfig : public IFeatureConfig {
    /// 可放置的花卉方块状态列表（随机选择）
    std::vector<const BlockState*> flowers;

    /// 花卉尝试放置次数（默认 128）
    i32 tries = 64;

    /// XZ 方向扩散范围（默认 7）
    i32 xzSpread = 7;

    /// Y 方向扩散范围（默认 3，花卉预设通常使用 2）
    i32 ySpread = 3;

    /// 是否需要特定方块才能放置
    bool requiresWater = false;

    /// 白名单：允许放置的下方方块类型（空=所有）
    std::vector<const BlockState*> whitelist;

    /// 黑名单：禁止放置的下方方块状态
    std::vector<const BlockState*> blacklist;

    /// 是否可替换（允许覆盖可替换方块）
    bool isReplaceable = false;

    FlowerFeatureConfig() = default;

    explicit FlowerFeatureConfig(const BlockState* flower)
        : flowers{flower}
    {}

    FlowerFeatureConfig(std::vector<const BlockState*> flowerList, i32 attemptCount)
        : flowers(std::move(flowerList))
        , tries(attemptCount)
    {}

    /**
     * @brief 添加花卉
     */
    void addFlower(const BlockState* flower) { flowers.push_back(flower); }

    /**
     * @brief 获取随机花卉
     */
    [[nodiscard]] const BlockState* getRandomFlower(math::IRandom& random) const;
};

/**
 * @brief 花卉特征
 *
 * 在指定位置周围随机放置花卉。
 */
class FlowerFeature {
public:
    /**
     * @brief 放置花卉特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 花卉配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const FlowerFeatureConfig& config);

private:
    /**
     * @brief 检查下方方块是否支持花卉生长
     * @param world 世界区域
     * @param pos 下方方块位置
     * @param config 配置（用于白名单/黑名单检查）
     */
    [[nodiscard]] bool _isValidGround(
        WorldGenRegion& world, const BlockPos& pos, const FlowerFeatureConfig& config) const;

    /**
     * @brief 检查是否有相邻的水
     * @param world 世界区域
     * @param pos 检查位置
     * @return 四个水平方向是否有水
     */
    [[nodiscard]] bool _hasAdjacentWater(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化花卉特征
 */
class ConfiguredFlowerFeature : public ConfiguredFeatureBase {
public:
    ConfiguredFlowerFeature(std::unique_ptr<FlowerFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const FlowerFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<FlowerFeatureConfig> m_config;
    std::string m_name;
    // FlowerFeature::place() 算法重载非 const（工具类无状态），但 ConfiguredFlowerFeature::place() 语义不变
    // feature 对象本身在放置时不可变。标记 mutable 使 const override 可调用算法。
    mutable FlowerFeature m_feature;
};

} // namespace mc
