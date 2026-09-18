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

#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// BlockState::getBlockMutable() 测试
//
// 验证 getBlockMutable() 返回的引用与 getBlock() 指向同一个 Block 对象，
// 且返回类型为非 const 的 Block&。
// ============================================================================

class BlockStateGetBlockMutableTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockStateGetBlockMutableTest, GetBlockMutableReturnsSameBlockAsGetBlock)
{
    // 获取石头的默认状态
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    // getBlock() 返回 const Block&，getBlockMutable() 返回 Block&
    const Block& constRef = stoneState->getBlock();
    Block& mutableRef = stoneState->getBlockMutable();

    // 两个引用必须指向同一个 Block 对象
    EXPECT_EQ(&constRef, &mutableRef) << "getBlockMutable() should return a reference to the same Block as getBlock()";
}

TEST_F(BlockStateGetBlockMutableTest, GetBlockMutableReturnsNonNullForRegisteredBlock)
{
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    ASSERT_NE(dirtState, nullptr);

    Block& block = dirtState->getBlockMutable();
    EXPECT_NE(&block, nullptr);
}

TEST_F(BlockStateGetBlockMutableTest, GetBlockMutableAllowsCallingNonConstMethods)
{
    const BlockState* oakLogState = &VanillaBlocks::OAK_LOG->defaultState();
    ASSERT_NE(oakLogState, nullptr);

    // getBlockMutable() 返回 Block&，允许调用非 const 方法
    Block& block = oakLogState->getBlockMutable();

    // 验证可以获取 blockLocation()（const 方法，但通过非 const 引用也可以调用）
    const auto& location = block.blockLocation();
    EXPECT_EQ(location.toString(), "minecraft:oak_log");
}

TEST_F(BlockStateGetBlockMutableTest, GetBlockMutableConsistentAcrossMultipleCalls)
{
    const BlockState* state = &VanillaBlocks::GRASS_BLOCK->defaultState();
    ASSERT_NE(state, nullptr);

    // 多次调用应返回同一个引用
    Block& ref1 = state->getBlockMutable();
    Block& ref2 = state->getBlockMutable();

    EXPECT_EQ(&ref1, &ref2) << "Multiple calls to getBlockMutable() should return the same reference";
}

TEST_F(BlockStateGetBlockMutableTest, GetBlockMutableWorksForDifferentBlockTypes)
{
    // 测试多种不同方块类型
    const Block* blocks[] = {
        VanillaBlocks::STONE,
        VanillaBlocks::OAK_PLANKS,
        VanillaBlocks::GLASS,
        VanillaBlocks::WATER,
    };

    for (const Block* block : blocks) {
        if (block == nullptr) continue;
        const BlockState* state = &block->defaultState();
        const Block& constBlock = state->getBlock();
        Block& mutableBlock = state->getBlockMutable();
        EXPECT_EQ(&constBlock, &mutableBlock) << "getBlockMutable() should reference the same Block as getBlock() for "
                                              << constBlock.blockLocation().toString();
    }
}

TEST_F(BlockStateGetBlockMutableTest, GetBlockMutableWorksWithStateWithProperties)
{
    // 获取带属性的方块状态
    const BlockState* composterState = &VanillaBlocks::COMPOSTER->defaultState();
    if (composterState == nullptr) return;

    // Level 0 的状态
    Block& block0 = composterState->getBlockMutable();

    // Level 5 的状态
    const BlockState& level5State = composterState->with(BlockStateProperties::LEVEL_0_8(), 5);
    Block& block5 = level5State.getBlockMutable();

    // 不同状态应该指向同一个 Block 对象（因为它们属于同一个方块）
    EXPECT_EQ(&block0, &block5) << "Different states of the same block should return the same Block reference";
}
