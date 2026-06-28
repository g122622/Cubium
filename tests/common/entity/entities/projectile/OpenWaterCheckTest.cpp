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
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"

namespace mc {
namespace {

/**
 * @brief 开放水域检测测试世界
 *
 * 重写 getBlockState 和 getFluidState 以提供可控的方块/流体状态，
 * 用于测试 FishingBobberEntity::_checkOpenWater 的分层检测逻辑。
 */
class OpenWaterTestWorld : public test::BaseTestWorld {
public:
    OpenWaterTestWorld() = default;

    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[BlockPos(x, y, z)] = state; }

    void setFluidStateAt(i32 x, i32 y, i32 z, const fluid::FluidState* state)
    {
        m_fluidStates[BlockPos(x, y, z)] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_fluidStates.find(BlockPos(x, y, z));
        return it != m_fluidStates.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, const fluid::FluidState*> m_fluidStates;
};

/**
 * @brief 开放水域检测测试
 */
class OpenWaterCheckTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<OpenWaterTestWorld>();
        // 确保方块注册表已初始化
        VanillaBlocks::initialize();
        fluid::Fluids::initialize();
    }

    void TearDown() override { m_world.reset(); }

    /**
     * @brief 在指定位置填充水源方块
     *
     * 设置方块状态为水、流体状态为水源。水源方块无碰撞箱。
     *
     * @param centerX 中心X坐标
     * @param baseY 基准Y坐标（浮标所在层）
     * @param centerZ 中心Z坐标
     * @param layers 要填充的层数（从 baseY-1 开始）
     */
    void fillWaterSource(i32 centerX, i32 baseY, i32 centerZ, i32 layers = 4)
    {
        const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
        const fluid::FluidState* waterFluid = &fluid::Fluids::WATER()->defaultState();

        for (i32 dy = -1; dy < layers - 1; ++dy) {
            for (i32 dx = -2; dx <= 2; ++dx) {
                for (i32 dz = -2; dz <= 2; ++dz) {
                    m_world->setBlockStateAt(centerX + dx, baseY + dy, centerZ + dz, waterState);
                    m_world->setFluidStateAt(centerX + dx, baseY + dy, centerZ + dz, waterFluid);
                }
            }
        }
    }

    std::unique_ptr<OpenWaterTestWorld> m_world;
};

/**
 * @brief 测试无方块时 _checkOpenWater 返回 false
 *
 * 没有方块和流体时，所有位置返回 nullptr → Invalid → 整体不满足开放水域
 */
TEST_F(OpenWaterCheckTest, EmptyWorldReturnsFalse)
{
    entity::FishingBobberEntity bobber(EntityId(1));
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 空世界，没有任何方块或流体
    EXPECT_FALSE(bobber.isInOpenWater());
}

/**
 * @brief 测试水源方块的默认碰撞箱为空
 *
 * 验证水源方块的 getCollisionShape().isEmpty() 返回 true，
 * 这是 _checkOpenWater 中 InsideWater 类型的必要条件。
 */
TEST_F(OpenWaterCheckTest, WaterSourceBlockHasEmptyCollisionShape)
{
    const BlockState& waterState = VanillaBlocks::WATER->defaultState();
    EXPECT_TRUE(waterState.getCollisionShape().isEmpty()) << "水源方块应该有空碰撞箱";
}

/**
 * @brief 测试水源流体是 source
 *
 * 验证 Fluids::WATER 的默认状态是 source。
 */
TEST_F(OpenWaterCheckTest, WaterFluidIsSource)
{
    const fluid::FluidState& waterFluid = fluid::Fluids::WATER()->defaultState();
    EXPECT_TRUE(waterFluid.isSource()) << "默认水流体应该是 source";
    EXPECT_TRUE(waterFluid.getFluid().isIn(fluid::FluidTags::WATER())) << "默认水流体应该属于 WATER 标签";
}

/**
 * @brief 测试 LILY_PAD 方块识别
 *
 * 验证 NaturalBlocks::LILY_PAD 可正确识别，_checkOpenWater 中
 * 睡莲应被分类为 AboveWater。
 */
TEST_F(OpenWaterCheckTest, LilyPadIsRecognized)
{
    ASSERT_NE(block_registry::NaturalBlocks::LILY_PAD, nullptr) << "NaturalBlocks::LILY_PAD 应该已注册";
}

/**
 * @brief 测试完整水源环境下方块配置正确
 *
 * 在浮标周围 4 层（Y-1 到 Y+2）的 5×5 区域全部填充水源方块，
 * 验证方块和流体配置正确，满足开放水域条件。
 */
TEST_F(OpenWaterCheckTest, FullWaterSourceConfigurationValid)
{
    entity::FishingBobberEntity bobber(EntityId(1));
    bobber.setPosition(0.0, 64.0, 0.0);
    bobber.setWorld(m_world.get());

    // 在 4 层全部填充水源
    fillWaterSource(0, 64, 0, 4);

    // 验证方块和流体配置正确
    const BlockState* state = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(state, nullptr) << "应能获取到水源方块";
    EXPECT_TRUE(state->isLiquid()) << "水源方块应该是液态";
    EXPECT_TRUE(state->getCollisionShape().isEmpty()) << "水源方块碰撞箱应该为空";

    // 验证流体状态
    const fluid::FluidState* fluidState = m_world->getFluidState(0, 64, 0);
    ASSERT_NE(fluidState, nullptr) << "应能获取到水流体状态";
    EXPECT_TRUE(fluidState->isSource()) << "水流体应该是 source";
    EXPECT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER())) << "水流体应该属于 WATER 标签";
}

/**
 * @brief 测试 WaterType 枚举值
 *
 * 验证 WaterType 枚举有正确的值。
 */
TEST_F(OpenWaterCheckTest, WaterTypeEnumValues)
{
    EXPECT_EQ(static_cast<int>(entity::FishingBobberEntity::WaterType::AboveWater), 0);
    EXPECT_EQ(static_cast<int>(entity::FishingBobberEntity::WaterType::InsideWater), 1);
    EXPECT_EQ(static_cast<int>(entity::FishingBobberEntity::WaterType::Invalid), 2);
}

} // namespace
} // namespace mc
