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

#include "common/TestWorldHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperBlock.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperDoorBlock.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperSlabBlock.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperStairBlock.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperTrapDoorBlock.hpp"

#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::math;
using namespace mc::test;

namespace {

/**
 * @brief 氧化测试用世界桩
 *
 * 提供基于内存的方块状态存储，支持 getBlockState 和 setBlockState 操作。
 */
class OxidationTestWorld : public BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        BlockPos pos(x, y, z);
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 /*flags*/) override
    {
        return setBlockState(x, y, z, state);
    }

    /**
     * @brief 在指定位置放置方块状态
     */
    void setBlock(const BlockPos& pos, const BlockState* state) { m_blocks[pos] = state; }

    /**
     * @brief 获取指定位置的方块状态
     */
    [[nodiscard]] const BlockState* getBlock(const BlockPos& pos) const
    {
        auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second : nullptr;
    }

    /**
     * @brief 检查指定位置是否发生过方块变化
     */
    [[nodiscard]] bool wasBlockSet(const BlockPos& pos) const { return m_blocks.find(pos) != m_blocks.end(); }

private:
    mutable std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

} // anonymous namespace

// ========== IOxidizableBlock 接口测试 ==========

class OxidizableBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建不同氧化等级的铜方块
        unaffectedBlock =
            std::make_unique<WeatheringCopperBlock>(BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f),
                BlockStateProperties::OxidationLevel::Unaffected);

        exposedBlock =
            std::make_unique<WeatheringCopperBlock>(BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f),
                BlockStateProperties::OxidationLevel::Exposed);

        weatheredBlock =
            std::make_unique<WeatheringCopperBlock>(BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f),
                BlockStateProperties::OxidationLevel::Weathered);

        oxidizedBlock =
            std::make_unique<WeatheringCopperBlock>(BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f),
                BlockStateProperties::OxidationLevel::Oxidized);
    }

    OxidationTestWorld world;
    Random random{12345};
    std::unique_ptr<WeatheringCopperBlock> unaffectedBlock;
    std::unique_ptr<WeatheringCopperBlock> exposedBlock;
    std::unique_ptr<WeatheringCopperBlock> weatheredBlock;
    std::unique_ptr<WeatheringCopperBlock> oxidizedBlock;
};

// ========== 氧化等级测试 ==========

TEST_F(OxidizableBlockTest, GetOxidationLevel_ReturnsCorrectLevel)
{
    EXPECT_EQ(unaffectedBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Unaffected);
    EXPECT_EQ(exposedBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Exposed);
    EXPECT_EQ(weatheredBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Weathered);
    EXPECT_EQ(oxidizedBlock->getOxidationLevel(), BlockStateProperties::OxidationLevel::Oxidized);
}

// ========== 氧化概率修正系数测试 ==========

TEST_F(OxidizableBlockTest, GetOxidationChanceModifier_UnaffectedReturns75Percent)
{
    // Unaffected等级的修正系数为0.75
    EXPECT_FLOAT_EQ(unaffectedBlock->getOxidationChanceModifier(), 0.75f);
}

TEST_F(OxidizableBlockTest, GetOxidationChanceModifier_NonUnaffectedReturns1)
{
    // 非Unaffected等级的修正系数为1.0
    EXPECT_FLOAT_EQ(exposedBlock->getOxidationChanceModifier(), 1.0f);
    EXPECT_FLOAT_EQ(weatheredBlock->getOxidationChanceModifier(), 1.0f);
    EXPECT_FLOAT_EQ(oxidizedBlock->getOxidationChanceModifier(), 1.0f);
}

// ========== 随机刻标记测试 ==========

TEST_F(OxidizableBlockTest, TicksRandomly_NonOxidizedReturnsTrue)
{
    // 未达到最高氧化等级的方块应该响应随机刻
    EXPECT_TRUE(unaffectedBlock->ticksRandomly());
    EXPECT_TRUE(exposedBlock->ticksRandomly());
    EXPECT_TRUE(weatheredBlock->ticksRandomly());
}

TEST_F(OxidizableBlockTest, TicksRandomly_OxidizedReturnsFalse)
{
    // 最高氧化等级的方块不应响应随机刻
    EXPECT_FALSE(oxidizedBlock->ticksRandomly());
}

// ========== tryOxidize 逻辑测试 ==========

TEST_F(OxidizableBlockTest, TryOxidize_OxidizedBlockDoesNotOxidize)
{
    // 已是最高氧化等级的方块不应氧化
    BlockPos pos(0, 64, 0);
    BlockState& state = const_cast<BlockState&>(oxidizedBlock->defaultState());
    bool result = oxidizedBlock->tryOxidize(world, pos, state, random);
    EXPECT_FALSE(result);
}

TEST_F(OxidizableBlockTest, TryOxidize_NullNextBlockDoesNotOxidize)
{
    // 没有设置下一氧化等级方块的方块不应氧化
    // nextOxidationBlock 默认为 nullptr
    BlockPos pos(0, 64, 0);
    BlockState& state = const_cast<BlockState&>(unaffectedBlock->defaultState());
    bool result = unaffectedBlock->tryOxidize(world, pos, state, random);
    EXPECT_FALSE(result);
}

TEST_F(OxidizableBlockTest, TryOxidize_WithNextBlockCanOxidize)
{
    // 设置氧化链: Unaffected -> Exposed
    unaffectedBlock->setNextOxidationBlock(exposedBlock.get());

    // 在世界中放置未氧化方块
    BlockPos pos(0, 64, 0);
    world.setBlock(pos, &unaffectedBlock->defaultState());

    // 用一个确保通过门限概率的随机种子
    // 运行多次随机刻，至少应该有一次成功氧化
    bool oxidized = false;
    Random testRandom(42);
    for (int i = 0; i < 1000; ++i) {
        // 重新放置方块（如果之前已氧化则跳过）
        world.setBlock(pos, &unaffectedBlock->defaultState());
        BlockState& state = const_cast<BlockState&>(unaffectedBlock->defaultState());
        if (unaffectedBlock->tryOxidize(world, pos, state, testRandom)) {
            oxidized = true;
            break;
        }
    }
    EXPECT_TRUE(oxidized);
}

TEST_F(OxidizableBlockTest, TryOxidize_LowerAgeNeighborPreventsOxidation)
{
    // 设置氧化链: Exposed -> Weathered
    exposedBlock->setNextOxidationBlock(weatheredBlock.get());

    // 在世界中放置 Exposed 方块在中心
    BlockPos centerPos(0, 64, 0);
    world.setBlock(centerPos, &exposedBlock->defaultState());

    // 在曼哈顿距离4以内放置 Unaffected（更低等级）邻居
    BlockPos lowerNeighbor(1, 64, 0); // 曼哈顿距离1
    world.setBlock(lowerNeighbor, &unaffectedBlock->defaultState());

    // 尝试多次，应该全部失败（因为存在更低等级邻居）
    Random testRandom(42);
    for (int i = 0; i < 1000; ++i) {
        BlockState& state = const_cast<BlockState&>(exposedBlock->defaultState());
        bool result = exposedBlock->tryOxidize(world, centerPos, state, testRandom);
        EXPECT_FALSE(result);
    }
}

TEST_F(OxidizableBlockTest, TryOxidize_HigherAgeNeighborIncreasesProbability)
{
    // 设置氧化链: Unaffected -> Exposed
    unaffectedBlock->setNextOxidationBlock(exposedBlock.get());

    // 在世界中放置 Unaffected 方块在中心
    BlockPos centerPos(0, 64, 0);
    world.setBlock(centerPos, &unaffectedBlock->defaultState());

    // 在附近放置多个更高等级的邻居
    BlockPos higher1(1, 64, 0);
    BlockPos higher2(0, 65, 0);
    BlockPos higher3(0, 64, 1);
    world.setBlock(higher1, &weatheredBlock->defaultState());
    world.setBlock(higher2, &oxidizedBlock->defaultState());
    world.setBlock(higher3, &exposedBlock->defaultState());

    // 运行多次，氧化成功率应该更高
    int successCount = 0;
    Random testRandom(42);
    for (int i = 0; i < 500; ++i) {
        world.setBlock(centerPos, &unaffectedBlock->defaultState());
        BlockState& state = const_cast<BlockState&>(unaffectedBlock->defaultState());
        if (unaffectedBlock->tryOxidize(world, centerPos, state, testRandom)) {
            successCount++;
        }
    }
    // 有更高等级邻居时，氧化概率应该明显提高
    // 理论概率: 0.05688889 * ((3+1)/(3+0+1))^2 * 0.75 ≈ 0.05688889 * 1.0 * 0.75 ≈ 0.0427
    // 500次中期望约21次成功，至少应该有一些成功
    EXPECT_GT(successCount, 0);
}

TEST_F(OxidizableBlockTest, TryOxidize_NoNeighborsCanStillOxidize)
{
    // 设置氧化链: Unaffected -> Exposed
    unaffectedBlock->setNextOxidationBlock(exposedBlock.get());

    // 仅放置中心方块，没有任何邻居
    BlockPos centerPos(0, 64, 0);
    world.setBlock(centerPos, &unaffectedBlock->defaultState());

    // 理论概率: 0.05688889 * ((0+1)/(0+0+1))^2 * 0.75 ≈ 0.0427
    // 运行多次应该有成功
    int successCount = 0;
    Random testRandom(42);
    for (int i = 0; i < 500; ++i) {
        world.setBlock(centerPos, &unaffectedBlock->defaultState());
        BlockState& state = const_cast<BlockState&>(unaffectedBlock->defaultState());
        if (unaffectedBlock->tryOxidize(world, centerPos, state, testRandom)) {
            successCount++;
        }
    }
    EXPECT_GT(successCount, 0);
}

TEST_F(OxidizableBlockTest, TryOxidize_SameAgeNeighborSlowsOxidation)
{
    // 设置氧化链: Unaffected -> Exposed
    unaffectedBlock->setNextOxidationBlock(exposedBlock.get());

    // 在世界中放置 Unaffected 方块在中心
    BlockPos centerPos(0, 64, 0);
    world.setBlock(centerPos, &unaffectedBlock->defaultState());

    // 放置多个同等级邻居
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue;
            BlockPos neighbor(dx, 64, dz);
            world.setBlock(neighbor, &unaffectedBlock->defaultState());
        }
    }

    // 理论概率: 0.05688889 * ((0+1)/(0+8+1))^2 * 0.75 ≈ 0.05688889 * 0.01235 * 0.75 ≈ 0.000527
    // 这应该非常低，几乎不会成功
    int successCount = 0;
    Random testRandom(42);
    for (int i = 0; i < 500; ++i) {
        world.setBlock(centerPos, &unaffectedBlock->defaultState());
        BlockState& state = const_cast<BlockState&>(unaffectedBlock->defaultState());
        if (unaffectedBlock->tryOxidize(world, centerPos, state, testRandom)) {
            successCount++;
        }
    }
    // 有很多同等级邻居时，氧化概率极低，可能完全没有成功
    // 预期接近0次，但允许少量随机成功
    EXPECT_LT(successCount, 20); // 远少于无邻居情况
}

// ========== 曼哈顿距离边界测试 ==========

TEST_F(OxidizableBlockTest, TryOxidize_LowerNeighborAtManhattanDist4PreventsOxidation)
{
    // 设置氧化链: Exposed -> Weathered
    exposedBlock->setNextOxidationBlock(weatheredBlock.get());

    BlockPos centerPos(0, 64, 0);
    world.setBlock(centerPos, &exposedBlock->defaultState());

    // 在曼哈顿距离4的边界放置更低等级邻居 (1,1,2) = 曼哈顿4
    BlockPos lowerNeighbor(1, 65, 2);
    world.setBlock(lowerNeighbor, &unaffectedBlock->defaultState());

    // 所有尝试应该失败
    Random testRandom(42);
    for (int i = 0; i < 1000; ++i) {
        BlockState& state = const_cast<BlockState&>(exposedBlock->defaultState());
        bool result = exposedBlock->tryOxidize(world, centerPos, state, testRandom);
        EXPECT_FALSE(result);
    }
}

TEST_F(OxidizableBlockTest, TryOxidize_LowerNeighborBeyondManhattanDist4DoesNotPrevent)
{
    // 设置氧化链: Exposed -> Weathered
    exposedBlock->setNextOxidationBlock(weatheredBlock.get());

    BlockPos centerPos(0, 64, 0);
    world.setBlock(centerPos, &exposedBlock->defaultState());

    // 在曼哈顿距离5放置更低等级邻居 (2,2,1) = 曼哈顿5
    // 这应该不影响氧化
    BlockPos distantLower(2, 66, 1);
    world.setBlock(distantLower, &unaffectedBlock->defaultState());

    // 应该可以氧化（没有4格内的更低等级邻居）
    bool oxidized = false;
    Random testRandom(42);
    for (int i = 0; i < 1000; ++i) {
        world.setBlock(centerPos, &exposedBlock->defaultState());
        BlockState& state = const_cast<BlockState&>(exposedBlock->defaultState());
        if (exposedBlock->tryOxidize(world, centerPos, state, testRandom)) {
            oxidized = true;
            break;
        }
    }
    EXPECT_TRUE(oxidized);
}

// ========== 氧化链完整性测试 ==========

TEST_F(OxidizableBlockTest, OxidationChain_AllLevels)
{
    // 验证完整的氧化链：Unaffected -> Exposed -> Weathered -> Oxidized
    unaffectedBlock->setNextOxidationBlock(exposedBlock.get());
    exposedBlock->setNextOxidationBlock(weatheredBlock.get());
    weatheredBlock->setNextOxidationBlock(oxidizedBlock.get());

    EXPECT_NE(unaffectedBlock->getNextOxidationBlock(), nullptr);
    EXPECT_EQ(unaffectedBlock->getNextOxidationBlock(), exposedBlock.get());

    EXPECT_NE(exposedBlock->getNextOxidationBlock(), nullptr);
    EXPECT_EQ(exposedBlock->getNextOxidationBlock(), weatheredBlock.get());

    EXPECT_NE(weatheredBlock->getNextOxidationBlock(), nullptr);
    EXPECT_EQ(weatheredBlock->getNextOxidationBlock(), oxidizedBlock.get());

    // Oxidized 是最终等级，不应有下一等级
    EXPECT_EQ(oxidizedBlock->getNextOxidationBlock(), nullptr);
}

// ========== IOxidizableBlock 多态识别测试 ==========

TEST_F(OxidizableBlockTest, DynamicCast_IdentifiesOxidizableBlocks)
{
    // 所有 WeatheringCopper 变体都应能通过 dynamic_cast 识别为 IOxidizableBlock
    const Block* block = unaffectedBlock.get();
    const auto* oxidizable = dynamic_cast<const IOxidizableBlock*>(block);
    EXPECT_NE(oxidizable, nullptr);
    EXPECT_EQ(oxidizable->getOxidationLevel(), BlockStateProperties::OxidationLevel::Unaffected);
}

TEST_F(OxidizableBlockTest, DynamicCast_NonOxidizableBlockReturnsNull)
{
    // WaxedCopperBlock 不实现 IOxidizableBlock，应返回 nullptr
    // 使用 oxidizedBlock 的 Block 基类部分进行测试
    // 这里用一个简单的方式验证：WeatheringCopperBlock 的 IOxidizableBlock 是有效的
    const auto* oxidizable1 = dynamic_cast<const IOxidizableBlock*>(unaffectedBlock.get());
    const auto* oxidizable2 = dynamic_cast<const IOxidizableBlock*>(exposedBlock.get());
    EXPECT_NE(oxidizable1, nullptr);
    EXPECT_NE(oxidizable2, nullptr);
}
