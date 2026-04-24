#include "HttpSkinLoader.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

// 注意：实际 HTTP 实现需要依赖 curl 或 asio
// 这里提供框架代码，具体实现可以后续添加

namespace mc::skin {

HttpSkinLoader::HttpSkinLoader() = default;

HttpSkinLoader::~HttpSkinLoader() {
    shutdown();
}

Result<void> HttpSkinLoader::initialize() {
    if (m_initialized) {
        return {};
    }

    // TODO: 初始化 HTTP 客户端（如 curl）
    // curl_global_init(CURL_GLOBAL_DEFAULT);

    m_initialized = true;
    spdlog::info("HttpSkinLoader initialized");
    return {};
}

void HttpSkinLoader::shutdown() {
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

bool HttpSkinLoader::supportsUrl(const String& url) const {
    // 支持 Mojang 皮肤服务器
    return url.find("textures.minecraft.net") != String::npos ||
           url.find("://") == String::npos;  // 相对路径
}

Result<SkinLoadResult> HttpSkinLoader::load(const String& url) {
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "HttpSkinLoader not initialized");
    }

    SkinLoadResult result;

    // 提取哈希
    String hash = extractHashFromUrl(url);
    if (hash.empty()) {
        std::vector<u8> urlData(url.begin(), url.end());
        hash = calculateHash(urlData);
    }

    // 执行 HTTP GET
    auto httpResult = httpGet(url);
    if (!httpResult.success()) {
        return httpResult.error();
    }

    // 验证和转换
    auto validateResult = validateAndConvertSkin(httpResult.value());
    if (!validateResult.success()) {
        return validateResult.error();
    }

    result.pngData = validateResult.value();
    result.hash = hash;

    spdlog::debug("HttpSkinLoader: Downloaded skin from {} ({} bytes)",
                  url, result.pngData.size());
    return result;
}

void HttpSkinLoader::loadAsync(const String& url,
                               std::function<void(Result<SkinLoadResult>)> callback) {
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

void HttpSkinLoader::cancel(const String& url) {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingLoads.erase(url);
}

void HttpSkinLoader::cancelAll() {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingLoads.clear();
}

Result<std::vector<u8>> HttpSkinLoader::httpGet(const String& url) {
    // TODO: 实现真正的 HTTP GET
    // 这里需要使用 curl 或 asio 实现

    // 临时实现：返回错误
    spdlog::warn("HttpSkinLoader: HTTP GET not implemented, URL: {}", url);
    return Error(ErrorCode::Unsupported, "HTTP GET not implemented");
}

String HttpSkinLoader::extractHashFromUrl(const String& url) const {
    // URL 格式: http://textures.minecraft.net/texture/<hash>
    size_t lastSlash = url.rfind('/');
    if (lastSlash != String::npos && lastSlash + 1 < url.length()) {
        String hash = url.substr(lastSlash + 1);

        // 移除查询参数
        size_t queryPos = hash.find('?');
        if (queryPos != String::npos) {
            hash = hash.substr(0, queryPos);
        }

        return hash;
    }
    return "";
}

Result<std::vector<u8>> HttpSkinLoader::validateAndConvertSkin(const std::vector<u8>& pngData) {
    // 与 FileSkinLoader 相同的验证逻辑
    // 简化实现：假设数据有效

    if (pngData.size() < 64 * 32 * 4) {
        return Error(ErrorCode::InvalidData, "Skin data too small");
    }

    return pngData;
}

String HttpSkinLoader::calculateHash(const std::vector<u8>& data) {
    // 简化的哈希计算
    u64 hash = 0xcbf29ce484222325ULL;
    constexpr u64 prime = 0x100000001b3ULL;

    for (u8 byte : data) {
        hash ^= byte;
        hash *= prime;
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

} // namespace mc::skin
