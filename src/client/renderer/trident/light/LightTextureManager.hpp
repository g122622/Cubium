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
#include <array>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::light {

/// 光照贴图尺寸（16×16：blockLight × skyLight 网格）
constexpr u32 LIGHTMAP_SIZE = 16;

/**
 * @brief 光照贴图输入参数
 *
 * 聚合原版 LightTexture.updateLightTexture 所需参数，按项目可得性分类：
 * - 可得：skyLightColor/skyLightFactor/sunIntensity/ambientLight/darkenWorldAmount
 * - 暂不可得（置默认 + TODO）：gamma/darknessEffectScale/nightVision/darkness/waterVision/endFlash
 */
struct LightmapInputs {
    /// 天空光颜色（来自 SkyRenderer，RGB 线性 0..1）
    glm::vec3 skyLightColor = glm::vec3(1.0f);
    /// 天空光因子（日夜 + 雨雷衰减后的天空光强度，0..1）
    f32 skyLightFactor = 1.0f;
    /// 太阳强度（来自 SkyRenderer，0..1）
    f32 sunIntensity = 1.0f;
    /// 维度环境光（主世界 0.0，下界 0.1，末地 0.0）
    f32 ambientLight = 0.0f;
    /// 闪电闪烁时世界变亮程度（0..1，1 = 闪电中）
    f32 darkenWorldAmount = 0.0f;
    /// 用户 gamma 选项（项目暂未接入，默认 0.5） TODO: 接入 ClientSettings gamma
    f32 gamma = 0.5f;
    /// 黑暗效果缩放（项目暂未接入，默认 1.0） TODO: 接入玩家 DARKNESS 效果
    f32 darknessEffectScale = 1.0f;
    /// 夜视强度（项目暂未接入，默认 0） TODO: 接入玩家 NIGHT_VISION 效果
    f32 nightVision = 0.0f;
    /// 黑暗强度（项目暂未接入，默认 0） TODO: 接入玩家 DARKNESS 效果
    f32 darkness = 0.0f;
    /// 末地闪烁（项目暂未接入，默认 0） TODO: 末地 endFlash
    f32 endFlash = 0.0f;
};

/**
 * @brief 光照贴图管理器
 *
 * 维护一张 16×16 RGBA8 光照贴图纹理，每帧由 CPU 按原版 LightTexture 的 getBrightness
 * 曲线与各调制参数计算像素后上传 GPU，供天气渲染器（及未来其它渲染器）采样。
 *
 * 生成方式：CPU 计算 256 像素 + staging buffer 上传。16×16 开销可忽略，
 * 且复用现有 VulkanUtils 上传通路，无需建立离屏 render pass（项目无此先例）。
 */
class LightTextureManager {
public:
    LightTextureManager();
    ~LightTextureManager();

    // 禁止拷贝
    LightTextureManager(const LightTextureManager&) = delete;
    LightTextureManager& operator=(const LightTextureManager&) = delete;

    /**
     * @brief 初始化光照贴图资源
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池（用于初始化阶段的一次性上传命令）
     * @param graphicsQueue 图形队列
     * @param stagingPool 统一暂存缓冲池（每帧像素上传经此 stageAsync 子分配，不可为空）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        renderer::api::IStagingBufferPool* stagingPool);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 按输入参数重建光照贴图（同步上传，仅初始化/重载用）
     *
     * CPU 计算 16×16 像素后，用 stagingPool 的同步 stage/release 路径上传（独立 submit+wait）。
     * 仅用于 initialize 首帧；每帧热路径请用 updateLightTextureAsync。
     *
     * @param inputs 光照贴图输入参数
     */
    void updateLightTexture(const LightmapInputs& inputs);

    /**
     * @brief 按输入参数重建光照贴图并异步上传（每帧热路径）
     *
     * CPU 计算 16×16 像素后，用 stagingPool 的 stageAsync 子分配暂存区间（登记到 frameIndex
     * 回收桶，由 recycleFrame 回收），把布局转换 + 拷贝录进传入的帧命令缓冲，随帧 submit、
     * 用帧 fence 同步，不再独立 submit+wait 阻塞 CPU。
     *
     * 必须在 render pass 之前录制（vkCmdCopyBufferToImage 不能在 render pass 内）。
     *
     * @param inputs 光照贴图输入参数
     * @param cmd 当前帧命令缓冲（由调用方在 beginFrame 后获取，不可为空）
     * @param frameIndex 当前帧索引（用于 stageAsync 回收桶登记）
     */
    void updateLightTextureAsync(const LightmapInputs& inputs, VkCommandBuffer cmd, u32 frameIndex);

    /**
     * @brief 光照贴图图像视图（供 descriptor 绑定）
     */
    [[nodiscard]] VkImageView textureView() const { return m_lightmapView; }

    /**
     * @brief 光照贴图采样器
     */
    [[nodiscard]] VkSampler textureSampler() const { return m_sampler; }

    /**
     * @brief 是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /// 计算单个 (blockLight, skyLight) 格点的光照颜色（对齐原版 getBrightness 曲线 + 调制）
    [[nodiscard]] static glm::vec3 _computePixel(u32 blockLight, u32 skyLight, const LightmapInputs& inputs);

    /// 把 m_pixels 按输入重算（纯 CPU，无 Vulkan 调用）
    void _computePixels(const LightmapInputs& inputs);

    /// 把 staging 区间内的像素拷进光照贴图图像：布局转换 + copy + 转回采样布局，录进 cmd
    void _recordUpload(VkCommandBuffer cmd, VkBuffer stagingBuffer, VkDeviceSize stagingOffset);

    /// 布局转换：SHADER_READ_ONLY ↔ TRANSFER_DST
    void _transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    renderer::api::IStagingBufferPool* m_stagingPool = nullptr;
    bool m_initialized = false;

    VkImage m_lightmapImage = VK_NULL_HANDLE;
    VkDeviceMemory m_lightmapMemory = VK_NULL_HANDLE;
    VkImageView m_lightmapView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // 像素缓冲（CPU 端计算结果，16×16×4 字节）
    std::array<u8, LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4> m_pixels{};
};

} // namespace mc::client::renderer::trident::light
