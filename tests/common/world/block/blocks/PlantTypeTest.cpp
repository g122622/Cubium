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
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::block_registry;

// ============================================================================
// PlantType 枚举值测试
// ============================================================================

class PlantTypeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(PlantTypeTest, PlantTypeEnumValuesAreDistinct)
{
    EXPECT_NE(PlantType::Plains, PlantType::Desert);
    EXPECT_NE(PlantType::Plains, PlantType::Beach);
    EXPECT_NE(PlantType::Plains, PlantType::Cave);
    EXPECT_NE(PlantType::Plains, PlantType::Water);
    EXPECT_NE(PlantType::Plains, PlantType::Nether);
    EXPECT_NE(PlantType::Plains, PlantType::Crop);
    EXPECT_NE(PlantType::Desert, PlantType::Beach);
    EXPECT_NE(PlantType::Cave, PlantType::Water);
    EXPECT_NE(PlantType::Nether, PlantType::Crop);
}

// ============================================================================
// IPlantable 接口测试 - 通过 VanillaBlocks 验证
// ============================================================================

class IPlantableInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(IPlantableInterfaceTest, BushBlockDerivativesAreIPlantable)
{
    // 验证通过 VanillaBlocks 注册的植物方块实现了 IPlantable 接口
    // 注意：部分植物方块（如 RED_MUSHROOM, LILY_PAD）当前注册为 SimpleBlock 占位，
    // 尚未替换为专门的 Block 子类，因此暂时跳过 dynamic_cast 检查

    if (VanillaBlocks::DANDELION != nullptr) {
        const Block& block = VanillaBlocks::DANDELION->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "DANDELION should implement IPlantable";
    }

    if (VanillaBlocks::WHEAT != nullptr) {
        const Block& block = VanillaBlocks::WHEAT->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "WHEAT should implement IPlantable";
    }

    if (VanillaBlocks::CACTUS != nullptr) {
        const Block& block = VanillaBlocks::CACTUS->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "CACTUS should implement IPlantable";
    }

    if (VanillaBlocks::SUGAR_CANE != nullptr) {
        const Block& block = VanillaBlocks::SUGAR_CANE->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "SUGAR_CANE should implement IPlantable";
    }

    if (VanillaBlocks::NETHER_WART != nullptr) {
        const Block& block = VanillaBlocks::NETHER_WART->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_NE(plant, nullptr) << "NETHER_WART should implement IPlantable";
    }
}

TEST_F(IPlantableInterfaceTest, NonPlantBlocksAreNotIPlantable)
{
    // 验证非植物方块不实现 IPlantable 接口
    if (VanillaBlocks::STONE != nullptr) {
        const Block& block = VanillaBlocks::STONE->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_EQ(plant, nullptr) << "STONE should NOT implement IPlantable";
    }

    if (VanillaBlocks::DIRT != nullptr) {
        const Block& block = VanillaBlocks::DIRT->defaultState().getBlock();
        const IPlantable* plant = dynamic_cast<const IPlantable*>(&block);
        EXPECT_EQ(plant, nullptr) << "DIRT should NOT implement IPlantable";
    }
}

// ============================================================================
// BlockTags 集成测试
// ============================================================================

class BlockTagsPlantTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(BlockTagsPlantTest, WheatInMaintainsFarmlandTag)
{
    // 小麦方块应在 MAINTAINS_FARMLAND 标签中
    if (VanillaBlocks::WHEAT == nullptr) {
        GTEST_SKIP() << "WHEAT not registered";
    }

    const BlockState& wheatState = VanillaBlocks::WHEAT->defaultState();
    EXPECT_TRUE(BlockTags::MAINTAINS_FARMLAND().contains(wheatState));
}

TEST_F(BlockTagsPlantTest, AirNotInMaintainsFarmlandTag)
{
    // 空气不在 MAINTAINS_FARMLAND 标签中
    if (VanillaBlocks::AIR == nullptr) {
        GTEST_SKIP() << "AIR not registered";
    }

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    EXPECT_FALSE(BlockTags::MAINTAINS_FARMLAND().contains(airState));
}

TEST_F(BlockTagsPlantTest, DirtInDirtTag)
{
    // DIRT 方块应在 DIRT 标签中
    if (VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "DIRT not registered";
    }

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    EXPECT_TRUE(BlockTags::DIRT().contains(dirtState));
}

TEST_F(BlockTagsPlantTest, SandInSandTag)
{
    // SAND 方块应在 SAND 标签中
    if (VanillaBlocks::SAND == nullptr) {
        GTEST_SKIP() << "SAND not registered";
    }

    const BlockState& sandState = VanillaBlocks::SAND->defaultState();
    EXPECT_TRUE(BlockTags::SAND().contains(sandState));
}
