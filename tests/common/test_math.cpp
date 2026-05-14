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

#include <gtest/gtest.h>

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/ChunkPos.hpp"

// 使用明确的命名空间避免歧义
using mc::BlockPos;
using mc::ChunkPos;
using mc::Error;
using mc::ErrorCode;
using mc::f32;
using mc::Result;
using mc::u64;
using mc::math::approxEqual;
using mc::math::clamp;
using mc::math::exponentialDecayFactor;
using mc::math::fastInverseSqrt;
using mc::math::HALF_PI;
using mc::math::isZero;
using mc::math::lerp;
using mc::math::PI;
using mc::math::toChunkCoord;
using mc::math::toDegrees;
using mc::math::toLocalCoord;
using mc::math::toRadians;
using mc::math::Vector3f;

// 辅助函数
Result<int> divide(int a, int b)
{
    if (b == 0) {
        return Error(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;
}

// ============================================================================
// MathUtils 测试
// ============================================================================

TEST(MathUtils, ToRadiansToDegrees)
{
    EXPECT_NEAR(toRadians(180.0f), PI, 0.0001f);
    EXPECT_NEAR(toRadians(90.0f), HALF_PI, 0.0001f);
    EXPECT_NEAR(toDegrees(PI), 180.0f, 0.0001f);
    EXPECT_NEAR(toDegrees(HALF_PI), 90.0f, 0.0001f);
}

TEST(MathUtils, Clamp)
{
    EXPECT_EQ(clamp(5, 0, 10), 5);
    EXPECT_EQ(clamp(-5, 0, 10), 0);
    EXPECT_EQ(clamp(15, 0, 10), 10);
}

TEST(MathUtils, Lerp)
{
    EXPECT_NEAR(lerp(0.0f, 10.0f, 0.5f), 5.0f, 0.0001f);
    EXPECT_NEAR(lerp(0.0f, 10.0f, 0.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(lerp(0.0f, 10.0f, 1.0f), 10.0f, 0.0001f);
}

TEST(MathUtils, IsZero)
{
    EXPECT_TRUE(isZero(0.0f));
    EXPECT_TRUE(isZero(0.0000001f));
    EXPECT_FALSE(isZero(0.01f));
}

TEST(MathUtils, ApproxEqual)
{
    EXPECT_TRUE(approxEqual(1.0f, 1.0f));
    EXPECT_TRUE(approxEqual(1.0f, 1.000001f));
    EXPECT_FALSE(approxEqual(1.0f, 1.01f));
}

TEST(MathUtils, FastInverseSqrt)
{
    EXPECT_NEAR(fastInverseSqrt(1.0f), 1.0f, 0.01f);
    EXPECT_NEAR(fastInverseSqrt(4.0f), 0.5f, 0.01f);
    EXPECT_NEAR(fastInverseSqrt(9.0f), 1.0f / 3.0f, 0.01f);
}

TEST(MathUtils, ChunkCoordConversion)
{
    // 正坐标
    EXPECT_EQ(toChunkCoord(0), 0);
    EXPECT_EQ(toChunkCoord(15), 0);
    EXPECT_EQ(toChunkCoord(16), 1);
    EXPECT_EQ(toChunkCoord(31), 1);
    EXPECT_EQ(toChunkCoord(32), 2);
}

TEST(MathUtils, LocalCoordConversion)
{
    EXPECT_EQ(toLocalCoord(0), 0);
    EXPECT_EQ(toLocalCoord(15), 15);
    EXPECT_EQ(toLocalCoord(16), 0);
    EXPECT_EQ(toLocalCoord(31), 15);
}

// ============================================================================
// Vector3 测试
// ============================================================================

TEST(Vector3, Construction)
{
    Vector3f v1;
    EXPECT_FLOAT_EQ(v1.x, 0.0f);
    EXPECT_FLOAT_EQ(v1.y, 0.0f);
    EXPECT_FLOAT_EQ(v1.z, 0.0f);

    Vector3f v2(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v2.x, 1.0f);
    EXPECT_FLOAT_EQ(v2.y, 2.0f);
    EXPECT_FLOAT_EQ(v2.z, 3.0f);
}

TEST(Vector3, Arithmetic)
{
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);

    Vector3f sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);

    Vector3f diff = b - a;
    EXPECT_FLOAT_EQ(diff.x, 3.0f);
    EXPECT_FLOAT_EQ(diff.y, 3.0f);
    EXPECT_FLOAT_EQ(diff.z, 3.0f);

    Vector3f scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);
    EXPECT_FLOAT_EQ(scaled.z, 6.0f);
}

TEST(Vector3, DotProduct)
{
    Vector3f a(1.0f, 0.0f, 0.0f);
    Vector3f b(0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);
    EXPECT_FLOAT_EQ(a.dot(a), 1.0f);
}

TEST(Vector3, Length)
{
    Vector3f v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
}

TEST(Vector3, Normalize)
{
    Vector3f v(3.0f, 4.0f, 0.0f);
    Vector3f normalized = v.normalized();

    EXPECT_NEAR(normalized.length(), 1.0f, 0.0001f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
}

// ============================================================================
// BlockPos 和 ChunkPos 测试
// ============================================================================

TEST(BlockPos, Construction)
{
    BlockPos p1(10, 20, 30);
    EXPECT_EQ(p1.x, 10);
    EXPECT_EQ(p1.y, 20);
    EXPECT_EQ(p1.z, 30);
}

TEST(BlockPos, ChunkCoord)
{
    BlockPos p1(0, 0, 0);
    EXPECT_EQ(p1.chunkX(), 0);
    EXPECT_EQ(p1.chunkZ(), 0);

    BlockPos p2(16, 0, 16);
    EXPECT_EQ(p2.chunkX(), 1);
    EXPECT_EQ(p2.chunkZ(), 1);
}

TEST(BlockPos, Adjacent)
{
    BlockPos p(0, 0, 0);

    EXPECT_EQ(p.up(), BlockPos(0, 1, 0));
    EXPECT_EQ(p.down(), BlockPos(0, -1, 0));
    EXPECT_EQ(p.north(), BlockPos(0, 0, -1));
    EXPECT_EQ(p.south(), BlockPos(0, 0, 1));
    EXPECT_EQ(p.east(), BlockPos(1, 0, 0));
    EXPECT_EQ(p.west(), BlockPos(-1, 0, 0));
}

TEST(ChunkPos, Construction)
{
    ChunkPos c1(10, 20);
    EXPECT_EQ(c1.x, 10);
    EXPECT_EQ(c1.z, 20);

    BlockPos b(16, 0, 32);
    ChunkPos c2(b);
    EXPECT_EQ(c2.x, 1);
    EXPECT_EQ(c2.z, 2);
}

TEST(ChunkPos, WorldCoord)
{
    ChunkPos c(10, 20);
    EXPECT_EQ(c.worldX(), 160);
    EXPECT_EQ(c.worldZ(), 320);
}

TEST(ChunkPos, ToId)
{
    ChunkPos c1(10, 20);
    u64 id = c1.toId();

    ChunkPos c2 = ChunkPos::fromId(id);
    EXPECT_EQ(c2.x, 10);
    EXPECT_EQ(c2.z, 20);
}

// ============================================================================
// Result 测试
// ============================================================================

TEST(Error, Construction)
{
    Error e1;
    EXPECT_TRUE(e1.success());
    EXPECT_FALSE(e1.failed());

    Error e2(ErrorCode::NotFound, "Resource not found");
    EXPECT_FALSE(e2.success());
    EXPECT_TRUE(e2.failed());
    EXPECT_EQ(static_cast<int>(e2.code()), static_cast<int>(ErrorCode::NotFound));
}

TEST(ResultVoid, Success)
{
    Result<void> result;
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.failed());
}

TEST(ResultVoid, Failure)
{
    Result<void> result(Error(ErrorCode::NotFound, "Not found"));
    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());
}

TEST(ResultT, SuccessWithValue)
{
    Result<int> result(42);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultT, Failure)
{
    Result<int> result(Error(ErrorCode::NotFound, "Not found"));
    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());
}

TEST(ResultT, ValueOr)
{
    Result<int> success{42};
    Result<int> failure{Error(ErrorCode::NotFound, "")};

    EXPECT_EQ(success.valueOr(0), 42);
    EXPECT_EQ(failure.valueOr(0), 0);
}

TEST(Result, RealWorldUsage)
{
    auto result1 = divide(10, 2);
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 5);

    auto result2 = divide(10, 0);
    EXPECT_FALSE(result2.success());
}

// ============================================================================
// ExponentialDecayFactor 测试 (帧率无关的时间纠正因子)
// ============================================================================

TEST(ExponentialDecayFactor, ZeroDeltaTime)
{
    // deltaTime 为 0 时，纠正因子应为 0
    EXPECT_FLOAT_EQ(exponentialDecayFactor(0.5f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(exponentialDecayFactor(1.0f, 0.0f), 0.0f);
}

TEST(ExponentialDecayFactor, ZeroRate)
{
    // ratePerSecond 为 0 时，纠正因子应为 0
    EXPECT_FLOAT_EQ(exponentialDecayFactor(0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(exponentialDecayFactor(0.0f, 0.5f), 0.0f);
}

TEST(ExponentialDecayFactor, FullRate)
{
    // ratePerSecond 为 1 时，纠正因子应为 1（立即纠正）
    EXPECT_FLOAT_EQ(exponentialDecayFactor(1.0f, 0.016f), 1.0f);
    EXPECT_FLOAT_EQ(exponentialDecayFactor(1.0f, 1.0f), 1.0f);
}

TEST(ExponentialDecayFactor, HalfRateOneSecond)
{
    // ratePerSecond = 0.5, deltaTime = 1.0 时
    // correctionFactor = 1 - (1 - 0.5)^1 = 0.5
    EXPECT_FLOAT_EQ(exponentialDecayFactor(0.5f, 1.0f), 0.5f);
}

TEST(ExponentialDecayFactor, FrameRateIndependence)
{
    // 关键测试：验证帧率无关性
    // 无论帧率如何，一秒内的总纠正量应相同

    // 使用 ratePerSecond = 0.5
    constexpr f32 rate = 0.5f;

    // 60 FPS: deltaTime = 1/60 ≈ 0.0167s
    const f32 dt60 = 1.0f / 60.0f;
    const f32 factor60 = exponentialDecayFactor(rate, dt60);

    // 30 FPS: deltaTime = 1/30 ≈ 0.0333s
    const f32 dt30 = 1.0f / 30.0f;
    const f32 factor30 = exponentialDecayFactor(rate, dt30);

    // 一秒内的总纠正量（使用指数衰减公式）
    // 60帧后剩余: (1 - factor60)^60
    // 30帧后剩余: (1 - factor30)^30
    // 两者应该相等
    const f32 remainingAfter1Sec60 = std::pow(1.0f - factor60, 60.0f);
    const f32 remainingAfter1Sec30 = std::pow(1.0f - factor30, 30.0f);

    // 验证两者都约为 0.5（即每秒纠正 50%）
    EXPECT_NEAR(remainingAfter1Sec60, 0.5f, 0.01f);
    EXPECT_NEAR(remainingAfter1Sec30, 0.5f, 0.01f);
    EXPECT_NEAR(remainingAfter1Sec60, remainingAfter1Sec30, 0.001f);
}

TEST(ExponentialDecayFactor, TypicalUseCases)
{
    // 典型使用场景测试

    // 时间同步：ratePerSecond = 0.5, 60 FPS
    constexpr f32 timeSyncRate = 0.5f;
    const f32 dt60fps = 1.0f / 60.0f;
    const f32 factor = exponentialDecayFactor(timeSyncRate, dt60fps);

    // 每帧应该纠正约 1.15% (0.5 纠正率的 60 FPS 版本)
    EXPECT_NEAR(factor, 0.0115f, 0.0002f);

    // 验证 60 帧后的总纠正
    const f32 totalCorrection = 1.0f - std::pow(1.0f - factor, 60.0f);
    EXPECT_NEAR(totalCorrection, 0.5f, 0.01f);
}

TEST(ExponentialDecayFactor, EdgeCases)
{
    // 负值 deltaTime
    EXPECT_FLOAT_EQ(exponentialDecayFactor(0.5f, -1.0f), 0.0f);

    // 负值 ratePerSecond
    EXPECT_FLOAT_EQ(exponentialDecayFactor(-0.5f, 1.0f), 0.0f);

    // 非常大的 deltaTime（如低帧率场景）
    // ratePerSecond = 0.5, deltaTime = 0.1 (10 FPS)
    const f32 factor = exponentialDecayFactor(0.5f, 0.1f);
    EXPECT_GT(factor, 0.0f);
    EXPECT_LT(factor, 1.0f);

    // 验证 10 帧后总纠正约 50%
    const f32 totalCorrection = 1.0f - std::pow(1.0f - factor, 10.0f);
    EXPECT_NEAR(totalCorrection, 0.5f, 0.02f);
}

TEST(ExponentialDecayFactor, FormulaCorrectness)
{
    // 验证公式正确性
    // correctionFactor = 1 - (1 - ratePerSecond)^deltaTime

    // 已知值测试
    // rate = 0.5, dt = 0.5: factor = 1 - 0.5^0.5 = 1 - sqrt(0.5) ≈ 0.293
    const f32 expected1 = 1.0f - std::sqrt(0.5f);
    EXPECT_NEAR(exponentialDecayFactor(0.5f, 0.5f), expected1, 0.0001f);

    // rate = 0.25, dt = 2.0: factor = 1 - 0.75^2 = 1 - 0.5625 = 0.4375
    EXPECT_NEAR(exponentialDecayFactor(0.25f, 2.0f), 0.4375f, 0.0001f);

    // rate = 0.1, dt = 1.0: factor = 1 - 0.9^1 = 0.1
    EXPECT_NEAR(exponentialDecayFactor(0.1f, 1.0f), 0.1f, 0.0001f);
}
