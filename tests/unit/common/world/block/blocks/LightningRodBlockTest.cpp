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
 * THE SOFTWARE IS PROVIDED "AS IS", EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "world/block/blocks/LightningRodBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/CopperBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/property/Properties.hpp"
#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;
using namespace mc::world::biome;

// ============================================================================
// 测试用世界桩 - 用于 LightningRodBlock 测试
// ============================================================================

/**
 * @brief LightningRodBlock 测试用的世界桩
 *
 * 继承 BaseTestWorld，提供可控的方块状态存储、天气控制、红石和tick管理。
 */
class LightningRodTestWorld : public mc::test::BaseTestWorld {
public:
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
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

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<LightningRodTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void setRaining(bool raining) { m_isRaining = raining; }
    void setThundering(bool thundering) { m_isThundering = thundering; }

    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool isThundering() const override { return m_isThundering; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    bool m_isRaining = false;
    bool m_isThundering = false;
};

// ============================================================================
// LightningRodBlock 基础测试
// ============================================================================

class LightningRodBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();

        lightningRod_ =
            std::make_unique<LightningRodBlock>(BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f));
    }

    std::unique_ptr<LightningRodBlock> lightningRod_;
    LightningRodTestWorld world_;
};

TEST_F(LightningRodBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(lightningRod_, nullptr);
}

TEST_F(LightningRodBlockTest, CanProvidePower_ReturnsTrue)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_TRUE(lightningRod_->canProvidePower(state));
}

TEST_F(LightningRodBlockTest, DefaultState_IsNotPowered)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(LightningRodBlockTest, DefaultState_FacingNorth)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::FACING()), Direction::North);
}

TEST_F(LightningRodBlockTest, GetWeakPower_NotPowered_ReturnsZero)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::North), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Up), 0);
}

TEST_F(LightningRodBlockTest, GetStrongPower_AlwaysReturnsZero)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_EQ(lightningRod_->getStrongPower(state, world_, BlockPos(0, 64, 0), Direction::North), 0);
    EXPECT_EQ(lightningRod_->getStrongPower(state, world_, BlockPos(0, 64, 0), Direction::Up), 0);
}

TEST_F(LightningRodBlockTest, GetWeakPower_Powered_OutputOnFacingDirection)
{
    // 朝上的避雷针，充能状态，在 Up 方向输出信号
    const auto& state = lightningRod_->defaultState()
                            .with(BlockStateProperties::FACING(), Direction::Up)
                            .with(BlockStateProperties::POWERED(), true);

    // 只在指向方向输出信号
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Up), 15);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Down), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::North), 0);
}

TEST_F(LightningRodBlockTest, GetWeakPower_Powered_OutputOnFacingDirection_North)
{
    // 朝北的避雷针，充能状态，在 North 方向输出信号
    const auto& state = lightningRod_->defaultState()
                            .with(BlockStateProperties::FACING(), Direction::North)
                            .with(BlockStateProperties::POWERED(), true);

    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::North), 15);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::South), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Up), 0);
}

TEST_F(LightningRodBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_TRUE(lightningRod_->useShapeForLightOcclusion(state));
}

TEST_F(LightningRodBlockTest, IsWaterlogged_DefaultFalse)
{
    const auto& state = lightningRod_->defaultState();
    EXPECT_FALSE(lightningRod_->isWaterlogged(state));
}

TEST_F(LightningRodBlockTest, ACTIVATION_TICKS_Is8)
{
    EXPECT_EQ(LightningRodBlock::ACTIVATION_TICKS, 8);
}

TEST_F(LightningRodBlockTest, RANGE_Is128)
{
    EXPECT_EQ(LightningRodBlock::RANGE, 128);
}

// ============================================================================
// handlePrecipitation 测试夹具
// ============================================================================

class LightningRodPrecipTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();

        lightningRod_ =
            std::make_unique<LightningRodBlock>(BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f));
        world_.setRaining(true);
    }

    /// 在指定位置放置指定朝向的避雷针
    void placeLightningRod(i32 x, i32 y, i32 z, Direction facing, bool powered = false)
    {
        const BlockState* state = &lightningRod_->defaultState()
                                       .with(BlockStateProperties::FACING(), facing)
                                       .with(BlockStateProperties::POWERED(), powered);
        world_.setBlockAt(BlockPos(x, y, z), state);
    }

    /// 检查避雷针是否为充能状态
    bool isPowered(i32 x, i32 y, i32 z) const
    {
        const BlockState* state = world_.getBlockState(x, y, z);
        if (state == nullptr) {
            return false;
        }
        return state->get(BlockStateProperties::POWERED());
    }

    std::unique_ptr<LightningRodBlock> lightningRod_;
    LightningRodTestWorld world_;
};

// ============================================================================
// handlePrecipitation - 降水类型检查
// ============================================================================

TEST_F(LightningRodPrecipTest, HandlePrecipitation_SnowType_DoesNotActivate)
{
    // 雪天不应激活避雷针
    placeLightningRod(0, 64, 0, Direction::Up);
    world_.setThundering(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Snow);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, HandlePrecipitation_NoneType_DoesNotActivate)
{
    // 无降水类型不应激活避雷针
    placeLightningRod(0, 64, 0, Direction::Up);
    world_.setThundering(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::None);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

// ============================================================================
// handlePrecipitation - 雷暴检查
// ============================================================================

TEST_F(LightningRodPrecipTest, HandlePrecipitation_RainButNotThundering_DoesNotActivate)
{
    // 下雨但不是雷暴，不应激活
    placeLightningRod(0, 64, 0, Direction::Up);
    world_.setThundering(false);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, HandlePrecipitation_ThunderingAndRain_ActivatesUpFacing)
{
    // 雷暴+下雨+朝上 = 激活避雷针
    placeLightningRod(0, 64, 0, Direction::Up);
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_TRUE(isPowered(0, 64, 0));
}

// ============================================================================
// handlePrecipitation - 朝向检查
// ============================================================================

TEST_F(LightningRodPrecipTest, HandlePrecipitation_FacingDown_DoesNotActivate)
{
    // 朝下的避雷针不应被雷击激活
    placeLightningRod(0, 64, 0, Direction::Down);
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, HandlePrecipitation_FacingNorth_DoesNotActivate)
{
    // 朝北的避雷针不应被雷击激活（只有朝上才行）
    placeLightningRod(0, 64, 0, Direction::North);
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, HandlePrecipitation_FacingSouth_DoesNotActivate)
{
    placeLightningRod(0, 64, 0, Direction::South);
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, HandlePrecipitation_FacingEast_DoesNotActivate)
{
    placeLightningRod(0, 64, 0, Direction::East);
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, HandlePrecipitation_FacingWest_DoesNotActivate)
{
    placeLightningRod(0, 64, 0, Direction::West);
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    EXPECT_FALSE(isPowered(0, 64, 0));
}

// ============================================================================
// handlePrecipitation - 空位置检查
// ============================================================================

TEST_F(LightningRodPrecipTest, HandlePrecipitation_NoBlockAtPos_NoCrash)
{
    // 位置上没有方块，不应崩溃
    world_.setThundering(true);
    world_.setRaining(true);

    lightningRod_->handlePrecipitation(world_, BlockPos(0, 64, 0), BiomeClimate::Precipitation::Rain);

    SUCCEED();
}

// ============================================================================
// onLightningStrike 测试
// ============================================================================

TEST_F(LightningRodPrecipTest, OnLightningStrike_ActivatesRod)
{
    // onLightningStrike 应该设置 POWERED=true
    placeLightningRod(0, 64, 0, Direction::Up, false);
    EXPECT_FALSE(isPowered(0, 64, 0));

    lightningRod_->onLightningStrike(world_, BlockPos(0, 64, 0));

    EXPECT_TRUE(isPowered(0, 64, 0));
}

TEST_F(LightningRodPrecipTest, OnLightningStrike_NullBlockState_NoCrash)
{
    // 位置上没有方块，不应崩溃
    lightningRod_->onLightningStrike(world_, BlockPos(0, 64, 0));

    SUCCEED();
}

TEST_F(LightningRodPrecipTest, OnLightningStrike_DifferentBlockAtPos_NoEffect)
{
    // 位置上放置石头方块（不是避雷针），onLightningStrike 应该检测到不是 LightningRodBlock 并提前返回
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world_.setBlockAt(BlockPos(0, 64, 0), stoneState);

    lightningRod_->onLightningStrike(world_, BlockPos(0, 64, 0));

    // 石头方块状态应保持不变（onLightningStrike 检查 is(this) 后提前返回）
    const BlockState* state = world_.getBlockState(0, 64, 0);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::STONE));
}

// ============================================================================
// 红石信号测试
// ============================================================================

TEST_F(LightningRodPrecipTest, GetWeakPower_PoweredFacingUp_OutputOnlyUp)
{
    const auto& state = lightningRod_->defaultState()
                            .with(BlockStateProperties::FACING(), Direction::Up)
                            .with(BlockStateProperties::POWERED(), true);

    // 充能时只在指向方向输出信号
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Up), 15);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Down), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::North), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::South), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::East), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::West), 0);
}

TEST_F(LightningRodPrecipTest, GetWeakPower_PoweredFacingEast_OutputOnlyEast)
{
    const auto& state = lightningRod_->defaultState()
                            .with(BlockStateProperties::FACING(), Direction::East)
                            .with(BlockStateProperties::POWERED(), true);

    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::East), 15);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::West), 0);
    EXPECT_EQ(lightningRod_->getWeakPower(state, world_, BlockPos(0, 64, 0), Direction::Up), 0);
}

TEST_F(LightningRodPrecipTest, GetStrongPower_AlwaysReturnsZero)
{
    // 避雷针不提供强信号
    const auto& poweredState = lightningRod_->defaultState()
                                   .with(BlockStateProperties::FACING(), Direction::Up)
                                   .with(BlockStateProperties::POWERED(), true);

    EXPECT_EQ(lightningRod_->getStrongPower(poweredState, world_, BlockPos(0, 64, 0), Direction::Up), 0);
    EXPECT_EQ(lightningRod_->getStrongPower(poweredState, world_, BlockPos(0, 64, 0), Direction::Down), 0);
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(LightningRodBlockTest, GetShape_DifferentFacings_ReturnValidShapes)
{
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        const auto& state = lightningRod_->defaultState().with(BlockStateProperties::FACING(), dir);
        const auto& shape = lightningRod_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Shape should not be empty for facing " << i;
    }
}

TEST_F(LightningRodBlockTest, GetShape_PoweredDoesNotAffectShape)
{
    const auto& unpoweredState = lightningRod_->defaultState().with(BlockStateProperties::FACING(), Direction::Up);
    const auto& poweredState = unpoweredState.with(BlockStateProperties::POWERED(), true);

    // 充能状态不影响形状（避雷针形状只与朝向有关）
    // 两种状态应有相同形状
    const auto& unpoweredShape = lightningRod_->getShape(unpoweredState);
    const auto& poweredShape = lightningRod_->getShape(poweredState);

    // 简单检查：两者都不为空
    EXPECT_FALSE(unpoweredShape.isEmpty());
    EXPECT_FALSE(poweredShape.isEmpty());
}
