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

#include "client/renderer/api/texture/ITexture.hpp"
#include "client/renderer/api/texture/ITextureAtlas.hpp"
#include "client/renderer/api/texture/TextureRegion.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前向声明
class TridentContext;

/**
 * @brief Vulkan 纹理实现
 *
 * 实现 api::ITexture 接口，提供 Vulkan 纹理功能。
 */
class TridentTexture : public api::ITexture {
public:
    TridentTexture();
    ~TridentTexture() override;

    // 禁止拷贝
    TridentTexture(const TridentTexture&) = delete;
    TridentTexture& operator=(const TridentTexture&) = delete;

    // 允许移动
    TridentTexture(TridentTexture&& other) noexcept;
    TridentTexture& operator=(TridentTexture&& other) noexcept;

    /**
     * @brief 创建纹理
     * @param context Trident 上下文
     * @param desc 纹理描述
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(TridentContext* context, const api::TextureDesc& desc);

    /**
     * @brief 从现有 Vulkan 图像创建纹理
     * @param context Trident 上下文
     * @param image Vulkan 图像
     * @param imageView 图像视图
     * @param format 格式
     * @param width 宽度
     * @param height 高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> createFromExisting(
        TridentContext* context, VkImage image, VkImageView imageView, VkFormat format, u32 width, u32 height);

    // ITexture 接口实现
    void destroy() override;
    [[nodiscard]] u32 width() const override { return m_width; }
    [[nodiscard]] u32 height() const override { return m_height; }
    [[nodiscard]] u32 depth() const override { return m_depth; }
    [[nodiscard]] u32 mipLevels() const override { return m_mipLevels; }
    [[nodiscard]] api::TextureFormat format() const override { return m_format; }
    [[nodiscard]] bool isValid() const override { return m_image != VK_NULL_HANDLE; }
    [[nodiscard]] Result<void> upload(const void* data, u64 size, u32 level) override;
    [[nodiscard]] Result<void> uploadRegion(
        const void* data, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 level, u32 rowLength) override;
    void bind(u32 binding) override;

    // Vulkan 特有方法
    [[nodiscard]] VkImage image() const { return m_image; }
    [[nodiscard]] VkImageView imageView() const { return m_imageView; }
    [[nodiscard]] VkSampler sampler() const { return m_sampler; }
    [[nodiscard]] VkImageLayout layout() const { return m_layout; }

    /**
     * @brief 转换图像布局
     */
    void transitionLayout(VkCommandBuffer cmd,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage);

    /**
     * @brief 生成 mipmaps
     */
    [[nodiscard]] Result<void> generateMipmaps(VkCommandBuffer cmd);

private:
    TridentContext* m_context = nullptr;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    VkFormat m_vkFormat = VK_FORMAT_UNDEFINED;
    api::TextureFormat m_format = api::TextureFormat::R8G8B8A8_SRGB;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_depth = 1;
    u32 m_mipLevels = 1;
    u32 m_arrayLayers = 1;
    bool m_ownsImage = true; // 是否拥有图像资源

    /**
     * @brief 创建图像
     */
    [[nodiscard]] Result<void> createImage(VkImageUsageFlags usage, VkImageTiling tiling);

    /**
     * @brief 创建图像视图
     */
    [[nodiscard]] Result<void> createImageView(VkImageAspectFlags aspect);

    /**
     * @brief 创建采样器
     */
    [[nodiscard]] Result<void> createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode);

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 转换纹理格式到 Vulkan 格式
     */
    static VkFormat toVkFormat(api::TextureFormat format);

    /**
     * @brief 转换过滤模式到 Vulkan 过滤模式
     */
    static VkFilter toVkFilter(api::TextureFilter filter);

    /**
     * @brief 转换寻址模式到 Vulkan 寻址模式
     */
    static VkSamplerAddressMode toVkAddressMode(api::TextureAddressMode mode);
};

/**
 * @brief Vulkan 纹理图集实现
 *
 * 实现 api::ITextureAtlasBuilder 接口。
 */
class TridentTextureAtlas {
public:
    TridentTextureAtlas();
    ~TridentTextureAtlas();

    // 禁止拷贝
    TridentTextureAtlas(const TridentTextureAtlas&) = delete;
    TridentTextureAtlas& operator=(const TridentTextureAtlas&) = delete;

    // 允许移动
    TridentTextureAtlas(TridentTextureAtlas&& other) noexcept;
    TridentTextureAtlas& operator=(TridentTextureAtlas&& other) noexcept;

    /**
     * @brief 创建纹理图集
     * @param context Trident 上下文
     * @param width 图集宽度
     * @param height 图集高度
     * @param tileSize 瓦片大小
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(TridentContext* context, u32 width, u32 height, u32 tileSize);

    /**
     * @brief 从构建结果上传图集数据
     * @param result 图集构建结果
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> upload(const api::AtlasBuildResult& result);

    void destroy();

    // 获取器
    [[nodiscard]] TridentTexture& texture() { return m_texture; }
    [[nodiscard]] const TridentTexture& texture() const { return m_texture; }
    [[nodiscard]] u32 width() const { return m_width; }
    [[nodiscard]] u32 height() const { return m_height; }
    [[nodiscard]] u32 tileSize() const { return m_tileSize; }
    [[nodiscard]] u32 tilesPerRow() const { return m_tilesPerRow; }
    [[nodiscard]] bool isValid() const { return m_texture.isValid(); }

    /**
     * @brief 获取指定位置的纹理区域
     * @param tileX 瓦片X坐标
     * @param tileY 瓦片Y坐标
     * @return 纹理区域
     */
    [[nodiscard]] api::TextureRegion getRegion(u32 tileX, u32 tileY) const;

    /**
     * @brief 获取指定索引的纹理区域
     * @param tileIndex 瓦片索引
     * @return 纹理区域
     */
    [[nodiscard]] api::TextureRegion getRegion(u32 tileIndex) const;

private:
    TridentContext* m_context = nullptr;
    TridentTexture m_texture;
    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_tileSize = 16;
    u32 m_tilesPerRow = 0;
    f64 m_tileU = 0.0; // 单个瓦片在U方向的宽度
    f64 m_tileV = 0.0; // 单个瓦片在V方向的高度
};

} // namespace mc::client::renderer::trident
