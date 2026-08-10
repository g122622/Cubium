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
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/functional/StonecutterBlock.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "core/Types.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/block/BlockRegistry.hpp"

#include <memory>
#include <unordered_map>

// 前向声明
namespace mc::server {
class ServerWorld;
}

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用 Mock World，用于测试 StonecutterBlock 的交互逻辑
 *
 * 实现了 IWorld 接口的关键方法，特别是：
 * - isClientSide() / asServerWorld() 用于区分客户端/服务端
 * - openContainer() 用于测试容器打开
 */
class StonecutterTestWorld : public mc::test::BaseTestWorld {
public:
    explicit StonecutterTestWorld(bool isClient = false)
        : m_isClient(isClient)
        , m_openContainerCalled(false)
        , m_lastContainerType(ContainerType::Player)
        , m_lastContainerPos(0, 0, 0)
        , m_lastContainerPlayer(nullptr)
    {}

    // ========== IWorld 核心接口 ==========

    [[nodiscard]] bool isClientSide() const override { return m_isClient; }

    [[nodiscard]] server::ServerWorld* asServerWorld() override
    {
        return m_isClient ? nullptr : reinterpret_cast<server::ServerWorld*>(0x1);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) return false;
        m_blockStates[BlockPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blockStates.find(pos);
        return it == m_blockStates.end() ? nullptr : it->second;
    }

    bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override
    {
        m_openContainerCalled = true;
        m_lastContainerType = type;
        m_lastContainerPos = pos;
        m_lastContainerPlayer = &player;
        return !m_isClient; // 客户端返回 false，服务端返回 true
    }

    // ========== 测试验证方法 ==========

    [[nodiscard]] bool wasOpenContainerCalled() const { return m_openContainerCalled; }
    [[nodiscard]] ContainerType getLastContainerType() const { return m_lastContainerType; }
    [[nodiscard]] BlockPos getLastContainerPos() const { return m_lastContainerPos; }
    [[nodiscard]] Player* getLastContainerPlayer() const { return m_lastContainerPlayer; }

    void setBlockStateAt(const BlockPos& pos, const BlockState* state) { m_blockStates[pos] = state; }

    // ========== IWorld 存根方法 ==========

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("StonecutterTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("StonecutterTestWorld::tickManager not implemented");
    }

private:
    bool m_isClient;
    bool m_openContainerCalled;
    ContainerType m_lastContainerType;
    BlockPos m_lastContainerPos;
    Player* m_lastContainerPlayer;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
};

} // namespace

// ========== StonecutterBlock 基础测试 ==========

class StonecutterBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stonecutter_ =
            std::make_unique<StonecutterBlock>(BlockProperties(Material::ROCK).hardness(3.5f).resistance(3.5f));
    }

    std::unique_ptr<StonecutterBlock> stonecutter_;
};

TEST_F(StonecutterBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(stonecutter_, nullptr);
}

TEST_F(StonecutterBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = stonecutter_->defaultState();
    const auto& shape = stonecutter_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(StonecutterBlockTest, HasBlockEntity_ReturnsFalse)
{
    // 切石机没有方块实体
    EXPECT_FALSE(stonecutter_->hasBlockEntity());
}

// ========== StonecutterBlock 交互测试 ==========

class StonecutterBlockInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stonecutter_ =
            std::make_unique<StonecutterBlock>(BlockProperties(Material::ROCK).hardness(3.5f).resistance(3.5f));
        pos_ = BlockPos(10, 64, 20);
    }

    std::unique_ptr<StonecutterBlock> stonecutter_;
    BlockPos pos_;
};

TEST_F(StonecutterBlockInteractionTest, OnBlockActivated_ClientSide_ReturnsSuccess)
{
    // 客户端世界
    StonecutterTestWorld world(true);

    // 设置方块状态
    world.setBlockStateAt(pos_, &stonecutter_->defaultState());

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = stonecutter_->defaultState();
    BlockRaycastResult hit;
    auto result = stonecutter_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 客户端应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 客户端不应调用 openContainer
    EXPECT_FALSE(world.wasOpenContainerCalled());
}

TEST_F(StonecutterBlockInteractionTest, OnBlockActivated_ServerSide_OpensContainer)
{
    // 服务端世界
    StonecutterTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &stonecutter_->defaultState());

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = stonecutter_->defaultState();
    BlockRaycastResult hit;
    auto result = stonecutter_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 服务端应返回 Consume
    EXPECT_EQ(result, ActionResultType::Consume);

    // 服务端应调用 openContainer 且容器类型为 Stonecutter
    EXPECT_TRUE(world.wasOpenContainerCalled());
    EXPECT_EQ(world.getLastContainerType(), ContainerType::Stonecutter);
    EXPECT_EQ(world.getLastContainerPos(), pos_);
    EXPECT_EQ(world.getLastContainerPlayer(), &player);
}

TEST_F(StonecutterBlockInteractionTest, OnBlockActivated_OffHand_SameBehavior)
{
    // 服务端世界
    StonecutterTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &stonecutter_->defaultState());

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 使用副手执行交互
    const auto& state = stonecutter_->defaultState();
    BlockRaycastResult hit;
    auto result = stonecutter_->onBlockActivated(state, world, pos_, player, Hand::OffHand, hit);

    // 副手交互应与主手行为一致
    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_TRUE(world.wasOpenContainerCalled());
    EXPECT_EQ(world.getLastContainerType(), ContainerType::Stonecutter);
}

TEST_F(StonecutterBlockInteractionTest, OnBlockActivated_OpenContainerFails_ReturnsPass)
{
    // 服务端世界，但 openContainer 返回 false
    class FailingOpenContainerWorld final : public StonecutterTestWorld {
    public:
        explicit FailingOpenContainerWorld()
            : StonecutterTestWorld(false)
        {}

        bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override
        {
            // 记录调用但返回 false
            StonecutterTestWorld::openContainer(type, pos, player);
            return false;
        }
    };

    FailingOpenContainerWorld world;

    // 设置方块状态
    world.setBlockStateAt(pos_, &stonecutter_->defaultState());

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = stonecutter_->defaultState();
    BlockRaycastResult hit;
    auto result = stonecutter_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // openContainer 返回 false 时应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}
