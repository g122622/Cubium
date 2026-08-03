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

#include "common/world/gen/surface/SurfaceRuleContext.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/surface/SurfaceCondition.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace mc::world::gen::surface {

namespace {

[[nodiscard]] i64 packXZ(i32 x, i32 z)
{
    return static_cast<i64>((static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z));
}

} // namespace

SurfaceRuleContext::SurfaceRuleContext(i32 seaLevel,
    i32 minY,
    i32 height,
    const world::gen::noise::NormalNoise* surfaceDepthNoise,
    const world::gen::noise::NormalNoise* surfaceSecondaryNoise,
    const world::gen::noise::NormalNoise* clayBandsOffsetNoise,
    const density::NoiseChunk& noiseChunk,
    const math::PositionalRandomFactory& positionalRandom,
    world::gen::RandomState* randomState,
    HeightProvider heightProvider)
    : m_seaLevel(seaLevel)
    , m_minY(minY)
    , m_height(height)
    , m_surfaceDepthNoise(surfaceDepthNoise)
    , m_surfaceSecondaryNoise(surfaceSecondaryNoise)
    , m_clayBandsOffsetNoise(clayBandsOffsetNoise)
    , m_noiseChunk(noiseChunk)
    , m_positionalRandom(positionalRandom)
    , m_randomState(randomState)
    , m_heightProvider(std::move(heightProvider))
{
    // MC: SurfaceSystem.generateBands() — 使用 fromHashOf("minecraft:clay_bands") 种子
    generateClayBands(positionalRandom);
}

void SurfaceRuleContext::updateXZ(i32 blockX, i32 blockZ)
{
    m_blockX = blockX;
    m_blockZ = blockZ;

    // MC 1.21: SurfaceRules.Context.updateXZ — 列变更使 XZ 缓存和 Y 缓存均失效
    ++m_updateCounterXZ;
    ++m_updateCounterY;

    // MC 1.21: SurfaceSystem.getSurfaceDepth(int, int)
    // (int)(noise * 2.75 + 3.0 + noiseRandom.at(x, 0, z).nextDouble() * 0.25)
    if (m_surfaceDepthNoise) {
        const f64 noiseVal = m_surfaceDepthNoise->getValue(static_cast<f64>(blockX), 0.0, static_cast<f64>(blockZ));
        auto rng = m_positionalRandom.at(blockX, 0, blockZ);
        const f64 perturbation = rng->nextDouble() * 0.25;
        m_surfaceDepth = static_cast<i32>(noiseVal * 2.75 + 3.0 + perturbation);
    } else {
        m_surfaceDepth = 3;
    }

    // 清除缓存
    m_surfaceSecondaryCached = false;
}

void SurfaceRuleContext::updateY(
    i32 stoneDepthAbove, i32 stoneDepthBelow, i32 waterHeight, i32 blockX, i32 blockY, i32 blockZ)
{
    // MC 1.21: SurfaceRules.Context.updateY — 仅 Y 缓存失效，XZ 缓存跨 Y 步复用
    ++m_updateCounterY;

    m_stoneDepthAbove = stoneDepthAbove;
    m_stoneDepthBelow = stoneDepthBelow;
    m_waterHeight = waterHeight;
    m_blockY = blockY;
    // blockX/blockZ 在 updateXZ 时已设置；原版 updateY 也接收 x/z 但本实现沿用 updateXZ 的值。
    (void)blockX;
    (void)blockZ;
}

bool SurfaceRuleContext::cachedXZ(const SurfaceCondition* self, const LazyXZCondition& cond) const
{
    // MC 1.21: SurfaceRules.LazyCondition.test() — 按 lastUpdateXZ 戳比对，命中则复用结果
    auto it = m_conditionCache.find(self);
    if (it != m_conditionCache.end() && it->second.stamp == m_updateCounterXZ) {
        return it->second.value;
    }
    const bool value = cond.compute(*this);
    m_conditionCache[self] = {m_updateCounterXZ, value};
    return value;
}

bool SurfaceRuleContext::cachedY(const SurfaceCondition* self, const LazyYCondition& cond) const
{
    auto it = m_conditionCache.find(self);
    if (it != m_conditionCache.end() && it->second.stamp == m_updateCounterY) {
        return it->second.value;
    }
    const bool value = cond.compute(*this);
    m_conditionCache[self] = {m_updateCounterY, value};
    return value;
}

f64 SurfaceRuleContext::surfaceSecondary() const
{
    if (!m_surfaceSecondaryCached) {
        m_surfaceSecondaryCached = true;
        if (m_surfaceSecondaryNoise) {
            m_surfaceSecondaryValue =
                m_surfaceSecondaryNoise->getValue(static_cast<f64>(m_blockX), 0.0, static_cast<f64>(m_blockZ));
        } else {
            m_surfaceSecondaryValue = 0.0;
        }
    }
    return m_surfaceSecondaryValue;
}

const BlockState* SurfaceRuleContext::getBand(i32 blockY) const
{
    if (m_clayBandsOffsetNoise && !m_clayBands.empty()) {
        const i32 offset = static_cast<i32>(std::round(
            m_clayBandsOffsetNoise->getValue(static_cast<f64>(m_blockX), 0.0, static_cast<f64>(m_blockZ)) * 4.0));
        const i32 index =
            ((blockY + offset) % static_cast<i32>(m_clayBands.size()) + static_cast<i32>(m_clayBands.size())) %
            static_cast<i32>(m_clayBands.size());
        return m_clayBands[static_cast<size_t>(index)];
    }
    return VanillaBlocks::TERRACOTTA ? &VanillaBlocks::TERRACOTTA->defaultState() : nullptr;
}

bool SurfaceRuleContext::abovePreliminarySurface() const
{
    return m_blockY >= _minSurfaceLevel();
}

i32 SurfaceRuleContext::_minSurfaceLevel() const
{
    // MC 1.21: SurfaceRules.Context.getMinSurfaceLevel()
    // 使用 16 方块网格（SURFACE_CELL_SIZE=16, SURFACE_CELL_BITS=4）缓存 preliminarySurfaceLevel
    const i64 currentXZ = packXZ(m_blockX, m_blockZ);
    if (m_lastMinSurfaceLevelXZ == currentXZ) {
        return m_minSurfaceLevel;
    }

    m_lastMinSurfaceLevelXZ = currentXZ;

    const i32 cellX = m_blockX >> 4; // blockCoordToSurfaceCell
    const i32 cellZ = m_blockZ >> 4;
    const i64 cellOrigin = packXZ(cellX, cellZ);
    if (m_lastPreliminarySurfaceCellOrigin != cellOrigin) {
        m_lastPreliminarySurfaceCellOrigin = cellOrigin;
        // MC 1.21: 通过 NoiseChunk.preliminarySurfaceLevel() 查询，内部有 4 方块网格缓存
        const i32 blockX0 = cellX << 4; // surfaceCellToBlockCoord
        const i32 blockZ0 = cellZ << 4;
        m_preliminarySurfaceCache[0] = m_noiseChunk.samplePreliminarySurfaceLevel(blockX0, blockZ0);
        m_preliminarySurfaceCache[1] = m_noiseChunk.samplePreliminarySurfaceLevel(blockX0 + 16, blockZ0);
        m_preliminarySurfaceCache[2] = m_noiseChunk.samplePreliminarySurfaceLevel(blockX0, blockZ0 + 16);
        m_preliminarySurfaceCache[3] = m_noiseChunk.samplePreliminarySurfaceLevel(blockX0 + 16, blockZ0 + 16);
    }

    // MC 1.21: Mth.lerp2 在 16 方块网格内双线性插值
    const f64 deltaX = static_cast<f64>(m_blockX & 15) / 16.0;
    const f64 deltaZ = static_cast<f64>(m_blockZ & 15) / 16.0;
    const f64 z0 = math::lerp(
        static_cast<f64>(m_preliminarySurfaceCache[0]), static_cast<f64>(m_preliminarySurfaceCache[1]), deltaX);
    const f64 z1 = math::lerp(
        static_cast<f64>(m_preliminarySurfaceCache[2]), static_cast<f64>(m_preliminarySurfaceCache[3]), deltaX);
    m_minSurfaceLevel = static_cast<i32>(std::floor(math::lerp(z0, z1, deltaZ))) + m_surfaceDepth - 8;
    return m_minSurfaceLevel;
}

bool SurfaceRuleContext::steep() const
{
    // MC 1.21.11: SurfaceRules.SteepMaterialCondition
    // 使用 chunk-local 坐标 clamp 到 [0,15]，比较同一轴上两个相反方向邻居的高度差
    // Z 轴: height(x, z+1) >= height(x, z-1) + 4
    // X 轴: height(x-1, z) >= height(x+1, z) + 4
    if (!m_heightProvider) {
        return false;
    }

    // 转换为 chunk-local 坐标并 clamp 到 [0, 15]
    const i32 localX = m_blockX & 15;
    const i32 localZ = m_blockZ & 15;

    // Z 轴方向：比较 z-1 和 z+1 列的高度
    const i32 zMinus = std::max(localZ - 1, 0);
    const i32 zPlus = std::min(localZ + 1, 15);
    const i32 heightZMinus = m_heightProvider(m_blockX - localX + localX, m_blockZ - localZ + zMinus);
    const i32 heightZPlus = m_heightProvider(m_blockX - localX + localX, m_blockZ - localZ + zPlus);
    if (heightZPlus >= heightZMinus + 4) {
        return true;
    }

    // X 轴方向：比较 x-1 和 x+1 列的高度
    const i32 xMinus = std::max(localX - 1, 0);
    const i32 xPlus = std::min(localX + 1, 15);
    const i32 heightXMinus = m_heightProvider(m_blockX - localX + xMinus, m_blockZ - localZ + localZ);
    const i32 heightXPlus = m_heightProvider(m_blockX - localX + xPlus, m_blockZ - localZ + localZ);
    return heightXMinus >= heightXPlus + 4;
}

bool SurfaceRuleContext::temperature() const
{
    // MC 1.21: SurfaceRules.TemperatureCondition uses Biome.coldEnoughToSnow(pos, seaLevel).
    // 使用 SurfaceRuleContext 中的 seaLevel 而非硬编码的 SEA_LEVEL
    const Biome& biome = BiomeRegistry::instance().get(m_biome);
    return biome.doesSnowGenerate(m_blockX, m_blockY, m_blockZ, m_seaLevel);
}

void SurfaceRuleContext::generateClayBands(const math::PositionalRandomFactory& random)
{
    // MC: SurfaceSystem.generateBands()
    // 使用 fromHashOf("minecraft:clay_bands") 种子
    auto rng = random.fromHashOf("minecraft:clay_bands");

    // 生成 192 个陶土带
    m_clayBands.resize(192);

    const BlockState* terracotta = VanillaBlocks::TERRACOTTA ? &VanillaBlocks::TERRACOTTA->defaultState() : nullptr;
    const BlockState* orangeTerracotta =
        VanillaBlocks::ORANGE_TERRACOTTA ? &VanillaBlocks::ORANGE_TERRACOTTA->defaultState() : nullptr;
    const BlockState* yellowTerracotta =
        VanillaBlocks::YELLOW_TERRACOTTA ? &VanillaBlocks::YELLOW_TERRACOTTA->defaultState() : nullptr;
    const BlockState* brownTerracotta =
        VanillaBlocks::BROWN_TERRACOTTA ? &VanillaBlocks::BROWN_TERRACOTTA->defaultState() : nullptr;
    const BlockState* redTerracotta =
        VanillaBlocks::RED_TERRACOTTA ? &VanillaBlocks::RED_TERRACOTTA->defaultState() : nullptr;
    const BlockState* whiteTerracotta =
        VanillaBlocks::WHITE_TERRACOTTA ? &VanillaBlocks::WHITE_TERRACOTTA->defaultState() : nullptr;
    const BlockState* lightGrayTerracotta =
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA ? &VanillaBlocks::LIGHT_GRAY_TERRACOTTA->defaultState() : nullptr;

    // 用 terracotta 填充
    for (auto& band : m_clayBands) {
        band = terracotta;
    }

    // 橙色条纹
    for (size_t k = 0; k < m_clayBands.size();) {
        k += static_cast<size_t>(rng->nextInt(5)) + 1;
        if (k < m_clayBands.size()) {
            m_clayBands[k] = orangeTerracotta;
        }
    }

    // 辅助 lambda: 生成指定颜色的条纹
    auto makeBands = [&](i32 count, const BlockState* color) {
        const i32 bandCount = rng->nextInt(10) + 6;
        for (i32 j = 0; j < bandCount; ++j) {
            i32 bandWidth = count + rng->nextInt(3);
            i32 start = rng->nextInt(static_cast<i32>(m_clayBands.size()));
            for (i32 i1 = 0; (start + i1) < static_cast<i32>(m_clayBands.size()) && i1 < bandWidth; ++i1) {
                m_clayBands[static_cast<size_t>(start + i1)] = color;
            }
        }
    };

    makeBands(1, yellowTerracotta);
    makeBands(2, brownTerracotta);
    makeBands(1, redTerracotta);

    // 白色条纹
    const i32 l = rng->nextInt(7) + 9;
    i32 i = 0;
    for (i32 j = 0; i < l && j < static_cast<i32>(m_clayBands.size()); j += rng->nextInt(16) + 4) {
        m_clayBands[static_cast<size_t>(j)] = whiteTerracotta;
        if (j - 1 > 0 && rng->nextInt(2) == 0) {
            m_clayBands[static_cast<size_t>(j - 1)] = lightGrayTerracotta;
        }
        if (j + 1 < static_cast<i32>(m_clayBands.size()) && rng->nextInt(2) == 0) {
            m_clayBands[static_cast<size_t>(j + 1)] = lightGrayTerracotta;
        }
        ++i;
    }
}

} // namespace mc::world::gen::surface
