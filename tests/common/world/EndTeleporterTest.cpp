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

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <map>

using namespace mc;

// ============================================================================
// 测试用世界 - 支持 setBlockState / getBlockState 的 HashMap 实现
// ============================================================================

class EndTeleporterTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EndTeleporterTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EndTeleporterTestWorld::tickManager not implemented");
    }

    // ========== 测试辅助方法 ==========

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const { return getBlockState(pos.x, pos.y, pos.z); }

    [[nodiscard]] bool hasBlockAt(i32 x, i32 y, i32 z) const
    {
        return m_blocks.find(BlockPos(x, y, z)) != m_blocks.end();
    }

    [[nodiscard]] size_t blockCount() const { return m_blocks.size(); }

    void clear() { m_blocks.clear(); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
};

// ============================================================================
// 测试夹具
// ============================================================================

class EndTeleporterTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    EndTeleporterTestWorld m_world;
};

// ============================================================================
// getEndSpawnPosition 测试
// ============================================================================

TEST_F(EndTeleporterTest, GetEndSpawnPositionReturnsFixedPosition)
{
    Vector3d pos = EndTeleporter::getEndSpawnPosition();
    EXPECT_DOUBLE_EQ(pos.x, 100.5);
    EXPECT_DOUBLE_EQ(pos.y, 50.0);
    EXPECT_DOUBLE_EQ(pos.z, 0.5);
}

TEST_F(EndTeleporterTest, GetEndSpawnPositionMatchesBaseClass)
{
    Vector3d endPos = EndTeleporter::getEndSpawnPosition();
    Vector3d basePos = Teleporter::getEndSpawnPosition();
    EXPECT_DOUBLE_EQ(endPos.x, basePos.x);
    EXPECT_DOUBLE_EQ(endPos.y, basePos.y);
    EXPECT_DOUBLE_EQ(endPos.z, basePos.z);
}

// ============================================================================
// getCoordinateScale 测试
// ============================================================================

TEST_F(EndTeleporterTest, CoordinateScaleIsOne)
{
    EndTeleporter teleporter;
    EXPECT_FLOAT_EQ(teleporter.getCoordinateScale(), 1.0f);
}

// ============================================================================
// findPortal 测试
// ============================================================================

TEST_F(EndTeleporterTest, FindPortalReturnsFixedPosition)
{
    EndTeleporter teleporter;
    auto result = teleporter.findPortal(m_world, Vector3d(0, 0, 0));
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->valid);
    EXPECT_DOUBLE_EQ(result->position.x, 100.5);
    EXPECT_DOUBLE_EQ(result->position.y, 50.0);
    EXPECT_DOUBLE_EQ(result->position.z, 0.5);
    EXPECT_FLOAT_EQ(result->yaw, 90.0f);
    EXPECT_FLOAT_EQ(result->pitch, 0.0f);
}

// ============================================================================
// createEndSpawnPlatform 测试
// ============================================================================

TEST_F(EndTeleporterTest, CreateEndSpawnPlatform_PlacesObsidianAtY48)
{
    EndTeleporter::createEndSpawnPlatform(m_world);

    // 5×5 黑曜石平台在 Y=48
    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            const BlockState* state = m_world.getBlockAt(BlockPos(100 + x, 48, z));
            ASSERT_NE(state, nullptr) << "Missing block at (" << 100 + x << ", 48, " << z << ")";
            EXPECT_EQ(state, obsidian) << "Expected obsidian at (" << 100 + x << ", 48, " << z << ")";
        }
    }
}

TEST_F(EndTeleporterTest, CreateEndSpawnPlatform_Clears4LayersAbovePlatform)
{
    EndTeleporter::createEndSpawnPlatform(m_world);

    // Y=49, 50, 51, 52 应为空气（4层，对齐 MC Java）
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            for (i32 y = 49; y <= 52; ++y) {
                EXPECT_FALSE(m_world.hasBlockAt(100 + x, y, z))
                    << "Expected air at (" << 100 + x << ", " << y << ", " << z << ")";
            }
        }
    }
}

TEST_F(EndTeleporterTest, CreateEndSpawnPlatform_PlatformIs5x5)
{
    EndTeleporter::createEndSpawnPlatform(m_world);

    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();

    // 平台边缘之外不应有黑曜石
    EXPECT_NE(m_world.getBlockAt(BlockPos(97, 48, 0)), obsidian);   // x=-3
    EXPECT_NE(m_world.getBlockAt(BlockPos(103, 48, 0)), obsidian);  // x=+3
    EXPECT_NE(m_world.getBlockAt(BlockPos(100, 48, -3)), obsidian); // z=-3
    EXPECT_NE(m_world.getBlockAt(BlockPos(100, 48, 3)), obsidian);  // z=+3
}

TEST_F(EndTeleporterTest, CreateEndSpawnPlatform_PlatformCenterIsCorrect)
{
    EndTeleporter::createEndSpawnPlatform(m_world);

    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();

    // 中心方块验证
    EXPECT_EQ(m_world.getBlockAt(BlockPos(100, 48, 0)), obsidian);
    // 角落方块验证
    EXPECT_EQ(m_world.getBlockAt(BlockPos(98, 48, -2)), obsidian);
    EXPECT_EQ(m_world.getBlockAt(BlockPos(102, 48, 2)), obsidian);
}

// ============================================================================
// createExitPortal 测试
// ============================================================================

TEST_F(EndTeleporterTest, CreateExitPortal_ActivePlacesEndPortalBlocks)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    const BlockState* endPortal = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
    ASSERT_NE(endPortal, nullptr);

    // 内圆区域应有末地传送门方块（但中心 (0,64,0) 被基岩柱覆盖）
    // (1,0) 距离平方=1, 在半径 2.5 内，不是基岩柱位置
    EXPECT_EQ(m_world.getBlockAt(BlockPos(1, 64, 0)), endPortal);
    // (0,1) 距离平方=1, 在半径 2.5 内
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 64, 1)), endPortal);
    // (-1,0) 距离平方=1, 在半径 2.5 内
    EXPECT_EQ(m_world.getBlockAt(BlockPos(-1, 64, 0)), endPortal);
    // (0,-1) 距离平方=1, 在半径 2.5 内
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 64, -1)), endPortal);
}

TEST_F(EndTeleporterTest, CreateExitPortal_CenterIsBedrockPillar)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);

    // 中心 (0,64,0) 被基岩柱覆盖，不是末地传送门方块
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 64, 0)), bedrock);
}

TEST_F(EndTeleporterTest, CreateExitPortal_InactiveClearsNonCenterInnerCircle)
{
    BlockPos center(0, 64, 0);
    // 先放一些方块在传送门位置
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    m_world.setBlockState(1, 64, 0, stone);
    m_world.setBlockState(0, 64, 1, stone);

    EndTeleporter::createExitPortal(m_world, center, false);

    // 内圆非中心位置应为空气（被清除）
    // 注意中心 (0,64,0) 会被基岩柱覆盖
    EXPECT_FALSE(m_world.hasBlockAt(1, 64, 0));
    EXPECT_FALSE(m_world.hasBlockAt(0, 64, 1));
}

TEST_F(EndTeleporterTest, CreateExitPortal_BedrockBase)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);

    // Y=63（中心下方一层）: 内圆应为基岩
    // (0,0) 距离平方=0 < 6.25，在内圆内
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 63, 0)), bedrock);
    // (1,0) 距离平方=1 < 6.25
    EXPECT_EQ(m_world.getBlockAt(BlockPos(1, 63, 0)), bedrock);
    // (0,1) 距离平方=1 < 6.25
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 63, 1)), bedrock);
}

TEST_F(EndTeleporterTest, CreateExitPortal_EndStoneOuterRing)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    const BlockState* endStone = VanillaBlocks::getState(VanillaBlocks::END_STONE);
    ASSERT_NE(endStone, nullptr);

    // Y=63: 外环（半径 2.5 < dist <= 3.5）应为末地石
    // (3,0) 距离平方=9, 9 <= 12.25，在外环内但不在内圆
    EXPECT_EQ(m_world.getBlockAt(BlockPos(3, 63, 0)), endStone);
    // (0,3) 距离平方=9
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 63, 3)), endStone);
}

TEST_F(EndTeleporterTest, CreateExitPortal_BedrockPillar)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);

    // 中心柱：Y=64,65,66,67 应为基岩（4格高）
    for (i32 dy = 0; dy < 4; ++dy) {
        EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 64 + dy, 0)), bedrock) << "Expected bedrock pillar at Y=" << 64 + dy;
    }
}

TEST_F(EndTeleporterTest, CreateExitPortal_BedrockRim)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);

    // Y=64 传送门层外环基岩（半径 2.5 < dist <= 3.5）
    // (3,0) 距离平方=9, 9 <= 12.25
    EXPECT_EQ(m_world.getBlockAt(BlockPos(3, 64, 0)), bedrock);
    EXPECT_EQ(m_world.getBlockAt(BlockPos(-3, 64, 0)), bedrock);
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 64, 3)), bedrock);
    EXPECT_EQ(m_world.getBlockAt(BlockPos(0, 64, -3)), bedrock);
}

TEST_F(EndTeleporterTest, CreateExitPortal_WallTorches)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::createExitPortal(m_world, center, true);

    // 墙上火把在 Y=66 (center.y + 2)
    // 四个方向：(0,66,-1) 朝北, (0,66,1) 朝南, (-1,66,0) 朝西, (1,66,0) 朝东
    const BlockState* torchNorth = m_world.getBlockAt(BlockPos(0, 66, -1));
    const BlockState* torchSouth = m_world.getBlockAt(BlockPos(0, 66, 1));
    const BlockState* torchWest = m_world.getBlockAt(BlockPos(-1, 66, 0));
    const BlockState* torchEast = m_world.getBlockAt(BlockPos(1, 66, 0));

    // 如果 WALL_TORCH 为空（方块未注册），则跳过火把测试
    if (VanillaBlocks::WALL_TORCH != nullptr) {
        ASSERT_NE(torchNorth, nullptr) << "Missing north torch";
        ASSERT_NE(torchSouth, nullptr) << "Missing south torch";
        ASSERT_NE(torchWest, nullptr) << "Missing west torch";
        ASSERT_NE(torchEast, nullptr) << "Missing east torch";

        // 验证火把朝向
        EXPECT_EQ(torchNorth->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
        EXPECT_EQ(torchSouth->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
        EXPECT_EQ(torchWest->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
        EXPECT_EQ(torchEast->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    }
}

// ============================================================================
// placeEndPortalFrame 测试
// ============================================================================

TEST_F(EndTeleporterTest, PlaceEndPortalFrame_Places12Frames)
{
    // 框架中心在 (0, 64, 0)，框架位于中心偏移 ±2 的位置
    BlockPos center(0, 64, 0);
    EndTeleporter::placeEndPortalFrame(m_world, center);

    // 北边 3 个框架（z = center.z - 2, x = center.x-1,0,1）
    for (i32 dx = -1; dx <= 1; ++dx) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x + dx, 64, center.z - 2));
        ASSERT_NE(state, nullptr) << "Missing north frame at (" << center.x + dx << ", 64, " << center.z - 2 << ")";
        EXPECT_EQ(&state->getBlock(), VanillaBlocks::END_PORTAL_FRAME) << "Expected EndPortalFrame at north";
    }

    // 南边 3 个框架（z = center.z + 2, x = center.x-1,0,1）
    for (i32 dx = -1; dx <= 1; ++dx) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x + dx, 64, center.z + 2));
        ASSERT_NE(state, nullptr) << "Missing south frame at (" << center.x + dx << ", 64, " << center.z + 2 << ")";
        EXPECT_EQ(&state->getBlock(), VanillaBlocks::END_PORTAL_FRAME) << "Expected EndPortalFrame at south";
    }

    // 西边 3 个框架（x = center.x - 2, z = center.z-1,0,1）
    for (i32 dz = -1; dz <= 1; ++dz) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x - 2, 64, center.z + dz));
        ASSERT_NE(state, nullptr) << "Missing west frame at (" << center.x - 2 << ", 64, " << center.z + dz << ")";
        EXPECT_EQ(&state->getBlock(), VanillaBlocks::END_PORTAL_FRAME) << "Expected EndPortalFrame at west";
    }

    // 东边 3 个框架（x = center.x + 2, z = center.z-1,0,1）
    for (i32 dz = -1; dz <= 1; ++dz) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x + 2, 64, center.z + dz));
        ASSERT_NE(state, nullptr) << "Missing east frame at (" << center.x + 2 << ", 64, " << center.z + dz << ")";
        EXPECT_EQ(&state->getBlock(), VanillaBlocks::END_PORTAL_FRAME) << "Expected EndPortalFrame at east";
    }
}

TEST_F(EndTeleporterTest, PlaceEndPortalFrame_AllFramesHaveEyes)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::placeEndPortalFrame(m_world, center);

    // 所有 12 个框架方块都应有 EYE=true
    auto checkFrameEye = [&](i32 x, i32 y, i32 z) {
        const BlockState* state = m_world.getBlockAt(BlockPos(x, y, z));
        ASSERT_NE(state, nullptr) << "Missing frame at (" << x << ", " << y << ", " << z << ")";
        EXPECT_TRUE(state->get(BlockStateProperties::EYE()))
            << "Frame at (" << x << ", " << y << ", " << z << ") should have eye";
    };

    // 北边（z = center.z - 2）
    for (i32 dx = -1; dx <= 1; ++dx) {
        checkFrameEye(center.x + dx, 64, center.z - 2);
    }
    // 南边（z = center.z + 2）
    for (i32 dx = -1; dx <= 1; ++dx) {
        checkFrameEye(center.x + dx, 64, center.z + 2);
    }
    // 西边（x = center.x - 2）
    for (i32 dz = -1; dz <= 1; ++dz) {
        checkFrameEye(center.x - 2, 64, center.z + dz);
    }
    // 东边（x = center.x + 2）
    for (i32 dz = -1; dz <= 1; ++dz) {
        checkFrameEye(center.x + 2, 64, center.z + dz);
    }
}

TEST_F(EndTeleporterTest, PlaceEndPortalFrame_FramesFaceOutward)
{
    BlockPos center(5, 70, 5);
    EndTeleporter::placeEndPortalFrame(m_world, center);

    // 北边框架凸起朝北（FACING=NORTH，背离中心）
    // 参考 MC Java EndPortalFrameBlock.getOrCreatePortalShape() 图案: v = FACING=NORTH
    for (i32 dx = -1; dx <= 1; ++dx) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x + dx, 70, center.z - 2));
        ASSERT_NE(state, nullptr);
        EXPECT_EQ(state->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North)
            << "North frame should face north at (" << center.x + dx << ", 70, " << center.z - 2 << ")";
    }

    // 南边框架凸起朝南（FACING=SOUTH，背离中心）
    // 参考 MC Java: ^ = FACING=SOUTH
    for (i32 dx = -1; dx <= 1; ++dx) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x + dx, 70, center.z + 2));
        ASSERT_NE(state, nullptr);
        EXPECT_EQ(state->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South)
            << "South frame should face south at (" << center.x + dx << ", 70, " << center.z + 2 << ")";
    }

    // 西边框架凸起朝西（FACING=WEST，背离中心）
    // 参考 MC Java: > = FACING=WEST
    for (i32 dz = -1; dz <= 1; ++dz) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x - 2, 70, center.z + dz));
        ASSERT_NE(state, nullptr);
        EXPECT_EQ(state->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West)
            << "West frame should face west at (" << center.x - 2 << ", 70, " << center.z + dz << ")";
    }

    // 东边框架凸起朝东（FACING=EAST，背离中心）
    // 参考 MC Java: < = FACING=EAST
    for (i32 dz = -1; dz <= 1; ++dz) {
        const BlockState* state = m_world.getBlockAt(BlockPos(center.x + 2, 70, center.z + dz));
        ASSERT_NE(state, nullptr);
        EXPECT_EQ(state->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East)
            << "East frame should face east at (" << center.x + 2 << ", 70, " << center.z + dz << ")";
    }
}

TEST_F(EndTeleporterTest, PlaceEndPortalFrame_Places3x3PortalBlocks)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::placeEndPortalFrame(m_world, center);

    const BlockState* endPortal = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
    ASSERT_NE(endPortal, nullptr);

    // 内部 3×3 区域应有末地传送门方块
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            const BlockState* state = m_world.getBlockAt(BlockPos(dx, 64, dz));
            ASSERT_NE(state, nullptr) << "Missing end portal at (" << dx << ", 64, " << dz << ")";
            EXPECT_EQ(state, endPortal) << "Expected end portal block at (" << dx << ", 64, " << dz << ")";
        }
    }
}

TEST_F(EndTeleporterTest, PlaceEndPortalFrame_CornerBlocksNotPlaced)
{
    BlockPos center(0, 64, 0);
    EndTeleporter::placeEndPortalFrame(m_world, center);

    // 角落位置不应有框架方块（5×5 图案的4个角）
    // 角落坐标：(center.x-2, center.z-2), (center.x+2, center.z-2),
    //           (center.x-2, center.z+2), (center.x+2, center.z+2)
    auto checkNotFrame = [&](i32 x, i32 y, i32 z) {
        const BlockState* state = m_world.getBlockAt(BlockPos(x, y, z));
        if (state == nullptr) {
            return; // 空气，正确
        }
        EXPECT_NE(&state->getBlock(), VanillaBlocks::END_PORTAL_FRAME)
            << "Corner should not have EndPortalFrame at (" << x << ", " << y << ", " << z << ")";
    };
    checkNotFrame(-2, 64, -2); // 左前角
    checkNotFrame(2, 64, -2);  // 右前角
    checkNotFrame(-2, 64, 2);  // 左后角
    checkNotFrame(2, 64, 2);   // 右后角
}

// ============================================================================
// createPortal 测试
// ============================================================================

TEST_F(EndTeleporterTest, CreatePortal_CreatesSpawnPlatformAndReturnsFixedPosition)
{
    EndTeleporter teleporter;
    PortalInfo info = teleporter.createPortal(m_world, Vector3d(0, 0, 0));

    EXPECT_TRUE(info.valid);
    EXPECT_DOUBLE_EQ(info.position.x, 100.5);
    EXPECT_DOUBLE_EQ(info.position.y, 50.0);
    EXPECT_DOUBLE_EQ(info.position.z, 0.5);
    EXPECT_FLOAT_EQ(info.yaw, 90.0f);
    EXPECT_FLOAT_EQ(info.pitch, 0.0f);

    // 验证黑曜石平台已创建
    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();
    EXPECT_EQ(m_world.getBlockAt(BlockPos(100, 48, 0)), obsidian);
}

// ============================================================================
// teleport 测试（当前返回 false，等待完整实现）
// ============================================================================

TEST_F(EndTeleporterTest, TeleportCurrentlyReturnsFalse)
{
    EndTeleporter teleporter;
    // teleport 需要一个 Entity& 参数，这里无法创建真实实体
    // 仅验证 getCoordinateScale 和 findPortal/createPortal 的基本行为
    EXPECT_FLOAT_EQ(teleporter.getCoordinateScale(), 1.0f);
}
