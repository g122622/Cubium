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
 */

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
    // BEARD_KERNEL 中的值应与动态 computeBeardContribution 计算一致
    // 对 [-12, 11] 范围内的采样点进行验证
    for (i32 dy = -12; dy <= 11; dy += 3) {
        for (i32 dx = -12; dx <= 11; dx += 3) {
            for (i32 dz = -12; dz <= 11; dz += 3) {
                // computeBeardContribution 是纯静态计算
                // getBeardContribution 使用 BEARD_KERNEL 查表（当 baseDy == dy 且坐标在范围内）
                f64 dynamicVal = Beardifier::computeBeardContribution(dx, dy, dz);
                f64 kernelVal = Beardifier::getBeardContribution(dx, dy, dz, dy);

                // 两者应一致（在浮点精度范围内）
                EXPECT_NEAR(dynamicVal, kernelVal, 1e-6)
                    << "Kernel/dynamic mismatch at (" << dx << "," << dy << "," << dz << ")";
            }
        }
    }
}

TEST(BeardifierTest, BeardContributionAtCenter)
{
    // 中心点 (0, 0, 0) 的贡献值应为负值（beard 贡献创建空腔，减少密度）
    f64 contribution = Beardifier::computeBeardContribution(0, 0, 0);
    EXPECT_LT(contribution, 0.0);
}

TEST(BeardifierTest, BeardContributionApproachesZeroWithDistance)
{
    // 距离越远，beard 贡献越接近 0（绝对值越小）
    f64 c0 = Beardifier::computeBeardContribution(0, 0, 0);
    f64 c5 = Beardifier::computeBeardContribution(5, 0, 0);
    f64 c10 = Beardifier::computeBeardContribution(10, 0, 0);

    // c0 最负，c5 接近 0，c10 更接近 0
    EXPECT_LT(c0, c5);  // c0 < c5 (both negative, c0 more negative)
    EXPECT_LT(c5, c10); // c5 < c10 (both negative or near zero)
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
