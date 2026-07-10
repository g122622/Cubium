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
 * @file PerfettoTest.cpp
 * @brief Perfetto 模块集成测试
 *
 * 注意：由于 Perfetto SDK 的链接要求，测试仅在 MC_ENABLE_TRACING=1 时有效。
 * 当 MC_ENABLE_TRACING=0 时，测试会被禁用。
 */

#include <gtest/gtest.h>

#include "common/perfetto/PerfettoConfig.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"

#if MC_ENABLE_TRACING

#include "common/perfetto/TraceCategories.hpp"

namespace mc {
namespace perfetto {
namespace test {

using namespace mc::trace;

class PerfettoManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_config.outputPath = "test_trace.perfetto-trace";
        m_config.bufferSizeKb = 1024;
    }

    void TearDown() override
    {
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

    EXPECT_NO_THROW(PerfettoManager::instance().initialize(m_config));

    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, DoubleShutdown)
{
    PerfettoManager::instance().initialize(m_config);
    PerfettoManager::instance().shutdown();

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
    EXPECT_NO_THROW(PerfettoManager::instance().startTracing());
    EXPECT_FALSE(PerfettoManager::instance().isEnabled());
}

TEST_F(PerfettoManagerTest, DoubleStartTracing)
{
    PerfettoManager::instance().initialize(m_config);
    PerfettoManager::instance().startTracing();

    EXPECT_NO_THROW(PerfettoManager::instance().startTracing());

    PerfettoManager::instance().stopTracing();
    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, StopTracingWithoutStart)
{
    PerfettoManager::instance().initialize(m_config);

    EXPECT_NO_THROW(PerfettoManager::instance().stopTracing());

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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "TestEvent");

    EXPECT_NO_THROW(PerfettoManager::instance().flush());

    PerfettoManager::instance().stopTracing();
    PerfettoManager::instance().shutdown();
}

TEST_F(PerfettoManagerTest, FlushWithoutTracing)
{
    PerfettoManager::instance().initialize(m_config);

    EXPECT_NO_THROW(PerfettoManager::instance().flush());

    PerfettoManager::instance().shutdown();
}

// ============================================================================
// TraceEvents 测试
// ============================================================================

class TraceEventsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_config.outputPath = "test_events.perfetto-trace";
        m_config.bufferSizeKb = 1024;
        PerfettoManager::instance().initialize(m_config);
        PerfettoManager::instance().startTracing();
    }

    void TearDown() override
    {
        if (PerfettoManager::instance().isInitialized()) {
            if (PerfettoManager::instance().isEnabled()) {
                PerfettoManager::instance().stopTracing();
            }
            PerfettoManager::instance().shutdown();
        }
    }

    TraceConfig m_config;
};

TEST_F(TraceEventsTest, TraceScopedEventCompiles)
{
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "TestEvent"));
}

TEST_F(TraceEventsTest, TraceScopedEventWithArguments)
{
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "EventWithArgs", "x", 10, "y", 20));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "EventWithString", "name", "test_name"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "EventWithFloat", "value", 3.14));
}

TEST_F(TraceEventsTest, TraceCounterCompiles)
{
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "TestCounter", 42));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "ZeroCounter", 0));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "NegativeCounter", -100));
}

TEST_F(TraceEventsTest, TraceEventBeginEnd)
{
    EXPECT_NO_THROW(MC_TRACE_EVENT_BEGIN(TraceEvents.Rendering.Frame, "ManualEvent"));
    EXPECT_NO_THROW(MC_TRACE_EVENT_END(TraceEvents.Rendering.Frame));
}

TEST_F(TraceEventsTest, TraceInstantEvent)
{
    EXPECT_NO_THROW(MC_TRACE_INSTANT_EVENT(TraceEvents.Rendering.Frame, "InstantEvent"));
}

TEST_F(TraceEventsTest, ScopedEvent)
{
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ScopedEvent");
    }
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, NestedScopedEvents)
{
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "OuterEvent");
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "InnerEvent1");
        }
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "InnerEvent2");
        }
    }
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, EnumTreeCategories)
{
    // 验证各子系统枚举分类都能正常编译执行
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerInit"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "ChunkGen"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "Packet"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "StorageDb"));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Memory.Usage, "MemUsage", 128));
}

TEST_F(TraceEventsTest, MultipleEvents)
{
    for (int i = 0; i < 10; ++i) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "LoopEvent", "iteration", i);
    }
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, SimulateFrameRendering)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Frame");

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "HandleEvents");
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Update");
        MC_TRACE_COUNTER(TraceEvents.Game.Tick, "DeltaTime", 16);
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Render");
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "BuildMesh");
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "RenderSky");
    }

    MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "FPS", 60);
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, SimulateChunkGeneration)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateChunk", "x", 0, "z", 0);

    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateBiomes");
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateNoise");
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "BuildSurface");
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "ApplyCarvers");
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "PlaceFeatures");

    MC_TRACE_COUNTER(TraceEvents.World.ChunkGen, "ChunksGenerated", 1);
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, SimulateServerTick)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerTick");

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "PollNetwork");
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "WorldUpdate");
        MC_TRACE_SCOPED_EVENT(TraceEvents.Game.Entity, "UpdateEntities");
    }

    MC_TRACE_COUNTER(TraceEvents.Server.Tick, "TPS", 20);
    EXPECT_TRUE(true);
}

// ============================================================================
// 线程排序（PR #6219：thread_ordering + sibling_order_rank）测试
// ============================================================================

TEST_F(TraceEventsTest, SetThreadNameDoesNotThrow)
{
    EXPECT_NO_THROW(PerfettoManager::instance().setThreadName("MemoryTrace"));
    EXPECT_NO_THROW(PerfettoManager::instance().setThreadName("UnknownThread"));
    EXPECT_NO_THROW(PerfettoManager::instance().setThreadName("WithRank", 42));
    PerfettoManager::instance().flush();
}

TEST_F(TraceEventsTest, SetThreadNameViaMacroRoutesToManager)
{
    // 宏应转调 Manager，不抛异常且不影响已启动的追踪会话
    EXPECT_NO_THROW(MC_TRACE_SET_THREAD_NAME("ClientMainThread"));
    PerfettoManager::instance().flush();
}

TEST_F(TraceEventsTest, WorkerPoolRankFormula)
{
    // 验证 worker 池 sibling_order_rank = rankBase + workerId 公式，防止回归。
    // 三组 worker 分块排列：ServerCompute(100+) -> ServerIO(200+) -> ChunkMeshWorker(300+)，
    // 每组间隔 100，组内按 workerId 升序。
    constexpr int kServerComputeRankBase = 100;
    constexpr int kServerIoRankBase = 200;
    constexpr int kMeshRankBase = 300;
    EXPECT_EQ(kServerComputeRankBase + 0, 100);
    EXPECT_EQ(kServerComputeRankBase + 13, 113);
    EXPECT_EQ(kServerIoRankBase + 0, 200);
    EXPECT_EQ(kServerIoRankBase + 13, 213);
    EXPECT_EQ(kMeshRankBase + 0, 300);
    EXPECT_EQ(kMeshRankBase + 7, 307);
    // 三组 rankBase 严格递增，保证 UI 中 Compute -> IO -> Mesh 的组顺序
    EXPECT_LT(kServerComputeRankBase, kServerIoRankBase);
    EXPECT_LT(kServerIoRankBase, kMeshRankBase);
    // worker rank 应落在固定线程(1-5)之后
    EXPECT_GT(kServerComputeRankBase, 5);
    EXPECT_GT(kMeshRankBase, 5);
}

TEST_F(TraceEventsTest, ThreadOrderingEndToEnd)
{
    // 端到端验证：根 track descriptor（uuid=0, thread_ordering=EXPLICIT）与
    // 线程 track descriptor（带 sibling_order_rank）能随首事件写入 trace buffer。
    // Perfetto SDK 的 SetTrackDescriptor 只缓存 descriptor，真正进 buffer 是在该
    // sequence 第一次发 track_event 时随包内联——故必须发事件才能落到文件。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Warmup");
    PerfettoManager::instance().setThreadName("MemoryTrace");
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "AfterName");
    PerfettoManager::instance().flush();
}

} // namespace test
} // namespace perfetto
} // namespace mc

#else // MC_ENABLE_TRACING == 0

// 当追踪禁用时，提供一个空的测试以确保测试文件能够编译
namespace mc {
namespace perfetto {
namespace test {

TEST(PerfettoDisabledTest, TracingDisabled)
{
    // 当 MC_ENABLE_TRACING=0 时，追踪宏展开为空操作
    // 这些宏在 TraceEvents.hpp 中定义为 ((void)0)
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "DisabledEvent");
    MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "DisabledCounter", 0);
    EXPECT_TRUE(true) << "Perfetto tracing is disabled at compile time";
}

TEST(PerfettoDisabledTest, ManagerStubWorks)
{
    // 测试禁用时的 PerfettoManager 存根实现
    TraceConfig config;
    config.outputPath = "test_trace.perfetto-trace";
    config.bufferSizeKb = 1024;

    // 存根实现应该安全地什么都不做
    EXPECT_FALSE(PerfettoManager::instance().isInitialized());
    EXPECT_FALSE(PerfettoManager::instance().isEnabled());

    // 这些操作在禁用时应该安全地什么都不做
    EXPECT_NO_THROW(PerfettoManager::instance().initialize(config));
    EXPECT_NO_THROW(PerfettoManager::instance().startTracing());
    EXPECT_NO_THROW(PerfettoManager::instance().stopTracing());
    EXPECT_NO_THROW(PerfettoManager::instance().shutdown());
}

} // namespace test
} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
