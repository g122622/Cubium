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

/**
 * @file BiomeColorsTest.cpp
 * @brief 生物群系颜色系统单元测试
 *
 * 测试 BiomeColors、ColorResolver、BiomeEffects 的颜色解析功能。
 */

#include "client/world/color/BiomeColors.hpp"
#include "client/world/color/ColorResolver.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace client {
namespace test {

using namespace world::biome;

/**
 * @brief 测试 BiomeEffects 默认颜色
 */
TEST(BiomeEffectsTest, DefaultColors)
{
    BiomeEffects effects;

    EXPECT_EQ(effects.waterColor(), BiomeEffects::DEFAULT_WATER_COLOR);
    EXPECT_EQ(effects.waterFogColor(), BiomeEffects::DEFAULT_WATER_FOG_COLOR);
    EXPECT_EQ(effects.fogColor(), BiomeEffects::DEFAULT_FOG_COLOR);
    EXPECT_EQ(effects.skyColor(), BiomeEffects::DEFAULT_SKY_COLOR);
    EXPECT_EQ(effects.grassColorModifier(), GrassColorModifier::None);
    EXPECT_FALSE(effects.grassColor().has_value());
    EXPECT_FALSE(effects.foliageColor().has_value());
}

/**
 * @brief 测试 BiomeEffects Builder
 */
TEST(BiomeEffectsTest, BuilderPattern)
{
    auto effects = BiomeEffects::Builder()
                       .waterColor(0x123456)
                       .waterFogColor(0x789ABC)
                       .fogColor(0xDEF012)
                       .skyColor(0x345678)
                       .grassColor(0x9ABCDEF)
                       .foliageColor(0x111111)
                       .grassColorModifier(GrassColorModifier::Swamp)
                       .build();

    EXPECT_EQ(effects.waterColor(), 0x123456);
    EXPECT_EQ(effects.waterFogColor(), 0x789ABC);
    EXPECT_EQ(effects.fogColor(), 0xDEF012);
    EXPECT_EQ(effects.skyColor(), 0x345678);
    EXPECT_TRUE(effects.grassColor().has_value());
    EXPECT_EQ(effects.grassColor().value(), 0x9ABCDEF);
    EXPECT_TRUE(effects.foliageColor().has_value());
    EXPECT_EQ(effects.foliageColor().value(), 0x111111);
    EXPECT_EQ(effects.grassColorModifier(), GrassColorModifier::Swamp);
}

/**
 * @brief 测试 BiomeEffects 特殊生物群系常量
 */
TEST(BiomeEffectsTest, SpecialBiomeColors)
{
    // 沼泽颜色
    EXPECT_EQ(BiomeEffects::SWAMP_WATER_COLOR, 0x617B64);
    EXPECT_EQ(BiomeEffects::SWAMP_WATER_FOG_COLOR, 0x232817);
    EXPECT_EQ(BiomeEffects::SWAMP_FOG_COLOR, 0x7E8E8E);
    EXPECT_EQ(BiomeEffects::SWAMP_GRASS_COLOR, 0x6A7039);
    EXPECT_EQ(BiomeEffects::SWAMP_GRASS_COLOR_DARK, 0x4C613C);
    EXPECT_EQ(BiomeEffects::SWAMP_FOLIAGE_COLOR, 0x6A7039);
    EXPECT_EQ(BiomeEffects::SWAMP_FOLIAGE_COLOR_DARK, 0x4C613C);

    // 冻洋颜色
    EXPECT_EQ(BiomeEffects::FROZEN_OCEAN_WATER_COLOR, 0x3938C9);

    // 暖水海洋颜色
    EXPECT_EQ(BiomeEffects::WARM_OCEAN_WATER_COLOR, 0x43D5EE);
    EXPECT_EQ(BiomeEffects::WARM_OCEAN_WATER_FOG_COLOR, 0x041F33);

    // 温水海洋颜色
    EXPECT_EQ(BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR, 0x45ADF2);
    EXPECT_EQ(BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR, 0x0E4673);

    // 冷水海洋颜色
    EXPECT_EQ(BiomeEffects::COLD_OCEAN_WATER_COLOR, 0x3D57E6);
    EXPECT_EQ(BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR, 0x1A3AA3);

    // 恶地颜色
    EXPECT_EQ(BiomeEffects::BADLANDS_GRASS_COLOR, 0x90814D);
    EXPECT_EQ(BiomeEffects::BADLANDS_FOLIAGE_COLOR, 0x9E814D);

    // 黑森林颜色
    EXPECT_EQ(BiomeEffects::DARK_FOREST_GRASS_COLOR, 0x507A50);
}

/**
 * @brief 测试 BiomeColors 常量（仅云杉和桦树叶）
 */
TEST(BiomeColorsTest, ColorConstants)
{
    // 云杉和桦树树叶颜色（这些是固定值，不属于 BiomeEffects）
    EXPECT_EQ(BiomeColors::SPRUCE_LEAVES_COLOR, 0x619961);
    EXPECT_EQ(BiomeColors::BIRCH_LEAVES_COLOR, 0x80A755);

    // 注意：沼泽、黑森林、恶地的颜色常量定义在 BiomeEffects 中
    // 使用 BiomeEffects::SWAMP_GRASS_COLOR 等
    EXPECT_EQ(BiomeEffects::SWAMP_GRASS_COLOR, 0x6A7039);
    EXPECT_EQ(BiomeEffects::SWAMP_FOLIAGE_COLOR, 0x6A7039);
    EXPECT_EQ(BiomeEffects::DARK_FOREST_GRASS_COLOR, 0x507A50);
    EXPECT_EQ(BiomeEffects::BADLANDS_GRASS_COLOR, 0x90814D);
    EXPECT_EQ(BiomeEffects::BADLANDS_FOLIAGE_COLOR, 0x9E814D);
}

/**
 * @brief 测试沼泽颜色计算
 */
TEST(BiomeColorsTest, SwampColorCalculation)
{
    // 测试沼泽颜色计算的确定性
    // 同一坐标应返回相同颜色
    u32 color1 = BiomeColors::calculateSwampColor(100.0, 200.0, 0x6A7039, 0x4C613C);
    u32 color2 = BiomeColors::calculateSwampColor(100.0, 200.0, 0x6A7039, 0x4C613C);
    EXPECT_EQ(color1, color2);

    // 不同坐标可能返回不同颜色
    // 由于噪声算法，无法确定具体颜色，但可以验证返回的是两种颜色之一
    u32 color3 = BiomeColors::calculateSwampColor(1000.0, 2000.0, 0x6A7039, 0x4C613C);
    EXPECT_TRUE(color3 == 0x6A7039 || color3 == 0x4C613C);

    // 测试极端坐标
    u32 color4 = BiomeColors::calculateSwampColor(0.0, 0.0, 0x6A7039, 0x4C613C);
    EXPECT_TRUE(color4 == 0x6A7039 || color4 == 0x4C613C);

    u32 color5 = BiomeColors::calculateSwampColor(-100.0, -200.0, 0x6A7039, 0x4C613C);
    EXPECT_TRUE(color5 == 0x6A7039 || color5 == 0x4C613C);
}

/**
 * @brief 测试颜色解析器单例
 */
TEST(BiomeColorsTest, ResolverSingletons)
{
    // 测试解析器单例是否正常工作
    const auto& grassResolver = BiomeColors::grassColorResolver();
    const auto& foliageResolver = BiomeColors::foliageColorResolver();
    const auto& dryFoliageResolver = BiomeColors::dryFoliageColorResolver();
    const auto& waterResolver = BiomeColors::waterColorResolver();

    EXPECT_NE(&grassResolver, nullptr);
    EXPECT_NE(&foliageResolver, nullptr);
    EXPECT_NE(&dryFoliageResolver, nullptr);
    EXPECT_NE(&waterResolver, nullptr);

    // 再次获取应该是同一个实例
    const auto& grassResolver2 = BiomeColors::grassColorResolver();
    const auto& dryFoliageResolver2 = BiomeColors::dryFoliageColorResolver();
    EXPECT_EQ(&grassResolver, &grassResolver2);
    EXPECT_EQ(&dryFoliageResolver, &dryFoliageResolver2);

    // 不同解析器应为不同实例
    EXPECT_NE(&dryFoliageResolver, &foliageResolver);
}

/**
 * @brief 创建测试用生物群系
 */
class TestBiomeForColor : public Biome {
public:
    TestBiomeForColor(const BiomeEffects& effects)
        : Biome(BiomeId(1), "test_biome")
    {
        setEffects(effects);
    }
};

/**
 * @brief 测试 WaterColorResolver
 */
TEST(WaterColorResolverTest, BasicResolution)
{
    WaterColorResolver resolver;

    // 测试默认水颜色
    BiomeEffects defaultEffects;
    TestBiomeForColor defaultBiome(defaultEffects);
    EXPECT_EQ(resolver.getColor(defaultBiome, 0.0, 0.0), BiomeEffects::DEFAULT_WATER_COLOR);

    // 测试自定义水颜色
    BiomeEffects customEffects = BiomeEffects::Builder().waterColor(0x123456).build();
    TestBiomeForColor customBiome(customEffects);
    EXPECT_EQ(resolver.getColor(customBiome, 0.0, 0.0), 0x123456);

    // 测试沼泽水颜色
    BiomeEffects swampEffects = BiomeEffects::Builder().waterColor(BiomeEffects::SWAMP_WATER_COLOR).build();
    TestBiomeForColor swampBiome(swampEffects);
    EXPECT_EQ(resolver.getColor(swampBiome, 0.0, 0.0), BiomeEffects::SWAMP_WATER_COLOR);

    // 测试暖水海洋颜色
    BiomeEffects warmOceanEffects = BiomeEffects::Builder().waterColor(BiomeEffects::WARM_OCEAN_WATER_COLOR).build();
    TestBiomeForColor warmOceanBiome(warmOceanEffects);
    EXPECT_EQ(resolver.getColor(warmOceanBiome, 0.0, 0.0), BiomeEffects::WARM_OCEAN_WATER_COLOR);
}

/**
 * @brief 测试 GrassColorResolver
 */
TEST(GrassColorResolverTest, BasicResolution)
{
    GrassColorResolver resolver;

    // 测试无覆盖颜色时返回 colormap 标记
    BiomeEffects defaultEffects;
    TestBiomeForColor defaultBiome(defaultEffects);
    EXPECT_EQ(resolver.getColor(defaultBiome, 0.0, 0.0), 0xFFFFFFFF);

    // 测试覆盖颜色
    BiomeEffects overrideEffects = BiomeEffects::Builder().grassColor(0x9ABCDEF).build();
    TestBiomeForColor overrideBiome(overrideEffects);
    EXPECT_EQ(resolver.getColor(overrideBiome, 0.0, 0.0), 0x9ABCDEF);
}

/**
 * @brief 测试 GrassColorResolver - 沼泽颜色修改器
 */
TEST(GrassColorResolverTest, SwampModifier)
{
    GrassColorResolver resolver;

    BiomeEffects swampEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::Swamp).build();
    TestBiomeForColor swampBiome(swampEffects);

    // 沼泽颜色应该返回双色混合之一
    u32 color = resolver.getColor(swampBiome, 100.0, 200.0);
    EXPECT_TRUE(color == BiomeEffects::SWAMP_GRASS_COLOR || color == BiomeEffects::SWAMP_GRASS_COLOR_DARK);
}

/**
 * @brief 测试 GrassColorResolver - 黑森林颜色修改器
 */
TEST(GrassColorResolverTest, DarkForestModifier)
{
    GrassColorResolver resolver;

    BiomeEffects darkForestEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::DarkForest).build();
    TestBiomeForColor darkForestBiome(darkForestEffects);

    // 黑森林应返回固定颜色
    EXPECT_EQ(resolver.getColor(darkForestBiome, 0.0, 0.0), BiomeEffects::DARK_FOREST_GRASS_COLOR);
}

/**
 * @brief 测试 GrassColorResolver - 恶地颜色修改器
 */
TEST(GrassColorResolverTest, BadlandsModifier)
{
    GrassColorResolver resolver;

    BiomeEffects badlandsEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::Badlands).build();
    TestBiomeForColor badlandsBiome(badlandsEffects);

    // 恶地应返回固定颜色
    EXPECT_EQ(resolver.getColor(badlandsBiome, 0.0, 0.0), BiomeEffects::BADLANDS_GRASS_COLOR);
}

/**
 * @brief 测试 FoliageColorResolver
 */
TEST(FoliageColorResolverTest, BasicResolution)
{
    FoliageColorResolver resolver;

    // 测试无覆盖颜色时返回 colormap 标记
    BiomeEffects defaultEffects;
    TestBiomeForColor defaultBiome(defaultEffects);
    EXPECT_EQ(resolver.getColor(defaultBiome, 0.0, 0.0), 0xFFFFFFFF);

    // 测试覆盖颜色
    BiomeEffects overrideEffects = BiomeEffects::Builder().foliageColor(0x123456).build();
    TestBiomeForColor overrideBiome(overrideEffects);
    EXPECT_EQ(resolver.getColor(overrideBiome, 0.0, 0.0), 0x123456);
}

/**
 * @brief 测试 FoliageColorResolver - 沼泽颜色
 */
TEST(FoliageColorResolverTest, SwampFoliage)
{
    FoliageColorResolver resolver;

    BiomeEffects swampEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::Swamp).build();
    TestBiomeForColor swampBiome(swampEffects);

    // 沼泽树叶颜色应该返回双色混合之一
    u32 color = resolver.getColor(swampBiome, 100.0, 200.0);
    EXPECT_TRUE(color == BiomeEffects::SWAMP_FOLIAGE_COLOR || color == BiomeEffects::SWAMP_FOLIAGE_COLOR_DARK);
}

/**
 * @brief 测试 FoliageColorResolver - 恶地颜色
 */
TEST(FoliageColorResolverTest, BadlandsFoliage)
{
    FoliageColorResolver resolver;

    BiomeEffects badlandsEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::Badlands).build();
    TestBiomeForColor badlandsBiome(badlandsEffects);

    // 恶地树叶应返回固定颜色
    EXPECT_EQ(resolver.getColor(badlandsBiome, 0.0, 0.0), BiomeEffects::BADLANDS_FOLIAGE_COLOR);
}

/**
 * @brief 测试 DryFoliageColorResolver - 基本解析
 *
 * 对应原版 BiomeColors.DRY_FOLIAGE_COLOR_RESOLVER：
 * - 有 dryFoliageColor 覆盖时返回覆盖色
 * - 无覆盖时返回 0xFFFFFFFF 标记，由调用方走 dry_foliage colormap
 */
TEST(DryFoliageColorResolverTest, BasicResolution)
{
    DryFoliageColorResolver resolver;

    // 无覆盖颜色时返回 colormap 标记
    BiomeEffects defaultEffects;
    TestBiomeForColor defaultBiome(defaultEffects);
    EXPECT_EQ(resolver.getColor(defaultBiome, 0.0, 0.0), 0xFFFFFFFF);

    // 有覆盖颜色时返回覆盖色
    BiomeEffects overrideEffects = BiomeEffects::Builder().dryFoliageColor(0x123456).build();
    TestBiomeForColor overrideBiome(overrideEffects);
    EXPECT_EQ(resolver.getColor(overrideBiome, 0.0, 0.0), 0x123456);
}

/**
 * @brief 测试 DryFoliageColorResolver - 不受 grassColorModifier 影响
 *
 * 与 grass/foliage 不同，dry foliage 没有沼泽/恶地修改器分支：
 * 即使设置了 grassColorModifier，无 dryFoliageColor 覆盖仍返回 colormap 标记。
 */
TEST(DryFoliageColorResolverTest, IgnoresGrassColorModifier)
{
    DryFoliageColorResolver resolver;

    BiomeEffects swampEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::Swamp).build();
    TestBiomeForColor swampBiome(swampEffects);
    // 沼泽修改器不应影响干枯植被：无覆盖即返回 colormap 标记
    EXPECT_EQ(resolver.getColor(swampBiome, 100.0, 200.0), 0xFFFFFFFF);

    BiomeEffects badlandsEffects = BiomeEffects::Builder().grassColorModifier(GrassColorModifier::Badlands).build();
    TestBiomeForColor badlandsBiome(badlandsEffects);
    EXPECT_EQ(resolver.getColor(badlandsBiome, 0.0, 0.0), 0xFFFFFFFF);

    // 覆盖色优先于修改器
    BiomeEffects overrideWithModifier =
        BiomeEffects::Builder().dryFoliageColor(0xABCDEF).grassColorModifier(GrassColorModifier::Swamp).build();
    TestBiomeForColor overrideBiome(overrideWithModifier);
    EXPECT_EQ(resolver.getColor(overrideBiome, 100.0, 200.0), 0xABCDEF);
}

/**
 * @brief 测试颜色优先级：覆盖颜色 > 颜色修改器
 */
TEST(ColorResolverTest, OverridePriority)
{
    GrassColorResolver resolver;

    // 当同时有覆盖颜色和修改器时，覆盖颜色优先
    BiomeEffects overrideWithModifier =
        BiomeEffects::Builder().grassColor(0xABCDEF).grassColorModifier(GrassColorModifier::Swamp).build();
    TestBiomeForColor biome(overrideWithModifier);

    // 应该返回覆盖颜色，而不是沼泽双色混合
    EXPECT_EQ(resolver.getColor(biome, 100.0, 200.0), 0xABCDEF);
}

/**
 * @brief 测试 ColorResolver 接口多态
 */
TEST(ColorResolverTest, Polymorphism)
{
    // 测试通过基类指针调用
    std::unique_ptr<ColorResolver> grassResolver = std::make_unique<GrassColorResolver>();
    std::unique_ptr<ColorResolver> foliageResolver = std::make_unique<FoliageColorResolver>();
    std::unique_ptr<ColorResolver> waterResolver = std::make_unique<WaterColorResolver>();

    BiomeEffects effects =
        BiomeEffects::Builder().waterColor(0x123456).grassColor(0xABCDEF).foliageColor(0x111222).build();
    TestBiomeForColor biome(effects);

    EXPECT_EQ(grassResolver->getColor(biome, 0.0, 0.0), 0xABCDEF);
    EXPECT_EQ(foliageResolver->getColor(biome, 0.0, 0.0), 0x111222);
    EXPECT_EQ(waterResolver->getColor(biome, 0.0, 0.0), 0x123456);
}

} // namespace test
} // namespace client
} // namespace mc
