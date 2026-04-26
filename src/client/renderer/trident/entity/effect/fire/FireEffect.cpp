#include "FireEffect.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "common/resource/IResourcePack.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>

// STB image for texture loading
#define STBI_ONLY_PNG
#include <stb_image.h>

namespace mc::client::renderer::entity::effect::fire {

// 静态成员初始化
bool FireEffect::s_initialized = false;
VkDevice FireEffect::s_device = VK_NULL_HANDLE;
VkPhysicalDevice FireEffect::s_physicalDevice = VK_NULL_HANDLE;
VkCommandPool FireEffect::s_commandPool = VK_NULL_HANDLE;
VkQueue FireEffect::s_graphicsQueue = VK_NULL_HANDLE;
VkImage FireEffect::s_fireTexture = VK_NULL_HANDLE;
VkDeviceMemory FireEffect::s_fireTextureMemory = VK_NULL_HANDLE;
VkImageView FireEffect::s_fireTextureView = VK_NULL_HANDLE;
VkSampler FireEffect::s_fireSampler = VK_NULL_HANDLE;
u32 FireEffect::s_fireTextureWidth = 0;
u32 FireEffect::s_fireTextureHeight = 0;

// 火焰纹理路径（MC 1.16.5）
static const char* FIRE_TEXTURE_PATHS[] = {
    "textures/block/fire_0.png",
    "textures/block/fire_1.png"
};

bool FireEffect::initialize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const std::vector<IResourcePack*>& resourcePacks)
{
    if (s_initialized) {
        return true;
    }

    if (device == VK_NULL_HANDLE || commandPool == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
        spdlog::error("FireEffect: Invalid Vulkan resources");
        return false;
    }

    s_device = device;
    s_physicalDevice = physicalDevice;
    s_commandPool = commandPool;
    s_graphicsQueue = graphicsQueue;

    // 加载火焰纹理
    if (!loadFireTexture(resourcePacks)) {
        spdlog::warn("FireEffect: Failed to load fire texture, using placeholder");
        // 继续初始化，使用程序化生成的纹理
    }

    s_initialized = true;
    spdlog::info("FireEffect: Initialized successfully");
    return true;
}

void FireEffect::cleanup() {
    if (!s_initialized) {
        return;
    }

    // 销毁采样器
    if (s_fireSampler != VK_NULL_HANDLE) {
        vkDestroySampler(s_device, s_fireSampler, nullptr);
        s_fireSampler = VK_NULL_HANDLE;
    }

    // 销毁图像视图
    if (s_fireTextureView != VK_NULL_HANDLE) {
        vkDestroyImageView(s_device, s_fireTextureView, nullptr);
        s_fireTextureView = VK_NULL_HANDLE;
    }

    // 销毁图像
    if (s_fireTexture != VK_NULL_HANDLE) {
        vkDestroyImage(s_device, s_fireTexture, nullptr);
        s_fireTexture = VK_NULL_HANDLE;
    }

    // 释放内存
    if (s_fireTextureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_device, s_fireTextureMemory, nullptr);
        s_fireTextureMemory = VK_NULL_HANDLE;
    }

    s_device = VK_NULL_HANDLE;
    s_physicalDevice = VK_NULL_HANDLE;
    s_commandPool = VK_NULL_HANDLE;
    s_graphicsQueue = VK_NULL_HANDLE;
    s_initialized = false;
    spdlog::info("FireEffect: Cleaned up");
}

bool FireEffect::isInitialized() {
    return s_initialized;
}

bool FireEffect::isBurning(Entity& entity) {
    return entity.isOnFire();
}

bool FireEffect::isBurningClient(::mc::client::ClientEntity& entity) {
    return entity.isOnFire();
}

void FireEffect::renderFire(Entity& entity, f64 partialTicks) {
    if (!isBurning(entity)) {
        return;
    }

    // CPU 路径无法执行实际渲染
    (void)partialTicks;
}

void FireEffect::renderFire(
    VkCommandBuffer cmd,
    ::mc::client::ClientEntity& entity,
    f64 partialTicks,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized || !isBurningClient(entity)) {
        return;
    }

    // 参考 MC 1.16.5 EntityRendererManager.renderFire()
    // 获取实体尺寸
    f64 width = static_cast<f64>(entity.width());
    f64 height = static_cast<f64>(entity.height());
    Vector3 pos = entity.getInterpolatedPosition(static_cast<f32>(partialTicks));

    // 获取相机偏航角（简化：假设从实体朝向获取）
    f32 cameraYaw = static_cast<f32>(entity.yaw());

    // 调用多层火焰渲染
    renderFireLayers(cmd, entity, partialTicks, pipeline, cameraYaw);
}

void FireEffect::renderFireLayers(
    VkCommandBuffer cmd,
    ::mc::client::ClientEntity& entity,
    f64 partialTicks,
    pipeline::EntityPipeline& pipeline,
    f32 cameraYaw)
{
    // 参考 MC 1.16.5 EntityRendererManager.renderFire():654-696
    // 核心逻辑：
    // 1. 计算火焰尺寸 = width * 1.4
    // 2. 计算高度迭代次数 = height / fireSize
    // 3. 循环绘制多层火焰，每层：
    //    - 交替使用 fire_0 和 fire_1 纹理
    //    - 每两层翻转 UV
    //    - 尺寸递减 (f1 *= 0.9)
    //    - 高度递减 (f3 -= 0.45)
    //    - Z 偏移递增 (f5 += 0.03)

    f64 fireSize = static_cast<f64>(entity.width()) * 1.4;
    f64 height = static_cast<f64>(entity.height());

    // 初始参数（参考 MC 1.16.5）
    f64 f1 = 0.5;           // 火焰半宽
    f64 f3 = height / fireSize;  // 高度迭代次数
    f64 f4 = 0.0;           // Y 偏移累计
    f64 f5 = 0.0;           // Z 偏移

    // 绑定火焰纹理
    if (s_fireTextureView != VK_NULL_HANDLE && s_fireSampler != VK_NULL_HANDLE) {
        pipeline.setTextureAtlas(s_fireTextureView, s_fireSampler);
    }

    // 绑定管线
    pipeline.bind(cmd);
    pipeline.bindTextureDescriptor(cmd);

    // 计算实体位置
    Vector3 pos = entity.getInterpolatedPosition(static_cast<f32>(partialTicks));
    f64 x = pos.x;
    f64 y = pos.y;
    f64 z = pos.z;

    // 计算动画帧
    f64 time = static_cast<f64>(entity.ticksExisted()) + partialTicks;

    // 火焰颜色（MC 1.16.5: 使用白色，火焰纹理自带颜色）
    Vector4f fireColor(1.0f, 1.0f, 1.0f, 1.0f);
    Vector3f meshPos(0, 0, 0);

    // 计算 billboard 矩阵（面向相机）
    // MC 1.16.5: matrixStack.rotate(Vector3f.YP.rotationDegrees(-this.info.getYaw()));
    f32 yawRad = -mc::math::toRadians(cameraYaw);
    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);

    // 初始 Z 偏移（MC 1.16.5: -0.3F + (int)f3 * 0.02F）
    f64 zOffset = -0.3 + static_cast<f64>(static_cast<i32>(f3)) * 0.02;

    // 循环绘制多层火焰（MC 1.16.5: for(int i = 0; f3 > 0.0F; ++i)）
    for (i32 layer = 0; f3 > 0.0; ++layer) {
        // 交替使用 fire_0 和 fire_1 纹理
        u32 frameIndex = layer % 2;

        // UV 坐标
        f32 u0 = 0.0f;
        f32 u1 = 1.0f;
        f32 v0 = static_cast<f32>(frameIndex) * 0.5f;
        f32 v1 = static_cast<f32>(frameIndex + 1) * 0.5f;

        // 每两层翻转 UV（MC 1.16.5: if (i / 2 % 2 == 0) { float f10 = f8; f8 = f6; f6 = f10; }）
        if ((layer / 2) % 2 == 0) {
            std::swap(v0, v1);
        }

        // 当前层的高度
        f64 layerHeight = 1.4 - f4;

        // 生成火焰四边形（两个 billboard，互相垂直）
        std::vector<model::ModelVertex> vertices;
        std::vector<u32> indices;

        // 火焰宽度（MC 1.16.5: f1 是半宽）
        f64 currentHalfWidth = f1;

        // 第一个 billboard（面向相机）
        generateFireQuad(0, 0, 0, currentHalfWidth * 2.0, layerHeight, u0, v0, u1, v1, vertices, indices, 0);

        // 第二个 billboard（与第一个垂直，旋转 90 度）
        generateFireQuad(0, 0, 0, currentHalfWidth * 2.0, layerHeight, u0, v0, u1, v1, vertices, indices, 1);

        // 创建网格
        auto meshResult = pipeline.createMesh(vertices, indices);
        if (!meshResult.success()) {
            spdlog::trace("FireEffect: Failed to create fire mesh for layer {}", layer);
            continue;
        }

        auto& mesh = meshResult.value();

        // 计算变换矩阵
        // MC 1.16.5:
        // matrixStack.translate(0.0D, 0.0D, (double)(-0.3F + (float)((int)f3) * 0.02F));
        // 然后每层在 Y 方向有 f4 偏移

        // Billboard 矩阵（面向相机）
        std::array<f64, 16> billboardMatrix1 = {
            static_cast<f64>(cosYaw), 0.0, static_cast<f64>(-sinYaw), x,
            0.0, 1.0, 0.0, y - f4,  // Y 偏移累计
            static_cast<f64>(sinYaw), 0.0, static_cast<f64>(cosYaw), z + zOffset,
            0.0, 0.0, 0.0, 1.0
        };

        // 第二个 billboard（旋转 90 度）
        std::array<f64, 16> billboardMatrix2 = {
            static_cast<f64>(-sinYaw), 0.0, static_cast<f64>(-cosYaw), x,
            0.0, 1.0, 0.0, y - f4,
            static_cast<f64>(cosYaw), 0.0, static_cast<f64>(-sinYaw), z + zOffset,
            0.0, 0.0, 0.0, 1.0
        };

        // 绘制两个 billboard
        pipeline.drawMesh(cmd, mesh, billboardMatrix1, meshPos, static_cast<f32>(fireSize), fireColor, 0.0f, 0.0f);
        pipeline.drawMesh(cmd, mesh, billboardMatrix2, meshPos, static_cast<f32>(fireSize), fireColor, 0.0f, 0.0f);

        // 销毁临时网格
        pipeline.destroyMesh(mesh);

        // 递减参数（MC 1.16.5）
        f3 -= 0.45;
        f4 += 0.45;
        f1 *= 0.9;
        f5 += 0.03;
    }

    spdlog::trace("FireEffect: Rendered fire at ({}, {}, {}) with {} layers", x, y, z, static_cast<i32>(height / fireSize / 0.45));
}

bool FireEffect::loadFireTexture(const std::vector<IResourcePack*>& resourcePacks) {
    // 尝试从资源包加载火焰纹理
    std::vector<u8> combinedPixels;
    u32 frameWidth = 0;
    u32 frameHeight = 0;

    for (const char* path : FIRE_TEXTURE_PATHS) {
        for (auto* pack : resourcePacks) {
            if (!pack) continue;

            auto result = pack->readResource(path);
            if (result.success() && !result.value().empty()) {
                // 解码 PNG
                int width, height, channels;
                u8* pixels = stbi_load_from_memory(
                    result.value().data(),
                    static_cast<int>(result.value().size()),
                    &width, &height, &channels, 4);

                if (pixels != nullptr) {
                    if (frameWidth == 0) {
                        frameWidth = static_cast<u32>(width);
                        frameHeight = static_cast<u32>(height);
                        combinedPixels.resize(frameWidth * frameHeight * FIRE_FRAME_COUNT * 4);
                    }

                    // 复制像素数据到组合纹理
                    size_t destOffset = combinedPixels.empty() ? 0 : (combinedPixels.size() / FIRE_FRAME_COUNT) * (&path - FIRE_TEXTURE_PATHS);
                    std::memcpy(combinedPixels.data() + destOffset, pixels, width * height * 4);
                    stbi_image_free(pixels);

                    spdlog::info("FireEffect: Loaded fire texture {} ({}x{})", path, width, height);
                    break;
                }
            }
        }
    }

    // 如果没有找到纹理文件，创建程序化纹理
    if (combinedPixels.empty()) {
        spdlog::info("FireEffect: Creating procedural fire texture");
        frameWidth = 16;
        frameHeight = 16;
        combinedPixels.resize(frameWidth * frameHeight * FIRE_FRAME_COUNT * 4);

        // 生成简单的火焰图案
        for (u32 frame = 0; frame < FIRE_FRAME_COUNT; ++frame) {
            for (u32 y = 0; y < frameHeight; ++y) {
                for (u32 x = 0; x < frameWidth; ++x) {
                    size_t idx = (frame * frameWidth * frameHeight + y * frameWidth + x) * 4;

                    // 火焰颜色渐变（底部红色到顶部黄色）
                    f32 t = static_cast<f32>(y) / static_cast<f32>(frameHeight);
                    f32 intensity = 1.0f - t;  // 底部更亮

                    // 添加一些随机变化
                    f32 variation = static_cast<f32>((x + frame * 8) % 16) / 16.0f * 0.3f;
                    intensity = std::min(1.0f, intensity + variation);

                    // 设置 RGBA
                    combinedPixels[idx + 0] = static_cast<u8>(255 * intensity);                           // R
                    combinedPixels[idx + 1] = static_cast<u8>(128 * intensity * (1.0f - t * 0.5f));       // G
                    combinedPixels[idx + 2] = static_cast<u8>(32 * intensity);                            // B
                    combinedPixels[idx + 3] = static_cast<u8>(255 * intensity);                           // A
                }
            }
        }
    }

    return createFireTexture(combinedPixels, frameWidth, frameHeight * FIRE_FRAME_COUNT);
}

bool FireEffect::createFireTexture(const std::vector<u8>& pixels, u32 width, u32 height) {
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

    if (vkCreateImage(s_device, &imageInfo, nullptr, &s_fireTexture) != VK_SUCCESS) {
        spdlog::error("FireEffect: Failed to create fire texture image");
        return false;
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(s_device, s_fireTexture, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(s_physicalDevice, &memProps);

    u32 memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        spdlog::error("FireEffect: Failed to find suitable memory type");
        vkDestroyImage(s_device, s_fireTexture, nullptr);
        s_fireTexture = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(s_device, &allocInfo, nullptr, &s_fireTextureMemory) != VK_SUCCESS) {
        spdlog::error("FireEffect: Failed to allocate fire texture memory");
        vkDestroyImage(s_device, s_fireTexture, nullptr);
        s_fireTexture = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(s_device, s_fireTexture, s_fireTextureMemory, 0);

    // 上传数据
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(pixels.size());

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(s_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        spdlog::error("FireEffect: Failed to create staging buffer");
        return false;
    }

    vkGetBufferMemoryRequirements(s_device, stagingBuffer, &memRequirements);

    memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memoryTypeIndex = i;
            break;
        }
    }

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(s_device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(s_device, stagingBuffer, nullptr);
        return false;
    }

    vkBindBufferMemory(s_device, stagingBuffer, stagingMemory, 0);

    void* data;
    vkMapMemory(s_device, stagingMemory, 0, imageSize, 0, &data);
    std::memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(s_device, stagingMemory);

    // 转换布局并复制
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = s_commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(s_device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = s_fireTexture;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, s_fireTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(s_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(s_graphicsQueue);

    vkFreeCommandBuffers(s_device, s_commandPool, 1, &cmd);
    vkDestroyBuffer(s_device, stagingBuffer, nullptr);
    vkFreeMemory(s_device, stagingMemory, nullptr);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = s_fireTexture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(s_device, &viewInfo, nullptr, &s_fireTextureView) != VK_SUCCESS) {
        spdlog::error("FireEffect: Failed to create fire texture view");
        return false;
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo{};
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

    if (vkCreateSampler(s_device, &samplerInfo, nullptr, &s_fireSampler) != VK_SUCCESS) {
        spdlog::error("FireEffect: Failed to create fire sampler");
        return false;
    }

    s_fireTextureWidth = width;
    s_fireTextureHeight = height;

    spdlog::info("FireEffect: Fire texture created ({}x{})", width, height);
    return true;
}

void FireEffect::generateFireQuad(
    f64 x, f64 y, f64 z,
    f64 width, f64 height,
    f32 u0, f32 v0, f32 u1, f32 v1,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    u32 transformIndex)
{
    u32 baseIndex = static_cast<u32>(vertices.size());

    // 四个顶点（火焰四边形）
    // 火焰从底部向上燃烧
    vertices.emplace_back(model::ModelVertex(
        static_cast<f32>(x - width / 2), static_cast<f32>(y), static_cast<f32>(z),
        u0, v0,
        0.0f, 0.0f, 1.0f
    ));
    vertices.emplace_back(model::ModelVertex(
        static_cast<f32>(x - width / 2), static_cast<f32>(y + height), static_cast<f32>(z),
        u0, v1,
        0.0f, 0.0f, 1.0f
    ));
    vertices.emplace_back(model::ModelVertex(
        static_cast<f32>(x + width / 2), static_cast<f32>(y + height), static_cast<f32>(z),
        u1, v1,
        0.0f, 0.0f, 1.0f
    ));
    vertices.emplace_back(model::ModelVertex(
        static_cast<f32>(x + width / 2), static_cast<f32>(y), static_cast<f32>(z),
        u1, v0,
        0.0f, 0.0f, 1.0f
    ));

    // 两个三角形
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    (void)transformIndex;  // billboard 变换在绘制时应用
}

f64 FireEffect::computeFireOffset(f64 time, f64 seed) {
    // 计算火焰动画偏移
    // 使用正弦波创建火焰摇曳效果
    return std::sin(time * 0.3 + seed) * 0.1;
}

void FireEffect::computeBillboardMatrices(
    const Vector3f& position,
    std::array<std::array<f64, 16>, 2>& outMatrices)
{
    // 第一个 billboard：面向相机（假设相机在 Z 轴正方向）
    // 这将在渲染时使用实际的视图矩阵
    outMatrices[0] = {
        1.0, 0.0, 0.0, static_cast<f64>(position.x),
        0.0, 1.0, 0.0, static_cast<f64>(position.y),
        0.0, 0.0, 1.0, static_cast<f64>(position.z),
        0.0, 0.0, 0.0, 1.0
    };

    // 第二个 billboard：与第一个垂直（旋转 90 度）
    outMatrices[1] = {
        0.0, 0.0, 1.0, static_cast<f64>(position.x),
        0.0, 1.0, 0.0, static_cast<f64>(position.y),
        -1.0, 0.0, 0.0, static_cast<f64>(position.z),
        0.0, 0.0, 0.0, 1.0
    };
}

} // namespace mc::client::renderer::entity::effect::fire
