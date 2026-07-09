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

#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"

#include <memory>

namespace mc {

/**
 * @brief 海洋装饰特征配置
 *
 * 该配置用于补齐海洋可见装饰闭环：
 * - 潮涌核心
 * - 干海带块
 * - 海龟蛋
 * - 气泡柱
 * - 海晶石楼梯与台阶
 */
struct OceanDecorationFeatureConfig : public IFeatureConfig {
    const BlockState* conduitState = nullptr;
    const BlockState* driedKelpBlockState = nullptr;
    const BlockState* turtleEggState = nullptr;
    const BlockState* bubbleColumnState = nullptr;
    const BlockState* prismarineStairsState = nullptr;
    const BlockState* prismarineSlabState = nullptr;
    const BlockState* prismarineState = nullptr;
    const BlockState* magmaState = nullptr;
    const BlockState* sandState = nullptr;

    i32 tries = 2;
    i32 bubbleColumnMaxHeight = 8;
    i32 driedKelpCount = 4;
};

/**
 * @brief 海洋装饰特征
 *
 * 在海洋底部生成装饰性结构，包括：
 * - 潮涌核心框架（海晶石楼梯、台阶环绕）
 * - 干海带块
 * - 海龟蛋巢穴
 * - 气泡柱（岩浆块上方）
 */
class OceanDecorationFeature {
public:
    /**
     * @brief 在指定位置尝试生成海洋装饰
     * @param world 世界生成区域
     * @param random 随机数生成器
     * @param pos 装饰中心位置（区块内坐标）
     * @param config 装饰配置
     * @return 是否成功放置了任何装饰
     */
    bool place(
        WorldGenRegion& world, math::Random& random, const BlockPos& pos, const OceanDecorationFeatureConfig& config);

private:
    /**
     * @brief 检查指定位置是否为水
     */
    [[nodiscard]] bool _isWater(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查指定位置下方是否有固体支撑
     */
    [[nodiscard]] bool _hasSolidSupport(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 查找海洋底部Y坐标
     * @return 海洋底部Y坐标，若未找到返回-1
     */
    [[nodiscard]] i32 _findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const;

    /**
     * @brief 在单个位置生成装饰
     */
    bool _placeSingleDecoration(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& centerPos,
        const OceanDecorationFeatureConfig& config);
};

/**
 * @brief 配置化海洋装饰特征
 */
class ConfiguredOceanDecorationFeature : public ConfiguredFeatureBase {
public:
    ConfiguredOceanDecorationFeature(std::unique_ptr<OceanDecorationFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const OceanDecorationFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<OceanDecorationFeatureConfig> m_config;
    std::string m_name;
    mutable OceanDecorationFeature m_feature;
};

} // namespace mc
