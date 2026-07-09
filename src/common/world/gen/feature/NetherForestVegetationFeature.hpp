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

#include "ConfiguredFeature.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/feature/parser/BlockStateProviderParser.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief nether_forest_vegetation 配置
 *
 * 对应 MC 1.21.11 NetherForestVegetationConfig{stateProvider, spreadWidth, spreadHeight}。
 */
struct NetherForestVegetationConfig {
    std::unique_ptr<parser::BlockStateProviderHandle> stateProvider;
    i32 spreadWidth = 0;
    i32 spreadHeight = 0;

    NetherForestVegetationConfig() = default;
};

/**
 * @brief 下界森林植被特征（nether_forest_vegetation）
 *
 * 忠实复刻 MC 1.21.11 NetherForestVegetationFeature：
 * - origin 下方须为 NYLIUM 标签方块，否则 return false；
 * - 在 [minY+1, maxY-1] 范围内，做 spreadWidth^2 次尝试；
 * - 每次尝试位置 = origin + (nextInt(w)-nextInt(w), nextInt(h)-nextInt(h), nextInt(w)-nextInt(w))；
 * - 目标格为空且 y>minY 时放置 stateProvider 采样状态（canSurvive 检查省略：下方已确认 nylium，
 *   植被在 nylium 上 canSurvive 恒真）。
 *
 * 装饰阶段 VegetalDecoration。
 */
class ConfiguredNetherForestVegetationFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredNetherForestVegetationFeature(
        std::unique_ptr<NetherForestVegetationConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<NetherForestVegetationConfig> m_config;
    std::string m_name;
};

} // namespace world::gen::feature
} // namespace mc
