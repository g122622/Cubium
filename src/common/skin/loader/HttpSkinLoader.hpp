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

#pragma once

#include "SkinLoader.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace mc::skin {

/**
 * @brief HTTP 皮肤加载器
 *
 * 从 Mojang 皮肤服务器下载皮肤。
 *
 * 支持的 URL 格式：
 * - http://textures.minecraft.net/texture/<hash>
 * - https://textures.minecraft.net/texture/<hash>
 *
 * 异步加载通过注入的 UniversalWorkerPool 实现，回调在 worker 线程触发。
 * 若未注入线程池，loadAsync 降级为同步执行后立即回调。
 *
 * 注意：_httpGet 当前未实现（返回 Unsupported），HTTP 下载功能待后续补充。
 */
class HttpSkinLoader : public ISkinLoader {
public:
    HttpSkinLoader();
    ~HttpSkinLoader() noexcept override;

    Result<void> initialize() override;
    void shutdown() override;

    [[nodiscard]] bool supportsUrl(const std::string& url) const override;
    Result<SkinLoadResult> load(const std::string& url) override;
    void loadAsync(const std::string& url, std::function<void(Result<SkinLoadResult>)> callback) override;
    void cancel(const std::string& url) override;
    void cancelAll() override;

    [[nodiscard]] std::string name() const override { return "HttpSkinLoader"; }

    /**
     * @brief 设置连接超时
     * @param timeoutMs 超时时间（毫秒）
     */
    void setConnectTimeout(u32 timeoutMs) { m_connectTimeout = timeoutMs; }

    /**
     * @brief 设置读取超时
     * @param timeoutMs 超时时间（毫秒）
     */
    void setReadTimeout(u32 timeoutMs) { m_readTimeout = timeoutMs; }

    /**
     * @brief 注入工作线程池用于异步加载
     *
     * 线程池由调用方拥有，必须保证生命周期长于本加载器（或在 shutdown 后释放）。
     * 传入 nullptr 切换回同步降级模式。
     *
     * @param workerPool 工作线程池指针（非所有权）
     */
    void setWorkerPool(util::UniversalWorkerPool* workerPool) { m_workerPool = workerPool; }

private:
    /**
     * @brief 执行 HTTP GET 请求
     */
    Result<std::vector<u8>> _httpGet(const std::string& url);

    /**
     * @brief 从 URL 提取哈希
     */
    [[nodiscard]] std::string _extractHashFromUrl(const std::string& url) const;

    /**
     * @brief 验证皮肤 PNG 数据
     */
    Result<std::vector<u8>> _validateAndConvertSkin(const std::vector<u8>& pngData);

    /**
     * @brief 计算数据哈希
     */
    std::string _calculateHash(const std::vector<u8>& data);

    /**
     * @brief 在途任务计数增加（loadAsync 提交时调用）
     */
    void _incrementPending();

    /**
     * @brief 在途任务计数减少（回调完成时调用），通知 shutdown 等待者
     */
    void _decrementPending();

    u32 m_connectTimeout = 5000; // 5 秒
    u32 m_readTimeout = 30000;   // 30 秒
    bool m_initialized = false;

    // 异步加载基础设施
    util::UniversalWorkerPool* m_workerPool = nullptr;

    // 在途任务管理：url → 取消信号
    std::mutex m_pendingMutex;
    std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> m_pendingLoads;

    // shutdown 同步：等待所有在途回调完成
    std::atomic<size_t> m_pendingCount{0};
    std::mutex m_shutdownMutex;
    std::condition_variable m_shutdownCondition;
};

} // namespace mc::skin
