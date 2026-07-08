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

namespace mc::world::gen::feature::cave {

/**
 * @brief 沙漠水井特征（MC DesertWellFeature，NoneFeatureConfiguration）
 *
 * 在沙地上生成 3×3 沙岩基座 + 中央水源十字 + 沙岩墙与台阶 + 顶部立柱，
 * 并在井底随机两处放置可疑沙（BrushableBlockEntity 持 archaeology/desert_well 战利品表）。
 * 装饰阶段为 SurfaceStructures。
 */
class DesertWellFeature {
public:
    bool place(IWorld& world, IChunkGenerator& generator, math::Random& random, const BlockPos& pos);
};

/**
 * @brief 配置化沙漠水井特征
 */
class ConfiguredDesertWellFeature : public ConfiguredFeatureBase {
public:
    ConfiguredDesertWellFeature(const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::SurfaceStructures; }

private:
    std::string m_name;
    mutable DesertWellFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
