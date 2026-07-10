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
 * @file PerfettoManagerTest.cpp
 * @brief PerfettoManager 单元测试
 */

#include <gtest/gtest.h>

#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceCategories.hpp"
#include "common/perfetto/TraceEvents.hpp"

namespace mc {
namespace perfetto {
namespace test {

class PerfettoManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试使用唯一的输出文件
        m_config.outputPath = "test_trace.perfetto-trace";
        m_config.bufferSizeKb = 1024; // 1 MB 测试缓冲区
    }

    void TearDown() override
    {
        // 确保每个测试后清理
        if (PerfettoManager::instance().isInitialized()) {
            if (PerfettoManager::instance().isEnabled()) {
                PerfettoManager::instance().stopTracing();
            }
            PerfettoManager::instance().shutdown();
        }
    }

    TraceConfig m_config;
};

// ============================================================================
// 基础功能测试
// ============================================================================

TEST_F(PerfettoManagerTest, SingletonPattern)
{
    auto& instance1 = PerfettoManager::instance();
    auto& instance2 = PerfettoManager::instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(PerfettoManagerTest, InitializeAndShutdown)
{
    EXPECT_FALSE(PerfettoManager::instance().isInitialized());

    PerfettoManager::instance().initialize(m_config);
    EXPECT_TRUE(PerfettoManager::instance().isInitialized());

    PerfettoManager::instance().shutdown();
    EXPECT_FALSE(PerfettoManager::instance().isInitialized());
}

TEST_F(PerfettoManagerTest, DoubleInitialize)
{
    PerfettoManager::instance().initialize(m_config);
    EXPECT_TRUE(PerfettoManager::instance().isInitialized());

    // 二次初始化应该安全忽略
    EXPECT_NO_THROW(PerfettoManager::instance().initialize(m_config));

    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, DoubleShutdown)
{
    PerfettoManager::instance().initialize(m_config);
    PerfettoManager::instance().shutdown();

    // 二次关闭应该安全
    EXPECT_NO_THROW(PerfettoManager::instance().shutdown());
}

// ============================================================================
// 追踪会话测试
// ============================================================================

TEST_F(PerfettoManagerTest, StartStopTracing)
{
    PerfettoManager::instance().initialize(m_config);

    EXPECT_FALSE(PerfettoManager::instance().isEnabled());

    PerfettoManager::instance().startTracing();
    EXPECT_TRUE(PerfettoManager::instance().isEnabled());

    PerfettoManager::instance().stopTracing();
    EXPECT_FALSE(PerfettoManager::instance().isEnabled());

    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, StartTracingWithoutInitialize)
{
    // 未初始化时启动追踪应该安全失败
    EXPECT_NO_THROW(PerfettoManager::instance().startTracing());
    EXPECT_FALSE(PerfettoManager::instance().isEnabled());
}

TEST_F(PerfettoManagerTest, DoubleStartTracing)
{
    PerfettoManager::instance().initialize(m_config);
    PerfettoManager::instance().startTracing();

    // 二次启动应该安全忽略
    EXPECT_NO_THROW(PerfettoManager::instance().startTracing());

    PerfettoManager::instance().stopTracing();
    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, StopTracingWithoutStart)
{
    PerfettoManager::instance().initialize(m_config);

    // 未启动追踪时停止应该安全
    EXPECT_NO_THROW(PerfettoManager::instance().stopTracing());

    PerfettoManager::instance().shutdown();
}

// ============================================================================
// 运行时控制测试
// ============================================================================

TEST_F(PerfettoManagerTest, RuntimeEnableDisable)
{
    PerfettoManager::instance().initialize(m_config);
    PerfettoManager::instance().startTracing();

    EXPECT_TRUE(PerfettoManager::instance().isEnabled());

    PerfettoManager::instance().setEnabled(false);
    // 注意：setEnabled 不影响 isEnabled()，它只控制是否记录事件
    // isEnabled() 返回 m_initialized && m_enabled && m_tracing

    PerfettoManager::instance().setEnabled(true);

    PerfettoManager::instance().stopTracing();
    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, ConfigAccess)
{
    PerfettoManager::instance().initialize(m_config);

    const auto& config = PerfettoManager::instance().config();
    EXPECT_EQ(config.outputPath, m_config.outputPath);
    EXPECT_EQ(config.bufferSizeKb, m_config.bufferSizeKb);

    PerfettoManager::instance().shutdown();
}

// ============================================================================
// Flush 测试
// ============================================================================

TEST_F(PerfettoManagerTest, Flush)
{
    PerfettoManager::instance().initialize(m_config);
    PerfettoManager::instance().startTracing();

    // 记录一些事件（使用枚举树分类）
    MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Rendering.Frame, "TestEvent");

    // 刷新应该成功
    EXPECT_NO_THROW(PerfettoManager::instance().flush());

    PerfettoManager::instance().stopTracing();
    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, FlushWithoutTracing)
{
    PerfettoManager::instance().initialize(m_config);

    // 未启动追踪时刷新应该安全
    EXPECT_NO_THROW(PerfettoManager::instance().flush());

    PerfettoManager::instance().shutdown();
}

// ============================================================================
// 禁用追踪时的存根测试
// ============================================================================

#if !MC_ENABLE_TRACING

TEST_F(PerfettoManagerTest, StubImplementation)
{
    // 当追踪禁用时，所有操作应该是空操作
    EXPECT_FALSE(PerfettoManager::instance().isInitialized());
    EXPECT_FALSE(PerfettoManager::instance().isEnabled());

    PerfettoManager::instance().initialize(m_config);
    EXPECT_FALSE(PerfettoManager::instance().isInitialized());

    PerfettoManager::instance().startTracing();
    EXPECT_FALSE(PerfettoManager::instance().isEnabled());

    PerfettoManager::instance().stopTracing();
    PerfettoManager::instance().shutdown();
}

#endif // !MC_ENABLE_TRACING

} // namespace test
} // namespace perfetto
} // namespace mc
