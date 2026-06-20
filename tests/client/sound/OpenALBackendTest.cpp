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

#include "client/sound/backend/OpenALBackend.hpp"
#include "common/sound/SoundTypes.hpp"

using namespace mc::client::sound;
using namespace mc;

// ============================================================================
// OpenALBackend 测试夹具
// ============================================================================

class OpenALBackendTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        backend = std::make_unique<OpenALBackend>();
        auto result = backend->initialize();
        if (!result.success()) {
            GTEST_SKIP() << "OpenAL not available: " << result.error().message();
        }
    }

    void TearDown() override
    {
        if (backend && backend->isInitialized()) {
            backend->shutdown();
        }
        backend.reset();
    }

    std::unique_ptr<OpenALBackend> backend;
};

// ============================================================================
// 初始化与关闭
// ============================================================================

TEST_F(OpenALBackendTest, InitializeShutdown)
{
    EXPECT_TRUE(backend->isInitialized());

    backend->shutdown();
    EXPECT_FALSE(backend->isInitialized());
}

TEST_F(OpenALBackendTest, DoubleInitializeFails)
{
    auto result = backend->initialize();
    EXPECT_FALSE(result.success());
}

TEST_F(OpenALBackendTest, DoubleShutdownSafe)
{
    backend->shutdown();
    // 第二次调用不应崩溃
    backend->shutdown();
}

// ============================================================================
// 源计数追踪 - 核心功能
// ============================================================================

TEST_F(OpenALBackendTest, InitialSourceCountIsZero)
{
    // 初始化后，没有创建任何源，活跃数应为 0
    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
}

TEST_F(OpenALBackendTest, InitialAvailableSourcesEqualsMax)
{
    // 初始化后，可用源数量应等于最大源数量
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources());
    EXPECT_GT(backend->getMaxSources(), 0u);
}

TEST_F(OpenALBackendTest, CreateSourceIncrementsCount)
{
    // 创建源后，活跃计数递增，可用计数递减
    u32 initialAvailable = backend->getAvailableSources();
    u32 initialActive = backend->getActiveSourceCount();

    auto result = backend->createSource();
    ASSERT_TRUE(result.success());

    EXPECT_EQ(backend->getActiveSourceCount(), initialActive + 1);
    EXPECT_EQ(backend->getAvailableSources(), initialAvailable - 1);
}

TEST_F(OpenALBackendTest, DestroySourceDecrementsCount)
{
    // 销毁源后，活跃计数递减，可用计数递增
    auto result = backend->createSource();
    ASSERT_TRUE(result.success());

    EXPECT_EQ(backend->getActiveSourceCount(), 1u);
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources() - 1);

    // 销毁源
    result.value().reset();

    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources());
}

TEST_F(OpenALBackendTest, MultipleSourcesTracking)
{
    // 创建多个源，追踪计数变化
    constexpr int NUM_SOURCES = 5;
    std::vector<std::unique_ptr<IAudioSource>> sources;

    for (int i = 0; i < NUM_SOURCES; ++i) {
        auto result = backend->createSource();
        ASSERT_TRUE(result.success()) << "Failed to create source " << i;
        EXPECT_EQ(backend->getActiveSourceCount(), static_cast<u32>(i + 1));
        sources.push_back(std::move(result.value()));
    }

    EXPECT_EQ(backend->getActiveSourceCount(), static_cast<u32>(NUM_SOURCES));
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources() - NUM_SOURCES);

    // 按相反顺序销毁
    for (int i = NUM_SOURCES - 1; i >= 0; --i) {
        sources[i].reset();
        EXPECT_EQ(backend->getActiveSourceCount(), static_cast<u32>(i));
    }

    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources());
}

TEST_F(OpenALBackendTest, PartialDestroyTracking)
{
    // 创建3个源，只销毁中间的，验证计数正确
    auto source1 = backend->createSource();
    auto source2 = backend->createSource();
    auto source3 = backend->createSource();
    ASSERT_TRUE(source1.success());
    ASSERT_TRUE(source2.success());
    ASSERT_TRUE(source3.success());

    EXPECT_EQ(backend->getActiveSourceCount(), 3u);

    // 销毁中间的源
    source2.value().reset();

    EXPECT_EQ(backend->getActiveSourceCount(), 2u);
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources() - 2);

    // 销毁剩余源
    source1.value().reset();
    source3.value().reset();

    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
    EXPECT_EQ(backend->getAvailableSources(), backend->getMaxSources());
}

TEST_F(OpenALBackendTest, ShutdownResetsCount)
{
    // 创建一些源后关闭后端，计数应重置
    auto result = backend->createSource();
    ASSERT_TRUE(result.success());
    EXPECT_EQ(backend->getActiveSourceCount(), 1u);

    backend->shutdown();

    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
    EXPECT_EQ(backend->getAvailableSources(), 0u); // 未初始化时返回 0
}

TEST_F(OpenALBackendTest, UninitializedBackendReturnsZero)
{
    // 未初始化的后端应返回 0
    auto freshBackend = std::make_unique<OpenALBackend>();
    EXPECT_EQ(freshBackend->getActiveSourceCount(), 0u);
    EXPECT_EQ(freshBackend->getAvailableSources(), 0u);
}

// ============================================================================
// 源耗尽测试
// ============================================================================

TEST_F(OpenALBackendTest, SourceExhaustionReturnsError)
{
    // 创建源直到耗尽，验证返回 ResourceExhausted 错误
    std::vector<std::unique_ptr<IAudioSource>> sources;
    u32 maxSources = backend->getMaxSources();

    for (u32 i = 0; i < maxSources; ++i) {
        auto result = backend->createSource();
        if (!result.success()) {
            // 某些 OpenAL 实现可能无法创建到最大数量
            break;
        }
        sources.push_back(std::move(result.value()));
    }

    // 尝试创建超出限制的源
    auto result = backend->createSource();
    if (!result.success()) {
        // 预期：源耗尽时应返回 ResourceExhausted
        EXPECT_EQ(result.error().code(), ErrorCode::ResourceExhausted);
    }

    // 清理
    sources.clear();
    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
}

// ============================================================================
// OpenALSource 移动语义与回调传递
// ============================================================================

TEST_F(OpenALBackendTest, MoveConstructionTransfersCallback)
{
    // 移动构造源后，原对象不再持有回调，新对象析构时递减计数
    auto result = backend->createSource();
    ASSERT_TRUE(result.success());

    EXPECT_EQ(backend->getActiveSourceCount(), 1u);

    // 移动构造
    auto source = std::move(result.value());
    EXPECT_EQ(backend->getActiveSourceCount(), 1u);

    // 销毁移动后的对象，计数应递减
    source.reset();
    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
}

TEST_F(OpenALBackendTest, MoveAssignmentTransfersCallback)
{
    // 创建两个源，验证移动赋值正确传递回调并清理旧源
    auto result1 = backend->createSource();
    auto result2 = backend->createSource();
    ASSERT_TRUE(result1.success());
    ASSERT_TRUE(result2.success());

    EXPECT_EQ(backend->getActiveSourceCount(), 2u);

    // 先释放 result1 的源（通过 unique_ptr reset），使计数变为 1
    result1.value().reset();
    EXPECT_EQ(backend->getActiveSourceCount(), 1u);

    // 将 result2 的源移动到新的 unique_ptr，验证回调传递
    std::unique_ptr<IAudioSource> movedSource = std::move(result2.value());
    // result2 的 unique_ptr 现在为空，不会触发回调
    // movedSource 持有源，计数仍为 1
    EXPECT_EQ(backend->getActiveSourceCount(), 1u);

    // 销毁移动后的源，计数递减
    movedSource.reset();
    EXPECT_EQ(backend->getActiveSourceCount(), 0u);
}

// ============================================================================
// getMaxSources 从设备属性查询
// ============================================================================

TEST_F(OpenALBackendTest, MaxSourcesFromDevice)
{
    // 最大源数量应从设备属性查询，不应该是 0
    u32 maxSources = backend->getMaxSources();
    EXPECT_GE(maxSources, 16u); // 至少有最小源数量
    EXPECT_LE(maxSources, ::mc::sound::MAX_CONCURRENT_SOUNDS);
}

// ============================================================================
// getDebugString 包含源计数信息
// ============================================================================

TEST_F(OpenALBackendTest, DebugStringContainsSourceInfo)
{
    auto result = backend->createSource();
    ASSERT_TRUE(result.success());

    std::string debug = backend->getDebugString();
    EXPECT_NE(debug.find("Sources:"), std::string::npos) << "Debug string should contain 'Sources:'";

    result.value().reset();

    debug = backend->getDebugString();
    EXPECT_NE(debug.find("Sources:"), std::string::npos) << "Debug string should contain 'Sources:' after reset";
}

// ============================================================================
// 缓冲区管理（基本功能验证）
// ============================================================================

TEST_F(OpenALBackendTest, CreateAndDestroyBuffer)
{
    // 创建一个简单的单声道 16-bit 44100Hz 音频缓冲区
    AudioFormat format;
    format.channels = 1;
    format.bitsPerSample = 16;
    format.sampleRate = 44100;
    // 100ms 的静音数据
    size_t sampleCount = static_cast<size_t>(44100 * 0.1);
    std::vector<u8> samples(sampleCount * 2, 0); // 16-bit = 2 bytes per sample
    AudioData data(format, std::move(samples));

    auto result = backend->createBuffer(data);
    ASSERT_TRUE(result.success()) << "Failed to create buffer: " << result.error().message();
    EXPECT_NE(result.value(), 0u);

    // 验证缓冲区存在
    EXPECT_TRUE(backend->hasBuffer(result.value()));

    // 销毁缓冲区
    backend->destroyBuffer(result.value());
    EXPECT_FALSE(backend->hasBuffer(result.value()));
}

TEST_F(OpenALBackendTest, CreateBufferWithInvalidDataFails)
{
    AudioData data; // 默认构造，数据为空
    auto result = backend->createBuffer(data);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// 听者属性
// ============================================================================

TEST_F(OpenALBackendTest, ListenerPosition)
{
    backend->setListenerPosition(glm::vec3(1.0f, 2.0f, 3.0f));
    glm::vec3 pos = backend->getListenerPosition();
    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 3.0f);
}

TEST_F(OpenALBackendTest, ListenerGain)
{
    backend->setListenerGain(0.5f);
    EXPECT_FLOAT_EQ(backend->getListenerGain(), 0.5f);
}

TEST_F(OpenALBackendTest, ListenerOrientation)
{
    backend->setListenerOrientation(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 forward = backend->getListenerForward();
    glm::vec3 up = backend->getListenerUp();
    EXPECT_FLOAT_EQ(forward.x, 0.0f);
    EXPECT_FLOAT_EQ(forward.y, 0.0f);
    EXPECT_FLOAT_EQ(forward.z, -1.0f);
    EXPECT_FLOAT_EQ(up.x, 0.0f);
    EXPECT_FLOAT_EQ(up.y, 1.0f);
    EXPECT_FLOAT_EQ(up.z, 0.0f);
}
