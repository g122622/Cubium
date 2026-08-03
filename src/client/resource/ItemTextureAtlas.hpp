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

#include "client/renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {

class Item;

namespace client {

// 纹理图集像素缓冲区的追踪分配器：截获 m_pixels 每次 allocate/deallocate（含
// realloc 与 shrink_to_fit 的成对 free+alloc），自动维持 Tracy 的「同一指针严格
// 一对一 alloc/free」不变量。仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件。
// 详见 common/profiler/MemoryTracking.hpp。
template <typename T>
using TextureAtlasAlloc = ::mc::profiler::TracyTrackingAlloc<T, "TextureAtlas">;

/**
 * @brief 物品纹理图集
 *
 * 管理非方块物品的纹理图集（如工具、食物、材料等）。
 * 方块物品使用方块纹理图集。
 *
 * 纹理来源：
 * - 资源包中的 textures/item 目录下 PNG 纹理
 * - 方块纹理图集中的纹理（用于方块物品）
 */
class ItemTextureAtlas {
public:
    ItemTextureAtlas();
    ~ItemTextureAtlas();

    // 禁止拷贝
    ItemTextureAtlas(const ItemTextureAtlas&) = delete;
    ItemTextureAtlas& operator=(const ItemTextureAtlas&) = delete;

    // 允许移动
    ItemTextureAtlas(ItemTextureAtlas&&) noexcept;
    ItemTextureAtlas& operator=(ItemTextureAtlas&&) noexcept;

    /**
     * @brief 创建物品纹理图集
     *
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池（用于纹理上传）
     * @param graphicsQueue 图形队列（用于纹理上传）
     * @param width 图集宽度
     * @param height 图集高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 width,
        u32 height);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 从资源包加载物品纹理
     *
     * 加载 textures/item 目录下的 PNG 文件
     *
     * @param resourcePacks 资源包列表
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadFromResourcePacks(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks);

    /**
     * @brief 上传纹理数据到GPU
     *
     * 必须在调用 loadFromResourcePacks() 之后调用。
     * 使用内部暂存缓冲区上传数据。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> upload();

    /**
     * @brief 获取物品纹理区域
     *
     * @param itemId 物品ID
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTexture(u32 itemId) const;

    /**
     * @brief 获取物品纹理区域（通过资源位置）
     *
     * @param location 纹理资源位置
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTexture(const ResourceLocation& location) const;

    /**
     * @brief 获取物品的所有纹理层
     *
     * 用于多层纹理物品（如附魔光效）。
     * 物品模型可以有多个纹理层：layer0, layer1, layer2...
     *
     * @param textureLocations 纹理资源位置列表（如从 BakedItemModel::textureLayers 获取）
     * @return 纹理区域列表（跳过不存在的纹理）
     */
    [[nodiscard]] std::vector<TextureRegion> getItemTextureLayers(
        const std::vector<ResourceLocation>& textureLocations) const;

    /**
     * @brief 检查是否有该物品的纹理
     */
    [[nodiscard]] bool hasItemTexture(u32 itemId) const;

    /**
     * @brief 添加纹理区域映射
     *
     * 用于将方块纹理映射到物品
     *
     * @param itemId 物品ID
     * @param region 纹理区域
     */
    void addTextureRegion(u32 itemId, const TextureRegion& region);

    /**
     * @brief 添加纹理区域映射（通过资源位置）
     *
     * @param location 纹理资源位置
     * @param region 纹理区域
     */
    void addTextureRegion(const ResourceLocation& location, const TextureRegion& region);

    /**
     * @brief 获取图像视图
     */
    VkImageView imageView() const { return m_imageView; }

    /**
     * @brief 获取采样器
     */
    VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return m_image != VK_NULL_HANDLE; }

    /**
     * @brief 检查纹理数据是否已上传
     */
    [[nodiscard]] bool isUploaded() const { return m_uploaded; }

    /**
     * @brief 获取图集宽度
     */
    [[nodiscard]] u32 width() const { return m_width; }

    /**
     * @brief 获取图集高度
     */
    [[nodiscard]] u32 height() const { return m_height; }

    /**
     * @brief 获取已加载的纹理数量
     */
    [[nodiscard]] size_t textureCount() const { return m_regionsByItemId.size(); }

    /**
     * @brief 上传纹理图集子区域数据
     *
     * 用于动画纹理帧更新，只更新图集中指定区域的像素。
     *
     * @param pixelData RGBA8 像素数据
     * @param size 数据大小（字节）
     * @param offsetX 目标区域在图集中的 X 偏移（像素）
     * @param offsetY 目标区域在图集中的 Y 偏移（像素）
     * @param width 区域宽度（像素）
     * @param height 区域高度（像素）
     * @param rowLength 源数据行长度（像素），0 表示使用 width
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> uploadRegion(
        const void* pixelData, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 rowLength);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 纹理资源
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // GPU 图像实际分配尺寸（用于检测是否需要重建）
    u32 m_imageWidth = 0;
    u32 m_imageHeight = 0;

    u32 m_width = 0;
    u32 m_height = 0;
    bool m_uploaded = false;

    // 纹理区域映射（按物品ID）
    std::unordered_map<u32, TextureRegion> m_regionsByItemId;

    // 纹理区域映射（按资源位置）
    std::unordered_map<ResourceLocation, TextureRegion> m_regionsByLocation;

    // 待上传的像素数据
    std::vector<u8, TextureAtlasAlloc<u8>> m_pixels;

    /**
     * @brief 创建图像
     */
    [[nodiscard]] Result<void> _createImage();

    /**
     * @brief 创建采样器
     */
    [[nodiscard]] Result<void> _createSampler();

    /**
     * @brief 创建图像视图
     */
    [[nodiscard]] Result<void> _createImageView();

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer _beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void _endSingleTimeCommands(VkCommandBuffer commandBuffer);

    /**
     * @brief 转换图像布局
     */
    void _transitionImageLayout(VkCommandBuffer cmd,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage);
};

} // namespace client
} // namespace mc
