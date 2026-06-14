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

#include "HttpSkinLoader.hpp"
#include "common/util/crypto/Sha1.hpp"
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
        callback(Error(ErrorCode::NotInitialized, "HttpSkinLoader not initialized"));
        return;
    }

    // TODO: 使用线程池替代 detached thread，限制并发下载数
    // 当前简化实现：同步调用并立即回调
    // 实际 HTTP 实现时应使用有界线程池（如 4-8 工作线程）
    auto result = load(url);
    callback(std::move(result));
}

void HttpSkinLoader::cancel(const std::string& url)
{
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingLoads.erase(url);
}

void HttpSkinLoader::cancelAll()
{
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingLoads.clear();
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
    // 与 FileSkinLoader 相同的验证逻辑
    // 简化实现：假设数据有效

    if (pngData.size() < 64 * 32 * 4) {
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
