#pragma once

#include "SkinLoader.hpp"
#include <unordered_map>
#include <mutex>
#include <future>

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

    [[nodiscard]] bool supportsUrl(const String& url) const override;
    Result<SkinLoadResult> load(const String& url) override;
    void loadAsync(const String& url, std::function<void(Result<SkinLoadResult>)> callback) override;
    void cancel(const String& url) override;
    void cancelAll() override;

    [[nodiscard]] String name() const override { return "HttpSkinLoader"; }

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
    Result<std::vector<u8>> httpGet(const String& url);

    /**
     * @brief 从 URL 提取哈希
     */
    [[nodiscard]] String extractHashFromUrl(const String& url) const;

    /**
     * @brief 验证皮肤 PNG 数据
     */
    Result<std::vector<u8>> validateAndConvertSkin(const std::vector<u8>& pngData);

    /**
     * @brief 计算数据哈希
     */
    String calculateHash(const std::vector<u8>& data);

    std::mutex m_pendingMutex;
    std::unordered_map<String, std::future<Result<SkinLoadResult>>> m_pendingLoads;

    u32 m_connectTimeout = 5000;   // 5 秒
    u32 m_readTimeout = 30000;     // 30 秒
    bool m_initialized = false;
};

} // namespace mc::skin
