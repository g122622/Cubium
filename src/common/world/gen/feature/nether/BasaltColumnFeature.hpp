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
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {

/**
 * @brief 玄武岩柱特征配置
 *
 * 对应 MC 1.21.11: ColumnFeatureConfiguration(reach, height)。
 * reach: 每列周围搜索半径 IntProvider[0,3]；height: 柱高 IntProvider[1,10]。
 */
struct BasaltColumnFeatureConfig : public IFeatureConfig {
    std::unique_ptr<world::gen::valueprovider::IntProvider> reach;
    std::unique_ptr<world::gen::valueprovider::IntProvider> height;

    BasaltColumnFeatureConfig() = default;
    BasaltColumnFeatureConfig(std::unique_ptr<world::gen::valueprovider::IntProvider> r,
        std::unique_ptr<world::gen::valueprovider::IntProvider> h)
        : reach(std::move(r))
        , height(std::move(h))
    {}
};

/**
 * @brief 玄武岩柱特征
 *
 * 对应 MC 1.21.11: BasaltColumnsFeature。在下界地板/熔岩海向上生长玄武岩柱。
 * 90% 聚类模式（柱数 50，半径封顶 5），10% 非聚类模式（柱数 15，半径封顶 8）。
 */
class BasaltColumnFeature {
public:
    bool place(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 seaLevel,
        const BasaltColumnFeatureConfig& config);

private:
    /// 在单点周围 reach 半径内放置玄武岩柱（返回是否放置了任意方块）
    [[nodiscard]] bool placeColumn(
        WorldGenRegion& world, i32 seaLevel, const BlockPos& center, i32 columnHeight, i32 reach);
};

/**
 * @brief 配置化玄武岩柱特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有，本类在已确定的 pos 处放置。
 */
class ConfiguredBasaltColumnFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBasaltColumnFeature(std::unique_ptr<BasaltColumnFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const BasaltColumnFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BasaltColumnFeatureConfig> m_config;
    std::string m_name;
    mutable BasaltColumnFeature m_feature;
};

} // namespace mc
