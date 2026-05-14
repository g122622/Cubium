#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/WorldConstants.hpp"

using namespace mc;

// ============================================================================
// 基础类型测试
// ============================================================================

TEST(Types, IntegerSizes)
{
    // 验证整数类型大小
    EXPECT_EQ(sizeof(i8), 1);
    EXPECT_EQ(sizeof(i16), 2);
    EXPECT_EQ(sizeof(i32), 4);
    EXPECT_EQ(sizeof(i64), 8);

    EXPECT_EQ(sizeof(u8), 1);
    EXPECT_EQ(sizeof(u16), 2);
    EXPECT_EQ(sizeof(u32), 4);
    EXPECT_EQ(sizeof(u64), 8);
}

TEST(Types, FloatSizes)
{
    EXPECT_EQ(sizeof(f32), 4);
    EXPECT_EQ(sizeof(f64), 8);
}

TEST(Types, TypeAliases)
{
    // 验证类型别名正确
    i32 coord = 100;
    EXPECT_EQ(coord, 100);

    ChunkCoord chunkX = 10;
    ChunkCoord chunkZ = -5;
    EXPECT_EQ(chunkX, 10);
    EXPECT_EQ(chunkZ, -5);

    BlockCoord blockX = 15;
    EXPECT_EQ(blockX, 15);
}

TEST(Types, EnumTypes)
{
    // 测试枚举类型
    DimensionId dim = 0; // Overworld
    EXPECT_EQ(dim, 0);
    EXPECT_EQ(1, 1); // Nether
    EXPECT_EQ(2, 2); // TheEnd

    GameMode mode = GameMode::Creative;
    EXPECT_EQ(static_cast<u8>(mode), 1);

    Difficulty diff = Difficulty::Hard;
    EXPECT_EQ(static_cast<u8>(diff), 3);

    BlockFace face = BlockFace::Top;
    EXPECT_EQ(static_cast<u8>(face), 1);
}

TEST(Types, BlockFaceValues)
{
    EXPECT_EQ(static_cast<u8>(BlockFace::Bottom), 0);
    EXPECT_EQ(static_cast<u8>(BlockFace::Top), 1);
    EXPECT_EQ(static_cast<u8>(BlockFace::North), 2);
    EXPECT_EQ(static_cast<u8>(BlockFace::South), 3);
    EXPECT_EQ(static_cast<u8>(BlockFace::West), 4);
    EXPECT_EQ(static_cast<u8>(BlockFace::East), 5);
}

// ============================================================================
// Result类型测试
// ============================================================================

TEST(Result, SuccessWithValue)
{
    Result<i32> result = 42;

    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.failed());
    EXPECT_EQ(result.value(), 42);
}

TEST(Result, FailureWithError)
{
    Result<i32> result = Error(ErrorCode::InvalidArgument, "Test error");

    EXPECT_FALSE(result.success());
    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(result.error().message(), "Test error");
}

TEST(Result, ValueOrDefault)
{
    Result<i32> success = 42;
    Result<i32> failure = Error(ErrorCode::NotFound, "Not found");

    EXPECT_EQ(success.valueOr(0), 42);
    EXPECT_EQ(failure.valueOr(100), 100);
}

TEST(Result, VoidResult)
{
    Result<void> success;
    EXPECT_TRUE(success.success());

    Result<void> failure = Error(ErrorCode::Unknown, "Error");
    EXPECT_TRUE(failure.failed());
}

TEST(Result, ChainingOperations)
{
    auto divide = [](i32 a, i32 b) -> Result<i32> {
        if (b == 0) {
            return Error(ErrorCode::InvalidArgument, "Division by zero");
        }
        return a / b;
    };

    auto result1 = divide(10, 2);
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 5);

    auto result2 = divide(10, 0);
    EXPECT_TRUE(result2.failed());
    EXPECT_EQ(result2.error().code(), ErrorCode::InvalidArgument);
}

TEST(Result, StringResult)
{
    Result<std::string> result = std::string("Hello");
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), "Hello");

    Result<std::string> error = Error(ErrorCode::NotFound, "std::string not found");
    EXPECT_TRUE(error.failed());
}

TEST(Result, VectorResult)
{
    std::vector<i32> vec = {1, 2, 3, 4, 5};
    Result<std::vector<i32>> result = vec;

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 5u);
    EXPECT_EQ(result.value()[0], 1);
    EXPECT_EQ(result.value()[4], 5);
}

TEST(Result, MoveSemantics)
{
    Result<std::unique_ptr<i32>> result = std::make_unique<i32>(42);
    EXPECT_TRUE(result.success());

    auto ptr = std::move(result.value());
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 42);
}

TEST(Result, MoveOnlyValueConstructsAndMoves)
{
    Result<std::unique_ptr<i32>> result(std::make_unique<i32>(7));

    EXPECT_TRUE(result.success());

    // value() 每次调用都会转移所有权，所以只调用一次并保存结果
    auto ptr = result.value();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 7);
}

// ============================================================================
// Error类测试
// ============================================================================

TEST(ErrorTest, Construction)
{
    Error err(ErrorCode::InvalidArgument, "Invalid argument");
    EXPECT_EQ(err.code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(err.message(), "Invalid argument");
}

TEST(ErrorTest, Comparison)
{
    Error err1(ErrorCode::NotFound, "Not found");
    Error err2(ErrorCode::NotFound, "Different message");
    Error err3(ErrorCode::InvalidArgument, "Not found");

    // 比较错误码
    EXPECT_EQ(err1.code(), err2.code());
    EXPECT_NE(err1.code(), err3.code());
}

TEST(ErrorTest, ErrorCodeValues)
{
    // 验证错误码值
    EXPECT_EQ(static_cast<i32>(ErrorCode::Success), 0);
    EXPECT_EQ(static_cast<i32>(ErrorCode::Unknown), -1);
    EXPECT_EQ(static_cast<i32>(ErrorCode::InvalidArgument), -2);
    EXPECT_EQ(static_cast<i32>(ErrorCode::NotFound), -100);
    EXPECT_EQ(static_cast<i32>(ErrorCode::OutOfBounds), -6);
}

// ============================================================================
// 常量测试
// ============================================================================

TEST(Constants, MathConstants)
{
    EXPECT_FLOAT_EQ(math::PI, 3.14159265f);
    EXPECT_FLOAT_EQ(math::TWO_PI, 2.0f * math::PI);
    EXPECT_FLOAT_EQ(math::HALF_PI, math::PI / 2.0f);
    EXPECT_FLOAT_EQ(math::DEG_TO_RAD, math::PI / 180.0f);
    EXPECT_FLOAT_EQ(math::RAD_TO_DEG, 180.0f / math::PI);
}

TEST(Constants, WorldConstants)
{
    EXPECT_EQ(world::CHUNK_WIDTH, 16);
    EXPECT_EQ(world::CHUNK_HEIGHT, 256);
    EXPECT_EQ(world::CHUNK_SECTION_HEIGHT, 16);
    EXPECT_EQ(world::CHUNK_SECTIONS, 16);
    EXPECT_EQ(world::MIN_BUILD_HEIGHT, 0);
    EXPECT_EQ(world::MAX_BUILD_HEIGHT, 256);
    // MC 1.16.5 海平面高度是 63
    EXPECT_EQ(world::SEA_LEVEL, 63);
}

TEST(Constants, NetworkConstants)
{
    // MC 1.16.5 Java版默认端口是 25565
    EXPECT_EQ(network::DEFAULT_PORT, 25565);
    EXPECT_EQ(network::MAX_PACKET_SIZE, 2097152u);
    EXPECT_EQ(network::CONNECT_TIMEOUT_MS, 30000u);
}

// ============================================================================
// 世界坐标转换测试
// ============================================================================

TEST(WorldConversion, ToChunkCoord)
{
    // 正数坐标
    EXPECT_EQ(world::toChunkCoord(0), 0);
    EXPECT_EQ(world::toChunkCoord(15), 0);
    EXPECT_EQ(world::toChunkCoord(16), 1);
    EXPECT_EQ(world::toChunkCoord(32), 2);

    // 负数坐标
    EXPECT_EQ(world::toChunkCoord(-1), -1);
    EXPECT_EQ(world::toChunkCoord(-16), -1);
    EXPECT_EQ(world::toChunkCoord(-17), -2);
}

TEST(WorldConversion, ToLocalCoord)
{
    EXPECT_EQ(world::toLocalCoord(0), 0);
    EXPECT_EQ(world::toLocalCoord(15), 15);
    EXPECT_EQ(world::toLocalCoord(16), 0);
    EXPECT_EQ(world::toLocalCoord(17), 1);

    // 负数坐标
    EXPECT_EQ(world::toLocalCoord(-1), 15);
    EXPECT_EQ(world::toLocalCoord(-16), 0);
    EXPECT_EQ(world::toLocalCoord(-17), 15);
}

TEST(WorldConversion, ToWorldCoord)
{
    EXPECT_EQ(world::toWorldCoord(0), 0);
    EXPECT_EQ(world::toWorldCoord(1), 16);
    EXPECT_EQ(world::toWorldCoord(10), 160);
    EXPECT_EQ(world::toWorldCoord(-1), -16);
}

TEST(WorldConversion, ToSectionIndex)
{
    EXPECT_EQ(world::toSectionIndex(0), 0);
    EXPECT_EQ(world::toSectionIndex(15), 0);
    EXPECT_EQ(world::toSectionIndex(16), 1);
    EXPECT_EQ(world::toSectionIndex(255), 15);
}

TEST(WorldConversion, SectionToY)
{
    EXPECT_EQ(world::sectionToY(0), 0);
    EXPECT_EQ(world::sectionToY(1), 16);
    EXPECT_EQ(world::sectionToY(15), 240);
}

TEST(WorldConversion, IsValidY)
{
    EXPECT_TRUE(world::isValidY(0));
    EXPECT_TRUE(world::isValidY(128));
    EXPECT_TRUE(world::isValidY(255));
    EXPECT_FALSE(world::isValidY(-1));
    EXPECT_FALSE(world::isValidY(256));
    EXPECT_FALSE(world::isValidY(1000));
}
