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

#include "EntityTextureAtlas.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/resource/atlas/TexturePathVariant.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <cstring>
#include <fstream>
#include <spdlog/spdlog.h>

// stb_image is already implemented in TextureAtlasBuilder.cpp
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <ios>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::pipeline {

EntityTextureAtlas::EntityTextureAtlas() = default;

EntityTextureAtlas::~EntityTextureAtlas()
{
    destroy();
}

Result<void> EntityTextureAtlas::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 maxTextures,
    u32 textureSize)
{
    if (m_initialized) {
        return {};
    }

    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Device is null");
    }
    if (commandPool == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Command pool is null");
    }
    if (graphicsQueue == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Graphics queue is null");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_maxTextures = maxTextures;
    m_textureSize = textureSize;
    m_textures.reserve(maxTextures);

    // 创建采样器
    auto result = _createSampler();
    if (!result.success()) {
        return result;
    }

    m_initialized = true;
    spdlog::info("EntityTextureAtlas initialized (max: {}, size: {})", maxTextures, textureSize);
    return {};
}

void EntityTextureAtlas::destroy()
{
    if (!m_initialized) {
        return;
    }

    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }

    if (m_imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    m_textures.clear();
    m_queuedTextures.clear();
    m_regions.clear();
    m_width = 0;
    m_height = 0;

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;

    m_built = false;
    m_needsRebuild = false;
    m_initialized = false;

    spdlog::info("EntityTextureAtlas destroyed");
}

Result<void> EntityTextureAtlas::addTexture(mc::IResourcePack& pack, const ResourceLocation& location)
{
    if (m_built) {
        return Error(ErrorCode::InvalidState, "Atlas already built");
    }

    // 检查是否已存在（在已加载纹理中）
    for (const auto& tex : m_textures) {
        if (tex.location == location) {
            return {}; // 已存在，忽略
        }
    }

    // 加载纹理
    TextureData texData;
    texData.location = location;

    auto result = _loadTextureWithFallback(pack, location, texData.pixels, texData.width, texData.height);
    if (!result.success()) {
        spdlog::warn("Failed to load entity texture: {} - {}", location.toString(), result.error().toString());
        return result.error();
    }

    m_textures.push_back(std::move(texData));
    return {};
}

Result<void> EntityTextureAtlas::addTextureFromFile(
    const std::filesystem::path& filePath, const ResourceLocation& location)
{
    // 检查是否已存在（按资源位置去重）
    for (const auto& tex : m_textures) {
        if (tex.location == location) {
            return {};
        }
    }
    for (const auto& tex : m_queuedTextures) {
        if (tex.location == location) {
            return {};
        }
    }

    if (!std::filesystem::exists(filePath)) {
        return Error(ErrorCode::FileNotFound, "Skin file not found: " + filePath.string());
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open skin file: " + filePath.string());
    }

    std::vector<u8> encoded((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    if (encoded.empty()) {
        return Error(ErrorCode::FileReadFailed, "Skin file is empty: " + filePath.string());
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    u8* pixels = stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()), &width, &height, &channels, 4);

    if (pixels == nullptr) {
        return Error(ErrorCode::InvalidData, "Failed to decode skin PNG: " + filePath.string());
    }

    TextureData texData;
    texData.location = location;

    // 兼容旧版 Java 皮肤（64x32）
    if (width == 64 && height == 32) {
        texData.width = 64;
        texData.height = 64;
        texData.pixels.assign(static_cast<size_t>(64 * 64 * 4), 0);

        // 顶部 32 行直接复制
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>(y * 64 * 4);
            std::memcpy(texData.pixels.data() + dstOffset, pixels + srcOffset, static_cast<size_t>(64 * 4));
        }

        // 旧皮肤没有第二层与独立左肢，复制一份到下半区作为兼容兜底。
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>((y + 32) * 64 * 4);
            std::memcpy(texData.pixels.data() + dstOffset, pixels + srcOffset, static_cast<size_t>(64 * 4));
        }

        spdlog::info("EntityTextureAtlas: Loaded legacy 64x32 skin and expanded to 64x64: {}", filePath.string());
    } else {
        texData.width = static_cast<u32>(width);
        texData.height = static_cast<u32>(height);
        texData.pixels.resize(static_cast<size_t>(width * height * 4));
        std::memcpy(texData.pixels.data(), pixels, texData.pixels.size());
        spdlog::info("EntityTextureAtlas: Loaded skin from file {} ({}x{})", filePath.string(), width, height);
    }

    stbi_image_free(pixels);

    if (m_built) {
        // 图集已构建，添加到队列等待重建
        m_queuedTextures.push_back(std::move(texData));
        m_needsRebuild = true;
        spdlog::info("EntityTextureAtlas: Queued texture for rebuild: {}", location.toString());
    } else {
        m_textures.push_back(std::move(texData));
    }
    return {};
}

Result<void> EntityTextureAtlas::addTextureFromPixels(
    const std::vector<u8>& pixels, u32 width, u32 height, const ResourceLocation& location)
{
    // 检查是否已存在
    for (const auto& tex : m_textures) {
        if (tex.location == location) {
            return {};
        }
    }
    for (const auto& tex : m_queuedTextures) {
        if (tex.location == location) {
            return {};
        }
    }

    if (pixels.empty()) {
        return Error(ErrorCode::InvalidData, "Empty pixel data for: " + location.toString());
    }

    if (pixels.size() != static_cast<size_t>(width * height * 4)) {
        return Error(ErrorCode::InvalidData, "Pixel data size mismatch for: " + location.toString());
    }

    TextureData texData;
    texData.location = location;
    texData.width = width;
    texData.height = height;
    texData.pixels = pixels;

    if (m_built) {
        m_queuedTextures.push_back(std::move(texData));
        m_needsRebuild = true;
        spdlog::info(
            "EntityTextureAtlas: Queued pixel texture for rebuild: {} ({}x{})", location.toString(), width, height);
    } else {
        m_textures.push_back(std::move(texData));
    }

    return {};
}

Result<EntityAtlasBuildResult> EntityTextureAtlas::build()
{
    if (m_built) {
        EntityAtlasBuildResult result;
        result.image = m_image;
        result.imageMemory = m_imageMemory;
        result.imageView = m_imageView;
        result.width = m_width;
        result.height = m_height;
        result.regions = m_regions;
        return result; // 隐式转换
    }

    if (m_textures.empty()) {
        spdlog::warn("EntityTextureAtlas::build() called with no textures");
        return Error(ErrorCode::InvalidState, "No textures to build");
    }

    // 计算图集尺寸
    // 使用简单的行布局
    u32 texturesPerRow = static_cast<u32>(std::sqrt(static_cast<f64>(m_textures.size())));
    if (texturesPerRow == 0) texturesPerRow = 1;

    u32 rowCount = static_cast<u32>((m_textures.size() + texturesPerRow - 1) / texturesPerRow);

    m_width = texturesPerRow * m_textureSize;
    u32 staticHeight = rowCount * m_textureSize;

    // 确保尺寸是2的幂次方（有利于GPU）
    auto nextPowerOf2 = [](u32 n) {
        u32 power = 1;
        while (power < n)
            power *= 2;
        return power;
    };

    m_width = nextPowerOf2(m_width);
    // 静态布局高度先单独 round up，作为动态区域起始 Y；再为动态区域预留空间
    m_dynamicOffsetY = nextPowerOf2(staticHeight);
    m_dynamicUsedHeight = 0;
    m_height = nextPowerOf2(m_dynamicOffsetY + DYNAMIC_RESERVE_HEIGHT);
    // 宽度至少能容纳一个纹理槽位
    if (m_width < m_textureSize) m_width = m_textureSize;

    spdlog::info("Building entity texture atlas: {}x{} ({} textures, dynamic reserve from Y={})",
        m_width,
        m_height,
        m_textures.size(),
        m_dynamicOffsetY);

    // 创建图像
    auto result = _createImage(m_width, m_height);
    if (!result.success()) {
        return result.error();
    }

    // 准备图集像素数据
    std::vector<u8> atlasData(m_width * m_height * 4, 0);

    // 放置纹理
    for (size_t i = 0; i < m_textures.size(); ++i) {
        const auto& tex = m_textures[i];

        u32 col = static_cast<u32>(i) % texturesPerRow;
        u32 row = static_cast<u32>(i) / texturesPerRow;

        u32 offsetX = col * m_textureSize;
        u32 offsetY = row * m_textureSize;

        // 计算UV坐标
        TextureRegion region;
        region.u0 = static_cast<f64>(offsetX) / static_cast<f64>(m_width);
        region.v0 = static_cast<f64>(offsetY) / static_cast<f64>(m_height);
        region.u1 = static_cast<f64>(offsetX + tex.width) / static_cast<f64>(m_width);
        region.v1 = static_cast<f64>(offsetY + tex.height) / static_cast<f64>(m_height);

        m_regions[tex.location] = region;

        // 复制像素数据
        for (u32 y = 0; y < tex.height; ++y) {
            for (u32 x = 0; x < tex.width; ++x) {
                u32 srcIdx = (y * tex.width + x) * 4;
                u32 dstIdx = ((offsetY + y) * m_width + (offsetX + x)) * 4;

                if (srcIdx + 3 < tex.pixels.size() && dstIdx + 3 < atlasData.size()) {
                    atlasData[dstIdx + 0] = tex.pixels[srcIdx + 0];
                    atlasData[dstIdx + 1] = tex.pixels[srcIdx + 1];
                    atlasData[dstIdx + 2] = tex.pixels[srcIdx + 2];
                    atlasData[dstIdx + 3] = tex.pixels[srcIdx + 3];
                }
            }
        }
    }

    // 上传到GPU
    auto uploadResult = _uploadTextureData(atlasData);
    if (!uploadResult.success()) {
        return uploadResult.error();
    }

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create image view");
    }

    m_built = true;
    m_needsRebuild = false;
    spdlog::info("Entity texture atlas built successfully");

    EntityAtlasBuildResult buildResult;
    buildResult.image = m_image;
    buildResult.imageMemory = m_imageMemory;
    buildResult.imageView = m_imageView;
    buildResult.width = m_width;
    buildResult.height = m_height;
    buildResult.regions = m_regions;

    // 注意：不清除 m_textures，保留用于后续重建
    // 清理临时队列
    m_queuedTextures.clear();

    return buildResult; // 隐式转换
}

Result<EntityAtlasBuildResult> EntityTextureAtlas::rebuild()
{
    if (!m_built) {
        // 如果从未构建过，使用 build()
        return build();
    }

    if (m_queuedTextures.empty()) {
        // 没有待添加的纹理，直接返回当前状态
        EntityAtlasBuildResult result;
        result.image = m_image;
        result.imageMemory = m_imageMemory;
        result.imageView = m_imageView;
        result.width = m_width;
        result.height = m_height;
        result.regions = m_regions;
        return result;
    }

    spdlog::info("EntityTextureAtlas: Rebuilding atlas with {} new textures (existing: {})",
        m_queuedTextures.size(),
        m_textures.size());

    // 合并现有纹理和新纹理
    // 检查新纹理是否已存在
    for (auto& newTex : m_queuedTextures) {
        bool exists = false;
        for (const auto& existingTex : m_textures) {
            if (existingTex.location == newTex.location) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_textures.push_back(std::move(newTex));
        }
    }
    m_queuedTextures.clear();

    // 清除旧的区域映射
    m_regions.clear();

    // 销毁旧的图集资源
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }
    if (m_imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }

    // 重新构建图集
    m_built = false;
    m_needsRebuild = false;
    auto result = build();

    if (result.success()) {
        spdlog::info("EntityTextureAtlas: Atlas rebuilt successfully (now has {} textures)", m_regions.size());
    } else {
        spdlog::error("EntityTextureAtlas: Failed to rebuild atlas: {}", result.error().toString());
    }

    return result;
}

const TextureRegion* EntityTextureAtlas::getRegion(const ResourceLocation& location) const
{
    auto it = m_regions.find(location);
    return it != m_regions.end() ? &it->second : nullptr;
}

const TextureRegion* EntityTextureAtlas::getRegion(const std::string& location) const
{
    ResourceLocation loc(location);
    return getRegion(loc);
}

Result<void> EntityTextureAtlas::_loadTextureWithFallback(
    mc::IResourcePack& pack, const ResourceLocation& location, std::vector<u8>& outData, u32& outWidth, u32& outHeight)
{
    // 尝试直接加载（使用文件路径格式）
    std::string filePath = location.toFilePath(mc::resource::PackType::ClientResources);
    filePath.erase(0, std::string("assets/").size());

    auto result = pack.readResource(mc::resource::PackType::ClientResources, filePath);
    if (result.success()) {
        auto& data = result.value();
        int width, height, channels;
        u8* pixels = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);
        if (pixels) {
            outWidth = static_cast<u32>(width);
            outHeight = static_cast<u32>(height);
            outData.resize(width * height * 4);
            std::memcpy(outData.data(), pixels, outData.size());
            stbi_image_free(pixels);
            return {};
        }
    }

    // 尝试路径变体（使用 getAltTexturePath 集中化转换）
    // 例如：textures/entity/pig/pig -> textures/entity/pig
    //       textures/entity/pig     -> textures/entity/pig/pig
    //       textures/block/stone    -> textures/blocks/stone
    std::string altPath = resource::atlas::TexturePathVariant::getAltTexturePath(location.path());
    if (!altPath.empty()) {
        ResourceLocation altLoc(location.namespace_(), altPath);
        std::string altFilePath = altLoc.toFilePath(mc::resource::PackType::ClientResources);
        altFilePath.erase(0, std::string("assets/").size());

        result = pack.readResource(mc::resource::PackType::ClientResources, altFilePath);
        if (result.success()) {
            auto& data = result.value();
            int width, height, channels;
            u8* pixels =
                stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);
            if (pixels) {
                outWidth = static_cast<u32>(width);
                outHeight = static_cast<u32>(height);
                outData.resize(width * height * 4);
                std::memcpy(outData.data(), pixels, outData.size());
                stbi_image_free(pixels);
                return {};
            }
        }
    }

    // 尝试不带 textures/ 前缀的路径（某些资源包格式）
    std::string texturePath = location.path();
    if (texturePath.find("textures/") == 0) {
        std::string directPath = location.namespace_() + "/" + texturePath;
        result = pack.readResource(mc::resource::PackType::ClientResources, directPath);
        if (result.success()) {
            auto& data = result.value();
            int width, height, channels;
            u8* pixels =
                stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);
            if (pixels) {
                outWidth = static_cast<u32>(width);
                outHeight = static_cast<u32>(height);
                outData.resize(width * height * 4);
                std::memcpy(outData.data(), pixels, outData.size());
                stbi_image_free(pixels);
                return {};
            }
        }
    }

    return Error(ErrorCode::ResourceNotFound, "Failed to load texture: " + location.toString());
}

Result<void> EntityTextureAtlas::_createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST; // 实体使用最近邻过滤
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sampler");
    }

    return {};
}

Result<void> EntityTextureAtlas::_createImage(u32 width, u32 height)
{
    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memTypeResult.success()) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate image memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);

    return {};
}

Result<void> EntityTextureAtlas::_uploadTextureData(const std::vector<u8>& data)
{
    VkDeviceSize imageSize = data.size();

    // 创建暂存缓冲区
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create staging buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = _findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!memTypeResult.success()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate staging memory");
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingMemory, 0);

    // 复制数据
    void* mappedData = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mappedData);
    if (mapResult != VK_SUCCESS || mappedData == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to map entity texture staging memory");
    }
    std::memcpy(mappedData, data.data(), imageSize);
    vkUnmapMemory(m_device, stagingMemory);

    // 转换图像布局并复制
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    // 转换到传输目标布局
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换到着色器只读布局
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    _endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

Result<u32> EntityTextureAtlas::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return ::mc::client::renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer EntityTextureAtlas::_beginSingleTimeCommands()
{
    return ::mc::client::renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void EntityTextureAtlas::_endSingleTimeCommands(VkCommandBuffer cmd)
{
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    ::mc::client::renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);
}

void EntityTextureAtlas::_transitionImageLayout(VkCommandBuffer cmd,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

Result<void> EntityTextureAtlas::_uploadRegion(const u8* pixels, u32 offsetX, u32 offsetY, u32 width, u32 height)
{
    if (pixels == nullptr) {
        return Error(ErrorCode::NullPointer, "Entity atlas region pixel data is null");
    }
    if (m_image == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Entity atlas not built");
    }
    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "Entity atlas region dimensions must be non-zero");
    }

    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    // 创建暂存缓冲区
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    auto bufferResult = renderer::VulkanUtils::createBuffer(m_device,
        m_physicalDevice,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);
    if (!bufferResult.success()) {
        return bufferResult.error();
    }

    // 映射并复制数据
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, stagingMemory, 0, size, 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer for entity atlas region upload");
    }
    std::memcpy(mapped, pixels, static_cast<size_t>(size));
    vkUnmapMemory(m_device, stagingMemory);

    // 录制命令：SHADER_READ_ONLY → TRANSFER_DST → copy → SHADER_READ_ONLY
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = width;
    region.bufferImageHeight = height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY), 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

const TextureRegion* EntityTextureAtlas::injectRegion(
    const ResourceLocation& location, u32 width, u32 height, const u8* rgbaPixels)
{
    if (!m_built) {
        spdlog::warn("EntityTextureAtlas::injectRegion called before build: {}", location.toString());
        return nullptr;
    }
    if (rgbaPixels == nullptr) {
        spdlog::warn("EntityTextureAtlas::injectRegion null pixels: {}", location.toString());
        return nullptr;
    }
    if (width > m_width) {
        spdlog::warn("EntityTextureAtlas::injectRegion width {} exceeds atlas width {}: {}",
            width,
            m_width,
            location.toString());
        return nullptr;
    }

    // 纵向 shelf 分配：新区域放在 m_dynamicOffsetY + m_dynamicUsedHeight
    const u32 offsetY = m_dynamicOffsetY + m_dynamicUsedHeight;
    if (offsetY + height > m_height) {
        spdlog::warn("EntityTextureAtlas::injectRegion dynamic reserve exhausted "
                     "(need Y={}, atlas height={}): {}",
            offsetY + height,
            m_height,
            location.toString());
        return nullptr;
    }

    auto uploadResult = _uploadRegion(rgbaPixels, 0, offsetY, width, height);
    if (!uploadResult.success()) {
        spdlog::warn("EntityTextureAtlas::injectRegion upload failed: {} ({})",
            location.toString(),
            uploadResult.error().toString());
        return nullptr;
    }

    TextureRegion region;
    region.u0 = 0.0;
    region.v0 = static_cast<f64>(offsetY) / static_cast<f64>(m_height);
    region.u1 = static_cast<f64>(width) / static_cast<f64>(m_width);
    region.v1 = static_cast<f64>(offsetY + height) / static_cast<f64>(m_height);

    const auto [it, inserted] = m_regions.try_emplace(location, region);
    if (!inserted) {
        // 同名区域已存在（幂等重注入），仅更新 UV
        it->second = region;
    } else {
        m_dynamicRegions.push_back(location);
    }

    m_dynamicUsedHeight += height;
    ++m_contentVersion;
    return &it->second;
}

void EntityTextureAtlas::removeDynamicRegion(const ResourceLocation& location)
{
    auto it = m_regions.find(location);
    if (it == m_regions.end()) {
        return;
    }
    // 仅移除动态区域；静态区域（build 时加入）不受影响
    auto dynIt = std::find(m_dynamicRegions.begin(), m_dynamicRegions.end(), location);
    if (dynIt == m_dynamicRegions.end()) {
        return;
    }
    m_dynamicRegions.erase(dynIt);
    m_regions.erase(it);
    ++m_contentVersion;
}

} // namespace mc::client::renderer::entity::pipeline
