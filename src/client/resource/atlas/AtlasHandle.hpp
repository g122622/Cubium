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

#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <vector>
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
 * 所有 CPU→GPU 像素上传（整图 upload、单区域 uploadRegion、批量 uploadRegionsBatch）
 * 统一经注入的 IStagingBufferPool 子分配暂存区间，单次命令缓冲提交，不再每次新建
 * 临时 staging buffer + 阻塞 fence。由 AtlasManager 持有，每个具名图集一个 AtlasHandle。
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
     * @param stagingPool 统一暂存缓冲池（所有像素上传经此子分配，不可为空）
     * @param width 图集宽度
     * @param height 图集高度
     * @param filter 采样过滤模式（NEAREST 用于 blocks/items，LINEAR 用于 particles/gui）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        renderer::api::IStagingBufferPool* stagingPool,
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
     * @brief 上传图集子区域像素（低频单次上传，如运行时注入皮肤）
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

    /**
     * @brief 单次提交批量上传多个图集子区域（动画帧更新主路径）
     *
     * 所有区域先经暂存池子分配到同一 backing buffer（各自独立 offset），CPU memcpy
     * 各自像素；随后在单个命令缓冲内录制一次 SHADER_READ_ONLY→TRANSFER_DST layout
     * 转换、N 个 vkCmdCopyBufferToImage（每个用各自的 staging offset）、一次
     * TRANSFER_DST→SHADER_READ_ONLY 转换；最后一次 submit+wait，逐个 release。
     *
     * 相比逐区域 uploadRegion 的 N 次 submit+wait，本方法把 GPU 同步开销从 N 降到 1，
     * 是动画帧上传的性能关键路径。
     *
     * @param regions 待上传区域数组（像素指针需在调用期间保持有效）
     * @return 成功或错误；单个区域 stage 失败（池容量不足）则整批返回错误
     */
    struct RegionUpload {
        const void* pixelData; ///< RGBA8 像素数据
        u64 size;              ///< 字节数
        u32 offsetX;           ///< 目标区域 X 偏移（像素）
        u32 offsetY;           ///< 目标区域 Y 偏移（像素）
        u32 width;             ///< 区域宽度（像素）
        u32 height;            ///< 区域高度（像素）
        u32 rowLength;         ///< 源数据行长度（像素），0 表示使用 width
    };
    [[nodiscard]] Result<void> uploadRegionsBatch(const std::vector<RegionUpload>& regions);

    /**
     * @brief 批量上传多个图集子区域，录进调用方提供的帧命令缓冲（动画帧热路径）
     *
     * 与 uploadRegionsBatch 的区别：暂存区间用 stageAsync 分配（登记到 frameIndex 回收桶，
     * 由 stagingPool 在下一轮同 slot 的 recycleFrame 回收，调用方不 release），layout
     * 转换 + N 次 vkCmdCopyBufferToImage 全部录进传入的 cmd，随帧命令缓冲一次性 submit，
     * 用帧 fence 同步。不创建独立命令缓冲、不 vkWaitForFences，消除每帧阻塞等待。
     *
     * 必须在 beginFrame 之后、且在该图集被任何 draw 采样之前录制，保证 copy→屏障→draw
     * 的执行顺序。
     *
     * @param regions 待上传区域数组（像素指针需在帧 submit 前保持有效）
     * @param cmd 当前帧命令缓冲（由调用方在 beginFrame 后获取，不可为空）
     * @param frameIndex 当前帧索引（用于 stageAsync 回收桶登记）
     * @return 成功或错误；单个区域 stageAsync 失败（池容量不足）则整批返回错误，
     *         未标记 markUploaded 的 sprite 下帧重试。已成功 stageAsync 的区间由
     *         recycleFrame 回收，不泄漏。
     */
    [[nodiscard]] Result<void> uploadRegionsBatchAsync(
        const std::vector<RegionUpload>& regions, VkCommandBuffer cmd, u32 frameIndex);

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
    renderer::api::IStagingBufferPool* m_stagingPool = nullptr;

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
