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

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/blocks/ConcretePowderBlock.hpp"
#include "world/block/blocks/FallingBlock.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 空气穿透测试
// ============================================================================

TEST(FallingBlockCanFallThroughTest, Air_ReturnsTrue)
{
    // 空气应该可以穿透
    EXPECT_TRUE(FallingBlock::canFallThrough(nullptr));
    EXPECT_TRUE(FallingBlock::canFallThrough(&VanillaBlocks::AIR->defaultState()));
}

// ============================================================================
// 火焰穿透测试（MC 1.16.5: state.isIn(BlockTags.FIRE)）
// ============================================================================

TEST(FallingBlockCanFallThroughTest, Fire_ReturnsTrue)
{
    // 普通火应该可以穿透
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    const BlockState* fireState = &VanillaBlocks::FIRE->defaultState();
    ASSERT_NE(fireState, nullptr);

    EXPECT_TRUE(FallingBlock::canFallThrough(fireState));
}

TEST(FallingBlockCanFallThroughTest, SoulFire_ReturnsTrue)
{
    // 灵魂火应该可以穿透
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);
    const BlockState* soulFireState = &VanillaBlocks::SOUL_FIRE->defaultState();
    ASSERT_NE(soulFireState, nullptr);

    EXPECT_TRUE(FallingBlock::canFallThrough(soulFireState));
}

TEST(FallingBlockCanFallThroughTest, FireTagContainsBothFireTypes)
{
    // 验证 FIRE 标签同时包含普通火和灵魂火
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);

    EXPECT_TRUE(BlockTags::FIRE().contains(VanillaBlocks::FIRE));
    EXPECT_TRUE(BlockTags::FIRE().contains(VanillaBlocks::SOUL_FIRE));

    const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
    const BlockState& soulFireState = VanillaBlocks::SOUL_FIRE->defaultState();

    EXPECT_TRUE(BlockTags::FIRE().contains(fireState));
    EXPECT_TRUE(BlockTags::FIRE().contains(soulFireState));
}

// ============================================================================
// 液体穿透测试
// ============================================================================

TEST(FallingBlockCanFallThroughTest, Water_ReturnsTrue)
{
    // 水应该可以穿透
    ASSERT_NE(VanillaBlocks::WATER, nullptr);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    ASSERT_NE(waterState, nullptr);

    EXPECT_TRUE(FallingBlock::canFallThrough(waterState));
}

TEST(FallingBlockCanFallThroughTest, Lava_ReturnsTrue)
{
    // 岩浆应该可以穿透
    ASSERT_NE(VanillaBlocks::LAVA, nullptr);
    const BlockState* lavaState = &VanillaBlocks::LAVA->defaultState();
    ASSERT_NE(lavaState, nullptr);

    EXPECT_TRUE(FallingBlock::canFallThrough(lavaState));
}

// ============================================================================
// 可替换材质穿透测试
// ============================================================================

TEST(FallingBlockCanFallThroughTest, TallGrass_ReturnsTrue)
{
    // 草应该可以穿透（可替换材质）
    // 注意：TALL_GRASS 可能为空，如果未注册则跳过
    if (VanillaBlocks::TALL_GRASS != nullptr) {
        const BlockState* grassState = &VanillaBlocks::TALL_GRASS->defaultState();
        EXPECT_TRUE(FallingBlock::canFallThrough(grassState));
    }
}

// ============================================================================
// 固体方块不穿透测试
// ============================================================================

TEST(FallingBlockCanFallThroughTest, Stone_ReturnsFalse)
{
    // 石头不应该可以穿透
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    EXPECT_FALSE(FallingBlock::canFallThrough(stoneState));
}

TEST(FallingBlockCanFallThroughTest, Dirt_ReturnsFalse)
{
    // 泥土不应该可以穿透
    ASSERT_NE(VanillaBlocks::DIRT, nullptr);
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    ASSERT_NE(dirtState, nullptr);

    EXPECT_FALSE(FallingBlock::canFallThrough(dirtState));
}

// ============================================================================
// 沙子下落测试
// ============================================================================

TEST(FallingBlockBehaviorTest, SandIsRegisteredAsFallingBlock)
{
    // 验证沙子是下落方块
    ASSERT_NE(VanillaBlocks::SAND, nullptr);
    const Block* sandBlock = VanillaBlocks::SAND;
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(sandBlock);
    EXPECT_NE(fallingBlock, nullptr);
}

TEST(FallingBlockBehaviorTest, GravelIsRegisteredAsFallingBlock)
{
    // 验证砾石是下落方块
    ASSERT_NE(VanillaBlocks::GRAVEL, nullptr);
    const Block* gravelBlock = VanillaBlocks::GRAVEL;
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(gravelBlock);
    EXPECT_NE(fallingBlock, nullptr);
}

TEST(FallingBlockBehaviorTest, RedSandIsRegisteredAsFallingBlock)
{
    // 验证红沙是下落方块
    ASSERT_NE(VanillaBlocks::RED_SAND, nullptr);
    const Block* redSandBlock = VanillaBlocks::RED_SAND;
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(redSandBlock);
    EXPECT_NE(fallingBlock, nullptr);
}

// ============================================================================
// canFallThrough 行为变更测试
//
// MC 1.21.11 FallingBlock.isFree() 对齐后的 canFallThrough 逻辑：
// nullptr/air → true, FIRE tag → true, isLiquid → true, canBeReplaced → true
// 旧逻辑额外包含 !blocksMovement()（如火把等非阻挡移动方块），
// 这在 MC 原版中不存在，已移除。
// ============================================================================

TEST(FallingBlockCanFallThroughTest, Torch_NotPenetrableAfterFix)
{
    // 火把 blocksMovement=false，但不是空气、火焰、液体、可替换
    // 旧逻辑(!blocksMovement)：穿透 → 新逻辑：不穿透
    // 对齐 MC 1.21.11：火把不应被下落方块穿透
    if (VanillaBlocks::TORCH != nullptr) {
        const BlockState* torchState = &VanillaBlocks::TORCH->defaultState();
        ASSERT_NE(torchState, nullptr);
        EXPECT_FALSE(FallingBlock::canFallThrough(torchState));
    }
}

TEST(FallingBlockCanFallThroughTest, SnowLayer_ReturnsTrueBecauseReplaceable)
{
    // 雪层 canBeReplaced=true，应该可以穿透
    if (VanillaBlocks::SNOW != nullptr) {
        const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
        ASSERT_NE(snowState, nullptr);
        EXPECT_TRUE(FallingBlock::canFallThrough(snowState));
    }
}

TEST(FallingBlockCanFallThroughTest, LiquidBlocksArePenetrable)
{
    // 水/岩浆方块 isLiquid=true，应该可以穿透
    // 这在旧逻辑中可能通过 canBeReplaced 间接支持，
    // 新逻辑通过 isLiquid 明确支持
    ASSERT_NE(VanillaBlocks::WATER, nullptr);
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    EXPECT_TRUE(FallingBlock::canFallThrough(waterState));

    ASSERT_NE(VanillaBlocks::LAVA, nullptr);
    const BlockState* lavaState = &VanillaBlocks::LAVA->defaultState();
    EXPECT_TRUE(FallingBlock::canFallThrough(lavaState));
}

TEST(FallingBlockCanFallThroughTest, ConcretePowderIsFallingBlock)
{
    // 验证混凝土粉末是下落方块
    ASSERT_NE(VanillaBlocks::WHITE_CONCRETE_POWDER, nullptr);
    const Block* concretePowder = VanillaBlocks::WHITE_CONCRETE_POWDER;
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(concretePowder);
    EXPECT_NE(fallingBlock, nullptr);
}

TEST(FallingBlockCanFallThroughTest, ConcretePowderIsConcretePowderBlock)
{
    // 验证混凝土粉末注册为 ConcretePowderBlock
    ASSERT_NE(VanillaBlocks::WHITE_CONCRETE_POWDER, nullptr);
    const ConcretePowderBlock* cpb = dynamic_cast<const ConcretePowderBlock*>(VanillaBlocks::WHITE_CONCRETE_POWDER);
    EXPECT_NE(cpb, nullptr);
}

TEST(FallingBlockCanFallThroughTest, ConcreteBlockIsNotFallingBlock)
{
    // 验证混凝土不是下落方块
    ASSERT_NE(VanillaBlocks::WHITE_CONCRETE, nullptr);
    const FallingBlock* fallingBlock = dynamic_cast<const FallingBlock*>(VanillaBlocks::WHITE_CONCRETE);
    EXPECT_EQ(fallingBlock, nullptr);
}
