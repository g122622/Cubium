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
#include "common/resource/ResourceLocation.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class IResourcePack; // Forward declaration in mc namespace
}

namespace mc::client::renderer::entity::pipeline {

/**
 * @brief 实体纹理图集构建结果
 */
struct EntityAtlasBuildResult {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    u32 width = 0;
    u32 height = 0;
    std::unordered_map<ResourceLocation, TextureRegion> regions;
};

/**
 * @brief 实体纹理图集
 *
 * 管理所有实体纹理的图集，类似方块纹理图集。
 * 支持 MC 1.12 和 MC 1.13+ 的纹理路径格式。
 */
class EntityTextureAtlas {
public:
    EntityTextureAtlas();
    ~EntityTextureAtlas();

    // 禁止拷贝
    EntityTextureAtlas(const EntityTextureAtlas&) = delete;
    EntityTextureAtlas& operator=(const EntityTextureAtlas&) = delete;

    /**
     * @brief 初始化图集
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池（用于纹理上传）
     * @param graphicsQueue 图形队列（用于纹理上传）
     * @param maxTextures 最大纹理数量
     * @param textureSize 单个纹理大小（默认64）
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 maxTextures = 256,
        u32 textureSize = 64);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 添加实体纹理
     * @param pack 资源包
     * @param location 纹理资源位置（如 minecraft:textures/entity/pig/pig.png）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> addTexture(mc::IResourcePack& pack, const ResourceLocation& location);

    /**
     * @brief 从文件系统添加实体纹理
     *
     * 支持直接加载 Java 版玩家皮肤 PNG 文件，用于本地玩家皮肤覆盖。
     *
     * @param filePath PNG 文件路径
     * @param location 图集中的资源位置键
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> addTextureFromFile(
        const std::filesystem::path& filePath, const ResourceLocation& location);

    /**
     * @brief 从原始像素数据添加纹理
     *
     * 用于运行时皮肤上传等场景。
     * 可以在图集构建后添加纹理，添加后需要调用 rebuild() 更新图集。
     *
     * @param pixels 像素数据（RGBA 格式）
     * @param width 宽度
     * @param height 高度
     * @param location 图集中的资源位置键
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> addTextureFromPixels(
        const std::vector<u8>& pixels, u32 width, u32 height, const ResourceLocation& location);

    /**
     * @brief 是否需要重建
     *
     * 如果在构建后添加了新纹理，返回 true。
     */
    [[nodiscard]] bool needsRebuild() const { return m_needsRebuild; }

    /**
     * @brief 重建图集
     *
     * 在运行时添加新纹理后调用，重新生成图集。
     * 注意：这是一个昂贵的操作，应批量添加纹理后一次性重建。
     *
     * @return 构建结果
     */
    [[nodiscard]] Result<EntityAtlasBuildResult> rebuild();

    /**
     * @brief 构建图集
     *
     * 将所有添加的纹理合并到一张大纹理中。
     * 必须在添加完所有纹理后调用。
     *
     * @return 构建结果
     */
    [[nodiscard]] Result<EntityAtlasBuildResult> build();

    /**
     * @brief 获取纹理区域
     * @param location 纹理资源位置
     * @return 纹理区域，如果不存在返回nullptr
     */
    [[nodiscard]] const TextureRegion* getRegion(const ResourceLocation& location) const;

    /**
     * @brief 获取纹理区域
     * @param location 纹理资源位置字符串
     * @return 纹理区域，如果不存在返回nullptr
     */
    [[nodiscard]] const TextureRegion* getRegion(const std::string& location) const;

    /**
     * @brief 获取图集图像视图
     */
    [[nodiscard]] VkImageView imageView() const { return m_imageView; }

    /**
     * @brief 获取图集采样器
     */
    [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 是否已构建
     */
    [[nodiscard]] bool isBuilt() const { return m_built; }

    /**
     * @brief 获取图集尺寸
     */
    [[nodiscard]] u32 width() const { return m_width; }
    [[nodiscard]] u32 height() const { return m_height; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 配置
    u32 m_maxTextures = 256;
    u32 m_textureSize = 64;

    // 图集
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    u32 m_width = 0;
    u32 m_height = 0;

    // 纹理数据（持久化存储，用于重建）
    struct TextureData {
        std::vector<u8> pixels;
        u32 width = 0;
        u32 height = 0;
        ResourceLocation location;
    };
    std::vector<TextureData> m_textures;

    // 等待添加的纹理（构建后添加）
    std::vector<TextureData> m_queuedTextures;

    // 纹理区域映射
    std::unordered_map<ResourceLocation, TextureRegion> m_regions;

    bool m_initialized = false;
    bool m_built = false;
    bool m_needsRebuild = false;

    /**
     * @brief 尝试加载纹理（支持多种路径格式）
     * @param pack 资源包
     * @param location 原始位置
     * @param outData 输出像素数据
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> _loadTextureWithFallback(mc::IResourcePack& pack,
        const ResourceLocation& location,
        std::vector<u8>& outData,
        u32& outWidth,
        u32& outHeight);

    /**
     * @brief 创建纹理采样器
     */
    [[nodiscard]] Result<void> _createSampler();

    /**
     * @brief 创建图像
     */
    [[nodiscard]] Result<void> _createImage(u32 width, u32 height);

    /**
     * @brief 上传纹理数据到图像
     */
    [[nodiscard]] Result<void> _uploadTextureData(const std::vector<u8>& data);

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
    void _endSingleTimeCommands(VkCommandBuffer cmd);
};

} // namespace mc::client::renderer::entity::pipeline
