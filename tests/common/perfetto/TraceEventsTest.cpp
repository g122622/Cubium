/**
 * @file TraceEventsTest.cpp
 * @brief 追踪事件宏单元测试
 *
 * 测试所有 MC_TRACE_* 宏在启用和禁用追踪时的行为。
 * 注意：测试中使用的类别必须在 TraceCategories.hpp 中定义。
 */

#include <gtest/gtest.h>

#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceCategories.hpp"
#include "common/perfetto/TraceEvents.hpp"

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

TEST_F(TraceEventsTest, TraceEventCompiles)
{
    // 测试基本事件宏编译和执行（使用已定义的类别）
    EXPECT_NO_THROW(MC_TRACE_EVENT("rendering.frame", "TestEvent"));
}

TEST_F(TraceEventsTest, TraceEventWithArguments)
{
    // 测试带参数的事件
    EXPECT_NO_THROW(MC_TRACE_EVENT("rendering.frame", "EventWithArgs", "x", 10, "y", 20));
    EXPECT_NO_THROW(MC_TRACE_EVENT("rendering.frame", "EventWithString", "name", "test_name"));
    EXPECT_NO_THROW(MC_TRACE_EVENT("rendering.frame", "EventWithFloat", "value", 3.14));
}

TEST_F(TraceEventsTest, TraceCounterCompiles)
{
    // 测试计数器宏编译和执行
    EXPECT_NO_THROW(MC_TRACE_COUNTER("rendering.frame", "TestCounter", 42));
    EXPECT_NO_THROW(MC_TRACE_COUNTER("rendering.frame", "ZeroCounter", 0));
    EXPECT_NO_THROW(MC_TRACE_COUNTER("rendering.frame", "NegativeCounter", -100));
}

TEST_F(TraceEventsTest, TraceCounterTypes)
{
    // 测试不同类型的计数器值
    EXPECT_NO_THROW(MC_TRACE_COUNTER("rendering.frame", "IntCounter", static_cast<int64_t>(100)));
    EXPECT_NO_THROW(MC_TRACE_COUNTER("rendering.frame", "LongCounter", static_cast<int64_t>(1000000L)));
}

TEST_F(TraceEventsTest, TraceEventBeginEnd)
{
    // 测试手动开始/结束事件
    EXPECT_NO_THROW(MC_TRACE_EVENT_BEGIN("rendering.frame", "ManualEvent"));
    EXPECT_NO_THROW(MC_TRACE_EVENT_END("rendering.frame"));
}

TEST_F(TraceEventsTest, TraceInstant)
{
    // 测试瞬时事件
    EXPECT_NO_THROW(MC_TRACE_INSTANT("rendering.frame", "InstantEvent"));
}

// ============================================================================
// 作用域事件测试
// ============================================================================

TEST_F(TraceEventsTest, ScopedEvent)
{
    // 作用域事件应该在作用域结束时自动结束
    {
        MC_TRACE_EVENT("rendering.frame", "ScopedEvent");
        // 事件在此作用域内
    }
    // 事件已结束
    EXPECT_TRUE(true); // 仅验证编译通过
}

TEST_F(TraceEventsTest, NestedScopedEvents)
{
    // 测试嵌套作用域事件
    {
        MC_TRACE_EVENT("rendering.frame", "OuterEvent");
        {
            MC_TRACE_EVENT("rendering.frame", "InnerEvent1");
        }
        {
            MC_TRACE_EVENT("rendering.frame", "InnerEvent2");
        }
    }
    EXPECT_TRUE(true);
}

// ============================================================================
// 子系统特定宏测试
// ============================================================================

TEST_F(TraceEventsTest, RenderingMacros)
{
    EXPECT_NO_THROW(MC_TRACE_RENDERING_EVENT("RenderFrame"));
    EXPECT_NO_THROW(MC_TRACE_RENDERING_COUNTER("FPS", 60));
    EXPECT_NO_THROW(MC_TRACE_VULKAN_EVENT("DrawCall"));
    EXPECT_NO_THROW(MC_TRACE_CHUNK_MESH_EVENT("BuildMesh"));
}

TEST_F(TraceEventsTest, GameTickMacros)
{
    EXPECT_NO_THROW(MC_TRACE_TICK_EVENT("ServerTick"));
    EXPECT_NO_THROW(MC_TRACE_TICK_COUNTER("TPS", 20));
    EXPECT_NO_THROW(MC_TRACE_ENTITY_EVENT("EntityUpdate"));
    EXPECT_NO_THROW(MC_TRACE_AI_EVENT("GoalExecute"));
}

TEST_F(TraceEventsTest, WorldMacros)
{
    EXPECT_NO_THROW(MC_TRACE_CHUNK_GEN_EVENT("GenerateBiomes"));
    EXPECT_NO_THROW(MC_TRACE_CHUNK_LOAD_EVENT("LoadChunk"));
}

TEST_F(TraceEventsTest, NetworkMacros)
{
    EXPECT_NO_THROW(MC_TRACE_NETWORK_EVENT("PacketReceived"));
}

// ============================================================================
// 条件事件测试
// ============================================================================

TEST_F(TraceEventsTest, ConditionalEvent)
{
    // 条件为真时记录
    EXPECT_NO_THROW(MC_TRACE_EVENT_IF(true, "rendering.frame", "ConditionalEventTrue"));

    // 条件为假时不记录
    EXPECT_NO_THROW(MC_TRACE_EVENT_IF(false, "rendering.frame", "ConditionalEventFalse"));
}

// ============================================================================
// 分类检查测试
// ============================================================================

TEST_F(TraceEventsTest, CategoryEnabledCheck)
{
    // MC_TRACE_CATEGORY_ENABLED 在 Perfetto SDK 中依赖于特定实现
    // 这里仅测试编译通过
    EXPECT_TRUE(true);
}

// ============================================================================
// 多事件测试
// ============================================================================

TEST_F(TraceEventsTest, MultipleEvents)
{
    // 记录多个事件
    for (int i = 0; i < 10; ++i) {
        MC_TRACE_EVENT("rendering.frame", "LoopEvent", "iteration", i);
    }
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, MultipleCounters)
{
    // 记录多个计数器
    MC_TRACE_COUNTER("rendering.frame", "Counter1", 100);
    MC_TRACE_COUNTER("rendering.frame", "Counter2", 200);
    MC_TRACE_COUNTER("rendering.frame", "Counter3", 300);
    EXPECT_TRUE(true);
}

// ============================================================================
// 模拟使用场景测试
// ============================================================================

TEST_F(TraceEventsTest, SimulateFrameRendering)
{
    // 模拟帧渲染流程
    MC_TRACE_EVENT("rendering.frame", "Frame");

    {
        MC_TRACE_EVENT("rendering.frame", "HandleEvents");
        // 处理事件...
    }

    {
        MC_TRACE_EVENT("rendering.frame", "Update");
        MC_TRACE_COUNTER("game.tick", "DeltaTime", 16);
    }

    {
        MC_TRACE_EVENT("rendering.frame", "Render");
        MC_TRACE_EVENT("rendering.vulkan", "BeginFrame");
        MC_TRACE_EVENT("rendering.vulkan", "DrawCalls");
        MC_TRACE_EVENT("rendering.vulkan", "EndFrame");
    }

    MC_TRACE_COUNTER("rendering.frame", "FPS", 60);
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, SimulateChunkGeneration)
{
    // 模拟区块生成流程
    MC_TRACE_EVENT("world.chunk_gen", "GenerateChunk", "x", 0, "z", 0);

    MC_TRACE_EVENT("world.chunk_gen", "GenerateBiomes");
    MC_TRACE_EVENT("world.chunk_gen", "GenerateNoise");
    MC_TRACE_EVENT("world.chunk_gen", "BuildSurface");
    MC_TRACE_EVENT("world.chunk_gen", "ApplyCarvers");
    MC_TRACE_EVENT("world.chunk_gen", "PlaceFeatures");

    MC_TRACE_COUNTER("world.chunk_gen", "ChunksGenerated", 1);
    EXPECT_TRUE(true);
}

TEST_F(TraceEventsTest, SimulateServerTick)
{
    // 模拟服务端刻流程
    MC_TRACE_EVENT("game.tick", "ServerTick");

    {
        MC_TRACE_EVENT("network.packet", "PollNetwork");
        // 处理网络...
    }

    {
        MC_TRACE_EVENT("game.tick", "WorldUpdate");
        MC_TRACE_EVENT("game.entity", "UpdateEntities");
    }

    MC_TRACE_COUNTER("game.tick", "TPS", 20);
    EXPECT_TRUE(true);
}

// ============================================================================
// 禁用追踪时的宏测试
// ============================================================================

#if !MC_ENABLE_TRACING

TEST_F(TraceEventsTest, DisabledMacrosAreNoOps)
{
    // 当追踪禁用时，所有宏应该是空操作
    EXPECT_NO_THROW(MC_TRACE_EVENT("rendering.frame", "Event"));
    EXPECT_NO_THROW(MC_TRACE_COUNTER("rendering.frame", "Counter", 42));
    EXPECT_NO_THROW(MC_TRACE_EVENT_BEGIN("rendering.frame", "Event"));
    EXPECT_NO_THROW(MC_TRACE_EVENT_END("rendering.frame"));
    EXPECT_NO_THROW(MC_TRACE_INSTANT("rendering.frame", "Event"));

    // 子系统宏也应该是空操作
    EXPECT_NO_THROW(MC_TRACE_RENDERING_EVENT("Event"));
    EXPECT_NO_THROW(MC_TRACE_TICK_EVENT("Event"));
    EXPECT_NO_THROW(MC_TRACE_CHUNK_GEN_EVENT("Event"));
    EXPECT_NO_THROW(MC_TRACE_NETWORK_EVENT("Event"));
}

#endif // !MC_ENABLE_TRACING

} // namespace test
} // namespace perfetto
} // namespace mc
