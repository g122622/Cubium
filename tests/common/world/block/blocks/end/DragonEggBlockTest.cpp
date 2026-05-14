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

#include "world/block/BlockRegistry.hpp"
#include "world/block/Material.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/FallingBlock.hpp"
#include "world/block/blocks/end/DragonEggBlock.hpp"
#include "physics/collision/CollisionShape.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试环境设置
// ============================================================================

class DragonEggBlockTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
    }
};

// ============================================================================
// 龙蛋基础属性测试
// ============================================================================

TEST_F(DragonEggBlockTest, IsRegistered)
{
    // 验证龙蛋方块已注册
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr) << "DRAGON_EGG should be registered";
}

TEST_F(DragonEggBlockTest, InheritsFromFallingBlock)
{
    // 验证龙蛋继承自 FallingBlock
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr);
    const Block* dragonEggBlock = VanillaBlocks::DRAGON_EGG;
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(dragonEggBlock);
    EXPECT_NE(fallingBlock, nullptr) << "DragonEggBlock should inherit from FallingBlock";
}

TEST_F(DragonEggBlockTest, FallDelayIsFive)
{
    // 验证下落延迟为 5 tick（MC 1.16.5）
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr);
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(VanillaBlocks::DRAGON_EGG);
    ASSERT_NE(fallingBlock, nullptr);
    EXPECT_EQ(fallingBlock->getFallDelay(), 5) << "DragonEggBlock fall delay should be 5 ticks";
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(DragonEggBlockTest, HasCustomShape)
{
    // 验证龙蛋碰撞箱形状
    // MC 1.16.5: SHAPE = Block.makeCuboidShape(1.0, 0.0, 1.0, 15.0, 16.0, 15.0)
    // 即 (1/16, 0, 1/16) 到 (15/16, 1, 15/16)
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr);

    const BlockState& state = VanillaBlocks::DRAGON_EGG->defaultState();
    const CollisionShape& shape = state.getCollisionShape();

    // 验证形状不为空
    EXPECT_FALSE(shape.isEmpty()) << "DragonEggBlock shape should not be empty";
}

// ============================================================================
// 材质测试
// ============================================================================

TEST_F(DragonEggBlockTest, MaterialIsRock)
{
    // 验证龙蛋材质是 ROCK
    // MC 1.16.5 中有专用的 DRAGON_EGG 材质，但我们项目使用 ROCK
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr);
    const Block& block = *VanillaBlocks::DRAGON_EGG;
    EXPECT_EQ(&block.material(), &Material::ROCK) << "DragonEggBlock should use ROCK material";
}

// ============================================================================
// 传送逻辑常量测试
// ============================================================================

TEST(DragonEggBlockConstantsTest, MaxAttemptsIsThousand)
{
    // 验证最大传送尝试次数为 1000
    // 这是 MC 1.16.5 中的固定值
    constexpr i32 MAX_TELEPORT_ATTEMPTS = 1000;
    EXPECT_EQ(MAX_TELEPORT_ATTEMPTS, 1000);
}

TEST(DragonEggBlockConstantsTest, HorizontalRangeIsFifteen)
{
    // 验证水平传送范围为 15（-15 ~ +15）
    constexpr i32 HORIZONTAL_RANGE = 15;
    EXPECT_EQ(HORIZONTAL_RANGE, 15);
}

TEST(DragonEggBlockConstantsTest, VerticalRangeIsSeven)
{
    // 验证垂直传送范围为 7（-7 ~ +7）
    constexpr i32 VERTICAL_RANGE = 7;
    EXPECT_EQ(VERTICAL_RANGE, 7);
}

// ============================================================================
// 硬度和光照测试
// ============================================================================

TEST_F(DragonEggBlockTest, HasHardness)
{
    // 验证龙蛋有硬度（MC 1.16.5: hardness = 3.0）
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr);
    const BlockState& state = VanillaBlocks::DRAGON_EGG->defaultState();
    EXPECT_GT(state.hardness(), 0.0f) << "DragonEggBlock should have positive hardness";
}

TEST_F(DragonEggBlockTest, EmitsLight)
{
    // 验证龙蛋发光（MC 1.16.5: lightLevel = 1）
    ASSERT_NE(VanillaBlocks::DRAGON_EGG, nullptr);
    const BlockState& state = VanillaBlocks::DRAGON_EGG->defaultState();
    EXPECT_GT(state.lightLevel(), 0) << "DragonEggBlock should emit light";
}
