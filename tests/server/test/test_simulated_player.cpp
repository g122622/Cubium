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

// SimulatedPlayer 单元测试（最小可用集，不 spawn 进真实世界）。
//
// SimulatedPlayer 是 ServerPlayer 子类，构造 ctor 不需世界指针。完整 spawn + 移动路径需
// GameTestHelper 绑定的真实 ServerWorld（由 test_gametest_server.cpp 端到端覆盖）。
// 本测试验证：
//   - 公开 ctor 不崩溃
//   - setHelper/helper 回指 getter 对称
//   - TODO stub 方法（flyToLocation/attack）调用不崩溃（占位实现）
//
// 注：构造的 SimulatedPlayer 不挂世界，故仅测元数据 + 回指，不测 moveToLocation（会经
// m_helper->worldBlockPosition 触发，NullGameTestHelper 下虽可走通但语义无意义，留端到端测）。

#include <gtest/gtest.h>

#include "common/test/framework/helper/NullGameTestHelper.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp"

#include <memory>
#include <string>

using mc::BlockPos; // BlockPos 属 mc::（非 mc::test），测试内简写

TEST(SimulatedPlayerTest, ConstructWithIdAndName)
{
    auto player =
        std::make_unique<mc::test::SimulatedPlayer>(static_cast<mc::EntityInstanceId>(1001), std::string{"TestBot"});
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->helper(), nullptr);
}

TEST(SimulatedPlayerTest, SetHelperRoundTrip)
{
    mc::test::NullGameTestHelper helper;
    auto player =
        std::make_unique<mc::test::SimulatedPlayer>(static_cast<mc::EntityInstanceId>(1002), std::string{"HelperBot"});
    ASSERT_NE(player, nullptr);

    player->setHelper(helper);
    EXPECT_EQ(player->helper(), &helper);
}

TEST(SimulatedPlayerTest, TodoStubsDoNotCrash)
{
    // flyToLocation/attack 为 TODO stub（占位实现），验证调用不崩溃。
    // TODO: 待原生侧实现飞行/攻击后改为行为断言。
    mc::test::NullGameTestHelper helper;
    auto player =
        std::make_unique<mc::test::SimulatedPlayer>(static_cast<mc::EntityInstanceId>(1003), std::string{"StubBot"});
    ASSERT_NE(player, nullptr);
    player->setHelper(helper);

    player->flyToLocation(BlockPos{0, 0, 0}, 1.0f);
    SUCCEED();
}
