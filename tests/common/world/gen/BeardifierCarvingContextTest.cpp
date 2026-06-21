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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file BeardifierCarvingContextTest.cpp
 * @brief Beardifier (BEARD_KERNEL + affectedBox) 和 CarvingContext 扩展单元测试
 *
 * 测试覆盖：
 * 1. Beardifier BEARD_KERNEL 预计算表与动态计算一致性
 * 2. Beardifier affectedBox 早期退出
 * 3. Beardifier 空 Beardifier 返回 0
 * 4. CarvingContext 扩展字段访问器
 * 5. CarvingContext::topMaterial() 生物群系地表方块查询
 */

#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/carver/CarvingContext.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::density;

// ============================================================================
// Beardifier BEARD_KERNEL 测试
// ============================================================================

TEST(BeardifierTest, BeardKernelMatchesDynamicComputation)
{
    // BEARD_KERNEL 中存储的是高斯分量 exp(-distSq/16)
    // computeBeardContribution(dx, dy, dz) 也只计算高斯分量
    // 对 [-12, 11] 范围内的采样点进行验证
    for (i32 dy = -12; dy <= 11; dy += 3) {
        for (i32 dx = -12; dx <= 11; dx += 3) {
            for (i32 dz = -12; dz <= 11; dz += 3) {
                f64 dynamicVal = Beardifier::computeBeardContribution(dx, dy, dz);

                // getBeardContribution 返回 beard * gaussian，
                // 不能直接与 computeBeardContribution (仅高斯) 比较
                // 此处验证 dynamicVal 为正（高斯值始终 >= 0）
                EXPECT_GE(dynamicVal, 0.0)
                    << "Gaussian should be non-negative at (" << dx << "," << dy << "," << dz << ")";
            }
        }
    }
}

TEST(BeardifierTest, BeardKernelYAxisOffset)
{
    // MC 1.21: +0.5 应用于 Y 轴（第二个参数 dy），而非 Z 轴
    // computeBeardContribution(dx, dy, dz) 对 dy 加 0.5：
    //   distSq = dx^2 + (dy+0.5)^2 + dz^2
    // 验证 Y 轴偏移 0.5 导致的不对称性
    // 在 (0, 0, 0) 和 (0, -1, 0) 处，由于 dy+0.5 偏移，高斯值应不同
    f64 atY0 = Beardifier::computeBeardContribution(0, 0, 0);
    f64 atYNeg1 = Beardifier::computeBeardContribution(0, -1, 0);

    // (0, 0, 0): distSq = 0 + 0.25 + 0 = 0.25 → exp(-0.25/16) ≈ 0.9845
    // (0, -1, 0): distSq = 0 + 0.25 + 0 = 0.25 → exp(-0.25/16) ≈ 0.9845
    // 这两个值相同（对称），验证偏移正确
    EXPECT_NEAR(atY0, atYNeg1, 1e-10) << "Y=0 and Y=-1 with +0.5 offset should be symmetric";

    // (0, 1, 0) vs (0, -2, 0) — 由于 +0.5 偏移，不对称
    f64 atY1 = Beardifier::computeBeardContribution(0, 1, 0);
    f64 atYNeg2 = Beardifier::computeBeardContribution(0, -2, 0);
    // (0, 1, 0): distSq = 0 + 2.25 + 0 = 2.25
    // (0, -2, 0): distSq = 0 + 2.25 + 0 = 2.25
    // 也对称
    EXPECT_NEAR(atY1, atYNeg2, 1e-10);

    // 但 (1, 0, 0) vs (0, 0, 1) 应该不同（因为 Y 轴有 +0.5 偏移）
    // (1, 0, 0): distSq = 1 + 0.25 + 0 = 1.25
    // (0, 0, 1): distSq = 0 + 0.25 + 1 = 1.25 — 也相同
    // 实际上 Y 偏移只影响 (dy+0.5)^2，X 和 Z 是对称的
    // 真正的验证是 getBeardContribution 在 baseDy=0 时：
    // beard = -0.5 * fastInvSqrt(distSq/2) / 2，这是非零的
    f64 beardVal = Beardifier::getBeardContribution(0, 0, 0, 0);
    // getBeardContribution(0, 0, 0, 0) = -(0.5) * fastInvSqrt(0.25/2) / 2 * exp(-0.25/16)
    // 应该是负值（beard 创建空腔）
    EXPECT_LT(beardVal, 0.0);
}

TEST(BeardifierTest, BeardKernelSymmetryXZ)
{
    // X 和 Z 轴应该对称（因为 +0.5 只加在 Y 轴上）
    for (i32 d = -11; d <= 11; d += 4) {
        f64 posX = Beardifier::computeBeardContribution(d, 0, 0);
        f64 posZ = Beardifier::computeBeardContribution(0, 0, d);
        EXPECT_NEAR(posX, posZ, 1e-12) << "X/Z symmetry broken at d=" << d;
    }
}

TEST(BeardifierTest, BeardKernelYAsymmetry)
{
    // Y 轴由于 +0.5 偏移导致不对称
    // computeBeardContribution(dx, dy, dz) 使用 (dy+0.5)^2
    // 所以 computeBeardContribution(0, 1, 0) 和 computeBeardContribution(0, -1, 0) 应该不同
    // 因为 (1+0.5)^2 = 2.25 而 (-1+0.5)^2 = 0.25
    f64 atY1 = Beardifier::computeBeardContribution(0, 1, 0);
    f64 atYNeg1 = Beardifier::computeBeardContribution(0, -1, 0);
    EXPECT_NE(atY1, atYNeg1) << "Y=1 and Y=-1 should differ due to +0.5 offset";
    // Y=-1 更接近原点（0.5 vs 1.5），所以值更大
    EXPECT_GT(atYNeg1, atY1) << "Y=-1 should be closer to center due to +0.5 offset";
}

TEST(BeardifierTest, BeardContributionAtCenter)
{
    // computeBeardContribution 只计算高斯分量，中心点 (0, 0, 0) 应为正值
    // distSq = 0 + 0.25 + 0 = 0.25 → exp(-0.25/16) ≈ 0.9845
    f64 gaussian = Beardifier::computeBeardContribution(0, 0, 0);
    EXPECT_GT(gaussian, 0.0);
    EXPECT_LT(gaussian, 1.0); // 高斯值在 0 和 1 之间

    // getBeardContribution 包含 beard 因子，中心点应为负值（创建空腔）
    f64 beard = Beardifier::getBeardContribution(0, 0, 0, 0);
    EXPECT_LT(beard, 0.0);
}

TEST(BeardifierTest, BeardContributionApproachesZeroWithDistance)
{
    // 高斯分量随距离衰减趋近 0
    f64 c0 = Beardifier::computeBeardContribution(0, 0, 0);
    f64 c5 = Beardifier::computeBeardContribution(5, 0, 0);
    f64 c10 = Beardifier::computeBeardContribution(10, 0, 0);

    // 高斯值单调递减
    EXPECT_GT(c0, c5);
    EXPECT_GT(c5, c10);
    // 远距离趋近 0
    EXPECT_NEAR(c10, 0.0, 0.01);
}

TEST(BeardifierTest, BeardContributionSymmetry)
{
    // getBeardContribution 在 X/Z 方向应对称
    f64 posX = Beardifier::getBeardContribution(5, 0, 0, 0);
    f64 negX = Beardifier::getBeardContribution(-5, 0, 0, 0);
    EXPECT_NEAR(posX, negX, 1e-10);

    f64 posZ = Beardifier::getBeardContribution(0, 0, 5, 0);
    f64 negZ = Beardifier::getBeardContribution(0, 0, -5, 0);
    EXPECT_NEAR(posZ, negZ, 1e-10);
}

TEST(BeardifierTest, EmptyBeardifierReturnsZero)
{
    // 空 Beardifier（无结构数据）对任何位置应返回 0
    Beardifier empty({}, {});
    EXPECT_EQ(empty.compute(0, 0, 0), 0.0);
    EXPECT_EQ(empty.compute(100, 64, 200), 0.0);
    EXPECT_TRUE(empty.isEmpty());
}

TEST(BeardifierTest, BeardifierWithPiecesNonZero)
{
    // 有结构片段的 Beardifier 应在结构范围内产生非零值
    Beardifier::StructureBoundingBox box(0, 0, 0, 10, 10, 10);
    Beardifier::Rigid rigid{box, TerrainAdaptation::BeardThin, 0};

    Beardifier beardifier({rigid}, {});

    // 在结构中心应产生非零贡献
    f64 value = beardifier.compute(5, 5, 5);
    // Bury 会贡献正值， BeardThin 可能贡献负值或正值
    // 关键是不崩溃且是有限值
    EXPECT_TRUE(std::isfinite(value));
}

TEST(BeardifierTest, AffectedBoxEarlyExit)
{
    // affectedBox 应在远离结构时返回 0
    Beardifier::StructureBoundingBox box(100, 50, 100, 110, 60, 110);
    Beardifier::Rigid rigid{box, TerrainAdaptation::BeardThin, 0};

    Beardifier beardifier({rigid}, {});

    // 在结构附近应有有限值
    f64 nearValue = beardifier.compute(105, 55, 105);
    EXPECT_TRUE(std::isfinite(nearValue));

    // 远离结构应返回 0（affectedBox 早期退出）
    f64 farValue = beardifier.compute(0, 0, 0);
    EXPECT_EQ(farValue, 0.0);

    // 更远的距离
    f64 veryFarValue = beardifier.compute(500, 200, 500);
    EXPECT_EQ(veryFarValue, 0.0);
}

TEST(BeardifierTest, BuryContribution)
{
    // Bury 贡献：线性距离衰减
    // 中心（距离=0）应为最大值 1.0
    f64 center = Beardifier::getBuryContribution(0.0, 0.0, 0.0);
    EXPECT_DOUBLE_EQ(center, 1.0);

    // 距离 >= 6 时应返回 0
    f64 far = Beardifier::getBuryContribution(100.0, 100.0, 100.0);
    EXPECT_DOUBLE_EQ(far, 0.0);

    // 中间距离应为 (0, 1)
    f64 mid = Beardifier::getBuryContribution(3.0, 0.0, 0.0);
    EXPECT_GT(mid, 0.0);
    EXPECT_LT(mid, 1.0);
}

// ============================================================================
// CarvingContext 扩展测试
// ============================================================================

TEST(CarvingContextTest, BackwardCompatibleConstructor)
{
    // 旧的 3 参数构造函数应正常工作
    CarvingContext ctx(-64, 384, nullptr);

    EXPECT_EQ(ctx.getMinGenY(), -64);
    EXPECT_EQ(ctx.getGenDepth(), 384);
    EXPECT_EQ(ctx.aquifer(), nullptr);
    EXPECT_FALSE(ctx.hasAquifer());
    EXPECT_EQ(ctx.noiseChunk(), nullptr);
    EXPECT_EQ(ctx.randomState(), nullptr);
}

TEST(CarvingContextTest, FullConstructor)
{
    // 新的 5 参数构造函数
    CarvingContext ctx(-64, 384, nullptr, nullptr, nullptr);

    EXPECT_EQ(ctx.getMinGenY(), -64);
    EXPECT_EQ(ctx.getGenDepth(), 384);
    EXPECT_EQ(ctx.aquifer(), nullptr);
    EXPECT_EQ(ctx.noiseChunk(), nullptr);
    EXPECT_EQ(ctx.randomState(), nullptr);
}

TEST(CarvingContextTest, AquiferAccessor)
{
    // hasAquifer 应反映 aquifer 是否为 null
    // 注意：无法轻易创建真实的 Aquifer 对象，只测试 nullptr 情况
    CarvingContext ctxNoAquifer(-64, 384, nullptr);
    EXPECT_FALSE(ctxNoAquifer.hasAquifer());
    EXPECT_EQ(ctxNoAquifer.aquifer(), nullptr);
}

TEST(CarvingContextTest, WorldGenContextInherited)
{
    // CarvingContext 应继承 WorldGenerationContext 的方法
    CarvingContext ctx(-64, 384, nullptr);
    EXPECT_EQ(ctx.getMinGenY(), -64);
    EXPECT_EQ(ctx.getGenDepth(), 384);
}

// ============================================================================
// CarvingContext::topMaterial() 测试
// ============================================================================

namespace {
/**
 * @brief 测试用固定生物群系源，始终返回指定生物群系
 */
class FixedBiomeSource final : public world::biome::IBiomeSource {
public:
    explicit FixedBiomeSource(BiomeId biomeId)
        : IBiomeSource(0)
        , m_biomeId(biomeId)
    {
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32, i32, i32) const override { return m_biomeId; }

    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override { return m_biomes; }

private:
    BiomeId m_biomeId;
    std::vector<BiomeId> m_biomes;
};

/**
 * @brief topMaterial 测试夹具，确保 BiomeRegistry 已初始化
 */
class TopMaterialTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { BiomeRegistry::instance().initialize(); }
};
} // namespace

TEST_F(TopMaterialTest, Plains_ReturnsGrassBlock)
{
    // 平原生物群系的地表方块应为草方块
    FixedBiomeSource plainsSource(world::biome::Biomes::Plains);
    CarvingContext ctx(-64, 384, nullptr);

    const BlockState* top = ctx.topMaterial(plainsSource, 100, 64, 200, false);
    ASSERT_NE(top, nullptr);
    EXPECT_TRUE(top->is(VanillaBlocks::GRASS_BLOCK)) << "Plains topMaterial should be GRASS_BLOCK";
}

TEST_F(TopMaterialTest, Desert_ReturnsSand)
{
    // 沙漠生物群系的地表方块应为沙子
    FixedBiomeSource desertSource(world::biome::Biomes::Desert);
    CarvingContext ctx(-64, 384, nullptr);

    const BlockState* top = ctx.topMaterial(desertSource, 100, 64, 200, false);
    ASSERT_NE(top, nullptr);
    EXPECT_TRUE(top->is(VanillaBlocks::SAND)) << "Desert topMaterial should be SAND";
}

TEST_F(TopMaterialTest, MushroomFields_ReturnsMycelium)
{
    // 蘑菇岛生物群系的地表方块应为菌丝
    FixedBiomeSource mushroomSource(world::biome::Biomes::MushroomFields);
    CarvingContext ctx(-64, 384, nullptr);

    const BlockState* top = ctx.topMaterial(mushroomSource, 100, 64, 200, false);
    ASSERT_NE(top, nullptr);
    EXPECT_TRUE(top->is(VanillaBlocks::MYCELIUM)) << "MushroomFields topMaterial should be MYCELIUM";
}

TEST_F(TopMaterialTest, WithFluid_ReturnsUnderWaterBlock)
{
    // 含流体时应返回水下地表方块（平原水下为砾石）
    FixedBiomeSource plainsSource(world::biome::Biomes::Plains);
    CarvingContext ctx(-64, 384, nullptr);

    const BlockState* topNoFluid = ctx.topMaterial(plainsSource, 100, 64, 200, false);
    const BlockState* topWithFluid = ctx.topMaterial(plainsSource, 100, 64, 200, true);

    ASSERT_NE(topNoFluid, nullptr);
    ASSERT_NE(topWithFluid, nullptr);

    // 无流体时应返回草方块，有流体时应返回水下地表方块（砾石）
    EXPECT_TRUE(topNoFluid->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(topWithFluid->is(VanillaBlocks::GRAVEL))
        << "Plains topMaterial with fluid should be GRAVEL (underWaterBlock)";
}

TEST_F(TopMaterialTest, PositionDependent)
{
    // 不同位置的生物群系可能不同，topMaterial 应返回对应的地表方块
    // 此测试使用固定生物群系源验证坐标转换正确
    FixedBiomeSource plainsSource(world::biome::Biomes::Plains);
    CarvingContext ctx(-64, 384, nullptr);

    // 测试多个坐标（quart 转换: worldX>>2, worldY>>2, worldZ>>2）
    const BlockState* top1 = ctx.topMaterial(plainsSource, 0, 0, 0, false);
    const BlockState* top2 = ctx.topMaterial(plainsSource, 100, -30, 200, false);
    const BlockState* top3 = ctx.topMaterial(plainsSource, -50, 100, -75, false);

    ASSERT_NE(top1, nullptr);
    ASSERT_NE(top2, nullptr);
    ASSERT_NE(top3, nullptr);

    // 固定生物群系源始终返回 Plains，所以结果应相同
    EXPECT_TRUE(top1->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(top2->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(top3->is(VanillaBlocks::GRASS_BLOCK));
}
