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

#include "common/world/gen/surface/SurfaceRulesFactory.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/noise/Noises.hpp"
#include <utility>

namespace mc::world::gen::surface {

namespace SurfaceRules {

// ============================================================================
// 常用条件快捷方式
// ============================================================================

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

// ============================================================================
// 条件工厂
// ============================================================================

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

std::unique_ptr<SurfaceCondition> noiseCondition(std::string noiseName, f64 minThreshold, f64 maxThreshold)
{
    return std::make_unique<NoiseThresholdCondition>(std::move(noiseName), minThreshold, maxThreshold);
}

std::unique_ptr<SurfaceCondition> verticalGradient(
    std::string randomName, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove)
{
    return std::make_unique<VerticalGradientCondition>(std::move(randomName), trueAtAndBelow, falseAtAndAbove);
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

// ============================================================================
// 规则工厂
// ============================================================================

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

[[maybe_unused]] static std::unique_ptr<SurfaceCondition> surfaceNoiseAbove(f64 threshold)
{
    // MC: noiseCondition(Noises.SURFACE, threshold / 8.25, Double.MAX_VALUE)
    return noiseCondition(noise::Noises::SURFACE, threshold / 8.25, 1e30);
}

// ============================================================================
// 主世界表面规则 — MC 1.21 SurfaceRuleData.overworld()
// ============================================================================

std::unique_ptr<SurfaceRule> overworld()
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
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
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
    // MC 1.21 SurfaceRuleData.overworldLike() — 完整规则树
    // 所有噪声条件使用 Noises:: 注册名称，通过 RandomState 查找
    //
    // MC 原版顶层序列结构：
    //   1. bedrock_floor（基岩底部）
    //   2. abovePreliminarySurface() 包裹的表面规则
    //   3. deepslate（深板岩，在 abovePreliminarySurface 之后）
    std::vector<std::unique_ptr<SurfaceRule>> rules;

    // 1. 基岩层底部 (Y 0-4 渐变) — 在 abovePreliminarySurface 外面
    if (bedrock) {
        rules.push_back(ifTrue(
            verticalGradient("minecraft:bedrock_floor", VerticalAnchor::aboveBottom(0), VerticalAnchor::aboveBottom(5)),
            blockState(bedrock)));
    }

    // 2. 表面规则 — 在 abovePreliminarySurface() 里面
    std::vector<std::unique_ptr<SurfaceRule>> surfaceRules;
    if (coarseDirt && grass && dirt) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::WoodedBadlands}),
            ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(97), 2),
                    sequence(ifTrue(noiseCondition(noise::Noises::SURFACE, -0.909 / 8.25, -0.5454 / 8.25),
                                 blockState(coarseDirt)),
                        ifTrue(noiseCondition(noise::Noises::SURFACE, -0.1818 / 8.25, 0.1818 / 8.25),
                            blockState(coarseDirt)),
                        ifTrue(noiseCondition(noise::Noises::SURFACE, 0.5454 / 8.25, 0.909 / 8.25),
                            blockState(coarseDirt)),
                        ifTrue(waterBlockCheck(0, 0), blockState(grass)),
                        blockState(dirt))))));
    }

    // 4. ON_FLOOR: 沼泽水面（Y>=62 且 Y<63，噪声触发时放水）
    // MC 1.21: noiseCondition(Noises.SWAMP, 0.0)
    if (water) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::Swamp}),
            ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(62), 0),
                    ifTrue(yStartCheck(VerticalAnchor::absolute(63), -1),
                        ifTrue(noiseCondition(noise::Noises::SWAMP, 0.0, 1e30), blockState(water)))))));
    }

    // 5. ON_FLOOR: 红树林沼泽水面（Y>=60 且 Y<63）
    // MC 1.21: noiseCondition(Noises.SWAMP, 0.0)
    if (water) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::MangroveSwamp}),
            ifTrue(onFloor(),
                ifTrue(yBlockCheck(VerticalAnchor::absolute(60), 0),
                    ifTrue(yStartCheck(VerticalAnchor::absolute(63), -1),
                        ifTrue(noiseCondition(noise::Noises::SWAMP, 0.0, 1e30), blockState(water)))))));
    }

    // 6. 恶地家族 ON_FLOOR/UNDER_FLOOR/VERY_DEEP_UNDER_FLOOR
    if (orangeTerracotta && redSand && whiteTerracotta && gravel && stone) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::Badlands, Biomes::ErodedBadlands, Biomes::WoodedBadlands}),
            sequence(
                // ON_FLOOR
                ifTrue(onFloor(),
                    sequence(
                        // Y >= 256: 橙色陶土
                        ifTrue(yBlockCheck(VerticalAnchor::absolute(256), 0), blockState(orangeTerracotta)),
                        // Y <= 74+surfaceDepth: 噪声陶土带（MC 1.21.11 使用 TERRACOTTA）
                        ifTrue(yStartCheck(VerticalAnchor::absolute(74), 1),
                            sequence(ifTrue(noiseCondition(noise::Noises::SURFACE, -0.909 / 8.25, -0.5454 / 8.25),
                                         blockState(terracotta)),
                                ifTrue(noiseCondition(noise::Noises::SURFACE, -0.1818 / 8.25, 0.1818 / 8.25),
                                    blockState(terracotta)),
                                ifTrue(noiseCondition(noise::Noises::SURFACE, 0.5454 / 8.25, 0.909 / 8.25),
                                    blockState(terracotta)),
                                bandlands())),
                        // !hole && waterCheck(-1): 红沙（含红砂岩天花板）
                        ifTrue(waterBlockCheck(-1, 0),
                            sequence(ifTrue(onCeiling(), blockState(redSandstone)), blockState(redSand))),
                        // !hole: 橙色陶土（MC 1.21.11 使用 ORANGE_TERRACOTTA）
                        ifTrue(notCondition(hole()), blockState(orangeTerracotta)),
                        // waterStartCheck(-6, -1): 白色陶土
                        ifTrue(waterStartCheck(-6, -1), blockState(whiteTerracotta)),
                        // 兜底: 砾石/石头
                        sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel)))),
                // yStartCheck(63, -1): 仅 Y 条件检查，无 underFloor 包裹
                // MC 源码中此处不是 underFloor 条件，而是纯 Y 坐标条件
                ifTrue(yStartCheck(VerticalAnchor::absolute(63), -1),
                    sequence(
                        // Y >= 63 且不在 terracotta 带范围: 橙色陶土
                        ifTrue(yBlockCheck(VerticalAnchor::absolute(63), 0),
                            ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(74), 1)),
                                blockState(orangeTerracotta))),
                        // bandlands
                        bandlands())),
                // UNDER_FLOOR + waterStartCheck → 白色陶土
                // MC 源码使用 underFloor()（secondaryDepthRange=0），非 veryDeepUnderFloor()
                ifTrue(underFloor(), ifTrue(waterStartCheck(-6, -1), blockState(whiteTerracotta))))));
    }

    // 7. 冰冻海洋 hole 处理（水面上方放置空气/冰/水）
    // MC 1.21.11: 序列中第一个条件是 waterBlockCheck(0,0)（在水面上方），而非 onFloor()
    if (water && ice && air) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::FrozenOcean, Biomes::DeepFrozenOcean}),
            ifTrue(waterBlockCheck(-1, 0),
                ifTrue(hole(),
                    sequence(ifTrue(waterBlockCheck(0, 0), blockState(air)),
                        ifTrue(temperature(), blockState(ice)),
                        blockState(water))))));
    }

    // 8. 水旁 onFloor: 冰冻海洋 hole → 水
    if (water) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::FrozenOcean, Biomes::DeepFrozenOcean}),
            ifTrue(waterStartCheck(-6, -1), ifTrue(onFloor(), ifTrue(hole(), blockState(water))))));
    }

    // 9. waterBlockCheck + ON_FLOOR: 地表顶层（MC 原版第3条规则）
    // 此规则必须在 underFloor 材料层规则（规则10）之前，因为 onFloor 条件对地表第一个石头方块成立，
    // 如果 underFloor 先匹配（surfaceDepth 使得 stoneDepth <= 1 + surfaceDepth 为真），
    // 地表会错误地被替换为泥土而非草方块。
    {
        std::vector<std::unique_ptr<SurfaceRule>> topRules;

        // 冰冻峰 ON_FLOOR: steep→PACKED_ICE, noise→PACKED_ICE/ICE, !water→SNOW_BLOCK
        // MC 1.21.11 ON_FLOOR: PACKED_ICE 噪声范围 [0.0, 0.2], ICE 噪声范围 [0.0, 0.025]
        if (packedIce && ice && snowBlock) {
            topRules.push_back(ifTrue(isBiome({Biomes::FrozenPeaks}),
                sequence(ifTrue(steep(), blockState(packedIce)),
                    ifTrue(noiseCondition(noise::Noises::PACKED_ICE, 0.0, 0.2), blockState(packedIce)),
                    ifTrue(noiseCondition(noise::Noises::ICE, 0.0, 0.025), blockState(ice)),
                    ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)))));
        }

        // 雪山斜坡: steep→STONE, powderSnow(高阈值), !water→SNOW_BLOCK
        if (stone && snowBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> snowySlopesSeq;
            snowySlopesSeq.push_back(ifTrue(steep(), blockState(stone)));
            if (powderSnow) {
                snowySlopesSeq.push_back(ifTrue(noiseCondition(noise::Noises::POWDER_SNOW, 0.35, 0.6),
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
                groveSeq.push_back(ifTrue(noiseCondition(noise::Noises::POWDER_SNOW, 0.35, 0.6),
                    ifTrue(waterBlockCheck(0, 0), blockState(powderSnow))));
            }
            groveSeq.push_back(ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)));
            topRules.push_back(ifTrue(isBiome({Biomes::Grove}), sequence(std::move(groveSeq))));
        }

        // 石峰: calcite noise → CALCITE, else STONE
        if (calcite && stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::StonyPeaks}),
                sequence(ifTrue(noiseCondition(noise::Noises::CALCITE, -0.0125, 0.0125), blockState(calcite)),
                    blockState(stone))));
        }

        // 石岸: gravel noise → gravel/stone, else STONE
        if (gravel && stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::StonyShore}),
                sequence(ifTrue(noiseCondition(noise::Noises::GRAVEL, -0.05, 0.05),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    blockState(stone))));
        }

        // 风蚀丘陵: surfaceNoiseAbove(1.0) → STONE
        if (stone) {
            topRules.push_back(ifTrue(isBiome({Biomes::WindsweptHills}),
                ifTrue(noiseCondition(noise::Noises::SURFACE, 1.0 / 8.25, 1e30), blockState(stone))));
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
                sequence(ifTrue(noiseCondition(noise::Noises::SURFACE, 1.75 / 8.25, 1e30), blockState(stone)),
                    ifTrue(noiseCondition(noise::Noises::SURFACE, -0.5 / 8.25, 1e30), blockState(coarseDirt)))));
        }

        // 风蚀砾石丘陵: noise分层
        if (gravel && stone && grass && dirt) {
            topRules.push_back(ifTrue(isBiome({Biomes::WindsweptGravellyHills}),
                sequence(ifTrue(noiseCondition(noise::Noises::SURFACE, 2.0 / 8.25, 1e30),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    ifTrue(noiseCondition(noise::Noises::SURFACE, 1.0 / 8.25, 1e30), blockState(stone)),
                    ifTrue(noiseCondition(noise::Noises::SURFACE, -1.0 / 8.25, 1e30),
                        sequence(ifTrue(waterBlockCheck(0, 0), blockState(grass)), blockState(dirt))),
                    sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel)))));
        }

        // 大型针叶林: noise→coarseDirt/podzol
        if (podzol && coarseDirt) {
            topRules.push_back(ifTrue(isBiome({Biomes::OldGrowthPineTaiga, Biomes::OldGrowthSpruceTaiga}),
                sequence(ifTrue(noiseCondition(noise::Noises::SURFACE, 1.75 / 8.25, 1e30), blockState(coarseDirt)),
                    ifTrue(noiseCondition(noise::Noises::SURFACE, -0.95 / 8.25, 1e30), blockState(podzol)))));
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

        // 蘑菇岛: MYCELIUM（已在 onFloor 父级内，无需额外 onFloor 检查）
        if (mycelium) {
            topRules.push_back(ifTrue(isBiome({Biomes::MushroomFields}), blockState(mycelium)));
        }

        // 默认: grass(water)/dirt
        if (grass && dirt) {
            topRules.push_back(ifTrue(waterBlockCheck(0, 0), blockState(grass)));
            topRules.push_back(blockState(dirt));
        }

        if (!topRules.empty()) {
            surfaceRules.push_back(ifTrue(onFloor(), ifTrue(waterBlockCheck(-1, 0), sequence(std::move(topRules)))));
        }
    }

    // 10. 水旁 underFloor: 地表材料层（MC 原版第4条规则）
    {
        std::vector<std::unique_ptr<SurfaceRule>> matRules;

        // 冰冻峰: steep→PACKED_ICE, noise→PACKED_ICE/ICE, !water→SNOW_BLOCK
        // MC 1.21.11 UNDER_FLOOR: PACKED_ICE 噪声范围 [-0.5, 0.2], ICE 噪声范围 [-0.0625, 0.025]
        if (packedIce && ice && snowBlock) {
            matRules.push_back(ifTrue(isBiome({Biomes::FrozenPeaks}),
                sequence(ifTrue(steep(), blockState(packedIce)),
                    ifTrue(noiseCondition(noise::Noises::PACKED_ICE, -0.5, 0.2), blockState(packedIce)),
                    ifTrue(noiseCondition(noise::Noises::ICE, -0.0625, 0.025), blockState(ice)),
                    ifTrue(waterBlockCheck(0, 0), blockState(snowBlock)))));
        }

        // 雪山斜坡: steep→STONE, powderSnow, !water→SNOW_BLOCK
        if (stone && snowBlock) {
            std::vector<std::unique_ptr<SurfaceRule>> snowySlopesSeq;
            snowySlopesSeq.push_back(ifTrue(steep(), blockState(stone)));
            if (powderSnow) {
                snowySlopesSeq.push_back(ifTrue(noiseCondition(noise::Noises::POWDER_SNOW, 0.45, 0.58),
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
                groveSeq.push_back(ifTrue(noiseCondition(noise::Noises::POWDER_SNOW, 0.45, 0.58),
                    ifTrue(waterBlockCheck(0, 0), blockState(powderSnow))));
            }
            groveSeq.push_back(blockState(dirt));
            matRules.push_back(ifTrue(isBiome({Biomes::Grove}), sequence(std::move(groveSeq))));
        }

        // 石峰: calcite noise → CALCITE, else STONE
        if (calcite && stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::StonyPeaks}),
                sequence(ifTrue(noiseCondition(noise::Noises::CALCITE, -0.0125, 0.0125), blockState(calcite)),
                    blockState(stone))));
        }

        // 石岸: gravel noise → gravel/stone, else STONE
        if (gravel && stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::StonyShore}),
                sequence(ifTrue(noiseCondition(noise::Noises::GRAVEL, -0.05, 0.05),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    blockState(stone))));
        }

        // 风蚀丘陵: surfaceNoiseAbove(1.0) → STONE
        if (stone) {
            matRules.push_back(ifTrue(isBiome({Biomes::WindsweptHills}),
                ifTrue(noiseCondition(noise::Noises::SURFACE, 1.0 / 8.25, 1e30), blockState(stone))));
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
                ifTrue(noiseCondition(noise::Noises::SURFACE, 1.75 / 8.25, 1e30), blockState(stone))));
        }

        // 风蚀砾石丘陵: noise分层 → gravel/stone/dirt/gravel（underFloor 用 DIRT，无草方块分裂）
        if (gravel && stone && dirt) {
            matRules.push_back(ifTrue(isBiome({Biomes::WindsweptGravellyHills}),
                sequence(ifTrue(noiseCondition(noise::Noises::SURFACE, 2.0 / 8.25, 1e30),
                             sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))),
                    ifTrue(noiseCondition(noise::Noises::SURFACE, 1.0 / 8.25, 1e30), blockState(stone)),
                    ifTrue(noiseCondition(noise::Noises::SURFACE, -1.0 / 8.25, 1e30), blockState(dirt)),
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
            surfaceRules.push_back(
                ifTrue(waterStartCheck(-6, -1), ifTrue(underFloor(), sequence(std::move(matRules)))));
        }
    }

    // 10. 水旁 deepUnderFloor: 温暖海洋/海滩 → sandstone
    if (sandstone) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::Beach, Biomes::SnowyBeach}),
            ifTrue(waterStartCheck(-6, -1), ifTrue(deepUnderFloor(), blockState(sandstone)))));
    }

    // 11. 水旁 veryDeepUnderFloor: 沙漠 → sandstone
    if (sandstone) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::Desert}),
            ifTrue(waterStartCheck(-6, -1), ifTrue(veryDeepUnderFloor(), blockState(sandstone)))));
    }

    // 12. ON_FLOOR: 冰冻峰/尖峭山峰 → STONE（最终 onFloor 规则区块）
    // MC 源码: ifTrue(ON_FLOOR, sequence(ifTrue(isBiome(FROZEN_PEAKS, JAGGED_PEAKS), STONE), ...))
    if (stone) {
        surfaceRules.push_back(
            ifTrue(isBiome({Biomes::FrozenPeaks, Biomes::JaggedPeaks}), ifTrue(onFloor(), blockState(stone))));
    }

    // 13. ON_FLOOR: 温暖海洋/温水海洋 → sand/sandstone
    if (sand && sandstone) {
        surfaceRules.push_back(ifTrue(isBiome({Biomes::WarmOcean, Biomes::LukewarmOcean, Biomes::DeepLukewarmOcean}),
            ifTrue(onFloor(), sequence(ifTrue(onCeiling(), blockState(sandstone)), blockState(sand)))));
    }

    // 14. ON_FLOOR: 默认 → gravel/stone（MC 原版最后一条规则）
    if (gravel && stone) {
        surfaceRules.push_back(ifTrue(onFloor(), sequence(ifTrue(onCeiling(), blockState(stone)), blockState(gravel))));
    }

    // MC 源码：abovePreliminarySurface 在 deepslate 之前，sequence 短路求值
    // 使得表面规则优先于 deepslate 应用
    rules.push_back(ifTrue(abovePreliminarySurface(), sequence(std::move(surfaceRules))));

    // 3. 深板岩层 (Y 0-8 渐变过渡) — 在 abovePreliminarySurface 之后
    // MC 源码中 deepslate 位于 abovePreliminarySurface 之后，
    // sequence 短路求值使得表面规则对 abovePreliminarySurface 的位置优先生效
    if (deepslate) {
        rules.push_back(
            ifTrue(verticalGradient("minecraft:deepslate", VerticalAnchor::absolute(0), VerticalAnchor::absolute(8)),
                blockState(deepslate)));
    }

    return sequence(std::move(rules));
}

// ============================================================================

std::unique_ptr<SurfaceRule> nether()
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

    // MC 1.21: 下界噪声条件使用 Noises:: 注册名称
    auto netherStateSelector = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(noise::Noises::NETHER_STATE_SELECTOR, 0.0);
    };
    auto patchNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(noise::Noises::PATCH, -0.012);
    };
    auto netherrackNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(noise::Noises::NETHERRACK, 0.54);
    };
    auto netherWartNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(noise::Noises::NETHER_WART, 1.17);
    };
    auto soulSandLayerNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(noise::Noises::SOUL_SAND_LAYER, -0.012);
    };
    auto gravelLayerNoise = [&]() -> std::unique_ptr<SurfaceCondition> {
        return noiseCondition(noise::Noises::GRAVEL_LAYER, -0.012);
    };

    // MC 1.21.11: 共享的 PATCH 砾石层规则
    auto patchGravelRule = [&]() -> std::unique_ptr<SurfaceRule> {
        if (!gravel) {
            return blockState(nullptr);
        }
        return ifTrue(patchNoise(),
            ifTrue(yStartCheck(VerticalAnchor::absolute(30), 0),
                ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(35), 0)), blockState(gravel))));
    };

    std::vector<std::unique_ptr<SurfaceRule>> rules;

    // 1. 基岩层底部
    // MC 1.21: verticalGradient("minecraft:bedrock_floor", aboveBottom(0), aboveBottom(5))
    if (bedrock) {
        rules.push_back(ifTrue(
            verticalGradient("minecraft:bedrock_floor", VerticalAnchor::aboveBottom(0), VerticalAnchor::aboveBottom(5)),
            blockState(bedrock)));
    }

    // 2. 基岩层顶部
    // MC 1.21: not(verticalGradient("minecraft:bedrock_roof", belowTop(5), top()))
    if (bedrock) {
        rules.push_back(ifTrue(notCondition(verticalGradient(
                                   "minecraft:bedrock_roof", VerticalAnchor::belowTop(5), VerticalAnchor::belowTop(0))),
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
            underFloorSeq.push_back(patchGravelRule());
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
            std::vector<std::unique_ptr<SurfaceRule>> underCeilingSeq;
            underCeilingSeq.push_back(ifTrue(netherStateSelector(), blockState(soulSand)));
            underCeilingSeq.push_back(blockState(soulSoil));
            soulSandValleySeq.push_back(ifTrue(underCeiling(), sequence(std::move(underCeilingSeq))));
        }
        {
            std::vector<std::unique_ptr<SurfaceRule>> underFloorSeq;
            underFloorSeq.push_back(patchGravelRule());
            underFloorSeq.push_back(ifTrue(netherStateSelector(), blockState(soulSand)));
            underFloorSeq.push_back(blockState(soulSoil));
            soulSandValleySeq.push_back(ifTrue(underFloor(), sequence(std::move(underFloorSeq))));
        }
        rules.push_back(ifTrue(isBiome({Biomes::SoulSandValley}), sequence(std::move(soulSandValleySeq))));
    }

    // 6. ON_FLOOR 通用规则
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

        // 7a. UNDER_FLOOR soul_sand_layer
        {
            std::vector<std::unique_ptr<SurfaceRule>> soulSandLayerSeq;
            soulSandLayerSeq.push_back(ifTrue(notCondition(hole()),
                ifTrue(yStartCheck(VerticalAnchor::absolute(30), 0),
                    ifTrue(notCondition(yStartCheck(VerticalAnchor::absolute(35), 0)), blockState(soulSand)))));
            soulSandLayerSeq.push_back(blockState(netherrack));
            netherWastesSeq.push_back(
                ifTrue(underFloor(), ifTrue(soulSandLayerNoise(), sequence(std::move(soulSandLayerSeq)))));
        }

        // 7b. ON_FLOOR gravel_layer
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

} // namespace mc::world::gen::surface
