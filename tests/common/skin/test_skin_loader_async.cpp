/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/skin/loader/FileSkinLoader.hpp"
#include "common/skin/loader/HttpSkinLoader.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <gtest/gtest.h>

// stb_image_write for generating test PNG data
// STB_IMAGE_WRITE_IMPLEMENTATION 已在 TextureAtlasBuilder.cpp 中定义
#include <stb_image_write.h>

using namespace mc;
using namespace mc::skin;
using namespace mc::util;

namespace {

/// 生成 64x64 纯色 RGBA PNG 字节流
std::vector<mc::u8> makeSolidColorPng(mc::u8 r, mc::u8 g, mc::u8 b, mc::u8 a)
{
    std::vector<mc::u8> pixels(64 * 64 * 4);
    for (size_t i = 0; i < 64 * 64; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }

    std::vector<mc::u8> png;
    struct CaptureContext {
        std::vector<mc::u8>* out;
    } ctx{&png};

    stbi_write_png_to_func(
        [](void* userdata, void* data, int size) {
            auto* context = static_cast<CaptureContext*>(userdata);
            auto* bytes = static_cast<mc::u8*>(data);
            context->out->insert(context->out->end(), bytes, bytes + size);
        },
        &ctx,
        64, // width
        64, // height
        4,  // components (RGBA)
        pixels.data(),
        64 * 4 // stride
    );

    return png;
}

/// 写入 PNG 数据到临时文件
std::string writeTempPng(const std::string& dir, const std::string& filename, const std::vector<mc::u8>& pngData)
{
    std::string path = dir + "/" + filename;
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(pngData.data()), static_cast<std::streamsize>(pngData.size()));
    file.close();
    return path;
}

} // namespace

// ============================================================================
// FileSkinLoader 异步加载测试
// ============================================================================

class FileSkinLoaderAsyncTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testDir_ = "./test_file_skin_loader_async_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(testDir_);

        // 生成测试用 PNG
        validPng_ = makeSolidColorPng(255, 0, 0, 255);
        skinPath_ = writeTempPng(testDir_, "skin.png", validPng_);

        loader_ = std::make_unique<FileSkinLoader>();
        loader_->initialize();
    }

    void TearDown() override
    {
        loader_->shutdown();
        loader_.reset();

        std::error_code ec;
        std::filesystem::remove_all(testDir_, ec);
    }

    std::string testDir_;
    std::vector<mc::u8> validPng_;
    std::string skinPath_;
    std::unique_ptr<FileSkinLoader> loader_;
};

// ============================================================================
// 同步加载测试（基础验证）
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, LoadSyncValidFile)
{
    auto result = loader_->load(skinPath_);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().pngData.empty());
    EXPECT_FALSE(result.value().hash.empty());
}

TEST_F(FileSkinLoaderAsyncTest, LoadSyncNonExistentFile)
{
    auto result = loader_->load(testDir_ + "/nonexistent.png");
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::FileNotFound);
}

// ============================================================================
// 无线程池降级测试（loadAsync 应同步执行后立即回调）
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncWithoutPoolSynchronous)
{
    std::atomic<bool> callbackCalled{false};
    std::atomic<bool> callbackSuccess{false};

    loader_->loadAsync(skinPath_, [&](Result<SkinLoadResult> result) {
        callbackCalled = true;
        callbackSuccess = result.success();
    });

    // 无线程池时，回调应在 loadAsync 返回前同步触发
    EXPECT_TRUE(callbackCalled.load());
    EXPECT_TRUE(callbackSuccess.load());
}

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncWithoutPoolErrorPropagated)
{
    std::atomic<bool> callbackCalled{false};
    std::atomic<bool> callbackFailed{false};

    loader_->loadAsync(testDir_ + "/nonexistent.png", [&](Result<SkinLoadResult> result) {
        callbackCalled = true;
        callbackFailed = result.failed();
    });

    EXPECT_TRUE(callbackCalled.load());
    EXPECT_TRUE(callbackFailed.load());
}

// ============================================================================
// 有线程池异步加载测试
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncWithPoolCompletes)
{
    ServerWorkerPool pool(2, "SkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<bool> completed{false};
    std::atomic<bool> success{false};
    std::mutex resultMutex;
    std::condition_variable resultCv;

    loader_->loadAsync(skinPath_, [&](Result<SkinLoadResult> result) {
        std::lock_guard<std::mutex> lock(resultMutex);
        completed = true;
        success = result.success();
        resultCv.notify_one();
    });

    // 等待回调完成（超时 5 秒）
    {
        std::unique_lock<std::mutex> lock(resultMutex);
        resultCv.wait_for(lock, std::chrono::seconds(5), [&] { return completed.load(); });
    }

    EXPECT_TRUE(completed.load());
    EXPECT_TRUE(success.load());

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncWithPoolReturnsValidData)
{
    ServerWorkerPool pool(2, "SkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<bool> completed{false};
    std::vector<mc::u8> returnedPng;
    std::string returnedHash;
    std::mutex resultMutex;
    std::condition_variable resultCv;

    loader_->loadAsync(skinPath_, [&](Result<SkinLoadResult> result) {
        std::lock_guard<std::mutex> lock(resultMutex);
        if (result.success()) {
            returnedPng = result.value().pngData;
            returnedHash = result.value().hash;
        }
        completed = true;
        resultCv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(resultMutex);
        resultCv.wait_for(lock, std::chrono::seconds(5), [&] { return completed.load(); });
    }

    ASSERT_TRUE(completed.load());
    EXPECT_FALSE(returnedPng.empty());
    EXPECT_FALSE(returnedHash.empty());

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncWithPoolErrorPropagated)
{
    ServerWorkerPool pool(2, "SkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<bool> completed{false};
    std::atomic<bool> failed{false};
    mc::ErrorCode errorCode = mc::ErrorCode::Success;
    std::mutex resultMutex;
    std::condition_variable resultCv;

    loader_->loadAsync(testDir_ + "/nonexistent.png", [&](Result<SkinLoadResult> result) {
        std::lock_guard<std::mutex> lock(resultMutex);
        if (result.failed()) {
            failed = true;
            errorCode = result.error().code();
        }
        completed = true;
        resultCv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(resultMutex);
        resultCv.wait_for(lock, std::chrono::seconds(5), [&] { return completed.load(); });
    }

    EXPECT_TRUE(completed.load());
    EXPECT_TRUE(failed.load());
    EXPECT_EQ(errorCode, mc::ErrorCode::FileNotFound);

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

// ============================================================================
// 多个异步任务并发测试
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncMultipleConcurrent)
{
    ServerWorkerPool pool(4, "SkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    // 生成多个皮肤文件
    std::vector<std::string> paths;
    for (int i = 0; i < 5; ++i) {
        auto png = makeSolidColorPng(static_cast<mc::u8>(i * 50), 0, 0, 255);
        paths.push_back(writeTempPng(testDir_, "skin_" + std::to_string(i) + ".png", png));
    }

    std::atomic<int> completedCount{0};
    std::atomic<int> successCount{0};
    std::mutex resultMutex;
    std::condition_variable resultCv;

    for (const auto& path : paths) {
        loader_->loadAsync(path, [&](Result<SkinLoadResult> result) {
            std::lock_guard<std::mutex> lock(resultMutex);
            if (result.success()) {
                successCount++;
            }
            completedCount++;
            resultCv.notify_one();
        });
    }

    // 等待所有完成
    {
        std::unique_lock<std::mutex> lock(resultMutex);
        resultCv.wait_for(lock, std::chrono::seconds(10), [&] { return completedCount.load() == 5; });
    }

    EXPECT_EQ(completedCount.load(), 5);
    EXPECT_EQ(successCount.load(), 5);

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

// ============================================================================
// 取消测试
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, CancelAllSetsAbortSignals)
{
    ServerWorkerPool pool(2, "SkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<int> completedCount{0};
    std::mutex resultMutex;

    // 提交多个任务
    for (int i = 0; i < 3; ++i) {
        loader_->loadAsync(skinPath_, [&](Result<SkinLoadResult> /*result*/) {
            std::lock_guard<std::mutex> lock(resultMutex);
            completedCount++;
        });
    }

    // 立即取消所有
    loader_->cancelAll();

    // 等待所有回调完成（取消的任务也会触发回调）
    for (int i = 0; i < 500 && completedCount.load() < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedCount.load(), 3);

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

// ============================================================================
// shutdown 等待在途任务测试
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, ShutdownWaitsForPendingTasks)
{
    ServerWorkerPool pool(2, "SkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<int> completedCount{0};

    // 提交多个任务
    for (int i = 0; i < 5; ++i) {
        loader_->loadAsync(skinPath_, [&](Result<SkinLoadResult> /*result*/) { completedCount++; });
    }

    // shutdown 应该等待所有在途回调完成
    loader_->shutdown();

    // shutdown 后所有回调应已完成
    EXPECT_EQ(completedCount.load(), 5);

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

// ============================================================================
// 初始化状态测试
// ============================================================================

TEST_F(FileSkinLoaderAsyncTest, LoadAsyncBeforeInitializeReturnsError)
{
    FileSkinLoader uninitLoader;
    std::atomic<bool> callbackCalled{false};
    std::atomic<bool> callbackFailed{false};

    uninitLoader.loadAsync(skinPath_, [&](Result<SkinLoadResult> result) {
        callbackCalled = true;
        callbackFailed = result.failed();
    });

    EXPECT_TRUE(callbackCalled.load());
    EXPECT_TRUE(callbackFailed.load());
}

// ============================================================================
// HttpSkinLoader 异步加载测试（框架验证，HTTP 未实现）
// ============================================================================

class HttpSkinLoaderAsyncTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        loader_ = std::make_unique<HttpSkinLoader>();
        loader_->initialize();
    }

    void TearDown() override
    {
        loader_->shutdown();
        loader_.reset();
    }

    std::unique_ptr<HttpSkinLoader> loader_;
};

TEST_F(HttpSkinLoaderAsyncTest, LoadAsyncWithoutPoolReturnsUnsupported)
{
    std::atomic<bool> callbackCalled{false};
    std::atomic<bool> callbackFailed{false};
    mc::ErrorCode errorCode = mc::ErrorCode::Success;

    loader_->loadAsync("http://textures.minecraft.net/texture/abc123", [&](Result<SkinLoadResult> result) {
        callbackCalled = true;
        callbackFailed = result.failed();
        if (result.failed()) {
            errorCode = result.error().code();
        }
    });

    // _httpGet 未实现，返回 Unsupported
    EXPECT_TRUE(callbackCalled.load());
    EXPECT_TRUE(callbackFailed.load());
    EXPECT_EQ(errorCode, mc::ErrorCode::Unsupported);
}

TEST_F(HttpSkinLoaderAsyncTest, LoadAsyncWithPoolReturnsUnsupported)
{
    ServerWorkerPool pool(2, "HttpSkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<bool> completed{false};
    std::atomic<bool> failed{false};
    mc::ErrorCode errorCode = mc::ErrorCode::Success;
    std::mutex resultMutex;
    std::condition_variable resultCv;

    loader_->loadAsync("http://textures.minecraft.net/texture/abc123", [&](Result<SkinLoadResult> result) {
        std::lock_guard<std::mutex> lock(resultMutex);
        failed = result.failed();
        if (result.failed()) {
            errorCode = result.error().code();
        }
        completed = true;
        resultCv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(resultMutex);
        resultCv.wait_for(lock, std::chrono::seconds(5), [&] { return completed.load(); });
    }

    EXPECT_TRUE(completed.load());
    EXPECT_TRUE(failed.load());
    EXPECT_EQ(errorCode, mc::ErrorCode::Unsupported);

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}

TEST_F(HttpSkinLoaderAsyncTest, LoadAsyncBeforeInitializeReturnsError)
{
    HttpSkinLoader uninitLoader;
    std::atomic<bool> callbackCalled{false};
    std::atomic<bool> callbackFailed{false};

    uninitLoader.loadAsync("http://textures.minecraft.net/texture/abc123", [&](Result<SkinLoadResult> result) {
        callbackCalled = true;
        callbackFailed = result.failed();
    });

    EXPECT_TRUE(callbackCalled.load());
    EXPECT_TRUE(callbackFailed.load());
}

TEST_F(HttpSkinLoaderAsyncTest, ShutdownWaitsForPendingTasks)
{
    ServerWorkerPool pool(2, "HttpSkinTestWorker");
    pool.start();
    loader_->setWorkerPool(&pool);

    std::atomic<int> completedCount{0};

    // 提交多个任务
    for (int i = 0; i < 3; ++i) {
        loader_->loadAsync("http://textures.minecraft.net/texture/abc" + std::to_string(i),
            [&](Result<SkinLoadResult> /*result*/) { completedCount++; });
    }

    // shutdown 应该等待所有在途回调完成
    loader_->shutdown();

    EXPECT_EQ(completedCount.load(), 3);

    loader_->setWorkerPool(nullptr);
    pool.shutdown();
}
