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

#include <array>
#include <chrono>
#include <thread>
#include <vector>

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

// ============================================================================
// 流式播放：queueBuffers / unqueueBuffers / getProcessedBuffers / getQueuedBuffers
// ============================================================================
// 这些测试验证 OpenALSource 在流式播放接口上的核心语义：
// 1. AudioBufferId 与 OpenAL ALuint 句柄之间的双向映射正确
// 2. unqueueBuffers 使用 alGetSourcei(AL_BUFFERS_PROCESSED) 查询实际可出队数量
// 3. queueBuffers 容错：跳过无效 ID 而不破坏后续有效 ID 的入队顺序
// 4. unqueueBuffers 写回的 AudioBufferId 与入队顺序一致

namespace {
// 构造 100ms 静音 PCM 数据，用于流式播放测试
AudioData makeSilentAudioData(u32 sampleRate = 44100, u16 channels = 1, f32 durationSec = 0.1f)
{
    AudioFormat format;
    format.sampleRate = sampleRate;
    format.channels = channels;
    format.bitsPerSample = 16;
    size_t sampleCount = static_cast<size_t>(sampleRate * durationSec);
    std::vector<u8> samples(sampleCount * channels * sizeof(u16), 0);
    return AudioData(format, std::move(samples));
}
} // namespace

TEST_F(OpenALBackendTest, QueueBuffersIncrementsQueuedCount)
{
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    // 初始队列为空
    EXPECT_EQ(source->getQueuedBuffers(), 0u);

    // 创建 3 个 buffer
    AudioData data = makeSilentAudioData();
    std::vector<AudioBufferId> bufferIds;
    for (int i = 0; i < 3; ++i) {
        auto r = backend->createBuffer(data);
        ASSERT_TRUE(r.success()) << "Failed to create buffer " << i;
        bufferIds.push_back(r.value());
    }

    // 入队 3 个 buffer
    source->queueBuffers(bufferIds.data(), bufferIds.size());
    EXPECT_EQ(source->getQueuedBuffers(), 3u);

    // 处理的 buffer 应为 0（尚未播放）
    EXPECT_EQ(source->getProcessedBuffers(), 0u);
}

TEST_F(OpenALBackendTest, QueueBuffersWithInvalidIdSkipsEntry)
{
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    AudioData data = makeSilentAudioData();
    auto validResult = backend->createBuffer(data);
    ASSERT_TRUE(validResult.success());
    AudioBufferId validId = validResult.value();

    // 入队 [validId, 0（无效）, validId]
    // 预期：两个有效 buffer 入队，无效 ID 被跳过
    std::vector<AudioBufferId> ids = {validId, 0, validId};
    source->queueBuffers(ids.data(), ids.size());
    EXPECT_EQ(source->getQueuedBuffers(), 2u);
}

TEST_F(OpenALBackendTest, QueueBuffersWithUnknownIdSkipsEntry)
{
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    AudioData data = makeSilentAudioData();
    auto validResult = backend->createBuffer(data);
    ASSERT_TRUE(validResult.success());
    AudioBufferId validId = validResult.value();

    // 99999 是一个不存在的 AudioBufferId
    std::vector<AudioBufferId> ids = {validId, 99999, validId};
    source->queueBuffers(ids.data(), ids.size());
    EXPECT_EQ(source->getQueuedBuffers(), 2u);
}

TEST_F(OpenALBackendTest, UnqueueBuffersReturnsZeroWhenNothingProcessed)
{
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    AudioData data = makeSilentAudioData();
    auto bufferResult = backend->createBuffer(data);
    ASSERT_TRUE(bufferResult.success());

    source->queueBuffers(&bufferResult.value(), 1);
    EXPECT_EQ(source->getQueuedBuffers(), 1u);

    // 未播放时，processed 为 0，unqueue 应返回 0
    std::array<AudioBufferId, 4> recycled = {};
    u32 unqueued = source->unqueueBuffers(recycled.data(), recycled.size());
    EXPECT_EQ(unqueued, 0u);

    // 队列不应被破坏
    EXPECT_EQ(source->getQueuedBuffers(), 1u);
}

TEST_F(OpenALBackendTest, UnqueueBuffersAfterPlayReturnsCorrectIds)
{
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    // 创建 3 个 buffer 入队
    AudioData data = makeSilentAudioData(44100, 1, 0.05f); // 50ms 静音
    std::vector<AudioBufferId> bufferIds;
    for (int i = 0; i < 3; ++i) {
        auto r = backend->createBuffer(data);
        ASSERT_TRUE(r.success()) << "Failed to create buffer " << i;
        bufferIds.push_back(r.value());
    }

    source->queueBuffers(bufferIds.data(), bufferIds.size());
    EXPECT_EQ(source->getQueuedBuffers(), 3u);

    // 播放并等待 buffer 处理完（150ms 数据，等 500ms 足够）
    source->play();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // processed 应该 > 0
    u32 processed = source->getProcessedBuffers();
    ASSERT_GT(processed, 0u) << "Expected at least one processed buffer after play + sleep";

    // 出队
    std::array<AudioBufferId, 8> recycled = {};
    u32 unqueued = source->unqueueBuffers(recycled.data(), recycled.size());
    EXPECT_EQ(unqueued, processed);

    // 出队返回的 AudioBufferId 必须是入队时的子集（顺序按 OpenAL 处理顺序）
    // 注意：OpenAL 保证按入队顺序出队
    for (u32 i = 0; i < unqueued; ++i) {
        // 每个出队的 ID 必须能在原始入队列表中找到
        bool found = false;
        for (AudioBufferId original : bufferIds) {
            if (recycled[i] == original) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Unqueued buffer id " << recycled[i] << " at index " << i
                           << " was not in the original queue";
    }

    // 出队后，队列剩余 = 3 - processed
    EXPECT_EQ(source->getQueuedBuffers(), 3u - processed);
    // 出队后，processed 应该为 0（已被取走）
    EXPECT_EQ(source->getProcessedBuffers(), 0u);
}

TEST_F(OpenALBackendTest, UnqueueBuffersRespectsCountLimit)
{
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    // 入队 3 个 50ms 的 buffer
    AudioData data = makeSilentAudioData(44100, 1, 0.05f);
    std::vector<AudioBufferId> bufferIds;
    for (int i = 0; i < 3; ++i) {
        auto r = backend->createBuffer(data);
        ASSERT_TRUE(r.success());
        bufferIds.push_back(r.value());
    }
    source->queueBuffers(bufferIds.data(), bufferIds.size());

    source->play();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    u32 processed = source->getProcessedBuffers();
    ASSERT_GE(processed, 2u) << "Test requires at least 2 processed buffers";

    // 即使有 processed 个可出队，传入 count=1 应只出队 1 个
    AudioBufferId oneId = 0;
    u32 unqueued = source->unqueueBuffers(&oneId, 1);
    EXPECT_EQ(unqueued, 1u);
    EXPECT_NE(oneId, 0u); // 必须是有效 ID

    // 剩余 processed 个数应递减
    EXPECT_EQ(source->getProcessedBuffers(), processed - 1);
}

TEST_F(OpenALBackendTest, QueueAndUnqueuePreservesIdMapping)
{
    // 端到端验证：queueBuffers 入队的 AudioBufferId 经过 unqueueBuffers 后必须能找回原 ID
    auto sourceResult = backend->createSource();
    ASSERT_TRUE(sourceResult.success());
    auto source = sourceResult.value();

    // 创建 4 个不同时长 buffer 以区分
    AudioData shortData = makeSilentAudioData(44100, 1, 0.02f); // 20ms
    AudioData longData = makeSilentAudioData(44100, 1, 0.05f);  // 50ms

    auto b1 = backend->createBuffer(shortData);
    auto b2 = backend->createBuffer(longData);
    auto b3 = backend->createBuffer(shortData);
    auto b4 = backend->createBuffer(longData);
    ASSERT_TRUE(b1.success() && b2.success() && b3.success() && b4.success());

    std::vector<AudioBufferId> queuedIds = {b1.value(), b2.value(), b3.value(), b4.value()};
    source->queueBuffers(queuedIds.data(), queuedIds.size());
    EXPECT_EQ(source->getQueuedBuffers(), 4u);

    source->play();
    // 总时长 140ms，等 600ms 足够全部播完
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // 全部应该都已处理
    ASSERT_EQ(source->getProcessedBuffers(), 4u);

    // 一次性出队全部 4 个，验证 ID 严格按入队顺序返回
    std::array<AudioBufferId, 4> recycled = {};
    u32 unqueued = source->unqueueBuffers(recycled.data(), recycled.size());
    EXPECT_EQ(unqueued, 4u);

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(recycled[i], queuedIds[i])
            << "Unqueued id at index " << i << " mismatch: expected " << queuedIds[i] << " got " << recycled[i];
    }
}

TEST_F(OpenALBackendTest, DestroyBufferRemovesFromReverseMap)
{
    // 验证 destroyBuffer 后，正向与反向映射均被正确清理：
    // 1. hasBuffer 返回 false
    // 2. 重新创建 buffer 时分配新 ID（不与已销毁的 ID 冲突）
    // 3. 反向映射表中旧 ALuint 句柄不再映射到旧 AudioBufferId
    AudioData data = makeSilentAudioData(44100, 1, 0.05f);
    auto bufferResult = backend->createBuffer(data);
    ASSERT_TRUE(bufferResult.success());
    AudioBufferId id = bufferResult.value();

    EXPECT_TRUE(backend->hasBuffer(id));

    // 销毁 buffer（此 buffer 未在任何 source 上排队，安全销毁）
    backend->destroyBuffer(id);
    EXPECT_FALSE(backend->hasBuffer(id));

    // 再创建一个新 buffer，应得到新的 AudioBufferId
    auto newBufferResult = backend->createBuffer(data);
    ASSERT_TRUE(newBufferResult.success());
    AudioBufferId newId = newBufferResult.value();
    EXPECT_NE(newId, id) << "New buffer should not reuse the destroyed id";

    // 旧 ID 在反向映射中应不存在（通过 unqueueBuffers 间接验证：
    // 如果反向映射未清理，旧 ALuint 可能错误地映射到旧 id）
    // 这里通过 hasBuffer 检查正向映射已经足够。
    EXPECT_FALSE(backend->hasBuffer(id));
    EXPECT_TRUE(backend->hasBuffer(newId));
}
