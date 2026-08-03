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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "CarverConfiguration.hpp"
#include "CarvingContext.hpp"
#include "WorldCarver.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/structure/Structure.hpp"

namespace mc {

/**
 * @brief 洞穴世界雕刻器
 *
 * 生成洞穴系统，继承 WorldCarver 基类。
 * 使用 CaveCarverConfiguration 配置，支持灵活的高度范围、半径乘数和地板高度设置。
 */
class CaveCarver : public WorldCarver<CaveCarverConfiguration> {
public:
    CaveCarver();

    ~CaveCarver() override = default;

    bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        ChunkCoord originChunkX,
        ChunkCoord originChunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng,
        const CaveCarverConfiguration& config) override;

    [[nodiscard]] bool shouldCarve(math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const CaveCarverConfiguration& config) const noexcept override;

protected:
    /** @brief 获取洞穴最大数量上限，默认返回 15，下界版本重写为 10 */
    [[nodiscard]] virtual i32 getCaveBound() const noexcept { return 15; }

    /** @brief 获取洞穴厚度（半径基础值） */
    [[nodiscard]] virtual f32 getThickness(math::IRandom& rng) const;

    /** @brief 获取 Y 缩放因子，默认返回 1.0，下界版本重写为其他值 */
    [[nodiscard]] virtual f64 getYScale() const noexcept { return 1.0; }

private:
    void _createTunnel(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        i64 seed,
        f64 startX,
        f64 startY,
        f64 startZ,
        f64 horizontalRadiusMultiplier,
        f64 verticalRadiusMultiplier,
        f32 thickness,
        f32 yaw,
        f32 pitch,
        i32 startIndex,
        i32 endIndex,
        f64 yScale,
        CarvingMask& carvingMask,
        const CarveSkipChecker& skipChecker,
        const CaveCarverConfiguration& config);

    void _createRoom(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        f64 centerX,
        f64 centerY,
        f64 centerZ,
        f32 radius,
        f64 yScale,
        CarvingMask& carvingMask,
        const CarveSkipChecker& skipChecker,
        const CaveCarverConfiguration& config);
};

} // namespace mc
