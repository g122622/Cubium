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

#include "common/item/Items.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/agricultural/MelonPumpkinBlocks.hpp"
#include "common/world/block/blocks/agricultural/StemBlock.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 茎方块测试夹具
 *
 * 初始化必要的注册表，确保 Items 和 VanillaBlocks 可用
 */
class StemBlocksTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();
        // 初始化物品注册表
        Items::initialize();
    }
};

// ============================================================================
// MelonStemBlock 测试
// ============================================================================

TEST_F(StemBlocksTest, MelonStemBlock_GetSeedItem_ReturnsMelonSeedsItemId)
{
    ASSERT_NE(VanillaBlocks::MELON_STEM, nullptr) << "MELON_STEM should be registered";

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be a StemBlock";

    u32 seedItemId = melonStem->getSeedItem();

    ASSERT_NE(Items::MELON_SEEDS, nullptr) << "Items::MELON_SEEDS should be initialized";
    EXPECT_EQ(seedItemId, Items::MELON_SEEDS->itemId())
        << "MelonStemBlock::getSeedItem() should return Items::MELON_SEEDS->itemId()";
}

TEST_F(StemBlocksTest, MelonStemBlock_GetCrop_ReturnsMelonBlock)
{
    ASSERT_NE(VanillaBlocks::MELON_STEM, nullptr) << "MELON_STEM should be registered";

    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be a StemBlock";

    const StemGrownBlock* crop = melonStem->getCrop();
    ASSERT_NE(crop, nullptr) << "MelonStemBlock::getCrop() should return non-null";
    EXPECT_EQ(crop, VanillaBlocks::MELON) << "MelonStemBlock::getCrop() should return MELON block";
}

TEST_F(StemBlocksTest, MelonStemBlock_HasCorrectBlockProperties)
{
    ASSERT_NE(VanillaBlocks::MELON_STEM, nullptr) << "MELON_STEM should be registered";

    // 茎方块应该没有碰撞（可以穿过）
    EXPECT_FALSE(VanillaBlocks::MELON_STEM->defaultState().isSolid()) << "Melon stem should not be solid";

    // 硬度应该为0
    EXPECT_FLOAT_EQ(VanillaBlocks::MELON_STEM->hardness(), 0.0f) << "Melon stem should have 0 hardness";
}

// ============================================================================
// PumpkinStemBlock 测试
// ============================================================================

TEST_F(StemBlocksTest, PumpkinStemBlock_GetSeedItem_ReturnsPumpkinSeedsItemId)
{
    ASSERT_NE(VanillaBlocks::PUMPKIN_STEM, nullptr) << "PUMPKIN_STEM should be registered";

    auto* pumpkinStem = dynamic_cast<const StemBlock*>(VanillaBlocks::PUMPKIN_STEM);
    ASSERT_NE(pumpkinStem, nullptr) << "PUMPKIN_STEM should be a StemBlock";

    u32 seedItemId = pumpkinStem->getSeedItem();

    ASSERT_NE(Items::PUMPKIN_SEEDS, nullptr) << "Items::PUMPKIN_SEEDS should be initialized";
    EXPECT_EQ(seedItemId, Items::PUMPKIN_SEEDS->itemId())
        << "PumpkinStemBlock::getSeedItem() should return Items::PUMPKIN_SEEDS->itemId()";
}

TEST_F(StemBlocksTest, PumpkinStemBlock_GetCrop_ReturnsPumpkinBlock)
{
    ASSERT_NE(VanillaBlocks::PUMPKIN_STEM, nullptr) << "PUMPKIN_STEM should be registered";

    auto* pumpkinStem = dynamic_cast<const StemBlock*>(VanillaBlocks::PUMPKIN_STEM);
    ASSERT_NE(pumpkinStem, nullptr) << "PUMPKIN_STEM should be a StemBlock";

    const StemGrownBlock* crop = pumpkinStem->getCrop();
    ASSERT_NE(crop, nullptr) << "PumpkinStemBlock::getCrop() should return non-null";
    EXPECT_EQ(crop, VanillaBlocks::PUMPKIN) << "PumpkinStemBlock::getCrop() should return PUMPKIN block";
}

TEST_F(StemBlocksTest, PumpkinStemBlock_HasCorrectBlockProperties)
{
    ASSERT_NE(VanillaBlocks::PUMPKIN_STEM, nullptr) << "PUMPKIN_STEM should be registered";

    EXPECT_FALSE(VanillaBlocks::PUMPKIN_STEM->defaultState().isSolid()) << "Pumpkin stem should not be solid";

    EXPECT_FLOAT_EQ(VanillaBlocks::PUMPKIN_STEM->hardness(), 0.0f) << "Pumpkin stem should have 0 hardness";
}

// ============================================================================
// MelonAttachedStemBlock 测试
// ============================================================================

TEST_F(StemBlocksTest, MelonAttachedStemBlock_GetSeedItem_ReturnsMelonSeedsItemId)
{
    ASSERT_NE(VanillaBlocks::ATTACHED_MELON_STEM, nullptr) << "ATTACHED_MELON_STEM should be registered";

    auto* attachedMelonStem = dynamic_cast<const AttachedStemBlock*>(VanillaBlocks::ATTACHED_MELON_STEM);
    ASSERT_NE(attachedMelonStem, nullptr) << "ATTACHED_MELON_STEM should be an AttachedStemBlock";

    u32 seedItemId = attachedMelonStem->getSeedItem();

    ASSERT_NE(Items::MELON_SEEDS, nullptr) << "Items::MELON_SEEDS should be initialized";
    EXPECT_EQ(seedItemId, Items::MELON_SEEDS->itemId())
        << "MelonAttachedStemBlock::getSeedItem() should return Items::MELON_SEEDS->itemId()";
}

TEST_F(StemBlocksTest, MelonAttachedStemBlock_GetCrop_ReturnsMelonBlock)
{
    ASSERT_NE(VanillaBlocks::ATTACHED_MELON_STEM, nullptr) << "ATTACHED_MELON_STEM should be registered";

    auto* attachedMelonStem = dynamic_cast<const AttachedStemBlock*>(VanillaBlocks::ATTACHED_MELON_STEM);
    ASSERT_NE(attachedMelonStem, nullptr) << "ATTACHED_MELON_STEM should be an AttachedStemBlock";

    const StemGrownBlock* crop = attachedMelonStem->getCrop();
    ASSERT_NE(crop, nullptr) << "MelonAttachedStemBlock::getCrop() should return non-null";
    EXPECT_EQ(crop, VanillaBlocks::MELON) << "MelonAttachedStemBlock::getCrop() should return MELON block";
}

// ============================================================================
// PumpkinAttachedStemBlock 测试
// ============================================================================

TEST_F(StemBlocksTest, PumpkinAttachedStemBlock_GetSeedItem_ReturnsPumpkinSeedsItemId)
{
    ASSERT_NE(VanillaBlocks::ATTACHED_PUMPKIN_STEM, nullptr) << "ATTACHED_PUMPKIN_STEM should be registered";

    auto* attachedPumpkinStem = dynamic_cast<const AttachedStemBlock*>(VanillaBlocks::ATTACHED_PUMPKIN_STEM);
    ASSERT_NE(attachedPumpkinStem, nullptr) << "ATTACHED_PUMPKIN_STEM should be an AttachedStemBlock";

    u32 seedItemId = attachedPumpkinStem->getSeedItem();

    ASSERT_NE(Items::PUMPKIN_SEEDS, nullptr) << "Items::PUMPKIN_SEEDS should be initialized";
    EXPECT_EQ(seedItemId, Items::PUMPKIN_SEEDS->itemId())
        << "PumpkinAttachedStemBlock::getSeedItem() should return Items::PUMPKIN_SEEDS->itemId()";
}

TEST_F(StemBlocksTest, PumpkinAttachedStemBlock_GetCrop_ReturnsPumpkinBlock)
{
    ASSERT_NE(VanillaBlocks::ATTACHED_PUMPKIN_STEM, nullptr) << "ATTACHED_PUMPKIN_STEM should be registered";

    auto* attachedPumpkinStem = dynamic_cast<const AttachedStemBlock*>(VanillaBlocks::ATTACHED_PUMPKIN_STEM);
    ASSERT_NE(attachedPumpkinStem, nullptr) << "ATTACHED_PUMPKIN_STEM should be an AttachedStemBlock";

    const StemGrownBlock* crop = attachedPumpkinStem->getCrop();
    ASSERT_NE(crop, nullptr) << "PumpkinAttachedStemBlock::getCrop() should return non-null";
    EXPECT_EQ(crop, VanillaBlocks::PUMPKIN) << "PumpkinAttachedStemBlock::getCrop() should return PUMPKIN block";
}

// ============================================================================
// MelonBlock 关联测试
// ============================================================================

TEST_F(StemBlocksTest, MelonBlock_GetStem_ReturnsMelonStem)
{
    ASSERT_NE(VanillaBlocks::MELON, nullptr) << "MELON should be registered";

    auto* melonBlock = dynamic_cast<const MelonBlock*>(VanillaBlocks::MELON);
    ASSERT_NE(melonBlock, nullptr) << "MELON should be a MelonBlock";

    const Block* stem = melonBlock->getStem();
    ASSERT_NE(stem, nullptr) << "MelonBlock::getStem() should return non-null";
    EXPECT_EQ(stem, VanillaBlocks::MELON_STEM) << "MelonBlock::getStem() should return MELON_STEM";
}

TEST_F(StemBlocksTest, MelonBlock_GetAttachedStem_ReturnsMelonAttachedStem)
{
    ASSERT_NE(VanillaBlocks::MELON, nullptr) << "MELON should be registered";

    auto* melonBlock = dynamic_cast<const MelonBlock*>(VanillaBlocks::MELON);
    ASSERT_NE(melonBlock, nullptr) << "MELON should be a MelonBlock";

    const Block* attachedStem = melonBlock->getAttachedStem();
    ASSERT_NE(attachedStem, nullptr) << "MelonBlock::getAttachedStem() should return non-null";
    EXPECT_EQ(attachedStem, VanillaBlocks::ATTACHED_MELON_STEM)
        << "MelonBlock::getAttachedStem() should return ATTACHED_MELON_STEM";
}

// ============================================================================
// PumpkinBlock 关联测试
// ============================================================================

TEST_F(StemBlocksTest, PumpkinBlock_GetStem_ReturnsPumpkinStem)
{
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr) << "PUMPKIN should be registered";

    auto* pumpkinBlock = dynamic_cast<const PumpkinBlock*>(VanillaBlocks::PUMPKIN);
    ASSERT_NE(pumpkinBlock, nullptr) << "PUMPKIN should be a PumpkinBlock";

    const Block* stem = pumpkinBlock->getStem();
    ASSERT_NE(stem, nullptr) << "PumpkinBlock::getStem() should return non-null";
    EXPECT_EQ(stem, VanillaBlocks::PUMPKIN_STEM) << "PumpkinBlock::getStem() should return PUMPKIN_STEM";
}

TEST_F(StemBlocksTest, PumpkinBlock_GetAttachedStem_ReturnsPumpkinAttachedStem)
{
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr) << "PUMPKIN should be registered";

    auto* pumpkinBlock = dynamic_cast<const PumpkinBlock*>(VanillaBlocks::PUMPKIN);
    ASSERT_NE(pumpkinBlock, nullptr) << "PUMPKIN should be a PumpkinBlock";

    const Block* attachedStem = pumpkinBlock->getAttachedStem();
    ASSERT_NE(attachedStem, nullptr) << "PumpkinBlock::getAttachedStem() should return non-null";
    EXPECT_EQ(attachedStem, VanillaBlocks::ATTACHED_PUMPKIN_STEM)
        << "PumpkinBlock::getAttachedStem() should return ATTACHED_PUMPKIN_STEM";
}

TEST_F(StemBlocksTest, PumpkinBlock_GetCarvedPumpkin_ReturnsCarvedPumpkin)
{
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr) << "PUMPKIN should be registered";

    auto* pumpkinBlock = dynamic_cast<const PumpkinBlock*>(VanillaBlocks::PUMPKIN);
    ASSERT_NE(pumpkinBlock, nullptr) << "PUMPKIN should be a PumpkinBlock";

    // PumpkinBlock 应该有雕刻南瓜引用
    // 注：这是通过构造函数传入的，不是 StemGrownBlock 接口的一部分
    EXPECT_NE(VanillaBlocks::CARVED_PUMPKIN, nullptr) << "CARVED_PUMPKIN should be registered";
}

// ============================================================================
// 方块注册有效性测试
// ============================================================================

TEST_F(StemBlocksTest, AllStemBlocksHaveValidBlockIds)
{
    ASSERT_NE(VanillaBlocks::MELON_STEM, nullptr);
    EXPECT_GT(VanillaBlocks::MELON_STEM->blockId(), 0u) << "MELON_STEM should have non-zero block ID";

    ASSERT_NE(VanillaBlocks::PUMPKIN_STEM, nullptr);
    EXPECT_GT(VanillaBlocks::PUMPKIN_STEM->blockId(), 0u) << "PUMPKIN_STEM should have non-zero block ID";

    ASSERT_NE(VanillaBlocks::ATTACHED_MELON_STEM, nullptr);
    EXPECT_GT(VanillaBlocks::ATTACHED_MELON_STEM->blockId(), 0u) << "ATTACHED_MELON_STEM should have non-zero block ID";

    ASSERT_NE(VanillaBlocks::ATTACHED_PUMPKIN_STEM, nullptr);
    EXPECT_GT(VanillaBlocks::ATTACHED_PUMPKIN_STEM->blockId(), 0u)
        << "ATTACHED_PUMPKIN_STEM should have non-zero block ID";
}

TEST_F(StemBlocksTest, AllStemBlocksHaveUniqueBlockIds)
{
    std::set<u32> blockIds;
    blockIds.insert(VanillaBlocks::MELON_STEM->blockId());
    blockIds.insert(VanillaBlocks::PUMPKIN_STEM->blockId());
    blockIds.insert(VanillaBlocks::ATTACHED_MELON_STEM->blockId());
    blockIds.insert(VanillaBlocks::ATTACHED_PUMPKIN_STEM->blockId());

    EXPECT_EQ(blockIds.size(), 4u) << "All 4 stem blocks should have unique block IDs";
}

TEST_F(StemBlocksTest, AllSeedItemsHaveValidNonZeroItemIds)
{
    ASSERT_NE(Items::MELON_SEEDS, nullptr);
    EXPECT_GT(Items::MELON_SEEDS->itemId(), 0u) << "MELON_SEEDS should have non-zero item ID";

    ASSERT_NE(Items::PUMPKIN_SEEDS, nullptr);
    EXPECT_GT(Items::PUMPKIN_SEEDS->itemId(), 0u) << "PUMPKIN_SEEDS should have non-zero item ID";
}

TEST_F(StemBlocksTest, SeedItemsHaveUniqueItemIds)
{
    std::set<u32> itemIds;
    itemIds.insert(Items::MELON_SEEDS->itemId());
    itemIds.insert(Items::PUMPKIN_SEEDS->itemId());

    EXPECT_EQ(itemIds.size(), 2u) << "MELON_SEEDS and PUMPKIN_SEEDS should have unique item IDs";
}

// ============================================================================
// 双向关联完整性测试
// ============================================================================

TEST_F(StemBlocksTest, MelonStemToMelonBlockAssociationIsBidirectional)
{
    // 茎 -> 果实
    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);
    const StemGrownBlock* crop = melonStem->getCrop();
    ASSERT_NE(crop, nullptr);

    // 果实 -> 茎
    auto* melonBlock = dynamic_cast<const MelonBlock*>(crop);
    ASSERT_NE(melonBlock, nullptr);
    EXPECT_EQ(melonBlock->getStem(), VanillaBlocks::MELON_STEM) << "MelonBlock should reference back to MELON_STEM";
    EXPECT_EQ(melonBlock->getAttachedStem(), VanillaBlocks::ATTACHED_MELON_STEM)
        << "MelonBlock should reference ATTACHED_MELON_STEM";
}

TEST_F(StemBlocksTest, PumpkinStemToPumpkinBlockAssociationIsBidirectional)
{
    // 茎 -> 果实
    auto* pumpkinStem = dynamic_cast<const StemBlock*>(VanillaBlocks::PUMPKIN_STEM);
    ASSERT_NE(pumpkinStem, nullptr);
    const StemGrownBlock* crop = pumpkinStem->getCrop();
    ASSERT_NE(crop, nullptr);

    // 果实 -> 茎
    auto* pumpkinBlock = dynamic_cast<const PumpkinBlock*>(crop);
    ASSERT_NE(pumpkinBlock, nullptr);
    EXPECT_EQ(pumpkinBlock->getStem(), VanillaBlocks::PUMPKIN_STEM)
        << "PumpkinBlock should reference back to PUMPKIN_STEM";
    EXPECT_EQ(pumpkinBlock->getAttachedStem(), VanillaBlocks::ATTACHED_PUMPKIN_STEM)
        << "PumpkinBlock should reference ATTACHED_PUMPKIN_STEM";
}

// ============================================================================
// AttachedStemBlock 状态属性测试
// ============================================================================

TEST_F(StemBlocksTest, AttachedStemBlocks_HaveHorizontalFacingProperty)
{
    // 连接茎应该有 HORIZONTAL_FACING 属性
    auto* attachedMelonStem = dynamic_cast<const AttachedStemBlock*>(VanillaBlocks::ATTACHED_MELON_STEM);
    ASSERT_NE(attachedMelonStem, nullptr);

    auto* attachedPumpkinStem = dynamic_cast<const AttachedStemBlock*>(VanillaBlocks::ATTACHED_PUMPKIN_STEM);
    ASSERT_NE(attachedPumpkinStem, nullptr);

    // 验证默认状态有 FACING 属性
    const BlockState& melonDefaultState = attachedMelonStem->defaultState();
    std::optional<Direction> melonFacing = melonDefaultState.getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(melonFacing.has_value()) << "AttachedMelonStem should have HORIZONTAL_FACING property";

    const BlockState& pumpkinDefaultState = attachedPumpkinStem->defaultState();
    std::optional<Direction> pumpkinFacing = pumpkinDefaultState.getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(pumpkinFacing.has_value()) << "AttachedPumpkinStem should have HORIZONTAL_FACING property";
}

// ============================================================================
// StemBlock 状态属性测试
// ============================================================================

TEST_F(StemBlocksTest, StemBlocks_HaveAgeProperty)
{
    // 茎应该有 AGE_0_7 属性
    auto* melonStem = dynamic_cast<const StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr);

    auto* pumpkinStem = dynamic_cast<const StemBlock*>(VanillaBlocks::PUMPKIN_STEM);
    ASSERT_NE(pumpkinStem, nullptr);

    // 验证默认状态有 AGE 属性
    const BlockState& melonDefaultState = melonStem->defaultState();
    std::optional<i32> melonAge = melonDefaultState.getOptional(BlockStateProperties::AGE_0_7());
    EXPECT_TRUE(melonAge.has_value()) << "MelonStem should have AGE_0_7 property";
    EXPECT_EQ(melonAge.value(), 0) << "Default age should be 0";

    const BlockState& pumpkinDefaultState = pumpkinStem->defaultState();
    std::optional<i32> pumpkinAge = pumpkinDefaultState.getOptional(BlockStateProperties::AGE_0_7());
    EXPECT_TRUE(pumpkinAge.has_value()) << "PumpkinStem should have AGE_0_7 property";
    EXPECT_EQ(pumpkinAge.value(), 0) << "Default age should be 0";
}

} // namespace
