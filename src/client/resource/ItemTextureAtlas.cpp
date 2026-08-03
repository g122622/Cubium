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

#include "ItemTextureAtlas.hpp"
#include "TextureAtlasBuilder.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/resource/atlas/TexturePathVariant.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <spdlog/spdlog.h>

// stb_image - only header, implementation in TextureAtlasBuilder.cpp
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace mc::client {

namespace {

Result<void> loadTexturePixels(IResourcePack& pack,
    const ResourceLocation& location,
    std::vector<u8>& outPixels,
    u32& outWidth,
    u32& outHeight,
    u32& outFrameWidth,
    u32& outFrameHeight)
{
    std::string pngPath = location.toFilePath(mc::resource::PackType::ClientResources, "png");
    pngPath.erase(0, std::string("assets/").size());
    const auto readResult = pack.readResource(mc::resource::PackType::ClientResources, pngPath);
    if (readResult.failed()) {
        return readResult.error();
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(
        readResult.value().data(), static_cast<int>(readResult.value().size()), &width, &height, &channels, 4);

    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return Error(ErrorCode::TextureLoadFailed, "Failed to decode item texture: " + location.toString());
    }

    outWidth = static_cast<u32>(width);
    outHeight = static_cast<u32>(height);
    outFrameWidth = outWidth;
    outFrameHeight = outHeight;

    const std::string mcmetaPath = pngPath + ".mcmeta";
    if (pack.hasResource(mc::resource::PackType::ClientResources, mcmetaPath)) {
        const auto mcmetaResult = pack.readResource(mc::resource::PackType::ClientResources, mcmetaPath);
        if (mcmetaResult.success()) {
            const auto metadata =
                mc::resource::metadata::AnimationMetadata::fromMcmeta(mcmetaResult.value(), outWidth, outHeight);
            if (metadata.width > 0 && metadata.height > 0) {
                outFrameWidth = static_cast<u32>(metadata.width);
                outFrameHeight = static_cast<u32>(metadata.height);
            }
        }
    }

    outPixels.assign(pixels, pixels + (static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 4));
    stbi_image_free(pixels);
    return {};
}

std::vector<u8> resizeNearestRGBA(
    const std::vector<u8>& srcPixels, u32 srcWidth, u32 srcHeight, u32 dstWidth, u32 dstHeight)
{
    std::vector<u8> dstPixels(static_cast<size_t>(dstWidth) * static_cast<size_t>(dstHeight) * 4, 0);

    for (u32 y = 0; y < dstHeight; ++y) {
        const u32 srcY = (y * srcHeight) / dstHeight;
        for (u32 x = 0; x < dstWidth; ++x) {
            const u32 srcX = (x * srcWidth) / dstWidth;

            const size_t srcIndex = (static_cast<size_t>(srcY) * srcWidth + srcX) * 4;
            const size_t dstIndex = (static_cast<size_t>(y) * dstWidth + x) * 4;

            dstPixels[dstIndex + 0] = srcPixels[srcIndex + 0];
            dstPixels[dstIndex + 1] = srcPixels[srcIndex + 1];
            dstPixels[dstIndex + 2] = srcPixels[srcIndex + 2];
            dstPixels[dstIndex + 3] = srcPixels[srcIndex + 3];
        }
    }

    return dstPixels;
}

} // namespace

ItemTextureAtlas::ItemTextureAtlas() = default;

ItemTextureAtlas::~ItemTextureAtlas()
{
    destroy();
}

ItemTextureAtlas::ItemTextureAtlas(ItemTextureAtlas&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_commandPool(other.m_commandPool)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_image(other.m_image)
    , m_imageMemory(other.m_imageMemory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_imageWidth(other.m_imageWidth)
    , m_imageHeight(other.m_imageHeight)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_uploaded(other.m_uploaded)
    , m_regionsByItemId(std::move(other.m_regionsByItemId))
    , m_regionsByLocation(std::move(other.m_regionsByLocation))
    , m_pixels(std::move(other.m_pixels))
{
    other.m_device = VK_NULL_HANDLE;
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_imageMemory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_imageWidth = 0;
    other.m_imageHeight = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_uploaded = false;
}

ItemTextureAtlas& ItemTextureAtlas::operator=(ItemTextureAtlas&& other) noexcept
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
        m_imageWidth = other.m_imageWidth;
        m_imageHeight = other.m_imageHeight;
        m_width = other.m_width;
        m_height = other.m_height;
        m_uploaded = other.m_uploaded;
        m_regionsByItemId = std::move(other.m_regionsByItemId);
        m_regionsByLocation = std::move(other.m_regionsByLocation);
        m_pixels = std::move(other.m_pixels);

        other.m_device = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_commandPool = VK_NULL_HANDLE;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_imageMemory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_imageWidth = 0;
        other.m_imageHeight = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_uploaded = false;
    }
    return *this;
}

Result<void> ItemTextureAtlas::create(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 width,
    u32 height)
{
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
    m_width = width;
    m_height = height;

    // Create image
    auto imageResult = _createImage();
    if (!imageResult.success()) {
        return imageResult.error();
    }

    // Create image view
    auto viewResult = _createImageView();
    if (!viewResult.success()) {
        vkDestroyImage(m_device, m_image, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
        return viewResult.error();
    }

    // Create sampler
    auto samplerResult = _createSampler();
    if (!samplerResult.success()) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        vkDestroyImage(m_device, m_image, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageView = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
        return samplerResult.error();
    }

    // Initialize pixel buffer (transparent)
    m_pixels.resize(static_cast<size_t>(width) * height * 4, 0);

    return {};
}

void ItemTextureAtlas::destroy()
{
    if (m_sampler != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_imageView != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    if (m_image != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }

    if (m_imageMemory != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_imageWidth = 0;
    m_imageHeight = 0;
    m_width = 0;
    m_height = 0;
    m_uploaded = false;
    m_pixels.clear();
    m_regionsByItemId.clear();
    m_regionsByLocation.clear();
}

Result<void> ItemTextureAtlas::loadFromResourcePacks(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks)
{
    if (resourcePacks.empty()) {
        spdlog::warn("ItemTextureAtlas: No resource packs provided");
        return {};
    }

    m_uploaded = false;
    m_regionsByItemId.clear();
    m_regionsByLocation.clear();

    TextureAtlasBuilder builder;
    const u32 atlasWidth = (m_width > 0) ? m_width : 1024;
    const u32 atlasHeight = (m_height > 0) ? m_height : 1024;
    builder.setMaxSize(atlasWidth, atlasHeight);

    auto tryLoadToBuilder = [&](const ResourceLocation& atlasKey,
                                const std::vector<ResourceLocation>& sourceCandidates) -> bool {
        for (auto packIt = resourcePacks.rbegin(); packIt != resourcePacks.rend(); ++packIt) {
            const auto& pack = *packIt;
            if (pack == nullptr) {
                continue;
            }

            for (const auto& sourceLoc : sourceCandidates) {
                std::vector<u8> pixels;
                u32 width = 0;
                u32 height = 0;
                u32 frameWidth = 0;
                u32 frameHeight = 0;
                const auto loadResult =
                    loadTexturePixels(*pack, sourceLoc, pixels, width, height, frameWidth, frameHeight);
                if (loadResult.success()) {
                    constexpr u32 MAX_ICON_SIZE = 64;
                    if (width > MAX_ICON_SIZE || height > MAX_ICON_SIZE) {
                        const u32 dstWidth = std::min(width, MAX_ICON_SIZE);
                        const u32 dstHeight = std::min(height, MAX_ICON_SIZE);
                        pixels = resizeNearestRGBA(pixels, width, height, dstWidth, dstHeight);
                        width = dstWidth;
                        height = dstHeight;
                        frameWidth = std::min(frameWidth, dstWidth);
                        frameHeight = std::min(frameHeight, dstHeight);
                    }

                    builder.addTextureFrame(atlasKey, pixels, width, height, frameWidth, frameHeight);
                    return true;
                }
            }
        }

        return false;
    };

    // 遍历所有物品并尝试加载纹理。
    // 规则：优先 textures/item/<item>，若是方块物品再回退到 block 纹理。
    // 使用 TexturePathVariant::getAltTexturePath() 集中化路径变体转换，消除硬编码回退逻辑。
    ItemRegistry::instance().forEachItem([&](Item& item) {
        const ResourceLocation& itemId = item.itemLocation();
        const ResourceLocation atlasKey(itemId.namespace_(), "textures/item/" + itemId.path());

        // 构建候选路径列表：现代路径 + getAltTexturePath() 自动计算的旧版路径变体
        std::vector<ResourceLocation> sourceCandidates;
        sourceCandidates.push_back(atlasKey);

        std::string altItemPath = resource::atlas::TexturePathVariant::getAltTexturePath(atlasKey.path());
        if (!altItemPath.empty()) {
            sourceCandidates.emplace_back(itemId.namespace_(), std::move(altItemPath));
        }

        const BlockItem* blockItem = dynamic_cast<const BlockItem*>(&item);
        if (blockItem != nullptr) {
            const ResourceLocation& blockId = blockItem->block().blockLocation();
            ResourceLocation blockLoc(blockId.namespace_(), "textures/block/" + blockId.path());
            sourceCandidates.push_back(blockLoc);

            std::string altBlockPath = resource::atlas::TexturePathVariant::getAltTexturePath(blockLoc.path());
            if (!altBlockPath.empty()) {
                sourceCandidates.emplace_back(blockId.namespace_(), std::move(altBlockPath));
            }
        }

        if (tryLoadToBuilder(atlasKey, sourceCandidates)) {
            // 纹理加载成功
        }
    });

    // Build atlas
    auto atlasResult = builder.build();
    if (!atlasResult.success()) {
        spdlog::warn("ItemTextureAtlas: Failed to build atlas: {}", atlasResult.error().message());
        return atlasResult.error();
    }

    const auto& atlas = atlasResult.value();
    if (atlas.pixels.empty()) {
        spdlog::info("ItemTextureAtlas: No item textures loaded");
        m_pixels.clear();
        return {};
    }

    // Update atlas size
    m_width = atlas.width;
    m_height = atlas.height;
    // atlas.pixels 是普通 std::vector<u8>（TextureAtlasBuilder 产物），与 m_pixels 的
    // 追踪分配器类型不同，无法直接赋值；用迭代器范围 assign 拷贝（分配由 m_pixels 的
    // 追踪分配器经 Tracy 截获）
    m_pixels.assign(atlas.pixels.begin(), atlas.pixels.end());

    // Store texture regions
    for (const auto& pair : atlas.regions) {
        m_regionsByLocation[pair.first] = pair.second;
    }

    // Map item ID to texture region
    ItemRegistry::instance().forEachItem([this, &atlas](Item& item) {
        const ResourceLocation& itemId = item.itemLocation();
        const ResourceLocation atlasKey(itemId.namespace_(), "textures/item/" + itemId.path());
        auto it = atlas.regions.find(atlasKey);
        if (it == atlas.regions.end()) {
            return;
        }

        const TextureRegion& region = it->second;
        m_regionsByItemId[item.itemId()] = region;

        m_regionsByLocation[atlasKey] = region;
        m_regionsByLocation[ResourceLocation(itemId.namespace_(), "item/" + itemId.path())] = region;

        // 使用 getAltTexturePath() 自动注册路径变体别名（如 textures/items/ 旧版路径）
        std::string altItemPath = resource::atlas::TexturePathVariant::getAltTexturePath(atlasKey.path());
        if (!altItemPath.empty()) {
            m_regionsByLocation[ResourceLocation(itemId.namespace_(), std::move(altItemPath))] = region;
        }

        const BlockItem* blockItem = dynamic_cast<const BlockItem*>(&item);
        if (blockItem != nullptr) {
            const ResourceLocation& blockId = blockItem->block().blockLocation();
            m_regionsByLocation[ResourceLocation(blockId.namespace_(), "block/" + blockId.path())] = region;

            ResourceLocation blockTextureLoc(blockId.namespace_(), "textures/block/" + blockId.path());
            m_regionsByLocation[blockTextureLoc] = region;

            std::string altBlockPath = resource::atlas::TexturePathVariant::getAltTexturePath(blockTextureLoc.path());
            if (!altBlockPath.empty()) {
                m_regionsByLocation[ResourceLocation(blockId.namespace_(), std::move(altBlockPath))] = region;
            }
        }
    });

    spdlog::info("ItemTextureAtlas: Loaded {} textures mapped to {} items ({}x{})",
        atlas.regions.size(),
        m_regionsByItemId.size(),
        m_width,
        m_height);

    return {};
}

Result<void> ItemTextureAtlas::upload()
{
    if (m_pixels.empty()) {
        return {};
    }

    const bool needRecreateImage =
        (m_image == VK_NULL_HANDLE) || (m_imageWidth != m_width) || (m_imageHeight != m_height);

    if (needRecreateImage) {
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

        auto imageResult = _createImage();
        if (!imageResult.success()) {
            return imageResult.error();
        }

        auto viewResult = _createImageView();
        if (!viewResult.success()) {
            vkDestroyImage(m_device, m_image, nullptr);
            vkFreeMemory(m_device, m_imageMemory, nullptr);
            m_image = VK_NULL_HANDLE;
            m_imageMemory = VK_NULL_HANDLE;
            return viewResult.error();
        }
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_pixels.size());

    // Create staging buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create staging buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
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

    // Map and copy data
    void* mappedData = nullptr;
    if (vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging memory");
    }
    std::memcpy(mappedData, m_pixels.data(), m_pixels.size());
    vkUnmapMemory(m_device, stagingMemory);

    const VkImageLayout uploadOldLayout =
        (needRecreateImage || !m_uploaded) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const VkPipelineStageFlags uploadSrcStage =
        (needRecreateImage || !m_uploaded) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    // Use single-time command to upload texture
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    // Transition image layout to transfer destination
    _transitionImageLayout(
        cmd, uploadOldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uploadSrcStage, VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Copy buffer to image
    VkBufferImageCopy region = {};
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

    // Transition to shader read-only layout
    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    // Cleanup staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    m_uploaded = true;

    // Clear pixel data (uploaded to GPU)
    m_pixels.clear();
    m_pixels.shrink_to_fit();

    return {};
}

const TextureRegion* ItemTextureAtlas::getItemTexture(u32 itemId) const
{
    auto it = m_regionsByItemId.find(itemId);
    return it != m_regionsByItemId.end() ? &it->second : nullptr;
}

const TextureRegion* ItemTextureAtlas::getItemTexture(const ResourceLocation& location) const
{
    auto it = m_regionsByLocation.find(location);
    return it != m_regionsByLocation.end() ? &it->second : nullptr;
}

std::vector<TextureRegion> ItemTextureAtlas::getItemTextureLayers(
    const std::vector<ResourceLocation>& textureLocations) const
{
    std::vector<TextureRegion> layers;
    layers.reserve(textureLocations.size());

    for (const auto& loc : textureLocations) {
        const TextureRegion* region = getItemTexture(loc);
        if (region != nullptr) {
            layers.push_back(*region);
        }
    }

    return layers;
}

bool ItemTextureAtlas::hasItemTexture(u32 itemId) const
{
    return m_regionsByItemId.find(itemId) != m_regionsByItemId.end();
}

void ItemTextureAtlas::addTextureRegion(u32 itemId, const TextureRegion& region)
{
    m_regionsByItemId[itemId] = region;
}

void ItemTextureAtlas::addTextureRegion(const ResourceLocation& location, const TextureRegion& region)
{
    m_regionsByLocation[location] = region;
}

Result<void> ItemTextureAtlas::_createImage()
{
    // Create image
    VkImageCreateInfo imageInfo = {};
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
        return Error(ErrorCode::OutOfMemory, "Failed to create item texture atlas image");
    }

    // Allocate memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
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
        return Error(ErrorCode::OutOfMemory, "Failed to allocate item texture atlas memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
    m_imageWidth = m_width;
    m_imageHeight = m_height;

    return {};
}

Result<void> ItemTextureAtlas::_createSampler()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create item texture atlas sampler");
    }

    return {};
}

Result<void> ItemTextureAtlas::_createImageView()
{
    VkImageViewCreateInfo viewInfo = {};
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
        return Error(ErrorCode::InitializationFailed, "Failed to create item texture atlas image view");
    }

    return {};
}

Result<u32> ItemTextureAtlas::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer ItemTextureAtlas::_beginSingleTimeCommands()
{
    return renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void ItemTextureAtlas::_endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
}

void ItemTextureAtlas::_transitionImageLayout(VkCommandBuffer cmd,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage)
{
    renderer::VulkanUtils::transitionImageLayout(cmd, m_image, oldLayout, newLayout, srcStage, dstStage);
}

Result<void> ItemTextureAtlas::uploadRegion(
    const void* pixelData, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 rowLength)
{
    if (pixelData == nullptr) {
        return Error(ErrorCode::NullPointer, "Item atlas region pixel data is null");
    }

    if (m_image == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Item atlas not initialized");
    }

    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "Item atlas region dimensions must be non-zero");
    }

    // 创建暂存缓冲区
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    auto result = renderer::VulkanUtils::createBuffer(m_device,
        m_physicalDevice,
        static_cast<VkDeviceSize>(size),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    if (result.failed()) {
        return result;
    }

    // 映射并复制数据
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, stagingMemory, 0, static_cast<VkDeviceSize>(size), 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer memory for item atlas region upload");
    }
    std::memcpy(mapped, pixelData, static_cast<size_t>(size));
    vkUnmapMemory(m_device, stagingMemory);

    // 开始命令缓冲区
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    // 转换到传输目标布局
    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像子区域
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = rowLength > 0 ? rowLength : width;
    region.bufferImageHeight = height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY), 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换回着色器只读布局
    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

} // namespace mc::client
