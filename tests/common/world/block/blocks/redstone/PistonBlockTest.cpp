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
 * @file PistonBlockTest.cpp
 * @brief PistonBlock 单元测试
 *
 * 测试活塞方块的推动功能，特别是世界边界检查。
 */

#include "common/world/block/blocks/redstone/PistonBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 用于 PistonBlock 测试的 Mock World 实现
 */
class PistonBlockTestWorld final : public ::mc::test::BaseTestWorld {
public:
    PistonBlockTestWorld()
    {
        // 设置默认世界边界（6000万格直径，中心在原点）
        m_worldBorder.setSize(60000000.0);
    }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] bool isClientSide() override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PistonBlockTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PistonBlockTestWorld::tickManager not implemented");
    }

    // 测试辅助方法

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    void advanceTick() { m_currentTick++; }

    /**
     * @brief 设置世界边界大小
     * @param size 边界直径
     */
    void setWorldBorderSize(double size) { m_worldBorder.setSize(size); }

    /**
     * @brief 设置世界边界中心
     * @param x X 坐标
     * @param z Z 坐标
     */
    void setWorldBorderCenter(double x, double z) { m_worldBorder.setCenter(x, z); }

    void clearState() { m_blocks.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
};

/**
 * @brief PistonBlock 测试固件
 */
class PistonBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override { m_world.clearState(); }

    PistonBlockTestWorld m_world;
};

// ========== canPush 世界边界测试 ==========

/**
 * @brief 测试 canPush - 边界内的方块可以被推动
 *
 * 默认世界边界中心在原点，直径6000万格，原点附近都在边界内。
 */
TEST_F(PistonBlockTest, CanPush_BlockInsideWorldBorder_ReturnsTrue)
{
    // 创建普通方块（石头可以被推动）
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 64, 0); // 原点附近，在边界内

    // 空气可以推动
    EXPECT_TRUE(PistonBlock::canPush(stoneState, m_world, pos, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 canPush - 边界外的方块不能被推动
 *
 * 设置小的世界边界，测试边界外的方块。
 */
TEST_F(PistonBlockTest, CanPush_BlockOutsideWorldBorder_ReturnsFalse)
{
    // 设置小边界（直径 10 格，中心在原点）
    m_world.setWorldBorderSize(10.0);

    // 创建普通方块（石头）
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 边界外的位置（X=10 超出了边界范围 -5 到 5）
    BlockPos posOutside(10, 64, 0);

    // 边界外的方块不能被推动
    EXPECT_FALSE(PistonBlock::canPush(stoneState, m_world, posOutside, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 canPush - 恰好在边界边缘的方块
 *
 * 测试边界边缘位置（边界范围是 -5 到 5）。
 */
TEST_F(PistonBlockTest, CanPush_BlockOnWorldBorderEdge)
{
    // 设置小边界（直径 10 格，中心在原点）
    // 边界范围：X: -5 到 5, Z: -5 到 5
    m_world.setWorldBorderSize(10.0);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 边界内的位置（X=4 在边界内）
    BlockPos posInside(4, 64, 0);
    EXPECT_TRUE(PistonBlock::canPush(stoneState, m_world, posInside, Direction::Up, true, Direction::Up));

    // 边界外的位置（X=6 超出边界）
    BlockPos posOutside(6, 64, 0);
    EXPECT_FALSE(PistonBlock::canPush(stoneState, m_world, posOutside, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 canPush - 边界中心偏移
 *
 * 测试世界边界中心不在原点的情况。
 */
TEST_F(PistonBlockTest, CanPush_OffCenterWorldBorder)
{
    // 设置边界中心在 (100, 100)，直径 10 格
    // 边界范围：X: 95 到 105, Z: 95 到 105
    m_world.setWorldBorderCenter(100.0, 100.0);
    m_world.setWorldBorderSize(10.0);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 边界内的位置
    BlockPos posInside(100, 64, 100);
    EXPECT_TRUE(PistonBlock::canPush(stoneState, m_world, posInside, Direction::Up, true, Direction::Up));

    // 边界外的位置（原点现在在边界外）
    BlockPos posOutside(0, 64, 0);
    EXPECT_FALSE(PistonBlock::canPush(stoneState, m_world, posOutside, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 canPush - 空气在世界边界外也不能推动
 *
 * 空气虽然特殊，但仍然受世界边界限制。
 */
TEST_F(PistonBlockTest, CanPush_AirOutsideWorldBorder_ReturnsFalse)
{
    // 设置小边界
    m_world.setWorldBorderSize(10.0);

    // 空气状态
    const BlockState& airState = VanillaBlocks::AIR->defaultState();

    // 边界外的位置
    BlockPos posOutside(10, 64, 0);

    // 空气在边界外也不能推动
    EXPECT_FALSE(PistonBlock::canPush(airState, m_world, posOutside, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 canPush - 不可推动方块在边界内仍不可推动
 *
 * 黑曜石等不可推动方块，即使在边界内也不能被活塞推动。
 */
TEST_F(PistonBlockTest, CanPush_UnpushableBlockInsideBorder_ReturnsFalse)
{
    // 黑曜石不能被活塞推动
    const BlockState& obsidianState = VanillaBlocks::OBSIDIAN->defaultState();

    // 边界内的位置
    BlockPos pos(0, 64, 0);

    // 黑曜石即使在边界内也不能推动
    EXPECT_FALSE(PistonBlock::canPush(obsidianState, m_world, pos, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 canPush - 高度边界检查与世界边界独立
 *
 * 世界边界只影响 X/Z 平面，高度边界由世界高度限制控制。
 */
TEST_F(PistonBlockTest, CanPush_HeightLimit)
{
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 在高度范围内的位置
    BlockPos posInHeight(0, 64, 0);
    EXPECT_TRUE(PistonBlock::canPush(stoneState, m_world, posInHeight, Direction::Up, true, Direction::Up));

    // 超出高度范围的位置（负高度）
    BlockPos posBelowHeight(0, -1, 0);
    EXPECT_FALSE(PistonBlock::canPush(stoneState, m_world, posBelowHeight, Direction::Up, true, Direction::Up));
}

/**
 * @brief 测试 PistonBlock 构造函数
 */
TEST_F(PistonBlockTest, Construction)
{
    BlockProperties props(Material::PISTON);
    auto pistonBlock = std::make_unique<PistonBlock>(props, false);
    ASSERT_NE(pistonBlock, nullptr);
    EXPECT_FALSE(pistonBlock->isSticky());

    auto stickyPistonBlock = std::make_unique<PistonBlock>(props, true);
    ASSERT_NE(stickyPistonBlock, nullptr);
    EXPECT_TRUE(stickyPistonBlock->isSticky());
}

/**
 * @brief 测试 isExtended 静态方法
 */
TEST_F(PistonBlockTest, IsExtended)
{
    BlockProperties props(Material::PISTON);
    auto pistonBlock = std::make_unique<PistonBlock>(props, false);
    const BlockState& defaultState = pistonBlock->defaultState();

    // 默认状态应该是未伸出
    EXPECT_FALSE(PistonBlock::isExtended(defaultState));
}

} // namespace test
} // namespace blocks
} // namespace mc
