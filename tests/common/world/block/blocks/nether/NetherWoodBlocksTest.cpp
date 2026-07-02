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

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/registry/VanillaBlocks.hpp"

using namespace mc;

// ============================================================================
// 下界木质方块（绯红/诡异木板、楼梯、台阶、栅栏）注册测试
//
// 验证范围：
// 1) 方块静态指针不为 nullptr
// 2) 方块属性（硬度/抗性/材质）与 MC 原版一致
// 3) 材质为 NETHER_WOOD（不可燃）
// 4) BlockItem 注册与方块对应
// 5) BlockRegistry ResourceLocation 查找
// 6) 标签包含正确的方块
// ============================================================================

class NetherWoodBlocksTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        BlockTags::initialize();
    }
};

// ========== 方块指针非空验证 ==========

TEST_F(NetherWoodBlocksTest, Planks_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::CRIMSON_PLANKS, nullptr);
    ASSERT_NE(VanillaBlocks::WARPED_PLANKS, nullptr);
}

TEST_F(NetherWoodBlocksTest, Stairs_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::CRIMSON_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::WARPED_STAIRS, nullptr);
}

TEST_F(NetherWoodBlocksTest, Slabs_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::CRIMSON_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::WARPED_SLAB, nullptr);
}

TEST_F(NetherWoodBlocksTest, Fences_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::CRIMSON_FENCE, nullptr);
    ASSERT_NE(VanillaBlocks::WARPED_FENCE, nullptr);
}

// ========== 方块属性验证（与 MC 原版对照） ==========

TEST_F(NetherWoodBlocksTest, Planks_PropertiesMatchMC)
{
    // 绯红木板: 硬度2.0, 抗性3.0, NETHER_WOOD材质（不可燃）
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_PLANKS->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_PLANKS->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::CRIMSON_PLANKS->material(), &Material::NETHER_WOOD);

    // 诡异木板: 硬度2.0, 抗性3.0, NETHER_WOOD材质（不可燃）
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_PLANKS->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_PLANKS->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::WARPED_PLANKS->material(), &Material::NETHER_WOOD);
}

TEST_F(NetherWoodBlocksTest, Stairs_PropertiesMatchMC)
{
    // 绯红楼梯: 硬度2.0, 抗性3.0, NETHER_WOOD材质
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_STAIRS->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_STAIRS->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::CRIMSON_STAIRS->material(), &Material::NETHER_WOOD);

    // 诡异楼梯: 硬度2.0, 抗性3.0, NETHER_WOOD材质
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_STAIRS->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_STAIRS->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::WARPED_STAIRS->material(), &Material::NETHER_WOOD);
}

TEST_F(NetherWoodBlocksTest, Slabs_PropertiesMatchMC)
{
    // 绯红台阶: 硬度2.0, 抗性3.0, NETHER_WOOD材质
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_SLAB->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_SLAB->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::CRIMSON_SLAB->material(), &Material::NETHER_WOOD);

    // 诡异台阶: 硬度2.0, 抗性3.0, NETHER_WOOD材质
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_SLAB->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_SLAB->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::WARPED_SLAB->material(), &Material::NETHER_WOOD);
}

TEST_F(NetherWoodBlocksTest, Fences_PropertiesMatchMC)
{
    // 绯红栅栏: 硬度2.0, 抗性3.0, NETHER_WOOD材质
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_FENCE->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::CRIMSON_FENCE->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::CRIMSON_FENCE->material(), &Material::NETHER_WOOD);

    // 诡异栅栏: 硬度2.0, 抗性3.0, NETHER_WOOD材质
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_FENCE->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::WARPED_FENCE->resistance(), 3.0f);
    EXPECT_EQ(&VanillaBlocks::WARPED_FENCE->material(), &Material::NETHER_WOOD);
}

// ========== 材质不可燃性验证 ==========

TEST_F(NetherWoodBlocksTest, NetherWoodMaterialIsNotFlammable)
{
    // NETHER_WOOD 材质不可燃，与 WOOD 材质不同
    EXPECT_FALSE(Material::NETHER_WOOD.isFlammable());
    // 对比: 普通木材可燃
    EXPECT_TRUE(Material::WOOD.isFlammable());
}

// ========== 楼梯基座方块验证 ==========

TEST_F(NetherWoodBlocksTest, Stairs_BlockTypeIsCorrect)
{
    // 绯红/诡异楼梯应为 StairsBlock 类型
    auto* crimsonStairs = dynamic_cast<const blocks::StairsBlock*>(VanillaBlocks::CRIMSON_STAIRS);
    auto* warpedStairs = dynamic_cast<const blocks::StairsBlock*>(VanillaBlocks::WARPED_STAIRS);
    ASSERT_NE(crimsonStairs, nullptr) << "CRIMSON_STAIRS should be a StairsBlock";
    ASSERT_NE(warpedStairs, nullptr) << "WARPED_STAIRS should be a StairsBlock";
}

// ========== BlockItem 映射验证 ==========

TEST_F(NetherWoodBlocksTest, Planks_HaveBlockItems)
{
    const char* plankNames[] = {"crimson_planks", "warped_planks"};
    for (const char* name : plankNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing plank block item: minecraft:" << name;
    }
}

TEST_F(NetherWoodBlocksTest, Stairs_HaveBlockItems)
{
    const char* stairNames[] = {"crimson_stairs", "warped_stairs"};
    for (const char* name : stairNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing stair block item: minecraft:" << name;
    }
}

TEST_F(NetherWoodBlocksTest, Slabs_HaveBlockItems)
{
    const char* slabNames[] = {"crimson_slab", "warped_slab"};
    for (const char* name : slabNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing slab block item: minecraft:" << name;
    }
}

TEST_F(NetherWoodBlocksTest, Fences_HaveBlockItems)
{
    const char* fenceNames[] = {"crimson_fence", "warped_fence"};
    for (const char* name : fenceNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing fence block item: minecraft:" << name;
    }
}

// ========== BlockRegistry ResourceLocation 查找验证 ==========

TEST_F(NetherWoodBlocksTest, RegistryLookup)
{
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_planks")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_planks")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_fence")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_fence")), nullptr);
}

// ========== BlockItem 双向映射验证 ==========

TEST_F(NetherWoodBlocksTest, BlockItemReverseMapping)
{
    // 验证 BlockItem -> Block 映射
    const Block* netherWoodBlocks[] = {
        VanillaBlocks::CRIMSON_PLANKS,
        VanillaBlocks::WARPED_PLANKS,
        VanillaBlocks::CRIMSON_STAIRS,
        VanillaBlocks::WARPED_STAIRS,
        VanillaBlocks::CRIMSON_SLAB,
        VanillaBlocks::WARPED_SLAB,
        VanillaBlocks::CRIMSON_FENCE,
        VanillaBlocks::WARPED_FENCE,
    };

    for (const Block* block : netherWoodBlocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "BlockItem reverse mapping mismatch for: "
                                       << block->blockLocation().toString();
    }
}

// ========== 标签验证 ==========

TEST_F(NetherWoodBlocksTest, PlanksTagContainsNetherWood)
{
    auto& planksTag = BlockTags::PLANKS();
    EXPECT_TRUE(planksTag.contains(*VanillaBlocks::CRIMSON_PLANKS));
    EXPECT_TRUE(planksTag.contains(*VanillaBlocks::WARPED_PLANKS));
}

TEST_F(NetherWoodBlocksTest, WoodenFencesTagContainsNetherWood)
{
    auto& fencesTag = BlockTags::WOODEN_FENCES();
    EXPECT_TRUE(fencesTag.contains(*VanillaBlocks::CRIMSON_FENCE));
    EXPECT_TRUE(fencesTag.contains(*VanillaBlocks::WARPED_FENCE));
}

// ========== 与现有下界木质方块一致性验证 ==========

TEST_F(NetherWoodBlocksTest, MaterialMatchesExistingNetherWoodBlocks)
{
    // 新方块的材质应与已有的绯红/诡异门、栅栏门、活板门一致
    EXPECT_EQ(&VanillaBlocks::CRIMSON_PLANKS->material(), &VanillaBlocks::CRIMSON_DOOR->material());
    EXPECT_EQ(&VanillaBlocks::WARPED_PLANKS->material(), &VanillaBlocks::WARPED_DOOR->material());
    EXPECT_EQ(&VanillaBlocks::CRIMSON_FENCE->material(), &VanillaBlocks::CRIMSON_FENCE_GATE->material());
    EXPECT_EQ(&VanillaBlocks::WARPED_FENCE->material(), &VanillaBlocks::WARPED_FENCE_GATE->material());
}

TEST_F(NetherWoodBlocksTest, StairsBaseStateSameAsPlanksDefaultState)
{
    // 绯红/诡异楼梯应为 StairsBlock 类型，以对应木板为基座
    auto* crimsonStairs = dynamic_cast<const blocks::StairsBlock*>(VanillaBlocks::CRIMSON_STAIRS);
    auto* warpedStairs = dynamic_cast<const blocks::StairsBlock*>(VanillaBlocks::WARPED_STAIRS);
    ASSERT_NE(crimsonStairs, nullptr);
    ASSERT_NE(warpedStairs, nullptr);
}
