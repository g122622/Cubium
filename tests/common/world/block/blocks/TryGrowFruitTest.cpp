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
#include "common/item/Items.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/agricultural/MelonPumpkinBlocks.hpp"
#include "common/world/block/blocks/agricultural/StemBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

// ============================================================================
// 测试用 IWorld 模拟，支持方块读写、光照和随机数控制
//
// 继承 BaseTestWorld，覆写方块存取和光照方法，以支持 tryGrowFruit 测试。
// ============================================================================

class TryGrowFruitTestWorld final : public mc::test::BaseTestWorld {
public:
    using BaseTestWorld::getBlockState;
    using BaseTestWorld::setBlockState;

    // ========== 方块存取 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(key(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        // 空位置返回 AIR 方块状态，而非 nullptr
        // MC 的 tryGrowFruit 检查 fruitState == nullptr || !fruitState->isAir()
        // 如果返回 nullptr，则条件为 true，会跳过该方向
        return VanillaBlocks::AIR != nullptr ? &VanillaBlocks::AIR->defaultState() : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const auto k = key(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(k);
        } else {
            m_blocks[k] = state;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void clearBlockAt(const BlockPos& pos) { m_blocks.erase(key(pos.x, pos.y, pos.z)); }

    // ========== 光照 ==========

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    // ========== 种子控制 ==========

    void setSeed(u64 seed) { m_seed = seed; }
    [[nodiscard]] u64 seed() const override { return m_seed; }

private:
    static i64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0xFFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 24) |
            ((static_cast<i64>(z) & 0xFFFFFF) << 36);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    u64 m_seed = 42;
};

} // namespace

// ============================================================================
// StemBlock::tryGrowFruit 测试
// ============================================================================

class TryGrowFruitTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

/**
 * @brief 测试：当所有方向都可用时，tryGrowFruit 能成功放置果实
 *
 * 在茎的四个水平方向都有空气和耕地，果实应能成功放置。
 * 通过 randomTick（光照>=9，概率=1）间接触发 tryGrowFruit。
 */
TEST_F(TryGrowFruitTest, AllDirectionsAvailable_FruitIsPlaced)
{
    auto* melonStem = dynamic_cast<StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be registered";

    TryGrowFruitTestWorld world;

    // 在 (10, 64, 10) 放置耕地
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(10, 63, 10), farmlandState);

    // 在茎周围四个方向的下方也放置耕地/泥土
    world.setBlockAt(BlockPos(9, 63, 10), farmlandState);  // West
    world.setBlockAt(BlockPos(11, 63, 10), farmlandState); // East
    world.setBlockAt(BlockPos(10, 63, 9), farmlandState);  // North
    world.setBlockAt(BlockPos(10, 63, 11), farmlandState); // South

    // 在 (10, 64, 10) 放置最大年龄的茎
    const BlockState& stemStateAge7 = melonStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);

    // 尝试多个种子，因为 randomTick 有概率检查（1/randomBound 的概率）
    bool fruitPlaced = false;
    for (u64 seed = 0; seed < 200; ++seed) {
        BlockState mutableStemState = stemStateAge7;
        world.setBlockAt(BlockPos(10, 64, 10), &mutableStemState);
        // 清除可能的果实
        world.clearBlockAt(BlockPos(9, 64, 10));
        world.clearBlockAt(BlockPos(11, 64, 10));
        world.clearBlockAt(BlockPos(10, 64, 9));
        world.clearBlockAt(BlockPos(10, 64, 11));

        math::Random random(seed);
        melonStem->randomTick(world, BlockPos(10, 64, 10), mutableStemState, random);

        // 验证：至少一个相邻位置应放置了西瓜
        for (auto dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
            i32 fx = 10 + Directions::xOffset(dir);
            i32 fz = 10 + Directions::zOffset(dir);
            const BlockState* state = world.getBlockState(fx, 64, fz);
            if (state != nullptr && !state->isAir() && state->is(VanillaBlocks::MELON)) {
                fruitPlaced = true;
                break;
            }
        }
        if (fruitPlaced) {
            break;
        }
    }
    EXPECT_TRUE(fruitPlaced) << "Fruit should be placed when at least one direction is available";
}

/**
 * @brief 测试：当第一个方向被阻挡但后续方向可用时，tryGrowFruit 仍能放置果实
 *
 * 模拟场景：茎的北面和东面被方块占据，但南面和西面有空气+耕地。
 * 旧实现只尝试一个随机方向，如果选到被阻挡的方向就直接失败。
 * 新实现（Fisher-Yates 洗牌）会继续尝试剩余方向，直到找到可用的。
 *
 * 注意：此测试通过直接调用 randomTick 触发，概率检查可能不通过，
 * 因此我们设置一个使随机概率检查通过的种子序列。
 */
TEST_F(TryGrowFruitTest, FirstDirectionBlocked_SecondDirectionSucceeds)
{
    auto* pumpkinStem = dynamic_cast<StemBlock*>(VanillaBlocks::PUMPKIN_STEM);
    ASSERT_NE(pumpkinStem, nullptr) << "PUMPKIN_STEM should be registered";

    TryGrowFruitTestWorld world;

    // 在 (0, 64, 0) 放置耕地
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), farmlandState);

    // 在 (0, 63, -1) 放置耕地（北面可用）
    world.setBlockAt(BlockPos(0, 63, -1), farmlandState);
    // 在 (-1, 63, 0) 放置耕地（西面可用）
    world.setBlockAt(BlockPos(-1, 63, 0), farmlandState);
    // 东面和南面：不放耕地，果实不应放在这里

    // 在东面和南面放置空气下方的石头（非耕地/泥土）
    if (VanillaBlocks::STONE != nullptr) {
        world.setBlockAt(BlockPos(1, 63, 0), &VanillaBlocks::STONE->defaultState()); // 东面下方
        world.setBlockAt(BlockPos(0, 63, 1), &VanillaBlocks::STONE->defaultState()); // 南面下方
    }

    // 在 (0, 64, 0) 放置最大年龄的茎
    const BlockState& stemStateAge7 = pumpkinStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
    BlockState mutableStemState = stemStateAge7;
    world.setBlockAt(BlockPos(0, 64, 0), &mutableStemState);

    // 尝试多次不同种子，确保至少有一次果实生成成功
    // （因为 randomTick 有概率检查，单次可能不通过）
    bool fruitPlaced = false;
    for (u64 seed = 0; seed < 200; ++seed) {
        // 重置世界状态
        world.setBlockAt(BlockPos(0, 64, 0), &mutableStemState);
        // 移除可能放置的果实
        world.clearBlockAt(BlockPos(0, 64, -1));
        world.clearBlockAt(BlockPos(-1, 64, 0));
        world.clearBlockAt(BlockPos(1, 64, 0));
        world.clearBlockAt(BlockPos(0, 64, 1));

        math::Random random(seed);
        BlockState stateCopy = stemStateAge7;
        pumpkinStem->randomTick(world, BlockPos(0, 64, 0), stateCopy, random);

        // 检查是否有果实放在可用方向（北或西）
        const BlockState* northState = world.getBlockState(0, 64, -1);
        const BlockState* westState = world.getBlockState(-1, 64, 0);

        if ((northState != nullptr && !northState->isAir() && northState->is(VanillaBlocks::PUMPKIN)) ||
            (westState != nullptr && !westState->isAir() && westState->is(VanillaBlocks::PUMPKIN))) {
            fruitPlaced = true;
            break;
        }

        // 也确认果实不会放在不可用方向（东或南）
        const BlockState* eastState = world.getBlockState(1, 64, 0);
        const BlockState* southState = world.getBlockState(0, 64, 1);
        EXPECT_TRUE(eastState == nullptr || eastState->isAir() || !eastState->is(VanillaBlocks::PUMPKIN))
            << "Fruit should not be placed east (no farmland/dirt below)";
        EXPECT_TRUE(southState == nullptr || southState->isAir() || !southState->is(VanillaBlocks::PUMPKIN))
            << "Fruit should not be placed south (no farmland/dirt below)";
    }

    EXPECT_TRUE(fruitPlaced) << "Fruit should eventually be placed in an available direction "
                             << "(north or west) when east and south are blocked";
}

/**
 * @brief 测试：当所有方向都不可用时，tryGrowFruit 返回 false 且不放置果实
 *
 * 茎周围四个方向要么不是空气，要么下方不是耕地/泥土标签方块。
 */
TEST_F(TryGrowFruitTest, AllDirectionsBlocked_NoFruitPlaced)
{
    auto* melonStem = dynamic_cast<StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be registered";

    TryGrowFruitTestWorld world;

    // 只在 (5, 64, 5) 放置耕地（茎本身所在位置），不在任何相邻位置放耕地
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(5, 63, 5), farmlandState);

    // 四个相邻位置的下方都放石头（不是耕地也不是泥土标签方块）
    if (VanillaBlocks::STONE != nullptr) {
        world.setBlockAt(BlockPos(4, 63, 5), &VanillaBlocks::STONE->defaultState()); // West
        world.setBlockAt(BlockPos(6, 63, 5), &VanillaBlocks::STONE->defaultState()); // East
        world.setBlockAt(BlockPos(5, 63, 4), &VanillaBlocks::STONE->defaultState()); // North
        world.setBlockAt(BlockPos(5, 63, 6), &VanillaBlocks::STONE->defaultState()); // South
    }

    // 在 (5, 64, 5) 放置最大年龄的茎
    const BlockState& stemStateAge7 = melonStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
    BlockState mutableStemState = stemStateAge7;
    world.setBlockAt(BlockPos(5, 64, 5), &mutableStemState);

    // 多次尝试不同种子
    bool anyFruitPlaced = false;
    for (u64 seed = 0; seed < 100; ++seed) {
        // 重置：移除可能放置的果实
        world.clearBlockAt(BlockPos(4, 64, 5));
        world.clearBlockAt(BlockPos(6, 64, 5));
        world.clearBlockAt(BlockPos(5, 64, 4));
        world.clearBlockAt(BlockPos(5, 64, 6));
        // 重置茎
        world.setBlockAt(BlockPos(5, 64, 5), &mutableStemState);

        math::Random random(seed);
        BlockState stateCopy = stemStateAge7;
        melonStem->randomTick(world, BlockPos(5, 64, 5), stateCopy, random);

        // 检查四个方向都没有放果实
        for (auto dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
            i32 fx = 5 + Directions::xOffset(dir);
            i32 fz = 5 + Directions::zOffset(dir);
            const BlockState* state = world.getBlockState(fx, 64, fz);
            if (state != nullptr && !state->isAir() && state->is(VanillaBlocks::MELON)) {
                anyFruitPlaced = true;
            }
        }
    }

    EXPECT_FALSE(anyFruitPlaced) << "Fruit should NOT be placed when all directions have no valid ground";
}

/**
 * @brief 测试：果实成功放置后，茎变为连接茎，朝向与果实方向一致
 *
 * 验证 AttachedStemBlock 的 HORIZONTAL_FACING 属性与果实放置方向一致。
 */
TEST_F(TryGrowFruitTest, AttachedStemFacingMatchesFruitDirection)
{
    auto* melonStem = dynamic_cast<StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be registered";
    auto* attachedMelonStem = dynamic_cast<AttachedStemBlock*>(VanillaBlocks::ATTACHED_MELON_STEM);
    ASSERT_NE(attachedMelonStem, nullptr) << "ATTACHED_MELON_STEM should be registered";

    TryGrowFruitTestWorld world;

    // 只在南面放置耕地，确保果实只能放在南面
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), farmlandState); // 茎下方
    world.setBlockAt(BlockPos(0, 63, 1), farmlandState); // 南面下方

    // 在 (0, 64, 0) 放置最大年龄的茎
    const BlockState& stemStateAge7 = melonStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
    BlockState mutableStemState = stemStateAge7;
    world.setBlockAt(BlockPos(0, 64, 0), &mutableStemState);

    // 多次尝试，找到种子使果实放在南面
    bool found = false;
    for (u64 seed = 0; seed < 200; ++seed) {
        // 重置世界
        world.setBlockAt(BlockPos(0, 64, 0), &mutableStemState);
        world.clearBlockAt(BlockPos(0, 64, 1)); // 南面空气

        math::Random random(seed);
        BlockState stateCopy = stemStateAge7;
        melonStem->randomTick(world, BlockPos(0, 64, 0), stateCopy, random);

        // 检查南面是否放置了西瓜
        const BlockState* southState = world.getBlockState(0, 64, 1);
        if (southState != nullptr && !southState->isAir() && southState->is(VanillaBlocks::MELON)) {
            // 验证茎位置变为连接茎，朝向南（Direction::South）
            const BlockState* stemPosState = world.getBlockState(0, 64, 0);
            ASSERT_NE(stemPosState, nullptr);

            // 连接茎应该是 ATTACHED_MELON_STEM
            EXPECT_TRUE(stemPosState->is(VanillaBlocks::ATTACHED_MELON_STEM))
                << "Stem should have been replaced with attached stem";

            // 连接茎的朝向应该是南（指向果实方向）
            auto facing = stemPosState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
            EXPECT_TRUE(facing.has_value()) << "Attached stem should have HORIZONTAL_FACING property";
            if (facing.has_value()) {
                EXPECT_EQ(facing.value(), Direction::South) << "Attached stem should face south (toward the fruit)";
            }

            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "Should have found a seed where fruit is placed to the south";
}

/**
 * @brief 测试：Dirt 标签方块也能支撑果实生长
 *
 * 验证 tryGrowFruit 不仅检查 FARMLAND，也检查 DIRT 标签方块（如草地、泥土等）。
 */
TEST_F(TryGrowFruitTest, DirtTagBlockSupportsFruitGrowth)
{
    auto* pumpkinStem = dynamic_cast<StemBlock*>(VanillaBlocks::PUMPKIN_STEM);
    ASSERT_NE(pumpkinStem, nullptr) << "PUMPKIN_STEM should be registered";

    TryGrowFruitTestWorld world;

    // 在茎下方放耕地
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(3, 63, 3), farmlandState);

    // 在北面放草地（在 DIRT 标签中）
    if (VanillaBlocks::GRASS_BLOCK != nullptr) {
        world.setBlockAt(BlockPos(3, 63, 2), &VanillaBlocks::GRASS_BLOCK->defaultState()); // 北面下方
    }

    // 在 (3, 64, 3) 放置最大年龄的茎
    const BlockState& stemStateAge7 = pumpkinStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
    BlockState mutableStemState = stemStateAge7;
    world.setBlockAt(BlockPos(3, 64, 3), &mutableStemState);

    // 多次尝试
    bool fruitPlacedOnDirt = false;
    for (u64 seed = 0; seed < 200; ++seed) {
        // 重置
        world.setBlockAt(BlockPos(3, 64, 3), &mutableStemState);
        world.clearBlockAt(BlockPos(3, 64, 2)); // 北面空气

        math::Random random(seed);
        BlockState stateCopy = stemStateAge7;
        pumpkinStem->randomTick(world, BlockPos(3, 64, 3), stateCopy, random);

        // 检查北面是否放置了南瓜（草地标签方块在 DIRT 标签中）
        const BlockState* northState = world.getBlockState(3, 64, 2);
        if (northState != nullptr && !northState->isAir() && northState->is(VanillaBlocks::PUMPKIN)) {
            fruitPlacedOnDirt = true;
            break;
        }
    }

    EXPECT_TRUE(fruitPlacedOnDirt) << "Fruit should be able to grow on DIRT-tagged blocks (grass_block)";
}

/**
 * @brief 测试：Fisher-Yates 洗牌产生有效排列
 *
 * 验证修改后的 tryGrowFruit 使用 Fisher-Yates 洗牌算法，
 * 在多次调用中不会崩溃且能产生合理的结果分布。
 * 通过统计果实放置方向来间接验证洗牌的随机性。
 */
TEST_F(TryGrowFruitTest, FisherYatesShuffle_ProducesValidPermutation)
{
    auto* melonStem = dynamic_cast<StemBlock*>(VanillaBlocks::MELON_STEM);
    ASSERT_NE(melonStem, nullptr) << "MELON_STEM should be registered";

    TryGrowFruitTestWorld world;

    // 四个方向都有空气+耕地
    const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), farmlandState);
    world.setBlockAt(BlockPos(1, 63, 0), farmlandState);  // East
    world.setBlockAt(BlockPos(-1, 63, 0), farmlandState); // West
    world.setBlockAt(BlockPos(0, 63, 1), farmlandState);  // South
    world.setBlockAt(BlockPos(0, 63, -1), farmlandState); // North

    const BlockState& stemStateAge7 = melonStem->defaultState().with(BlockStateProperties::AGE_0_7(), 7);
    BlockState mutableStemState = stemStateAge7;

    // 统计各方向果实放置次数
    int northCount = 0;
    int southCount = 0;
    int eastCount = 0;
    int westCount = 0;
    int totalPlacements = 0;

    for (u64 seed = 0; seed < 200; ++seed) {
        // 重置世界
        world.setBlockAt(BlockPos(0, 64, 0), &mutableStemState);
        world.clearBlockAt(BlockPos(1, 64, 0));
        world.clearBlockAt(BlockPos(-1, 64, 0));
        world.clearBlockAt(BlockPos(0, 64, 1));
        world.clearBlockAt(BlockPos(0, 64, -1));

        math::Random random(seed);
        BlockState stateCopy = stemStateAge7;
        melonStem->randomTick(world, BlockPos(0, 64, 0), stateCopy, random);

        // 检查哪个方向放置了果实
        if (world.getBlockState(0, 64, -1) != nullptr && !world.getBlockState(0, 64, -1)->isAir() &&
            world.getBlockState(0, 64, -1)->is(VanillaBlocks::MELON)) {
            northCount++;
            totalPlacements++;
        } else if (world.getBlockState(0, 64, 1) != nullptr && !world.getBlockState(0, 64, 1)->isAir() &&
            world.getBlockState(0, 64, 1)->is(VanillaBlocks::MELON)) {
            southCount++;
            totalPlacements++;
        } else if (world.getBlockState(1, 64, 0) != nullptr && !world.getBlockState(1, 64, 0)->isAir() &&
            world.getBlockState(1, 64, 0)->is(VanillaBlocks::MELON)) {
            eastCount++;
            totalPlacements++;
        } else if (world.getBlockState(-1, 64, 0) != nullptr && !world.getBlockState(-1, 64, 0)->isAir() &&
            world.getBlockState(-1, 64, 0)->is(VanillaBlocks::MELON)) {
            westCount++;
            totalPlacements++;
        }
    }

    // 验证：多次运行中应该有多于1个方向被选中（洗牌的随机性）
    // 在4个方向都可用的情况下，如果实现正确地随机洗牌，
    // 至少应该有2个不同的方向被选中（统计上几乎不可能只选一个方向）
    int uniqueDirections = 0;
    if (northCount > 0) uniqueDirections++;
    if (southCount > 0) uniqueDirections++;
    if (eastCount > 0) uniqueDirections++;
    if (westCount > 0) uniqueDirections++;

    EXPECT_GT(totalPlacements, 0) << "Should have placed at least some fruits across 200 attempts";
    EXPECT_GE(uniqueDirections, 2) << "Fisher-Yates shuffle should produce varied directions. "
                                   << "North=" << northCount << " South=" << southCount << " East=" << eastCount
                                   << " West=" << westCount;
}
