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
 * @file ConcretePowderBlockTest.cpp
 * @brief ConcretePowderBlock 单元测试
 *
 * 测试混凝土粉末方块的固化行为：
 * - 注册为 ConcretePowderBlock（继承 FallingBlock）
 * - getConcreteBlock() 返回对应混凝土方块
 * - onEndFalling: 落地遇水固化
 * - updatePostPlacement: 邻居更新遇水固化
 * - touchesLiquid / canSolidify / shouldSolidify 辅助方法
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/ConcretePowderBlock.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 用于 ConcretePowderBlock 测试的 Mock World 实现
 *
 * 提供方块存储、流体存储、setBlockState 记录等功能。
 */
class ConcretePowderTestWorld final : public mc::test::BaseTestWorld {
public:
    ConcretePowderTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
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
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        m_blockChanges.push_back({BlockPos(x, y, z), state});
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // 优先检查显式设置的流体状态
        const auto fluidIt = m_fluids.find(packPos(x, y, z));
        if (fluidIt != m_fluids.end() && fluidIt->second != nullptr) {
            return fluidIt->second;
        }

        // 回退到方块的流体状态
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }

        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<ConcretePowderTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_UNUSED(entity);
        return EntityInstanceId(0);
    }

    // 测试辅助方法
    const std::vector<std::pair<BlockPos, const BlockState*>>& getBlockChanges() const { return m_blockChanges; }

    void clearRecords() { m_blockChanges.clear(); }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    std::vector<std::pair<BlockPos, const BlockState*>> m_blockChanges;
};

} // namespace

// ============================================================================
// 方块注册测试
// ============================================================================

class ConcretePowderBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

TEST_F(ConcretePowderBlockTest, WhiteConcretePowderIsRegistered)
{
    ASSERT_NE(VanillaBlocks::WHITE_CONCRETE_POWDER, nullptr);
}

TEST_F(ConcretePowderBlockTest, WhiteConcreteIsRegistered)
{
    ASSERT_NE(VanillaBlocks::WHITE_CONCRETE, nullptr);
}

TEST_F(ConcretePowderBlockTest, ConcretePowderIsConcretePowderBlock)
{
    // 验证混凝土粉末注册为 ConcretePowderBlock（继承自 FallingBlock）
    const Block* powder = VanillaBlocks::WHITE_CONCRETE_POWDER;
    ASSERT_NE(powder, nullptr);

    auto* concretePowder = dynamic_cast<const ConcretePowderBlock*>(powder);
    EXPECT_NE(concretePowder, nullptr);
}

TEST_F(ConcretePowderBlockTest, ConcretePowderIsAlsoFallingBlock)
{
    // 验证混凝土粉末也是下落方块
    const Block* powder = VanillaBlocks::WHITE_CONCRETE_POWDER;
    ASSERT_NE(powder, nullptr);

    auto* fallingBlock = dynamic_cast<const FallingBlock*>(powder);
    EXPECT_NE(fallingBlock, nullptr);
}

TEST_F(ConcretePowderBlockTest, GetConcreteBlockReturnsCorrespondingConcrete)
{
    // 验证 getConcreteBlock() 返回对应颜色的混凝土方块
    const Block* powder = VanillaBlocks::WHITE_CONCRETE_POWDER;
    ASSERT_NE(powder, nullptr);

    auto* concretePowder = dynamic_cast<const ConcretePowderBlock*>(powder);
    ASSERT_NE(concretePowder, nullptr);

    const Block* concrete = concretePowder->getConcreteBlock();
    ASSERT_NE(concrete, nullptr);
    EXPECT_EQ(concrete, VanillaBlocks::WHITE_CONCRETE);
}

TEST_F(ConcretePowderBlockTest, AllConcretePowdersAreConcretePowderBlocks)
{
    // 验证所有16种颜色的混凝土粉末都是 ConcretePowderBlock
    const Block* powders[] = {
        VanillaBlocks::WHITE_CONCRETE_POWDER,
        VanillaBlocks::ORANGE_CONCRETE_POWDER,
        VanillaBlocks::MAGENTA_CONCRETE_POWDER,
        VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER,
        VanillaBlocks::YELLOW_CONCRETE_POWDER,
        VanillaBlocks::LIME_CONCRETE_POWDER,
        VanillaBlocks::PINK_CONCRETE_POWDER,
        VanillaBlocks::GRAY_CONCRETE_POWDER,
        VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER,
        VanillaBlocks::CYAN_CONCRETE_POWDER,
        VanillaBlocks::PURPLE_CONCRETE_POWDER,
        VanillaBlocks::BLUE_CONCRETE_POWDER,
        VanillaBlocks::BROWN_CONCRETE_POWDER,
        VanillaBlocks::GREEN_CONCRETE_POWDER,
        VanillaBlocks::RED_CONCRETE_POWDER,
        VanillaBlocks::BLACK_CONCRETE_POWDER,
    };

    for (const Block* powder : powders) {
        ASSERT_NE(powder, nullptr);
        auto* concretePowder = dynamic_cast<const ConcretePowderBlock*>(powder);
        EXPECT_NE(concretePowder, nullptr)
            << "Block should be ConcretePowderBlock: " << powder->blockLocation().toString();
    }
}

// ============================================================================
// 固化行为测试
// ============================================================================

TEST_F(ConcretePowderBlockTest, OnEndFallingSolidifiesInWater)
{
    // 测试：混凝土粉末落地时，如果落地点接触水，应固化为混凝土
    ConcretePowderTestWorld world;

    // 放置白色混凝土粉末
    BlockPos powderPos(0, 0, 0);
    const BlockState& powderState = VanillaBlocks::WHITE_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 在旁边放置水源
    BlockPos waterPos(1, 0, 0);
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    world.setFluidDirectly(waterPos, waterState);
    const BlockState& waterBlockState = VanillaBlocks::WATER->defaultState();
    world.setBlockDirectly(waterPos, &waterBlockState);

    // 获取混凝土粉末方块
    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::WHITE_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    // 创建一个模拟的 FallingBlockEntity（用 nullptr，onEndFalling 不使用实体）
    // 模拟落地：调用 onEndFalling
    // 创建 FallingBlockEntity 用于 onEndFalling 回调
    auto entity = std::make_unique<entity::FallingBlockEntity>();
    entity->setBlockId(powderState.blockId());

    const BlockState& hitState = VanillaBlocks::AIR->defaultState();
    concretePowder->onEndFalling(world, powderPos, powderState, hitState, *entity);

    // 验证粉末位置已变为混凝土
    const BlockState* finalState = world.getBlockState(powderPos.x, powderPos.y, powderPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::WHITE_CONCRETE);
}

TEST_F(ConcretePowderBlockTest, OnEndFallingNoSolidifyWithoutWater)
{
    // 测试：混凝土粉末落地时，如果周围没有水，不应固化
    ConcretePowderTestWorld world;

    BlockPos powderPos(0, 0, 0);
    const BlockState& powderState = VanillaBlocks::WHITE_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 不放置任何水源

    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::WHITE_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    // 创建 FallingBlockEntity 用于 onEndFalling 回调
    auto entity = std::make_unique<entity::FallingBlockEntity>();
    entity->setBlockId(powderState.blockId());

    const BlockState& hitState = VanillaBlocks::AIR->defaultState();
    concretePowder->onEndFalling(world, powderPos, powderState, hitState, *entity);

    // 验证粉末位置不变（仍然是混凝土粉末）
    const BlockState* finalState = world.getBlockState(powderPos.x, powderPos.y, powderPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::WHITE_CONCRETE_POWDER);
}

TEST_F(ConcretePowderBlockTest, OnEndFallingSolidifiesWithWaterBelow)
{
    // 测试：混凝土粉末落地时，下方有水应固化
    ConcretePowderTestWorld world;

    BlockPos powderPos(0, 1, 0);
    const BlockState& powderState = VanillaBlocks::RED_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 在下方放置水
    BlockPos waterPos(0, 0, 0);
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    world.setFluidDirectly(waterPos, waterState);
    world.setBlockDirectly(waterPos, &VanillaBlocks::WATER->defaultState());

    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::RED_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    // 创建 FallingBlockEntity 用于 onEndFalling 回调
    auto entity = std::make_unique<entity::FallingBlockEntity>();
    entity->setBlockId(powderState.blockId());

    const BlockState& hitState = VanillaBlocks::AIR->defaultState();
    concretePowder->onEndFalling(world, powderPos, powderState, hitState, *entity);

    const BlockState* finalState = world.getBlockState(powderPos.x, powderPos.y, powderPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::RED_CONCRETE);
}

TEST_F(ConcretePowderBlockTest, OnEndFallingSolidifiesWithWaterAbove)
{
    // 测试：混凝土粉末落地时，上方有水应固化
    ConcretePowderTestWorld world;

    BlockPos powderPos(0, 0, 0);
    const BlockState& powderState = VanillaBlocks::BLUE_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 在上方放置水
    BlockPos waterPos(0, 1, 0);
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    world.setFluidDirectly(waterPos, waterState);
    world.setBlockDirectly(waterPos, &VanillaBlocks::WATER->defaultState());

    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::BLUE_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    // 创建 FallingBlockEntity 用于 onEndFalling 回调
    auto entity = std::make_unique<entity::FallingBlockEntity>();
    entity->setBlockId(powderState.blockId());

    const BlockState& hitState = VanillaBlocks::AIR->defaultState();
    concretePowder->onEndFalling(world, powderPos, powderState, hitState, *entity);

    const BlockState* finalState = world.getBlockState(powderPos.x, powderPos.y, powderPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::BLUE_CONCRETE);
}

// ============================================================================
// updatePostPlacement 固化测试
// ============================================================================

TEST_F(ConcretePowderBlockTest, UpdatePostPlacementSolidifiesWithWater)
{
    // 测试：邻居更新时接触水，应返回混凝土状态
    ConcretePowderTestWorld world;

    BlockPos powderPos(0, 0, 0);
    const BlockState& powderState = VanillaBlocks::WHITE_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 在旁边放置水
    BlockPos waterPos(1, 0, 0);
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState* waterState = &waterFluid->defaultState();
    world.setFluidDirectly(waterPos, waterState);
    world.setBlockDirectly(waterPos, &VanillaBlocks::WATER->defaultState());

    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::WHITE_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    // 模拟从水源方向的邻居更新
    const BlockState& waterBlockState = VanillaBlocks::WATER->defaultState();
    BlockState updatedState =
        concretePowder->updatePostPlacement(powderState, Direction::East, waterBlockState, world, powderPos, waterPos);

    // 验证返回了混凝土状态
    EXPECT_EQ(&updatedState.getBlock(), VanillaBlocks::WHITE_CONCRETE);
}

TEST_F(ConcretePowderBlockTest, UpdatePostPlacementNoSolidifyWithoutWater)
{
    // 测试：邻居更新时没有水，应返回原始粉末状态
    ConcretePowderTestWorld world;

    BlockPos powderPos(0, 0, 0);
    const BlockState& powderState = VanillaBlocks::WHITE_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 旁边放石头（不是水）
    BlockPos stonePos(1, 0, 0);
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    world.setBlockDirectly(stonePos, &stoneState);

    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::WHITE_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    BlockState updatedState =
        concretePowder->updatePostPlacement(powderState, Direction::East, stoneState, world, powderPos, stonePos);

    // 验证返回的仍是粉末状态（未固化）
    EXPECT_EQ(&updatedState.getBlock(), VanillaBlocks::WHITE_CONCRETE_POWDER);
}

// ============================================================================
// 各颜色对应关系测试
// ============================================================================

TEST_F(ConcretePowderBlockTest, EachPowderMapsToCorrectConcrete)
{
    // 验证每种颜色的混凝土粉末都指向正确颜色的混凝土
    struct ColorPair {
        const Block* powder;
        const Block* concrete;
        const char* name;
    };

    ColorPair pairs[] = {
        {VanillaBlocks::WHITE_CONCRETE_POWDER, VanillaBlocks::WHITE_CONCRETE, "white"},
        {VanillaBlocks::ORANGE_CONCRETE_POWDER, VanillaBlocks::ORANGE_CONCRETE, "orange"},
        {VanillaBlocks::MAGENTA_CONCRETE_POWDER, VanillaBlocks::MAGENTA_CONCRETE, "magenta"},
        {VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER, VanillaBlocks::LIGHT_BLUE_CONCRETE, "light_blue"},
        {VanillaBlocks::YELLOW_CONCRETE_POWDER, VanillaBlocks::YELLOW_CONCRETE, "yellow"},
        {VanillaBlocks::LIME_CONCRETE_POWDER, VanillaBlocks::LIME_CONCRETE, "lime"},
        {VanillaBlocks::PINK_CONCRETE_POWDER, VanillaBlocks::PINK_CONCRETE, "pink"},
        {VanillaBlocks::GRAY_CONCRETE_POWDER, VanillaBlocks::GRAY_CONCRETE, "gray"},
        {VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER, VanillaBlocks::LIGHT_GRAY_CONCRETE, "light_gray"},
        {VanillaBlocks::CYAN_CONCRETE_POWDER, VanillaBlocks::CYAN_CONCRETE, "cyan"},
        {VanillaBlocks::PURPLE_CONCRETE_POWDER, VanillaBlocks::PURPLE_CONCRETE, "purple"},
        {VanillaBlocks::BLUE_CONCRETE_POWDER, VanillaBlocks::BLUE_CONCRETE, "blue"},
        {VanillaBlocks::BROWN_CONCRETE_POWDER, VanillaBlocks::BROWN_CONCRETE, "brown"},
        {VanillaBlocks::GREEN_CONCRETE_POWDER, VanillaBlocks::GREEN_CONCRETE, "green"},
        {VanillaBlocks::RED_CONCRETE_POWDER, VanillaBlocks::RED_CONCRETE, "red"},
        {VanillaBlocks::BLACK_CONCRETE_POWDER, VanillaBlocks::BLACK_CONCRETE, "black"},
    };

    for (const auto& pair : pairs) {
        ASSERT_NE(pair.powder, nullptr) << "Powder block should exist: " << pair.name;
        ASSERT_NE(pair.concrete, nullptr) << "Concrete block should exist: " << pair.name;

        auto* concretePowder = dynamic_cast<const ConcretePowderBlock*>(pair.powder);
        ASSERT_NE(concretePowder, nullptr) << "Powder should be ConcretePowderBlock: " << pair.name;

        EXPECT_EQ(concretePowder->getConcreteBlock(), pair.concrete)
            << "Concrete powder " << pair.name << " should map to corresponding concrete";
    }
}

// ============================================================================
// Lava 不触发固化测试
// ============================================================================

TEST_F(ConcretePowderBlockTest, OnEndFallingNoSolidifyWithLava)
{
    // 测试：岩浆不应触发混凝土粉末固化（仅水可以）
    ConcretePowderTestWorld world;

    BlockPos powderPos(0, 0, 0);
    const BlockState& powderState = VanillaBlocks::WHITE_CONCRETE_POWDER->defaultState();
    world.setBlockDirectly(powderPos, &powderState);

    // 在旁边放置岩浆
    BlockPos lavaPos(1, 0, 0);
    fluid::Fluid* lavaFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID);
    ASSERT_NE(lavaFluid, nullptr);
    const fluid::FluidState* lavaState = &lavaFluid->defaultState();
    world.setFluidDirectly(lavaPos, lavaState);
    world.setBlockDirectly(lavaPos, &VanillaBlocks::LAVA->defaultState());

    auto* concretePowder = dynamic_cast<ConcretePowderBlock*>(VanillaBlocks::WHITE_CONCRETE_POWDER);
    ASSERT_NE(concretePowder, nullptr);

    // 创建 FallingBlockEntity 用于 onEndFalling 回调
    auto entity = std::make_unique<entity::FallingBlockEntity>();
    entity->setBlockId(powderState.blockId());

    const BlockState& hitState = VanillaBlocks::AIR->defaultState();
    concretePowder->onEndFalling(world, powderPos, powderState, hitState, *entity);

    // 验证粉末位置不变（岩浆不应触发固化）
    const BlockState* finalState = world.getBlockState(powderPos.x, powderPos.y, powderPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(&finalState->getBlock(), VanillaBlocks::WHITE_CONCRETE_POWDER);
}
