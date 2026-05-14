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

#pragma once

#include "SkinLoader.hpp"
#include <future>
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
 * 注意：这个加载器使用异步下载，避免阻塞主线程。
 */
class HttpSkinLoader : public ISkinLoader {
public:
    HttpSkinLoader();
    ~HttpSkinLoader() override;

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

private:
    /**
     * @brief 执行 HTTP GET 请求
     */
    Result<std::vector<u8>> httpGet(const std::string& url);

    /**
     * @brief 从 URL 提取哈希
     */
    [[nodiscard]] std::string extractHashFromUrl(const std::string& url) const;

    /**
     * @brief 验证皮肤 PNG 数据
     */
    Result<std::vector<u8>> validateAndConvertSkin(const std::vector<u8>& pngData);

    /**
     * @brief 计算数据哈希
     */
    std::string calculateHash(const std::vector<u8>& data);

    std::mutex m_pendingMutex;
    std::unordered_map<std::string, std::future<Result<SkinLoadResult>>> m_pendingLoads;

    u32 m_connectTimeout = 5000; // 5 秒
    u32 m_readTimeout = 30000;   // 30 秒
    bool m_initialized = false;
};

} // namespace mc::skin
