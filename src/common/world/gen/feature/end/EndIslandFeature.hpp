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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

/**
 * @file EndIslandFeature.hpp
 * @brief 末地小岛特征
 *
 * 在小型末地岛屿生物群系中生成末地石岛屿。
 * 对应 MC 原版 EndIslandFeature，使用 NoneFeatureConfiguration。
 *
 * 生成锥形/泪滴形末地石岛屿：初始半径 4.0-6.0，
 * 每层向下收缩 0.5-2.5，横截面为圆形。
 */

#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <memory>
#include <vector>

namespace mc {

class WorldGenRegion;
class ChunkPrimer;
class IChunkGenerator;

/**
 * @brief 末地小岛特征
 *
 * 生成末地石构成的锥形岛屿。
 * 初始半径在 4.0-6.0 之间随机，每层向下收缩，
 * 形成泪滴状的小岛结构。
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.feature.EndIslandFeature
 */
class EndIslandFeature {
public:
    /**
     * @brief 放置末地小岛
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @return true 如果成功放置
     */
    static bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos);
};

/**
 * @brief 配置化末地小岛特征
 *
 * 包装 EndIslandFeature 和放置链，用于注册到 FeatureRegistry。
 */
class ConfiguredEndIslandFeature : public ConfiguredFeatureBase {
public:
    ConfiguredEndIslandFeature(std::unique_ptr<ConfiguredPlacement> placement, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::RawGeneration; }

private:
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 预定义末地小岛特征
 *
 * 管理末地小岛特征的初始化和注册。
 * 调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct EndIslandFeatures {
    /// 初始化末地小岛特征
    static void initialize();

    /// 获取所有末地小岛特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredEndIslandFeature>>& getAllFeatures();

    /// 获取所有末地小岛特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredEndIslandFeature>> getAllFeaturesAndClear();

    /// 创建末地小岛特征（小型末地岛屿，稀有度1/14）
    static std::unique_ptr<ConfiguredEndIslandFeature> createEndIsland();

private:
    static std::vector<std::unique_ptr<ConfiguredEndIslandFeature>> s_features;
};

} // namespace mc
