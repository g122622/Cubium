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
 * @file CandleCakeBlockTest.cpp
 * @brief CandleCakeBlock 单元测试
 *
 * 测试内容：
 * 1. 方块状态属性（LIT）正确性
 * 2. 默认状态值（未点燃）
 * 3. 光照等级（点燃=3，未点燃=0）
 * 4. canLight 静态方法
 * 5. 碰撞形状不为空
 * 6. isValidPosition 需要固体支撑
 * 7. updatePostPlacement 下方支撑丢失时变为空气
 * 8. 粒子偏移位置（固定1个偏移）
 * 9. 比较器输出（14）
 * 10. 渲染属性
 * 11. 关联蜡烛方块
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/decorative/CandleBlock.hpp"
#include "common/world/block/blocks/functional/CandleCakeBlock.hpp"
#include "common/world/block/registry/CandleBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

namespace {

// ============================================================================
// 测试用世界
// ============================================================================

class CandleCakeTestWorld final : public mc::test::BaseTestWorld {
public:
    CandleCakeTestWorld()
    {
        VanillaBlocks::initialize();
        m_airState = &VanillaBlocks::AIR->defaultState();
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
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
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

class CandleCakeBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        candleCake_ = dynamic_cast<CandleCakeBlock*>(CandleBlocks::CANDLE_CAKE);
        ASSERT_NE(candleCake_, nullptr);
    }

    CandleCakeBlock* candleCake_ = nullptr;
};

TEST_F(CandleCakeBlockTest, DefaultState_NotLit)
{
    // 默认状态：未点燃
    const BlockState& state = candleCake_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::LIT()));
}

TEST_F(CandleCakeBlockTest, DefaultState_HasOnlyLitProperty)
{
    // 蜡烛蛋糕只有 LIT 属性（没有 CANDLES 和 WATERLOGGED）
    const BlockState& state = candleCake_->defaultState();
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::LIT()));
    // 蜡烛蛋糕不应该有 CANDLES 属性
    EXPECT_FALSE(state.hasProperty(BlockStateProperties::CANDLES()));
    // 蜡烛蛋糕不应该有 WATERLOGGED 属性
    EXPECT_FALSE(state.hasProperty(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CandleCakeBlockTest, StateProperties_LitToggle)
{
    // LIT 属性可以切换
    const BlockState& state = candleCake_->defaultState();
    BlockState lit = state.with(BlockStateProperties::LIT(), true);
    EXPECT_TRUE(lit.get(BlockStateProperties::LIT()));

    BlockState unlit = lit.with(BlockStateProperties::LIT(), false);
    EXPECT_FALSE(unlit.get(BlockStateProperties::LIT()));
}

// ============================================================================
// 光照等级测试
// ============================================================================

TEST_F(CandleCakeBlockTest, GetLightLevel_Unlit_Returns0)
{
    // 未点燃时光照为0
    const BlockState& state = candleCake_->defaultState();
    EXPECT_EQ(candleCake_->getLightLevel(state), 0);
}

TEST_F(CandleCakeBlockTest, GetLightLevel_Lit_Returns3)
{
    // 点燃时光照为3（与1根蜡烛相同）
    const BlockState& state = candleCake_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_EQ(candleCake_->getLightLevel(state), 3);
}

// ============================================================================
// canLight 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, CanLight_Unlit_ReturnsTrue)
{
    // 未点燃时可以点燃
    const BlockState& state = candleCake_->defaultState();
    EXPECT_TRUE(CandleCakeBlock::canLight(state));
}

TEST_F(CandleCakeBlockTest, CanLight_Lit_ReturnsFalse)
{
    // 已点燃时不能再次点燃
    const BlockState& state = candleCake_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_FALSE(CandleCakeBlock::canLight(state));
}

TEST_F(CandleCakeBlockTest, CanLight_BlockWithoutLitProperty_ReturnsFalse)
{
    // 没有 LIT 属性的方块（如石头）应返回 false
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(CandleCakeBlock::canLight(stoneState));
}

// ============================================================================
// canBeLit 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, CanBeLit_Unlit_ReturnsTrue)
{
    // 未点燃时可以点燃
    const BlockState& state = candleCake_->defaultState();
    EXPECT_TRUE(candleCake_->canBeLit(state));
}

TEST_F(CandleCakeBlockTest, CanBeLit_Lit_ReturnsFalse)
{
    // 已点燃时不能再次点燃
    const BlockState& state = candleCake_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_FALSE(candleCake_->canBeLit(state));
}

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST_F(CandleCakeBlockTest, GetShape_NonEmpty)
{
    // 蜡烛蛋糕的碰撞形状不应为空
    const BlockState& state = candleCake_->defaultState();
    const CollisionShape& shape = candleCake_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CandleCakeBlockTest, GetShape_SameForLitAndUnlit)
{
    // 点燃和未点燃的碰撞形状应相同
    const BlockState& unlitState = candleCake_->defaultState();
    const BlockState& litState = candleCake_->defaultState().with(BlockStateProperties::LIT(), true);

    // 碰撞形状不依赖于 LIT 属性
    const CollisionShape& unlitShape = candleCake_->getShape(unlitState);
    const CollisionShape& litShape = candleCake_->getShape(litState);
    // 形状应该相同（不是同一对象的判断，而是值相等）
    // 由于形状存储在 m_shape 成员中，两个状态应返回相同的引用
    EXPECT_EQ(&unlitShape, &litShape);
}

// ============================================================================
// isValidPosition 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, IsValidPosition_SolidBelow_ReturnsTrue)
{
    // 下方有固体方块时可以放置
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(0, 0, 0), &solidState);

    const BlockState& state = candleCake_->defaultState();
    EXPECT_TRUE(candleCake_->isValidPosition(state, world, BlockPos(0, 1, 0)));
}

TEST_F(CandleCakeBlockTest, IsValidPosition_NoBlockBelow_ReturnsFalse)
{
    // 下方无方块时不能放置
    CandleCakeTestWorld world;
    const BlockState& state = candleCake_->defaultState();
    EXPECT_FALSE(candleCake_->isValidPosition(state, world, BlockPos(0, 100, 0)));
}

TEST_F(CandleCakeBlockTest, IsValidPosition_AirBelow_ReturnsFalse)
{
    // 下方为空气时不能放置
    CandleCakeTestWorld world;
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockDirectly(BlockPos(0, 0, 0), &airState);

    const BlockState& state = candleCake_->defaultState();
    EXPECT_FALSE(candleCake_->isValidPosition(state, world, BlockPos(0, 1, 0)));
}

// ============================================================================
// updatePostPlacement 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, UpdatePostPlacement_FloorRemoved_ReturnsAir)
{
    // 下方支撑移除后蜡烛蛋糕应变为空气
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidState);

    const BlockState& cakeState = candleCake_->defaultState();

    // 移除下方方块
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &airState);

    BlockState result = candleCake_->updatePostPlacement(
        cakeState, Direction::Down, airState, world, BlockPos(5, 10, 5), BlockPos(5, 9, 5));

    EXPECT_TRUE(result.isAir());
}

TEST_F(CandleCakeBlockTest, UpdatePostPlacement_FloorStillPresent_ReturnsSameState)
{
    // 下方支撑仍然存在时状态不变
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidState);

    const BlockState& cakeState = candleCake_->defaultState();

    BlockState result = candleCake_->updatePostPlacement(
        cakeState, Direction::Down, solidState, world, BlockPos(5, 10, 5), BlockPos(5, 9, 5));

    EXPECT_EQ(&result.getBlock(), candleCake_);
}

TEST_F(CandleCakeBlockTest, UpdatePostPlacement_SideUpdate_ReturnsSameState)
{
    // 侧面方块更新不影响蜡烛蛋糕状态
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    const BlockState& solidState = solidBlock.defaultState();
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidState);

    const BlockState& cakeState = candleCake_->defaultState();

    BlockState result = candleCake_->updatePostPlacement(
        cakeState, Direction::North, solidState, world, BlockPos(5, 10, 5), BlockPos(5, 10, 4));

    EXPECT_EQ(&result.getBlock(), candleCake_);
}

// ============================================================================
// 粒子偏移位置测试
// ============================================================================

TEST_F(CandleCakeBlockTest, GetParticleOffsets_SingleOffset)
{
    // 蜡烛蛋糕只有1根蜡烛，返回1个偏移位置
    const BlockState& state = candleCake_->defaultState();
    auto offsets = candleCake_->getParticleOffsets(state);
    EXPECT_EQ(offsets.size(), 1u);
}

TEST_F(CandleCakeBlockTest, GetParticleOffsets_OffsetInUnitCube)
{
    // 偏移位置应在 [0, 1] 范围内
    const BlockState& state = candleCake_->defaultState();
    auto offsets = candleCake_->getParticleOffsets(state);
    for (const auto& offset : offsets) {
        EXPECT_GE(offset.x, 0.0f);
        EXPECT_LE(offset.x, 1.0f);
        EXPECT_GE(offset.y, 0.0f);
        EXPECT_LE(offset.y, 1.0f);
        EXPECT_GE(offset.z, 0.0f);
        EXPECT_LE(offset.z, 1.0f);
    }
}

// ============================================================================
// 比较器输出测试
// ============================================================================

TEST_F(CandleCakeBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    // 蜡烛蛋糕有比较器输出覆写
    const BlockState& state = candleCake_->defaultState();
    EXPECT_TRUE(candleCake_->hasComparatorInputOverride(state));
}

TEST_F(CandleCakeBlockTest, GetComparatorInputOverride_Returns14)
{
    // 蜡烛蛋糕的比较器输出始终为14
    CandleCakeTestWorld world;
    const BlockState& state = candleCake_->defaultState();
    EXPECT_EQ(candleCake_->getComparatorInputOverride(state, world, BlockPos(0, 0, 0)), 14);
}

TEST_F(CandleCakeBlockTest, GetComparatorInputOverride_LitAlsoReturns14)
{
    // 点燃状态的蜡烛蛋糕比较器输出也是14
    CandleCakeTestWorld world;
    const BlockState& state = candleCake_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_EQ(candleCake_->getComparatorInputOverride(state, world, BlockPos(0, 0, 0)), 14);
}

// ============================================================================
// 渲染属性测试
// ============================================================================

TEST_F(CandleCakeBlockTest, IsOpaque_ReturnsFalse)
{
    // 蜡烛蛋糕不透明
    const BlockState& state = candleCake_->defaultState();
    EXPECT_FALSE(candleCake_->isOpaque(state));
}

// ============================================================================
// 关联蜡烛方块测试
// ============================================================================

TEST_F(CandleCakeBlockTest, GetCandleBlock_NotNull)
{
    // 蜡烛蛋糕应有关联的蜡烛方块
    Block* candleBlock = candleCake_->getCandleBlock();
    EXPECT_NE(candleBlock, nullptr);
}

TEST_F(CandleCakeBlockTest, GetCandleBlock_IsCandleBlock)
{
    // 关联的蜡烛方块应该是 CandleBlock 类型
    Block* candleBlock = candleCake_->getCandleBlock();
    ASSERT_NE(candleBlock, nullptr);
    const auto* asCandle = dynamic_cast<const CandleBlock*>(candleBlock);
    EXPECT_NE(asCandle, nullptr);
}

// ============================================================================
// VanillaBlocks 集成测试
// ============================================================================

TEST_F(CandleCakeBlockTest, VanillaCandleCakeBlock_IsCandleCakeBlock)
{
    // 验证注册的蜡烛蛋糕方块是 CandleCakeBlock 类型
    const Block* cakeBlock = CandleBlocks::CANDLE_CAKE;
    ASSERT_NE(cakeBlock, nullptr);
    const auto* asCake = dynamic_cast<const CandleCakeBlock*>(cakeBlock);
    EXPECT_NE(asCake, nullptr);
}

TEST_F(CandleCakeBlockTest, VanillaCandleCakeBlock_HasCorrectProperties)
{
    // 验证注册的蜡烛蛋糕方块拥有 LIT 属性
    const BlockState& state = CandleBlocks::CANDLE_CAKE->defaultState();
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::LIT()));
    // 不应有 CANDLES 和 WATERLOGGED
    EXPECT_FALSE(state.hasProperty(BlockStateProperties::CANDLES()));
    EXPECT_FALSE(state.hasProperty(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 各颜色蜡烛蛋糕集成测试
// ============================================================================

TEST_F(CandleCakeBlockTest, AllCandleCakeVariants_HaveLitProperty)
{
    // 所有蜡烛蛋糕变体都应有 LIT 属性
    std::vector<Block*> candleCakes = {
        CandleBlocks::CANDLE_CAKE,
        CandleBlocks::WHITE_CANDLE_CAKE,
        CandleBlocks::ORANGE_CANDLE_CAKE,
        CandleBlocks::MAGENTA_CANDLE_CAKE,
        CandleBlocks::LIGHT_BLUE_CANDLE_CAKE,
        CandleBlocks::YELLOW_CANDLE_CAKE,
        CandleBlocks::LIME_CANDLE_CAKE,
        CandleBlocks::PINK_CANDLE_CAKE,
        CandleBlocks::GRAY_CANDLE_CAKE,
        CandleBlocks::LIGHT_GRAY_CANDLE_CAKE,
        CandleBlocks::CYAN_CANDLE_CAKE,
        CandleBlocks::PURPLE_CANDLE_CAKE,
        CandleBlocks::BLUE_CANDLE_CAKE,
        CandleBlocks::BROWN_CANDLE_CAKE,
        CandleBlocks::GREEN_CANDLE_CAKE,
        CandleBlocks::RED_CANDLE_CAKE,
        CandleBlocks::BLACK_CANDLE_CAKE,
    };

    for (Block* block : candleCakes) {
        ASSERT_NE(block, nullptr) << "Candle cake block should not be null";
        const BlockState& state = block->defaultState();
        EXPECT_TRUE(state.hasProperty(BlockStateProperties::LIT())) << "Candle cake should have LIT property";
        EXPECT_FALSE(state.hasProperty(BlockStateProperties::CANDLES()))
            << "Candle cake should NOT have CANDLES property";
        EXPECT_FALSE(state.hasProperty(BlockStateProperties::WATERLOGGED()))
            << "Candle cake should NOT have WATERLOGGED property";
    }
}

// ============================================================================
// extinguish 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, Extinguish_LitCandleCake_SetsLitFalse)
{
    // 熄灭点燃的蜡烛蛋糕应将 LIT 设为 false
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* litState =
        world.storeBlockState(candleCake_->defaultState().with(BlockStateProperties::LIT(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), litState);

    BlockState mutableState = *litState;
    candleCake_->extinguish(world, BlockPos(5, 10, 5), mutableState, nullptr);

    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
}

TEST_F(CandleCakeBlockTest, Extinguish_UnlitCandleCake_NoChange)
{
    // 熄灭未点燃的蜡烛蛋糕不应改变状态
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* unlitState =
        world.storeBlockState(candleCake_->defaultState().with(BlockStateProperties::LIT(), false));
    world.setBlockDirectly(BlockPos(5, 10, 5), unlitState);

    BlockState mutableState = *unlitState;
    candleCake_->extinguish(world, BlockPos(5, 10, 5), mutableState, nullptr);

    // 未点燃时调用 extinguish 不应改变状态
    EXPECT_FALSE(mutableState.get(BlockStateProperties::LIT()));
}

// ============================================================================
// setLit 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, SetLit_True_SetsLitToTrue)
{
    // setLit(world, pos, state, true) 应将 LIT 设为 true
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* unlitState = world.storeBlockState(candleCake_->defaultState());
    world.setBlockDirectly(BlockPos(5, 10, 5), unlitState);

    AbstractCandleBlock::setLit(world, BlockPos(5, 10, 5), *unlitState, true);

    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_TRUE(afterState->get(BlockStateProperties::LIT()));
}

TEST_F(CandleCakeBlockTest, SetLit_False_SetsLitToFalse)
{
    // setLit(world, pos, state, false) 应将 LIT 设为 false
    CandleCakeTestWorld world;
    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* litState =
        world.storeBlockState(candleCake_->defaultState().with(BlockStateProperties::LIT(), true));
    world.setBlockDirectly(BlockPos(5, 10, 5), litState);

    AbstractCandleBlock::setLit(world, BlockPos(5, 10, 5), *litState, false);

    const BlockState* afterState = world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
}

// ============================================================================
// isLit 测试
// ============================================================================

TEST_F(CandleCakeBlockTest, IsLit_Unlit_ReturnsFalse)
{
    const BlockState& state = candleCake_->defaultState();
    EXPECT_FALSE(AbstractCandleBlock::isLit(state));
}

TEST_F(CandleCakeBlockTest, IsLit_Lit_ReturnsTrue)
{
    const BlockState& state = candleCake_->defaultState().with(BlockStateProperties::LIT(), true);
    EXPECT_TRUE(AbstractCandleBlock::isLit(state));
}

// ============================================================================
// onBlockActivated 食物喂养测试
// ============================================================================

namespace {

/**
 * @brief 支持 playSound 和 Player 交互的蜡烛蛋糕测试世界
 */
class CandleCakeInteractionWorld final : public mc::test::BaseTestWorld {
public:
    CandleCakeInteractionWorld()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        m_airState = &VanillaBlocks::AIR->defaultState();
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
        if (state == nullptr) {
            m_blocks.erase(packPos(x, y, z));
        } else {
            // 存储副本，避免悬空指针
            m_ownedStates[packPos(x, y, z)] = std::make_unique<BlockState>(*state);
            m_blocks[packPos(x, y, z)] = m_ownedStates[packPos(x, y, z)].get();
        }
        return true;
    }

    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos.x, pos.y, pos.z, stored);
    }

    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        (void)entity;
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("CandleCakeInteractionWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("CandleCakeInteractionWorld::tickManager not implemented");
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }

    void setSeed(u64 seed) { m_seed = seed; }

    struct SoundRecord {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    void clearSounds() { m_sounds.clear(); }

private:
    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unordered_map<i64, std::unique_ptr<BlockState>> m_ownedStates;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    std::vector<SoundRecord> m_sounds;
    const BlockState* m_airState;
    EntityInstanceId m_lastEntityId = 0;
    u64 m_seed = 12345;
};

} // anonymous namespace

class CandleCakeBlockInteractionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    CandleCakeInteractionWorld m_world;
};

TEST_F(CandleCakeBlockInteractionTest, EatCake_IncreasesFoodLevel)
{
    // 放置蜡烛蛋糕
    ASSERT_NE(CandleBlocks::CANDLE_CAKE, nullptr);
    auto* cakeBlock = dynamic_cast<CandleCakeBlock*>(CandleBlocks::CANDLE_CAKE);
    ASSERT_NE(cakeBlock, nullptr);

    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    m_world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* cakeState = m_world.storeBlockState(cakeBlock->defaultState());
    m_world.setBlockDirectly(BlockPos(5, 10, 5), cakeState);

    // 创建生存模式玩家（默认饥饿值20，canEat=false）
    // 需要降低饥饿值才能 canEat
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    player.foodStats().setFoodLevel(18); // 低于20，canEat(true) 返回 true

    // 点击蛋糕下半部分（y <= 0.5）→ 不触发熄灭，触发吃蛋糕
    // hitPosition = BlockPos(5,10,5) + y=0.3 → hitY = 0.3 < 0.5 → 不熄灭
    BlockRaycastResult hitResult =
        BlockRaycastResult::hit(Vector3(5.5f, 10.3f, 5.5f), BlockPos(5, 10, 5), Direction::Up, 1.0f);

    const BlockState& state = cakeBlock->defaultState();
    auto result = cakeBlock->onBlockActivated(state, m_world, BlockPos(5, 10, 5), player, Hand::MainHand, hitResult);

    // 应该成功吃蛋糕
    EXPECT_EQ(result, ActionResultType::Success);

    // 验证饥饿值增加了 2
    EXPECT_EQ(player.foodStats().foodLevel(), 20); // 18 + 2 = 20
}

TEST_F(CandleCakeBlockInteractionTest, EatCake_FullHunger_ReturnsPass)
{
    // 放置蜡烛蛋糕
    ASSERT_NE(CandleBlocks::CANDLE_CAKE, nullptr);
    auto* cakeBlock = dynamic_cast<CandleCakeBlock*>(CandleBlocks::CANDLE_CAKE);
    ASSERT_NE(cakeBlock, nullptr);

    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    m_world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* cakeState = m_world.storeBlockState(cakeBlock->defaultState());
    m_world.setBlockDirectly(BlockPos(5, 10, 5), cakeState);

    // 创建满饥饿值的生存模式玩家 → canEat(false) 返回 false
    Player player(EntityInstanceId(2), "TestPlayer2", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);
    // 默认 foodLevel=20，canEat(false) = false

    BlockRaycastResult hitResult =
        BlockRaycastResult::hit(Vector3(5.5f, 10.3f, 5.5f), BlockPos(5, 10, 5), Direction::Up, 1.0f);

    const BlockState& state = cakeBlock->defaultState();
    auto result = cakeBlock->onBlockActivated(state, m_world, BlockPos(5, 10, 5), player, Hand::MainHand, hitResult);

    // 满饥饿值 → Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(CandleCakeBlockInteractionTest, ExtinguishLitCandle_EmptyHandUpperHalf)
{
    // 放置已点燃的蜡烛蛋糕
    ASSERT_NE(CandleBlocks::CANDLE_CAKE, nullptr);
    auto* cakeBlock = dynamic_cast<CandleCakeBlock*>(CandleBlocks::CANDLE_CAKE);
    ASSERT_NE(cakeBlock, nullptr);

    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    m_world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* litState =
        m_world.storeBlockState(cakeBlock->defaultState().with(BlockStateProperties::LIT(), true));
    m_world.setBlockDirectly(BlockPos(5, 10, 5), litState);

    // 创建生存模式玩家
    Player player(EntityInstanceId(3), "TestPlayer3", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    // 点击蜡烛上半部分（y > 0.5）→ 熄灭
    // hitPosition = BlockPos(5,10,5) + y=0.7 → hitY = 0.7 > 0.5 → 熄灭
    BlockRaycastResult hitResult =
        BlockRaycastResult::hit(Vector3(5.5f, 10.7f, 5.5f), BlockPos(5, 10, 5), Direction::Up, 1.0f);

    const BlockState& state = cakeBlock->defaultState().with(BlockStateProperties::LIT(), true);
    auto result = cakeBlock->onBlockActivated(state, m_world, BlockPos(5, 10, 5), player, Hand::MainHand, hitResult);

    // 应该成功熄灭
    EXPECT_EQ(result, ActionResultType::Success);

    // 验证蜡烛蛋糕被熄灭
    const BlockState* afterState = m_world.getBlockState(5, 10, 5);
    ASSERT_NE(afterState, nullptr);
    EXPECT_FALSE(afterState->get(BlockStateProperties::LIT()));
}

TEST_F(CandleCakeBlockInteractionTest, CreativeMode_CannotEatCake)
{
    // 放置蜡烛蛋糕
    ASSERT_NE(CandleBlocks::CANDLE_CAKE, nullptr);
    auto* cakeBlock = dynamic_cast<CandleCakeBlock*>(CandleBlocks::CANDLE_CAKE);
    ASSERT_NE(cakeBlock, nullptr);

    TestSolidBlock solidBlock(BlockProperties(Material::ROCK).hardness(1.5f));
    m_world.setBlockDirectly(BlockPos(5, 9, 5), &solidBlock.defaultState());

    const BlockState* cakeState = m_world.storeBlockState(cakeBlock->defaultState());
    m_world.setBlockDirectly(BlockPos(5, 10, 5), cakeState);

    // 创建创造模式玩家 → canEat(false) 返回 false
    Player player(EntityInstanceId(4), "TestPlayer4", mc::test::testEcsRegistry());
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    BlockRaycastResult hitResult =
        BlockRaycastResult::hit(Vector3(5.5f, 10.3f, 5.5f), BlockPos(5, 10, 5), Direction::Up, 1.0f);

    const BlockState& state = cakeBlock->defaultState();
    auto result = cakeBlock->onBlockActivated(state, m_world, BlockPos(5, 10, 5), player, Hand::MainHand, hitResult);

    // 创造模式不能吃蛋糕 → Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}
