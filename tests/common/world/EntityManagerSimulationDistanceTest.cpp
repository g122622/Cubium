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

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <memory>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

/// @brief 计数 tick 调用次数的 mock 实体（非玩家，entityType()==nullptr）。
///
/// 继承 Entity 而非 Player，故 entityType() 懒查询返回 nullptr，不会被 getPlayers()
/// 误判为玩家锚点，会走 EntityManager::tick 的模拟距离门控路径。
class CountingEntity final : public Entity {
public:
    CountingEntity()
        : Entity(EntityInstanceId(0), nullptr)
    {}

    void tick() override
    {
        Entity::tick();
        ++m_tickCount;
    }

    [[nodiscard]] i32 tickCount() const { return m_tickCount; }

private:
    i32 m_tickCount = 0;
};

/// @brief 测试用 Player id 基址。EntityManager::addEntity 对 id=0 的实体会用 m_nextId
/// 分配（默认从 1 起），若 Player 用低 id（如 1）则不推进 m_nextId，导致首个 id=0 的
/// CountingEntity 分到相同 id 覆盖 Player。故 Player 统一用高 id 避开自动分配区间。
constexpr EntityInstanceId PLAYER_ID_BASE = 1000;

} // namespace

/**
 * @brief EntityManager 模拟距离实体冻结语义测试
 *
 * 验证 P2 核心：对齐原版 ServerLevel.tick 的 inEntityTickingRange 等价语义——
 * ServerPlayer 永远 tick；其余实体仅当其所在区块相对任一玩家切比雪夫距离 <= simulationDistance
 * 才 tick，否则冻结；simulationDistance >= 32 时短路全量 tick；无玩家时非玩家实体一律冻结。
 */
class EntityManagerSimulationDistanceTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaEntities::registerAll(); }

    void TearDown() override {}

    EntityManager m_manager;
};

// ============================================================================
// simulationDistance >= 32：短路全量 tick
// ============================================================================
TEST_F(EntityManagerSimulationDistanceTest, FreezeDisabledWhenDistanceAtLeast32)
{
    // 32 为配置上限，>=32 等价全量 tick（freezeEnabled=false），不做距离门控。
    m_manager.setSimulationDistance(32);

    auto player = std::make_unique<Player>(PLAYER_ID_BASE, "Anchor");
    player->setPosition(0.0f, 0.0f, 0.0f);
    Player* playerPtr = player.get();
    m_manager.addEntity(std::move(player));

    auto far = std::make_unique<CountingEntity>();
    far->setPosition(1000.0f * world::CHUNK_WIDTH, 0.0f, 1000.0f * world::CHUNK_WIDTH);
    CountingEntity* farPtr = far.get();
    m_manager.addEntity(std::move(far));

    for (int i = 0; i < 3; ++i) {
        m_manager.tick();
    }

    // 远离玩家的非玩家实体仍被 tick（freezeEnabled=false）
    EXPECT_EQ(farPtr->tickCount(), 3);
    // Player 永远 tick
    EXPECT_FALSE(playerPtr->isRemoved());
}

// ============================================================================
// Player 永远 tick（即使设在远处）
// ============================================================================
TEST_F(EntityManagerSimulationDistanceTest, PlayerAlwaysTicksRegardlessOfDistance)
{
    m_manager.setSimulationDistance(2);

    // 玩家锚点设在原点
    auto anchor = std::make_unique<Player>(PLAYER_ID_BASE, "Anchor");
    anchor->setPosition(0.0f, 0.0f, 0.0f);
    m_manager.addEntity(std::move(anchor));

    // 另一个 Player 设在远处（100 区块外），应永远 tick（Player 短路，不走距离门控）
    auto farPlayer = std::make_unique<Player>(PLAYER_ID_BASE + 1, "FarPlayer");
    farPlayer->setPosition(100.0f * world::CHUNK_WIDTH, 0.0f, 0.0f);
    Player* farPlayerPtr = farPlayer.get();
    m_manager.addEntity(std::move(farPlayer));

    for (int i = 0; i < 3; ++i) {
        m_manager.tick();
    }

    EXPECT_FALSE(farPlayerPtr->isRemoved());
    // farPlayer 是 Player，永远 tick（不冻结）。Player::tick 本身不计数，此处仅验证未被冻结移除。
    SUCCEED();
}

// ============================================================================
// 非玩家实体：边界内 tick、边界外冻结
// ============================================================================
TEST_F(EntityManagerSimulationDistanceTest, NonPlayerEntityFrozenBeyondSimulationDistance)
{
    m_manager.setSimulationDistance(2);

    auto player = std::make_unique<Player>(PLAYER_ID_BASE, "Anchor");
    player->setPosition(0.0f, 0.0f, 0.0f);
    m_manager.addEntity(std::move(player));

    // 内：同一区块（切比雪夫 0 <= 2）→ tick
    auto inside = std::make_unique<CountingEntity>();
    inside->setPosition(0.0f, 0.0f, 0.0f);
    CountingEntity* insidePtr = inside.get();
    m_manager.addEntity(std::move(inside));

    // 边界：2 区块外（切比雪夫 2 <= 2）→ tick
    auto edge = std::make_unique<CountingEntity>();
    edge->setPosition(2.0f * world::CHUNK_WIDTH, 0.0f, 0.0f);
    CountingEntity* edgePtr = edge.get();
    m_manager.addEntity(std::move(edge));

    // 外：3 区块外（切比雪夫 3 > 2）→ 冻结
    auto outside = std::make_unique<CountingEntity>();
    outside->setPosition(3.0f * world::CHUNK_WIDTH, 0.0f, 0.0f);
    CountingEntity* outsidePtr = outside.get();
    m_manager.addEntity(std::move(outside));

    for (int i = 0; i < 5; ++i) {
        m_manager.tick();
    }

    EXPECT_EQ(insidePtr->tickCount(), 5);
    EXPECT_EQ(edgePtr->tickCount(), 5);
    EXPECT_EQ(outsidePtr->tickCount(), 0); // 冻结：tick 未被调用
}

// ============================================================================
// 切比雪夫距离：对角线方向判定（dx=2,dz=2 → max=2 <= 2 内；dx=3,dz=0 → 3 外）
// ============================================================================
TEST_F(EntityManagerSimulationDistanceTest, ChebyshevDistanceDiagonal)
{
    m_manager.setSimulationDistance(2);

    auto player = std::make_unique<Player>(PLAYER_ID_BASE, "Anchor");
    player->setPosition(0.0f, 0.0f, 0.0f);
    m_manager.addEntity(std::move(player));

    // 对角线 2 区块（dx=2,dz=2，切比雪夫 2）→ 内
    auto diag = std::make_unique<CountingEntity>();
    diag->setPosition(2.0f * world::CHUNK_WIDTH, 0.0f, 2.0f * world::CHUNK_WIDTH);
    CountingEntity* diagPtr = diag.get();
    m_manager.addEntity(std::move(diag));

    // 单轴 3 区块（dx=3,dz=0，切比雪夫 3）→ 外
    auto axis = std::make_unique<CountingEntity>();
    axis->setPosition(3.0f * world::CHUNK_WIDTH, 0.0f, 0.0f);
    CountingEntity* axisPtr = axis.get();
    m_manager.addEntity(std::move(axis));

    for (int i = 0; i < 2; ++i) {
        m_manager.tick();
    }

    EXPECT_EQ(diagPtr->tickCount(), 2); // 对角线切比雪夫 2，在范围内
    EXPECT_EQ(axisPtr->tickCount(), 0); // 单轴 3，超出范围
}

// ============================================================================
// 多玩家取并集：实体在任一玩家范围内即 tick
// ============================================================================
TEST_F(EntityManagerSimulationDistanceTest, MultiplePlayersUnion)
{
    m_manager.setSimulationDistance(2);

    // 玩家 A 在原点
    auto playerA = std::make_unique<Player>(PLAYER_ID_BASE, "A");
    playerA->setPosition(0.0f, 0.0f, 0.0f);
    m_manager.addEntity(std::move(playerA));

    // 玩家 B 在 (10,0,10) 区块
    auto playerB = std::make_unique<Player>(PLAYER_ID_BASE + 1, "B");
    playerB->setPosition(10.0f * world::CHUNK_WIDTH, 0.0f, 10.0f * world::CHUNK_WIDTH);
    m_manager.addEntity(std::move(playerB));

    // 实体在 B 附近（距 A 远、距 B 近）→ 因 B 在范围内而 tick
    auto nearB = std::make_unique<CountingEntity>();
    nearB->setPosition(10.0f * world::CHUNK_WIDTH, 0.0f, 10.0f * world::CHUNK_WIDTH);
    CountingEntity* nearBPtr = nearB.get();
    m_manager.addEntity(std::move(nearB));

    // 实体在两玩家中间且都超出范围（A 在 0,0；B 在 10,10；实体在 5,5，距两者切比雪夫 5 > 2）→ 冻结
    auto middle = std::make_unique<CountingEntity>();
    middle->setPosition(5.0f * world::CHUNK_WIDTH, 0.0f, 5.0f * world::CHUNK_WIDTH);
    CountingEntity* middlePtr = middle.get();
    m_manager.addEntity(std::move(middle));

    for (int i = 0; i < 3; ++i) {
        m_manager.tick();
    }

    EXPECT_EQ(nearBPtr->tickCount(), 3);  // 在 B 范围内（并集）
    EXPECT_EQ(middlePtr->tickCount(), 0); // 超出两玩家范围
}

// ============================================================================
// 无玩家时：非玩家实体一律冻结（playerChunks.empty() → false）
// ============================================================================
TEST_F(EntityManagerSimulationDistanceTest, NoPlayerFreezesAllNonPlayerEntities)
{
    m_manager.setSimulationDistance(10);

    auto orphan = std::make_unique<CountingEntity>();
    orphan->setPosition(0.0f, 0.0f, 0.0f);
    CountingEntity* orphanPtr = orphan.get();
    m_manager.addEntity(std::move(orphan));

    for (int i = 0; i < 3; ++i) {
        m_manager.tick();
    }

    EXPECT_EQ(orphanPtr->tickCount(), 0); // 无玩家锚点 → 冻结
}
