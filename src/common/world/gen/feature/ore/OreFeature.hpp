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

#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;

/**
 * @brief 矿石特征
 *
 * 生成矿脉形状的矿石。
 * 使用球形采样算法在石头中放置矿石。
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
        math::Random& random,
        const BlockPos& origin,
        const OreFeatureConfig& config);

    /**
     * @brief 获取特征名称
     */
    [[nodiscard]] static const char* name() { return "ore"; }

private:
    /**
     * @brief 在指定范围内生成球形矿石
     * @param chunk 区块数据
     * @param random 随机数生成器
     * @param config 矿石配置
     * @param x1 起点X
     * @param y1 起点Y
     * @param z1 起点Z
     * @param x2 终点X
     * @param y2 终点Y
     * @param z2 终点Z
     * @param minX 边界最小X
     * @param minY 边界最小Y
     * @param minZ 边界最小Z
     * @param sizeX 范围大小X
     * @param sizeY 范围大小Y
     * @param sizeZ 范围大小Z
     * @param placedCount 已放置计数（输出）
     */
    void _generateSphere(WorldGenRegion& region,
        math::Random& random,
        const OreFeatureConfig& config,
        f32 x1,
        f32 y1,
        f32 z1,
        f32 x2,
        f32 y2,
        f32 z2,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 sizeX,
        i32 sizeY,
        i32 sizeZ,
        i32& placedCount);
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
        math::Random& random,
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
