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
 * @file EntityClimbableTest.cpp
 * @brief Entity::isOnLadder 可攀爬判定测试
 *
 * 覆盖两条判定来源：
 * - BlockTags::CLIMBABLE 标签（梯子/藤蔓/脚手架/垂泪藤/扭曲藤/洞穴藤蔓，含 _plant 变体）
 * - Block::isLadder 虚函数（LadderBlock/VineBlock/ScaffoldingBlock/打开的木活板门）
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"

#include <map>

using namespace mc;

namespace {

/**
 * @brief isOnLadder 测试用世界桩
 *
 * 支持在任意坐标放置方块状态（存副本避免悬空指针），其余 IWorld 方法走 BaseTestWorld 默认实现。
 */
class ClimbableTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    void placeBlock(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
};

} // namespace

class EntityClimbableTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }

    /// 构造一个位于 (0.5, 64.0, 0.5) 的实体并绑定测试世界
    void setUpEntity()
    {
        m_entity = std::make_unique<Entity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
        m_entity->setPosition(0.5f, 64.0f, 0.5f);
    }

    ClimbableTestWorld m_world;
    std::unique_ptr<Entity> m_entity;
};

TEST_F(EntityClimbableTest, NoBlockAtFeetIsNotClimbable)
{
    setUpEntity();
    EXPECT_FALSE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, LadderIsClimbable)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::LADDER, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::LADDER->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
    ASSERT_TRUE(m_entity->getLastClimbPos().has_value());
    EXPECT_EQ(m_entity->getLastClimbPos().value(), BlockPos(0, 64, 0));
}

TEST_F(EntityClimbableTest, WeepingVinesIsClimbableViaTag)
{
    // 垂泪藤未重写 Block::isLadder，仅靠 CLIMBABLE 标签触发攀爬
    setUpEntity();
    ASSERT_NE(VanillaBlocks::WEEPING_VINES, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::WEEPING_VINES->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, WeepingVinesPlantIsClimbableViaTag)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::WEEPING_VINES_PLANT, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::WEEPING_VINES_PLANT->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, TwistingVinesIsClimbableViaTag)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::TWISTING_VINES, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::TWISTING_VINES->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, TwistingVinesPlantIsClimbableViaTag)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::TWISTING_VINES_PLANT, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::TWISTING_VINES_PLANT->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, CaveVinesIsClimbableViaTag)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::CAVE_VINES, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::CAVE_VINES->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, CaveVinesPlantIsClimbableViaTag)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::CAVE_VINES_PLANT, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::CAVE_VINES_PLANT->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, ScaffoldingIsClimbable)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::SCAFFOLDING, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::SCAFFOLDING->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, VineIsClimbable)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::VINE, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::VINE->defaultState());

    EXPECT_TRUE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, StoneIsNotClimbable)
{
    setUpEntity();
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(m_entity->isOnLadder());
    EXPECT_FALSE(m_entity->getLastClimbPos().has_value());
}

TEST_F(EntityClimbableTest, ClosedTrapdoorIsNotClimbable)
{
    // 关闭的木活板门非可攀爬（TrapDoorBlock::isLadder 检查 OPEN），且不在 CLIMBABLE 标签内
    setUpEntity();
    ASSERT_NE(VanillaBlocks::OAK_TRAPDOOR, nullptr);
    m_world.placeBlock(BlockPos(0, 64, 0), &VanillaBlocks::OAK_TRAPDOOR->defaultState());

    EXPECT_FALSE(m_entity->isOnLadder());
}

TEST_F(EntityClimbableTest, NoWorldIsNotClimbable)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.setPosition(0.5f, 64.0f, 0.5f);

    EXPECT_FALSE(entity.isOnLadder());
}
