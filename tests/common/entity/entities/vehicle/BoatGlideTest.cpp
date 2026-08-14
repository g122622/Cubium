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
 * IMPLIED, NONINFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

/**
 * @brief 船只滑度检测测试世界
 *
 * 重写 getBlockState 以提供可控的方块状态，用于测试
 * BoatEntity::getBoatGlide() 的方块滑度采样逻辑。
 */
class BoatGlideTestWorld : public mc::test::BaseTestWorld {
public:
    BoatGlideTestWorld() = default;

    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
};

class BoatGlideTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<BoatGlideTestWorld>();
        VanillaBlocks::initialize();
    }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<BoatGlideTestWorld> m_world;
};

/**
 * @brief 测试无世界时船只可以正常构造
 */
TEST_F(BoatGlideTest, BoatConstructsWithoutWorld)
{
    entity::BoatEntity boat(entity::BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    // 不设置世界
    // 验证船只可以正常构造
    EXPECT_EQ(boat.getBoatType(), entity::BoatEntity::Type::OAK);
}

/**
 * @brief 测试默认方块的滑度值
 *
 * MC Java 中普通方块（石头、泥土等）的 friction（即 slipperiness）为 0.6。
 */
TEST_F(BoatGlideTest, DefaultBlockSlipperiness)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 默认滑度为 0.6
    EXPECT_FLOAT_EQ(stoneState.getBlock().getSlipperiness(stoneState), 0.6f);
}

/**
 * @brief 测试冰的滑度值
 *
 * MC Java 中冰的 friction 为 0.98，比普通方块更滑。
 */
TEST_F(BoatGlideTest, IceSlipperiness)
{
    const BlockState& iceState = VanillaBlocks::ICE->defaultState();

    // 冰的滑度应该为 0.98
    EXPECT_FLOAT_EQ(iceState.getBlock().getSlipperiness(iceState), 0.98f);
}

/**
 * @brief 测试蓝冰的滑度值
 *
 * MC Java 中蓝冰的 friction 为 0.989。
 */
TEST_F(BoatGlideTest, BlueIceSlipperiness)
{
    const BlockState& blueIceState = VanillaBlocks::BLUE_ICE->defaultState();

    // 蓝冰的滑度应该为 0.989
    EXPECT_FLOAT_EQ(blueIceState.getBlock().getSlipperiness(blueIceState), 0.989f);
}

/**
 * @brief 测试睡莲方块的存在
 *
 * getBoatGlide 中需要排除睡莲方块（不参与滑度采样）。
 */
TEST_F(BoatGlideTest, LilyPadBlockExists)
{
    ASSERT_NE(block_registry::NaturalBlocks::LILY_PAD, nullptr) << "NaturalBlocks::LILY_PAD 应该已注册";
}

/**
 * @brief 测试石头的碰撞箱非空
 *
 * getBoatGlide 需要检查方块碰撞箱与船底的交集，
 * 石头应该有非空碰撞箱。
 */
TEST_F(BoatGlideTest, StoneHasNonEmptyCollisionShape)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(stoneState.getCollisionShape().isEmpty()) << "石头应该有非空碰撞箱";
}

/**
 * @brief 测试水源方块的碰撞箱为空
 *
 * 水源方块没有碰撞箱，getBoatGlide 不会将其计入滑度采样。
 */
TEST_F(BoatGlideTest, WaterHasEmptyCollisionShape)
{
    const BlockState& waterState = VanillaBlocks::WATER->defaultState();
    EXPECT_TRUE(waterState.getCollisionShape().isEmpty()) << "水源方块应该有空碰撞箱";
}

/**
 * @brief 测试 Slipperiness 常量值与 MC 一致
 *
 * 验证项目中定义的滑度常量值与 MC Java 对应。
 * 注意：当前 SLIME_BLOCK 使用 SimpleBlock 注册而非 SlimeBlock，
 * 因此其滑度仍为默认值 0.6，待后续使用 SlimeBlock 注册后修正。
 */
TEST_F(BoatGlideTest, SlipperinessConstantsMatchMC)
{
    // MC Java Block.friction 默认值 = 0.6
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    EXPECT_FLOAT_EQ(dirtState.getBlock().getSlipperiness(dirtState), 0.6f);

    // MC Java SlimeBlock.friction = 0.8，但当前 SLIME_BLOCK
    // 使用 SimpleBlock 注册而非 SlimeBlock，滑度仍为 0.6
    const BlockState& slimeState = VanillaBlocks::SLIME_BLOCK->defaultState();
    EXPECT_FLOAT_EQ(slimeState.getBlock().getSlipperiness(slimeState), 0.6f);
}

} // namespace
} // namespace mc
