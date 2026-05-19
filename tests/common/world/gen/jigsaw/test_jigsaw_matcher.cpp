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

#include "common/world/gen/jigsaw/JigsawOrientation.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::jigsaw;

class JigsawMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// JigsawOrientation 测试
// ============================================================================

class JigsawOrientationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(JigsawOrientationTest, FromFacingAndRotation)
{
    // DOWN facing (rotation可以是水平方向)
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::Down, Direction::East), JigsawOrientation::DownEast);
    EXPECT_EQ(
        JigsawOrientations::fromFacingAndRotation(Direction::Down, Direction::North), JigsawOrientation::DownNorth);
    EXPECT_EQ(
        JigsawOrientations::fromFacingAndRotation(Direction::Down, Direction::South), JigsawOrientation::DownSouth);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::Down, Direction::West), JigsawOrientation::DownWest);

    // UP facing (rotation可以是水平方向)
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::Up, Direction::East), JigsawOrientation::UpEast);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::Up, Direction::North), JigsawOrientation::UpNorth);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::Up, Direction::South), JigsawOrientation::UpSouth);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::Up, Direction::West), JigsawOrientation::UpWest);

    // 水平 facing (rotation只能是UP)
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::North, Direction::Up), JigsawOrientation::NorthUp);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::South, Direction::Up), JigsawOrientation::SouthUp);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::West, Direction::Up), JigsawOrientation::WestUp);
    EXPECT_EQ(JigsawOrientations::fromFacingAndRotation(Direction::East, Direction::Up), JigsawOrientation::EastUp);
}

TEST_F(JigsawOrientationTest, GetFacing)
{
    // DOWN facing
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::DownEast), Direction::Down);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::DownNorth), Direction::Down);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::DownSouth), Direction::Down);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::DownWest), Direction::Down);

    // UP facing
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::UpEast), Direction::Up);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::UpNorth), Direction::Up);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::UpSouth), Direction::Up);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::UpWest), Direction::Up);

    // 水平 facing
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::NorthUp), Direction::North);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::SouthUp), Direction::South);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::WestUp), Direction::West);
    EXPECT_EQ(JigsawOrientations::getFacing(JigsawOrientation::EastUp), Direction::East);
}

TEST_F(JigsawOrientationTest, GetRotation)
{
    // DOWN facing
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::DownEast), Direction::East);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::DownNorth), Direction::North);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::DownSouth), Direction::South);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::DownWest), Direction::West);

    // UP facing
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::UpEast), Direction::East);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::UpNorth), Direction::North);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::UpSouth), Direction::South);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::UpWest), Direction::West);

    // 水平 facing (rotation固定为UP)
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::NorthUp), Direction::Up);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::SouthUp), Direction::Up);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::WestUp), Direction::Up);
    EXPECT_EQ(JigsawOrientations::getRotation(JigsawOrientation::EastUp), Direction::Up);
}

TEST_F(JigsawOrientationTest, CanConnectOrientation)
{
    // Rollable: 只需facing相反

    // UP和DOWN相对（rollable）
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpEast, JigsawOrientation::DownNorth, JigsawJointType::Rollable));
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpNorth, JigsawOrientation::DownSouth, JigsawJointType::Rollable));

    // NORTH和SOUTH相对（rollable）
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::NorthUp, JigsawOrientation::SouthUp, JigsawJointType::Rollable));

    // EAST和WEST相对（rollable）
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::EastUp, JigsawOrientation::WestUp, JigsawJointType::Rollable));

    // 不相对的facing（应该失败）
    EXPECT_FALSE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpEast, JigsawOrientation::NorthUp, JigsawJointType::Rollable));
    EXPECT_FALSE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::DownEast, JigsawOrientation::WestUp, JigsawJointType::Rollable));

    // Aligned: facing相反且rotation相同

    // UP和DOWN相对，rotation相同
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpEast, JigsawOrientation::DownEast, JigsawJointType::Aligned));
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpNorth, JigsawOrientation::DownNorth, JigsawJointType::Aligned));

    // UP和DOWN相对，但rotation不同（应该失败）
    EXPECT_FALSE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpEast, JigsawOrientation::DownNorth, JigsawJointType::Aligned));
    EXPECT_FALSE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::UpNorth, JigsawOrientation::DownSouth, JigsawJointType::Aligned));

    // 水平facing的Aligned（rotation都是Up，所以总是匹配）
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::NorthUp, JigsawOrientation::SouthUp, JigsawJointType::Aligned));
    EXPECT_TRUE(JigsawMatcher::canConnectOrientation(
        JigsawOrientation::EastUp, JigsawOrientation::WestUp, JigsawJointType::Aligned));
}

TEST_F(JigsawOrientationTest, Rotate)
{
    using namespace mc;

    // 测试旋转
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::NorthUp, Rotation::None), JigsawOrientation::NorthUp);
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::NorthUp, Rotation::Clockwise90), JigsawOrientation::EastUp);
    EXPECT_EQ(
        JigsawOrientations::rotate(JigsawOrientation::NorthUp, Rotation::Clockwise180), JigsawOrientation::SouthUp);
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::NorthUp, Rotation::CounterClockwise90),
        JigsawOrientation::WestUp);

    // 测试UP facing的旋转（rotation是水平方向）
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::UpNorth, Rotation::Clockwise90), JigsawOrientation::UpEast);
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::UpEast, Rotation::Clockwise90), JigsawOrientation::UpSouth);
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::UpSouth, Rotation::Clockwise90), JigsawOrientation::UpWest);
    EXPECT_EQ(JigsawOrientations::rotate(JigsawOrientation::UpWest, Rotation::Clockwise90), JigsawOrientation::UpNorth);
}

TEST_F(JigsawOrientationTest, Mirror)
{
    using namespace mc;

    // LeftRight镜像（Z轴镜像：North<->South）
    EXPECT_EQ(JigsawOrientations::mirror(JigsawOrientation::NorthUp, Mirror::LeftRight), JigsawOrientation::SouthUp);
    EXPECT_EQ(JigsawOrientations::mirror(JigsawOrientation::SouthUp, Mirror::LeftRight), JigsawOrientation::NorthUp);
    EXPECT_EQ(JigsawOrientations::mirror(JigsawOrientation::EastUp, Mirror::LeftRight),
        JigsawOrientation::EastUp); // East不受影响

    // FrontBack镜像（X轴镜像：East<->West）
    EXPECT_EQ(JigsawOrientations::mirror(JigsawOrientation::EastUp, Mirror::FrontBack), JigsawOrientation::WestUp);
    EXPECT_EQ(JigsawOrientations::mirror(JigsawOrientation::WestUp, Mirror::FrontBack), JigsawOrientation::EastUp);
    EXPECT_EQ(JigsawOrientations::mirror(JigsawOrientation::NorthUp, Mirror::FrontBack),
        JigsawOrientation::NorthUp); // North不受影响
}

TEST_F(JigsawOrientationTest, Opposite)
{
    // UP和DOWN互为相反
    EXPECT_EQ(JigsawOrientations::opposite(JigsawOrientation::UpEast), JigsawOrientation::DownEast);
    EXPECT_EQ(JigsawOrientations::opposite(JigsawOrientation::DownNorth), JigsawOrientation::UpNorth);

    // 水平facing的相反
    EXPECT_EQ(JigsawOrientations::opposite(JigsawOrientation::NorthUp), JigsawOrientation::SouthUp);
    EXPECT_EQ(JigsawOrientations::opposite(JigsawOrientation::SouthUp), JigsawOrientation::NorthUp);
    EXPECT_EQ(JigsawOrientations::opposite(JigsawOrientation::EastUp), JigsawOrientation::WestUp);
    EXPECT_EQ(JigsawOrientations::opposite(JigsawOrientation::WestUp), JigsawOrientation::EastUp);
}

TEST_F(JigsawOrientationTest, FromName)
{
    auto downEast = JigsawOrientations::fromName("down_east");
    EXPECT_TRUE(downEast.has_value());
    EXPECT_EQ(downEast.value(), JigsawOrientation::DownEast);

    auto upNorth = JigsawOrientations::fromName("up_north");
    EXPECT_TRUE(upNorth.has_value());
    EXPECT_EQ(upNorth.value(), JigsawOrientation::UpNorth);

    auto northUp = JigsawOrientations::fromName("north_up");
    EXPECT_TRUE(northUp.has_value());
    EXPECT_EQ(northUp.value(), JigsawOrientation::NorthUp);

    auto invalid = JigsawOrientations::fromName("invalid");
    EXPECT_FALSE(invalid.has_value());
}

TEST_F(JigsawOrientationTest, ToString)
{
    EXPECT_EQ(JigsawOrientations::toString(JigsawOrientation::DownEast), "down_east");
    EXPECT_EQ(JigsawOrientations::toString(JigsawOrientation::UpNorth), "up_north");
    EXPECT_EQ(JigsawOrientations::toString(JigsawOrientation::NorthUp), "north_up");
    EXPECT_EQ(JigsawOrientations::toString(JigsawOrientation::SouthUp), "south_up");
    EXPECT_EQ(JigsawOrientations::toString(JigsawOrientation::WestUp), "west_up");
    EXPECT_EQ(JigsawOrientations::toString(JigsawOrientation::EastUp), "east_up");
}

// ============================================================================
// JigsawMatcher 测试
// ============================================================================

TEST_F(JigsawMatcherTest, CanMatchByName)
{
    // 相同名称匹配
    EXPECT_TRUE(JigsawMatcher::canMatchByName("village:street", "village:street"));
    EXPECT_TRUE(JigsawMatcher::canMatchByName("minecraft:top", "minecraft:top"));

    // 不同名称不匹配
    EXPECT_FALSE(JigsawMatcher::canMatchByName("village:street", "village:house"));
    EXPECT_FALSE(JigsawMatcher::canMatchByName("minecraft:top", "minecraft:bottom"));

    // 空名称不匹配
    EXPECT_FALSE(JigsawMatcher::canMatchByName("", "minecraft:top"));
    EXPECT_FALSE(JigsawMatcher::canMatchByName("minecraft:top", ""));
    EXPECT_FALSE(JigsawMatcher::canMatchByName("", ""));

    // minecraft:empty 不匹配
    EXPECT_FALSE(JigsawMatcher::canMatchByName("minecraft:empty", "anything"));
    EXPECT_FALSE(JigsawMatcher::canMatchByName("anything", "minecraft:empty"));
    EXPECT_FALSE(JigsawMatcher::canMatchByName("minecraft:empty", "minecraft:empty"));
}

TEST_F(JigsawMatcherTest, GetDefaultJointType)
{
    // 水平facing默认为Aligned
    EXPECT_EQ(JigsawMatcher::getDefaultJointType(JigsawOrientation::NorthUp), JigsawJointType::Aligned);
    EXPECT_EQ(JigsawMatcher::getDefaultJointType(JigsawOrientation::SouthUp), JigsawJointType::Aligned);
    EXPECT_EQ(JigsawMatcher::getDefaultJointType(JigsawOrientation::EastUp), JigsawJointType::Aligned);
    EXPECT_EQ(JigsawMatcher::getDefaultJointType(JigsawOrientation::WestUp), JigsawJointType::Aligned);

    // 垂直facing默认为Rollable
    EXPECT_EQ(JigsawMatcher::getDefaultJointType(JigsawOrientation::UpEast), JigsawJointType::Rollable);
    EXPECT_EQ(JigsawMatcher::getDefaultJointType(JigsawOrientation::DownNorth), JigsawJointType::Rollable);
}

TEST_F(JigsawMatcherTest, JointTypeFromString)
{
    EXPECT_EQ(JigsawMatcher::jointTypeFromString("rollable"), JigsawJointType::Rollable);
    EXPECT_EQ(JigsawMatcher::jointTypeFromString("aligned"), JigsawJointType::Aligned);

    auto invalid = JigsawMatcher::jointTypeFromString("invalid");
    EXPECT_FALSE(invalid.has_value());
}

TEST_F(JigsawMatcherTest, JointTypeToString)
{
    EXPECT_EQ(JigsawMatcher::jointTypeToString(JigsawJointType::Rollable), "rollable");
    EXPECT_EQ(JigsawMatcher::jointTypeToString(JigsawJointType::Aligned), "aligned");
}

TEST_F(JigsawMatcherTest, CanMatchFull)
{
    // 测试完整的匹配逻辑

    // Rollable连接：facing相反即可
    EXPECT_TRUE(JigsawMatcher::canMatch("village:street",
        "village:street",
        JigsawOrientation::UpEast,
        JigsawOrientation::DownNorth,
        JigsawJointType::Rollable));

    // Aligned连接：facing相反且rotation相同
    EXPECT_TRUE(JigsawMatcher::canMatch("village:street",
        "village:street",
        JigsawOrientation::UpEast,
        JigsawOrientation::DownEast,
        JigsawJointType::Aligned));

    // Aligned连接：facing相反但rotation不同（应该失败）
    EXPECT_FALSE(JigsawMatcher::canMatch("village:street",
        "village:street",
        JigsawOrientation::UpEast,
        JigsawOrientation::DownNorth,
        JigsawJointType::Aligned));

    // 名称不匹配
    EXPECT_FALSE(JigsawMatcher::canMatch("village:street",
        "village:house",
        JigsawOrientation::UpEast,
        JigsawOrientation::DownEast,
        JigsawJointType::Aligned));

    // facing不相反
    EXPECT_FALSE(JigsawMatcher::canMatch("village:street",
        "village:street",
        JigsawOrientation::UpEast,
        JigsawOrientation::NorthUp,
        JigsawJointType::Rollable));
}

TEST_F(JigsawMatcherTest, RotateName)
{
    // top/bottom不受旋转影响
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:top", 0), "minecraft:top");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:top", 90), "minecraft:top");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:top", 180), "minecraft:top");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:bottom", 90), "minecraft:bottom");

    // 水平方向旋转
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:front", 0), "minecraft:front");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:front", 90), "minecraft:right");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:front", 180), "minecraft:back");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:front", 270), "minecraft:left");

    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:right", 90), "minecraft:back");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:back", 90), "minecraft:left");
    EXPECT_EQ(JigsawMatcher::rotateName("minecraft:left", 90), "minecraft:front");

    // 空名称
    EXPECT_EQ(JigsawMatcher::rotateName("", 90), "");

    // 非标准名称保持不变
    EXPECT_EQ(JigsawMatcher::rotateName("village:custom", 90), "village:custom");
}

// ============================================================================
// JigsawJoint 测试
// ============================================================================

TEST_F(JigsawMatcherTest, JigsawJointDefaults)
{
    JigsawJoint joint;

    EXPECT_EQ(joint.sourcePos, BlockPos());
    EXPECT_TRUE(joint.sourceName.empty());
    EXPECT_TRUE(joint.targetPool.empty());
    EXPECT_TRUE(joint.targetName.empty());
    EXPECT_EQ(joint.projection, JigsawPlacementBehaviour::Rigid);
    EXPECT_EQ(joint.jointType, JigsawJointType::Rollable);
    EXPECT_EQ(joint.orientation, JigsawOrientation::NorthUp);
    EXPECT_EQ(joint.sourceGroundY, 0);
}
