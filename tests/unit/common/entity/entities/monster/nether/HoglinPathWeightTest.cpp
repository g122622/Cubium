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
 * @file HoglinPathWeightTest.cpp
 * @brief 疣猪兽寻路权重（排斥物判定）单元测试
 *
 * 对齐 MC 1.21.11：
 *   - HoglinSpecificSensor.findNearestRepellent：以 blockPosition() 为中心，
 *     BlockPos.findClosestMatch(pos, 8, 4) 搜索最近 HOGLIN_REPELLENTS 方块；
 *   - HoglinAi.isPosNearNearestRepellent：NEAREST_REPELLENT.closerThan(candidate, 8.0)
 *     （三方块坐标差平方和 < 64）；
 *   - Hoglin.getWalkTargetValue：排斥范围内返回 -1.0，否则绯红菌岩上 10.0，其余 0.0。
 *
 * 覆盖点：
 *   - 排斥物位于疣猪兽垂直 4 格内（含 vertical 边界）时，疣猪兽脚下位置被判定为受排斥；
 *   - 排斥物在垂直反向 4 格内同理；
 *   - 候选位置到最近排斥物的距离取欧氏距离 8（垂直分量计入），而非水平 8 / 垂直 4 的方形范围；
 *   - 标签成员 potted_warped_fungus 与普通 warped_fungus 同等生效（此前的 TODO 声称标签不含盆栽成员）；
 *   - 无排斥物时，绯红菌岩上返回 10.0，其他位置返回 0.0。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/FlowerPotBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <unordered_map>

namespace mc {
namespace {

/// 块存储测试世界：记录任意方块状态，用于放置排斥物。
class HoglinTestWorld final : public mc::test::BaseTestWorld {
public:
    HoglinTestWorld() { VanillaBlocks::initialize(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = state;
        }
        return true;
    }

    void setBlock(const BlockPos& pos, const BlockState* state) { setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

class RepellentPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }

    static std::unique_ptr<HoglinEntity> _makeHoglin(HoglinTestWorld& world)
    {
        auto hoglin = std::make_unique<HoglinEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        hoglin->setWorld(&world);
        return hoglin;
    }
};

// 疣猪兽脚下（y=64）的排斥物（y=64）：直接受排斥。
TEST_F(RepellentPathWeightTest, RepellentAtSameLevelReturnsNegative)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(3, 64, 0), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), -1.0f);
}

// 排斥物位于疣猪兽上方 4 格（搜索范围垂直边界，含端点）：受排斥。
TEST_F(RepellentPathWeightTest, RepellentAtVerticalSearchBoundaryIsDetected)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(0, 68, 0), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), -1.0f);
}

// 排斥物位于疣猪兽上方 5 格：超出垂直搜索范围，不受排斥（脚下非绯红菌岩，返回 0.0）。
TEST_F(RepellentPathWeightTest, RepellentBeyondVerticalSearchRangeIgnored)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(0, 69, 0), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), 0.0f);
}

// 距离判定为欧氏 8（垂直计入）：排斥物在疣猪兽上方 4 格但水平距离 7 格，
// 距离平方 = 49 + 16 = 65 >= 64，候选位置不受排斥。
// 旧实现只要求"搜索范围内存在排斥物"即返回 -1，此用例可区分新旧语义。
TEST_F(RepellentPathWeightTest, DistanceUsesEuclideanEightIncludingVertical)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(7, 68, 0), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), 0.0f);
}

// 距离恰好小于 8：排斥物在疣猪兽上方 4 格、水平 6 格，距离平方 = 36 + 16 = 52 < 64。
TEST_F(RepellentPathWeightTest, RepellentWithinEuclideanEightReturnsNegative)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(6, 68, 0), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), -1.0f);
}

// 候选位置的距离以"疣猪兽搜索到的最近排斥物"为基准，而非以疣猪兽自身为基准：
// 疣猪兽 (0,64,0) 旁放排斥物 (0,64,3)；候选位置 (0,64,11) 距疣猪兽 11 格（超出欧氏 8），
// 但到最近排斥物恰好 8 格（平方 64），同样不受排斥。
TEST_F(RepellentPathWeightTest, CandidateDistanceMeasuredFromNearestRepellent)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(0, 64, 3), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    // 疣猪兽自身位置距排斥物 3 格：受排斥。
    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), -1.0f);
    // 候选位置恰距排斥物 8 格：不受排斥。
    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 11.5f), 0.0f);
    // 候选位置距排斥物 7 格：受排斥。
    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 10.5f), -1.0f);
}

// 标签成员 potted_warped_fungus（盆栽诡异菌）与普通诡异菌同等生效。
TEST_F(RepellentPathWeightTest, PottedWarpedFungusIsRepellent)
{
    HoglinTestWorld world;
    ASSERT_TRUE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "potted_warped_fungus")));
    world.setBlock(BlockPos(2, 64, 2), &block_registry::FlowerPotBlocks::POTTED_WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), -1.0f);
}

// 无排斥物：站在绯红菌岩上返回 10.0，其他位置返回 0.0。
TEST_F(RepellentPathWeightTest, CrimsonNyliumPreferredWithoutRepellent)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(0, 63, 0), &block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), 10.0f);
    EXPECT_FLOAT_EQ(hoglin->getPathWeight(10.5f, 64.0f, 10.5f), 0.0f);
}

// 排斥物优先于绯红菌岩偏好：站在绯红菌岩上且附近有排斥物时返回 -1.0。
TEST_F(RepellentPathWeightTest, RepellentOverridesCrimsonNyliumPreference)
{
    HoglinTestWorld world;
    world.setBlock(BlockPos(0, 63, 0), &block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState());
    world.setBlock(BlockPos(1, 64, 0), &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());
    auto hoglin = _makeHoglin(world);

    EXPECT_FLOAT_EQ(hoglin->getPathWeight(0.5f, 64.0f, 0.5f), -1.0f);
}

} // namespace
} // namespace mc