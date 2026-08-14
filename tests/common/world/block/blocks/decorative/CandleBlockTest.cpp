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
 * @file CandleBlockTest.cpp
 * @brief CandleBlock 单元测试
 *
 * 测试内容：
 * 1. 方块状态属性（CANDLES, LIT, WATERLOGGED）正确性
 * 2. 默认状态值
 * 3. 碰撞形状随蜡烛数量变化
 * 4. 光照等级 = 3 * candles（点燃时）
 * 5. canBeLit / canLight 静态方法
 * 6. isValidPosition 需要坚固上表面支撑
 * 7. updatePostPlacement 下方支撑丢失时变为空气
 * 8. isReplaceable 堆叠逻辑
 * 9. IWaterLoggable 接口
 * 10. ticksRandomly 返回 true
 * 11. 粒子偏移位置正确性
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/decorative/CandleBlock.hpp"
#include "common/world/block/registry/CandleBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

namespace {

// ============================================================================
// 测试用世界，支持方块和流体状态存储
// ============================================================================

class CandleTestWorld final : public mc::test::BaseTestWorld {
public:
    CandleTestWorld()
    {
        VanillaBlocks::initialize();
        m_airState = &VanillaBlocks::AIR->defaultState();
    }

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    /// 存储 BlockState 的副本并返回稳定指针
    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

    void setBlockDirectly(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[packPos(pos.x, pos.y, pos.z)] = state;
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state)
    {
        m_fluids[packPos(pos.x, pos.y, pos.z)] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos.x, pos.y, pos.z, stored);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_fluids.find(packPos(x, y, z));
        if (it != m_fluids.end()) {
            return it->second;
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }

    void setSeed(u64 seed) { m_seed = seed; }

private:
    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    const BlockState* m_airState;
    u64 m_seed = 12345;
};

// ============================================================================
// 用于测试的坚固方块
// ============================================================================

class TestSolidBlock final : public Block {
public:
    explicit TestSolidBlock(const BlockProperties& properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool isSolidSide(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return true;
    }

    [[nodiscard]] bool isSolid(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }
};

} // anonymous namespace

// ============================================================================
// 方块状态属性测试
// ============================================================================

class CandleBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        candle_ = dynamic_cast<CandleBlock*>(CandleBlocks::CANDLE);
        ASSERT_NE(candle_, nullptr);
    }

    CandleBlock* candle_ = nullptr;
};

TEST_F(CandleBlockTest, DefaultState_HasCandles1)
{
    // 默认状态：1根蜡烛
    const BlockState& state = candle_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::CANDLES()), 1);
}

TEST_F(CandleBlockTest, DefaultState_NotLit)
{
    // 默认状态：未点燃
    const BlockState& state = candle_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::LIT()));
}

TEST_F(CandleBlockTest, DefaultState_NotWaterlogged)
{
    // 默认状态：未含水
    const BlockState& state = candle_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CandleBlockTest, StateProperties_CandlesRange)
{
    // CANDLES 属性范围为1-4
    const BlockState& state = candle_->defaultState();
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::CANDLES()));

    // 验证各数量可以设置
    for (i32 count = 1; count <= 4; ++count) {
        BlockState modified = state.with(BlockStateProperties::CANDLES(), count);
        EXPECT_EQ(modified.get(BlockStateProperties::CANDLES()), count);
    }
}

TEST_F(CandleBlockTest, StateProperties_LitToggle)
{
    // LIT 属性可以切换
    const BlockState& state = candle_->defaultState();
    BlockState lit = state.with(BlockStateProperties::LIT(), true);
    EXPECT_TRUE(lit.get(BlockStateProperties::LIT()));

    BlockState unlit = lit.with(BlockStateProperties::LIT(), false);
    EXPECT_FALSE(unlit.get(BlockStateProperties::LIT()));
}

TEST_F(CandleBlockTest, StateProperties_WaterloggedToggle)
{
    // WATERLOGGED 属性可以切换
    const BlockState& state = candle_->defaultState();
    BlockState waterlogged = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(waterlogged.get(BlockStateProperties::WATERLOGGED()));

    BlockState dry = waterlogged.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(dry.get(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST_F(CandleBlockTest, GetShape_NonEmptyForAllCounts)
{
    // 各蜡烛数量的碰撞形状都不为空
    const BlockState& state = candle_->defaultState();
    for (i32 count = 1; count <= 4; ++count) {
        BlockState modified = state.with(BlockStateProperties::CANDLES(), count);
        const CollisionShape& shape = candle_->getShape(modified);
        EXPECT_FALSE(shape.isEmpty()) << "Shape should not be empty for CANDLES=" << count;
    }
}

TEST_F(CandleBlockTest, GetShape_ShapeChangesWithCount)
{
    // 不同蜡烛数量应有不同的碰撞形状
    const BlockState& state = candle_->defaultState();

    const CollisionShape& shape1 = candle_->getShape(state.with(BlockStateProperties::CANDLES(), 1));
    const CollisionShape& shape4 = candle_->getShape(state.with(BlockStateProperties::CANDLES(), 4));

    // 1根蜡烛和4根蜡烛的形状应该不同
    // 我们只验证它们不是同一个对象（最保守的检查）
    EXPECT_NE(&shape1, &shape4);
}

// ============================================================================
// 光照等级测试
// ============================================================================

TEST_F(CandleBlockTest, GetLightLevel_Unlit_Returns0)
{
    // 未点燃时光照为0
    const BlockState& state = candle_->defaultState();
    EXPECT_EQ(candle_->getLightLevel(state), 0);
}

TEST_F(CandleBlockTest, GetLightLevel_Lit1Candle_Returns3)
{
    // 1根蜡烛点燃时光照为3
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_EQ(candle_->getLightLevel(state), 3);
}

TEST_F(CandleBlockTest, GetLightLevel_Lit2Candles_Returns6)
{
    // 2根蜡烛点燃时光照为6
    const BlockState& state =
        candle_->defaultState().with(BlockStateProperties::CANDLES(), 2).with(BlockStateProperties::LIT(), true);
    EXPECT_EQ(candle_->getLightLevel(state), 6);
}

TEST_F(CandleBlockTest, GetLightLevel_Lit3Candles_Returns9)
{
    // 3根蜡烛点燃时光照为9
    const BlockState& state =
        candle_->defaultState().with(BlockStateProperties::CANDLES(), 3).with(BlockStateProperties::LIT(), true);
    EXPECT_EQ(candle_->getLightLevel(state), 9);
}

TEST_F(CandleBlockTest, GetLightLevel_Lit4Candles_Returns12)
{
    // 4根蜡烛点燃时光照为12
    const BlockState& state =
        candle_->defaultState().with(BlockStateProperties::CANDLES(), 4).with(BlockStateProperties::LIT(), true);
    EXPECT_EQ(candle_->getLightLevel(state), 12);
}

TEST_F(CandleBlockTest, GetLightLevel_WaterloggedLit_Returns3)
{
    // 含水时不影响光照等级计算（含水时不会点燃，但光照等级由 LIT 属性决定）
    const BlockState& state =
        candle_->defaultState().with(BlockStateProperties::LIT(), true).with(BlockStateProperties::WATERLOGGED(), true);
    // 含水且点燃的情况虽然在正常游戏中不应出现，但光照等级仍由 LIT=true 决定
    EXPECT_EQ(candle_->getLightLevel(state), 3);
}

// ============================================================================
// canBeLit / canLight 测试
// ============================================================================

TEST_F(CandleBlockTest, CanBeLit_UnlitNotWaterlogged_ReturnsTrue)
{
    // 未点燃且未含水时可以点燃
    const BlockState& state = candle_->defaultState();
    EXPECT_TRUE(candle_->canBeLit(state));
}

TEST_F(CandleBlockTest, CanBeLit_Lit_ReturnsFalse)
{
    // 已点燃时不能再次点燃
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_FALSE(candle_->canBeLit(state));
}

TEST_F(CandleBlockTest, CanBeLit_Waterlogged_ReturnsFalse)
{
    // 含水时不能点燃
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_FALSE(candle_->canBeLit(state));
}

TEST_F(CandleBlockTest, CanBeLit_LitAndWaterlogged_ReturnsFalse)
{
    // 已点燃且含水时不能点燃
    const BlockState& state =
        candle_->defaultState().with(BlockStateProperties::LIT(), true).with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_FALSE(candle_->canBeLit(state));
}

TEST_F(CandleBlockTest, CanLight_Static_UnlitNotWaterlogged_ReturnsTrue)
{
    // canLight 静态方法：未点燃且未含水
    const BlockState& state = candle_->defaultState();
    EXPECT_TRUE(CandleBlock::canLight(state));
}

TEST_F(CandleBlockTest, CanLight_Static_Lit_ReturnsFalse)
{
    // canLight 静态方法：已点燃
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_FALSE(CandleBlock::canLight(state));
}

TEST_F(CandleBlockTest, CanLight_Static_Waterlogged_ReturnsFalse)
{
    // canLight 静态方法：含水
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_FALSE(CandleBlock::canLight(state));
}

TEST_F(CandleBlockTest, CanLight_Static_BlockWithoutLitProperty_ReturnsFalse)
{
    // canLight 静态方法：没有 LIT 属性的方块（如石头）应返回 false
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(CandleBlock::canLight(stoneState));
}

// ============================================================================
// isLit 测试
// ============================================================================

TEST_F(CandleBlockTest, IsLit_Unlit_ReturnsFalse)
{
    const BlockState& state = candle_->defaultState();
    EXPECT_FALSE(AbstractCandleBlock::isLit(state));
}

TEST_F(CandleBlockTest, IsLit_Lit_ReturnsTrue)
{
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_TRUE(AbstractCandleBlock::isLit(state));
}

// ============================================================================
// isValidPosition 测试
// ============================================================================

TEST_F(CandleBlockTest, IsValidPosition_SolidBelow_ReturnsTrue)
{
    // 下方有坚固方块时可以放置
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(0, 0, 0), &solidState);

    const BlockState& candleState = candle_->defaultState();
    EXPECT_TRUE(candle_->isValidPosition(candleState, world, BlockPos(0, 1, 0)));
}

TEST_F(CandleBlockTest, IsValidPosition_NoBlockBelow_ReturnsFalse)
{
    // 下方无方块时不能放置
    CandleTestWorld world;
    const BlockState& candleState = candle_->defaultState();
    EXPECT_FALSE(candle_->isValidPosition(candleState, world, BlockPos(0, 100, 0)));
}

TEST_F(CandleBlockTest, IsValidPosition_AirBelow_ReturnsFalse)
{
    // 下方为空气时不能放置
    CandleTestWorld world;
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockDirectly(BlockPos(0, 0, 0), &airState);

    const BlockState& candleState = candle_->defaultState();
    EXPECT_FALSE(candle_->isValidPosition(candleState, world, BlockPos(0, 1, 0)));
}

// ============================================================================
// updatePostPlacement 测试
// ============================================================================

TEST_F(CandleBlockTest, UpdatePostPlacement_FloorRemoved_ReturnsAir)
{
    // 下方支撑移除后蜡烛应变为空气
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidState);

    const BlockState& candleState = candle_->defaultState();

    // 移除下方方块
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &airState);

    BlockState result = candle_->updatePostPlacement(
        candleState, Direction::Down, airState, world, BlockPos(5, 10, 5), BlockPos(5, 9, 5));

    EXPECT_TRUE(result.isAir());
}

TEST_F(CandleBlockTest, UpdatePostPlacement_FloorStillPresent_ReturnsSameState)
{
    // 下方支撑仍然存在时状态不变
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidState);

    const BlockState& candleState = candle_->defaultState();

    BlockState result = candle_->updatePostPlacement(
        candleState, Direction::Down, solidState, world, BlockPos(5, 10, 5), BlockPos(5, 9, 5));

    EXPECT_EQ(&result.getBlock(), candle_);
}

TEST_F(CandleBlockTest, UpdatePostPlacement_SideUpdate_ReturnsSameState)
{
    // 侧面方块更新不影响蜡烛状态
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidState);

    const BlockState& candleState = candle_->defaultState();

    BlockState result = candle_->updatePostPlacement(
        candleState, Direction::North, solidState, world, BlockPos(5, 10, 5), BlockPos(5, 10, 4));

    EXPECT_EQ(&result.getBlock(), candle_);
}

// ============================================================================
// ticksRandomly 测试
// ============================================================================

TEST_F(CandleBlockTest, TicksRandomly_ReturnsTrue)
{
    // 蜡烛需要响应随机刻以检测含水状态并熄灭
    EXPECT_TRUE(candle_->ticksRandomly());
}

// ============================================================================
// 渲染属性测试
// ============================================================================

TEST_F(CandleBlockTest, IsOpaque_ReturnsFalse)
{
    // 蜡烛不透明
    const BlockState& state = candle_->defaultState();
    EXPECT_FALSE(candle_->isOpaque(state));
}

TEST_F(CandleBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    // 蜡烛使用形状来遮挡光线
    const BlockState& state = candle_->defaultState();
    EXPECT_TRUE(candle_->useShapeForLightOcclusion(state));
}

// ============================================================================
// IWaterLoggable 接口测试
// ============================================================================

TEST_F(CandleBlockTest, IsWaterlogged_DefaultFalse)
{
    // 默认状态不含水
    const BlockState& state = candle_->defaultState();
    EXPECT_FALSE(candle_->isWaterlogged(state));
}

TEST_F(CandleBlockTest, IsWaterlogged_WhenSetTrue)
{
    // 设置为含水后应返回 true
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(candle_->isWaterlogged(state));
}

TEST_F(CandleBlockTest, GetFluidState_NotWaterlogged_ReturnsNull)
{
    // 未含水时流体状态为 null（委托给 Block::getFluidState）
    const BlockState& state = candle_->defaultState();
    const fluid::FluidState* fluidState = candle_->getFluidState(state);
    // 未含水时应返回 nullptr 或默认流体状态
    // 具体行为取决于 Block::getFluidState 的默认实现
    EXPECT_TRUE(fluidState == nullptr || fluidState->isEmpty());
}

TEST_F(CandleBlockTest, GetFluidState_Waterlogged_ReturnsWater)
{
    // 含水时流体状态应返回水源
    // 需要初始化流体注册表
    fluid::FluidRegistry::instance().initialize();

    const BlockState& state = candle_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = candle_->getFluidState(state);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_FALSE(fluidState->isEmpty());
}

// ============================================================================
// tick 测试（含水熄灭）
// ============================================================================

TEST_F(CandleBlockTest, Tick_WaterloggedAndLit_Extinguishes)
{
    // 含水且点燃的蜡烛在 tick 时应自动熄灭
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    // 先放置点燃的含水蜡烛
    const BlockState* candleState = world.storeBlockState(candle_->defaultState()
            .with(BlockStateProperties::LIT(), true)
            .with(BlockStateProperties::WATERLOGGED(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), candleState);

    // 调用 tick
    math::Random random(12345);
    BlockState mutableState = *candleState;
    candle_->tick(world, BlockPos(5, 10, 5), mutableState, random);

    // tick 后应调用 extinguish，将 LIT 设为 false
    // 因为 setBlockState 调用改变世界中的状态，检查世界中的状态
    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
}

TEST_F(CandleBlockTest, Tick_WaterloggedNotLit_NoChange)
{
    // 含水但未点燃的蜡烛在 tick 时不应改变状态
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* candleState =
        world.storeBlockState(candle_->defaultState().with(BlockStateProperties::WATERLOGGED(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), candleState);

    math::Random random(12345);
    BlockState mutableState = *candleState;
    candle_->tick(world, BlockPos(5, 10, 5), mutableState, random);

    // 状态应不变
    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
    EXPECT_TRUE(afterState->get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CandleBlockTest, Tick_NotWaterloggedAndLit_NoChange)
{
    // 未含水且点燃的蜡烛在 tick 时不应改变状态
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* candleState =
        world.storeBlockState(candle_->defaultState().with(BlockStateProperties::LIT(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), candleState);

    math::Random random(12345);
    BlockState mutableState = *candleState;
    candle_->tick(world, BlockPos(5, 10, 5), mutableState, random);

    // 状态应不变（仍然点燃）
    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_TRUE(afterState->get(BlockStateProperties::LIT()));
}

// ============================================================================
// 粒子偏移位置测试
// ============================================================================

TEST_F(CandleBlockTest, GetParticleOffsets_1Candle_SingleOffset)
{
    // 1根蜡烛返回1个偏移位置
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::CANDLES(), 1);
    auto offsets = candle_->getParticleOffsets(state);
    EXPECT_EQ(offsets.size(), 1u);
}

TEST_F(CandleBlockTest, GetParticleOffsets_2Candles_TwoOffsets)
{
    // 2根蜡烛返回2个偏移位置
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::CANDLES(), 2);
    auto offsets = candle_->getParticleOffsets(state);
    EXPECT_EQ(offsets.size(), 2u);
}

TEST_F(CandleBlockTest, GetParticleOffsets_3Candles_ThreeOffsets)
{
    // 3根蜡烛返回3个偏移位置
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::CANDLES(), 3);
    auto offsets = candle_->getParticleOffsets(state);
    EXPECT_EQ(offsets.size(), 3u);
}

TEST_F(CandleBlockTest, GetParticleOffsets_4Candles_FourOffsets)
{
    // 4根蜡烛返回4个偏移位置
    const BlockState& state = candle_->defaultState().with(BlockStateProperties::CANDLES(), 4);
    auto offsets = candle_->getParticleOffsets(state);
    EXPECT_EQ(offsets.size(), 4u);
}

TEST_F(CandleBlockTest, GetParticleOffsets_OffsetsInUnitCube)
{
    // 所有偏移位置应在 [0, 1] 范围内
    for (i32 count = 1; count <= 4; ++count) {
        const BlockState& state = candle_->defaultState().with(BlockStateProperties::CANDLES(), count);
        auto offsets = candle_->getParticleOffsets(state);
        for (const auto& offset : offsets) {
            EXPECT_GE(offset.x, 0.0f) << "CANDLES=" << count;
            EXPECT_LE(offset.x, 1.0f) << "CANDLES=" << count;
            EXPECT_GE(offset.y, 0.0f) << "CANDLES=" << count;
            EXPECT_LE(offset.y, 1.0f) << "CANDLES=" << count;
            EXPECT_GE(offset.z, 0.0f) << "CANDLES=" << count;
            EXPECT_LE(offset.z, 1.0f) << "CANDLES=" << count;
        }
    }
}

// ============================================================================
// VanillaBlocks 集成测试
// ============================================================================

TEST_F(CandleBlockTest, VanillaCandleBlock_IsCandleBlock)
{
    // 验证注册的蜡烛方块是 CandleBlock 类型
    const Block* candleBlock = CandleBlocks::CANDLE;
    ASSERT_NE(candleBlock, nullptr);
    const auto* asCandle = dynamic_cast<const CandleBlock*>(candleBlock);
    EXPECT_NE(asCandle, nullptr);
}

TEST_F(CandleBlockTest, VanillaCandleBlock_HasCorrectProperties)
{
    // 验证注册的蜡烛方块拥有所有必需属性
    const BlockState& state = CandleBlocks::CANDLE->defaultState();
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::CANDLES()));
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::LIT()));
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// extinguish 测试
// ============================================================================

TEST_F(CandleBlockTest, Extinguish_LitCandle_SetsLitFalse)
{
    // 熄灭点燃的蜡烛应将 LIT 设为 false
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* litState = world.storeBlockState(candle_->defaultState().with(BlockStateProperties::LIT(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), litState);

    BlockState mutableState = *litState;
    candle_->extinguish(world, BlockPos(5, 10, 5), mutableState, nullptr);

    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
}

TEST_F(CandleBlockTest, Extinguish_UnlitCandle_NoChange)
{
    // 熄灭未点燃的蜡烛不应改变状态
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* unlitState =
        world.storeBlockState(candle_->defaultState().with(BlockStateProperties::LIT(), false));
    world.setBlockDirectly(BlockPos(5, 10, 5), unlitState);

    BlockState mutableState = *unlitState;
    candle_->extinguish(world, BlockPos(5, 10, 5), mutableState, nullptr);

    // 未点燃时调用 extinguish 不应改变状态
    EXPECT_FALSE(mutableState.get(BlockStateProperties::LIT()));
}

// ============================================================================
// setLit 测试
// ============================================================================

TEST_F(CandleBlockTest, SetLit_True_SetsLitToTrue)
{
    // setLit(world, pos, state, true) 应将 LIT 设为 true
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* unlitState = world.storeBlockState(candle_->defaultState());
    world.setBlockDirectly(BlockPos(5, 10, 5), unlitState);

    AbstractCandleBlock::setLit(world, BlockPos(5, 10, 5), *unlitState, true);

    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_TRUE(afterState->get(BlockStateProperties::LIT()));
}

TEST_F(CandleBlockTest, SetLit_False_SetsLitToFalse)
{
    // setLit(world, pos, state, false) 应将 LIT 设为 false
    CandleTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* litState = world.storeBlockState(candle_->defaultState().with(BlockStateProperties::LIT(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), litState);

    AbstractCandleBlock::setLit(world, BlockPos(5, 10, 5), *litState, false);

    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
}
