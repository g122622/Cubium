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

#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

/**
 * @brief 方块遮挡（canOcclude）与光照衰减测试
 *
 * 覆盖 vanilla `BlockBehaviour.Properties#noOcclusion` 语义在本项目的落地：
 * - canOcclude 是 `getLightBlock`（本项目 `Block::getOpacity`）、`isSolidRender` 与面遮挡检测的总开关，
 *   与材质无关；
 * - 铁栏杆（金属材质）、潜影盒（木质材质）等 vanilla noOcclusion 方块不再按材质被误判为完全遮光；
 * - 普通实心方块（石头、木板）保持 opacity=15 的完全遮挡。
 */
class BlockOcclusionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

TEST_F(BlockOcclusionTest, OpaqueCubeKeepsFullBlockLight)
{
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    ASSERT_NE(VanillaBlocks::OAK_PLANKS, nullptr);

    for (const Block* block : {VanillaBlocks::STONE, VanillaBlocks::OAK_PLANKS}) {
        const BlockState& state = block->defaultState();
        EXPECT_TRUE(block->canOcclude());
        EXPECT_TRUE(state.canOcclude());
        EXPECT_TRUE(state.isSolidRender());
        EXPECT_EQ(state.getOpacity(), 15);
        EXPECT_EQ(block->getShadeBrightness(state), 0.2f);
        EXPECT_FALSE(state.getFaceOcclusionShape(Direction::Up).isEmpty());
    }
}

TEST_F(BlockOcclusionTest, IronBarsDoNotOccludeLikeVanilla)
{
    ASSERT_NE(VanillaBlocks::IRON_BARS, nullptr);
    const Block& block = *VanillaBlocks::IRON_BARS;
    const BlockState& state = block.defaultState();

    // vanilla iron_bars 注册 noOcclusion()，遮挡形状为栅栏板形状（非完整立方体）→ 透传天空光，
    // lightBlock = 0（铁栏杆不衰减光）；noOcclusion 又使面遮挡检测整体失效。
    EXPECT_FALSE(block.canOcclude());
    EXPECT_FALSE(state.canOcclude());
    EXPECT_FALSE(state.isSolidRender());
    EXPECT_FALSE(state.isConditionallyFullOpaque());
    EXPECT_TRUE(state.propagatesSkylightDown());
    EXPECT_EQ(state.getOpacity(), 0);
    EXPECT_TRUE(state.getFaceOcclusionShape(Direction::Up).isEmpty());
    EXPECT_EQ(block.getShadeBrightness(state), 1.0f);
}

TEST_F(BlockOcclusionTest, ShulkerBoxAttenuatesLightByOne)
{
    ASSERT_NE(VanillaBlocks::SHULKER_BOX, nullptr);
    const Block& block = *VanillaBlocks::SHULKER_BOX;
    const BlockState& state = block.defaultState();

    // vanilla shulker_box 注册 noOcclusion()，遮挡形状为完整立方体 → propagatesSkylightDown=false，
    // lightBlock = 1（衰减 1 级），而非材质（木质）隐含的 15。
    EXPECT_FALSE(state.canOcclude());
    EXPECT_FALSE(state.isSolidRender());
    EXPECT_FALSE(state.propagatesSkylightDown());
    EXPECT_EQ(state.getOpacity(), 1);
}

TEST_F(BlockOcclusionTest, LeavesKeepSkylightPropagation)
{
    ASSERT_NE(VanillaBlocks::OAK_LEAVES, nullptr);
    const BlockState& state = VanillaBlocks::OAK_LEAVES->defaultState();

    // 树叶 noOcclusion 且 getShape 非完整 → 透传天空光，光衰减 1 级
    EXPECT_FALSE(state.canOcclude());
    EXPECT_TRUE(state.propagatesSkylightDown());
    EXPECT_EQ(state.getOpacity(), 1);
}

TEST_F(BlockOcclusionTest, DoorsAndTrapdoorsDoNotOcclude)
{
    const Block* doors[] = {VanillaBlocks::OAK_DOOR, VanillaBlocks::IRON_DOOR, VanillaBlocks::CRIMSON_DOOR};
    for (const Block* block : doors) {
        ASSERT_NE(block, nullptr);
        EXPECT_FALSE(block->canOcclude()) << block->blockLocation().toString();
    }

    const Block* trapdoors[] = {
        VanillaBlocks::OAK_TRAPDOOR, VanillaBlocks::IRON_TRAPDOOR, VanillaBlocks::WARPED_TRAPDOOR};
    for (const Block* block : trapdoors) {
        ASSERT_NE(block, nullptr);
        EXPECT_FALSE(block->canOcclude()) << block->blockLocation().toString();
    }
}

TEST_F(BlockOcclusionTest, IceAttenuatesLightByOne)
{
    // vanilla Blocks.ICE 注册 noOcclusion()：canOcclude=false → 非实体渲染；遮挡形状为完整立方体 →
    // propagatesSkylightDown=false → lightBlock=1（冰只衰减 1 级光，融化门槛 > 11-1=10）。
    for (const Block* block : {VanillaBlocks::ICE, VanillaBlocks::FROSTED_ICE}) {
        ASSERT_NE(block, nullptr);
        const BlockState& state = block->defaultState();
        EXPECT_FALSE(block->canOcclude()) << block->blockLocation().toString();
        EXPECT_FALSE(state.canOcclude()) << block->blockLocation().toString();
        EXPECT_FALSE(state.isSolidRender()) << block->blockLocation().toString();
        EXPECT_EQ(state.getOpacity(), 1) << block->blockLocation().toString();
    }
}

TEST_F(BlockOcclusionTest, GlassAttenuatesLightLikeVanilla)
{
    // vanilla glass 注册 noOcclusion() 且未重写 getLightBlock：遮挡形状为完整立方体 →
    // propagatesSkylightDown=false → lightBlock=1（衰减 1 级天空光）。
    ASSERT_NE(VanillaBlocks::GLASS, nullptr);
    const BlockState& state = VanillaBlocks::GLASS->defaultState();
    EXPECT_FALSE(state.canOcclude());
    EXPECT_FALSE(state.isSolidRender());
    EXPECT_EQ(state.getOpacity(), 1);
}