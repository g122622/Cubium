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
 * @file AnvilBlockTest.cpp
 * @brief AnvilBlock 单元测试
 *
 * 测试 AnvilBlock 的核心功能：状态属性、损坏状态转换、朝向旋转等。
 */

#include "common/world/block/blocks/functional/AnvilBlock.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief AnvilBlock 测试固件
 */
class AnvilBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override {}
};

/**
 * @brief 测试铁砧方块默认朝向
 *
 * MC 原版：铁砧默认朝向北
 */
TEST_F(AnvilBlockTest, DefaultFacingIsNorth)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& defaultState = anvil->defaultState();
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::HORIZONTAL_FACING()));
    Direction facing = defaultState.get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::North);
}

/**
 * @brief 测试铁砧三种变体都注册成功
 */
TEST_F(AnvilBlockTest, AllAnvilVariantsRegistered)
{
    EXPECT_NE(block_registry::BuildingBlocks::ANVIL, nullptr);
    EXPECT_NE(block_registry::BuildingBlocks::CHIPPED_ANVIL, nullptr);
    EXPECT_NE(block_registry::BuildingBlocks::DAMAGED_ANVIL, nullptr);
}

/**
 * @brief 测试铁砧变体的方块ID互不相同
 */
TEST_F(AnvilBlockTest, AnvilVariantsHaveDifferentBlockIds)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;

    EXPECT_NE(anvil->blockId(), chipped->blockId());
    EXPECT_NE(chipped->blockId(), damaged->blockId());
    EXPECT_NE(anvil->blockId(), damaged->blockId());
}

/**
 * @brief 测试铁砧损坏状态转换：anvil → chipped_anvil
 */
TEST_F(AnvilBlockTest, DamageAnvilToIntact)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& state = anvil->defaultState();
    const BlockState* damaged = AnvilBlock::damageAnvil(state);

    ASSERT_NE(damaged, nullptr);
    EXPECT_EQ(damaged->getBlock().blockLocation(), ResourceLocation("minecraft", "chipped_anvil"));
}

/**
 * @brief 测试铁砧损坏状态转换：chipped_anvil → damaged_anvil
 */
TEST_F(AnvilBlockTest, DamageAnvilToChipped)
{
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    ASSERT_NE(chipped, nullptr);

    const BlockState& state = chipped->defaultState();
    const BlockState* damaged = AnvilBlock::damageAnvil(state);

    ASSERT_NE(damaged, nullptr);
    EXPECT_EQ(damaged->getBlock().blockLocation(), ResourceLocation("minecraft", "damaged_anvil"));
}

/**
 * @brief 测试铁砧损坏状态转换：damaged_anvil → nullptr（完全摧毁）
 */
TEST_F(AnvilBlockTest, DamageAnvilToDestroyed)
{
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;
    ASSERT_NE(damaged, nullptr);

    const BlockState& state = damaged->defaultState();
    const BlockState* result = AnvilBlock::damageAnvil(state);

    EXPECT_EQ(result, nullptr);
}

/**
 * @brief 测试铁砧损坏时保留朝向属性
 *
 * 损坏后的铁砧应保留原始的 HORIZONTAL_FACING
 */
TEST_F(AnvilBlockTest, DamageAnvilPreservesFacing)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // 设置朝向为东
    const BlockState& facingEast =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState* damaged = AnvilBlock::damageAnvil(facingEast);

    ASSERT_NE(damaged, nullptr);
    Direction facing = damaged->get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::East);
}

/**
 * @brief 测试铁砧损坏链完整性：anvil → chipped → damaged → destroyed
 */
TEST_F(AnvilBlockTest, DamageChainComplete)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // 第一级损坏
    const BlockState* state1 = &anvil->defaultState();
    const BlockState* state2 = AnvilBlock::damageAnvil(*state1);
    ASSERT_NE(state2, nullptr);
    EXPECT_EQ(state2->getBlock().blockLocation(), ResourceLocation("minecraft", "chipped_anvil"));

    // 第二级损坏
    const BlockState* state3 = AnvilBlock::damageAnvil(*state2);
    ASSERT_NE(state3, nullptr);
    EXPECT_EQ(state3->getBlock().blockLocation(), ResourceLocation("minecraft", "damaged_anvil"));

    // 第三级损坏（完全摧毁）
    const BlockState* state4 = AnvilBlock::damageAnvil(*state3);
    EXPECT_EQ(state4, nullptr);
}

/**
 * @brief 测试非铁砧方块调用 damageAnvil 返回 nullptr
 */
TEST_F(AnvilBlockTest, DamageAnvilOnNonAnvilReturnsNull)
{
    const Block* sand = VanillaBlocks::SAND;
    ASSERT_NE(sand, nullptr);

    const BlockState& state = sand->defaultState();
    const BlockState* result = AnvilBlock::damageAnvil(state);

    EXPECT_EQ(result, nullptr);
}

/**
 * @brief 测试铁砧方块在 ANVIL 标签中
 */
TEST_F(AnvilBlockTest, AnvilInAnvilBlockTag)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;

    ASSERT_NE(anvil, nullptr);
    ASSERT_NE(chipped, nullptr);
    ASSERT_NE(damaged, nullptr);

    EXPECT_TRUE(BlockTags::ANVIL().contains(*anvil));
    EXPECT_TRUE(BlockTags::ANVIL().contains(*chipped));
    EXPECT_TRUE(BlockTags::ANVIL().contains(*damaged));

    // 非铁砧方块不在标签中
    const Block* sand = VanillaBlocks::SAND;
    ASSERT_NE(sand, nullptr);
    EXPECT_FALSE(BlockTags::ANVIL().contains(*sand));
}

/**
 * @brief 测试铁砧旋转
 */
TEST_F(AnvilBlockTest, Rotation)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& north = anvil->defaultState();
    EXPECT_EQ(north.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // 顺时针旋转 90 度
    const BlockState& east = anvil->rotate(north, Rotation::Clockwise90);
    EXPECT_EQ(east.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 旋转 180 度
    const BlockState& south = anvil->rotate(north, Rotation::Clockwise180);
    EXPECT_EQ(south.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

/**
 * @brief 测试铁砧镜像
 */
TEST_F(AnvilBlockTest, Mirror)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // 北朝向
    const BlockState& north = anvil->defaultState();

    // 前后镜像：南北互换
    const BlockState& mirroredFB = anvil->mirror(north, Mirror::FrontBack);
    EXPECT_EQ(mirroredFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // 左右镜像：北不变
    const BlockState& mirroredLR = anvil->mirror(north, Mirror::LeftRight);
    EXPECT_EQ(mirroredLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

} // namespace test
} // namespace blocks
} // namespace mc
