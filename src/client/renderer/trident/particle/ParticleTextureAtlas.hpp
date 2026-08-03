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
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {}

namespace mc::client::renderer::trident::particle {

/**
 * @brief 精灵纹理信息
 *
 * 存储粒子纹理在图集中的位置和动画信息。
 */
struct SpriteInfo {
    glm::vec2 uvMin;     ///< UV 左上角坐标
    glm::vec2 uvMax;     ///< UV 右下角坐标
    u32 frameCount = 1;  ///< 动画帧数（1 表示静态纹理）
    f64 frameTime = 0.0; ///< 每帧时间（秒）

    /**
     * @brief 检查是否为动画精灵
     */
    [[nodiscard]] bool isAnimated() const { return frameCount > 1; }

    /**
     * @brief 获取单帧 UV 高度
     */
    [[nodiscard]] f64 frameHeight() const
    {
        if (frameCount <= 1) {
            return uvMax.y - uvMin.y;
        }
        return (uvMax.y - uvMin.y) / static_cast<f64>(frameCount);
    }
};

/**
 * @brief 粒子纹理图集
 *
 * 管理粒子纹理的加载、打包和查询。
 *
 * 功能：
 * - 从资源包加载粒子纹理（textures/particle 目录下的 PNG 文件）
 * - 支持动画纹理（垂直帧条）
 * - 纹理打包优化
 * - GPU 纹理上传
 *
 * 用法示例：
 * @code
 * ParticleTextureAtlas atlas;
 * atlas.create(device, physicalDevice, commandPool, graphicsQueue, atlasWidth, atlasHeight);
 * atlas.loadFromResourcePacks(resourcePacks);
 * atlas.upload();
 *
 * // 获取精灵信息
 * const SpriteInfo* sprite = atlas.getSprite(ResourceLocation("minecraft:particle/flame"));
 * if (sprite) {
 *     // 使用 UV 坐标
 * }
 * @endcode
 */
class ParticleTextureAtlas {
public:
    ParticleTextureAtlas();
    ~ParticleTextureAtlas();

    // 禁止拷贝
    ParticleTextureAtlas(const ParticleTextureAtlas&) = delete;
    ParticleTextureAtlas& operator=(const ParticleTextureAtlas&) = delete;

    // 允许移动
    ParticleTextureAtlas(ParticleTextureAtlas&&) noexcept;
    ParticleTextureAtlas& operator=(ParticleTextureAtlas&&) noexcept;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 创建纹理图集
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
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
     * @brief 从资源包加载粒子纹理
     *
     * 加载 textures/particle 目录下的 PNG 文件。
     * 动画纹理通过垂直帧条存储（帧数 = 高度 / 宽度）。
     *
     * @param resourcePacks 资源包列表
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadFromResourcePacks(const std::vector<IResourcePack*>& resourcePacks);

    /**
     * @brief 上传纹理数据到 GPU
     *
     * 必须在调用 loadFromResourcePacks() 之后调用。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> upload();

    // ========================================================================
    // 查询
    // ========================================================================

    /**
     * @brief 获取精灵信息
     *
     * @param location 纹理资源位置（如 ResourceLocation("minecraft:particle/flame")）
     * @return 精灵信息指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const SpriteInfo* getSprite(const ResourceLocation& location) const;

    /**
     * @brief 获取动画精灵的当前帧 UV 坐标
     *
     * 对于动画精灵，根据粒子年龄计算当前帧的 UV 坐标。
     * 对于静态精灵，直接返回原始 UV 坐标。
     *
     * @param location 纹理资源位置
     * @param age 粒子年龄（秒）
     * @param maxAge 粒子最大年龄（秒）
     * @return UV 坐标 (minU, minV, maxU, maxV)
     */
    [[nodiscard]] glm::vec4 getAnimatedFrameUV(const ResourceLocation& location, f64 age, f64 maxAge) const;

    /**
     * @brief 获取动画精灵的随机帧 UV 坐标
     *
     * 用于随机选择初始帧的粒子。
     *
     * @param location 纹理资源位置
     * @param seed 随机种子
     * @return UV 坐标 (minU, minV, maxU, maxV)
     */
    [[nodiscard]] glm::vec4 getRandomFrameUV(const ResourceLocation& location, u32 seed) const;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 获取 Vulkan 图像视图
     */
    [[nodiscard]] VkImageView imageView() const { return m_imageView; }

    /**
     * @brief 获取 Vulkan 采样器
     */
    [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 检查纹理是否有效
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
     * @brief 获取已加载的精灵数量
     */
    [[nodiscard]] size_t spriteCount() const { return m_sprites.size(); }

private:
    // Vulkan 设备资源
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 纹理资源
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // 图集尺寸
    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_imageWidth = 0;  // GPU 图像实际宽度
    u32 m_imageHeight = 0; // GPU 图像实际高度

    // 精灵映射
    std::unordered_map<ResourceLocation, SpriteInfo> m_sprites;

    // 像素数据（上传前暂存）
    std::vector<u8> m_pixels;
    bool m_uploaded = false;

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 创建 Vulkan 图像
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
     * @brief 查找合适的内存类型
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

} // namespace mc::client::renderer::trident::particle
