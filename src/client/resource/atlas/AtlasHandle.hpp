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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <vulkan/vulkan.h>

namespace mc::client::resource::atlas {

/**
 * @brief 单图集 Vulkan 资源管理
 *
 * 从 ItemTextureAtlas/EntityTextureAtlas 等旧图集类中抽取的 Vulkan 资源管理，
 * 消除 4+ 份重复的 _createImage/_createSampler/_createImageView/uploadRegion 代码。
 *
 * 与旧类的区别：采样模式（NEAREST/LINEAR）作为 create() 参数，按 atlasId 查配置表
 * （blocks/items=NEAREST，particles/gui=LINEAR）。
 *
 * 由 AtlasManager 持有，每个具名图集一个 AtlasHandle。
 */
class AtlasHandle {
public:
    AtlasHandle() = default;
    ~AtlasHandle();

    AtlasHandle(const AtlasHandle&) = delete;
    AtlasHandle& operator=(const AtlasHandle&) = delete;

    AtlasHandle(AtlasHandle&& other) noexcept;
    AtlasHandle& operator=(AtlasHandle&& other) noexcept;

    /**
     * @brief 创建图集 Vulkan 资源
     *
     * @param device Vulkan 设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池（用于纹理上传）
     * @param graphicsQueue 图形队列（用于纹理上传）
     * @param width 图集宽度
     * @param height 图集高度
     * @param filter 采样过滤模式（NEAREST 用于 blocks/items，LINEAR 用于 particles/gui）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 width,
        u32 height,
        VkFilter filter);

    /// 销毁资源
    void destroy();

    /**
     * @brief 上传整张图集像素到 GPU
     *
     * 当像素尺寸与已分配 GPU 图像尺寸不一致时自动重建图像。
     *
     * @param pixels RGBA8 像素数据
     * @param size 字节数
     * @param width 像素宽度
     * @param height 像素高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> upload(const void* pixels, u64 size, u32 width, u32 height);

    /**
     * @brief 上传图集子区域像素（动画帧更新用）
     *
     * @param pixelData RGBA8 像素数据
     * @param size 字节数
     * @param offsetX 目标区域在图集中的 X 偏移（像素）
     * @param offsetY 目标区域在图集中的 Y 偏移（像素）
     * @param width 区域宽度（像素）
     * @param height 区域高度（像素）
     * @param rowLength 源数据行长度（像素），0 表示使用 width
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> uploadRegion(
        const void* pixelData, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 rowLength);

    [[nodiscard]] VkImageView imageView() const noexcept { return m_imageView; }
    [[nodiscard]] VkSampler sampler() const noexcept { return m_sampler; }
    [[nodiscard]] u32 width() const noexcept { return m_width; }
    [[nodiscard]] u32 height() const noexcept { return m_height; }
    [[nodiscard]] bool isValid() const noexcept { return m_image != VK_NULL_HANDLE; }
    [[nodiscard]] bool isUploaded() const noexcept { return m_uploaded; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    VkFilter m_filter = VK_FILTER_NEAREST;

    // GPU 图像实际分配尺寸（用于检测是否需要重建）
    u32 m_imageWidth = 0;
    u32 m_imageHeight = 0;

    u32 m_width = 0;
    u32 m_height = 0;
    bool m_uploaded = false;

    [[nodiscard]] Result<void> _createImage();
    [[nodiscard]] Result<void> _createSampler();
    [[nodiscard]] Result<void> _createImageView();
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer _beginSingleTimeCommands();
    void _endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void _transitionImageLayout(VkCommandBuffer cmd,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage);
};

} // namespace mc::client::resource::atlas
