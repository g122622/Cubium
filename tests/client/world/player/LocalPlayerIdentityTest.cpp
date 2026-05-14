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

#include "client/world/player/LocalPlayerIdentity.hpp"
#include "common/core/Types.hpp"

using namespace mc;
using namespace mc::client;

/**
 * @brief LocalPlayerIdentity 单元测试
 */
class LocalPlayerIdentityTest : public ::testing::Test {
protected:
    LocalPlayerIdentity identity;
};

TEST_F(LocalPlayerIdentityTest, InitialState)
{
    EXPECT_FALSE(identity.hasIdentity());
    EXPECT_EQ(identity.playerId(), 0u);
    EXPECT_EQ(identity.entityId(), mc::INVALID_ENTITY_ID);
}

TEST_F(LocalPlayerIdentityTest, SetIdentity)
{
    identity.setIdentity(42, 100);

    EXPECT_TRUE(identity.hasIdentity());
    EXPECT_EQ(identity.playerId(), 42u);
    EXPECT_EQ(identity.entityId(), 100u);
}

TEST_F(LocalPlayerIdentityTest, Clear)
{
    identity.setIdentity(42, 100);
    EXPECT_TRUE(identity.hasIdentity());

    identity.clear();

    EXPECT_FALSE(identity.hasIdentity());
    EXPECT_EQ(identity.playerId(), 0u);
    EXPECT_EQ(identity.entityId(), mc::INVALID_ENTITY_ID);
}

TEST_F(LocalPlayerIdentityTest, IsLocalPlayerEntity)
{
    identity.setIdentity(42, 100);

    EXPECT_TRUE(identity.isLocalPlayerEntity(100));
    EXPECT_FALSE(identity.isLocalPlayerEntity(101));
    EXPECT_FALSE(identity.isLocalPlayerEntity(1));
    EXPECT_FALSE(identity.isLocalPlayerEntity(mc::INVALID_ENTITY_ID));
}

TEST_F(LocalPlayerIdentityTest, IsLocalPlayer)
{
    identity.setIdentity(42, 100);

    EXPECT_TRUE(identity.isLocalPlayer(42));
    EXPECT_FALSE(identity.isLocalPlayer(43));
    EXPECT_FALSE(identity.isLocalPlayer(0));
}

TEST_F(LocalPlayerIdentityTest, ClearBeforeSetDoesNotCrash)
{
    // 清除未设置的身份应该安全
    EXPECT_NO_THROW(identity.clear());
    EXPECT_FALSE(identity.hasIdentity());
}

TEST_F(LocalPlayerIdentityTest, MultipleSetIdentity)
{
    // 第一次设置
    identity.setIdentity(42, 100);
    EXPECT_EQ(identity.playerId(), 42u);
    EXPECT_EQ(identity.entityId(), 100u);

    // 第二次设置覆盖
    identity.setIdentity(99, 200);
    EXPECT_EQ(identity.playerId(), 99u);
    EXPECT_EQ(identity.entityId(), 200u);

    // 检查身份判断是否正确
    EXPECT_TRUE(identity.isLocalPlayer(99));
    EXPECT_TRUE(identity.isLocalPlayerEntity(200));
    EXPECT_FALSE(identity.isLocalPlayer(42));
    EXPECT_FALSE(identity.isLocalPlayerEntity(100));
}

TEST_F(LocalPlayerIdentityTest, InvalidEntityIdCheck)
{
    identity.setIdentity(42, 100);

    // INVALID_ENTITY_ID 不应该匹配
    EXPECT_FALSE(identity.isLocalPlayerEntity(mc::INVALID_ENTITY_ID));
}

TEST_F(LocalPlayerIdentityTest, ZeroPlayerIdCheck)
{
    identity.setIdentity(42, 100);

    // PlayerId 0 不应该匹配
    EXPECT_FALSE(identity.isLocalPlayer(0));
}
