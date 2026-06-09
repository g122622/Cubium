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

#include "common/world/gen/surface/SurfaceRules.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace mc::world::gen::surface {

namespace {

[[nodiscard]] i64 packXZ(i32 x, i32 z)
{
    return static_cast<i64>((static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z));
}

} // namespace

// ============================================================================
// VerticalAnchor
// ============================================================================

i32 VerticalAnchor::resolveY(i32 minY, i32 height) const
{
    switch (type) {
        case VerticalAnchorType::Absolute:
            return value;
        case VerticalAnchorType::AboveBottom:
            return minY + value;
        case VerticalAnchorType::BelowTop:
            return minY + height - 1 - value;
    }
    return value;
}

// ============================================================================
// SurfaceRuleContext
// ============================================================================

SurfaceRuleContext::SurfaceRuleContext(i32 seaLevel,
    i32 minY,
    i32 height,
    const world::gen::noise::NormalNoise* surfaceDepthNoise,
    const world::gen::noise::NormalNoise* surfaceSecondaryNoise,
    const world::gen::noise::NormalNoise* clayBandsOffsetNoise,
    const density::NoiseChunk& noiseChunk,
    const math::PositionalRandomFactory& positionalRandom,
    HeightProvider heightProvider)
    : m_seaLevel(seaLevel)
    , m_minY(minY)
    , m_height(height)
    , m_surfaceDepthNoise(surfaceDepthNoise)
    , m_surfaceSecondaryNoise(surfaceSecondaryNoise)
    , m_clayBandsOffsetNoise(clayBandsOffsetNoise)
    , m_noiseChunk(noiseChunk)
    , m_positionalRandom(positionalRandom)
    , m_heightProvider(std::move(heightProvider))
{
    // MC: SurfaceSystem.generateBands() — 使用 fromHashOf("minecraft:clay_bands") 种子
    generateClayBands(positionalRandom);
}

void SurfaceRuleContext::updateXZ(i32 blockX, i32 blockZ)
{
    m_blockX = blockX;
    m_blockZ = blockZ;

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
    m_stoneDepthAbove = stoneDepthAbove;
    m_stoneDepthBelow = stoneDepthBelow;
    m_waterHeight = waterHeight;
    m_blockY = blockY;
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
    // MC 1.21.11: SurfaceRules.SteepCondition
    // 比较当前列与 (x+1,z) 和 (x,z+1) 列的 WORLD_SURFACE_WG 高度差
    // 陡峭条件: height(x+1,z) - height(x,z) >= 4 || height(x,z+1) - height(x,z) >= 4
    if (!m_heightProvider) {
        return false;
    }

    const i32 currentHeight = m_heightProvider(m_blockX, m_blockZ);
    const i32 heightXPlus = m_heightProvider(m_blockX + 1, m_blockZ);
    const i32 heightZPlus = m_heightProvider(m_blockX, m_blockZ + 1);

    return (heightXPlus - currentHeight >= 4) || (heightZPlus - currentHeight >= 4);
}

bool SurfaceRuleContext::temperature() const
{
    // MC 1.21: SurfaceRules.TemperatureCondition uses Biome.coldEnoughToSnow(pos, seaLevel).
    // getTemperature 使用噪声 + 高度调整，coldEnoughToSnow 检查 temperature < 0.15F。
    const Biome& biome = BiomeRegistry::instance().get(m_biome);
    return biome.doesSnowGenerate(m_blockX, m_blockY, m_blockZ, world::SEA_LEVEL);
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

// ============================================================================
// StoneDepthCondition — MC: StoneDepthCheck
// MC 逻辑: stoneDepth <= 1 + offset + (addSurfaceDepth ? surfaceDepth : 0) + secondaryDepthRange映射
// ============================================================================

bool StoneDepthCondition::test(const SurfaceRuleContext& ctx) const
{
    const i32 stoneDepth = (m_surface == CaveSurface::Floor) ? ctx.stoneDepthAbove() : ctx.stoneDepthBelow();
    const i32 surfaceDepthOffset = m_addSurfaceDepth ? ctx.surfaceDepth() : 0;

    const i32 secondaryOffset = (m_secondaryDepthRange == 0)
        ? 0
        : static_cast<i32>(math::map(ctx.surfaceSecondary(), -1.0, 1.0, 0.0, static_cast<f64>(m_secondaryDepthRange)));

    return stoneDepth <= 1 + m_offset + surfaceDepthOffset + secondaryOffset;
}

// ============================================================================
// YCondition — MC: YConditionSource
// MC 逻辑: blockY + (addStoneDepth ? stoneDepthAbove : 0) >= anchorY + surfaceDepth * multiplier
// ============================================================================

bool YCondition::test(const SurfaceRuleContext& ctx) const
{
    const i32 anchorY = m_anchor.resolveY(ctx.minY(), ctx.height());
    const i32 y = ctx.blockY() + (m_addStoneDepth ? ctx.stoneDepthAbove() : 0);
    return y >= anchorY + ctx.surfaceDepth() * m_surfaceDepthMultiplier;
}

// ============================================================================
// WaterCondition — MC: WaterConditionSource
// MC 逻辑: waterHeight == MIN_VALUE || blockY + (addStoneDepth ? stoneDepthAbove : 0) >= waterHeight + offset +
// surfaceDepth * multiplier
// ============================================================================

bool WaterCondition::test(const SurfaceRuleContext& ctx) const
{
    if (ctx.waterHeight() == INT_MIN) {
        return true;
    }
    const i32 y = ctx.blockY() + (m_addStoneDepth ? ctx.stoneDepthAbove() : 0);
    return y >= ctx.waterHeight() + m_offset + ctx.surfaceDepth() * m_surfaceDepthMultiplier;
}

// ============================================================================
// BiomeCondition
// ============================================================================

bool BiomeCondition::test(const SurfaceRuleContext& ctx) const
{
    return std::find(m_biomes.begin(), m_biomes.end(), ctx.biome()) != m_biomes.end();
}

// ============================================================================
// NoiseThresholdCondition — MC: NoiseThresholdConditionSource
// ============================================================================

bool NoiseThresholdCondition::test(const SurfaceRuleContext& ctx) const
{
    const f64 value = m_noise.getValue(static_cast<f64>(ctx.blockX()), 0.0, static_cast<f64>(ctx.blockZ()));
    return value >= m_minThreshold && value <= m_maxThreshold;
}

// ============================================================================
// VerticalGradientCondition — MC: VerticalGradientConditionSource
// 用于基岩层等（随机梯度过渡）
// ============================================================================

bool VerticalGradientCondition::test(const SurfaceRuleContext& ctx) const
{
    const i32 trueY = m_trueAtAndBelow.resolveY(ctx.minY(), ctx.height());
    const i32 falseY = m_falseAtAndAbove.resolveY(ctx.minY(), ctx.height());
    const i32 blockY = ctx.blockY();

    if (blockY <= trueY) {
        return true;
    }
    if (blockY >= falseY) {
        return false;
    }

    // 在过渡区间内：使用确定性随机（基于位置的伪随机）
    const f64 t = math::map(static_cast<f64>(blockY), static_cast<f64>(trueY), static_cast<f64>(falseY), 1.0, 0.0);
    // 使用基于位置的简单哈希代替 RandomSource
    const u64 hash = static_cast<u64>(ctx.blockX()) * 341873128712ULL + static_cast<u64>(blockY) * 132897987541ULL +
        static_cast<u64>(ctx.blockZ()) * 7919 + m_seed;
    const f64 rand = static_cast<f64>((hash >> 32) & 0xFFFF) / 65536.0;
    return rand < t;
}

// ============================================================================
// SteepCondition, TemperatureCondition, HoleCondition, AbovePreliminarySurfaceCondition
// ============================================================================

bool SteepCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.steep();
}

bool TemperatureCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.temperature();
}

bool HoleCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.hole();
}

bool AbovePreliminarySurfaceCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.abovePreliminarySurface();
}

// ============================================================================
// BandlandsRule — MC: Bandlands（Badlands 陶土带）
// ============================================================================

const BlockState* BandlandsRule::tryApply(i32, i32 blockY, i32, const SurfaceRuleContext& ctx) const
{
    return ctx.getBand(blockY);
}

// ============================================================================
// SurfaceRules 工厂函数
// ============================================================================

namespace SurfaceRules {

std::unique_ptr<SurfaceCondition> onFloor()
{
    return std::make_unique<StoneDepthCondition>(0, false, 0, CaveSurface::Floor);
}

std::unique_ptr<SurfaceCondition> underFloor()
{
    return std::make_unique<StoneDepthCondition>(0, true, 0, CaveSurface::Floor);
}

std::unique_ptr<SurfaceCondition> deepUnderFloor()
{
    return std::make_unique<StoneDepthCondition>(0, true, 6, CaveSurface::Floor);
}

std::unique_ptr<SurfaceCondition> veryDeepUnderFloor()
{
    return std::make_unique<StoneDepthCondition>(0, true, 30, CaveSurface::Floor);
}

std::unique_ptr<SurfaceCondition> onCeiling()
{
    return std::make_unique<StoneDepthCondition>(0, false, 0, CaveSurface::Ceiling);
}

std::unique_ptr<SurfaceCondition> underCeiling()
{
    return std::make_unique<StoneDepthCondition>(0, true, 0, CaveSurface::Ceiling);
}

std::unique_ptr<SurfaceCondition> stoneDepthCheck(
    i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface)
{
    return std::make_unique<StoneDepthCondition>(offset, addSurfaceDepth, secondaryDepthRange, surface);
}

std::unique_ptr<SurfaceCondition> yBlockCheck(VerticalAnchor anchor, i32 surfaceDepthMultiplier)
{
    return std::make_unique<YCondition>(anchor, surfaceDepthMultiplier, false);
}

std::unique_ptr<SurfaceCondition> yStartCheck(VerticalAnchor anchor, i32 surfaceDepthMultiplier)
{
    return std::make_unique<YCondition>(anchor, -surfaceDepthMultiplier, true);
}

std::unique_ptr<SurfaceCondition> waterBlockCheck(i32 offset, i32 surfaceDepthMultiplier)
{
    return std::make_unique<WaterCondition>(offset, surfaceDepthMultiplier, false);
}

std::unique_ptr<SurfaceCondition> waterStartCheck(i32 offset, i32 surfaceDepthMultiplier)
{
    return std::make_unique<WaterCondition>(offset, -surfaceDepthMultiplier, true);
}

std::unique_ptr<SurfaceCondition> isBiome(std::vector<BiomeId> biomes)
{
    return std::make_unique<BiomeCondition>(std::move(biomes));
}

std::unique_ptr<SurfaceCondition> notCondition(std::unique_ptr<SurfaceCondition> condition)
{
    return std::make_unique<NotCondition>(std::move(condition));
}

std::unique_ptr<SurfaceCondition> noiseCondition(
    u64 seed, i32 firstOctave, std::vector<f64> amplitudes, f64 minThreshold, f64 maxThreshold)
{
    return std::make_unique<NoiseThresholdCondition>(
        seed, firstOctave, std::move(amplitudes), minThreshold, maxThreshold);
}

std::unique_ptr<SurfaceCondition> verticalGradient(
    u64 seed, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove)
{
    return std::make_unique<VerticalGradientCondition>(seed, trueAtAndBelow, falseAtAndAbove);
}

std::unique_ptr<SurfaceCondition> steep()
{
    return std::make_unique<SteepCondition>();
}

std::unique_ptr<SurfaceCondition> temperature()
{
    return std::make_unique<TemperatureCondition>();
}

std::unique_ptr<SurfaceCondition> hole()
{
    return std::make_unique<HoleCondition>();
}

std::unique_ptr<SurfaceCondition> abovePreliminarySurface()
{
    return std::make_unique<AbovePreliminarySurfaceCondition>();
}

std::unique_ptr<SurfaceRule> blockState(const BlockState* state)
{
    return std::make_unique<BlockRule>(state);
}

std::unique_ptr<SurfaceRule> ifTrue(std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule)
{
    return std::make_unique<IfTrueRule>(std::move(condition), std::move(thenRule));
}

std::unique_ptr<SurfaceRule> sequence(std::vector<std::unique_ptr<SurfaceRule>> rules)
{
    return std::make_unique<SequenceRule>(std::move(rules));
}

std::unique_ptr<SurfaceRule> bandlands()
{
    return std::make_unique<BandlandsRule>();
}

// ============================================================================
// surfaceNoiseAbove — MC: SurfaceRuleData.surfaceNoiseAbove
// 噪声阈值条件，检查 SURFACE 噪声值是否 >= threshold/8.25
// ============================================================================

[[maybe_unused]] static std::unique_ptr<SurfaceCondition> surfaceNoiseAbove(u64 seed, f64 threshold)
{
    // MC: noiseCondition(Noises.SURFACE, threshold / 8.25, Double.MAX_VALUE)
    // SURFACE 噪声参数: firstOctave=-3, amplitudes=[1, 1, 0, 1]
    return noiseCondition(seed, -3, {1.0, 1.0, 0.0, 1.0}, threshold / 8.25, 1e30);
}

// ============================================================================
// 主世界表面规则 — MC 1.21 SurfaceRuleData.overworld()
// ============================================================================

std::unique_ptr<SurfaceRule> overworld(u64 seed)
{
    // 方块状态快捷获取
    const BlockState* stone = VanillaBlocks::STONE ? &VanillaBlocks::STONE->defaultState() : nullptr;
    const BlockState* deepslate = block_registry::DeepslateBlocks::DEEPSLATE
        ? &block_registry::DeepslateBlocks::DEEPSLATE->defaultState()
        : nullptr;
    const BlockState* grass = VanillaBlocks::GRASS_BLOCK ? &VanillaBlocks::GRASS_BLOCK->defaultState() : nullptr;
    const BlockState* dirt = VanillaBlocks::DIRT ? &VanillaBlocks::DIRT->defaultState() : nullptr;
    const BlockState* water = VanillaBlocks::WATER ? &VanillaBlocks::WATER->defaultState() : nullptr;
    const BlockState* sand = VanillaBlocks::SAND ? &VanillaBlocks::SAND->defaultState() : nullptr;
    const BlockState* gravel = VanillaBlocks::GRAVEL ? &VanillaBlocks::GRAVEL->defaultState() : nullptr;
    const BlockState* bedrock = VanillaBlocks::BEDROCK ? &VanillaBlocks::BEDROCK->defaultState() : nullptr;
    const BlockState* ice = VanillaBlocks::ICE ? &VanillaBlocks::ICE->defaultState() : nullptr;
    const BlockState* sandstone = VanillaBlocks::SANDSTONE ? &VanillaBlocks::SANDSTONE->defaultState() : nullptr;
    const BlockState* coarseDirt = VanillaBlocks::COARSE_DIRT ? &VanillaBlocks::COARSE_DIRT->defaultState() : nullptr;
    const BlockState* podzol = VanillaBlocks::PODZOL ? &VanillaBlocks::PODZOL->defaultState() : nullptr;
    const BlockState* mycelium = VanillaBlocks::MYCELIUM ? &VanillaBlocks::MYCELIUM->defaultState() : nullptr;
    const BlockState* air = VanillaBlocks::AIR ? &VanillaBlocks::AIR->defaultState() : nullptr;
    const BlockState* calcite = VanillaBlocks::CALCITE ? &VanillaBlocks::CALCITE->defaultState() : nullptr;
    const BlockState* packedIce = VanillaBlocks::PACKED_ICE ? &VanillaBlocks::PACKED_ICE->defaultState() : nullptr;
    const BlockState* snowBlock = VanillaBlocks::SNOW_BLOCK ? &VanillaBlocks::SNOW_BLOCK->defaultState() : nullptr;
    const BlockState* powderSnow = VanillaBlocks::POWDER_SNOW ? &VanillaBlocks::POWDER_SNOW->defaultState() : nullptr;
    const BlockState* mud = VanillaBlocks::MUD ? &VanillaBlocks::MUD->defaultState() : nullptr;
    const BlockState* redSand = VanillaBlocks::RED_SAND ? &VanillaBlocks::RED_SAND->defaultState() : nullptr;
    const BlockState* redSandstone =
        VanillaBlocks::RED_SANDSTONE ? &VanillaBlocks::RED_SANDSTONE->defaultState() : nullptr;
    const BlockState* orangeTerracotta =
        VanillaBlocks::ORANGE_TERRACOTTA ? &VanillaBlocks::ORANGE_TERRACOTTA->defaultState() : nullptr;
    const BlockState* terracotta = VanillaBlocks::TERRACOTTA ? &VanillaBlocks::TERRACOTTA->defaultState() : nullptr;
    const BlockState* whiteTerracotta =
        VanillaBlocks::WHITE_TERRACOTTA ? &VanillaBlocks::WHITE_TERRACOTTA->defaultState() : nullptr;

    // ========== 构建主规则树 ==========
    // MC 1.21 SurfaceRuleData.overworld() — 完整规则树
    std::vector<std::unique_ptr<SurfaceRule>> rules;

    // 1. 基岩层底部 (Y 0-4 渐变)
    if (bedrock) {
        rules.push_back(ifTrue(
            verticalGradient(seed ^ 0xBEDB0001ULL, VerticalAnchor::aboveBottom(0), VerticalAnchor::aboveBottom(5)),
            blockState(bedrock)));
    }

    // 2. 深板岩层 (Y 0-8 渐变过渡)
    // MC 1.21: verticalGradient("deepslate", absolute(0), absolute(8)) → Y<=0 完全深板岩, Y=0~8 渐变
    if (deepslate) {
        rules.push_back(
            ifTrue(verticalGradient(seed ^ 0xDEE00001ULL, VerticalAnchor::absolute(0), VerticalAnchor::absolute(8)),
                blockState(deepslate)));
    }

    // 3. ON_FLOOR: 恶地树林高层（Y>=97+surfaceDepth*2 时粗糙泥土/草/泥土）
    if (coarseDirt && grass && dirt) {
        rules.push_back(ifTrue(isBiome({Biomes::WoodedBadlands}),
            ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(97), 2),
                    sequence(ifTrue(noiseCondition(
                                        seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.909 / 8.25, -0.5454 / 8.25),
                                 blockState(coarseDirt)),
                        ifTrue(noiseCondition(
                                   seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.1818 / 8.25, 0.1818 / 8.25),
                            blockState(coarseDirt)),
                        ifTrue(
                            noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.5454 / 8.25, 0.909 / 8.25),
                            blockState(coarseDirt)),
                        ifTrue(waterBlockCheck(0, 0), blockState(grass)),
                        blockState(dirt))))));
    }

    // 4. ON_FLOOR: 沼泽水面（Y>=62 且 Y<63，噪声触发时放水）
    if (water) {
        rules.push_back(ifTrue(isBiome({Biomes::Swamp}),
            ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(62), 0),
                    ifTrue(yStartCheck(VerticalAnchor::absolute(63), -1),
                        ifTrue(noiseCondition(seed ^ 0x5EA00001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.0, 1e30),
                            blockState(water)))))));
    }

    // 5. ON_FLOOR: 红树林沼泽水面（Y>=60 且 Y<63）
    if (water) {
        rules.push_back(ifTrue(isBiome({Biomes::MangroveSwamp}),
            ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(60), 0),
                    ifTrue(yStartCheck(VerticalAnchor::absolute(63), -1),
                        ifTrue(noiseCondition(seed ^ 0x5EA00001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.0, 1e30),
                            blockState(water)))))));
    }

    // 6. 恶地家族 ON_FLOOR/UNDER_FLOOR/VERY_DEEP_UNDER_FLOOR
    if (orangeTerracotta && redSand && whiteTerracotta && gravel && stone) {
        rules.push_back(ifTrue(isBiome({Biomes::Badlands, Biomes::ErodedBadlands, Biomes::WoodedBadlands}),
            sequence(
                // ON_FLOOR
                ifTrue(onFloor(),
                    sequence(
                        // Y >= 256: 橙色陶土
                        ifTrue(yBlockCheck(VerticalAnchor::absolute(256), 0), blockState(orangeTerracotta)),
                        // Y <= 74+surfaceDepth: 噪声陶土带
                        ifTrue(yStartCheck(VerticalAnchor::absolute(74), 1),
                            sequence(
                                ifTrue(
                                    noiseCondition(
                                        seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.909 / 8.25, -0.5454 / 8.25),
                                    blockState(orangeTerracotta)),
                                ifTrue(
                                    noiseCondition(
                                        seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.1818 / 8.25, 0.1818 / 8.25),
                                    blockState(orangeTerracotta)),
                                ifTrue(noiseCondition(
                                           seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 0.5454 / 8.25, 0.909 / 8.25),
                                    blockState(orangeTerracotta)),
                                bandlands())),
                        // !hole && waterCheck(-1): 红沙（含红砂岩天花板）
                        ifTrue(waterBlockCheck(-1, 0),
                            sequence(ifTrue(onCeiling(), blockState(redSandstone)), blockState(redSand))),
                        // !hole: 陶土（MC 1.21.11 使用 TERRACOTTA）
                        ifTrue(notCondition(hole()), blockState(terracotta)),
                        // waterStartCheck(-6, -1): 白色陶土
                        ifTrue(waterStartCheck(-6, -1), blockState(whiteTerracotta)),
                        // 兜底: 砾石/石头
                        sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel)))),
                // UNDER_FLOOR: Y <= 63-surfaceDepth
                ifTrue(underFloor(),
                    ifTrue(yStartCheck(VerticalAnchor::absolute(63), -1),
                        sequence(
                            // Y >= 63 且不在 badlands 范围: 橙色陶土
                            ifTrue(yBlockCheck(VerticalAnchor::absolute(63), 0),
                                ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(74), 1)),
                                    blockState(orangeTerracotta))),
                            // bandlands
                            bandlands()))),
                // VERY_DEEP_UNDER_FLOOR: waterStartCheck → 白色陶土
                ifTrue(veryDeepUnderFloor(), ifTrue(waterStartCheck(-6, -1), blockState(whiteTerracotta))))));
    }

    // 7. 冰冻海洋 hole 处理（水面上方放置空气/冰/水）
    if (water && ice && air) {
        rules.push_back(ifTrue(isBiome({Biomes::FrozenOcean, Biomes::DeepFrozenOcean}),
            ifTrue(waterBlockCheck(-1, 0),
                ifTrue(hole(),
                    sequence(ifTrue(onFloor(), blockState(air)),
                        ifTrue(temperature(), blockState(ice)),
                        blockState(water))))));
    }

    // 8. 水旁 onFloor: 冰冻海洋 hole → 水
    if (water) {
        rules.push_back(ifTrue(isBiome({Biomes::FrozenOcean, Biomes::DeepFrozenOcean}),
            ifTrue(waterStartCheck(-6, -1), ifTrue(onFloor(), ifTrue(hole(), blockState(water))))));
    }

    // 9. 水旁 underFloor: 地表材料层
    {
        std::vector<std::unique_ptr<SurfaceRule>> matRules;

        // 冰冻峰: steep→PACKED_ICE, noise→PACKED_ICE/ICE, !water→SNOW_BLOCK
        // MC 1.21.11: PACKED_ICE 噪声范围 [-0.5, 0.2], ICE 噪声范围 [-0.0625, 0.025]
        if (packedIce && ice && snowBlock) {
            matRules.push_back(ifTrue(isBiome({Biomes::FrozenPeaks}),
                sequence(ifTrue(steep(), blockState(packedIce)),
                    ifTrue(noiseCondition(seed ^ 0xAC100001ULL, -4, {1.0, 1.0}, -0.5, 0.2), blockState(packedIce)),
                    ifTrue(noiseCondition(seed ^ 0x1CE00001ULL, -4, {1.0, 1.0}, -0.0625, 0.025), blockState(ice)),
                    ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)))));
        }

        // 雪山斜坡: steep→STONE, powderSnow, !water→SNOW_BLOCK
        if (stone && snowBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> snowySlopesSeq;
            snowySlopesSeq.push_back(ifTrue(steep(), blockState(stone)));
            if (powderSnow) {
                snowySlopesSeq.push_back(
                    ifTrue(noiseCondition(seed ^ 0xB0D00001ULL, -5, {1.0, 1.0, 1.0, 1.0}, 0.45, 0.58),
                        ifTrue(waterBlockCheck(0, 0), blockState(powderSnow))));
            }
            snowySlopesSeq.push_back(ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)));
            matRules.push_back(ifTrue(isBiome({Biomes::SnowySlopes}), sequence(std::move(snowySlopesSeq))));
        }

        // 尖峭山峰: STONE
        if (stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::JaggedPeaks}), blockState(stone)));
        }

        // 树林: powderSnow, DIRT
        if (dirt) {
            std::vector<std::unique_ptr<SurfaceRule>> groveSeq;
            if (powderSnow) {
                groveSeq.push_back(ifTrue(noiseCondition(seed ^ 0xB0D00001ULL, -5, {1.0, 1.0, 1.0, 1.0}, 0.45, 0.58),
                    ifTrue(waterBlockCheck(0, 0), blockState(powderSnow))));
            }
            groveSeq.push_back(blockState(dirt));
            matRules.push_back(ifTrue(isBiome({Biomes::Grove}), sequence(std::move(groveSeq))));
        }

        // 石峰: calcite noise → CALCITE, else STONE
        if (calcite && stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::StonyPeaks}),
                sequence(
                    ifTrue(noiseCondition(seed ^ 0xCA1C0001ULL, -4, {1.0, 1.0}, -0.0125, 0.0125), blockState(calcite)),
                    blockState(stone))));
        }

        // 石岸: gravel noise → gravel/stone, else STONE
        if (gravel && stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::StonyShore}),
                sequence(ifTrue(noiseCondition(seed ^ 0x6AA50001ULL, -4, {1.0, 1.0}, -0.05, 0.05),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    blockState(stone))));
        }

        // 风蚀丘陵: surfaceNoiseAbove(1.0) → STONE
        if (stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::WindsweptHills}),
                ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.0 / 8.25, 1e30),
                    blockState(stone))));
        }

        // 温暖海洋/海滩/雪岸: sand/sandstone
        if (sand && sandstone) {
            matRules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::Beach, Biomes::SnowyBeach}),
                sequence(ifTrue(onCeiling(), blockState(sandstone)), blockState(sand))));
        }

        // 沙漠: sand/sandstone
        if (sand && sandstone) {
            matRules.push_back(ifTrue(
                isBiome({Biomes::Desert}), sequence(ifTrue(onCeiling(), blockState(sandstone)), blockState(sand))));
        }

        // 钟乳石洞: STONE
        if (stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::DripstoneCaves}), blockState(stone)));
        }

        // 风蚀萨凡纳: surfaceNoiseAbove(1.75) → STONE
        if (stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::WindsweptSavanna}),
                ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.75 / 8.25, 1e30),
                    blockState(stone))));
        }

        // 风蚀砾石丘陵: noise分层 → gravel/stone/dirt/gravel
        if (gravel && stone && dirt && grass) {
            matRules.push_back(ifTrue(isBiome({Biomes::WindsweptGravellyHills}),
                sequence(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 2.0 / 8.25, 1e30),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.0 / 8.25, 1e30),
                        blockState(stone)),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -1.0 / 8.25, 1e30),
                        sequence(ifTrue(waterBlockCheck(0, 0), blockState(grass)), blockState(dirt))),
                    sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel)))));
        }

        // 红树林沼泽: MUD
        if (mud) {
            matRules.push_back(ifTrue(isBiome({Biomes::MangroveSwamp}), blockState(mud)));
        }

        // 兜底: DIRT
        if (dirt) {
            matRules.push_back(blockState(dirt));
        }

        if (!matRules.empty()) {
            rules.push_back(ifTrue(waterStartCheck(-6, -1), ifTrue(underFloor(), sequence(std::move(matRules)))));
        }
    }

    // 10. 水旁 deepUnderFloor: 温暖海洋/海滩 → sandstone
    if (sandstone) {
        rules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::Beach, Biomes::SnowyBeach}),
            ifTrue(waterStartCheck(-6, -1), ifTrue(deepUnderFloor(), blockState(sandstone)))));
    }

    // 11. 水旁 veryDeepUnderFloor: 沙漠 → sandstone
    if (sandstone) {
        rules.push_back(ifTrue(isBiome({Biomes::Desert}),
            ifTrue(waterStartCheck(-6, -1), ifTrue(veryDeepUnderFloor(), blockState(sandstone)))));
    }

    // 12. ON_FLOOR: 尖峭山峰 → STONE（冰冻峰由规则15的详细packed_ice/ice/snow_block规则处理）
    if (stone) {
        rules.push_back(ifTrue(isBiome({Biomes::JaggedPeaks}), ifTrue(onFloor(), blockState(stone))));
    }

    // 13. ON_FLOOR: 温暖海洋/温水海洋 → sand/sandstone
    if (sand && sandstone) {
        rules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::LukewarmOcean, Biomes::DeepLukewarmOcean}),
            ifTrue(onFloor(), sequence(ifTrue(onCeiling(), blockState(sandstone)), blockState(sand)))));
    }

    // 14. ON_FLOOR: 默认 → gravel/stone
    if (gravel && stone) {
        rules.push_back(ifTrue(onFloor(), sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))));
    }

    // 15. waterBlockCheck + ON_FLOOR: 地表顶层
    {
        std::vector<std::unique_ptr<SurfaceRule>> topRules;

        // 冰冻峰 ON_FLOOR: steep→PACKED_ICE, noise→PACKED_ICE/ICE, !water→SNOW_BLOCK
        // MC 1.21.11 ON_FLOOR: PACKED_ICE 噪声范围 [0.0, 0.2], ICE 噪声范围 [0.0, 0.025]
        // 注意：UNDER_FLOOR 使用 [-0.5, 0.2] 和 [-0.0625, 0.025]，ON_FLOOR 不同
        if (packedIce && ice && snowBlock) {
            topRules.push_back(ifTrue(isBiome({Biomes::FrozenPeaks}),
                sequence(ifTrue(steep(), blockState(packedIce)),
                    ifTrue(noiseCondition(seed ^ 0xAC100001ULL, -4, {1.0, 1.0}, 0.0, 0.2), blockState(packedIce)),
                    ifTrue(noiseCondition(seed ^ 0x1CE00001ULL, -4, {1.0, 1.0}, 0.0, 0.025), blockState(ice)),
                    ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)))));
        }

        // 雪山斜坡: steep→STONE, powderSnow(高阈值), !water→SNOW_BLOCK
        if (stone && snowBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> snowySlopesSeq;
            snowySlopesSeq.push_back(ifTrue(steep(), blockState(stone)));
            if (powderSnow) {
                snowySlopesSeq.push_back(
                    ifTrue(noiseCondition(seed ^ 0xB0D00002ULL, -5, {1.0, 1.0, 1.0, 1.0}, 0.35, 0.6),
                        ifTrue(waterBlockCheck(0, 0), blockState(powderSnow))));
            }
            snowySlopesSeq.push_back(ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)));
            topRules.push_back(ifTrue(isBiome({Biomes::SnowySlopes}), sequence(std::move(snowySlopesSeq))));
        }

        // 尖峭山峰: steep→STONE, !water→SNOW_BLOCK
        if (stone && snowBlock) {
            topRules.push_back(ifTrue(isBiome({Biomes::JaggedPeaks}),
                sequence(ifTrue(steep(), blockState(stone)), ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)))));
        }

        // 树林: powderSnow(高阈值), !water→SNOW_BLOCK
        if (snowBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> groveSeq;
            if (powderSnow) {
                groveSeq.push_back(ifTrue(noiseCondition(seed ^ 0xB0D00002ULL, -5, {1.0, 1.0, 1.0, 1.0}, 0.35, 0.6),
                    ifTrue(waterBlockCheck(0, 0), blockState(powderSnow))));
            }
            groveSeq.push_back(ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)));
            topRules.push_back(ifTrue(isBiome({Biomes::Grove}), sequence(std::move(groveSeq))));
        }

        // 石峰: calcite noise → CALCITE, else STONE
        if (calcite && stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::StonyPeaks}),
                sequence(
                    ifTrue(noiseCondition(seed ^ 0xCA1C0001ULL, -4, {1.0, 1.0}, -0.0125, 0.0125), blockState(calcite)),
                    blockState(stone))));
        }

        // 石岸: gravel noise → gravel/stone, else STONE
        if (gravel && stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::StonyShore}),
                sequence(ifTrue(noiseCondition(seed ^ 0x6AA50001ULL, -4, {1.0, 1.0}, -0.05, 0.05),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    blockState(stone))));
        }

        // 风蚀丘陵: surfaceNoiseAbove(1.0) → STONE
        if (stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::WindsweptHills}),
                ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.0 / 8.25, 1e30),
                    blockState(stone))));
        }

        // 温暖海洋/海滩/雪岸: sand/sandstone
        if (sand && sandstone) {
            topRules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::Beach, Biomes::SnowyBeach}),
                sequence(ifTrue(onCeiling(), blockState(sandstone)), blockState(sand))));
        }

        // 沙漠: sand/sandstone
        if (sand && sandstone) {
            topRules.push_back(ifTrue(
                isBiome({Biomes::Desert}), sequence(ifTrue(onCeiling(), blockState(sandstone)), blockState(sand))));
        }

        // 钟乳石洞: STONE
        if (stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::DripstoneCaves}), blockState(stone)));
        }

        // 风蚀萨凡纳: noise→STONE or COARSE_DIRT
        if (stone && coarseDirt) {
            topRules.push_back(ifTrue(isBiome({Biomes::WindsweptSavanna}),
                sequence(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.75 / 8.25, 1e30),
                             blockState(stone)),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.5 / 8.25, 1e30),
                        blockState(coarseDirt)))));
        }

        // 风蚀砾石丘陵: noise分层
        if (gravel && stone && grass && dirt) {
            topRules.push_back(ifTrue(isBiome({Biomes::WindsweptGravellyHills}),
                sequence(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 2.0 / 8.25, 1e30),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.0 / 8.25, 1e30),
                        blockState(stone)),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -1.0 / 8.25, 1e30),
                        sequence(ifTrue(waterBlockCheck(0, 0), blockState(grass)), blockState(dirt))),
                    sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel)))));
        }

        // 大型针叶林: noise→coarseDirt/podzol
        if (podzol && coarseDirt) {
            topRules.push_back(ifTrue(isBiome({Biomes::OldGrowthPineTaiga, Biomes::OldGrowthSpruceTaiga}),
                sequence(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.75 / 8.25, 1e30),
                             blockState(coarseDirt)),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.95 / 8.25, 1e30),
                        blockState(podzol)))));
        }

        // 冰刺平原: !water→SNOW_BLOCK
        if (snowBlock) {
            topRules.push_back(
                ifTrue(isBiome({Biomes::IceSpikes}), ifTrue(waterBlockCheck(0, 0), blockState(snowBlock))));
        }

        // 红树林沼泽: MUD
        if (mud) {
            topRules.push_back(ifTrue(isBiome({Biomes::MangroveSwamp}), blockState(mud)));
        }

        // 蘑菇岛: MYCELIUM
        if (mycelium) {
            topRules.push_back(ifTrue(isBiome({Biomes::MushroomFields}), ifTrue(onFloor(), blockState(mycelium))));
        }

        // 默认: grass(water)/dirt
        if (grass && dirt) {
            topRules.push_back(ifTrue(waterBlockCheck(0, 0), blockState(grass)));
            topRules.push_back(blockState(dirt));
        }

        if (!topRules.empty()) {
            rules.push_back(ifTrue(onFloor(), ifTrue(waterBlockCheck(-1, 0), sequence(std::move(topRules)))));
        }
    }

    // 16. UNDER_FLOOR: grass(water)/dirt
    if (grass && dirt) {
        rules.push_back(ifTrue(underFloor(), ifTrue(waterBlockCheck(0, 0), blockState(grass))));
        rules.push_back(ifTrue(underFloor(), blockState(dirt)));
    }

    // 17. UNDER_FLOOR: 大型针叶林 coarseDirt/podzol
    if (podzol && coarseDirt) {
        rules.push_back(ifTrue(isBiome({Biomes::OldGrowthPineTaiga, Biomes::OldGrowthSpruceTaiga}),
            ifTrue(underFloor(),
                sequence(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.75 / 8.25, 1e30),
                             blockState(coarseDirt)),
                    ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.95 / 8.25, 1e30),
                        blockState(podzol))))));
    }

    // MC 1.21.11: 主世界规则树被 abovePreliminarySurface() 包裹
    // 确保只有初步表面以上的区域才应用地表规则
    return ifTrue(abovePreliminarySurface(), sequence(std::move(rules)));
}
// ============================================================================

std::unique_ptr<SurfaceRule> nether(u64 seed)
{
    const BlockState* bedrock = VanillaBlocks::BEDROCK ? &VanillaBlocks::BEDROCK->defaultState() : nullptr;
    const BlockState* netherrack = VanillaBlocks::NETHERRACK ? &VanillaBlocks::NETHERRACK->defaultState() : nullptr;
    const BlockState* soulSand = VanillaBlocks::SOUL_SAND ? &VanillaBlocks::SOUL_SAND->defaultState() : nullptr;
    const BlockState* soulSoil = VanillaBlocks::SOUL_SOIL ? &VanillaBlocks::SOUL_SOIL->defaultState() : nullptr;
    const BlockState* basalt = VanillaBlocks::BASALT ? &VanillaBlocks::BASALT->defaultState() : nullptr;
    const BlockState* blackstone = VanillaBlocks::BLACKSTONE ? &VanillaBlocks::BLACKSTONE->defaultState() : nullptr;
    const BlockState* lava = VanillaBlocks::LAVA ? &VanillaBlocks::LAVA->defaultState() : nullptr;
    const BlockState* warpedWartBlock =
        VanillaBlocks::WARPED_WART_BLOCK ? &VanillaBlocks::WARPED_WART_BLOCK->defaultState() : nullptr;
    const BlockState* warpedNylium =
        VanillaBlocks::WARPED_NYLIUM ? &VanillaBlocks::WARPED_NYLIUM->defaultState() : nullptr;
    const BlockState* netherWartBlock =
        VanillaBlocks::NETHER_WART_BLOCK ? &VanillaBlocks::NETHER_WART_BLOCK->defaultState() : nullptr;
    const BlockState* crimsonNylium =
        VanillaBlocks::CRIMSON_NYLIUM ? &VanillaBlocks::CRIMSON_NYLIUM->defaultState() : nullptr;
    const BlockState* gravel = VanillaBlocks::GRAVEL ? &VanillaBlocks::GRAVEL->defaultState() : nullptr;

    // MC 1.21.11 下界噪声参数（来自 NoiseData.java）
    // NETHER_STATE_SELECTOR: firstOctave=-4, amplitudes=[1.0]
    auto netherStateSelector = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(seed ^ 0x5A5A0001ULL, -4, {1.0}, 0.0);
    };
    // PATCH: firstOctave=-5, amplitudes=[1.0, 0.0, 0.0, 0.0, 0.0, 0.01333...]
    // 简化为 [1.0, 0.0, 0.0, 0.0, 0.0, 0.013] — 阈值 -0.012
    auto patchNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(seed ^ 0x5A5A0004ULL, -5, {1.0, 0.0, 0.0, 0.0, 0.0, 0.013}, -0.012);
    };
    // NETHERRACK: firstOctave=-3, amplitudes=[1.0, 0.0, 0.0, 0.35] — 阈值 0.54
    auto netherrackNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(seed ^ 0x5A5A0005ULL, -3, {1.0, 0.0, 0.0, 0.35}, 0.54);
    };
    // NETHER_WART: firstOctave=-3, amplitudes=[1.0, 0.0, 0.0, 0.9] — 阈值 1.17
    auto netherWartNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(seed ^ 0x5A5A0006ULL, -3, {1.0, 0.0, 0.0, 0.9}, 1.17);
    };
    // SOUL_SAND_LAYER: firstOctave=-8, amplitudes=[1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.01333...] — 阈值 -0.012
    auto soulSandLayerNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(seed ^ 0x5A5A0007ULL, -8, {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.013}, -0.012);
    };
    // GRAVEL_LAYER: firstOctave=-8, amplitudes=[1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.01333...] — 阈值 -0.012
    auto gravelLayerNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(seed ^ 0x5A5A0008ULL, -8, {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.013}, -0.012);
    };

    // MC 1.21.11: 共享的 PATCH 砾石层规则（用于玄武岩三角洲和灵魂沙峡谷的 UNDER_FLOOR）
    // ifTrue(noiseCondition(PATCH, -0.012),
    //   ifTrue(yStartCheck(absolute(30), 0),
    //     ifTrue(not(yStartCheck(absolute(35), 0)), GRAVEL)))
    auto patchGravelRule = [&]() -> std::unique_ptr<SurfaceRule> {
        if (!gravel) {
            return blockState(nullptr); // 占位
        }
        return ifTrue(patchNoise(),
            ifTrue(yStartCheck(VerticalAnchor::absolute(30), 0),
                ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(35), 0)), blockState(gravel))));
    };

    std::vector<std::unique_ptr<SurfaceRule>> rules;

    // 1. 基岩层底部
    if (bedrock) {
        rules.push_back(ifTrue(
            verticalGradient(seed ^ 0xBED10001ULL, VerticalAnchor::aboveBottom(0), VerticalAnchor::aboveBottom(5)),
            blockState(bedrock)));
    }

    // 2. 基岩层顶部
    if (bedrock) {
        rules.push_back(ifTrue(notCondition(verticalGradient(
                                   seed ^ 0xBED10002ULL, VerticalAnchor::belowTop(5), VerticalAnchor::belowTop(0))),
            blockState(bedrock)));
    }

    // 3. MC: yBlockCheck(belowTop(5), 0) -> NETHERRACK（顶部区域填充下界岩）
    if (netherrack) {
        rules.push_back(ifTrue(yBlockCheck(VerticalAnchor::belowTop(5), 0), blockState(netherrack)));
    }

    // 4. 玄武岩三角洲
    if (basalt && blackstone) {
        std::vector<std::unique_ptr<SurfaceRule>> basaltDeltasSeq;
        basaltDeltasSeq.push_back(ifTrue(underCeiling(), blockState(basalt)));
        {
            std::vector<std::unique_ptr<SurfaceRule>> underFloorSeq;
            // MC: PATCH 砾石层
            underFloorSeq.push_back(patchGravelRule());
            // MC: NETHER_STATE_SELECTOR 选择玄武岩/黑石
            underFloorSeq.push_back(ifTrue(netherStateSelector(), blockState(basalt)));
            underFloorSeq.push_back(blockState(blackstone));
            basaltDeltasSeq.push_back(ifTrue(underFloor(), sequence(std::move(underFloorSeq))));
        }
        rules.push_back(ifTrue(isBiome({Biomes::BasaltDeltas}), sequence(std::move(basaltDeltasSeq))));
    }

    // 5. 灵魂沙峡谷
    if (soulSand && soulSoil) {
        std::vector<std::unique_ptr<SurfaceRule>> soulSandValleySeq;
        {
            // UNDER_CEILING: NETHER_STATE_SELECTOR 选择 soul_sand/soul_soil
            std::vector<std::unique_ptr<SurfaceRule>> underCeilingSeq;
            underCeilingSeq.push_back(ifTrue(netherStateSelector(), blockState(soulSand)));
            underCeilingSeq.push_back(blockState(soulSoil));
            soulSandValleySeq.push_back(ifTrue(underCeiling(), sequence(std::move(underCeilingSeq))));
        }
        {
            // UNDER_FLOOR: PATCH 砾石层 + NETHER_STATE_SELECTOR
            std::vector<std::unique_ptr<SurfaceRule>> underFloorSeq;
            underFloorSeq.push_back(patchGravelRule());
            underFloorSeq.push_back(ifTrue(netherStateSelector(), blockState(soulSand)));
            underFloorSeq.push_back(blockState(soulSoil));
            soulSandValleySeq.push_back(ifTrue(underFloor(), sequence(std::move(underFloorSeq))));
        }
        rules.push_back(ifTrue(isBiome({Biomes::SoulSandValley}), sequence(std::move(soulSandValleySeq))));
    }

    // 6. ON_FLOOR 通用规则（所有生物群系）
    {
        std::vector<std::unique_ptr<SurfaceRule>> onFloorSeq;

        // 6a. MC: lava_floor — not(yBlockCheck(32)) && hole() -> LAVA
        if (lava) {
            onFloorSeq.push_back(
                ifTrue(notCondition(yBlockCheck(VerticalAnchor::absolute(32), 0)), ifTrue(hole(), blockState(lava))));
        }

        // 6b. 诡异森林 ON_FLOOR
        if (warpedNylium && warpedWartBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> warpedSeq;
            warpedSeq.push_back(ifTrue(netherWartNoise(), blockState(warpedWartBlock)));
            warpedSeq.push_back(blockState(warpedNylium));
            onFloorSeq.push_back(ifTrue(isBiome({Biomes::WarpedForest}),
                ifTrue(notCondition(netherrackNoise()),
                    ifTrue(yBlockCheck(VerticalAnchor::absolute(31), 0), sequence(std::move(warpedSeq))))));
        }

        // 6c. 绯红森林 ON_FLOOR
        // MC 1.21.11: not(yBlockCheck(32, 0)) + yBlockCheck(31, 0) → Y == 31 时触发
        // 修正: 还需要 not(noiseCondition(NETHERRACK, 0.54)) 条件
        if (crimsonNylium && netherWartBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> crimsonSeq;
            crimsonSeq.push_back(ifTrue(netherWartNoise(), blockState(netherWartBlock)));
            crimsonSeq.push_back(blockState(crimsonNylium));
            onFloorSeq.push_back(ifTrue(isBiome({Biomes::CrimsonForest}),
                ifTrue(notCondition(netherrackNoise()),
                    ifTrue(notCondition(yBlockCheck(VerticalAnchor::absolute(32), 0)),
                        ifTrue(yBlockCheck(VerticalAnchor::absolute(31), 0), sequence(std::move(crimsonSeq)))))));
        }

        rules.push_back(ifTrue(onFloor(), sequence(std::move(onFloorSeq))));
    }

    // 7. 下界荒地 UNDER_FLOOR + ON_FLOOR
    if (soulSand && netherrack && gravel) {
        std::vector<std::unique_ptr<SurfaceRule>> netherWastesSeq;

        // 7a. MC: UNDER_FLOOR soul_sand_layer
        // ifTrue(noiseCondition(SOUL_SAND_LAYER, -0.012),
        //   sequence(ifTrue(not(hole()), ifTrue(yStartCheck(30, 0),
        //     ifTrue(not(yStartCheck(35, 0)), SOUL_SAND))), NETHERRACK))
        {
            std::vector<std::unique_ptr<SurfaceRule>> soulSandLayerSeq;
            soulSandLayerSeq.push_back(ifTrue(notCondition(hole()),
                ifTrue(yStartCheck(VerticalAnchor::absolute(30), 0),
                    ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(35), 0)), blockState(soulSand)))));
            soulSandLayerSeq.push_back(blockState(netherrack));
            netherWastesSeq.push_back(
                ifTrue(underFloor(), ifTrue(soulSandLayerNoise(), sequence(std::move(soulSandLayerSeq)))));
        }

        // 7b. MC: ON_FLOOR gravel_layer
        // ifTrue(yBlockCheck(31, 0),
        //   ifTrue(not(yStartCheck(35, 0)),
        //     ifTrue(noiseCondition(GRAVEL_LAYER, -0.012),
        //       sequence(ifTrue(yBlockCheck(32, 0), GRAVEL), ifTrue(not(hole()), GRAVEL)))))
        {
            std::vector<std::unique_ptr<SurfaceRule>> gravelLayerSeq;
            gravelLayerSeq.push_back(ifTrue(yBlockCheck(VerticalAnchor::absolute(32), 0), blockState(gravel)));
            gravelLayerSeq.push_back(ifTrue(notCondition(hole()), blockState(gravel)));
            netherWastesSeq.push_back(ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(31), 0),
                    ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(35), 0)),
                        ifTrue(gravelLayerNoise(), sequence(std::move(gravelLayerSeq)))))));
        }

        rules.push_back(ifTrue(isBiome({Biomes::NetherWastes}), sequence(std::move(netherWastesSeq))));
    }

    // 8. 默认下界岩
    if (netherrack) {
        rules.push_back(blockState(netherrack));
    }

    return sequence(std::move(rules));
}

// ============================================================================
// 末地表面规则
// ============================================================================

std::unique_ptr<SurfaceRule> end()
{
    const BlockState* endStone = VanillaBlocks::END_STONE ? &VanillaBlocks::END_STONE->defaultState() : nullptr;
    return endStone ? blockState(endStone) : nullptr;
}

} // namespace SurfaceRules

// ============================================================================
// SurfaceSystem
// ============================================================================

SurfaceSystem::SurfaceSystem(std::unique_ptr<SurfaceRule> surfaceRule,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    i32 minY,
    i32 height,
    u64 seed,
    const math::PositionalRandomFactory& positionalRandom)
    : m_surfaceRule(std::move(surfaceRule))
    , m_defaultBlock(defaultBlock)
    , m_defaultFluid(defaultFluid)
    , m_seaLevel(seaLevel)
    , m_minY(minY)
    , m_height(height)
    , m_seed(seed)
    , m_positionalRandom(positionalRandom)
{
    // MC: SurfaceSystem 使用多个噪声生成器
    // SURFACE 噪声: firstOctave=-3, amplitudes=[1, 1, 0, 1]
    m_surfaceDepthNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0xAAAAAAA1ULL, -3, std::vector<f64>{1.0, 1.0, 0.0, 1.0});

    // SURFACE_SECONDARY 噪声: firstOctave=-4, amplitudes=[1, 1, 0, 1]
    m_surfaceSecondaryNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0xAAAAAAA2ULL, -4, std::vector<f64>{1.0, 1.0, 0.0, 1.0});

    // CLAY_BANDS_OFFSET 噪声: firstOctave=-5, amplitudes=[1, 1, 1, 1]
    m_clayBandsOffsetNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0xAAAAAAA3ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});

    // Badlands 噪声
    m_badlandsPillarNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0xBADA0001ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_badlandsPillarRoofNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0xBADA0002ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_badlandsSurfaceNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0xBADA0003ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});

    // 冰山噪声
    m_icebergPillarNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0x10EE0001ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_icebergPillarRoofNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0x10EE0002ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_icebergSurfaceNoise = std::make_unique<world::gen::noise::NormalNoise>(
        seed ^ 0x10EE0003ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
}

bool SurfaceSystem::isStone(const BlockState* state) const
{
    return state != nullptr && !state->isAir() && !state->isLiquid();
}

void SurfaceSystem::erodedBadlandsExtension(
    ChunkPrimer& chunk, i32 worldX, i32 worldZ, i32 surfaceY, i32 localX, i32 localZ) const
{
    // MC 1.21: SurfaceSystem.erodedBadlandsExtension()
    // 在 Eroded Badlands 中生成高耸石柱/方山地貌

    const f64 d1 = std::min(
        std::abs(m_badlandsSurfaceNoise->getValue(static_cast<f64>(worldX), 0.0, static_cast<f64>(worldZ)) * 8.25),
        m_badlandsPillarNoise->getValue(static_cast<f64>(worldX) * 0.2, 0.0, static_cast<f64>(worldZ) * 0.2) * 15.0);

    if (d1 <= 0.0) {
        return;
    }

    const f64 d4 = std::abs(
        m_badlandsPillarRoofNoise->getValue(static_cast<f64>(worldX) * 0.75, 0.0, static_cast<f64>(worldZ) * 0.75) *
        1.5);
    const f64 d5 = 64.0 + std::min(d1 * d1 * 2.5, std::ceil(d4 * 50.0) + 24.0);
    const i32 pillarTop = static_cast<i32>(d5);

    if (surfaceY > pillarTop) {
        return;
    }

    // 从 pillarTop 向下查找第一个默认方块（石头），碰到水则中止
    bool foundStone = false;
    for (i32 y = pillarTop; y >= m_minY; --y) {
        const BlockState* state = chunk.getBlockState(localX, y, localZ);
        if (state != nullptr && state->blockId() == m_defaultBlock->blockId()) {
            foundStone = true;
            break;
        }
        if (state != nullptr && state->isLiquid()) {
            return; // 水中不生成石柱
        }
    }
    if (!foundStone) {
        return;
    }

    // 从 pillarTop 向下填充空气为默认方块，直到碰到非空气方块
    for (i32 y = pillarTop; y >= m_minY; --y) {
        const BlockState* state = chunk.getBlockState(localX, y, localZ);
        if (state == nullptr || state->isAir()) {
            chunk.setBlockState(localX, y, localZ, m_defaultBlock);
        } else {
            break;
        }
    }
}

void SurfaceSystem::frozenOceanExtension(ChunkPrimer& chunk,
    i32 worldX,
    i32 worldZ,
    i32 surfaceY,
    i32 localX,
    i32 localZ,
    i32 minSurfaceLevel,
    bool isDeepFrozenOcean) const
{
    // MC 1.21: SurfaceSystem.frozenOceanExtension()
    // 在 Frozen Ocean / Deep Frozen Ocean 中生成冰山

    const f64 d1 = std::min(
        std::abs(m_icebergSurfaceNoise->getValue(static_cast<f64>(worldX), 0.0, static_cast<f64>(worldZ)) * 8.25),
        m_icebergPillarNoise->getValue(static_cast<f64>(worldX) * 1.28, 0.0, static_cast<f64>(worldZ) * 1.28) * 15.0);

    if (d1 <= 1.8) {
        return;
    }

    const f64 d5 =
        m_icebergPillarRoofNoise->getValue(static_cast<f64>(worldX) * 1.17, 0.0, static_cast<f64>(worldZ) * 1.17) * 1.5;
    f64 d6 = std::min(d1 * d1 * 1.2, std::ceil(d5 * 40.0) + 14.0);

    // MC: 如果生物群系温度导致冰面会融化，减少高度
    // FrozenOcean 的温度为 0.0，深海冻洋的温度也为 0.0
    // 简化处理：冻洋有轻微融化效果
    // 实际 MC 检查 biome.shouldMeltIce()，即 biome.getTemperature(pos, seaLevel) >= 0.15f 的一小部分区域
    // 但 frozenOceanExtension 使用的是 biome.hasTemperatureAdjustment() 和温度噪声
    // 简化为：不融化（因为只有 Frozen Ocean/Deep Frozen Ocean 才进入此函数）

    f64 icebergTop;
    f64 icebergBottom;

    if (d6 > 2.0) {
        icebergTop = static_cast<f64>(m_seaLevel);
        icebergBottom = static_cast<f64>(m_seaLevel) - d6 - 7.0;
    } else {
        icebergTop = 0.0;
        icebergBottom = 0.0;
    }

    if (icebergTop <= 0.0 && icebergBottom <= 0.0) {
        return;
    }

    // 使用位置随机数确定雪层参数
    auto rng = m_positionalRandom.at(worldX, 0, worldZ);
    const i32 snowLayerCount = 2 + rng->nextInt(4);                     // 2-5
    const i32 snowHeightThreshold = m_seaLevel + 18 + rng->nextInt(10); // seaLevel+18 到 seaLevel+27

    const BlockState* packedIce = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);
    const BlockState* snowBlock = VanillaBlocks::getState(VanillaBlocks::SNOW_BLOCK);

    const i32 startY = std::max(surfaceY, static_cast<i32>(icebergTop) + 1);
    i32 snowCount = 0;

    for (i32 y = startY; y >= minSurfaceLevel; --y) {
        const BlockState* state = chunk.getBlockState(localX, y, localZ);

        const bool isAir = (state == nullptr || state->isAir());
        const bool isWaterOrBelow = (state != nullptr && state->isLiquid()) &&
            (static_cast<f64>(y) <= static_cast<f64>(m_seaLevel) && static_cast<f64>(y) >= icebergBottom);

        if (isAir || isWaterOrBelow) {
            // MC 1.21: 空气在冰山顶部以下 99% 放置，水在冰山范围内 85% 放置
            // Java 的条件是放置条件（true = 放置），不是跳过条件
            const f64 rand = rng->nextDouble();
            bool shouldPlace = false;
            if (isAir && static_cast<f64>(y) < icebergTop && rand > 0.01) {
                shouldPlace = true;
            } else if (!isAir && icebergBottom != 0.0 && rand > 0.15) {
                shouldPlace = true;
            }
            if (!shouldPlace) {
                continue;
            }

            // 在雪层高度内且超过高度阈值：放雪块
            if (snowCount < snowLayerCount && y > snowHeightThreshold) {
                if (snowBlock) {
                    chunk.setBlockState(localX, y, localZ, snowBlock);
                }
                snowCount++;
            } else {
                // 放冰块
                if (packedIce) {
                    chunk.setBlockState(localX, y, localZ, packedIce);
                }
            }
        }
    }
}

void SurfaceSystem::buildSurface(ChunkPrimer& chunk,
    const std::function<BiomeId(i32, i32, i32)>& getBiomeAt,
    const density::NoiseChunk& noiseChunk) const
{
    if (!m_surfaceRule) {
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // 创建上下文（含高度查询回调用于 steep 条件）
    SurfaceRuleContext ctx(m_seaLevel,
        m_minY,
        m_height,
        m_surfaceDepthNoise.get(),
        m_surfaceSecondaryNoise.get(),
        m_clayBandsOffsetNoise.get(),
        noiseChunk,
        m_positionalRandom,
        [&chunk, startX, startZ, this](i32 worldX, i32 worldZ) -> i32 {
            // 将世界坐标转换为本地坐标
            const i32 localX = worldX - startX;
            const i32 localZ = worldZ - startZ;
            // 边界检查：超出区块范围时使用当前列高度
            if (localX < 0 || localX >= world::CHUNK_WIDTH || localZ < 0 || localZ >= world::CHUNK_WIDTH) {
                return m_seaLevel + 1; // 默认海平面高度
            }
            return chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;
        });

    for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
        for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
            const i32 worldX = startX + localX;
            const i32 worldZ = startZ + localZ;

            // MC 1.21: 先获取初始表面高度（erodedBadlands 可能修改此高度）
            const i32 initialSurfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;

            // 获取当前列的生物群系（使用表面高度处）
            const BiomeId biomeId = getBiomeAt(worldX, initialSurfaceY, worldZ);

            // MC 1.21: erodedBadlandsExtension — 在规则应用之前
            if (biomeId == Biomes::ErodedBadlands) {
                erodedBadlandsExtension(chunk, worldX, worldZ, initialSurfaceY, localX, localZ);
            }

            // 更新 XZ（erodedBadlandsExtension 可能修改了高度图，需重新获取）
            ctx.updateXZ(worldX, worldZ);
            ctx.setBiome(biomeId);

            // 从高度图获取表面高度（erodedBadlandsExtension 后重新获取）
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;

            // MC 1.21: 跟踪 stoneDepthAbove, stoneDepthBelow, waterHeight
            i32 stoneDepthAbove = 0;
            i32 waterHeight = INT_MIN;
            i32 stoneDepthBelowStart = INT_MAX;

            // 从上到下遍历列
            for (i32 y = surfaceY; y >= m_minY; --y) {
                const BlockState* currentState = chunk.getBlockState(localX, y, localZ);

                if (currentState == nullptr || currentState->isAir()) {
                    stoneDepthAbove = 0;
                    waterHeight = std::numeric_limits<i32>::min();
                    continue;
                }

                if (currentState->isLiquid()) {
                    if (waterHeight == INT_MIN) {
                        waterHeight = y + 1;
                    }
                    continue;
                }

                // 计算 stoneDepthBelow（到下方非石头方块的距离）
                if (stoneDepthBelowStart >= y) {
                    stoneDepthBelowStart = INT_MAX;
                    for (i32 dy = y - 1; dy >= m_minY - 1; --dy) {
                        const BlockState* belowState =
                            (dy >= m_minY) ? chunk.getBlockState(localX, dy, localZ) : nullptr;
                        if (!isStone(belowState)) {
                            stoneDepthBelowStart = dy + 1;
                            break;
                        }
                    }
                }

                const i32 stoneDepthBelow = y - stoneDepthBelowStart + 1;

                // 只替换默认方块
                if (currentState->blockId() != m_defaultBlock->blockId()) {
                    stoneDepthAbove++;
                    continue;
                }

                stoneDepthAbove++;

                // 更新 Y 相关状态
                ctx.updateY(stoneDepthAbove, stoneDepthBelow, waterHeight, worldX, y, worldZ);

                // 设置生物群系（使用当前位置的精确生物群系）
                const BiomeId yBiomeId = getBiomeAt(worldX, y, worldZ);
                ctx.setBiome(yBiomeId);

                // 应用表面规则
                const BlockState* replacement = m_surfaceRule->tryApply(worldX, y, worldZ, ctx);

                if (replacement != nullptr && replacement->blockId() != currentState->blockId()) {
                    chunk.setBlockState(localX, y, localZ, replacement);
                    // MC 1.21: SurfaceSystem — 写入流体方块时标记后处理
                    if (replacement->isLiquid()) {
                        chunk.markPosForPostprocessing(localX, y, localZ);
                    }
                }
            }

            // MC 1.21: frozenOceanExtension — 在规则应用之后
            if (biomeId == Biomes::FrozenOcean || biomeId == Biomes::DeepFrozenOcean) {
                frozenOceanExtension(chunk,
                    worldX,
                    worldZ,
                    initialSurfaceY,
                    localX,
                    localZ,
                    ctx.minSurfaceLevel(),
                    biomeId == Biomes::DeepFrozenOcean);
            }
        }
    }
}

} // namespace mc::world::gen::surface
