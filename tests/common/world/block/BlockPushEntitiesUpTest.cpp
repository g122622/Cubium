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
 * @file BlockPushEntitiesUpTest.cpp
 * @brief Block::pushEntitiesUp 单元测试
 *
 * 测试方块碰撞形状增大时将嵌入实体向上推出的行为，对照 MC Java 1.21.11
 * `net.minecraft.world.level.block.Block#pushEntitiesUp` 的实现：
 *
 * - 差集形状计算（BooleanOps::OnlySecond）
 * - 空差集短路（oldState == newState、newState 形状小于等于 oldState）
 * - 实体在差集范围内：被精确推出到差集形状顶部之上
 * - 实体不在差集范围内：不受影响
 * - 多实体同时推出
 *
 * 测试通过自定义 MockWorld 返回受控的实体列表，避免依赖真实世界/物理引擎。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/MoverType.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;

namespace {

/**
 * @brief 用于 pushEntitiesUp 测试的 MockWorld
 *
 * 提供受控的实体列表（通过 setEntityOverride 注入），其它 IWorld 方法均为空实现。
 * getEntitiesInAABB 直接返回注入的实体，不进行 AABB 过滤——是否在范围内的判断
 * 由调用者（pushEntitiesUp）自行处理。
 */
class PushEntitiesUpTestWorld final : public mc::test::BaseTestWorld {
public:
    PushEntitiesUpTestWorld() = default;

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
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        MC_UNUSED(x);
        MC_UNUSED(y);
        MC_UNUSED(z);
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    /// 注入 getEntitiesInAABB 返回的实体列表（测试控制）
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const override
    {
        MC_UNUSED(box);
        MC_UNUSED(except);
        return m_entities;
    }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<Entity*> m_entities;
};

/// 创建一个 FallingBlockEntity 并设置位置（其宽度 0.98、高度 0.98）
std::unique_ptr<entity::FallingBlockEntity> makeEntityAt(f32 x, f32 y, f32 z)
{
    auto e = std::make_unique<entity::FallingBlockEntity>(mc::test::testEcsRegistry());
    e->setPosition(x, y, z);
    return e;
}

} // namespace

// ============================================================================
// 测试固件
// ============================================================================

class BlockPushEntitiesUpTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// 基础行为测试
// ============================================================================

TEST_F(BlockPushEntitiesUpTest, AirToStone_PushesEntityUpByOne)
{
    // 场景：AIR (空碰撞) → STONE (完整方块)
    // 差集形状 = 完整方块（在 newState 中但不在 oldState 中）
    // 实体站在方块底部（脚 y=0，头 y≈0.98），应被上推 1 格（落到方块顶部 y=1）
    PushEntitiesUpTestWorld world;

    const BlockState& air = VanillaBlocks::AIR->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 0, 0);

    auto entity = makeEntityAt(0.5f, 0.0f, 0.5f);
    Entity* entityPtr = entity.get();
    world.setEntities({entityPtr});

    // 记录初始 Y
    f32 initialY = entity->position().y;

    Block::pushEntitiesUp(air, stone, world, pos);

    // 期望：实体被上推 1 格（从 y=0 → y=1）
    f32 finalY = entity->position().y;
    EXPECT_NEAR(static_cast<f64>(finalY - initialY), 1.0, 1.0e-4);
}

TEST_F(BlockPushEntitiesUpTest, SameState_NoPush)
{
    // 场景：STONE → STONE，差集为空，不应移动实体
    PushEntitiesUpTestWorld world;

    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 0, 0);

    auto entity = makeEntityAt(0.5f, 0.0f, 0.5f);
    Entity* entityPtr = entity.get();
    world.setEntities({entityPtr});

    f32 initialY = entity->position().y;

    Block::pushEntitiesUp(stone, stone, world, pos);

    // 期望：实体位置不变
    EXPECT_EQ(entity->position().y, initialY);
}

TEST_F(BlockPushEntitiesUpTest, StoneToAir_NoPush)
{
    // 场景：碰撞形状缩小（STONE → AIR），差集为空（ONLY_SECOND：在 AIR 中但不在 STONE 中）
    // 实体不应被推动
    PushEntitiesUpTestWorld world;

    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    const BlockState& air = VanillaBlocks::AIR->defaultState();
    BlockPos pos(0, 0, 0);

    auto entity = makeEntityAt(0.5f, 0.0f, 0.5f);
    Entity* entityPtr = entity.get();
    world.setEntities({entityPtr});

    f32 initialY = entity->position().y;

    Block::pushEntitiesUp(stone, air, world, pos);

    // 期望：实体位置不变
    EXPECT_EQ(entity->position().y, initialY);
}

TEST_F(BlockPushEntitiesUpTest, EntityOutOfRange_NotAffected)
{
    // 场景：AIR → STONE，但实体位于远处（不应被推动）
    // 注：测试 MockWorld 的 getEntitiesInAABB 不做范围过滤，
    // 因此 pushEntitiesUp 内部的 bounds() 查询仍会返回该实体；
    // 但实体 boundingBox 与 diffShape.bounds() 不相交，
    // collide(Y, shiftedBox, -1.0) 会返回 -1.0（无阻挡），
    // 故 pushUp = 1 + (-1) = 0，不会移动实体。
    PushEntitiesUpTestWorld world;

    const BlockState& air = VanillaBlocks::AIR->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 0, 0);

    // 实体在 (100, 100, 100)，远离方块
    auto entity = makeEntityAt(100.5f, 100.0f, 100.5f);
    Entity* entityPtr = entity.get();
    world.setEntities({entityPtr});

    f32 initialY = entity->position().y;

    Block::pushEntitiesUp(air, stone, world, pos);

    // 期望：实体位置不变（pushUp = 0，不调用 move）
    EXPECT_EQ(entity->position().y, initialY);
}

// ============================================================================
// 多实体测试
// ============================================================================

TEST_F(BlockPushEntitiesUpTest, MultipleEntities_AllPushed)
{
    // 场景：AIR → STONE，多个实体均脚踩方块底部（y=0），应都被上推 1 格
    // 注：实体脚 y=0 时，shiftedBox.minY=1.0，正好与 diffShape.maxY=1.0 相切，
    // collide 返回 d0=0，pushUp=1+0=1。
    PushEntitiesUpTestWorld world;

    const BlockState& air = VanillaBlocks::AIR->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 0, 0);

    auto e1 = makeEntityAt(0.5f, 0.0f, 0.5f);
    auto e2 = makeEntityAt(0.2f, 0.0f, 0.2f);
    auto e3 = makeEntityAt(0.8f, 0.0f, 0.8f);

    std::vector<Entity*> entities = {e1.get(), e2.get(), e3.get()};
    world.setEntities(entities);

    f32 y1Before = e1->position().y;
    f32 y2Before = e2->position().y;
    f32 y3Before = e3->position().y;

    Block::pushEntitiesUp(air, stone, world, pos);

    EXPECT_NEAR(static_cast<f64>(e1->position().y - y1Before), 1.0, 1.0e-4);
    EXPECT_NEAR(static_cast<f64>(e2->position().y - y2Before), 1.0, 1.0e-4);
    EXPECT_NEAR(static_cast<f64>(e3->position().y - y3Before), 1.0, 1.0e-4);
}

TEST_F(BlockPushEntitiesUpTest, EmptyEntityList_NoCrash)
{
    // 场景：AIR → STONE，但世界中无实体（getEntitiesInAABB 返回空）
    // 不应崩溃，且返回 newState
    PushEntitiesUpTestWorld world;

    const BlockState& air = VanillaBlocks::AIR->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 0, 0);

    world.setEntities({});

    auto& result = Block::pushEntitiesUp(air, stone, world, pos);
    EXPECT_EQ(&result, &stone);
}

// ============================================================================
// 非零坐标测试
// ============================================================================

TEST_F(BlockPushEntitiesUpTest, NonZeroBlockPos_PushesCorrectly)
{
    // 场景：方块在非零坐标 (5, 10, -3)，AIR → STONE
    // 实体站在该方块内（脚 y=10），应被上推 1 格（到 y=11）
    PushEntitiesUpTestWorld world;

    const BlockState& air = VanillaBlocks::AIR->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(5, 10, -3);

    auto entity = makeEntityAt(5.5f, 10.0f, -2.5f);
    Entity* entityPtr = entity.get();
    world.setEntities({entityPtr});

    f32 initialY = entity->position().y;

    Block::pushEntitiesUp(air, stone, world, pos);

    // 期望：实体被上推 1 格
    EXPECT_NEAR(static_cast<f64>(entity->position().y - initialY), 1.0, 1.0e-4);
}

// ============================================================================
// 返回值测试
// ============================================================================

TEST_F(BlockPushEntitiesUpTest, ReturnsNewState)
{
    // 验证 pushEntitiesUp 返回 newState（按 MC 实现）
    PushEntitiesUpTestWorld world;

    const BlockState& air = VanillaBlocks::AIR->defaultState();
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 0, 0);

    world.setEntities({});

    auto& result = Block::pushEntitiesUp(air, stone, world, pos);
    EXPECT_EQ(&result, &stone);

    // 反向调用也应返回 newState
    auto& result2 = Block::pushEntitiesUp(stone, air, world, pos);
    EXPECT_EQ(&result2, &air);
}
