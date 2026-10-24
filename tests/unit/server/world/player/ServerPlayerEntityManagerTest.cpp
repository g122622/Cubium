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
 * @file ServerPlayerEntityManagerTest.cpp
 * @brief ServerPlayerEntityManager 单元测试
 *
 * 重点覆盖 PlayerId ↔ EntityInstanceId 映射随实体生命周期清理的契约：
 * 玩家死亡 20 tick 后被 tickDeath 交给世界移除，若映射不同步清理，重生流程的
 * createPlayerEntity 会被"Player 已存在"拒绝，表现为死亡后永远无法重生。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/RemovalReason.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/application/IServer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <memory>

using namespace mc;
using namespace mc::server;

/// 最小服务端装配：基类 BaseTestServer 提供 IServer 的其余桩实现，本类只覆写
/// 玩家实体管理器（世界侧移除通知经它清理映射）。
class MappingTestServer final : public mc::test::BaseTestServer {
public:
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override
    {
        return m_entityManager;
    }

    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_entityManager;
    }

private:
    ServerPlayerEntityManager m_entityManager;
};

class ServerPlayerEntityManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<ServerWorld>(ServerWorldConfig{2, 2, 0, 1, "player-entity-mapping-test"});
        // 注入服务端，使世界的移除通知能到达玩家实体管理器（与真实装配一致）。
        m_world->setServer(&m_server);
    }

    void TearDown() override
    {
        m_world.reset();
    }

    /// 以原生 Player 实体注册映射（不经过 createPlayerEntity 的完整服务端上下文装配）
    EntityInstanceId registerPlayer(PlayerId playerId)
    {
        auto player = std::make_unique<Player>(0, "Tester", *m_world->entityRegistry());
        player->setWorld(m_world.get());
        const EntityInstanceId entityId = m_world->entityManager().addEntity(std::move(player));
        EXPECT_NE(entityId, INVALID_ENTITY_ID);
        EXPECT_TRUE(m_server.playerEntityManager().registerExistingPlayerEntity(playerId, entityId, *m_world));
        return entityId;
    }

    MappingTestServer m_server;
    std::unique_ptr<ServerWorld> m_world;
};

TEST_F(ServerPlayerEntityManagerTest, InitialState)
{
    EXPECT_EQ(m_server.playerEntityManager().playerCount(), 0);
    EXPECT_TRUE(m_server.playerEntityManager().getPlayerIds().empty());
}

TEST_F(ServerPlayerEntityManagerTest, GetPlayerEntityIdNotFound)
{
    // 未注册玩家时，查询应该返回 INVALID_ENTITY_ID
    EXPECT_EQ(m_server.playerEntityManager().getPlayerEntityId(999), INVALID_ENTITY_ID);
}

TEST_F(ServerPlayerEntityManagerTest, GetPlayerIdByEntityIdNotFound)
{
    // 未注册玩家时，查询应该返回 0
    EXPECT_EQ(m_server.playerEntityManager().getPlayerIdByEntityId(999), 0);
}

TEST_F(ServerPlayerEntityManagerTest, HasPlayerEmpty)
{
    EXPECT_FALSE(m_server.playerEntityManager().hasPlayer(1));
    EXPECT_FALSE(m_server.playerEntityManager().hasPlayer(100));
}

TEST_F(ServerPlayerEntityManagerTest, WorldRemovalClearsPlayerMapping)
{
    constexpr PlayerId playerId = 1;
    const EntityInstanceId entityId = registerPlayer(playerId);
    ASSERT_NE(entityId, INVALID_ENTITY_ID);

    // 世界侧移除（死亡消散、区块卸载、跨维度迁移走同一条通知路径）。移除通知在实体 tick
    // 安全点统一冲刷，这里直接驱动一次，等价于服务端主循环里世界 tick 之后的通知阶段。
    m_world->destroyEntity(entityId, RemovalReason::Killed);
    m_world->entityManager().removeDeadEntities();

    EXPECT_FALSE(m_server.playerEntityManager().hasPlayer(playerId));
    EXPECT_EQ(m_server.playerEntityManager().getPlayerEntityId(playerId), INVALID_ENTITY_ID);
    EXPECT_EQ(m_server.playerEntityManager().getPlayerIdByEntityId(entityId), 0);
}

TEST_F(ServerPlayerEntityManagerTest, PlayerIdCanBeRegisteredAgainAfterWorldRemoval)
{
    constexpr PlayerId playerId = 2;
    const EntityInstanceId oldEntityId = registerPlayer(playerId);
    ASSERT_NE(oldEntityId, INVALID_ENTITY_ID);

    m_world->destroyEntity(oldEntityId, RemovalReason::Killed);
    m_world->entityManager().removeDeadEntities();

    // 修复前这里会命中"Player already exists"并返回 false（重生永久失败）。
    auto replacement = std::make_unique<Player>(0, "Tester", *m_world->entityRegistry());
    replacement->setWorld(m_world.get());
    const EntityInstanceId newEntityId = m_world->entityManager().addEntity(std::move(replacement));
    ASSERT_NE(newEntityId, INVALID_ENTITY_ID);
    ASSERT_TRUE(m_server.playerEntityManager().registerExistingPlayerEntity(playerId, newEntityId, *m_world));

    EXPECT_NE(newEntityId, oldEntityId);
    EXPECT_EQ(m_server.playerEntityManager().getPlayerEntityId(playerId), newEntityId);
    EXPECT_EQ(m_server.playerEntityManager().playerCount(), 1u);
}

TEST_F(ServerPlayerEntityManagerTest, RemovePlayerEntityIsIdempotentWhenEntityAlreadyGone)
{
    constexpr PlayerId playerId = 3;
    const EntityInstanceId entityId = registerPlayer(playerId);
    ASSERT_NE(entityId, INVALID_ENTITY_ID);
    m_world->destroyEntity(entityId, RemovalReason::Killed);
    m_world->entityManager().removeDeadEntities();

    // 映射已由世界移除通知清理；重生流程再次调用必须安全（空操作）。
    m_server.playerEntityManager().removePlayerEntity(playerId, m_world.get(), RemovalReason::Killed);
    m_server.playerEntityManager().removePlayerEntity(playerId, nullptr);

    EXPECT_FALSE(m_server.playerEntityManager().hasPlayer(playerId));
    EXPECT_EQ(m_server.playerEntityManager().playerCount(), 0u);
}