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
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace mc::world::gen::surface {

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

SurfaceRuleContext::SurfaceRuleContext(i32 seaLevel, i32 minY, i32 height,
    const world::gen::noise::NormalNoise* surfaceDepthNoise,
    const world::gen::noise::NormalNoise* surfaceSecondaryNoise,
    const world::gen::noise::NormalNoise* clayBandsOffsetNoise)
    : m_seaLevel(seaLevel)
    , m_minY(minY)
    , m_height(height)
    , m_surfaceDepthNoise(surfaceDepthNoise)
    , m_surfaceSecondaryNoise(surfaceSecondaryNoise)
    , m_clayBandsOffsetNoise(clayBandsOffsetNoise)
{
    // 初始化 bandlands 陶土带
    generateClayBands(static_cast<u64>(seaLevel) * 341873128712ULL);
}

void SurfaceRuleContext::updateXZ(i32 blockX, i32 blockZ)
{
    m_blockX = blockX;
    m_blockZ = blockZ;

    // 计算地表深度（MC: SurfaceSystem.getSurfaceDepth）
    if (m_surfaceDepthNoise) {
        const f64 noiseVal = m_surfaceDepthNoise->getValue(static_cast<f64>(blockX), 0.0, static_cast<f64>(blockZ));
        m_surfaceDepth = static_cast<i32>(noiseVal * 2.75 + 3.0);
    } else {
        m_surfaceDepth = 3;
    }

    // 清除缓存
    m_surfaceSecondaryCached = false;
}

void SurfaceRuleContext::updateY(i32 stoneDepthAbove, i32 stoneDepthBelow, i32 waterHeight,
    i32 blockX, i32 blockY, i32 blockZ)
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
            m_surfaceSecondaryValue = m_surfaceSecondaryNoise->getValue(
                static_cast<f64>(m_blockX), 0.0, static_cast<f64>(m_blockZ));
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
        const i32 index = ((blockY + offset) % static_cast<i32>(m_clayBands.size()) +
                              static_cast<i32>(m_clayBands.size())) %
                          static_cast<i32>(m_clayBands.size());
        return m_clayBands[static_cast<size_t>(index)];
    }
    return VanillaBlocks::TERRACOTTA ? &VanillaBlocks::TERRACOTTA->defaultState() : nullptr;
}

bool SurfaceRuleContext::abovePreliminarySurface() const
{
    // 简化实现：使用海平面作为预估表面
    // 完整实现需要 NoiseChunk.preliminarySurfaceLevel()
    return m_blockY >= m_seaLevel - 8;
}

bool SurfaceRuleContext::steep() const
{
    // 简化实现：始终返回 false
    // 完整实现需要高度图数据（ChunkAccess.getHeight）
    return false;
}

bool SurfaceRuleContext::temperature() const
{
    // 简化实现：基于生物群系判断是否足够冷以降雪
    // 完整实现需要 Climate.Sampler.temperature()
    // 冰冻生物群系 ID 列表
    static const BiomeId frozenBiomes[] = {
        Biomes::FrozenOcean,
        Biomes::DeepFrozenOcean,
        Biomes::SnowyPlains,
        Biomes::IceSpikes,
        Biomes::SnowyTaiga,
        Biomes::SnowyBeach,
        Biomes::SnowySlopes,
        Biomes::FrozenPeaks,
        Biomes::JaggedPeaks,
        Biomes::Grove,
    };

    for (const auto& id : frozenBiomes) {
        if (m_biome == id) {
            return true;
        }
    }
    return false;
}

void SurfaceRuleContext::generateClayBands(u64 seed)
{
    // MC: SurfaceSystem.generateBands()
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

    std::mt19937 rng(static_cast<u32>(seed));

    // 橙色条纹
    for (size_t k = 0; k < m_clayBands.size();) {
        k += (rng() % 5) + 1;
        if (k < m_clayBands.size()) {
            m_clayBands[k] = orangeTerracotta;
        }
    }

    // 辅助 lambda: 生成指定颜色的条纹
    auto makeBands = [&](i32 count, const BlockState* color) {
        const i32 bandCount = static_cast<i32>(rng() % 10) + 6;
        for (i32 j = 0; j < bandCount; ++j) {
            i32 bandWidth = count + static_cast<i32>(rng() % 3);
            i32 start = static_cast<i32>(rng() % m_clayBands.size());
            for (i32 i1 = 0; (start + i1) < static_cast<i32>(m_clayBands.size()) && i1 < bandWidth; ++i1) {
                m_clayBands[static_cast<size_t>(start + i1)] = color;
            }
        }
    };

    makeBands(1, yellowTerracotta);
    makeBands(2, brownTerracotta);
    makeBands(1, redTerracotta);

    // 白色条纹
    const i32 l = static_cast<i32>(rng() % 7) + 9;
    i32 i = 0;
    for (i32 j = 0; i < l && j < static_cast<i32>(m_clayBands.size()); j += static_cast<i32>(rng() % 16) + 4) {
        m_clayBands[static_cast<size_t>(j)] = whiteTerracotta;
        if (j - 1 > 0 && rng() % 2 == 0) {
            m_clayBands[static_cast<size_t>(j - 1)] = lightGrayTerracotta;
        }
        if (j + 1 < static_cast<i32>(m_clayBands.size()) && rng() % 2 == 0) {
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
    return std::make_unique<NoiseThresholdCondition>(seed, firstOctave, std::move(amplitudes), minThreshold, maxThreshold);
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

std::unique_ptr<SurfaceRule> ifTrue(
    std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule)
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
    const BlockState* deepslate = nullptr; // TODO: VanillaBlocks::DEEPSLATE
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

    // ========== 构建规则树 ==========
    std::vector<std::unique_ptr<SurfaceRule>> rules;

    // 1. 基岩层底部 (Y 0-4 渐变)
    if (bedrock) {
        rules.push_back(ifTrue(
            verticalGradient(seed ^ 0xBEDB0001ULL,
                VerticalAnchor::aboveBottom(0),
                VerticalAnchor::aboveBottom(5)),
            blockState(bedrock)));
    }

    // 2. 深板岩层 (Y 0-8 渐变过渡)
    if (deepslate) {
        rules.push_back(ifTrue(
            verticalGradient(seed ^ 0xDEE00001ULL,
                VerticalAnchor::absolute(0),
                VerticalAnchor::absolute(8)),
            blockState(deepslate)));
    }

    // 3. 地表规则（在预备表面之上）
    // MC 1.21 overworld() 规则树简化版

    // 3a. ON_FLOOR + water → GRASS, else DIRT
    if (grass && dirt) {
        rules.push_back(ifTrue(onFloor(),
            ifTrue(waterBlockCheck(0, 0), blockState(grass))));
        rules.push_back(ifTrue(underFloor(), blockState(dirt)));
    }

    // 3b. 沙漠：沙 + 沙岩
    if (sand && sandstone) {
        std::vector<std::unique_ptr<SurfaceRule>> sandRules;
        sandRules.push_back(ifTrue(onCeiling(), blockState(sandstone)));
        sandRules.push_back(blockState(sand));
        rules.push_back(ifTrue(isBiome({Biomes::Desert}),
            sequence(std::move(sandRules))));
    }

    // 3c. 温暖海洋/海滩：沙 + 沙岩
    if (sand && sandstone) {
        std::vector<std::unique_ptr<SurfaceRule>> warmOceanRules;
        warmOceanRules.push_back(ifTrue(onCeiling(), blockState(sandstone)));
        warmOceanRules.push_back(blockState(sand));
        rules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::Beach, Biomes::SnowyBeach}),
            sequence(std::move(warmOceanRules))));
    }

    // 3d. 砾石海滩
    if (gravel) {
        std::vector<std::unique_ptr<SurfaceRule>> gravelRules;
        gravelRules.push_back(ifTrue(onCeiling(), blockState(stone)));
        gravelRules.push_back(blockState(gravel));
        rules.push_back(ifTrue(isBiome({Biomes::StonyShore}),
            sequence(std::move(gravelRules))));
    }

    // 3e. 冰冻海洋
    if (ice && water && air) {
        std::vector<std::unique_ptr<SurfaceRule>> frozenOceanSeq;
        frozenOceanSeq.push_back(ifTrue(onFloor(), blockState(air)));
        frozenOceanSeq.push_back(ifTrue(temperature(), blockState(ice)));
        frozenOceanSeq.push_back(blockState(water));
        rules.push_back(ifTrue(isBiome({Biomes::FrozenOcean, Biomes::DeepFrozenOcean}),
            ifTrue(waterBlockCheck(-1, 0),
                ifTrue(hole(),
                    sequence(std::move(frozenOceanSeq))))));
    }

    // 3f. 水面附近：浅水下方使用沙/沙岩
    if (sand && sandstone) {
        std::vector<std::unique_ptr<SurfaceRule>> waterSeq;
        waterSeq.push_back(ifTrue(isBiome({Biomes::Desert}), ifTrue(veryDeepUnderFloor(), blockState(sandstone))));
        rules.push_back(ifTrue(waterStartCheck(-6, -1),
            sequence(std::move(waterSeq))));
    }

    // 3g. 山地/山坡
    if (stone) {
        rules.push_back(ifTrue(isBiome({Biomes::StonyPeaks}), blockState(stone)));
        rules.push_back(ifTrue(isBiome({Biomes::JaggedPeaks}), blockState(stone)));
    }

    // 3h. 雪地
    const BlockState* snowBlock = VanillaBlocks::SNOW_BLOCK ? &VanillaBlocks::SNOW_BLOCK->defaultState() : nullptr;
    if (snowBlock) {
        rules.push_back(ifTrue(isBiome({Biomes::FrozenPeaks}),
            ifTrue(onFloor(), blockState(snowBlock))));
        rules.push_back(ifTrue(isBiome({Biomes::SnowySlopes}),
            ifTrue(onFloor(), blockState(snowBlock))));
    }

    // 3i. 蘑菇岛
    if (mycelium) {
        rules.push_back(ifTrue(isBiome({Biomes::MushroomFields}),
            ifTrue(onFloor(), blockState(mycelium))));
    }

    // 3j. 大型针叶林
    if (podzol && coarseDirt) {
        std::vector<std::unique_ptr<SurfaceRule>> taigaSeq;
        taigaSeq.push_back(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, 1.75 / 8.25, 1e30),
            blockState(coarseDirt)));
        taigaSeq.push_back(ifTrue(noiseCondition(seed ^ 0x5B5B0001ULL, -3, {1.0, 1.0, 0.0, 1.0}, -0.95 / 8.25, 1e30),
            blockState(podzol)));
        rules.push_back(ifTrue(isBiome({Biomes::OldGrowthPineTaiga, Biomes::OldGrowthSpruceTaiga}),
            sequence(std::move(taigaSeq))));
    }

    return sequence(std::move(rules));
}

// ============================================================================
// 下界表面规则 — MC 1.21 SurfaceRuleData.nether()
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

    // Y 条件 (unused directly but used inline below)

    // 生物群系条件 (used inline via isBiome())

    // 噪声条件 (used inline via noiseCondition())

    std::vector<std::unique_ptr<SurfaceRule>> rules;

    // 1. 基岩层底部
    if (bedrock) {
        rules.push_back(ifTrue(
            verticalGradient(seed ^ 0xBED10001ULL,
                VerticalAnchor::aboveBottom(0),
                VerticalAnchor::aboveBottom(5)),
            blockState(bedrock)));
    }

    // 2. 基岩层顶部
    if (bedrock) {
        rules.push_back(ifTrue(
            notCondition(verticalGradient(seed ^ 0xBED10002ULL,
                VerticalAnchor::belowTop(5),
                VerticalAnchor::belowTop(0))),
            blockState(bedrock)));
    }

    // 3. 玄武岩三角洲
    if (basalt && blackstone) {
        std::vector<std::unique_ptr<SurfaceRule>> basaltDeltasSeq;
        basaltDeltasSeq.push_back(ifTrue(underCeiling(), blockState(basalt)));
        {
            std::vector<std::unique_ptr<SurfaceRule>> underFloorSeq;
            underFloorSeq.push_back(ifTrue(noiseCondition(seed ^ 0x5A5A0001ULL, -5, {1.0, 1.0, 1.0, 1.0}, 0.0, 1e30), blockState(basalt)));
            underFloorSeq.push_back(blockState(blackstone));
            basaltDeltasSeq.push_back(ifTrue(underFloor(), sequence(std::move(underFloorSeq))));
        }
        rules.push_back(ifTrue(isBiome({Biomes::BasaltDeltas}),
            sequence(std::move(basaltDeltasSeq))));
    }

    // 4. 灵魂沙峡谷
    if (soulSand && soulSoil) {
        std::vector<std::unique_ptr<SurfaceRule>> soulSandValleySeq;
        {
            std::vector<std::unique_ptr<SurfaceRule>> underCeilingSeq;
            underCeilingSeq.push_back(blockState(soulSand));
            underCeilingSeq.push_back(blockState(soulSoil));
            soulSandValleySeq.push_back(ifTrue(underCeiling(), sequence(std::move(underCeilingSeq))));
        }
        {
            std::vector<std::unique_ptr<SurfaceRule>> underFloorSeq;
            underFloorSeq.push_back(blockState(soulSand));
            underFloorSeq.push_back(blockState(soulSoil));
            soulSandValleySeq.push_back(ifTrue(underFloor(), sequence(std::move(underFloorSeq))));
        }
        rules.push_back(ifTrue(isBiome({Biomes::SoulSandValley}),
            sequence(std::move(soulSandValleySeq))));
    }

    // 5. 诡异森林
    if (warpedNylium && warpedWartBlock) {
        std::vector<std::unique_ptr<SurfaceRule>> warpedForestSeq;
        warpedForestSeq.push_back(ifTrue(noiseCondition(seed ^ 0x5A5A0003ULL, -5, {1.0, 1.0, 1.0, 1.0}, 1.17, 1e30), blockState(warpedWartBlock)));
        warpedForestSeq.push_back(blockState(warpedNylium));
        rules.push_back(ifTrue(isBiome({Biomes::WarpedForest}),
            ifTrue(onFloor(),
                ifTrue(notCondition(noiseCondition(seed ^ 0x5A5A0002ULL, -5, {1.0, 1.0, 1.0, 1.0}, 0.54, 1e30)),
                    ifTrue(yBlockCheck(VerticalAnchor::absolute(31), 0),
                        sequence(std::move(warpedForestSeq)))))));
    }

    // 6. 绯红森林
    if (crimsonNylium && netherWartBlock) {
        std::vector<std::unique_ptr<SurfaceRule>> crimsonForestSeq;
        crimsonForestSeq.push_back(blockState(netherWartBlock));
        crimsonForestSeq.push_back(blockState(crimsonNylium));
        rules.push_back(ifTrue(isBiome({Biomes::CrimsonForest}),
            ifTrue(onFloor(),
                ifTrue(notCondition(yBlockCheck(VerticalAnchor::absolute(31), 0)),
                    ifTrue(yBlockCheck(VerticalAnchor::absolute(32), 0),
                        sequence(std::move(crimsonForestSeq)))))));
    }

    // 7. 默认下界岩
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
    u64 seed)
    : m_surfaceRule(std::move(surfaceRule))
    , m_defaultBlock(defaultBlock)
    , m_defaultFluid(defaultFluid)
    , m_seaLevel(seaLevel)
    , m_minY(minY)
    , m_height(height)
    , m_seed(seed)
{
    // MC: SurfaceSystem 使用多个噪声生成器
    // SURFACE 噪声: firstOctave=-3, amplitudes=[1, 1, 0, 1]
    m_surfaceDepthNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0xAAAAAAA1ULL, -3, std::vector<f64>{1.0, 1.0, 0.0, 1.0});

    // SURFACE_SECONDARY 噪声: firstOctave=-4, amplitudes=[1, 1, 0, 1]
    m_surfaceSecondaryNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0xAAAAAAA2ULL, -4, std::vector<f64>{1.0, 1.0, 0.0, 1.0});

    // CLAY_BANDS_OFFSET 噪声: firstOctave=-5, amplitudes=[1, 1, 1, 1]
    m_clayBandsOffsetNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0xAAAAAAA3ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});

    // Badlands 噪声
    m_badlandsPillarNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0xBADA0001ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_badlandsPillarRoofNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0xBADA0002ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_badlandsSurfaceNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0xBADA0003ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});

    // 冰山噪声
    m_icebergPillarNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0x10EE0001ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_icebergPillarRoofNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0x10EE0002ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
    m_icebergSurfaceNoise = std::make_unique<world::gen::noise::NormalNoise>(seed ^ 0x10EE0003ULL, -5, std::vector<f64>{1.0, 1.0, 1.0, 1.0});
}

bool SurfaceSystem::isStone(const BlockState* state) const
{
    return state != nullptr && !state->isAir() && !state->isLiquid();
}

void SurfaceSystem::buildSurface(ChunkPrimer& chunk,
    const std::function<BiomeId(i32, i32, i32)>& getBiomeAt) const
{
    if (!m_surfaceRule) {
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // 创建上下文
    SurfaceRuleContext ctx(m_seaLevel, m_minY, m_height,
        m_surfaceDepthNoise.get(), m_surfaceSecondaryNoise.get(), m_clayBandsOffsetNoise.get());

    for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
        for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
            const i32 worldX = startX + localX;
            const i32 worldZ = startZ + localZ;

            // 更新 XZ
            ctx.updateXZ(worldX, worldZ);

            // 从高度图获取表面高度
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
                        const BlockState* belowState = (dy >= m_minY) ? chunk.getBlockState(localX, dy, localZ) : nullptr;
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

                // 设置生物群系
                const BiomeId biomeId = getBiomeAt(worldX, y, worldZ);
                ctx.setBiome(biomeId);

                // 应用表面规则
                const BlockState* replacement = m_surfaceRule->tryApply(worldX, y, worldZ, ctx);

                if (replacement != nullptr && replacement->blockId() != currentState->blockId()) {
                    chunk.setBlockState(localX, y, localZ, replacement);
                }
            }
        }
    }
}

} // namespace mc::world::gen::surface
