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
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <vector>

namespace mc::world::gen::structure {
class StructureBoundingBox;
}

namespace mc::world::gen::feature::cave {

/**
 * @brief 化石特征配置（MC FossilFeatureConfiguration）
 *
 * fossil_structures / overlay_structures 各 8 个结构模板 ResourceLocation；
 * fossil_processors / overlay_processors 为 StructureProcessorList 引用；
 * max_empty_corners_allowed 为空角上限（超过则放弃放置）。
 */
struct FossilConfig : public IFeatureConfig {
    std::vector<ResourceLocation> fossilStructures;
    std::vector<ResourceLocation> overlayStructures;
    ResourceLocation fossilProcessors;
    ResourceLocation overlayProcessors;
    i32 maxEmptyCornersAllowed = 0;

    FossilConfig() = default;
};

/**
 * @brief 化石特征（MC FossilFeature）
 *
 * 随机选一对 fossil/overlay 模板，按随机旋转放置到地下（取 OCEAN_FLOOR_WG
 * 最低点 - 15 - rand(10)，且不低于 minY+10）。先统计模板包围盒 8 角中
 * 空气/水/岩浆格数，超过 maxEmptyCornersAllowed 则放弃；否则依次以
 * fossilProcessors 与 overlayProcessors 放置两层模板。
 *
 * 装饰阶段为 UndergroundDecoration。
 */
class FossilFeature {
public:
    bool place(IWorld& world,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const FossilConfig& config);

private:
    [[nodiscard]] static i32 countEmptyCorners(IWorld& world, const ::mc::world::gen::structure::StructureBoundingBox& box);
};

/**
 * @brief 配置化化石特征
 */
class ConfiguredFossilFeature : public ConfiguredFeatureBase {
public:
    ConfiguredFossilFeature(std::unique_ptr<FossilConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const FossilConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<FossilConfig> m_config;
    std::string m_name;
    mutable FossilFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
