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

#include "HttpSkinLoader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/skin/loader/SkinLoader.hpp"
#include "common/util/crypto/Sha1.hpp"
#include "common/util/thread/ITask.hpp"
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

// 注意：实际 HTTP 实现需要依赖 curl 或 asio
// 这里提供框架代码，具体实现可以后续添加

namespace mc::skin {

HttpSkinLoader::HttpSkinLoader() = default;

HttpSkinLoader::~HttpSkinLoader() noexcept
{
    shutdown();
}

Result<void> HttpSkinLoader::initialize()
{
    if (m_initialized) {
        return {};
    }

    // TODO: 初始化 HTTP 客户端（如 curl）
    // curl_global_init(CURL_GLOBAL_DEFAULT);

    m_initialized = true;
    spdlog::info("HttpSkinLoader initialized");
    return {};
}

void HttpSkinLoader::shutdown()
{
    if (!m_initialized) {
        return;
    }

    // 取消所有待处理的下载
    cancelAll();

    // TODO: 清理 HTTP 客户端
    // curl_global_cleanup();

    m_initialized = false;

    // 等待所有在途任务回调完成，避免回调访问已销毁对象
    {
        std::unique_lock<std::mutex> lock(m_shutdownMutex);
        m_shutdownCondition.wait(lock, [this] { return m_pendingCount.load() == 0; });
    }

    spdlog::info("HttpSkinLoader shutdown");
}

bool HttpSkinLoader::supportsUrl(const std::string& url) const
{
    // 支持 Mojang 皮肤服务器
    return url.find("textures.minecraft.net") != std::string::npos || url.find("://") == std::string::npos; // 相对路径
}

Result<SkinLoadResult> HttpSkinLoader::load(const std::string& url)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "HttpSkinLoader not initialized");
    }

    SkinLoadResult result;

    // 提取哈希
    std::string hash = _extractHashFromUrl(url);
    if (hash.empty()) {
        std::vector<u8> urlData(url.begin(), url.end());
        hash = _calculateHash(urlData);
    }

    // 执行 HTTP GET
    auto httpResult = _httpGet(url);
    if (!httpResult.success()) {
        return httpResult.error();
    }

    // 验证和转换
    auto validateResult = _validateAndConvertSkin(httpResult.value());
    if (!validateResult.success()) {
        return validateResult.error();
    }

    result.pngData = validateResult.value();
    result.hash = hash;

    return result;
}

void HttpSkinLoader::loadAsync(const std::string& url, std::function<void(Result<SkinLoadResult>)> callback)
{
    if (!m_initialized) {
        if (callback) {
            callback(Error(ErrorCode::NotInitialized, "HttpSkinLoader not initialized"));
        }
        return;
    }

    // 创建取消信号并登记在途任务
    auto abortSignal = std::make_shared<std::atomic<bool>>(false);
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        // 若同一 url 已有在途任务，覆盖其取消信号（旧任务将被取消）
        m_pendingLoads[url] = abortSignal;
    }
    _incrementPending();

    // 无线程池时降级为同步执行
    if (m_workerPool == nullptr) {
        auto result = load(url);
        if (callback) {
            callback(std::move(result));
        }
        _decrementPending();
        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            m_pendingLoads.erase(url);
        }
        return;
    }

    // 使用 shared_ptr<optional<Result<...>>> 在 executor 和 callback 之间共享结果
    // （FunctionTask::execute 只返回 bool，不能直接传递 Result<SkinLoadResult>；
    //  Result<T> 不可拷贝且默认构造被删除，用 optional 包装以表达"未设置"状态）
    auto sharedResult = std::make_shared<std::optional<Result<SkinLoadResult>>>();
    auto thisLoader = this;

    auto executor = [thisLoader, url, abortSignal, sharedResult](const std::atomic<bool>& signal) -> bool {
        // 优先检查 loader 自己的取消信号（支持 cancel(url)）
        if (abortSignal->load(std::memory_order::acquire)) {
            *sharedResult = Error(ErrorCode::OperationFailed, "Skin download cancelled: " + url);
            return false;
        }
        if (signal.load(std::memory_order::acquire)) {
            *sharedResult = Error(ErrorCode::OperationFailed, "Skin download aborted: " + url);
            return false;
        }
        *sharedResult = thisLoader->load(url);
        return sharedResult->has_value() && sharedResult->value().success();
    };

    auto task = std::make_unique<util::FunctionTask>(
        util::TaskType::Custom, "HttpSkinLoad(" + url + ")", std::move(executor), "worker_pool");

    // 完成回调（在 worker 线程触发）
    auto userCallback = std::move(callback);

    util::TaskCallback poolCallback = [sharedResult, userCallback = std::move(userCallback), url, this](
                                          bool /*success*/, util::ITask*) {
        // 取出结果：executor 已执行则从 sharedResult 取，否则构造取消错误
        Result<SkinLoadResult> result = sharedResult->has_value()
            ? std::move(sharedResult->value())
            : Result<SkinLoadResult>(Error(ErrorCode::OperationFailed, "Skin download cancelled: " + url));

        // 调用用户回调
        if (userCallback) {
            userCallback(std::move(result));
        }

        // 清除在途任务登记
        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            m_pendingLoads.erase(url);
        }

        // 减少计数并通知 shutdown 等待者
        _decrementPending();
    };

    m_workerPool->submit(std::move(task), std::move(poolCallback), util::TaskPriority::Low, abortSignal);
}

void HttpSkinLoader::cancel(const std::string& url)
{
    std::shared_ptr<std::atomic<bool>> signal;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        auto it = m_pendingLoads.find(url);
        if (it != m_pendingLoads.end()) {
            signal = it->second;
        }
        m_pendingLoads.erase(url);
    }
    if (signal) {
        signal->store(true, std::memory_order::release);
    }
}

void HttpSkinLoader::cancelAll()
{
    std::vector<std::shared_ptr<std::atomic<bool>>> signals;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        signals.reserve(m_pendingLoads.size());
        for (auto& [url, signal] : m_pendingLoads) {
            if (signal) {
                signals.push_back(signal);
            }
        }
        m_pendingLoads.clear();
    }
    for (auto& signal : signals) {
        signal->store(true, std::memory_order::release);
    }
}

void HttpSkinLoader::_incrementPending()
{
    m_pendingCount.fetch_add(1, std::memory_order::acq_rel);
}

void HttpSkinLoader::_decrementPending()
{
    m_pendingCount.fetch_sub(1, std::memory_order::acq_rel);
    m_shutdownCondition.notify_one();
}

Result<std::vector<u8>> HttpSkinLoader::_httpGet(const std::string& url)
{
    // TODO: 实现真正的 HTTP GET
    // 这里需要使用 curl 或 asio 实现

    // 临时实现：返回错误
    spdlog::warn("HttpSkinLoader: HTTP GET not implemented, URL: {}", url);
    return Error(ErrorCode::Unsupported, "HTTP GET not implemented");
}

std::string HttpSkinLoader::_extractHashFromUrl(const std::string& url) const
{
    // URL 格式: http://textures.minecraft.net/texture/<hash>
    size_t lastSlash = url.rfind('/');
    if (lastSlash != std::string::npos && lastSlash + 1 < url.length()) {
        std::string hash = url.substr(lastSlash + 1);

        // 移除查询参数
        size_t queryPos = hash.find('?');
        if (queryPos != std::string::npos) {
            hash = hash.substr(0, queryPos);
        }

        return hash;
    }
    return "";
}

Result<std::vector<u8>> HttpSkinLoader::_validateAndConvertSkin(const std::vector<u8>& pngData)
{
    // TODO: 当 HTTP 下载实现后，需要与 FileSkinLoader 保持一致的验证逻辑：
    // 1. 使用 stbi_load_from_memory 解码 PNG
    // 2. 验证尺寸为 64x64 或 64x32
    // 3. 如果是 64x32，转换为 64x64
    // 4. 使用 stbi_write_png_to_func 重新编码为 PNG
    // 当前 _httpGet 尚未实现，此方法暂时不会被调用

    if (pngData.size() < 64) {
        return Error(ErrorCode::InvalidData, "Skin data too small");
    }

    return pngData;
}

std::string HttpSkinLoader::_calculateHash(const std::vector<u8>& data)
{
    // 使用 SHA-1 哈希算法计算缓存键
    auto digest = util::crypto::Sha1::hash(std::span<const u8>(data.data(), data.size()));
    return util::crypto::Sha1::toHexString(digest);
}

} // namespace mc::skin
