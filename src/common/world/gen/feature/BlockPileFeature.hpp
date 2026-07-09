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
 * @brief block_pile 配置
 *
 * 对应 MC 1.21.11 BlockPileConfiguration{stateProvider}。仅一个 BlockStateProvider。
 */
struct BlockPileConfig {
    /// 方块状态提供者（simple / weighted / rule_based）。nullptr 表示无效配置。
    std::unique_ptr<parser::BlockStateProviderHandle> stateProvider;

    BlockPileConfig() = default;
};

/**
 * @brief 方块堆特征（block_pile）
 *
 * 忠实复刻 MC 1.21.11 BlockPileFeature：
 * - origin.y < minY+5 时 return false；
 * - i=2+nextInt(2), j=2+nextInt(2)；遍历 [origin(-i,0,-j), origin(i,1,j)] 的 AABB；
 * - 对每格：若 (dx*dx+dz*dz) <= nextFloat()*10 - nextFloat()*6 则放置；否则 nextFloat()<0.031 也放置；
 * - 仅当目标格为空且下方 isFaceSturdy(UP)（dirt_path 特例按 nextBoolean）才 setBlock。
 *
 * 装饰阶段 UndergroundDecoration。
 */
class ConfiguredBlockPileFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredBlockPileFeature(std::unique_ptr<BlockPileConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<BlockPileConfig> m_config;
    std::string m_name;
};

} // namespace world::gen::feature
} // namespace mc
