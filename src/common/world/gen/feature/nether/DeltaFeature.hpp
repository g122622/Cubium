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
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>

namespace mc {

/**
 * @brief 三角洲特征配置
 *
 * 对应 MC 1.21.11: DeltaFeatureConfiguration(contents, rim, size, rimSize)。
 * contents/rim 为方块状态；size/rimSize 为 IntProvider[0,16]。
 */
struct DeltaFeatureConfig : public IFeatureConfig {
    const BlockState* contents = nullptr; ///< 填充方块（如熔岩）
    const BlockState* rim = nullptr;      ///< 边缘方块（如岩浆块）
    std::unique_ptr<world::gen::valueprovider::IntProvider> size;
    std::unique_ptr<world::gen::valueprovider::IntProvider> rimSize;

    DeltaFeatureConfig() = default;
};

/**
 * @brief 三角洲特征
 *
 * 对应 MC 1.21.11: DeltaFeature。在下界生成熔岩池 + 岩浆块边缘的三角洲地貌。
 * 以原点为中心，在 (size,0,size) 曼哈顿菱形范围内放置 contents/rim。
 */
class DeltaFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const DeltaFeatureConfig& config);
};

/**
 * @brief 配置化三角洲特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有，本类在已确定的 pos 处放置。
 */
class ConfiguredDeltaFeature : public ConfiguredFeatureBase {
public:
    ConfiguredDeltaFeature(std::unique_ptr<DeltaFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<DeltaFeatureConfig> m_config;
    std::string m_name;
    mutable DeltaFeature m_feature;
};

} // namespace mc
