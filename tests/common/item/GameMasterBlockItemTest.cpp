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

#include "common/item/items/block/GameMasterBlockItem.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/GameMasterBlock.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/special/SpecialBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== GameMasterBlockItem 测试 ==========

// 测试用方块，用于测试 GameMasterBlockItem
class TestGameMasterBlock : public Block, public GameMasterBlock {
public:
    TestGameMasterBlock()
        : Block(BlockProperties(Material::ROCK).hardness(-1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool isGameMaster() const noexcept override { return true; }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

// 普通方块（非 GameMaster）
class TestNormalBlock : public Block {
public:
    TestNormalBlock()
        : Block(BlockProperties(Material::ROCK).hardness(1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

// ========== Block::isGameMaster 测试 ==========

TEST(GameMasterBlockTest, GameMasterBlockReturnsTrue)
{
    TestGameMasterBlock block;
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, NormalBlockReturnsFalse)
{
    TestNormalBlock block;
    EXPECT_FALSE(block.isGameMaster());
}

TEST(GameMasterBlockTest, GameMasterBlockIsGameMasterBlockInterface)
{
    TestGameMasterBlock block;
    auto* gameMasterBlock = dynamic_cast<const GameMasterBlock*>(&block);
    EXPECT_NE(gameMasterBlock, nullptr);
}

TEST(GameMasterBlockTest, NormalBlockIsNotGameMasterBlockInterface)
{
    TestNormalBlock block;
    auto* gameMasterBlock = dynamic_cast<const GameMasterBlock*>(&block);
    EXPECT_EQ(gameMasterBlock, nullptr);
}

// ========== 特殊方块 isGameMaster 测试 ==========

TEST(GameMasterBlockTest, CommandBlockIsGameMaster)
{
    CommandBlock block(BlockProperties(Material::ROCK).hardness(-1.0f));
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, StructureBlockIsGameMaster)
{
    StructureBlock block(BlockProperties(Material::ROCK).hardness(-1.0f).noLootTable());
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, JigsawBlockIsGameMaster)
{
    JigsawBlock block(BlockProperties(Material::ROCK).hardness(-1.0f).noLootTable());
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, BarrierBlockIsNotGameMaster)
{
    // BarrierBlock 不实现 GameMasterBlock 接口
    // 它通过 hardness=-1 防止破坏，而非通过权限检查
    BarrierBlock block(BlockProperties(Material::BARRIER).hardness(-1.0f).resistance(3600000.0f).noLootTable());
    EXPECT_FALSE(block.isGameMaster());
}

// ========== GameMasterBlockItem 测试 ==========

TEST(GameMasterBlockItemTest, InheritsFromBlockItem)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));

    // GameMasterBlockItem 应该是 BlockItem 的子类
    auto* blockItem = dynamic_cast<const BlockItem*>(&item);
    EXPECT_NE(blockItem, nullptr);
}

TEST(GameMasterBlockItemTest, ReturnsAssociatedBlock)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));

    EXPECT_EQ(&item.block(), &gameMasterBlock);
}

TEST(GameMasterBlockItemTest, MaxStackSizeFromProperties)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(1));

    EXPECT_EQ(item.maxStackSize(), 1);
}

// ========== GameMasterBlockItem 放置权限测试 ==========
//
// 注意：完整的放置权限测试需要完整的 BlockItemUseContext（包含 IWorld 和 Player），
// 这类集成测试在 test_block_item.cpp 和 BlockInteractionManagerTest.cpp 中进行。
// 此处仅测试 GameMasterBlockItem 的类层次结构和基本属性。
// 核心放置权限逻辑（getStateForPlacement 返回 nullptr 阻止放置）在集成测试中覆盖。

TEST(GameMasterBlockItemTest, GameMasterBlockItemCreation)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));

    // 验证基本属性
    EXPECT_EQ(&item.block(), &gameMasterBlock);
    EXPECT_EQ(item.maxStackSize(), 64);
}
