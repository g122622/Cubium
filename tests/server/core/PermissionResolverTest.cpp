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
 * @file PermissionResolverTest.cpp
 * @brief 单机主机作弊权限提升纯函数单元测试
 *
 * 锁定 applyOwnerCheatsBoost 的提升规则：主机且开启作弊时提升到 OpLevel::Owner，
 * 其它情况原样返回。该规则是 `/tp` 在单机作弊世界可用的核心修复点。
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "server/core/OpListManager.hpp"

using namespace mc::server::core;

TEST(ApplyOwnerCheatsBoostTest, OwnerWithCheatsBoostsToOwner)
{
    // 修复目标场景：单机主机开作弊，从权限 0 提升到 4
    EXPECT_EQ(
        applyOwnerCheatsBoost(static_cast<mc::i32>(OpLevel::Normal), true, true), static_cast<mc::i32>(OpLevel::Owner));
}

TEST(ApplyOwnerCheatsBoostTest, OwnerWithoutCheatsKeepsBase)
{
    // 主机未开作弊：保持原行为，无提升
    EXPECT_EQ(applyOwnerCheatsBoost(static_cast<mc::i32>(OpLevel::Normal), true, false),
        static_cast<mc::i32>(OpLevel::Normal));
}

TEST(ApplyOwnerCheatsBoostTest, NonOwnerWithCheatsKeepsBase)
{
    // LAN 其它玩家即使开作弊也不提升（非主机）
    EXPECT_EQ(applyOwnerCheatsBoost(static_cast<mc::i32>(OpLevel::Normal), false, true),
        static_cast<mc::i32>(OpLevel::Normal));
}

TEST(ApplyOwnerCheatsBoostTest, OwnerWithCheatsDoesNotLowerExistingOp)
{
    // 已是 OP3 的主机开作弊，仍提升到 Owner(4)，不会降低
    EXPECT_EQ(
        applyOwnerCheatsBoost(static_cast<mc::i32>(OpLevel::Admin), true, true), static_cast<mc::i32>(OpLevel::Owner));
}

TEST(ApplyOwnerCheatsBoostTest, RegularOpUnaffected)
{
    // 普通 OP2 不受主机作弊开关影响
    EXPECT_EQ(applyOwnerCheatsBoost(static_cast<mc::i32>(OpLevel::GameMaster), false, false),
        static_cast<mc::i32>(OpLevel::GameMaster));
}

TEST(ApplyOwnerCheatsBoostTest, AlreadyOwnerStaysOwner)
{
    // 已是 Owner 的主机开作弊，仍为 Owner
    EXPECT_EQ(
        applyOwnerCheatsBoost(static_cast<mc::i32>(OpLevel::Owner), true, true), static_cast<mc::i32>(OpLevel::Owner));
}
