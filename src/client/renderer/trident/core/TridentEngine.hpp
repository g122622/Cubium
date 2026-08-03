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

#include "client/renderer/api/IRenderEngine.hpp"
#include "client/renderer/api/Types.hpp"
#include "client/renderer/api/buffer/IBuffer.hpp"
#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include "client/renderer/api/camera/ICamera.hpp"
#include "client/renderer/api/pipeline/RenderType.hpp"
#include "client/renderer/api/texture/ITexture.hpp"
#include "client/renderer/api/texture/TextureRegion.hpp"
#include "client/renderer/trident/cloud/CloudMode.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/core/texture/AnimatedSprite.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <functional>
#include <map>
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

// 前置声明
struct GLFWwindow;

namespace mc {
struct TextureRegion;
}

namespace mc::client {
class ChunkRenderer;
class ClientWorld;
class Font;
} // namespace mc::client

namespace mc::client::renderer::entity {
class EntityRendererManager;

namespace pipeline {
class EntityPipeline;
}

} // namespace mc::client::renderer::entity

namespace mc::client::renderer::trident {

// 导入实体纹理图集类型
using entity::pipeline::EntityPipeline;
using entity::pipeline::EntityTextureAtlas;

// 前置声明
class TridentSwapchain;
class RenderPassManager;
class FrameManager;
class DescriptorManager;
class UniformManager;
class TridentPipeline;
class AtlasManagerOwner; // PIMPL：持有 resource::atlas::AtlasManager，避免本头引入
                         // mc::client::resource::atlas 命名空间（会破坏 FireAnimationState.hpp
                         // 中 resource::metadata 的父作用域查找）

// 子命名空间的前置声明
namespace gui {
class GuiRenderer;
}

namespace sky {
class SkyRenderer;
}

namespace fog {
class FogManager;
}

namespace light {
class LightTextureManager;
}

namespace cloud {
class CloudRenderer;
} // namespace cloud

namespace particle {
class ParticleManager;
}

namespace weather {
class WeatherRenderer;
}

namespace item {
class ItemRenderer;
}

namespace block {
class BreakProgressRenderer;
}

namespace firstperson {
class FirstPersonRenderer;
}

/**
 * @brief GUI 渲染回调类型
 *
 * 在每帧的 GUI 渲染阶段被调用，用于绘制自定义 GUI 元素。
 */
using GuiRenderCallback = std::function<void()>;

/**
 * @brief 实体渲染回调类型
 *
 * 在每帧的实体渲染阶段被调用。
 * @param cmd 当前命令缓冲区
 * @param partialTick 部分 tick（用于插值）
 */
using EntityRenderCallback = std::function<void(VkCommandBuffer, f64)>;

/**
 * @brief 第一人称手部渲染回调类型
 *
 * 在每帧的第一人称手部渲染阶段被调用。
 * @param cmd 当前命令缓冲区
 * @param cameraDescriptorSet 相机描述符集
 * @param partialTick 部分 tick（用于插值）
 */
using FirstPersonRenderCallback = std::function<void(VkCommandBuffer, VkDescriptorSet, f64)>;

/**
 * @brief 最大同时在飞帧数
 */
static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

/**
 * @brief Trident 渲染引擎
 *
 * 实现了平台无关的 IRenderEngine 接口。
 * 这是渲染系统的主入口点，协调所有渲染组件。
 */
class TridentEngine : public api::IRenderEngine {
public:
    TridentEngine();
    ~TridentEngine() override;

    // 禁止拷贝
    TridentEngine(const TridentEngine&) = delete;
    TridentEngine& operator=(const TridentEngine&) = delete;

    // ========================================================================
    // IRenderEngine 接口实现
    // ========================================================================

    [[nodiscard]] Result<void> initialize(void* window, const api::RenderEngineConfig& config) override;
    void destroy() override;

    [[nodiscard]] Result<void> beginFrame() override;
    [[nodiscard]] Result<void> endFrame() override;
    [[nodiscard]] Result<void> present() override;

    [[nodiscard]] Result<void> onResize(u32 width, u32 height) override;
    void setCamera(const api::ICamera* camera) override;

    [[nodiscard]] Result<std::unique_ptr<api::IVertexBuffer>> createVertexBuffer(u64 size, u32 vertexStride) override;
    [[nodiscard]] Result<std::unique_ptr<api::IIndexBuffer>> createIndexBuffer(u64 size, api::IndexType type) override;
    [[nodiscard]] Result<std::unique_ptr<api::IUniformBuffer>> createUniformBuffer(u64 size, u32 frameCount) override;
    [[nodiscard]] Result<std::unique_ptr<api::ITexture>> createTexture(const api::TextureDesc& desc) override;
    [[nodiscard]] Result<std::unique_ptr<api::ITextureAtlas>> createTextureAtlas(
        u32 width, u32 height, u32 tileSize) override;

    void setRenderType(const api::RenderType& type) override;
    [[nodiscard]] const api::RenderType& currentRenderType() const override;

    void bindTexture(u32 binding, const api::ITexture* texture) override;
    void bindUniformBuffer(u32 binding, const api::IUniformBuffer* buffer) override;

    void drawIndexed(u32 indexCount, u32 firstIndex, i32 vertexOffset) override;
    void draw(u32 vertexCount, u32 firstVertex) override;
    void drawIndexedInstanced(
        u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) override;

    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] u32 currentFrameIndex() const override;
    [[nodiscard]] u32 currentImageIndex() const override;
    [[nodiscard]] const api::FrameContext& frameContext() const override;
    [[nodiscard]] u32 maxFramesInFlight() const override;
    [[nodiscard]] bool isMinimized() const override;
    [[nodiscard]] u32 windowWidth() const override;
    [[nodiscard]] u32 windowHeight() const override;
    [[nodiscard]] const api::ICamera* camera() const override;

    // ========================================================================
    // Trident 特有接口
    // ========================================================================

    /**
     * @brief 获取 Trident 上下文
     */
    [[nodiscard]] TridentContext* context() { return m_context.get(); }
    [[nodiscard]] const TridentContext* context() const { return m_context.get(); }

    /**
     * @brief 获取交换链
     */
    [[nodiscard]] TridentSwapchain* swapchain() { return m_swapchain.get(); }
    [[nodiscard]] const TridentSwapchain* swapchain() const { return m_swapchain.get(); }

    /**
     * @brief 获取渲染通道管理器
     */
    [[nodiscard]] RenderPassManager* renderPassManager() { return m_renderPassManager.get(); }
    [[nodiscard]] const RenderPassManager* renderPassManager() const { return m_renderPassManager.get(); }

    /**
     * @brief 获取帧管理器
     */
    [[nodiscard]] FrameManager* frameManager() { return m_frameManager.get(); }
    [[nodiscard]] const FrameManager* frameManager() const { return m_frameManager.get(); }

    /**
     * @brief 获取描述符管理器
     */
    [[nodiscard]] DescriptorManager* descriptorManager() { return m_descriptorManager.get(); }
    [[nodiscard]] const DescriptorManager* descriptorManager() const { return m_descriptorManager.get(); }

    /**
     * @brief 获取 Uniform 管理器
     */
    [[nodiscard]] UniformManager* uniformManager() { return m_uniformManager.get(); }
    [[nodiscard]] const UniformManager* uniformManager() const { return m_uniformManager.get(); }

    /**
     * @brief 获取渲染通道
     */
    [[nodiscard]] VkRenderPass renderPass() const;

    /**
     * @brief 获取命令缓冲区
     */
    [[nodiscard]] VkCommandBuffer currentCommandBuffer() const;

    /**
     * @brief 获取管线布局
     */
    [[nodiscard]] VkPipelineLayout pipelineLayout() const;

    /**
     * @brief 获取描述符池
     */
    [[nodiscard]] VkDescriptorPool descriptorPool() const;

    /**
     * @brief 获取相机描述符布局
     */
    [[nodiscard]] VkDescriptorSetLayout cameraDescriptorLayout() const;

    /**
     * @brief 获取纹理描述符布局
     */
    [[nodiscard]] VkDescriptorSetLayout textureDescriptorLayout() const;

    /**
     * @brief 获取雾效果描述符布局
     */
    [[nodiscard]] VkDescriptorSetLayout fogDescriptorLayout() const;

    /**
     * @brief 获取当前帧的相机描述符集
     */
    [[nodiscard]] VkDescriptorSet cameraDescriptorSet() const;

    /**
     * @brief 更新时间状态
     */
    void updateTime(i64 dayTime, i64 gameTime, f64 partialTick = 0.0f);

    /**
     * @brief 更新天气状态
     *
     * @param rainStrength 降雨强度 (0.0 - 1.0)
     * @param thunderStrength 雷暴强度 (0.0 - 1.0)
     */
    void updateWeather(f64 rainStrength, f64 thunderStrength);

    /**
     * @brief 注入客户端世界
     *
     * 供雾距、lightmap 等需要在渲染帧查询相机位置 skyLight/biome 的子系统使用。
     * 仅缓存指针，调用方需保证 world 生命周期覆盖后续渲染帧。
     *
     * @param world 客户端世界（可为 nullptr 解绑）
     */
    void setClientWorld(mc::client::ClientWorld* world);

    /**
     * @brief 获取已注入的客户端世界
     */
    [[nodiscard]] mc::client::ClientWorld* clientWorld() const { return m_clientWorld; }

    /**
     * @brief 设置闪电闪烁亮度
     *
     * 当闪电击中时，天空会短暂变亮。
     *
     * @param brightness 闪电闪烁亮度 (0.0-1.0)，0表示无效果
     */
    void setLightningFlashBrightness(f64 brightness);

    /**
     * @brief 设置维度环境光
     *
     * 主世界 0.0，下界 0.2，末地 0.0。用于光照贴图与天空亮度下限计算。
     *
     * @param ambientLight 环境光强度 (0.0-1.0)
     */
    void setAmbientLight(f64 ambientLight);

    /**
     * @brief 运行时切换 VSync（会触发交换链重建）
     */
    [[nodiscard]] Result<void> setVSyncEnabled(bool enabled);

    /**
     * @brief 更新渲染距离（区块）
     *
     * 该值会用于地表雾计算。
     */
    void setRenderDistanceChunks(i32 renderDistanceChunks);

    /**
     * @brief 更新地表雾气密度（来自 video.fogDensity）
     *
     * 范围建议：[0.0, 2.0]，1.0 表示默认效果。
     */
    void setLandFogDensity(f64 fogDensity);

    /**
     * @brief 更新云渲染模式（Off/Fast/Fancy）
     */
    void setCloudMode(cloud::CloudMode mode);

    /**
     * @brief 更新云高度（维度相关）
     *
     * - 主世界: 192.0 (项目定义)
     * - 下界: 无云
     * - 末地: 无云
     *
     * 当 hasClouds 为 false 时，云渲染将被跳过。
     *
     * @param cloudHeight 云高度
     * @param hasClouds 该维度是否有云
     */
    void setCloudHeight(f64 cloudHeight, bool hasClouds);

    /**
     * @brief 更新液体状态（用于雾效果）
     *
     * @param inWater 是否在水中
     * @param inLava 是否在岩浆中
     * @param waterFogColor 水下雾颜色（RGB格式）
     */
    void updateLiquidState(bool inWater, bool inLava, u32 waterFogColor);

    /**
     * @brief 执行一帧渲染
     *
     * 这是便捷方法，整合了 beginFrame、渲染、endFrame 和 present。
     * 子渲染器应通过回调注册。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> render();

    /**
     * @brief 获取命令池
     */
    [[nodiscard]] VkCommandPool commandPool() const;

    /**
     * @brief 开始单次命令
     */
    [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;

    /**
     * @brief 结束单次命令
     */
    void endSingleTimeCommands(VkCommandBuffer cmd) const;

    // ========================================================================
    // 兼容性接口 - 用于迁移期间
    // ========================================================================

    /**
     * @brief 获取设备
     */
    [[nodiscard]] VkDevice device() const;

    /**
     * @brief 获取物理设备
     */
    [[nodiscard]] VkPhysicalDevice physicalDevice() const;

    /**
     * @brief 获取图形队列
     */
    [[nodiscard]] VkQueue graphicsQueue() const;

    /**
     * @brief 获取交换链图像视图
     */
    [[nodiscard]] VkImageView swapchainImageView(u32 index) const;

    /**
     * @brief 获取交换链图像数量
     */
    [[nodiscard]] u32 swapchainImageCount() const;

    /**
     * @brief 获取交换链格式
     */
    [[nodiscard]] VkFormat swapchainFormat() const;

    /**
     * @brief 获取交换链范围
     */
    [[nodiscard]] VkExtent2D swapchainExtent() const;

    /**
     * @brief 获取深度图像视图
     */
    [[nodiscard]] VkImageView depthImageView() const;

    /**
     * @brief 获取最大帧在飞数
     */
    [[nodiscard]] static constexpr u32 maxFramesInFlightStatic() { return MAX_FRAMES_IN_FLIGHT; }

    // ========================================================================
    // 渲染回调
    // ========================================================================

    /**
     * @brief 设置 GUI 渲染回调
     *
     * 回调会在每帧的 GUI 渲染阶段被调用，用于绘制自定义 GUI 元素。
     *
     * @param callback GUI 渲染回调函数
     */
    void setGuiRenderCallback(GuiRenderCallback callback);

    /**
     * @brief 设置实体渲染回调
     *
     * 回调会在每帧的实体渲染阶段被调用。
     *
     * @param callback 实体渲染回调函数
     */
    void setEntityRenderCallback(EntityRenderCallback callback);

    /**
     * @brief 设置第一人称手部渲染回调
     *
     * 回调会在每帧的第一人称手部渲染阶段被调用。
     *
     * @param callback 第一人称手部渲染回调函数
     */
    void setFirstPersonRenderCallback(FirstPersonRenderCallback callback);

    // ========================================================================
    // 子渲染器访问器（用于迁移期间）
    // ========================================================================

    /**
     * @brief 获取区块渲染器
     */
    [[nodiscard]] ChunkRenderer& chunkRenderer();
    [[nodiscard]] const ChunkRenderer& chunkRenderer() const;
    [[nodiscard]] bool isChunkRendererInitialized() const { return m_chunkRendererInitialized; }

    /**
     * @brief 获取天空渲染器
     */
    [[nodiscard]] sky::SkyRenderer& skyRenderer();
    [[nodiscard]] const sky::SkyRenderer& skyRenderer() const;
    [[nodiscard]] bool isSkyRendererInitialized() const { return m_skyRendererInitialized; }

    /**
     * @brief 获取 GUI 渲染器
     */
    [[nodiscard]] gui::GuiRenderer& guiRenderer();
    [[nodiscard]] const gui::GuiRenderer& guiRenderer() const;
    [[nodiscard]] bool isGuiRendererInitialized() const { return m_guiRendererInitialized; }

    /**
     * @brief 获取字体
     */
    [[nodiscard]] Font& font();
    [[nodiscard]] const Font& font() const;

    /**
     * @brief 获取物品渲染器
     */
    [[nodiscard]] item::ItemRenderer& itemRenderer();
    [[nodiscard]] const item::ItemRenderer& itemRenderer() const;
    [[nodiscard]] bool isItemRendererInitialized() const { return m_itemRendererInitialized; }

    /**
     * @brief 获取物品纹理图集
     */
    [[nodiscard]] ItemTextureAtlas& itemTextureAtlas();
    [[nodiscard]] const ItemTextureAtlas& itemTextureAtlas() const;

    /**
     * @brief 统一图集管理器状态（数据驱动）
     */
    [[nodiscard]] bool isAtlasManagerInitialized() const { return m_atlasManagerInitialized; }

    /**
     * @brief 获取实体渲染器管理器
     */
    [[nodiscard]] entity::EntityRendererManager& entityRendererManager();
    [[nodiscard]] const entity::EntityRendererManager& entityRendererManager() const;
    [[nodiscard]] bool isEntityRendererInitialized() const { return m_entityRendererInitialized; }

    /**
     * @brief 获取实体纹理图集
     */
    [[nodiscard]] EntityTextureAtlas& entityTextureAtlas();
    [[nodiscard]] const EntityTextureAtlas& entityTextureAtlas() const;

    /**
     * @brief 获取雾效果管理器
     */
    [[nodiscard]] fog::FogManager& fogManager();
    [[nodiscard]] const fog::FogManager& fogManager() const;
    [[nodiscard]] bool isFogManagerInitialized() const { return m_fogManagerInitialized; }

    /**
     * @brief 获取云渲染器
     */
    [[nodiscard]] cloud::CloudRenderer& cloudRenderer();
    [[nodiscard]] const cloud::CloudRenderer& cloudRenderer() const;
    [[nodiscard]] bool isCloudRendererInitialized() const { return m_cloudRendererInitialized; }

    /**
     * @brief 获取粒子管理器
     */
    [[nodiscard]] particle::ParticleManager& particleManager();
    [[nodiscard]] const particle::ParticleManager& particleManager() const;
    [[nodiscard]] bool isParticleManagerInitialized() const { return m_particleManagerInitialized; }

    /**
     * @brief 获取天气渲染器
     */
    [[nodiscard]] weather::WeatherRenderer& weatherRenderer();
    [[nodiscard]] const weather::WeatherRenderer& weatherRenderer() const;
    [[nodiscard]] bool isWeatherRendererInitialized() const { return m_weatherRendererInitialized; }

    /**
     * @brief 获取光照贴图管理器
     *
     * 维护 16×16 光照贴图纹理，每帧按天气/时间/维度参数重建，供天气渲染器采样。
     */
    [[nodiscard]] light::LightTextureManager& lightTextureManager();
    [[nodiscard]] const light::LightTextureManager& lightTextureManager() const;
    [[nodiscard]] bool isLightTextureManagerInitialized() const { return m_lightTextureManagerInitialized; }

    /**
     * @brief 获取破坏进度渲染器
     */
    [[nodiscard]] block::BreakProgressRenderer& breakProgressRenderer();
    [[nodiscard]] const block::BreakProgressRenderer& breakProgressRenderer() const;
    [[nodiscard]] bool isBreakProgressRendererInitialized() const { return m_breakProgressRendererInitialized; }

    /**
     * @brief 获取第一人称手部渲染器
     */
    [[nodiscard]] firstperson::FirstPersonRenderer& firstPersonRenderer();
    [[nodiscard]] const firstperson::FirstPersonRenderer& firstPersonRenderer() const;
    [[nodiscard]] bool isFirstPersonRendererInitialized() const { return m_firstPersonRendererInitialized; }

    /**
     * @brief 获取视锥体（用于视锥剔除）
     *
     * 视锥体在每帧 beginFrame 时根据相机状态更新。
     */
    [[nodiscard]] const mc::math::frustum::Frustum& frustum() const { return m_frustum; }

    // ========================================================================
    // 子渲染器初始化
    // ========================================================================

    /**
     * @brief 初始化区块渲染器
     */
    [[nodiscard]] Result<void> initializeChunkRenderer();

    /**
     * @brief 初始化天空渲染器
     */
    [[nodiscard]] Result<void> initializeSkyRenderer();

    /**
     * @brief 初始化 GUI 渲染器
     */
    [[nodiscard]] Result<void> initializeGuiRenderer();

    /**
     * @brief 初始化物品渲染器
     */
    [[nodiscard]] Result<void> initializeItemRenderer(ResourceManager* resourceManager);

    /**
     * @brief 初始化实体渲染器
     */
    [[nodiscard]] Result<void> initializeEntityRenderer();

    /**
     * @brief 初始化实体纹理图集
     */
    [[nodiscard]] Result<void> initializeEntityTextureAtlas(ResourceManager* resourceManager);

    /**
     * @brief 初始化统一图集管理器（数据驱动）
     *
     * 用 ResourceManager 的资源包加载 blocks/items 等图集，并把 blocks atlas
     * 的 GPU 句柄绑定到 chunk 渲染管线及依赖方（实体手持方块/第一人称等）。
     */
    [[nodiscard]] Result<void> initializeAtlasManager(ResourceManager* resourceManager);

    /**
     * @brief 重载统一图集管理器（资源包热重载时调用）
     *
     * 重新设置资源包引用并重建 blocks/items 图集，随后重新绑定 chunk 管线纹理。
     */
    [[nodiscard]] Result<void> reloadAtlasManager(ResourceManager* resourceManager);

    /**
     * @brief 获取 blocks atlas 的纹理区域查询回调
     *
     * 返回绑定到 AtlasManager::findSpriteByTexturePath 的回调，接受
     * `minecraft:textures/block/stone` 这类完整纹理路径，返回其在 blocks atlas
     * 中的 UV 区域；未找到返回 nullptr。
     *
     * 用于 ResourceManager::computeBlockAppearances 与 BlockModelCache 的液体面
     * 纹理查询——避免在 RM/BlockModelCache 头引入 AtlasManager 命名空间。
     *
     * AtlasManager 未初始化或 blocks atlas 未加载时返回空回调（查询恒返回 nullptr）。
     */
    [[nodiscard]] std::function<const TextureRegion*(const ResourceLocation&)> blockTextureRegionLookup() const;

    /**
     * @brief 初始化雾效果管理器
     */
    [[nodiscard]] Result<void> initializeFogManager();

    /**
     * @brief 初始化云渲染器
     */
    [[nodiscard]] Result<void> initializeCloudRenderer(ResourceManager* resourceManager);

    /**
     * @brief 初始化粒子管理器
     */
    [[nodiscard]] Result<void> initializeParticleManager();

    /**
     * @brief 初始化天气渲染器
     */
    [[nodiscard]] Result<void> initializeWeatherRenderer(ResourceManager* resourceManager);

    /**
     * @brief 初始化光照贴图管理器
     *
     * 创建 16×16 光照贴图纹理资源并完成首次上传。成功后会把图像视图与采样器
     * 注入天气渲染器（setLightmap），使其改用 lightmap 采样而非标量光照回退。
     */
    [[nodiscard]] Result<void> initializeLightTextureManager();

    /**
     * @brief 初始化破坏进度渲染器
     *
     * 纹理（blocks atlas 的 destroy_stage_N sprite）与阶段 UV 区域查询由
     * `_bindBlockAtlasToChunkPipeline` 在 AtlasManager 加载 blocks atlas 后注入。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initializeBreakProgressRenderer();

    /**
     * @brief 初始化第一人称手部渲染器
     */
    [[nodiscard]] Result<void> initializeFirstPersonRenderer();

    /**
     * @brief 重新加载云纹理
     *
     * @param resourceManager 资源管理器（允许为空；为空时回退程序化纹理）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> reloadCloudTexture(ResourceManager* resourceManager);

    /**
     * @brief 重新加载火焰纹理
     *
     * 在资源热重载后重新从资源包加载 fire_0.png / fire_1.png。
     *
     * @param resourceManager 资源管理器（允许为空；为空时回退程序化纹理）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> reloadFireTexture(ResourceManager* resourceManager);

    /**
     * @brief 每游戏tick更新动画纹理状态
     *
     * 遍历所有已注册的动画精灵，推进帧计数器。
     * 应在客户端固定 tick 循环中调用。
     */
    void tickTextureAnimations();

    /**
     * @brief 上传所有待更新的动画帧到 GPU
     *
     * @param cmd 非 null 时走异步热路径：copy 录进该帧命令缓冲，随帧 submit，
     *            用帧 fence 同步，不阻塞 CPU（必须在 beginFrame 之后、渲染通道开始前调用，
     *            因为 vkCmdCopyBufferToImage 不能在 render pass 内录制）。
     *            为 null 时回退同步路径：独立 submit+wait。
     * @param frameIndex 当前帧索引（仅异步路径使用，用于 staging 回收桶登记）
     */
    void uploadAnimationFrames(VkCommandBuffer cmd, u32 frameIndex);

    /**
     * @brief 设置 GUI 缩放倍率
     *
     * GUI 渲染会使用该倍率把窗口尺寸换算为逻辑 GUI 尺寸。
     * 这会影响 HUD、容器界面和所有基于 GUIRenderer 的文本测量。
     *
     * @param scaleFactor GUI 缩放倍率，必须大于 0
     */
    void setGuiScaleFactor(f64 scaleFactor);

    /**
     * @brief 获取 GUI 缩放倍率
     */
    [[nodiscard]] f64 guiScaleFactor() const { return m_guiScaleFactor; }

private:
    // 核心组件
    std::unique_ptr<TridentContext> m_context;
    std::unique_ptr<TridentSwapchain> m_swapchain;
    std::unique_ptr<RenderPassManager> m_renderPassManager;
    std::unique_ptr<FrameManager> m_frameManager;
    std::unique_ptr<DescriptorManager> m_descriptorManager;
    std::unique_ptr<UniformManager> m_uniformManager;
    std::unique_ptr<TridentPipeline> m_chunkPipeline;
    std::unique_ptr<TridentPipeline> m_chunkTranslucentPipeline;

    // 区块纹理描述符集（set = 1）
    VkDescriptorSet m_chunkTextureDescriptorSet = VK_NULL_HANDLE;

    // 配置
    api::RenderEngineConfig m_config;
    TridentConfig m_tridentConfig;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    // 帧上下文
    api::FrameContext m_frameContext;

    // 当前渲染类型
    api::RenderType m_currentRenderType;

    // 时间状态（用于天空/光照）
    i64 m_dayTime = 0;
    i64 m_gameTime = 0;
    f64 m_partialTick = 0.0f;

    // 天气状态
    f64 m_rainStrength = 0.0f;
    f64 m_thunderStrength = 0.0f;
    f64 m_lightningFlashBrightness = 0.0f; ///< 闪电闪烁亮度（0..1，驱动光照贴图增亮）

    // 维度环境光（主世界 0.0，下界 0.2，末地 0.0），供光照贴图下限
    f64 m_ambientLight = 0.0f;

    // 可配置渲染参数（由 options.json 驱动）
    i32 m_renderDistanceChunks = 12;
    f64 m_landFogDensity = 1.0f;
    cloud::CloudMode m_cloudMode = cloud::CloudMode::Fancy;

    // 维度相关渲染参数
    f64 m_cloudHeight = 192.0; ///< 云高度（默认主世界值）
    bool m_hasClouds = true;   ///< 是否有云（下界/末地为 false）

    // 窗口尺寸
    u32 m_windowWidth = 0;
    u32 m_windowHeight = 0;
    f64 m_guiScaleFactor = 1.0f;

    // 状态
    bool m_initialized = false;
    bool m_minimized = false;
    bool m_frameStarted = false;

    // 渲染回调
    GuiRenderCallback m_guiRenderCallback;
    EntityRenderCallback m_entityRenderCallback;
    FirstPersonRenderCallback m_firstPersonRenderCallback;

    // 子渲染器
    std::unique_ptr<ChunkRenderer> m_chunkRenderer;

    // 统一暂存缓冲池（OffsetAllocator 子分配），由所有上传路径复用
    std::unique_ptr<api::IStagingBufferPool> m_stagingPool;
    std::unique_ptr<sky::SkyRenderer> m_skyRendererPtr;
    std::unique_ptr<gui::GuiRenderer> m_guiRendererPtr;
    std::unique_ptr<item::ItemRenderer> m_itemRendererPtr;
    std::unique_ptr<entity::EntityRendererManager> m_entityRendererManager;
    std::unique_ptr<fog::FogManager> m_fogManager;
    std::unique_ptr<cloud::CloudRenderer> m_cloudRenderer;
    std::unique_ptr<particle::ParticleManager> m_particleManager;
    std::unique_ptr<weather::WeatherRenderer> m_weatherRenderer;
    std::unique_ptr<light::LightTextureManager> m_lightTextureManager;
    std::unique_ptr<block::BreakProgressRenderer> m_breakProgressRenderer;
    std::unique_ptr<firstperson::FirstPersonRenderer> m_firstPersonRenderer;

    // 客户端世界（由外部注入，供雾距/lightmap 等子系统查询相机 skyLight/biome，可为空）
    mc::client::ClientWorld* m_clientWorld = nullptr;

    // 实体渲染管线（独立于区块管线）
    std::unique_ptr<EntityPipeline> m_entityPipeline;

    // 字体
    std::unique_ptr<Font> m_font;

    // 纹理图集
    ItemTextureAtlas m_itemTextureAtlas;
    EntityTextureAtlas m_entityTextureAtlas;
    ResourceLocation m_localPlayerSkinLocation{"minecraft:textures/entity/player/slim/steve.png"};

    // 统一图集管理器（数据驱动）
    // 用 PIMPL（AtlasManagerOwner）隐藏 resource::atlas::AtlasManager，避免本头引入
    // mc::client::resource::atlas 命名空间（会破坏 FireAnimationState.hpp 中
    // resource::metadata 的父作用域查找）
    std::unique_ptr<AtlasManagerOwner> m_atlasManager;
    bool m_atlasManagerInitialized = false;

    // 子渲染器初始化状态
    bool m_chunkRendererInitialized = false;
    bool m_skyRendererInitialized = false;
    bool m_guiRendererInitialized = false;
    bool m_itemRendererInitialized = false;
    bool m_itemTextureAtlasInitialized = false;
    bool m_entityRendererInitialized = false;
    bool m_entityTextureAtlasInitialized = false;
    bool m_fogManagerInitialized = false;
    bool m_cloudRendererInitialized = false;
    bool m_particleManagerInitialized = false;
    bool m_weatherRendererInitialized = false;
    bool m_lightTextureManagerInitialized = false;
    bool m_breakProgressRendererInitialized = false;
    bool m_firstPersonRendererInitialized = false;

    // 液体状态（用于雾效果）
    bool m_inWater = false;
    bool m_inLava = false;
    u32 m_waterFogColor = 0x050533; // 默认水下雾颜色

    // 视锥体（用于视锥剔除）
    mc::math::frustum::Frustum m_frustum;

    // 内部方法
    [[nodiscard]] Result<void> _recreateSwapchain();

    /**
     * @brief 在当前帧命令缓冲上开始渲染通道并设置视口/裁剪
     *
     * 从 beginFrame() 拆出：beginFrame() 只到 acquireNextImage + 回收 staging +
     * 开始命令缓冲录制（不进入 render pass），把 render pass 的开始延后到这里。
     * 这样调用方可在 beginFrame() 与 _beginRenderPass() 之间向帧命令缓冲录制
     * vkCmdCopyBufferToImage 等不能在 render pass 内执行的 transfer 命令
     * （如异步动画帧上传）。
     */
    void _beginRenderPass();

    /**
     * @brief 把 AtlasManager 拥有的 blocks atlas 的 imageView/sampler 写入
     *        chunk 纹理描述符集，并下发给依赖 blocks atlas 的子渲染器
     *        （实体手持方块、第一人称手持方块、破坏叠加）。
     *
     * 在 initializeAtlasManager / reloadAtlasManager 加载完 blocks atlas 后调用。
     * 若 blocks atlas 尚未加载则跳过。
     */
    void _bindBlockAtlasToChunkPipeline();
};

} // namespace mc::client::renderer::trident
