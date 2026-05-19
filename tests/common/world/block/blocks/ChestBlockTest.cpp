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

#include "world/block/blocks/ChestBlock.hpp"
#include "util/property/Properties.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/TrappedChestBlock.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== ChestBlock 测试 ==========

class ChestBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建箱子方块
        chest_ = std::make_unique<ChestBlock>(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    }

    std::unique_ptr<ChestBlock> chest_;
};

TEST_F(ChestBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(chest_, nullptr);
}

TEST_F(ChestBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(chest_->hasBlockEntity());
}

TEST_F(ChestBlockTest, GetBlockEntityType_ReturnsChest)
{
    EXPECT_EQ(chest_->getBlockEntityType(), BlockEntityType::Chest);
}

TEST_F(ChestBlockTest, CanProvidePower_ReturnsFalse)
{
    const auto& state = chest_->defaultState();
    EXPECT_FALSE(chest_->canProvidePower(state));
}

TEST_F(ChestBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = chest_->defaultState();
    const auto& shape = chest_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

// ========== TrappedChestBlock 测试 ==========

class TrappedChestBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建陷阱箱方块
        trappedChest_ =
            std::make_unique<TrappedChestBlock>(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    }

    std::unique_ptr<TrappedChestBlock> trappedChest_;
};

TEST_F(TrappedChestBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(trappedChest_, nullptr);
}

TEST_F(TrappedChestBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(trappedChest_->hasBlockEntity());
}

TEST_F(TrappedChestBlockTest, GetBlockEntityType_ReturnsTrappedChest)
{
    EXPECT_EQ(trappedChest_->getBlockEntityType(), BlockEntityType::TrappedChest);
}
