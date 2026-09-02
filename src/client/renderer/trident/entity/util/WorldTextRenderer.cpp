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

#include "WorldTextRenderer.hpp"
#include "../pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool WorldTextRenderer::s_initialized = false;
Font* WorldTextRenderer::s_font = nullptr;
f32 WorldTextRenderer::s_maxDistance = DEFAULT_MAX_DISTANCE;
f32 WorldTextRenderer::s_scale = DEFAULT_SCALE;
bool WorldTextRenderer::s_showBackground = true;
u8 WorldTextRenderer::s_bgColorR = 0;
u8 WorldTextRenderer::s_bgColorG = 0;
u8 WorldTextRenderer::s_bgColorB = 0;
u8 WorldTextRenderer::s_bgColorA = 128;
Vector3d WorldTextRenderer::s_cameraPosition(0.0, 0.0, 0.0);
std::array<f64, 16> WorldTextRenderer::s_viewMatrix = {
    1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
mc::math::frustum::Frustum WorldTextRenderer::s_frustum;
Vector3f WorldTextRenderer::s_cameraForward(0.0f, 0.0f, -1.0f);
std::unordered_map<u32, WorldGlyphMesh> WorldTextRenderer::s_glyphMeshCache;
pipeline::EntityMesh* WorldTextRenderer::s_backgroundMesh = nullptr;

// Vulkan 资源
VkDevice WorldTextRenderer::s_device = VK_NULL_HANDLE;
VkPhysicalDevice WorldTextRenderer::s_physicalDevice = VK_NULL_HANDLE;
VkCommandPool WorldTextRenderer::s_commandPool = VK_NULL_HANDLE;
VkQueue WorldTextRenderer::s_graphicsQueue = VK_NULL_HANDLE;
VkImage WorldTextRenderer::s_fontTexture = VK_NULL_HANDLE;
VkDeviceMemory WorldTextRenderer::s_fontTextureMemory = VK_NULL_HANDLE;
VkImageView WorldTextRenderer::s_fontTextureView = VK_NULL_HANDLE;
VkSampler WorldTextRenderer::s_fontSampler = VK_NULL_HANDLE;

bool WorldTextRenderer::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    pipeline::EntityPipeline& pipeline,
    Font* font)
{
    if (s_initialized) {
        return true;
    }

    if (font == nullptr) {
        spdlog::error("WorldTextRenderer: Font is null");
        return false;
    }

    if (device == VK_NULL_HANDLE || commandPool == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
        spdlog::error("WorldTextRenderer: Invalid Vulkan resources");
        return false;
    }

    s_font = font;
    s_device = device;
    s_physicalDevice = physicalDevice;
    s_commandPool = commandPool;
    s_graphicsQueue = graphicsQueue;

    // 预缓存常用 ASCII 字符
    // 从 FontTextureAtlas 获取字形并创建世界空间网格
    for (u32 c = 32; c <= 126; ++c) { // 可打印 ASCII 字符
        const Glyph* glyph = font->getGlyph(c);
        if (glyph != nullptr) {
            WorldGlyphMesh mesh = createGlyphMeshFromGlyph(*glyph);
            s_glyphMeshCache[c] = std::move(mesh);
        }
    }

    // 创建字体纹理
    if (!createFontTexture()) {
        spdlog::error("WorldTextRenderer: Failed to create font texture");
        return false;
    }

    // 创建背景网格
    createBackgroundMesh(1.0f, 1.0f);

    s_initialized = true;
    spdlog::info(
        "WorldTextRenderer: Initialized successfully with {} cached glyphs and font texture", s_glyphMeshCache.size());

    (void)pipeline; // 管线用于后续绘制
    return true;
}

WorldGlyphMesh WorldTextRenderer::createGlyphMeshFromGlyph(const Glyph& glyph)
{
    WorldGlyphMesh mesh;

    // 创建字符四边形（billboard 格式，面向 +Z）
    // 字形原点在左上角
    f32 x0 = glyph.bearingX;
    f32 y0 = -glyph.bearingY; // 从基线向上
    f32 x1 = x0 + glyph.width;
    f32 y1 = y0 + glyph.height;

    // UV 坐标从 Glyph 获取
    f32 u0 = glyph.u0;
    f32 v0 = glyph.v0;
    f32 u1 = glyph.u1;
    f32 v1 = glyph.v1;

    // 创建顶点（位置 xyz, 纹理 uv, 法线 xyz）
    // 使用 ModelVertex 格式
    mesh.vertices = {
        // 第一个三角形
        model::ModelVertex(x0, y0, 0.0, u0, v0, 0.0, 0.0, 1.0),
        model::ModelVertex(x1, y0, 0.0, u1, v0, 0.0, 0.0, 1.0),
        model::ModelVertex(x1, y1, 0.0, u1, v1, 0.0, 0.0, 1.0),
        // 第二个三角形
        model::ModelVertex(x0, y0, 0.0, u0, v0, 0.0, 0.0, 1.0),
        model::ModelVertex(x1, y1, 0.0, u1, v1, 0.0, 0.0, 1.0),
        model::ModelVertex(x0, y1, 0.0, u0, v1, 0.0, 0.0, 1.0),
    };

    // 索引（两个三角形）
    mesh.indices = {0, 1, 2, 3, 4, 5};

    mesh.advanceX = glyph.advance;
    mesh.width = glyph.width;
    mesh.height = glyph.height;

    return mesh;
}

bool WorldTextRenderer::createFontTexture()
{
    if (s_font == nullptr || !s_font->isValid()) {
        spdlog::error("WorldTextRenderer: Font not valid");
        return false;
    }

    const u8* pixels = s_font->atlasPixels();
    u32 size = s_font->atlasSize();

    if (pixels == nullptr || size == 0) {
        spdlog::error("WorldTextRenderer: Font atlas has no pixel data");
        return false;
    }

    VkDevice device = s_device;

    // 创建图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = size;
    imageInfo.extent.height = size;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8_UNORM; // 字体图集是灰度图
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &imageInfo, nullptr, &s_fontTexture) != VK_SUCCESS) {
        spdlog::error("WorldTextRenderer: Failed to create font texture image");
        return false;
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, s_fontTexture, &memRequirements);

    auto memTypeResult = VulkanUtils::findMemoryType(
        s_physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memTypeResult.success()) {
        vkDestroyImage(device, s_fontTexture, nullptr);
        s_fontTexture = VK_NULL_HANDLE;
        spdlog::error("WorldTextRenderer: Failed to find memory type");
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(device, &allocInfo, nullptr, &s_fontTextureMemory) != VK_SUCCESS) {
        vkDestroyImage(device, s_fontTexture, nullptr);
        s_fontTexture = VK_NULL_HANDLE;
        spdlog::error("WorldTextRenderer: Failed to allocate font texture memory");
        return false;
    }

    vkBindImageMemory(device, s_fontTexture, s_fontTextureMemory, 0);

    // 上传数据
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(size) * size;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        spdlog::error("WorldTextRenderer: Failed to create staging buffer");
        return false;
    }

    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
    memTypeResult = VulkanUtils::findMemoryType(s_physicalDevice,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!memTypeResult.success()) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return false;
    }

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return false;
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    void* data = nullptr;
    const VkResult mapResult = vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingMemory);

    // 转换布局并复制
    VkCommandBuffer cmd = VulkanUtils::beginSingleTimeCommands(device, s_commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = s_fontTexture;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {size, size, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, s_fontTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

    VulkanUtils::endSingleTimeCommands(device, s_commandPool, s_graphicsQueue, cmd);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = s_fontTexture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &s_fontTextureView) != VK_SUCCESS) {
        spdlog::error("WorldTextRenderer: Failed to create font texture view");
        return false;
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST; // 像素风格字体
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &s_fontSampler) != VK_SUCCESS) {
        spdlog::error("WorldTextRenderer: Failed to create font sampler");
        return false;
    }

    spdlog::info("WorldTextRenderer: Font texture created ({}x{})", size, size);
    return true;
}

void WorldTextRenderer::cleanup()
{
    s_glyphMeshCache.clear();

    // 销毁背景网格
    s_backgroundMesh = nullptr; // 由 EntityPipeline 管理

    // 销毁 Vulkan 资源
    if (s_fontSampler != VK_NULL_HANDLE) {
        vkDestroySampler(s_device, s_fontSampler, nullptr);
        s_fontSampler = VK_NULL_HANDLE;
    }

    if (s_fontTextureView != VK_NULL_HANDLE) {
        vkDestroyImageView(s_device, s_fontTextureView, nullptr);
        s_fontTextureView = VK_NULL_HANDLE;
    }

    if (s_fontTexture != VK_NULL_HANDLE) {
        vkDestroyImage(s_device, s_fontTexture, nullptr);
        s_fontTexture = VK_NULL_HANDLE;
    }

    if (s_fontTextureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_device, s_fontTextureMemory, nullptr);
        s_fontTextureMemory = VK_NULL_HANDLE;
    }

    s_font = nullptr;
    s_initialized = false;
    spdlog::info("WorldTextRenderer: Cleaned up");
}

bool WorldTextRenderer::isInitialized()
{
    return s_initialized;
}

void WorldTextRenderer::setCameraPosition(const Vector3d& position)
{
    s_cameraPosition = position;
}

void WorldTextRenderer::setViewMatrix(const std::array<f64, 16>& viewMatrix)
{
    s_viewMatrix = viewMatrix;

    // 从视图矩阵提取相机前向向量（用于背面剔除）
    // 视图矩阵的第三列（Z轴）是相机的前向方向（在视图空间中指向相机前方）
    // 由于视图矩阵是相机的逆矩阵，我们取第三行的负值作为前向向量
    s_cameraForward =
        Vector3f(static_cast<f32>(-viewMatrix[8]), static_cast<f32>(-viewMatrix[9]), static_cast<f32>(-viewMatrix[10]));
    // 手动归一化
    f32 len = std::sqrt(s_cameraForward.x * s_cameraForward.x + s_cameraForward.y * s_cameraForward.y +
        s_cameraForward.z * s_cameraForward.z);
    if (len > 0.0001f) {
        s_cameraForward.x /= len;
        s_cameraForward.y /= len;
        s_cameraForward.z /= len;
    }
}

void WorldTextRenderer::setFrustum(const mc::math::frustum::Frustum& frustum)
{
    s_frustum = frustum;
}

void WorldTextRenderer::setCameraForward(const Vector3f& forward)
{
    // 手动归一化
    f32 len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len > 0.0001f) {
        s_cameraForward.x = forward.x / len;
        s_cameraForward.y = forward.y / len;
        s_cameraForward.z = forward.z / len;
    } else {
        s_cameraForward = forward;
    }
}

const WorldGlyphMesh* WorldTextRenderer::getGlyphMesh(u32 codepoint)
{
    auto it = s_glyphMeshCache.find(codepoint);
    if (it != s_glyphMeshCache.end()) {
        return &it->second;
    }

    // 如果字形不在缓存中，尝试从字体加载
    if (s_font != nullptr) {
        const Glyph* glyph = s_font->getGlyph(codepoint);
        if (glyph != nullptr) {
            WorldGlyphMesh mesh = createGlyphMeshFromGlyph(*glyph);
            s_glyphMeshCache[codepoint] = std::move(mesh);
            return &s_glyphMeshCache[codepoint];
        }
    }

    return nullptr;
}

void WorldTextRenderer::renderText(VkCommandBuffer cmd,
    const std::string& text,
    const Vector3f& position,
    f32 scale,
    const Vector4f& color,
    bool showBackground,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized || text.empty() || s_font == nullptr) {
        return;
    }

    // 计算到相机的距离
    Vector3 cameraPosF(static_cast<f32>(s_cameraPosition.x),
        static_cast<f32>(s_cameraPosition.y),
        static_cast<f32>(s_cameraPosition.z));
    Vector3 toCamera = cameraPosF - Vector3(position.x, position.y, position.z);
    f32 distance = toCamera.length();

    // 距离检查
    if (!shouldRenderText(position, distance)) {
        return;
    }

    // 计算缩放（距离越近越大）
    f32 effectiveScale = s_scale;
    if (distance < 10.0f) {
        effectiveScale *= 10.0f / distance;
    }
    effectiveScale *= scale;

    // 计算 billboard 矩阵
    std::array<f64, 16> billboardMatrix;
    computeBillboardMatrix(position, billboardMatrix);

    // 计算文本宽度
    f32 textWidth = calculateTextWidth(text, effectiveScale);
    f32 textHeight = CHAR_HEIGHT * effectiveScale;

    // 绑定字体纹理
    if (s_fontTextureView != VK_NULL_HANDLE) {
        pipeline.setTextureAtlas(s_fontTextureView, s_fontSampler);
    }

    // 渲染背景
    if (showBackground) {
        f32 bgWidth = textWidth + BACKGROUND_PADDING * 2.0f * effectiveScale;
        f32 bgHeight = textHeight + BACKGROUND_PADDING * effectiveScale;

        // 创建背景四边形顶点
        f32 halfWidth = bgWidth * 0.5f;
        f32 halfHeight = bgHeight * 0.5f;

        // 背景位于文本基线下方一点
        f32 bgY = -halfHeight * 0.5f;

        std::vector<model::ModelVertex> bgVertices = {
            // 第一个三角形
            model::ModelVertex(-halfWidth, bgY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
            model::ModelVertex(halfWidth, bgY, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
            model::ModelVertex(halfWidth, bgY + bgHeight, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
            // 第二个三角形
            model::ModelVertex(-halfWidth, bgY, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
            model::ModelVertex(halfWidth, bgY + bgHeight, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
            model::ModelVertex(-halfWidth, bgY + bgHeight, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f),
        };
        std::vector<u32> bgIndices = {0, 1, 2, 3, 4, 5};

        auto bgMeshResult = pipeline.createMesh(bgVertices, bgIndices);
        if (bgMeshResult.success()) {
            auto& bgMesh = bgMeshResult.value();
            Vector3f bgPos(0, 0, 0);
            // 使用背景颜色渲染（半透明）
            Vector4f bgColor(static_cast<f32>(s_bgColorR) / 255.0f,
                static_cast<f32>(s_bgColorG) / 255.0f,
                static_cast<f32>(s_bgColorB) / 255.0f,
                static_cast<f32>(s_bgColorA) / 255.0f);
            pipeline.drawMesh(cmd, bgMesh, billboardMatrix, bgPos, 1.0f, bgColor, 0.0f, 0.0f);
            pipeline.destroyMesh(bgMesh);
        }
    }

    // 渲染文本字符
    f32 cursorX = -textWidth * 0.5f; // 居中起始位置

    // 绑定管线
    pipeline.bind(cmd);
    pipeline.bindTextureDescriptor(cmd);

    // UTF-8 解码
    size_t pos = 0;
    while (pos < text.size()) {
        u32 codepoint = decodeCodepoint(text, pos);
        if (codepoint == 0) break;

        const WorldGlyphMesh* glyph = getGlyphMesh(codepoint);
        if (glyph == nullptr) {
            cursorX += DEFAULT_CHAR_WIDTH * effectiveScale; // 使用默认宽度
            continue;
        }

        // 创建偏移后的顶点数据
        std::vector<model::ModelVertex> vertices = glyph->vertices;
        for (auto& vertex : vertices) {
            // 应用光标偏移和缩放
            vertex.position.x = (vertex.position.x * effectiveScale) + cursorX;
            vertex.position.y *= effectiveScale;
            vertex.position.z *= effectiveScale;
        }

        // 创建临时网格并绘制
        auto meshResult = pipeline.createMesh(vertices, glyph->indices);
        if (meshResult.success()) {
            auto& mesh = meshResult.value();

            // 使用 billboard 矩阵绘制
            Vector3f meshPos(0, 0, 0); // 位置已在矩阵中
            pipeline.drawMesh(cmd, mesh, billboardMatrix, meshPos, 1.0, color, 0.0f, 0.0f);

            // 销毁临时网格
            pipeline.destroyMesh(mesh);
        }

        cursorX += glyph->advanceX * effectiveScale;
    }
}

void WorldTextRenderer::renderNameTag(VkCommandBuffer cmd,
    const std::string& name,
    const Vector3f& entityPosition,
    f32 entityHeight,
    pipeline::EntityPipeline& pipeline)
{
    // 计算名称标签位置（实体头顶上方）
    Vector3f tagPos = entityPosition;
    tagPos.y += entityHeight + HEIGHT_OFFSET;

    // 渲染文本
    renderText(cmd,
        name,
        tagPos,
        1.0f,                             // 使用默认缩放
        Vector4f(1.0f, 1.0f, 1.0f, 1.0f), // 白色文本
        s_showBackground,
        pipeline);
}

void WorldTextRenderer::setMaxDistance(f32 distance)
{
    s_maxDistance = distance;
}

f32 WorldTextRenderer::maxDistance()
{
    return s_maxDistance;
}

void WorldTextRenderer::setBackgroundColor(u8 r, u8 g, u8 b, u8 a)
{
    s_bgColorR = r;
    s_bgColorG = g;
    s_bgColorB = b;
    s_bgColorA = a;
}

void WorldTextRenderer::setShowBackground(bool show)
{
    s_showBackground = show;
}

void WorldTextRenderer::createBackgroundMesh(f32 width, f32 height)
{
    // 创建背景四边形
    f32 halfWidth = width * 0.5f;
    f32 halfHeight = height * 0.5f;

    // 背景网格在渲染时动态创建，这里只存储尺寸
    // 实际背景渲染在 renderText() 中完成

    (void)halfWidth;
    (void)halfHeight;
}

void WorldTextRenderer::computeBillboardMatrix(const Vector3f& position, std::array<f64, 16>& outMatrix)
{
    // 从视图矩阵中提取旋转部分，然后反转

    // 提取视图矩阵的上方向和右方向
    f64 view00 = s_viewMatrix[0];
    f64 view01 = s_viewMatrix[1];
    f64 view02 = s_viewMatrix[2];
    f64 view10 = s_viewMatrix[4];
    f64 view11 = s_viewMatrix[5];
    f64 view12 = s_viewMatrix[6];
    f64 view20 = s_viewMatrix[8];
    f64 view21 = s_viewMatrix[9];
    f64 view22 = s_viewMatrix[10];

    // billboard 矩阵是视图矩阵旋转部分的转置（即逆）
    outMatrix[0] = view00;
    outMatrix[1] = view10;
    outMatrix[2] = view20;
    outMatrix[3] = static_cast<f64>(position.x);
    outMatrix[4] = view01;
    outMatrix[5] = view11;
    outMatrix[6] = view21;
    outMatrix[7] = static_cast<f64>(position.y);
    outMatrix[8] = view02;
    outMatrix[9] = view12;
    outMatrix[10] = view22;
    outMatrix[11] = static_cast<f64>(position.z);
    outMatrix[12] = 0.0;
    outMatrix[13] = 0.0;
    outMatrix[14] = 0.0;
    outMatrix[15] = 1.0;
}

f32 WorldTextRenderer::calculateTextWidth(const std::string& text, f32 scale)
{
    if (s_font == nullptr) {
        return static_cast<f32>(text.size()) * DEFAULT_CHAR_WIDTH * scale;
    }

    f32 width = 0.0f;
    size_t pos = 0;
    while (pos < text.size()) {
        u32 codepoint = decodeCodepoint(text, pos);
        if (codepoint == 0) break;

        const WorldGlyphMesh* glyph = getGlyphMesh(codepoint);
        if (glyph != nullptr) {
            width += glyph->advanceX * scale;
        } else {
            width += DEFAULT_CHAR_WIDTH * scale;
        }
    }
    return width;
}

bool WorldTextRenderer::shouldRenderText(const Vector3f& position, f32 distance)
{
    // 距离检查
    if (distance > s_maxDistance) {
        return false;
    }

    // 视锥体剔除
    // 使用球体测试，名称标签大小约 1-2 格，使用半径 2.0f 进行保守测试
    if (s_frustum.isValid()) {
        mc::Vector3 frustumPos(position.x, position.y, position.z);
        if (!s_frustum.isSphereVisible(frustumPos, 2.0f)) {
            return false;
        }
    }

    // 背面剔除：检查玩家是否背对文本位置
    if (isBackFacing(position)) {
        return false;
    }

    return true;
}

bool WorldTextRenderer::isBackFacing(const Vector3f& textPosition)
{
    // 计算从文本位置指向相机的方向向量
    Vector3f toCamera(static_cast<f32>(s_cameraPosition.x - textPosition.x),
        static_cast<f32>(s_cameraPosition.y - textPosition.y),
        static_cast<f32>(s_cameraPosition.z - textPosition.z));

    f32 distanceSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z;

    // 如果相机非常接近文本位置（距离接近0），不进行背面剔除
    if (distanceSq < 0.0001f) {
        return false;
    }

    // 归一化方向向量
    f32 invDistance = 1.0f / std::sqrt(distanceSq);
    toCamera.x *= invDistance;
    toCamera.y *= invDistance;
    toCamera.z *= invDistance;

    // 计算相机前向向量与"到相机方向"的点积
    // 如果点积 >= 0，表示相机背对文本位置（文本在相机后方）
    f32 dot = toCamera.x * s_cameraForward.x + toCamera.y * s_cameraForward.y + toCamera.z * s_cameraForward.z;

    // 点积 >= 0 表示文本在相机背后（toCamera 与 cameraForward 方向相同）
    return dot >= 0.0f;
}

u32 WorldTextRenderer::decodeCodepoint(const std::string& text, size_t& pos)
{
    if (pos >= text.size()) {
        return 0;
    }

    u8 c = static_cast<u8>(text[pos]);
    pos++;

    // UTF-8 解码
    if ((c & 0x80) == 0) {
        // 单字节 ASCII
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        // 双字节
        if (pos >= text.size()) return c;
        u8 c2 = static_cast<u8>(text[pos++]);
        return (static_cast<u32>(c & 0x1F) << 6) | static_cast<u32>(c2 & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        // 三字节
        if (pos + 1 >= text.size()) return c;
        u8 c2 = static_cast<u8>(text[pos++]);
        u8 c3 = static_cast<u8>(text[pos++]);
        return (static_cast<u32>(c & 0x0F) << 12) | (static_cast<u32>(c2 & 0x3F) << 6) | static_cast<u32>(c3 & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        // 四字节
        if (pos + 2 >= text.size()) return c;
        u8 c2 = static_cast<u8>(text[pos++]);
        u8 c3 = static_cast<u8>(text[pos++]);
        u8 c4 = static_cast<u8>(text[pos++]);
        return (static_cast<u32>(c & 0x07) << 18) | (static_cast<u32>(c2 & 0x3F) << 12) |
            (static_cast<u32>(c3 & 0x3F) << 6) | static_cast<u32>(c4 & 0x3F);
    }

    return c;
}

} // namespace mc::client::renderer::entity::util
