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

#include "ParticleTextureAtlas.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::particle {

namespace {

/**
 * @brief 使用统一 AnimationMetadata 解析动画帧信息
 *
 * @param mcmetaData .mcmeta 文件原始数据
 * @param imageWidth 纹理宽度
 * @param imageHeight 纹理高度
 * @param outFrameWidth 输出帧宽度
 * @param outFrameHeight 输出帧高度
 * @param outFrameTime 输出帧时间（秒）
 */
[[nodiscard]] bool parseAnimatedFrameSizeFromMcmeta(const std::vector<u8>& mcmetaData,
    u32 imageWidth,
    u32 imageHeight,
    u32& outFrameWidth,
    u32& outFrameHeight,
    f64& outFrameTime)
{
    if (mcmetaData.empty() || imageWidth == 0 || imageHeight == 0) {
        return false;
    }

    const auto metadata = resource::metadata::AnimationMetadata::fromMcmeta(mcmetaData, imageWidth, imageHeight);
    if (metadata.width <= 0 || metadata.height <= 0) {
        return false;
    }

    outFrameWidth = static_cast<u32>(metadata.width);
    outFrameHeight = static_cast<u32>(metadata.height);
    outFrameTime = metadata.frametime > 0 ? static_cast<f64>(metadata.frametime) / 20.0 : 0.1;
    return true;
}

/**
 * @brief 加载纹理像素数据
 */
Result<std::vector<u8>> loadTexturePixels(
    IResourcePack& pack, const ResourceLocation& location, u32& outWidth, u32& outHeight)
{
    std::string pngPath = location.toFilePath(resource::PackType::ClientResources, "png");
    pngPath.erase(0, std::string("assets/").size());
    const auto readResult = pack.readResource(resource::PackType::ClientResources, pngPath);
    if (readResult.failed()) {
        return readResult.error();
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(readResult.value().data(),
        static_cast<int>(readResult.value().size()),
        &width,
        &height,
        &channels,
        4); // 强制 RGBA

    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return Error(ErrorCode::TextureLoadFailed, "Failed to decode particle texture: " + location.toString());
    }

    outWidth = static_cast<u32>(width);
    outHeight = static_cast<u32>(height);

    std::vector<u8> result(pixels, pixels + (static_cast<size_t>(width) * static_cast<size_t>(height) * 4));
    stbi_image_free(pixels);
    return result;
}

} // namespace

ParticleTextureAtlas::ParticleTextureAtlas() = default;

ParticleTextureAtlas::~ParticleTextureAtlas()
{
    destroy();
}

ParticleTextureAtlas::ParticleTextureAtlas(ParticleTextureAtlas&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_commandPool(other.m_commandPool)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_image(other.m_image)
    , m_imageMemory(other.m_imageMemory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_imageWidth(other.m_imageWidth)
    , m_imageHeight(other.m_imageHeight)
    , m_sprites(std::move(other.m_sprites))
    , m_pixels(std::move(other.m_pixels))
    , m_uploaded(other.m_uploaded)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_imageMemory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_width = 0;
    other.m_height = 0;
    other.m_imageWidth = 0;
    other.m_imageHeight = 0;
    other.m_uploaded = false;
}

ParticleTextureAtlas& ParticleTextureAtlas::operator=(ParticleTextureAtlas&& other) noexcept
{
    if (this != &other) {
        destroy();

        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_commandPool = other.m_commandPool;
        m_graphicsQueue = other.m_graphicsQueue;
        m_image = other.m_image;
        m_imageMemory = other.m_imageMemory;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_width = other.m_width;
        m_height = other.m_height;
        m_imageWidth = other.m_imageWidth;
        m_imageHeight = other.m_imageHeight;
        m_sprites = std::move(other.m_sprites);
        m_pixels = std::move(other.m_pixels);
        m_uploaded = other.m_uploaded;

        other.m_device = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_commandPool = VK_NULL_HANDLE;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_imageMemory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_width = 0;
        other.m_height = 0;
        other.m_imageWidth = 0;
        other.m_imageHeight = 0;
        other.m_uploaded = false;
    }
    return *this;
}

Result<void> ParticleTextureAtlas::create(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 width,
    u32 height)
{
    MC_ASSERT(device != VK_NULL_HANDLE);
    MC_ASSERT(physicalDevice != VK_NULL_HANDLE);
    MC_ASSERT(commandPool != VK_NULL_HANDLE);
    MC_ASSERT(graphicsQueue != VK_NULL_HANDLE);
    MC_ASSERT(width > 0);
    MC_ASSERT(height > 0);

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_width = width;
    m_height = height;

    // 初始化像素缓冲区
    m_pixels.resize(static_cast<size_t>(width) * height * 4, 0);

    // 创建采样器
    auto samplerResult = _createSampler();
    if (samplerResult.failed()) {
        return samplerResult.error();
    }

    spdlog::info("[ParticleTextureAtlas] Created with size {}x{}", width, height);
    return {};
}

void ParticleTextureAtlas::destroy()
{
    if (m_device == VK_NULL_HANDLE) {
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

    m_sprites.clear();
    m_pixels.clear();
    m_uploaded = false;
    m_width = 0;
    m_height = 0;
    m_imageWidth = 0;
    m_imageHeight = 0;
    m_device = VK_NULL_HANDLE;
}

Result<void> ParticleTextureAtlas::loadFromResourcePacks(const std::vector<IResourcePack*>& resourcePacks)
{

    if (resourcePacks.empty()) {
        spdlog::warn("[ParticleTextureAtlas] No resource packs provided");
        return {};
    }

    // 使用 TextureAtlasBuilder 打包纹理
    TextureAtlasBuilder builder;
    builder.setMaxSize(m_width, m_height);
    builder.setPadding(1); // 1 像素边距防止纹理溢出

    // 收集所有粒子纹理
    std::vector<std::pair<ResourceLocation, std::tuple<std::vector<u8>, u32, u32, u32, u32, f64>>> textureData;

    for (IResourcePack* pack : resourcePacks) {
        if (pack == nullptr) {
            continue;
        }

        // 遍历粒子纹理目录
        // 粒子纹理路径: textures/particle/*.png
        const std::string particleDir = "textures/particle";

        // 常见的粒子纹理名称
        static const std::vector<std::string> particleTextures = {// 环境类
            "bubble",
            "bubble_pop_0",
            "bubble_pop_1",
            "bubble_pop_2",
            "bubble_pop_3",
            "bubble_pop_4",
            "underwater",
            // 火焰和烟雾
            "flame",
            "soul_fire_flame",
            "smoke",
            "large_smoke",
            "lava",
            // 传送门
            "portal",
            "reverse_portal",
            // 效果
            "explosion",
            "poof",
            "critical_hit",
            "enchanted_hit",
            "spell",
            "instant_spell",
            "entity_effect",
            // 红石
            "redstone_dust",
            // 附魔台
            "enchant_glyph",
            "end_rod",
            // 液体滴落
            "drip_hang",
            "drip_fall",
            "drip_land",
            // 天气
            "rain",
            "snowflake",
            "splash",
            // 生物
            "heart",
            "angry_villager",
            "happy_villager",
            // 特殊
            "totem_of_undying",
            "flash",
            "nautilus",
            // 下界
            "ash",
            "white_ash",
            "crimson_spore",
            "warped_spore",
            // 通用粒子纹理（菌丝粒子 SuspendedTownParticle 使用）
            "generic",
            // 其他
            "dragon_breath",
            "soul",
            "sculk_soul",
            "glow",
            "glow_squid_ink"};

        for (const std::string& textureName : particleTextures) {
            ResourceLocation location("minecraft", "particle/" + textureName);
            std::string path = location.toFilePath(resource::PackType::ClientResources, "png");
            path.erase(0, std::string("assets/").size());

            // 检查是否已加载（优先使用第一个找到的纹理）
            if (builder.getTextureLocations().size() > 0) {
                bool alreadyLoaded = false;
                for (const auto& loc : builder.getTextureLocations()) {
                    if (loc.toString() == location.toString()) {
                        alreadyLoaded = true;
                        break;
                    }
                }
                if (alreadyLoaded) {
                    continue;
                }
            }

            // 尝试加载纹理
            u32 width = 0, height = 0;
            auto pixelResult = loadTexturePixels(*pack, location, width, height);
            if (pixelResult.failed()) {
                continue; // 纹理不存在，跳过
            }

            // 检查动画元数据
            u32 frameWidth = width;
            u32 frameHeight = width; // 默认为正方形帧
            f64 frameTime = 0.1;     // 默认帧时间

            const std::string mcmetaPath = path + ".mcmeta";
            if (pack->hasResource(resource::PackType::ClientResources, mcmetaPath)) {
                auto mcmetaResult = pack->readResource(resource::PackType::ClientResources, mcmetaPath);
                if (mcmetaResult.success()) {
                    static_cast<void>(parseAnimatedFrameSizeFromMcmeta(
                        mcmetaResult.value(), width, height, frameWidth, frameHeight, frameTime));
                }
            }

            // 添加到构建器（使用第一帧尺寸）
            builder.addTextureFrame(location, pixelResult.value(), width, height, frameWidth, frameHeight);

            // 存储动画信息
            textureData.emplace_back(
                location, std::make_tuple(pixelResult.value(), width, height, frameWidth, frameHeight, frameTime));
        }
    }

    // 构建图集
    auto buildResult = builder.build();
    if (buildResult.failed()) {
        return buildResult.error();
    }

    const auto& atlasResult = buildResult.value();
    m_pixels = std::move(atlasResult.pixels);

    // 更新图集尺寸
    if (atlasResult.width > m_width || atlasResult.height > m_height) {
        spdlog::warn("[ParticleTextureAtlas] Atlas size {}x{} exceeds configured {}x{}, resizing",
            atlasResult.width,
            atlasResult.height,
            m_width,
            m_height);
        m_width = atlasResult.width;
        m_height = atlasResult.height;
    }

    // 从构建结果创建精灵信息
    for (const auto& [location, region] : atlasResult.regions) {
        SpriteInfo info;
        info.uvMin = glm::vec2(region.u0, region.v0);
        info.uvMax = glm::vec2(region.u1, region.v1);

        // 查找动画信息
        for (const auto& [loc, data] : textureData) {
            if (loc == location) {
                const auto& [pixels, w, h, fw, fh, ft] = data;
                // 计算帧数（垂直帧条）
                info.frameCount = (fw > 0 && fh > 0) ? (h / fh) : 1;
                info.frameTime = ft;
                break;
            }
        }

        m_sprites[location] = info;
    }

    spdlog::info("[ParticleTextureAtlas] Loaded {} particle textures, atlas size {}x{}",
        m_sprites.size(),
        atlasResult.width,
        atlasResult.height);

    return {};
}

Result<void> ParticleTextureAtlas::upload()
{
    if (m_uploaded) {
        return {};
    }

    if (m_pixels.empty()) {
        spdlog::warn("[ParticleTextureAtlas] No texture data to upload");
        return {};
    }

    // 创建或重新创建图像
    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }
    if (m_imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    auto imageResult = _createImage();
    if (imageResult.failed()) {
        return imageResult.error();
    }

    auto viewResult = _createImageView();
    if (viewResult.failed()) {
        return viewResult.error();
    }

    // 上传纹理数据
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_width) * m_height * 4;

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

    auto memTypeResult = _findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (memTypeResult.failed()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate staging memory");
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingMemory, 0);

    void* data = nullptr;
    if (vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &data) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to map staging memory");
    }

    std::memcpy(data, m_pixels.data(), imageSize);
    vkUnmapMemory(m_device, stagingMemory);

    // 复制到图像
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

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

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    // 释放像素数据
    m_pixels.clear();
    m_pixels.shrink_to_fit();

    m_uploaded = true;
    m_imageWidth = m_width;
    m_imageHeight = m_height;

    spdlog::info("[ParticleTextureAtlas] Uploaded texture atlas to GPU");
    return {};
}

const SpriteInfo* ParticleTextureAtlas::getSprite(const ResourceLocation& location) const
{
    auto it = m_sprites.find(location);
    if (it != m_sprites.end()) {
        return &it->second;
    }
    return nullptr;
}

glm::vec4 ParticleTextureAtlas::getAnimatedFrameUV(const ResourceLocation& location, f64 age, f64 maxAge) const
{
    const SpriteInfo* sprite = getSprite(location);
    if (sprite == nullptr) {
        // 返回默认 UV（整个图集）
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    if (!sprite->isAnimated() || maxAge <= 0.0f) {
        return glm::vec4(sprite->uvMin.x, sprite->uvMin.y, sprite->uvMax.x, sprite->uvMax.y);
    }

    // 计算当前帧
    const f64 frameHeight = sprite->frameHeight();
    const f64 totalHeight = sprite->uvMax.y - sprite->uvMin.y;
    const f64 frameVHeight = frameHeight / totalHeight;

    // 基于时间的帧选择
    const u32 frameIndex = static_cast<u32>((age / maxAge) * static_cast<f64>(sprite->frameCount));
    const u32 clampedFrame = std::min(frameIndex, sprite->frameCount - 1);

    const f64 frameVOffset = sprite->uvMin.y + static_cast<f64>(clampedFrame) * frameVHeight;

    return glm::vec4(sprite->uvMin.x, frameVOffset, sprite->uvMax.x, frameVOffset + frameVHeight);
}

glm::vec4 ParticleTextureAtlas::getRandomFrameUV(const ResourceLocation& location, u32 seed) const
{
    const SpriteInfo* sprite = getSprite(location);
    if (sprite == nullptr) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    if (!sprite->isAnimated()) {
        return glm::vec4(sprite->uvMin.x, sprite->uvMin.y, sprite->uvMax.x, sprite->uvMax.y);
    }

    // 使用种子选择帧
    const u32 frameIndex = seed % sprite->frameCount;
    const f64 frameHeight = sprite->frameHeight();
    const f64 totalHeight = sprite->uvMax.y - sprite->uvMin.y;
    const f64 frameVHeight = frameHeight / totalHeight;
    const f64 frameVOffset = sprite->uvMin.y + static_cast<f64>(frameIndex) * frameVHeight;

    return glm::vec4(sprite->uvMin.x, frameVOffset, sprite->uvMax.x, frameVOffset + frameVHeight);
}

Result<void> ParticleTextureAtlas::_createImage()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create particle texture atlas image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    auto memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (memTypeResult.failed()) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate particle texture atlas memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
    return {};
}

Result<void> ParticleTextureAtlas::_createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create particle texture atlas sampler");
    }

    return {};
}

Result<void> ParticleTextureAtlas::_createImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create particle texture atlas image view");
    }

    return {};
}

Result<u32> ParticleTextureAtlas::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::OutOfMemory, "Failed to find suitable memory type");
}

VkCommandBuffer ParticleTextureAtlas::_beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void ParticleTextureAtlas::_endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void ParticleTextureAtlas::_transitionImageLayout(VkCommandBuffer cmd,
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

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

} // namespace mc::client::renderer::trident::particle
