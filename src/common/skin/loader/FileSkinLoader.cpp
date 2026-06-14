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

#include "FileSkinLoader.hpp"
#include "common/util/crypto/Sha1.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

// stb_image for PNG loading
#include <stb_image.h>

// stb_image_write for PNG encoding
#include <stb_image_write.h>

namespace mc::skin {

FileSkinLoader::FileSkinLoader(IResourcePack* resourcePack)
    : m_resourcePack(resourcePack)
{}

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
    m_initialized = false;
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
    // TODO: 使用线程池实现真正的异步加载
    auto result = load(url);
    callback(std::move(result));
}

void FileSkinLoader::cancel(const std::string& url)
{
    // 文件加载是同步的，无法取消
}

void FileSkinLoader::cancelAll()
{
    // 文件加载是同步的，无法取消
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
    if (!m_resourcePack) {
        return Error(ErrorCode::NotInitialized, "No resource pack available");
    }

    SkinLoadResult result;

    // 从资源包读取
    // toFilePath(PackType) 返回 "assets/namespace/path" 格式，
    // readResource 需要相对于 PackType 根目录的路径（不含 "assets/" 前缀）
    std::string resourcePath = location.toFilePath(resource::PackType::ClientResources);
    resourcePath.erase(0, std::string("assets/").size());

    auto readResult = m_resourcePack->readResource(resource::PackType::ClientResources, resourcePath);
    if (!readResult.success()) {
        return readResult.error();
    }

    auto& data = readResult.value();

    // 验证和转换
    auto validateResult = _validateAndConvertSkin(data);
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
