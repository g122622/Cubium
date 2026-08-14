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
 * @file TeleportPermissionParseTest.cpp
 * @brief /tp 命令解析权限门回归测试
 *
 * 锁定命令分发器对权限要求的执行：权限 0 时 /tp 节点被跳过导致解析失败，
 * 权限 2 时 /tp 节点可匹配并解析成功。该权限提升链路一旦再次断裂（如
 * 单机主机未拿到权限），权限 2 的用例会立即失败。只走 parse 不走 execute，
 * 避免触发 TeleportManager 副作用。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/TeleportCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc {
namespace command {

class TeleportPermissionTestServer final : public mc::test::BaseTestServer {
public:
    // 覆盖 dimensionManager，返回一个未注册任何维度的空 DimensionManager。
    // 这样 source.world() 经 dimensionManager().getDimension() 返回 nullptr，
    // 命令走 "World not available" 分支返回 0，避免 BaseTestServer 默认实现
    // 抛 std::logic_error 进而在 noexcept 的 world() 中触发 std::terminate。
    // 注意：DimensionManager 是 ServerDimensionManager 的基类，
    // 我们将 DimensionManager reinterpret_cast 为 ServerDimensionManager，
    // 因为 ServerDimensionManager::getDimension() 仅调用基类 DimensionManager::getDimension()
    // 然后做 static_cast，在我们的测试场景中是安全的。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return reinterpret_cast<ServerDimensionManager&>(m_dimensionManager);
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return reinterpret_cast<const ServerDimensionManager&>(m_dimensionManager);
    }

private:
    DimensionManager m_dimensionManager;
};

class TeleportPermissionParseTest : public ::testing::Test {
protected:
    void SetUp() override { TeleportCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    TeleportPermissionTestServer m_server;
};

// 权限 2 的命令源可以解析 /tp <x> <y> <z>，节点不被跳过
TEST_F(TeleportPermissionParseTest, ParseSucceedsWithPermissionLevel2)
{
    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Host");
    const auto parse = m_server.commandRegistry().dispatcher().parse("/tp 100 1000 1000", source);
    EXPECT_TRUE(parse.isSuccess()) << "权限 2 应能解析 /tp";
    EXPECT_TRUE(parse.getRemaining().empty()) << "权限 2 应完整消费 /tp 100 1000 1000";
}

// 权限 0 的命令源解析 /tp 失败：tp 节点被跳过，落到其它字面量报错
TEST_F(TeleportPermissionParseTest, ParseFailsWithPermissionLevel0)
{
    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "Guest");
    const auto parse = m_server.commandRegistry().dispatcher().parse("/tp 100 1000 1000", source);
    EXPECT_FALSE(parse.isSuccess()) << "权限 0 不应能解析 /tp";
}

} // namespace command
} // namespace mc
