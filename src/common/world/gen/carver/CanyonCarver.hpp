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

#include "CarverConfiguration.hpp"
#include "CarvingContext.hpp"
#include "WorldCarver.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <vector>

namespace mc {

/**
 * @brief 峡谷雕刻器
 *
 * 生成峡谷地形特征。峡谷是长而蜿蜒的地下峡谷，具有变化的宽度和深度。
 *
 * 使用 CanyonCarverConfiguration 进行配置，包括：
 * - probability: 生成概率
 * - y: 起始高度提供器
 * - yScale: Y轴缩放
 * - verticalRotation: 垂直旋转角度
 * - shape: 峡谷形状配置（距离因子、厚度、宽度平滑度等）
 */
class CanyonCarver : public WorldCarver<CanyonCarverConfiguration> {
public:
    explicit CanyonCarver(i32 maxHeight = world::MAX_BUILD_HEIGHT);

    ~CanyonCarver() override = default;

    bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        ChunkCoord originChunkX,
        ChunkCoord originChunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng,
        const CanyonCarverConfiguration& config) override;

    [[nodiscard]] bool shouldCarve(math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const CanyonCarverConfiguration& config) const override;

private:
    [[nodiscard]] std::vector<f32> _initWidthFactors(
        CarvingContext& context, const CanyonCarverConfiguration& config, math::IRandom& rng) const;

    [[nodiscard]] f32 _updateVerticalRadius(const CanyonCarverConfiguration& config,
        math::IRandom& rng,
        f32 baseRadius,
        f32 totalSteps,
        f32 currentStep) const;

    void _generateCanyon(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        i64 seed,
        f64 startX,
        f64 startY,
        f64 startZ,
        f32 thickness,
        f32 yaw,
        f32 pitch,
        i32 startIndex,
        i32 endIndex,
        f64 yScale,
        CarvingMask& carvingMask,
        const CanyonCarverConfiguration& config);

    [[nodiscard]] CarveSkipChecker _createSkipChecker(
        CarvingContext& context, const std::vector<f32>& heightThresholds) const;
};

} // namespace mc
