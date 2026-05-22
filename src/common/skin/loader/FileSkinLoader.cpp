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
#include <fstream>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

// stb_image for PNG loading
#include <stb_image.h>

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
        auto loadResult = loadFromResourcePack(location);
        if (loadResult.success()) {
            return loadResult;
        }
    }

    // 尝试作为文件路径加载
    return loadFromFilesystem(url);
}

void FileSkinLoader::loadAsync(const std::string& url, std::function<void(Result<SkinLoadResult>)> callback)
{
    // 简单实现：同步加载后调用回调
    // 生产环境应该使用线程池
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

Result<SkinLoadResult> FileSkinLoader::loadFromFilesystem(const std::string& path)
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
    auto validateResult = validateAndConvertSkin(data);
    if (!validateResult.success()) {
        return validateResult.error();
    }

    result.pngData = validateResult.value();
    result.hash = calculateHash(result.pngData);

    spdlog::debug("FileSkinLoader: Loaded skin from {} ({} bytes, hash: {})", path, result.pngData.size(), result.hash);
    return result;
}

Result<SkinLoadResult> FileSkinLoader::loadFromResourcePack(const ResourceLocation& location)
{
    if (!m_resourcePack) {
        return Error(ErrorCode::NotInitialized, "No resource pack available");
    }

    SkinLoadResult result;

    // 从资源包读取
    auto readResult =
        m_resourcePack->readResource(resource::PackType::ClientResources, location.toFilePath(resource::PackType::ClientResources));
    if (!readResult.success()) {
        return readResult.error();
    }

    auto& data = readResult.value();

    // 验证和转换
    auto validateResult = validateAndConvertSkin(data);
    if (!validateResult.success()) {
        return validateResult.error();
    }

    result.pngData = validateResult.value();
    result.hash = calculateHash(result.pngData);

    spdlog::debug("FileSkinLoader: Loaded skin from resource pack {} ({} bytes, hash: {})",
        location.toString(),
        result.pngData.size(),
        result.hash);
    return result;
}

Result<std::vector<u8>> FileSkinLoader::validateAndConvertSkin(const std::vector<u8>& pngData)
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

    // 将 RGBA 数据编码回 PNG（简化实现：直接返回 RGBA 数据）
    // 生产环境应该使用 stb_image_write 编码为 PNG
    // 这里简化处理，假设调用者会处理原始 RGBA 数据

    return result;
}

std::string FileSkinLoader::calculateHash(const std::vector<u8>& data)
{
    // 简化的哈希计算（生产环境应该使用 SHA1）
    // 这里使用简单的累加哈希作为 fallback
    u64 hash = 0xcbf29ce484222325ULL;       // FNV offset basis
    constexpr u64 prime = 0x100000001b3ULL; // FNV prime

    for (u8 byte : data) {
        hash ^= byte;
        hash *= prime;
    }

    // 转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>((hash >> (i * 4)) & 0xF);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            // UUID 格式
        }
    }

    return oss.str();
}

} // namespace mc::skin
