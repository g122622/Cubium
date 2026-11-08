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
#include "common/skin/loader/SkinImageValidator.hpp"
#include "common/skin/loader/SkinLoader.hpp"
#include "common/util/crypto/Sha1.hpp"
#include "common/util/thread/ITask.hpp"
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <openssl/ssl.h>
#include <spdlog/spdlog.h>

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

    m_initialized = false;

    // 等待所有在途任务回调完成，避免回调访问已销毁对象
    {
        std::unique_lock<std::mutex> lock(m_shutdownMutex);
        m_shutdownCondition.wait(lock, [this] {
            return m_pendingCount.load() == 0;
        });
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

namespace {

/**
 * @brief Parsed pieces of an HTTP(S) URL that the loader needs.
 */
struct ParsedUrl {
    bool tls = true;
    std::string host;
    std::string port;
    std::string target; // path + query, always starts with '/'
};

/**
 * @brief Split an HTTP(S) URL into scheme/host/port/target.
 *
 * Only http:// and https:// are accepted; anything else is reported as invalid so the caller
 * can fail loudly instead of silently issuing a malformed request.
 */
Result<ParsedUrl> _parseHttpUrl(const std::string& url)
{
    ParsedUrl parsed;
    std::string rest;
    if (url.rfind("https://", 0) == 0) {
        parsed.tls = true;
        rest = url.substr(8);
        parsed.port = "443";
    } else if (url.rfind("http://", 0) == 0) {
        parsed.tls = false;
        rest = url.substr(7);
        parsed.port = "80";
    } else {
        return Error(ErrorCode::InvalidArgument, "Unsupported URL scheme: " + url);
    }

    const size_t pathPos = rest.find('/');
    std::string authority = (pathPos == std::string::npos) ? rest : rest.substr(0, pathPos);
    parsed.target = (pathPos == std::string::npos) ? "/" : rest.substr(pathPos);

    const size_t colonPos = authority.find(':');
    if (colonPos == std::string::npos) {
        parsed.host = authority;
    } else {
        parsed.host = authority.substr(0, colonPos);
        parsed.port = authority.substr(colonPos + 1);
    }

    if (parsed.host.empty()) {
        return Error(ErrorCode::InvalidArgument, "URL has no host: " + url);
    }
    return parsed;
}

/**
 * @brief Case-insensitive ASCII comparison helper.
 */
bool _iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

Result<std::vector<u8>> HttpSkinLoader::_httpGet(const std::string& url)
{
    auto parsedResult = _parseHttpUrl(url);
    if (!parsedResult.success()) {
        return parsedResult.error();
    }
    const ParsedUrl& parsed = parsedResult.value();

    // A per-request io_context keeps the synchronous request self-contained: no shared reactor,
    // no cross-thread state, so concurrent loadAsync workers cannot interfere with each other.
    asio::io_context ioContext;
    asio::ip::tcp::resolver resolver(ioContext);
    asio::error_code error;

    auto endpoints = resolver.resolve(parsed.host, parsed.port, error);
    if (error) {
        spdlog::warn("HttpSkinLoader: DNS resolution failed for {}: {}", parsed.host, error.message());
        return Error(ErrorCode::ConnectionFailed, "DNS resolution failed: " + error.message());
    }

    asio::ip::tcp::socket socket(ioContext);
    asio::connect(socket, endpoints, error);
    if (error) {
        spdlog::warn("HttpSkinLoader: TCP connect failed for {}: {}", parsed.host, error.message());
        return Error(ErrorCode::ConnectionFailed, "TCP connect failed: " + error.message());
    }

    // TLS context must outlive the stream it serves; declaring it here keeps the lifetime obvious
    // instead of hiding it behind a branch-local object.
    asio::ssl::context sslContext(asio::ssl::context::tls_client);
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> tlsStream;

    if (parsed.tls) {
        sslContext.set_default_verify_paths();
        sslContext.set_verify_mode(asio::ssl::verify_peer);

        tlsStream = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(std::move(socket), sslContext);
        // SNI + hostname verification: textures.minecraft.net is served from a shared CDN, so
        // skipping verification would accept any certificate the CDN happens to present.
        if (!SSL_set_tlsext_host_name(tlsStream->native_handle(), parsed.host.c_str())) {
            return Error(ErrorCode::ConnectionFailed, "Failed to set TLS SNI hostname");
        }
        tlsStream->set_verify_callback(asio::ssl::host_name_verification(parsed.host));
        tlsStream->handshake(asio::ssl::stream_base::client, error);
        if (error) {
            spdlog::warn("HttpSkinLoader: TLS handshake failed for {}: {}", parsed.host, error.message());
            return Error(ErrorCode::ConnectionFailed, "TLS handshake failed: " + error.message());
        }
    }

    std::ostringstream request;
    request << "GET " << parsed.target << " HTTP/1.1\r\n";
    request << "Host: " << parsed.host << "\r\n";
    request << "User-Agent: Cubium\r\n";
    request << "Accept: image/png\r\n";
    request << "Connection: close\r\n\r\n";
    const std::string requestText = request.str();

    // Both transports are driven through the same lambdas so the HTTP layer below is written once.
    const auto writeAll = [&](const std::string& text) {
        if (parsed.tls) {
            asio::write(*tlsStream, asio::buffer(text), error);
        } else {
            asio::write(socket, asio::buffer(text), error);
        }
    };
    const auto readSome = [&](std::array<char, 8192>& buffer) -> size_t {
        return parsed.tls ? tlsStream->read_some(asio::buffer(buffer), error)
                          : socket.read_some(asio::buffer(buffer), error);
    };

    writeAll(requestText);
    if (error) {
        spdlog::warn("HttpSkinLoader: failed to send request for {}: {}", url, error.message());
        return Error(ErrorCode::ConnectionFailed, "Failed to send HTTP request: " + error.message());
    }

    std::string rawResponse;
    std::array<char, 8192> buffer{};
    while (true) {
        const size_t bytesRead = readSome(buffer);
        if (error == asio::error::eof) {
            break;
        }
        if (error) {
            spdlog::warn("HttpSkinLoader: failed to read response for {}: {}", url, error.message());
            return Error(ErrorCode::ConnectionFailed, "Failed to read HTTP response: " + error.message());
        }
        rawResponse.append(buffer.data(), bytesRead);
    }

    const size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return Error(ErrorCode::InvalidData, "Malformed HTTP response (no header terminator)");
    }

    // Parse the status line and the headers we care about (case-insensitive names).
    const std::string headerBlock = rawResponse.substr(0, headerEnd);
    std::istringstream headerStream(headerBlock);
    std::string statusLine;
    std::getline(headerStream, statusLine);

    i32 statusCode = 0;
    {
        std::istringstream statusStream(statusLine);
        std::string version;
        statusStream >> version >> statusCode;
    }
    if (statusCode != 200) {
        spdlog::warn("HttpSkinLoader: HTTP {} for {}", statusCode, url);
        return Error(ErrorCode::ConnectionFailed, "HTTP status " + std::to_string(statusCode));
    }

    bool chunked = false;
    size_t contentLength = 0;
    bool hasContentLength = false;
    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const size_t sep = line.find(':');
        if (sep == std::string::npos) {
            continue;
        }
        const std::string name = line.substr(0, sep);
        std::string value = line.substr(sep + 1);
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(value.begin());
        }
        if (_iequals(name, "Transfer-Encoding") && value.find("chunked") != std::string::npos) {
            chunked = true;
        } else if (_iequals(name, "Content-Length")) {
            contentLength = static_cast<size_t>(std::strtoull(value.c_str(), nullptr, 10));
            hasContentLength = true;
        }
    }

    const std::string body = rawResponse.substr(headerEnd + 4);

    if (!chunked) {
        if (hasContentLength) {
            if (body.size() < contentLength) {
                return Error(ErrorCode::InvalidData, "Truncated HTTP body");
            }
            return std::vector<u8>(body.begin(), body.begin() + static_cast<std::ptrdiff_t>(contentLength));
        }
        return std::vector<u8>(body.begin(), body.end());
    }

    // Chunked transfer encoding: accumulate chunk payloads, stop at the zero-length terminator.
    std::vector<u8> decoded;
    size_t cursor = 0;
    while (cursor < body.size()) {
        const size_t lineEnd = body.find("\r\n", cursor);
        if (lineEnd == std::string::npos) {
            return Error(ErrorCode::InvalidData, "Malformed chunked HTTP body");
        }
        const size_t chunkSize =
            static_cast<size_t>(std::strtoull(body.substr(cursor, lineEnd - cursor).c_str(), nullptr, 16));
        cursor = lineEnd + 2;
        if (chunkSize == 0) {
            break;
        }
        if (cursor + chunkSize > body.size()) {
            return Error(ErrorCode::InvalidData, "Truncated chunked HTTP body");
        }
        decoded.insert(decoded.end(),
            body.begin() + static_cast<std::ptrdiff_t>(cursor),
            body.begin() + static_cast<std::ptrdiff_t>(cursor + chunkSize));
        cursor += chunkSize + 2; // skip payload and trailing CRLF
    }

    if (decoded.empty()) {
        return Error(ErrorCode::InvalidData, "Empty HTTP body");
    }
    return decoded;
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
    // 与 FileSkinLoader 共用同一份校验/规范化实现：两个来源必须产出完全一致的字节，
    // 否则同一张皮肤由不同来源加载会得到不同缓存哈希。
    return SkinImageValidator::validateAndNormalize(pngData);
}

std::string HttpSkinLoader::_calculateHash(const std::vector<u8>& data)
{
    // 使用 SHA-1 哈希算法计算缓存键
    auto digest = util::crypto::Sha1::hash(std::span<const u8>(data.data(), data.size()));
    return util::crypto::Sha1::toHexString(digest);
}

} // namespace mc::skin
