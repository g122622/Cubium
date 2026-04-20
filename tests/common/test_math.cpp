#include <gtest/gtest.h>

#include "common/core/Result.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/chunk/ChunkPos.hpp"

using namespace mc;
using namespace mc::math;

// 辅助函数
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Error(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;
}

// ============================================================================
// MathUtils 测试
// ============================================================================

TEST(MathUtils, ToRadiansToDegrees) {
    EXPECT_NEAR(toRadians(180.0f), PI, 0.0001f);
    EXPECT_NEAR(toRadians(90.0f), HALF_PI, 0.0001f);
    EXPECT_NEAR(toDegrees(PI), 180.0f, 0.0001f);
    EXPECT_NEAR(toDegrees(HALF_PI), 90.0f, 0.0001f);
}

TEST(MathUtils, Clamp) {
    EXPECT_EQ(clamp(5, 0, 10), 5);
    EXPECT_EQ(clamp(-5, 0, 10), 0);
    EXPECT_EQ(clamp(15, 0, 10), 10);
}

TEST(MathUtils, Lerp) {
    EXPECT_NEAR(lerp(0.0f, 10.0f, 0.5f), 5.0f, 0.0001f);
    EXPECT_NEAR(lerp(0.0f, 10.0f, 0.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(lerp(0.0f, 10.0f, 1.0f), 10.0f, 0.0001f);
}

TEST(MathUtils, IsZero) {
    EXPECT_TRUE(isZero(0.0f));
    EXPECT_TRUE(isZero(0.0000001f));
    EXPECT_FALSE(isZero(0.01f));
}

TEST(MathUtils, ApproxEqual) {
    EXPECT_TRUE(approxEqual(1.0f, 1.0f));
    EXPECT_TRUE(approxEqual(1.0f, 1.000001f));
    EXPECT_FALSE(approxEqual(1.0f, 1.01f));
}

TEST(MathUtils, FastInverseSqrt) {
    EXPECT_NEAR(fastInverseSqrt(1.0f), 1.0f, 0.01f);
    EXPECT_NEAR(fastInverseSqrt(4.0f), 0.5f, 0.01f);
    EXPECT_NEAR(fastInverseSqrt(9.0f), 1.0f / 3.0f, 0.01f);
}

TEST(MathUtils, ChunkCoordConversion) {
    // 正坐标
    EXPECT_EQ(toChunkCoord(0), 0);
    EXPECT_EQ(toChunkCoord(15), 0);
    EXPECT_EQ(toChunkCoord(16), 1);
    EXPECT_EQ(toChunkCoord(31), 1);
    EXPECT_EQ(toChunkCoord(32), 2);
}

TEST(MathUtils, LocalCoordConversion) {
    EXPECT_EQ(toLocalCoord(0), 0);
    EXPECT_EQ(toLocalCoord(15), 15);
    EXPECT_EQ(toLocalCoord(16), 0);
    EXPECT_EQ(toLocalCoord(31), 15);
}

// ============================================================================
// Vector3 测试
// ============================================================================

TEST(Vector3, Construction) {
    Vector3 v1;
    EXPECT_FLOAT_EQ(v1.x, 0.0f);
    EXPECT_FLOAT_EQ(v1.y, 0.0f);
    EXPECT_FLOAT_EQ(v1.z, 0.0f);

    Vector3 v2(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v2.x, 1.0f);
    EXPECT_FLOAT_EQ(v2.y, 2.0f);
    EXPECT_FLOAT_EQ(v2.z, 3.0f);
}

TEST(Vector3, Arithmetic) {
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);

    Vector3 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);

    Vector3 diff = b - a;
    EXPECT_FLOAT_EQ(diff.x, 3.0f);
    EXPECT_FLOAT_EQ(diff.y, 3.0f);
    EXPECT_FLOAT_EQ(diff.z, 3.0f);

    Vector3 scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);
    EXPECT_FLOAT_EQ(scaled.z, 6.0f);
}

TEST(Vector3, DotProduct) {
    Vector3 a(1.0f, 0.0f, 0.0f);
    Vector3 b(0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);
    EXPECT_FLOAT_EQ(a.dot(a), 1.0f);
}

TEST(Vector3, Length) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
}

TEST(Vector3, Normalize) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    Vector3 normalized = v.normalized();

    EXPECT_NEAR(normalized.length(), 1.0f, 0.0001f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
}

// ============================================================================
// BlockPos 和 ChunkPos 测试
// ============================================================================

TEST(BlockPos, Construction) {
    BlockPos p1(10, 20, 30);
    EXPECT_EQ(p1.x, 10);
    EXPECT_EQ(p1.y, 20);
    EXPECT_EQ(p1.z, 30);
}

TEST(BlockPos, ChunkCoord) {
    BlockPos p1(0, 0, 0);
    EXPECT_EQ(p1.chunkX(), 0);
    EXPECT_EQ(p1.chunkZ(), 0);

    BlockPos p2(16, 0, 16);
    EXPECT_EQ(p2.chunkX(), 1);
    EXPECT_EQ(p2.chunkZ(), 1);
}

TEST(BlockPos, Adjacent) {
    BlockPos p(0, 0, 0);

    EXPECT_EQ(p.up(), BlockPos(0, 1, 0));
    EXPECT_EQ(p.down(), BlockPos(0, -1, 0));
    EXPECT_EQ(p.north(), BlockPos(0, 0, -1));
    EXPECT_EQ(p.south(), BlockPos(0, 0, 1));
    EXPECT_EQ(p.east(), BlockPos(1, 0, 0));
    EXPECT_EQ(p.west(), BlockPos(-1, 0, 0));
}

TEST(ChunkPos, Construction) {
    ChunkPos c1(10, 20);
    EXPECT_EQ(c1.x, 10);
    EXPECT_EQ(c1.z, 20);

    BlockPos b(16, 0, 32);
    ChunkPos c2(b);
    EXPECT_EQ(c2.x, 1);
    EXPECT_EQ(c2.z, 2);
}

TEST(ChunkPos, WorldCoord) {
    ChunkPos c(10, 20);
    EXPECT_EQ(c.worldX(), 160);
    EXPECT_EQ(c.worldZ(), 320);
}

TEST(ChunkPos, ToId) {
    ChunkPos c1(10, 20);
    u64 id = c1.toId();

    ChunkPos c2 = ChunkPos::fromId(id);
    EXPECT_EQ(c2.x, 10);
    EXPECT_EQ(c2.z, 20);
}

// ============================================================================
// Result 测试
// ============================================================================

TEST(Error, Construction) {
    Error e1;
    EXPECT_TRUE(e1.success());
    EXPECT_FALSE(e1.failed());

    Error e2(ErrorCode::NotFound, "Resource not found");
    EXPECT_FALSE(e2.success());
    EXPECT_TRUE(e2.failed());
    EXPECT_EQ(static_cast<int>(e2.code()), static_cast<int>(ErrorCode::NotFound));
}

TEST(ResultVoid, Success) {
    Result<void> result;
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.failed());
}

TEST(ResultVoid, Failure) {
    Result<void> result(Error(ErrorCode::NotFound, "Not found"));
    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());
}

TEST(ResultT, SuccessWithValue) {
    Result<int> result(42);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultT, Failure) {
    Result<int> result(Error(ErrorCode::NotFound, "Not found"));
    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());
}

TEST(ResultT, ValueOr) {
    Result<int> success{42};
    Result<int> failure{Error(ErrorCode::NotFound, "")};

    EXPECT_EQ(success.valueOr(0), 42);
    EXPECT_EQ(failure.valueOr(0), 0);
}

TEST(Result, RealWorldUsage) {
    auto result1 = divide(10, 2);
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 5);

    auto result2 = divide(10, 0);
    EXPECT_FALSE(result2.success());
}
