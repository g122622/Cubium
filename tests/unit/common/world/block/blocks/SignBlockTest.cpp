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
 * @file SignBlockTest.cpp
 * @brief SignBlock 基础测试
 *
 * 测试告示牌方块的基础功能：
 * - WoodType 属性
 * - 方块实体创建
 * - 状态属性
 */

#include <memory>
#include <gtest/gtest.h>

#include "util/property/Properties.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/SignBlock.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "world/blockentity/interactive/SignEntity.hpp"

using namespace mc;
using namespace mc::blocks;
using namespace mc::blockentity;

// ============================================================================
// SignBlock 基础测试
// ============================================================================

class SignBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册内置方块实体类型
        BlockEntityRegistry::instance().registerBuiltinTypes();

        // 创建站立告示牌
        standingSign_ = std::make_unique<StandingSignBlock>(
            BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::Oak);

        // 创建墙面告示牌
        wallSign_ =
            std::make_unique<WallSignBlock>(BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::Oak);
    }

    std::unique_ptr<StandingSignBlock> standingSign_;
    std::unique_ptr<WallSignBlock> wallSign_;
};

// ========== StandingSignBlock 测试 ==========

TEST_F(SignBlockTest, StandingSignBlock_Create)
{
    EXPECT_NE(standingSign_, nullptr);
    EXPECT_TRUE(standingSign_->hasBlockEntity());
}

TEST_F(SignBlockTest, StandingSignBlock_CreateBlockEntity)
{
    BlockPos pos(10, 64, 20);
    auto entity = standingSign_->createBlockEntity(pos);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(SignBlockTest, StandingSignBlock_WoodType)
{
    EXPECT_EQ(standingSign_->getWoodType(), WoodType::Oak);
}

TEST_F(SignBlockTest, StandingSignBlock_DefaultRotation)
{
    const auto& state = standingSign_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::ROTATION_0_15()), 0);
}

TEST_F(SignBlockTest, StandingSignBlock_DefaultWaterlogged)
{
    const auto& state = standingSign_->defaultState();
    EXPECT_FALSE(standingSign_->isWaterlogged(state));
}

TEST_F(SignBlockTest, StandingSignBlock_Rotate90)
{
    const auto& state = standingSign_->defaultState();
    const auto& rotated = standingSign_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::ROTATION_0_15()), 4);
}

TEST_F(SignBlockTest, StandingSignBlock_Rotate180)
{
    const auto& state = standingSign_->defaultState();
    const auto& rotated = standingSign_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated.get(BlockStateProperties::ROTATION_0_15()), 8);
}

TEST_F(SignBlockTest, StandingSignBlock_Rotate270)
{
    const auto& state = standingSign_->defaultState();
    const auto& rotated = standingSign_->rotate(state, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::ROTATION_0_15()), 12);
}

TEST_F(SignBlockTest, StandingSignBlock_MirrorNone)
{
    const auto& state = standingSign_->defaultState();
    const auto& mirrored = standingSign_->mirror(state, Mirror::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::ROTATION_0_15()), 0);
}

TEST_F(SignBlockTest, StandingSignBlock_GetShape)
{
    const auto& state = standingSign_->defaultState();
    const auto& shape = standingSign_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(SignBlockTest, StandingSignBlock_IsOpaque)
{
    const auto& state = standingSign_->defaultState();
    EXPECT_FALSE(standingSign_->isOpaque(state));
}

// ========== WallSignBlock 测试 ==========

TEST_F(SignBlockTest, WallSignBlock_Create)
{
    EXPECT_NE(wallSign_, nullptr);
    EXPECT_TRUE(wallSign_->hasBlockEntity());
}

TEST_F(SignBlockTest, WallSignBlock_CreateBlockEntity)
{
    BlockPos pos(5, 64, 10);
    auto entity = wallSign_->createBlockEntity(pos);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(SignBlockTest, WallSignBlock_WoodType)
{
    EXPECT_EQ(wallSign_->getWoodType(), WoodType::Oak);
}

TEST_F(SignBlockTest, WallSignBlock_DefaultFacing)
{
    const auto& state = wallSign_->defaultState();
    // WallSignBlock 注册的是 HORIZONTAL_FACING（4 向水平），而非 FACING（6 向含上下）。
    // 两者虽都名为 "facing"，但是不同的 BlockStateProperty 单例对象（按指针匹配），
    // 用 FACING() 查询 HORIZONTAL_FACING 注册的状态会取不到值（返回默认/断言失败）。
    // vanilla AbstractSignBlock / WallSignBlock 同样使用 HORIZONTAL_FACING。
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(SignBlockTest, WallSignBlock_DefaultWaterlogged)
{
    const auto& state = wallSign_->defaultState();
    EXPECT_FALSE(wallSign_->isWaterlogged(state));
}

TEST_F(SignBlockTest, WallSignBlock_Rotate90)
{
    const auto& state = wallSign_->defaultState();
    const auto& rotated = wallSign_->rotate(state, Rotation::Clockwise90);
    // 同上：HORIZONTAL_FACING（4 向水平），非 FACING（6 向）
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(SignBlockTest, WallSignBlock_Rotate180)
{
    const auto& state = wallSign_->defaultState();
    const auto& rotated = wallSign_->rotate(state, Rotation::Clockwise180);
    // 同上：HORIZONTAL_FACING（4 向水平），非 FACING（6 向）
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(SignBlockTest, WallSignBlock_GetShape)
{
    const auto& state = wallSign_->defaultState();
    const auto& shape = wallSign_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(SignBlockTest, WallSignBlock_IsOpaque)
{
    const auto& state = wallSign_->defaultState();
    EXPECT_FALSE(wallSign_->isOpaque(state));
}

// ========== WoodType 测试 ==========

TEST_F(SignBlockTest, WoodType_Spruce)
{
    auto spruceSign = std::make_unique<StandingSignBlock>(
        BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::Spruce);
    EXPECT_EQ(spruceSign->getWoodType(), WoodType::Spruce);
}

TEST_F(SignBlockTest, WoodType_Birch)
{
    auto birchSign =
        std::make_unique<StandingSignBlock>(BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::Birch);
    EXPECT_EQ(birchSign->getWoodType(), WoodType::Birch);
}

TEST_F(SignBlockTest, WoodType_Jungle)
{
    auto jungleSign = std::make_unique<StandingSignBlock>(
        BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::Jungle);
    EXPECT_EQ(jungleSign->getWoodType(), WoodType::Jungle);
}

TEST_F(SignBlockTest, WoodType_Acacia)
{
    auto acaciaSign = std::make_unique<StandingSignBlock>(
        BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::Acacia);
    EXPECT_EQ(acaciaSign->getWoodType(), WoodType::Acacia);
}

TEST_F(SignBlockTest, WoodType_DarkOak)
{
    auto darkOakSign = std::make_unique<StandingSignBlock>(
        BlockProperties(Material::WOOD).hardness(1.0f).notSolid(), WoodType::DarkOak);
    EXPECT_EQ(darkOakSign->getWoodType(), WoodType::DarkOak);
}

TEST_F(SignBlockTest, WoodType_Crimson)
{
    auto crimsonSign = std::make_unique<StandingSignBlock>(
        BlockProperties(Material::NETHER_WOOD).hardness(1.0f).notSolid(), WoodType::Crimson);
    EXPECT_EQ(crimsonSign->getWoodType(), WoodType::Crimson);
}

TEST_F(SignBlockTest, WoodType_Warped)
{
    auto warpedSign = std::make_unique<StandingSignBlock>(
        BlockProperties(Material::NETHER_WOOD).hardness(1.0f).notSolid(), WoodType::Warped);
    EXPECT_EQ(warpedSign->getWoodType(), WoodType::Warped);
}

// ========== SignEntity 与 SignBlock 关联测试 ==========

TEST_F(SignBlockTest, SignEntityCreatedFromBlock)
{
    BlockPos pos(100, 200, 300);
    auto entity = standingSign_->createBlockEntity(pos);

    auto* signEntity = dynamic_cast<SignEntity*>(entity.get());
    ASSERT_NE(signEntity, nullptr);

    // 验证 SignEntity 可以设置文本
    EXPECT_TRUE(signEntity->setLineFromLegacy(0, "Line 1"));
    EXPECT_TRUE(signEntity->setLineFromLegacy(1, "Line 2"));
    EXPECT_TRUE(signEntity->setLineFromLegacy(2, "Line 3"));
    EXPECT_TRUE(signEntity->setLineFromLegacy(3, "Line 4"));

    EXPECT_EQ(signEntity->getLineText(0), "Line 1");
    EXPECT_EQ(signEntity->getLineText(1), "Line 2");
    EXPECT_EQ(signEntity->getLineText(2), "Line 3");
    EXPECT_EQ(signEntity->getLineText(3), "Line 4");
}

TEST_F(SignBlockTest, WallSignEntityCreatedFromBlock)
{
    BlockPos pos(50, 100, 150);
    auto entity = wallSign_->createBlockEntity(pos);

    auto* signEntity = dynamic_cast<SignEntity*>(entity.get());
    ASSERT_NE(signEntity, nullptr);

    // 验证 SignEntity 可以设置颜色和发光
    signEntity->setTextColor(14); // Red
    signEntity->setGlowing(true);

    EXPECT_EQ(signEntity->getTextColor(), 14);
    EXPECT_TRUE(signEntity->isGlowing());
}
