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

#include "client/world/player/PlayerIdentityRegistry.hpp"
#include "common/core/Types.hpp"

using namespace mc;
using namespace mc::client;

namespace {

Uuid makeUuid(u8 b)
{
    Uuid uuid{};
    uuid[0] = b;
    return uuid;
}

} // namespace

/**
 * @brief PlayerIdentityRegistry 单元测试
 *
 * 覆盖三向映射（UUID↔entityId↔username）、乱序到达（PlayerListEntry 早于/晚于 spawn）、
 * local/network 区分及各 remove 路径。
 */
class PlayerIdentityRegistryTest : public ::testing::Test {
protected:
    PlayerIdentityRegistry registry;
};

TEST_F(PlayerIdentityRegistryTest, RegisterLocalPlayerIndexesAllFields)
{
    registry.registerLocalPlayer(100, 7u, makeUuid(1), "Steve");

    EXPECT_EQ(registry.size(), 1u);
    ASSERT_NE(registry.uuidOf(100), nullptr);
    EXPECT_EQ(*registry.uuidOf(100), makeUuid(1));
    EXPECT_EQ(registry.playerIdOf(100), 7u);
    EXPECT_EQ(registry.entityIdOf(makeUuid(1)), 100);
    EXPECT_EQ(registry.entityIdByUsername("Steve"), 100);
    ASSERT_NE(registry.uuidByUsername("Steve"), nullptr);
    EXPECT_EQ(*registry.uuidByUsername("Steve"), makeUuid(1));
    EXPECT_TRUE(registry.isLocal(100));
}

TEST_F(PlayerIdentityRegistryTest, NetworkPlayerWithoutUuidReturnsNull)
{
    registry.registerNetworkPlayer(200, 9u, "Alex");

    // spawn 包无 UUID 且无 PlayerListEntry 暂存 → uuidOf 返回 nullptr
    EXPECT_EQ(registry.uuidOf(200), nullptr);
    EXPECT_EQ(registry.playerIdOf(200), 9u);
    EXPECT_FALSE(registry.isLocal(200));
}

TEST_F(PlayerIdentityRegistryTest, PlayerListUuidBeforeSpawnCompletesOnSpawn)
{
    // PlayerListEntry 先到，暂存 username→uuid
    registry.registerPlayerListUuid(makeUuid(2), "Alex");
    ASSERT_NE(registry.uuidByUsername("Alex"), nullptr);
    EXPECT_EQ(*registry.uuidByUsername("Alex"), makeUuid(2));
    // 实体尚未注册，entityId 查不到
    EXPECT_EQ(registry.entityIdByUsername("Alex"), INVALID_ENTITY_ID);

    // spawn 包后到，从暂存表取用 UUID 补全
    registry.registerNetworkPlayer(200, 9u, "Alex");

    ASSERT_NE(registry.uuidOf(200), nullptr);
    EXPECT_EQ(*registry.uuidOf(200), makeUuid(2));
    EXPECT_EQ(registry.entityIdOf(makeUuid(2)), 200);
}

TEST_F(PlayerIdentityRegistryTest, PlayerListUuidAfterSpawnCompletesExistingEntry)
{
    // spawn 包先到（无 UUID）
    registry.registerNetworkPlayer(300, 11u, "Bob");
    EXPECT_EQ(registry.uuidOf(300), nullptr);

    // PlayerListEntry 后到，补全已注册条目的 UUID
    registry.registerPlayerListUuid(makeUuid(3), "Bob");

    ASSERT_NE(registry.uuidOf(300), nullptr);
    EXPECT_EQ(*registry.uuidOf(300), makeUuid(3));
    EXPECT_EQ(registry.entityIdOf(makeUuid(3)), 300);
}

TEST_F(PlayerIdentityRegistryTest, AssignUuidIsIdempotent)
{
    registry.registerNetworkPlayer(400, 13u, "Cara");
    EXPECT_TRUE(registry.assignUuidToEntity(400, makeUuid(4)));
    EXPECT_TRUE(registry.assignUuidToEntity(400, makeUuid(4))); // 幂等
    ASSERT_NE(registry.uuidOf(400), nullptr);
    EXPECT_EQ(*registry.uuidOf(400), makeUuid(4));

    // 未知 entityId
    EXPECT_FALSE(registry.assignUuidToEntity(999, makeUuid(4)));
}

TEST_F(PlayerIdentityRegistryTest, RemoveByEntityIdClearsAllIndexes)
{
    registry.registerLocalPlayer(100, 7u, makeUuid(1), "Steve");
    registry.removeByEntityId(100);

    EXPECT_EQ(registry.size(), 0u);
    EXPECT_EQ(registry.uuidOf(100), nullptr);
    EXPECT_EQ(registry.playerIdOf(100), 0u);
    EXPECT_EQ(registry.entityIdOf(makeUuid(1)), INVALID_ENTITY_ID);
    EXPECT_EQ(registry.entityIdByUsername("Steve"), INVALID_ENTITY_ID);
    EXPECT_FALSE(registry.isLocal(100));
}

TEST_F(PlayerIdentityRegistryTest, RemoveByPlayerIdRemovesEntry)
{
    registry.registerNetworkPlayer(200, 9u, "Alex");
    registry.registerPlayerListUuid(makeUuid(2), "Alex");

    registry.removeByPlayerId(9u);
    EXPECT_EQ(registry.size(), 0u);
    EXPECT_EQ(registry.entityIdOf(makeUuid(2)), INVALID_ENTITY_ID);
}

TEST_F(PlayerIdentityRegistryTest, RemoveByUuidRemovesEntry)
{
    registry.registerLocalPlayer(100, 7u, makeUuid(1), "Steve");
    registry.removeByUuid(makeUuid(1));

    EXPECT_EQ(registry.size(), 0u);
    EXPECT_FALSE(registry.isLocal(100));
}

TEST_F(PlayerIdentityRegistryTest, ClearRemovesEverything)
{
    registry.registerLocalPlayer(100, 7u, makeUuid(1), "Steve");
    registry.registerNetworkPlayer(200, 9u, "Alex");
    registry.registerPlayerListUuid(makeUuid(2), "Alex");

    registry.clear();

    EXPECT_EQ(registry.size(), 0u);
    EXPECT_FALSE(registry.isLocal(100));
    EXPECT_EQ(registry.entityIdOf(makeUuid(1)), INVALID_ENTITY_ID);
    EXPECT_EQ(registry.uuidByUsername("Steve"), nullptr);
}

TEST_F(PlayerIdentityRegistryTest, IsLocalOnlyForLocalPlayer)
{
    registry.registerLocalPlayer(100, 7u, makeUuid(1), "Steve");
    registry.registerNetworkPlayer(200, 9u, "Alex");

    EXPECT_TRUE(registry.isLocal(100));
    EXPECT_FALSE(registry.isLocal(200));
    EXPECT_FALSE(registry.isLocal(999));
}
