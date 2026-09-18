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
#include "common/core/Constants.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现，支持可编程的流体状态
 *
 * 在指定坐标返回水源 FluidState，其余位置返回空流体。
 */
class ContainsAnyLiquidTestWorld final : public mc::test::BaseTestWorld {
public:
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
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // 如果该位置标记为有水，返回水源 FluidState
        if (m_waterPositions.count(BlockPos(x, y, z)) > 0) {
            return &fluid::Fluids::WATER()->defaultState();
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ContainsAnyLiquidTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ContainsAnyLiquidTestWorld::tickManager not implemented");
    }

    /**
     * @brief 标记指定位置有水
     */
    void setWaterAt(i32 x, i32 y, i32 z) { m_waterPositions.emplace(x, y, z); }

    /**
     * @brief 清除所有水标记
     */
    void clearWater() { m_waterPositions.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_set<BlockPos> m_waterPositions;
};

class ContainsAnyLiquidTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化流体注册表和 Fluids 缓存指针
        fluid::FluidRegistry::instance().initialize();
        fluid::Fluids::initialize();
        m_world = std::make_unique<ContainsAnyLiquidTestWorld>();
    }

    std::unique_ptr<ContainsAnyLiquidTestWorld> m_world;
};

// ============================================================================
// containsAnyLiquid 测试
// ============================================================================

TEST_F(ContainsAnyLiquidTest, EmptyAABBReturnsFalse)
{
    // 空碰撞箱（无方块位置覆盖）
    AxisAlignedBB box(0.5, 64.0, 0.5, 0.5, 64.0, 0.5);
    EXPECT_FALSE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, NoFluidsReturnsFalse)
{
    // 没有任何流体的世界
    AxisAlignedBB box(0.0, 64.0, 0.0, 2.0, 66.0, 2.0);
    EXPECT_FALSE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, SingleFluidBlockInsideAABB)
{
    // 在 (0, 64, 0) 放置水
    m_world->setWaterAt(0, 64, 0);

    // 碰撞箱包含该位置
    AxisAlignedBB box(0.0, 64.0, 0.0, 1.0, 65.0, 1.0);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, FluidBlockOutsideAABB)
{
    // 在 (10, 64, 10) 放置水
    m_world->setWaterAt(10, 64, 10);

    // 碰撞箱不包含该位置
    AxisAlignedBB box(0.0, 64.0, 0.0, 2.0, 66.0, 2.0);
    EXPECT_FALSE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, FluidOnAABBEdge)
{
    // 碰撞箱边缘处的流体
    // AABB covers floor(0.0)=0 to floor(1.0)=1 for x, floor(64.0)=64 for y, etc.
    m_world->setWaterAt(1, 64, 1);

    AxisAlignedBB box(0.0, 64.0, 0.0, 1.0, 65.0, 1.0);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, MultipleFluidBlocksInAABB)
{
    // 多个流体方块
    m_world->setWaterAt(0, 64, 0);
    m_world->setWaterAt(1, 64, 0);

    AxisAlignedBB box(0.0, 64.0, 0.0, 2.0, 65.0, 1.0);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, MultiLayerAABB)
{
    // 多层碰撞箱中只有上层有流体
    m_world->setWaterAt(0, 66, 0);

    // AABB 跨越 y=64 到 y=67
    AxisAlignedBB box(0.0, 64.0, 0.0, 1.0, 67.0, 1.0);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, MultiLayerAABBFluidAtBottomOnly)
{
    // 多层碰撞箱中只有底层有流体
    m_world->setWaterAt(0, 64, 0);

    AxisAlignedBB box(0.0, 64.0, 0.0, 1.0, 67.0, 1.0);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, PointAABBAtFluidPosition)
{
    // 单点碰撞箱正好在流体位置
    m_world->setWaterAt(5, 70, 5);

    // 单点 AABB：floor(5.0)=5, floor(70.0)=70
    AxisAlignedBB box(5.0, 70.0, 5.0, 5.0, 70.0, 5.0);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, NegativeCoordinates)
{
    // 负坐标流体
    m_world->setWaterAt(-1, 64, -1);

    AxisAlignedBB box(-1.5, 64.0, -1.5, 0.5, 65.0, 0.5);
    EXPECT_TRUE(m_world->containsAnyLiquid(box));
}

TEST_F(ContainsAnyLiquidTest, LargeAABBNoFluid)
{
    // 大碰撞箱无流体
    AxisAlignedBB box(-10.0, 0.0, -10.0, 10.0, 128.0, 10.0);
    EXPECT_FALSE(m_world->containsAnyLiquid(box));
}

} // namespace
} // namespace mc
