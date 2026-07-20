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

#include "FileSkinLoader.hpp"
#include "common/util/crypto/Sha1.hpp"
#include "common/util/thread/ITask.hpp"
#include <algorithm>
#include <fstream>
#include <spdlog/spdlog.h>

// stb_image for PNG loading
#include <stb_image.h>

// stb_image_write for PNG encoding
#include <stb_image_write.h>

namespace mc::skin {

FileSkinLoader::FileSkinLoader() = default;

FileSkinLoader::~FileSkinLoader()
{
    shutdown();
}

Result<void> FileSkinLoader::initialize()
{
    if (m_initialized) {
        return {};
    }
    m_initialized = true;
    return {};
}

void FileSkinLoader::shutdown()
{
    if (!m_initialized) {
        return;
    }

    m_initialized = false;

    // 取消所有在途任务（设置取消信号）
    cancelAll();

    // 等待所有在途任务回调完成，避免回调访问已销毁对象
    {
        std::unique_lock<std::mutex> lock(m_shutdownMutex);
        m_shutdownCondition.wait(lock, [this] { return m_pendingCount.load() == 0; });
    }
}

bool FileSkinLoader::supportsUrl(const std::string& url) const
{
    // 支持文件路径和资源位置
    if (url.find("://") != std::string::npos) {
        // 有协议，只支持 file://
        return url.find("file://") == 0;
    }
    return true;
}

Result<SkinLoadResult> FileSkinLoader::load(const std::string& url)
{
    SkinLoadResult result;

    // 尝试解析为资源位置
    if (url.find(':') != std::string::npos && url.find("file://") != 0) {
        // 格式: namespace:path
        ResourceLocation location(url);
        auto loadResult = _loadFromResourcePack(location);
        if (loadResult.success()) {
            return loadResult;
        }
    }

    // 尝试作为文件路径加载
    return _loadFromFilesystem(url);
}

void FileSkinLoader::loadAsync(const std::string& url, std::function<void(Result<SkinLoadResult>)> callback)
{
    if (!m_initialized) {
        if (callback) {
            callback(Error(ErrorCode::NotInitialized, "FileSkinLoader not initialized"));
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
        // 取消时跳过回调（shutdown 路径已经在等待 pending 归零，仍需回调以计数归零）
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
            *sharedResult = Error(ErrorCode::OperationFailed, "Skin load cancelled: " + url);
            return false;
        }
        if (signal.load(std::memory_order::acquire)) {
            *sharedResult = Error(ErrorCode::OperationFailed, "Skin load aborted: " + url);
            return false;
        }
        *sharedResult = thisLoader->load(url);
        return sharedResult->has_value() && sharedResult->value().success();
    };

    auto task = std::make_unique<util::FunctionTask>(
        util::TaskType::Custom, "FileSkinLoad(" + url + ")", std::move(executor), "worker_pool");

    // 完成回调（在 worker 线程触发）
    auto userCallback = std::move(callback);

    util::TaskCallback poolCallback = [sharedResult, userCallback = std::move(userCallback), url, this](
                                          bool /*success*/, util::ITask*) {
        // 取出结果：executor 已执行则从 sharedResult 取，否则构造取消错误
        Result<SkinLoadResult> result = sharedResult->has_value()
            ? std::move(sharedResult->value())
            : Result<SkinLoadResult>(Error(ErrorCode::OperationFailed, "Skin load cancelled: " + url));

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

void FileSkinLoader::cancel(const std::string& url)
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

void FileSkinLoader::cancelAll()
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

void FileSkinLoader::_incrementPending()
{
    m_pendingCount.fetch_add(1, std::memory_order::acq_rel);
}

void FileSkinLoader::_decrementPending()
{
    m_pendingCount.fetch_sub(1, std::memory_order::acq_rel);
    m_shutdownCondition.notify_one();
}

Result<SkinLoadResult> FileSkinLoader::_loadFromFilesystem(const std::string& path)
{
    SkinLoadResult result;

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        return Error(ErrorCode::FileNotFound, "Skin file not found: " + path);
    }

    // 读取文件
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open skin file: " + path);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        return Error(ErrorCode::FileReadFailed, "Failed to read skin file: " + path);
    }

    // 验证和转换
    auto validateResult = _validateAndConvertSkin(data);
    if (!validateResult.success()) {
        return validateResult.error();
    }

    result.pngData = validateResult.value();
    result.hash = _calculateHash(result.pngData);

    return result;
}

Result<SkinLoadResult> FileSkinLoader::_loadFromResourcePack(const ResourceLocation& location)
{
    if (m_resourcePacks.empty()) {
        return Error(ErrorCode::NotInitialized, "No resource pack available");
    }

    SkinLoadResult result;

    // 从资源包读取（按优先级反向遍历，后添加的优先）
    // toFilePath(PackType) 返回 "assets/namespace/path" 格式，
    // readResource 需要相对于 PackType 根目录的路径（不含 "assets/" 前缀）
    std::string resourcePath = location.toFilePath(resource::PackType::ClientResources);
    resourcePath.erase(0, std::string("assets/").size());

    std::vector<u8> pngData;
    for (auto packIt = m_resourcePacks.rbegin(); packIt != m_resourcePacks.rend(); ++packIt) {
        IResourcePack* pack = *packIt;
        if (pack == nullptr) {
            continue;
        }
        if (!pack->hasResource(resource::PackType::ClientResources, resourcePath)) {
            continue;
        }
        auto readResult = pack->readResource(resource::PackType::ClientResources, resourcePath);
        if (!readResult.success() || readResult.value().empty()) {
            continue;
        }
        pngData = std::move(readResult.value());
        break;
    }

    if (pngData.empty()) {
        return Error(ErrorCode::NotFound, "Skin not found in any resource pack: " + location.toString());
    }

    // 验证和转换
    auto validateResult = _validateAndConvertSkin(pngData);
    if (!validateResult.success()) {
        return validateResult.error();
    }

    result.pngData = validateResult.value();
    result.hash = _calculateHash(result.pngData);

    return result;
}

Result<std::vector<u8>> FileSkinLoader::_validateAndConvertSkin(const std::vector<u8>& pngData)
{
    // 使用 stb_image 解析 PNG
    int width = 0;
    int height = 0;
    int channels = 0;

    u8* pixels = stbi_load_from_memory(pngData.data(),
        static_cast<int>(pngData.size()),
        &width,
        &height,
        &channels,
        4 // 强制 RGBA
    );

    if (!pixels) {
        return Error(ErrorCode::InvalidData, "Failed to parse skin PNG");
    }

    // 验证尺寸
    if (width != 64) {
        stbi_image_free(pixels);
        return Error(ErrorCode::InvalidData, "Invalid skin width: expected 64, got " + std::to_string(width));
    }

    if (height != 64 && height != 32) {
        stbi_image_free(pixels);
        return Error(ErrorCode::InvalidData, "Invalid skin height: expected 32 or 64, got " + std::to_string(height));
    }

    std::vector<u8> result;

    if (height == 32) {
        // 旧版皮肤，需要转换为 64x64
        result.resize(64 * 64 * 4, 0);

        // 复制上半部分（0-31 行）
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>(y * 64 * 4);
            std::memcpy(result.data() + dstOffset, pixels + srcOffset, 64 * 4);
        }

        spdlog::info("FileSkinLoader: Converted legacy 64x32 skin to 64x64");
    } else {
        // 64x64 皮肤，直接复制
        result.resize(64 * 64 * 4);
        std::memcpy(result.data(), pixels, 64 * 64 * 4);
    }

    stbi_image_free(pixels);

    // 使用 stb_image_write 将 RGBA 数据编码为 PNG 格式
    // 使用内存写入模式，避免临时文件
    struct PngWriteContext {
        std::vector<u8> buffer;
    };

    PngWriteContext ctx;
    stbi_write_png_to_func(
        [](void* userdata, void* data, int size) {
            auto* context = static_cast<PngWriteContext*>(userdata);
            auto* bytes = static_cast<u8*>(data);
            context->buffer.insert(context->buffer.end(), bytes, bytes + size);
        },
        &ctx,
        64,     // width
        height, // height
        4,      // components (RGBA)
        result.data(),
        64 * 4 // stride
    );

    if (ctx.buffer.empty()) {
        return Error(ErrorCode::InvalidData, "Failed to encode skin data to PNG");
    }

    // 用编码后的 PNG 数据替换原始 RGBA 数据
    result = std::move(ctx.buffer);

    return result;
}

std::string FileSkinLoader::_calculateHash(const std::vector<u8>& data)
{
    // 使用 SHA-1 哈希算法计算缓存键
    auto digest = util::crypto::Sha1::hash(std::span<const u8>(data.data(), data.size()));
    return util::crypto::Sha1::toHexString(digest);
}

} // namespace mc::skin
