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
 */

#pragma once

#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include <memory>
#include <string>

namespace mc::world::gen::feature::cave {

/**
 * @brief 简单方块配置
 *
 * 用于放置单个方块（如孢子花、苔藓植被）。to_place 为 BlockStateProvider：
 * simple_state_provider 取单一状态；weighted_state_provider 每次放置按权重采样。
 */
struct SimpleBlockConfig {
    /// 要放置的方块状态（simple_state_provider 的单一状态；weighted 时为 nullptr）
    const BlockState* toPlace = nullptr;

    /// 加权方块状态提供者（weighted_state_provider；simple 时为 nullptr）
    std::unique_ptr<state::WeightedBlockStateProvider> weightedProvider;

    /// 是否调度刻更新
    bool scheduleTick = false;

    SimpleBlockConfig() = default;
    explicit SimpleBlockConfig(const BlockState* state, bool tick = false)
        : toPlace(state)
        , scheduleTick(tick)
    {}
};

/**
 * @brief 简单方块放置特征
 *
 * 在指定位置放置单个方块，检查canSurvive条件。
 * 用于孢子花、苔藓植被等单方块放置。
 */
class SimpleBlockFeature {
public:
    /**
     * @brief 在指定位置放置方块
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 简单方块配置
     * @return 是否成功放置
     */
    static bool place(
        WorldGenRegion& region, math::Random& random, const BlockPos& pos, const SimpleBlockConfig& config);
};

/**
 * @brief 配置化简单方块特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置方块。
 */
class ConfiguredSimpleBlockFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSimpleBlockFeature(std::unique_ptr<SimpleBlockConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<SimpleBlockConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
