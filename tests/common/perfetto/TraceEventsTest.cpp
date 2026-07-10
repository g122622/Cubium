/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 * @file TraceEventsTest.cpp
 * @brief 追踪事件宏单元测试
 *
 * 测试所有 MC_TRACE_* 宏在启用和禁用追踪时的行为。
 * 注意：测试中使用的类别必须来自 TraceEvents 枚举树（TraceCategories.hpp）。
 */

#include <gtest/gtest.h>

#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceCategories.hpp"
#include "common/perfetto/TraceEvents.hpp"

using namespace mc::trace;

namespace mc {
namespace perfetto {
namespace test {

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

// ============================================================================
// 基础事件宏测试
// ============================================================================

TEST_F(TraceEventsTest, TraceScopedEventCompiles)
{
    // 测试基本作用域事件宏编译和执行（使用枚举树分类）
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "TestEvent"));
}

TEST_F(TraceEventsTest, TraceScopedEventWithArguments)
{
    // 测试带参数的事件
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "EventWithArgs", "x", 10, "y", 20));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "EventWithString", "name", "test_name"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "EventWithFloat", "value", 3.14));
}

TEST_F(TraceEventsTest, TraceCounterCompiles)
{
    // 测试计数器宏编译和执行
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "TestCounter", 42));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "ZeroCounter", 0));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "NegativeCounter", -100));
}

TEST_F(TraceEventsTest, TraceCounterTypes)
{
    // 测试不同类型的计数器值
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "IntCounter", static_cast<int64_t>(100)));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "LongCounter", static_cast<int64_t>(1000000L)));
}

TEST_F(TraceEventsTest, TraceEventBeginEnd)
{
    // 测试手动开始/结束事件
    EXPECT_NO_THROW(MC_TRACE_EVENT_BEGIN(TraceEvents.Rendering.Frame, "ManualEvent"));
    EXPECT_NO_THROW(MC_TRACE_EVENT_END(TraceEvents.Rendering.Frame));
}

TEST_F(TraceEventsTest, TraceInstantEvent)
{
    // 测试瞬时事件
    EXPECT_NO_THROW(MC_TRACE_INSTANT_EVENT(TraceEvents.Rendering.Frame, "InstantEvent"));
}

// ============================================================================
// 作用域事件测试
// ============================================================================

TEST_F(TraceEventsTest, ScopedEvent)
{
    // 作用域事件应该在作用域结束时自动结束
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ScopedEvent");
        // 事件在此作用域内
    }
    // 事件已结束
    EXPECT_TRUE(true); // 仅验证编译通过
}

TEST_F(TraceEventsTest, NestedScopedEvents)
{
    // 测试嵌套作用域事件
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

// ============================================================================
// 枚举树分类测试
// ============================================================================

TEST_F(TraceEventsTest, EnumTreeCategories)
{
    // 验证各子系统枚举分类都能正常编译执行
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerInit"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerTick"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "ClientNetwork"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "ChunkGen"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "Packet"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Game.Tick, "GameTick"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "StorageDb"));
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Fluid.Tick, "FluidTick"));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Memory.Usage, "MemUsage", 128));
}

// ============================================================================
// 多事件测试
// ============================================================================

TEST_F(TraceEventsTest, MultipleEvents)
{
    // 记录多个事件
    for (int i = 0; i < 10; ++i) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "LoopEvent", "iteration", i);
    }
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, MultipleCounters)
{
    // 记录多个计数器
    MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "Counter1", 100);
    MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "Counter2", 200);
    MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "Counter3", 300);
    EXPECT_TRUE(true);
}

// ============================================================================
// 模拟使用场景测试
// ============================================================================

TEST_F(TraceEventsTest, SimulateFrameRendering)
{
    // 模拟帧渲染流程
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Frame");

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "HandleEvents");
        // 处理事件...
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
    // 模拟区块生成流程
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
    // 模拟服务端刻流程
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerTick");

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Network.Packet, "PollNetwork");
        // 处理网络...
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "WorldUpdate");
        MC_TRACE_SCOPED_EVENT(TraceEvents.Game.Entity, "UpdateEntities");
    }

    MC_TRACE_COUNTER(TraceEvents.Server.Tick, "TPS", 20);
    EXPECT_TRUE(true);
}

// ============================================================================
// 禁用追踪时的宏测试
// ============================================================================

#if !MC_ENABLE_TRACING

TEST_F(TraceEventsTest, DisabledMacrosAreNoOps)
{
    // 当追踪禁用时，所有宏应该是空操作
    EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Event"));
    EXPECT_NO_THROW(MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "Counter", 42));
    EXPECT_NO_THROW(MC_TRACE_EVENT_BEGIN(TraceEvents.Rendering.Frame, "Event"));
    EXPECT_NO_THROW(MC_TRACE_EVENT_END(TraceEvents.Rendering.Frame));
    EXPECT_NO_THROW(MC_TRACE_INSTANT_EVENT(TraceEvents.Rendering.Frame, "Event"));
}

#endif // !MC_ENABLE_TRACING

} // namespace test
} // namespace perfetto
} // namespace mc
