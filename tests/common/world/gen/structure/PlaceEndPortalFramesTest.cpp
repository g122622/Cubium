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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

using namespace mc;
using namespace mc::block_registry;
using namespace mc::world::gen::structure;
using namespace mc::world::chunk;

// ============================================================================
// 测试辅助类
// ============================================================================

namespace {

/// 暴露 StructurePiece 受保护方法的测试子类
class TestStructurePiece : public StructurePiece {
public:
    TestStructurePiece(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
        : StructurePiece(0, minX, minY, minZ, maxX, maxY, maxZ)
    {}

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        world::chunk::ChunkPrimer* /*chunk*/,
        IChunkGenerator* /*generator*/) override
    {
        MC_UNUSED(world);
        MC_UNUSED(rng);
        MC_UNUSED(chunkX);
        MC_UNUSED(chunkZ);
        MC_UNUSED(chunkBounds);
    }

    // 暴露基类受保护的方法
    using StructurePiece::placeEndPortalFrames;
    using StructurePiece::setBlockState;
};

} // namespace

// ============================================================================
// placeEndPortalFrames 测试夹具
// ============================================================================

class PlaceEndPortalFramesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        // 创建 3x3 区块区域，用于模拟 WorldGenRegion
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 用石头填充 y=0..70 的基础地形
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        for (i32 y = 0; y <= 70; ++y) {
                            chunk->setBlockState(x, y, z, &VanillaBlocks::STONE->defaultState());
                        }
                    }
                }

                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));

        // 创建测试用 StructurePiece，覆盖 (0, 60, 0) ~ (20, 70, 20) 的区域
        // 传送门中心放在 (5, 63, 10)，这在该区域内
        m_piece = std::make_unique<TestStructurePiece>(0, 60, 0, 20, 70, 20);
        m_piece->setCoordBaseMode(Direction::South);
    }

    /// 验证指定世界坐标是否为末地传送门框架方块
    [[nodiscard]] bool isEndPortalFrame(i32 x, i32 y, i32 z) const
    {
        const BlockState* state = m_region->getBlockState(x, y, z);
        return state != nullptr && state->is(VanillaBlocks::END_PORTAL_FRAME);
    }

    /// 验证指定世界坐标是否为末地传送门方块
    [[nodiscard]] bool isEndPortal(i32 x, i32 y, i32 z) const
    {
        const BlockState* state = m_region->getBlockState(x, y, z);
        return state != nullptr && state->is(VanillaBlocks::END_PORTAL);
    }

    /// 获取指定世界坐标的方块状态
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const
    {
        return m_region->getBlockState(x, y, z);
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    std::unique_ptr<TestStructurePiece> m_piece;

    /// StructurePiece 边界框（覆盖区块范围）
    StructureBoundingBox m_bounds{-16, 0, -16, 32, 80, 32};
};

// ============================================================================
// 坐标映射正确性测试
// ============================================================================

TEST_F(PlaceEndPortalFramesTest, AllEyesFilled_Places12FramesAnd9PortalBlocks)
{
    // 所有框架都有末影之眼
    bool eyeStates[12] = {true, true, true, true, true, true, true, true, true, true, true, true};

    // 传送门中心在局部坐标 (5, 3, 10)
    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, true);

    // Direction::South 时坐标变换: worldX = minX + x, worldY = minY + y, worldZ = minZ + z
    // 所以世界坐标: worldX = 0 + 5 = 5, worldY = 60 + 3 = 63, worldZ = 0 + 10 = 10

    // 统计末地传送门框架方块数量
    i32 frameCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                if (isEndPortalFrame(x, y, z)) {
                    frameCount++;
                }
            }
        }
    }
    EXPECT_EQ(frameCount, 12);

    // 统计末地传送门方块数量
    i32 portalCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                if (isEndPortal(x, y, z)) {
                    portalCount++;
                }
            }
        }
    }
    EXPECT_EQ(portalCount, 9);
}

TEST_F(PlaceEndPortalFramesTest, NotAllEyesFilled_NoPortalBlocks)
{
    // 仅部分框架有末影之眼
    bool eyeStates[12] = {true, false, true, false, true, true, false, true, true, false, true, true};

    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, false);

    // 仍然有 12 个框架方块
    i32 frameCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                if (isEndPortalFrame(x, y, z)) {
                    frameCount++;
                }
            }
        }
    }
    EXPECT_EQ(frameCount, 12);

    // 没有末地传送门方块（因为不是所有框架都有眼）
    i32 portalCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                if (isEndPortal(x, y, z)) {
                    portalCount++;
                }
            }
        }
    }
    EXPECT_EQ(portalCount, 0);
}

TEST_F(PlaceEndPortalFramesTest, FramePositions_CorrectInSouthDirection)
{
    // 所有框架都有末影之眼
    bool eyeStates[12] = {true, true, true, true, true, true, true, true, true, true, true, true};

    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, true);

    // Direction::South: worldX = minX + x, worldY = minY + y, worldZ = minZ + z
    // 传送门中心世界坐标: (5, 63, 10)

    // 北边框架 (z = centerZ - 2 = 8, 局部): 世界 z = 0 + 8 = 8
    // x = centerX-1, centerX, centerX+1 = 4,5,6: 世界 x = 4,5,6
    EXPECT_TRUE(isEndPortalFrame(4, 63, 8));
    EXPECT_TRUE(isEndPortalFrame(5, 63, 8));
    EXPECT_TRUE(isEndPortalFrame(6, 63, 8));

    // 南边框架 (z = centerZ + 2 = 12): 世界 z = 12
    EXPECT_TRUE(isEndPortalFrame(4, 63, 12));
    EXPECT_TRUE(isEndPortalFrame(5, 63, 12));
    EXPECT_TRUE(isEndPortalFrame(6, 63, 12));

    // 西边框架 (x = centerX - 2 = 3): 世界 x = 3
    EXPECT_TRUE(isEndPortalFrame(3, 63, 9));
    EXPECT_TRUE(isEndPortalFrame(3, 63, 10));
    EXPECT_TRUE(isEndPortalFrame(3, 63, 11));

    // 东边框架 (x = centerX + 2 = 7): 世界 x = 7
    EXPECT_TRUE(isEndPortalFrame(7, 63, 9));
    EXPECT_TRUE(isEndPortalFrame(7, 63, 10));
    EXPECT_TRUE(isEndPortalFrame(7, 63, 11));

    // 传送门方块在内部 3×3 区域
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            EXPECT_TRUE(isEndPortal(5 + dx, 63, 10 + dz))
                << "Expected END_PORTAL at (" << 5 + dx << ", 63, " << 10 + dz << ")";
        }
    }

    // 角落不应有框架方块
    EXPECT_FALSE(isEndPortalFrame(3, 63, 8));  // 西北角
    EXPECT_FALSE(isEndPortalFrame(7, 63, 8));  // 东北角
    EXPECT_FALSE(isEndPortalFrame(3, 63, 12)); // 西南角
    EXPECT_FALSE(isEndPortalFrame(7, 63, 12)); // 东南角
}

TEST_F(PlaceEndPortalFramesTest, EyeStates_AppliedCorrectly)
{
    // 每个框架的末影之眼状态不同
    bool eyeStates[12] = {
        true,
        false,
        false, // 北边: 索引 0,1,2
        true,
        false,
        false, // 南边: 索引 3,4,5
        true,
        false,
        false, // 西边: 索引 6,7,8
        true,
        false,
        false // 东边: 索引 9,10,11
    };

    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, false);

    // Direction::South: 世界坐标变换
    // 北边 x=4,5,6, z=8, y=63
    // 检查索引 0 (有眼): (4, 63, 8)
    const BlockState* frame00 = getBlockState(4, 63, 8);
    ASSERT_NE(frame00, nullptr);
    EXPECT_TRUE(frame00->is(VanillaBlocks::END_PORTAL_FRAME));
    EXPECT_EQ(frame00->getOptional(BlockStateProperties::EYE()).value_or(false), true);

    // 检查索引 1 (无眼): (5, 63, 8)
    const BlockState* frame01 = getBlockState(5, 63, 8);
    ASSERT_NE(frame01, nullptr);
    EXPECT_TRUE(frame01->is(VanillaBlocks::END_PORTAL_FRAME));
    EXPECT_EQ(frame01->getOptional(BlockStateProperties::EYE()).value_or(true), false);

    // 检查索引 6 (有眼, 西边第一个): (3, 63, 9)
    const BlockState* frame60 = getBlockState(3, 63, 9);
    ASSERT_NE(frame60, nullptr);
    EXPECT_TRUE(frame60->is(VanillaBlocks::END_PORTAL_FRAME));
    EXPECT_EQ(frame60->getOptional(BlockStateProperties::EYE()).value_or(false), true);

    // 检查索引 7 (无眼, 西边第二个): (3, 63, 10)
    const BlockState* frame61 = getBlockState(3, 63, 10);
    ASSERT_NE(frame61, nullptr);
    EXPECT_TRUE(frame61->is(VanillaBlocks::END_PORTAL_FRAME));
    EXPECT_EQ(frame61->getOptional(BlockStateProperties::EYE()).value_or(true), false);
}

TEST_F(PlaceEndPortalFramesTest, FacingDirections_CorrectInSouthDirection)
{
    bool eyeStates[12] = {true, true, true, true, true, true, true, true, true, true, true, true};

    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, true);

    // Direction::South 时，setBlockState 会应用 LeftRight 镜像，但 HORIZONTAL_FACING
    // 也会被镜像变换。由于要塞房间方向通过 setCoordBaseMode 设置，
    // 传送门框架的方向会被自动处理。
    //
    // 对于 South 方向（LeftRight 镜像），FACING 属性会被镜像：
    // North -> South, South -> North, West -> West, East -> East
    // 这确保了在所有方向下，框架凸起都正确朝外

    // 验证北边框架（局部 FACING=NORTH，经 South 方向镜像后变为 FACING=SOUTH）
    const BlockState* northFrame = getBlockState(4, 63, 8);
    ASSERT_NE(northFrame, nullptr);
    auto northFacing = northFrame->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(northFacing.has_value());
    // South 方向镜像: North -> South
    EXPECT_EQ(northFacing.value(), Direction::South);

    // 验证南边框架（局部 FACING=SOUTH，经 South 方向镜像后变为 FACING=NORTH）
    const BlockState* southFrame = getBlockState(4, 63, 12);
    ASSERT_NE(southFrame, nullptr);
    auto southFacing = southFrame->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(southFacing.has_value());
    // South 方向镜像: South -> North
    EXPECT_EQ(southFacing.value(), Direction::North);

    // 验证西边框架（局部 FACING=WEST，经 LeftRight 镜像不变）
    const BlockState* westFrame = getBlockState(3, 63, 10);
    ASSERT_NE(westFrame, nullptr);
    auto westFacing = westFrame->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(westFacing.has_value());
    EXPECT_EQ(westFacing.value(), Direction::West);

    // 验证东边框架（局部 FACING=EAST，经 LeftRight 镜像不变）
    const BlockState* eastFrame = getBlockState(7, 63, 10);
    ASSERT_NE(eastFrame, nullptr);
    auto eastFacing = eastFrame->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(eastFacing.has_value());
    EXPECT_EQ(eastFacing.value(), Direction::East);
}

// ============================================================================
// 边界框裁剪测试
// ============================================================================

TEST_F(PlaceEndPortalFramesTest, BoundsClipping_PartiallyOutsideBounds)
{
    // 创建一个不覆盖传送门全部区域的边界框
    // 只包含 x >= 5, z >= 10 的部分
    bool eyeStates[12] = {true, true, true, true, true, true, true, true, true, true, true, true};
    StructureBoundingBox partialBounds(5, 60, 10, 32, 80, 32);

    m_piece->placeEndPortalFrames(*m_region, partialBounds, 5, 3, 10, eyeStates, true);

    // 只有边界框内的方块被放置
    // 北边框架 z=8: 在 partialBounds 外（z < 10），不应放置
    EXPECT_FALSE(isEndPortalFrame(4, 63, 8));
    EXPECT_FALSE(isEndPortalFrame(5, 63, 8));
    EXPECT_FALSE(isEndPortalFrame(6, 63, 8));

    // 南边框架 z=12: 在 partialBounds 内
    EXPECT_TRUE(isEndPortalFrame(5, 63, 12));
    EXPECT_TRUE(isEndPortalFrame(6, 63, 12));
    // x=4 的南边框架在 partialBounds 外（x < 5）
    EXPECT_FALSE(isEndPortalFrame(4, 63, 12));

    // 东边框架 x=7, z=9,10,11
    EXPECT_FALSE(isEndPortalFrame(7, 63, 9)); // z=9 < 10，在边界框外
    EXPECT_TRUE(isEndPortalFrame(7, 63, 10)); // 在边界框内
    EXPECT_TRUE(isEndPortalFrame(7, 63, 11)); // 在边界框内

    // 西边框架 x=3: 在 partialBounds 外（x < 5）
    EXPECT_FALSE(isEndPortalFrame(3, 63, 9));
    EXPECT_FALSE(isEndPortalFrame(3, 63, 10));
    EXPECT_FALSE(isEndPortalFrame(3, 63, 11));
}

// ============================================================================
// 北方向测试（验证坐标变换正确性）
// ============================================================================

TEST_F(PlaceEndPortalFramesTest, NorthDirection_CoordinateTransform)
{
    // 创建一个北方向的 StructurePiece
    auto northPiece = std::make_unique<TestStructurePiece>(0, 60, 0, 20, 70, 20);
    northPiece->setCoordBaseMode(Direction::North);

    bool eyeStates[12] = {true, true, true, true, true, true, true, true, true, true, true, true};

    northPiece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, true);

    // Direction::North: worldX = minX + x, worldY = minY + y, worldZ = maxZ - z
    // maxX=20, maxZ=20, 所以 worldZ = 20 - z
    // 传送门中心: worldX = 0 + 5 = 5, worldY = 60 + 3 = 63, worldZ = 20 - 10 = 10

    // 北边框架 (局部 z=8): worldZ = 20 - 8 = 12, worldX = 4,5,6
    EXPECT_TRUE(isEndPortalFrame(4, 63, 12));
    EXPECT_TRUE(isEndPortalFrame(5, 63, 12));
    EXPECT_TRUE(isEndPortalFrame(6, 63, 12));

    // 南边框架 (局部 z=12): worldZ = 20 - 12 = 8, worldX = 4,5,6
    EXPECT_TRUE(isEndPortalFrame(4, 63, 8));
    EXPECT_TRUE(isEndPortalFrame(5, 63, 8));
    EXPECT_TRUE(isEndPortalFrame(6, 63, 8));

    // 验证传送门方块在中心 3×3 区域
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            EXPECT_TRUE(isEndPortal(5 + dx, 63, 10 + dz))
                << "Expected END_PORTAL at (" << 5 + dx << ", 63, " << 10 + dz << ") in North direction";
        }
    }
}

// ============================================================================
// 空框架测试（VanillaBlocks 未注册时的安全性）
// ============================================================================

TEST_F(PlaceEndPortalFramesTest, NoDuplicateFrames_WhenCalledTwiceOnSamePosition)
{
    // 调用两次不应导致重复（setBlockState 会覆盖，所以框架数量不变）
    bool eyeStates[12] = {true, true, true, true, true, true, true, true, true, true, true, true};

    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, true);
    m_piece->placeEndPortalFrames(*m_region, m_bounds, 5, 3, 10, eyeStates, true);

    // 仍然只有 12 个框架方块（不是 24 个）
    i32 frameCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                if (isEndPortalFrame(x, y, z)) {
                    frameCount++;
                }
            }
        }
    }
    EXPECT_EQ(frameCount, 12);

    // 仍然只有 9 个传送门方块（不是 18 个）
    i32 portalCount = 0;
    for (i32 x = -16; x <= 32; ++x) {
        for (i32 z = -16; z <= 32; ++z) {
            for (i32 y = 0; y <= 80; ++y) {
                if (isEndPortal(x, y, z)) {
                    portalCount++;
                }
            }
        }
    }
    EXPECT_EQ(portalCount, 9);
}
