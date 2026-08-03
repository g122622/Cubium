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

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc::world::gen::feature::cave {

/**
 * @brief 尖端滴石特征配置（MC PointedDripstoneConfiguration）
 *
 * 4 个 [0,1] 概率：高滴石概率、定向扩散概率、二级扩散半径概率、三级扩散半径概率。
 */
struct PointedDripstoneConfig : public IFeatureConfig {
    f32 chanceOfTallerDripstone = 0.2F;
    f32 chanceOfDirectionalSpread = 0.7F;
    f32 chanceOfSpreadRadius2 = 0.5F;
    f32 chanceOfSpreadRadius3 = 0.5F;

    PointedDripstoneConfig() = default;
    PointedDripstoneConfig(f32 taller, f32 dirSpread, f32 radius2, f32 radius3)
        : chanceOfTallerDripstone(taller)
        , chanceOfDirectionalSpread(dirSpread)
        , chanceOfSpreadRadius2(radius2)
        , chanceOfSpreadRadius3(radius3)
    {}
};

/**
 * @brief 尖端滴石特征（MC PointedDripstoneFeature）
 *
 * 在 origin 处根据上下方是否为滴石基座决定朝向，向四周扩散铺放滴水石块，
 * 并在 origin 生长 1~2 格尖端滴石。
 */
class PointedDripstoneFeature {
public:
    bool place(IWorld& world, math::Random& random, const BlockPos& pos, const PointedDripstoneConfig& config);

private:
    [[nodiscard]] static std::optional<Direction> getTipDirection(
        IWorld& world, const BlockPos& pos, math::Random& random);

    static void createPatchOfDripstoneBlocks(
        IWorld& world, math::Random& random, const BlockPos& pos, const PointedDripstoneConfig& config);
};

/**
 * @brief 配置化尖端滴石特征
 */
class ConfiguredPointedDripstoneFeature : public ConfiguredFeatureBase {
public:
    ConfiguredPointedDripstoneFeature(std::unique_ptr<PointedDripstoneConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const PointedDripstoneConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<PointedDripstoneConfig> m_config;
    std::string m_name;
    mutable PointedDripstoneFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
