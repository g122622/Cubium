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
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "server/world/gen/feature/ConfiguredFeature.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class WorldGenRegion;

/**
 * @brief 矿石特征
 *
 * 生成椭圆体状的矿脉。算法逐行对齐原版 OreFeature.doPlace：
 * 沿一条随机朝向的轴线等距布置 size 个球心，半径由正弦包络决定，
 * 再对每个球心做一次球体扫描写入。
 *
 * 全程使用 f64：原版所有中间量都是 double，仅在正弦值处取 float
 * （Mth.sin 返回 float）。用 f32 计算会让球体边界在临界格上偏移，
 * 导致矿脉形状与原版不一致。
 */
class OreFeature {
public:
    /**
     * @brief 在指定位置放置矿石
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param random 随机数生成器
     * @param origin 起始位置
     * @param config 矿石配置
     * @return 是否成功放置了任何方块
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        math::IRandom& random,
        const BlockPos& origin,
        const OreFeatureConfig& config);

    /**
     * @brief 获取特征名称
     */
    [[nodiscard]] static const char* name() { return "ore"; }

private:
    /**
     * @brief 在已确定的轴线与包围盒内放置矿脉（对齐原版 OreFeature.doPlace）
     *
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param config 矿石配置
     * @param x1 轴线起点 X
     * @param y1 轴线起点 Y
     * @param z1 轴线起点 Z
     * @param x2 轴线终点 X
     * @param y2 轴线终点 Y
     * @param z2 轴线终点 Z
     * @param minX 包围盒最小 X
     * @param minY 包围盒最小 Y
     * @param minZ 包围盒最小 Z
     * @param sizeX 包围盒 X 边长
     * @param sizeY 包围盒 Y 边长
     * @param sizeZ 包围盒 Z 边长
     * @return 实际写入的方块数
     */
    static i32 _doPlace(WorldGenRegion& region,
        math::IRandom& random,
        const OreFeatureConfig& config,
        f64 x1,
        f64 y1,
        f64 z1,
        f64 x2,
        f64 y2,
        f64 z2,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 sizeX,
        i32 sizeY,
        i32 sizeZ);

    /**
     * @brief 判断某位置是否可放置矿石（对齐原版 OreFeature.canPlaceOre）
     *
     * 先做目标规则匹配，再按 discardChanceOnAirExposure 决定是否要求"不邻接空气"。
     */
    static bool _canPlaceOre(WorldGenRegion& region,
        math::IRandom& random,
        const OreFeatureConfig& config,
        const OreTarget& target,
        i32 x,
        i32 y,
        i32 z);
};

/**
 * @brief 预配置的矿石特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置矿脉。
 * 继承 ConfiguredFeatureBase 以支持统一的特征注册。
 */
class ConfiguredOreFeature : public ConfiguredFeatureBase {
public:
    ConfiguredOreFeature(std::unique_ptr<OreFeatureConfig> featureConfig, const char* featureName = "ore");

    /**
     * @brief 在指定位置放置矿石（实现 ConfiguredFeatureBase 接口）
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::IRandom& random,
        const BlockPos& pos) const override;

    /**
     * @brief 获取特征名称
     */
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }

    /**
     * @brief 获取装饰阶段
     */
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundOres; }

    /**
     * @brief 获取矿石配置
     */
    [[nodiscard]] const OreFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<OreFeatureConfig> m_config;
    std::string m_name;
};

} // namespace mc
