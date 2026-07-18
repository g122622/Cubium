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

#include "client/renderer/trident/core/TridentEngine.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/api/IRenderEngine.hpp"
#include "client/renderer/trident/block/BreakProgressRenderer.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/cloud/CloudRenderer.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/core/TridentSwapchain.hpp"
#include "client/renderer/trident/core/buffer/TridentBuffer.hpp"
#include "client/renderer/trident/core/pipeline/TridentPipeline.hpp"
#include "client/renderer/trident/core/render/DescriptorManager.hpp"
#include "client/renderer/trident/core/render/FrameManager.hpp"
#include "client/renderer/trident/core/render/RenderPassManager.hpp"
#include "client/renderer/trident/core/render/UniformManager.hpp"
#include "client/renderer/trident/core/texture/TridentTexture.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/effect/fire/FireEffect.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/renderer/trident/entity/util/WorldTextRenderer.hpp"
#include "client/renderer/trident/firstperson/FirstPersonRenderer.hpp"
#include "client/renderer/trident/fog/FogManager.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "client/renderer/trident/particle/ParticleManager.hpp"
#include "client/renderer/trident/particle/particles/block/ItemParticle.hpp"
#include "client/renderer/trident/sky/SkyRenderer.hpp"
#include "client/renderer/trident/weather/WeatherRenderer.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include "client/resource/EntityTextureLoader.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/ui/DefaultAsciiFont.hpp"
#include "client/ui/Font.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

using namespace mc::trace;

namespace mc::client::renderer::trident {

namespace {

/**
 * @brief 区块推送常量
 */
struct ChunkPushConstants {
    glm::mat4 model;
    glm::vec4 chunkRelativeOffset;
};

static_assert(sizeof(ChunkPushConstants) == 80, "ChunkPushConstants layout must match chunk.vert push-constant block");
static_assert(offsetof(ChunkPushConstants, chunkRelativeOffset) == 64, "ChunkPushConstants offset mismatch");

[[nodiscard]] u32 sampleCountToValue(VkSampleCountFlagBits sampleCount)
{
    switch (sampleCount) {
        case VK_SAMPLE_COUNT_64_BIT:
            return 64;
        case VK_SAMPLE_COUNT_32_BIT:
            return 32;
        case VK_SAMPLE_COUNT_16_BIT:
            return 16;
        case VK_SAMPLE_COUNT_8_BIT:
            return 8;
        case VK_SAMPLE_COUNT_4_BIT:
            return 4;
        case VK_SAMPLE_COUNT_2_BIT:
            return 2;
        default:
            return 1;
    }
}

[[nodiscard]] VkSampleCountFlagBits selectSampleCount(u32 requestedSamples, VkSampleCountFlagBits supportedSamples)
{
    const std::array<VkSampleCountFlagBits, 7> sampleOrder = {VK_SAMPLE_COUNT_64_BIT,
        VK_SAMPLE_COUNT_32_BIT,
        VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_2_BIT,
        VK_SAMPLE_COUNT_1_BIT};

    for (VkSampleCountFlagBits sampleCount : sampleOrder) {
        if (requestedSamples >= sampleCountToValue(sampleCount) && (supportedSamples & sampleCount) == sampleCount) {
            return sampleCount;
        }
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

/**
 * @brief 将 TridentTextureAtlas 适配为 ITextureAtlas 接口
 */
class TridentTextureAtlasAdapter final : public api::ITextureAtlas {
public:
    [[nodiscard]] Result<void> initialize(TridentContext* context, u32 width, u32 height, u32 tileSize)
    {
        return m_atlas.create(context, width, height, tileSize);
    }

    [[nodiscard]] u32 width() const override { return m_atlas.width(); }
    [[nodiscard]] u32 height() const override { return m_atlas.height(); }
    [[nodiscard]] u32 tileSize() const override { return m_atlas.tileSize(); }
    [[nodiscard]] u32 tilesPerRow() const override { return m_atlas.tilesPerRow(); }

    [[nodiscard]] api::TextureRegion getRegion(u32 tileX, u32 tileY) const override
    {
        return m_atlas.getRegion(tileX, tileY);
    }

    [[nodiscard]] api::TextureRegion getRegion(u32 tileIndex) const override { return m_atlas.getRegion(tileIndex); }

    [[nodiscard]] api::ITexture* texture() override { return &m_atlas.texture(); }
    [[nodiscard]] const api::ITexture* texture() const override { return &m_atlas.texture(); }

    [[nodiscard]] bool isValid() const override { return m_atlas.isValid(); }

private:
    TridentTextureAtlas m_atlas;
};

} // anonymous namespace

// ============================================================================
// 构造/析构
// ============================================================================

TridentEngine::TridentEngine() = default;

TridentEngine::~TridentEngine()
{
    destroy();
}

// ============================================================================
// IRenderEngine 接口实现 - 生命周期
// ============================================================================

Result<void> TridentEngine::initialize(void* window, const api::RenderEngineConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentEngine::initialize");

    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "TridentEngine already initialized");
    }
    if (!window) {
        return Error(ErrorCode::NullPointer, "Window pointer is null");
    }

    m_config = config;
    m_windowWidth = config.initialWindowWidth;
    m_windowHeight = config.initialWindowHeight;

    // 转换配置
    m_tridentConfig.appName = config.appName;
    m_tridentConfig.enableValidation = config.enableValidation;
    m_tridentConfig.enableVSync = config.enableVSync;
    m_tridentConfig.maxFramesInFlight = config.maxFramesInFlight;

    // 1. 创建 Vulkan 上下文
    m_context = std::make_unique<TridentContext>();
    auto contextResult = m_context->initialize(static_cast<GLFWwindow*>(window), m_tridentConfig);
    if (contextResult.failed()) {
        m_context.reset();
        return contextResult.error();
    }

    const VkSampleCountFlagBits supportedSampleCount = m_context->maxUsableSampleCount();
    const u32 requestedSampleCount = config.enableAntiAliasing ? std::max<u32>(1u, config.msaaSamples) : 1u;
    m_msaaSamples = selectSampleCount(requestedSampleCount, supportedSampleCount);
    m_config.msaaSamples = sampleCountToValue(m_msaaSamples);

    if (config.enableAntiAliasing && m_config.msaaSamples != requestedSampleCount) {
        spdlog::warn("Requested MSAA x{} is not fully supported by the device, falling back to x{}",
            requestedSampleCount,
            m_config.msaaSamples);
    }

    // 2. 创建交换链
    m_swapchain = std::make_unique<TridentSwapchain>();
    SwapChainConfig swapchainConfig{};
    swapchainConfig.width = config.initialWindowWidth;
    swapchainConfig.height = config.initialWindowHeight;
    swapchainConfig.vsync = config.enableVSync;

    auto swapchainResult = m_swapchain->initialize(m_context.get(), swapchainConfig);
    if (swapchainResult.failed()) {
        m_context->destroy();
        m_context.reset();
        m_swapchain.reset();
        return swapchainResult.error();
    }

    // 3. 创建渲染通道管理器
    m_renderPassManager = std::make_unique<RenderPassManager>();
    auto renderPassResult = m_renderPassManager->initialize(m_context.get(), m_swapchain.get(), m_msaaSamples);
    if (renderPassResult.failed()) {
        m_swapchain->destroy();
        m_context->destroy();
        m_swapchain.reset();
        m_context.reset();
        m_renderPassManager.reset();
        return renderPassResult.error();
    }

    // 4. 创建帧管理器
    m_frameManager = std::make_unique<FrameManager>();
    auto frameResult = m_frameManager->initialize(m_context.get(), config.maxFramesInFlight);
    if (frameResult.failed()) {
        m_renderPassManager->destroy();
        m_swapchain->destroy();
        m_context->destroy();
        m_renderPassManager.reset();
        m_swapchain.reset();
        m_context.reset();
        m_frameManager.reset();
        return frameResult.error();
    }

    // 5. 创建描述符管理器
    m_descriptorManager = std::make_unique<DescriptorManager>();
    auto descriptorResult = m_descriptorManager->initialize(m_context.get(), config.maxFramesInFlight);
    if (descriptorResult.failed()) {
        m_frameManager->destroy();
        m_renderPassManager->destroy();
        m_swapchain->destroy();
        m_context->destroy();
        m_frameManager.reset();
        m_renderPassManager.reset();
        m_swapchain.reset();
        m_context.reset();
        m_descriptorManager.reset();
        return descriptorResult.error();
    }

    // 6. 创建 Uniform 管理器
    m_uniformManager = std::make_unique<UniformManager>();
    auto uniformResult =
        m_uniformManager->initialize(m_context.get(), m_descriptorManager.get(), config.maxFramesInFlight);
    if (uniformResult.failed()) {
        m_descriptorManager->destroy();
        m_frameManager->destroy();
        m_renderPassManager->destroy();
        m_swapchain->destroy();
        m_context->destroy();
        m_descriptorManager.reset();
        m_frameManager.reset();
        m_renderPassManager.reset();
        m_swapchain.reset();
        m_context.reset();
        m_uniformManager.reset();
        return uniformResult.error();
    }

    // 初始化帧上下文
    m_frameContext = api::FrameContext{};

    m_initialized = true;
    spdlog::info("Renderer AA config: enabled={}, samples={}", config.enableAntiAliasing, m_config.msaaSamples);
    spdlog::info("TridentEngine initialized successfully");
    return {};
}

void TridentEngine::destroy()
{
    if (!m_initialized) return;

    // 等待设备空闲
    if (m_context) {
        m_context->waitIdle();
    }

    // 先销毁依赖 Vulkan 设备的子渲染器与纹理资源
    if (m_chunkRenderer) {
        m_chunkRenderer->destroy();
        m_chunkRenderer.reset();
    }

    if (m_skyRendererPtr) {
        m_skyRendererPtr->destroy();
        m_skyRendererPtr.reset();
    }

    if (m_guiRendererPtr) {
        m_guiRendererPtr->destroy();
        m_guiRendererPtr.reset();
    }

    if (m_firstPersonRenderer) {
        m_firstPersonRenderer->destroy();
        m_firstPersonRenderer.reset();
    }

    // 销毁实体管线（必须在 EntityRendererManager 之前）
    if (m_entityPipeline) {
        m_entityPipeline->destroy();
        m_entityPipeline.reset();
    }

    // 销毁雾管理器
    if (m_fogManager) {
        m_fogManager->destroy();
        m_fogManager.reset();
    }

    // 销毁云渲染器
    if (m_cloudRenderer) {
        m_cloudRenderer->destroy();
        m_cloudRenderer.reset();
    }

    // 销毁天气渲染器
    if (m_weatherRenderer) {
        m_weatherRenderer->destroy();
        m_weatherRenderer.reset();
    }

    // 销毁破坏进度渲染器
    if (m_breakProgressRenderer) {
        m_breakProgressRenderer->cleanup();
        m_breakProgressRenderer.reset();
    }

    // 销毁粒子管理器
    if (m_particleManager) {
        m_particleManager->destroy();
        m_particleManager.reset();
    }

    m_itemRendererPtr.reset();

    // 清理世界文本渲染器
    entity::util::WorldTextRenderer::cleanup();

    // 清理火焰效果渲染器
    entity::effect::fire::FireEffect::cleanup();

    m_entityRendererManager.reset();
    m_font.reset();

    // 清除 ItemMeshBuilder 对 ItemTextureAtlas 的静态引用，防止悬垂指针
    ::mc::client::renderer::entity::item::ItemMeshBuilder::setItemTextureAtlas(nullptr);

    // 清除 ItemParticle 对 ItemTextureAtlas 的静态引用，防止悬垂指针
    ::mc::client::renderer::trident::particle::particles::ItemParticle::setItemTextureAtlas(nullptr);

    m_itemTextureAtlas.destroy();
    m_entityTextureAtlas.destroy();
    m_textureRegions.clear();

    m_chunkRendererInitialized = false;
    m_skyRendererInitialized = false;
    m_guiRendererInitialized = false;
    m_itemRendererInitialized = false;
    m_itemTextureAtlasInitialized = false;
    m_entityRendererInitialized = false;
    m_entityTextureAtlasInitialized = false;
    m_fogManagerInitialized = false;
    m_cloudRendererInitialized = false;
    m_particleManagerInitialized = false;
    m_weatherRendererInitialized = false;
    m_breakProgressRendererInitialized = false;
    m_firstPersonRendererInitialized = false;

    m_guiRenderCallback = nullptr;
    m_entityRenderCallback = nullptr;
    m_firstPersonRenderCallback = nullptr;

    // 按相反顺序销毁
    m_uniformManager.reset();
    m_descriptorManager.reset();
    m_frameManager.reset();
    m_renderPassManager.reset();
    m_swapchain.reset();
    m_chunkPipeline.reset();
    m_chunkTranslucentPipeline.reset();
    m_chunkTextureDescriptorSet = VK_NULL_HANDLE;

    if (m_context) {
        m_context->destroy();
        m_context.reset();
    }

    m_initialized = false;
    spdlog::info("TridentEngine destroyed");
}

// ============================================================================
// IRenderEngine 接口实现 - 帧渲染
// ============================================================================

Result<void> TridentEngine::beginFrame()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    // 检查窗口是否最小化
    if (m_minimized) {
        return Error(ErrorCode::InvalidState, "Window is minimized");
    }

    // 获取下一帧图像
    auto imageResult = m_frameManager->acquireNextImage(m_swapchain.get());
    if (imageResult.failed()) {
        // 需要重建交换链
        if (imageResult.error().code() == ErrorCode::InvalidState) {
            return _recreateSwapchain();
        }
        return imageResult.error();
    }

    m_frameContext.imageIndex = imageResult.value();
    m_frameContext.frameIndex = m_frameManager->currentFrameIndex();

    // 开始帧录制
    m_frameManager->beginFrame();

    // 开始渲染通道
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Command buffer is null");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPassManager->renderPass();
    renderPassInfo.framebuffer = m_renderPassManager->framebuffer(m_frameContext.imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->extent();

    // 清除值
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.2f, 1.0f}}; // 天空蓝
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 设置视口和裁剪
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_swapchain->extent().width);
    viewport.height = static_cast<f32>(m_swapchain->extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->extent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    m_frameStarted = true;
    return {};
}

Result<void> TridentEngine::endFrame()
{
    if (!m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame not started");
    }

    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    vkCmdEndRenderPass(cmd);

    m_frameManager->endFrame();
    m_frameStarted = false;

    return {};
}

Result<void> TridentEngine::present()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    auto result = m_frameManager->submitAndPresent(m_swapchain.get());
    if (result.failed()) {
        // 需要重建交换链
        if (result.error().code() == ErrorCode::InvalidState) {
            return _recreateSwapchain();
        }
        return result.error();
    }

    return {};
}

Result<void> TridentEngine::render()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    // 上传动画纹理帧（在渲染通道之前执行）
    uploadAnimationFrames();

    // GUI 纹理更新必须在渲染通道外执行
    if (m_guiRendererInitialized && m_guiRendererPtr) {
        VkCommandBuffer prepareCmd = m_context->beginSingleTimeCommands();
        if (prepareCmd != VK_NULL_HANDLE) {
            m_guiRendererPtr->prepareFrame(prepareCmd);
            m_context->endSingleTimeCommands(prepareCmd);
        }
    }

    // 1. 开始帧
    auto beginResult = beginFrame();
    if (beginResult.failed()) {
        return beginResult.error();
    }

    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Command buffer is null");
    }

    // 2. 每帧更新相机矩阵与 Uniform
    if (m_frameContext.camera && m_uniformManager) {
        m_frameContext.viewMatrix = m_frameContext.camera->viewMatrix();
        m_frameContext.projectionMatrix = m_frameContext.camera->projectionMatrix();
        m_frameContext.viewProjectionMatrix = m_frameContext.projectionMatrix * m_frameContext.viewMatrix;
        m_uniformManager->updateCamera(
            m_frameContext.viewMatrix, m_frameContext.projectionMatrix, m_frameContext.frameIndex);
    }

    // 3. 渲染天空
    if (m_skyRendererInitialized && m_skyRendererPtr) {
        m_skyRendererPtr->update(m_dayTime, m_gameTime, m_partialTick, m_rainStrength, m_thunderStrength);

        glm::dvec3 cameraPos(0.0);
        glm::dvec3 cameraForward(0.0, 0.0, -1.0);
        if (m_frameContext.camera) {
            cameraPos = m_frameContext.camera->position();
            cameraForward = m_frameContext.camera->forward();
        }

        m_skyRendererPtr->render(cmd,
            m_frameContext.projectionMatrix,
            m_frameContext.viewMatrix,
            glm::vec3(cameraPos),
            glm::vec3(cameraForward),
            m_frameContext.frameIndex);
    }

    // 4. 更新雾效果（在渲染区块之前）
    if (m_fogManagerInitialized && m_fogManager && m_skyRendererInitialized && m_skyRendererPtr) {
        glm::dvec3 cameraPos(0.0);
        if (m_frameContext.camera) {
            cameraPos = m_frameContext.camera->position();
        }

        // 根据液体状态切换雾模式
        if (m_inLava) {
            // 岩浆中的雾效果
            m_fogManager->setInLava();
        } else if (m_inWater) {
            // 水中的雾效果
            m_fogManager->setUnderwater(m_waterFogColor);
        } else {
            // 陆地上的雾效果
            m_fogManager->resetToLand();
            m_fogManager->update(m_renderDistanceChunks,
                m_rainStrength,
                m_thunderStrength,
                m_landFogDensity,
                m_skyRendererPtr->fogColor(),
                glm::vec3(cameraPos));
        }
    }

    // 4.5 渲染云（在天空之后，区块之前）
    // 云渲染需要满足两个条件：
    // 1. 云模式不为 Off（通过 m_cloudMode 控制，在 CloudRenderer::render 中检查）
    // 2. 当前维度有云（m_hasClouds 为 true）
    if (m_cloudRendererInitialized && m_cloudRenderer && m_skyRendererInitialized && m_skyRendererPtr) {
        // 检查当前维度是否有云
        if (m_hasClouds) {
            glm::dvec3 cameraPos(0.0);
            if (m_frameContext.camera) {
                cameraPos = m_frameContext.camera->position();
            }

            m_cloudRenderer->update(m_dayTime,
                m_gameTime,
                m_partialTick,
                m_cloudHeight,
                m_skyRendererPtr->fogColor() // 云颜色使用雾颜色
            );

            m_cloudRenderer->render(cmd,
                m_frameContext.projectionMatrix,
                m_frameContext.viewMatrix,
                glm::vec3(cameraPos),
                m_cloudMode,
                m_frameContext.frameIndex);
        }
    }

    // 5. 渲染区块
    if (m_chunkRendererInitialized && m_chunkRenderer && m_chunkPipeline && m_chunkPipeline->isValid() &&
        m_chunkTextureDescriptorSet != VK_NULL_HANDLE) {

        // 清理延迟销毁队列：在高负载时延长保留窗口，降低旧缓冲区被过早释放的风险。
        m_chunkRenderer->processPendingDestroys(32);

        const VkPipelineLayout chunkLayout = m_chunkPipeline->layout();
        if (chunkLayout == VK_NULL_HANDLE) {
            spdlog::error("Chunk pipeline layout is null, skipping chunk rendering for this frame");
        } else {
            m_chunkPipeline->bind(cmd);

            VkDescriptorSet cameraSet = m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, chunkLayout, 0, 1, &cameraSet, 0, nullptr);

            vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, chunkLayout, 1, 1, &m_chunkTextureDescriptorSet, 0, nullptr);

            // 绑定雾效果描述符集（set = 2）
            if (m_fogManagerInitialized && m_fogManager) {
                VkDescriptorSet fogSet = m_fogManager->descriptorSet(m_frameContext.frameIndex);
                if (fogSet != VK_NULL_HANDLE) {
                    vkCmdBindDescriptorSets(
                        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, chunkLayout, 2, 1, &fogSet, 0, nullptr);
                }
            }

            glm::dvec3 chunkCameraPos(0.0);
            if (m_frameContext.camera) {
                const auto cameraPos = m_frameContext.camera->position();
                chunkCameraPos = glm::dvec3(
                    static_cast<f64>(cameraPos.x), static_cast<f64>(cameraPos.y), static_cast<f64>(cameraPos.z));
            }

            m_chunkRenderer->render(cmd, chunkLayout, [cmd, chunkLayout, chunkCameraPos](const ChunkId& chunkId) {
                ChunkPushConstants pushConstants{};
                pushConstants.model = glm::mat4(1.0f);
                pushConstants.chunkRelativeOffset = glm::vec4(
                    static_cast<f32>(static_cast<f64>(chunkId.x * ::mc::world::CHUNK_WIDTH) - chunkCameraPos.x),
                    static_cast<f32>(-chunkCameraPos.y),
                    static_cast<f32>(static_cast<f64>(chunkId.z * ::mc::world::CHUNK_WIDTH) - chunkCameraPos.z),
                    0.0f);

                vkCmdPushConstants(
                    cmd, chunkLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ChunkPushConstants), &pushConstants);
            });

            // 半透明层（如水）延后渲染：开启混合，关闭深度写入，并按距离排序。
            if (m_chunkTranslucentPipeline && m_chunkTranslucentPipeline->isValid()) {
                const VkPipelineLayout translucentLayout = m_chunkTranslucentPipeline->layout();
                if (translucentLayout == VK_NULL_HANDLE) {
                    spdlog::error("Chunk translucent pipeline layout is null, skipping translucent chunk pass");
                } else {
                    m_chunkTranslucentPipeline->bind(cmd);

                    VkDescriptorSet translucentCameraSet =
                        m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
                    vkCmdBindDescriptorSets(cmd,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        translucentLayout,
                        0,
                        1,
                        &translucentCameraSet,
                        0,
                        nullptr);

                    vkCmdBindDescriptorSets(cmd,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        translucentLayout,
                        1,
                        1,
                        &m_chunkTextureDescriptorSet,
                        0,
                        nullptr);

                    if (m_fogManagerInitialized && m_fogManager) {
                        VkDescriptorSet fogSet = m_fogManager->descriptorSet(m_frameContext.frameIndex);
                        if (fogSet != VK_NULL_HANDLE) {
                            vkCmdBindDescriptorSets(
                                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, translucentLayout, 2, 1, &fogSet, 0, nullptr);
                        }
                    }

                    glm::dvec3 cameraPos(0.0);
                    if (m_frameContext.camera) {
                        cameraPos = m_frameContext.camera->position();
                    }

                    m_chunkRenderer->renderTransparent(
                        cmd,
                        translucentLayout,
                        [cmd, translucentLayout, chunkCameraPos](const ChunkId& chunkId) {
                            ChunkPushConstants pushConstants{};
                            pushConstants.model = glm::mat4(1.0f);
                            pushConstants.chunkRelativeOffset =
                                glm::vec4(static_cast<f32>(static_cast<f64>(chunkId.x * ::mc::world::CHUNK_WIDTH) -
                                              chunkCameraPos.x),
                                    static_cast<f32>(-chunkCameraPos.y),
                                    static_cast<f32>(
                                        static_cast<f64>(chunkId.z * ::mc::world::CHUNK_WIDTH) - chunkCameraPos.z),
                                    0.0f);

                            vkCmdPushConstants(cmd,
                                translucentLayout,
                                VK_SHADER_STAGE_VERTEX_BIT,
                                0,
                                sizeof(ChunkPushConstants),
                                &pushConstants);
                        },
                        cameraPos,
                        true);
                }
            }
        }
    }

    // 5.5 渲染破坏进度覆盖层
    if (m_breakProgressRendererInitialized && m_breakProgressRenderer) {
        // 更新网格数据
        glm::dvec3 cameraPos(0.0);
        if (m_frameContext.camera) {
            cameraPos = m_frameContext.camera->position();
        }
        m_breakProgressRenderer->updateMesh(
            Vector3(static_cast<f32>(cameraPos.x), static_cast<f32>(cameraPos.y), static_cast<f32>(cameraPos.z)));

        // 渲染
        if (m_breakProgressRenderer->hasProgressToRender()) {
            VkDescriptorSet cameraSet = m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
            VkDescriptorSet fogSet = VK_NULL_HANDLE;
            if (m_fogManagerInitialized && m_fogManager) {
                fogSet = m_fogManager->descriptorSet(m_frameContext.frameIndex);
            }
            m_breakProgressRenderer->render(cmd, cameraSet, fogSet);
        }
    }

    // 6. 调用实体渲染回调
    if (m_entityRenderCallback && m_entityRendererInitialized && m_entityRendererManager) {
        // 设置相机描述符集给实体渲染管理器
        VkDescriptorSet cameraSet = m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
        m_entityRendererManager->setCameraDescriptorSet(cameraSet);

        // 推进实体管线帧索引：setTextureAtlas/bindTextureDescriptor 据此选择当前帧私有
        // 纹理描述符集，避免改写仍被在飞帧引用的描述符。
        if (m_entityPipeline && m_entityPipeline->isInitialized()) {
            m_entityPipeline->beginFrame(m_frameContext.frameIndex);
            // beginFrame 已推进帧计数器，且 acquireNextImage 已等待上一帧 fence 完成，
            // 此时释放到期缓冲区是安全的（不会再被任何在飞命令缓冲区引用）。
            m_entityPipeline->processPendingDestroys();
        }

        m_entityRenderCallback(cmd, m_partialTick);
    }

    // 6.5 渲染天气效果（雨/雪）
    if (m_weatherRendererInitialized && m_weatherRenderer && m_rainStrength > 0.01f) {
        glm::dvec3 cameraPos(0.0);
        if (m_frameContext.camera) {
            cameraPos = m_frameContext.camera->position();
        }

        m_weatherRenderer->update(m_rainStrength, m_thunderStrength, m_gameTime, m_partialTick);
        m_weatherRenderer->render(cmd,
            m_frameContext.projectionMatrix,
            m_frameContext.viewMatrix,
            glm::vec3(cameraPos),
            m_frameContext.frameIndex,
            m_frustum);
    }

    // 6.6 渲染粒子
    if (m_particleManagerInitialized && m_particleManager && m_particleManager->particleCount() > 0) {
        glm::dvec3 cameraPos(0.0);
        if (m_frameContext.camera) {
            cameraPos = m_frameContext.camera->position();
        }

        m_particleManager->tick();
        m_particleManager->render(cmd,
            m_frameContext.projectionMatrix,
            m_frameContext.viewMatrix,
            glm::vec3(cameraPos),
            m_frameContext.frameIndex,
            m_frustum);
    }

    // 6.7 渲染第一人称手部
    if (m_firstPersonRendererInitialized && m_firstPersonRenderer && m_firstPersonRenderCallback) {
        VkDescriptorSet cameraSet = m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
        // 推进第一人称管线帧索引（per-frame 纹理描述符集 + 延迟销毁窗口）。
        // acquireNextImage 已等待上一帧 fence，此时释放到期缓冲区安全。
        m_firstPersonRenderer->beginFrame(m_frameContext.frameIndex);
        m_firstPersonRenderer->processPendingDestroys();
        m_firstPersonRenderCallback(cmd, cameraSet, m_partialTick);
    }

    // 7. 渲染 GUI
    if (m_guiRendererInitialized && m_guiRendererPtr) {
        const f64 guiScale = std::max(m_guiScaleFactor, 1.0);
        m_guiRendererPtr->setFontScale(guiScale);
        m_guiRendererPtr->beginFrame(
            static_cast<f64>(m_windowWidth) / guiScale, static_cast<f64>(m_windowHeight) / guiScale);
        if (m_guiRenderCallback) {
            m_guiRenderCallback();
        }
        m_guiRendererPtr->render(cmd);
    }

    // 8. 结束帧
    auto endResult = endFrame();
    if (endResult.failed()) {
        return endResult.error();
    }

    // 8. 呈现
    return present();
}

// ============================================================================
// IRenderEngine 接口实现 - 窗口和相机
// ============================================================================

Result<void> TridentEngine::onResize(u32 width, u32 height)
{
    if (!m_initialized) return {};

    if (width == 0 || height == 0) {
        m_minimized = true;
        return {};
    }

    m_minimized = false;
    m_windowWidth = width;
    m_windowHeight = height;

    return _recreateSwapchain();
}

void TridentEngine::setGuiScaleFactor(f64 scaleFactor)
{
    m_guiScaleFactor = std::max(scaleFactor, 1.0);

    if (m_guiRendererInitialized && m_guiRendererPtr) {
        m_guiRendererPtr->setFontScale(m_guiScaleFactor);
    }
}

void TridentEngine::setCamera(const api::ICamera* camera)
{
    m_frameContext.camera = camera;

    if (camera) {
        m_frameContext.viewMatrix = camera->viewMatrix();
        m_frameContext.projectionMatrix = camera->projectionMatrix();
        m_frameContext.viewProjectionMatrix = m_frameContext.projectionMatrix * m_frameContext.viewMatrix;

        // 更新视锥体
        m_frustum.extractFromMatrix(m_frameContext.viewProjectionMatrix);
        m_frustum.setCameraPosition(camera->position());

        // 更新 Uniform 缓冲区
        m_uniformManager->updateCamera(
            m_frameContext.viewMatrix, m_frameContext.projectionMatrix, m_frameContext.frameIndex);
    }
}

// ============================================================================
// IRenderEngine 接口实现 - 资源创建
// ============================================================================

Result<std::unique_ptr<api::IVertexBuffer>> TridentEngine::createVertexBuffer(u64 size, u32 vertexStride)
{
    auto buffer = std::make_unique<TridentVertexBuffer>();
    auto result = buffer->create(m_context.get(), size, vertexStride);
    if (result.failed()) {
        return result.error();
    }
    std::unique_ptr<api::IVertexBuffer> vertexBuffer = std::move(buffer);
    return std::move(vertexBuffer);
}

Result<std::unique_ptr<api::IIndexBuffer>> TridentEngine::createIndexBuffer(u64 size, api::IndexType type)
{
    auto buffer = std::make_unique<TridentIndexBuffer>();
    auto result = buffer->create(m_context.get(), size, type);
    if (result.failed()) {
        return result.error();
    }
    std::unique_ptr<api::IIndexBuffer> indexBuffer = std::move(buffer);
    return std::move(indexBuffer);
}

Result<std::unique_ptr<api::IUniformBuffer>> TridentEngine::createUniformBuffer(u64 size, u32 frameCount)
{
    auto buffer = std::make_unique<TridentUniformBuffer>();
    auto result = buffer->create(m_context.get(), size, frameCount);
    if (result.failed()) {
        return result.error();
    }
    std::unique_ptr<api::IUniformBuffer> uniformBuffer = std::move(buffer);
    return std::move(uniformBuffer);
}

Result<std::unique_ptr<api::ITexture>> TridentEngine::createTexture(const api::TextureDesc& desc)
{
    auto texture = std::make_unique<TridentTexture>();
    auto result = texture->create(m_context.get(), desc);
    if (result.failed()) {
        return result.error();
    }
    std::unique_ptr<api::ITexture> apiTexture = std::move(texture);
    return std::move(apiTexture);
}

Result<std::unique_ptr<api::ITextureAtlas>> TridentEngine::createTextureAtlas(u32 width, u32 height, u32 tileSize)
{
    auto atlas = std::make_unique<TridentTextureAtlasAdapter>();
    auto result = atlas->initialize(m_context.get(), width, height, tileSize);
    if (result.failed()) {
        return result.error();
    }
    std::unique_ptr<api::ITextureAtlas> apiAtlas = std::move(atlas);
    return std::move(apiAtlas);
}

// ============================================================================
// IRenderEngine 接口实现 - 渲染状态
// ============================================================================

void TridentEngine::setRenderType(const api::RenderType& type)
{
    m_currentRenderType = type;
    // TODO: 实际绑定对应的管线
}

const api::RenderType& TridentEngine::currentRenderType() const
{
    return m_currentRenderType;
}

void TridentEngine::bindTexture(u32 binding, const api::ITexture* texture)
{
    if (texture == nullptr || !texture->isValid()) {
        return;
    }

    if (binding != 0 || m_chunkTextureDescriptorSet == VK_NULL_HANDLE) {
        return;
    }

    const auto* tridentTexture = dynamic_cast<const TridentTexture*>(texture);
    if (tridentTexture == nullptr) {
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = tridentTexture->imageView();
    imageInfo.sampler = tridentTexture->sampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_chunkTextureDescriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device(), 1, &descriptorWrite, 0, nullptr);
}

void TridentEngine::bindUniformBuffer(u32 binding, const api::IUniformBuffer* buffer)
{
    if (buffer == nullptr || !buffer->isValid() || m_uniformManager == nullptr) {
        return;
    }

    if (binding > 1) {
        return;
    }

    VkDescriptorSet cameraSet = m_uniformManager->cameraDescriptorSet(m_frameContext.frameIndex);
    if (cameraSet == VK_NULL_HANDLE) {
        return;
    }

    VkBuffer vkBuffer = reinterpret_cast<VkBuffer>(buffer->nativeHandle());
    if (vkBuffer == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = buffer->size();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = cameraSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device(), 1, &descriptorWrite, 0, nullptr);
}

// ============================================================================
// IRenderEngine 接口实现 - 绘制
// ============================================================================

void TridentEngine::drawIndexed(u32 indexCount, u32 firstIndex, i32 vertexOffset)
{
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, vertexOffset, 0);
}

void TridentEngine::draw(u32 vertexCount, u32 firstVertex)
{
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    vkCmdDraw(cmd, vertexCount, 1, firstVertex, 0);
}

void TridentEngine::drawIndexedInstanced(
    u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
{
    VkCommandBuffer cmd = m_frameManager->currentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) return;

    vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

// ============================================================================
// IRenderEngine 接口实现 - 状态查询
// ============================================================================

bool TridentEngine::isInitialized() const
{
    return m_initialized;
}

u32 TridentEngine::currentFrameIndex() const
{
    return m_frameManager ? m_frameManager->currentFrameIndex() : 0;
}

u32 TridentEngine::currentImageIndex() const
{
    return m_frameManager ? m_frameManager->currentImageIndex() : 0;
}

const api::FrameContext& TridentEngine::frameContext() const
{
    return m_frameContext;
}

u32 TridentEngine::maxFramesInFlight() const
{
    return m_tridentConfig.maxFramesInFlight;
}

bool TridentEngine::isMinimized() const
{
    return m_minimized;
}

u32 TridentEngine::windowWidth() const
{
    return m_windowWidth;
}

u32 TridentEngine::windowHeight() const
{
    return m_windowHeight;
}

const api::ICamera* TridentEngine::camera() const
{
    return m_frameContext.camera;
}

// ============================================================================
// Trident 特有接口
// ============================================================================

VkRenderPass TridentEngine::renderPass() const
{
    return m_renderPassManager ? m_renderPassManager->renderPass() : VK_NULL_HANDLE;
}

VkCommandBuffer TridentEngine::currentCommandBuffer() const
{
    return m_frameManager ? m_frameManager->currentCommandBuffer() : VK_NULL_HANDLE;
}

VkPipelineLayout TridentEngine::pipelineLayout() const
{
    return m_descriptorManager ? m_descriptorManager->pipelineLayout() : VK_NULL_HANDLE;
}

VkDescriptorPool TridentEngine::descriptorPool() const
{
    return m_descriptorManager ? m_descriptorManager->pool() : VK_NULL_HANDLE;
}

VkDescriptorSetLayout TridentEngine::cameraDescriptorLayout() const
{
    return m_descriptorManager ? m_descriptorManager->cameraLayout() : VK_NULL_HANDLE;
}

VkDescriptorSetLayout TridentEngine::textureDescriptorLayout() const
{
    return m_descriptorManager ? m_descriptorManager->textureLayout() : VK_NULL_HANDLE;
}

VkDescriptorSetLayout TridentEngine::fogDescriptorLayout() const
{
    return m_descriptorManager ? m_descriptorManager->fogLayout() : VK_NULL_HANDLE;
}

VkDescriptorSet TridentEngine::cameraDescriptorSet() const
{
    if (!m_uniformManager || !m_frameManager) return VK_NULL_HANDLE;
    return m_uniformManager->cameraDescriptorSet(m_frameManager->currentFrameIndex());
}

void TridentEngine::updateTime(i64 dayTime, i64 gameTime, f64 partialTick)
{
    m_dayTime = dayTime;
    m_gameTime = gameTime;
    m_partialTick = partialTick;

    if (m_uniformManager) {
        m_uniformManager->updateLighting(dayTime, gameTime, partialTick);
    }
}

void TridentEngine::updateWeather(f64 rainStrength, f64 thunderStrength)
{
    m_rainStrength = rainStrength;
    m_thunderStrength = thunderStrength;
}

void TridentEngine::setLightningFlashBrightness(f64 brightness)
{
    if (m_skyRendererInitialized && m_skyRendererPtr) {
        m_skyRendererPtr->setLightningFlashBrightness(brightness);
    }
}

Result<void> TridentEngine::setVSyncEnabled(bool enabled)
{
    if (m_config.enableVSync == enabled) {
        return Result<void>::ok();
    }

    m_config.enableVSync = enabled;
    m_tridentConfig.enableVSync = enabled;

    if (m_swapchain) {
        m_swapchain->setVSync(enabled);
    }

    if (m_initialized && !m_minimized) {
        spdlog::info("Applying VSync change: {}", enabled);
        return _recreateSwapchain();
    }

    return Result<void>::ok();
}

void TridentEngine::setRenderDistanceChunks(i32 renderDistanceChunks)
{
    m_renderDistanceChunks = std::max<i32>(2, renderDistanceChunks);
}

void TridentEngine::setLandFogDensity(f64 fogDensity)
{
    m_landFogDensity = std::clamp(fogDensity, 0.0, 2.0);
}

void TridentEngine::setCloudMode(cloud::CloudMode mode)
{
    m_cloudMode = mode;
    if (m_cloudRenderer) {
        m_cloudRenderer->setCloudMode(mode);
    }
}

void TridentEngine::setCloudHeight(f64 cloudHeight, bool hasClouds)
{
    m_cloudHeight = cloudHeight;
    m_hasClouds = hasClouds;
}

void TridentEngine::updateLiquidState(bool inWater, bool inLava, u32 waterFogColor)
{
    m_inWater = inWater;
    m_inLava = inLava;
    m_waterFogColor = waterFogColor;
}

VkCommandPool TridentEngine::commandPool() const
{
    return m_frameManager ? m_frameManager->commandPool() : VK_NULL_HANDLE;
}

VkCommandBuffer TridentEngine::beginSingleTimeCommands() const
{
    return m_context ? m_context->beginSingleTimeCommands() : VK_NULL_HANDLE;
}

void TridentEngine::endSingleTimeCommands(VkCommandBuffer cmd) const
{
    if (m_context && cmd != VK_NULL_HANDLE) {
        m_context->endSingleTimeCommands(cmd);
    }
}

// ============================================================================
// 兼容性接口
// ============================================================================

VkDevice TridentEngine::device() const
{
    return m_context ? m_context->device() : VK_NULL_HANDLE;
}

VkPhysicalDevice TridentEngine::physicalDevice() const
{
    return m_context ? m_context->physicalDevice() : VK_NULL_HANDLE;
}

VkQueue TridentEngine::graphicsQueue() const
{
    return m_context ? m_context->graphicsQueue() : VK_NULL_HANDLE;
}

VkImageView TridentEngine::swapchainImageView(u32 index) const
{
    return m_swapchain ? m_swapchain->imageView(index) : VK_NULL_HANDLE;
}

u32 TridentEngine::swapchainImageCount() const
{
    return m_swapchain ? m_swapchain->imageCount() : 0;
}

VkFormat TridentEngine::swapchainFormat() const
{
    return m_swapchain ? m_swapchain->format() : VK_FORMAT_UNDEFINED;
}

VkExtent2D TridentEngine::swapchainExtent() const
{
    return m_swapchain ? m_swapchain->extent() : VkExtent2D{0, 0};
}

VkImageView TridentEngine::depthImageView() const
{
    return m_renderPassManager ? m_renderPassManager->depthImageView() : VK_NULL_HANDLE;
}

// ============================================================================
// 渲染回调
// ============================================================================

void TridentEngine::setGuiRenderCallback(GuiRenderCallback callback)
{
    // 只允许设置一次 GUI 渲染回调，以避免在渲染过程中被替换导致不一致行为
    MC_ASSERT_RELEASE(!m_guiRenderCallback);
    m_guiRenderCallback = std::move(callback);
}

void TridentEngine::setEntityRenderCallback(EntityRenderCallback callback)
{
    // 只允许设置一次实体渲染回调，以避免在渲染过程中被替换导致不一致行为
    MC_ASSERT_RELEASE(!m_entityRenderCallback);
    m_entityRenderCallback = std::move(callback);
}

void TridentEngine::setFirstPersonRenderCallback(FirstPersonRenderCallback callback)
{
    // 只允许设置一次第一人称渲染回调，以避免在渲染过程中被替换导致不一致行为
    MC_ASSERT_RELEASE(!m_firstPersonRenderCallback);
    m_firstPersonRenderCallback = std::move(callback);
}

// ============================================================================
// 私有方法
// ============================================================================

Result<void> TridentEngine::_recreateSwapchain()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    m_context->waitIdle();

    // 重建交换链
    auto swapchainResult = m_swapchain->recreate(m_windowWidth, m_windowHeight);
    if (swapchainResult.failed()) {
        return swapchainResult.error();
    }

    // 重建渲染通道资源
    auto renderPassResult = m_renderPassManager->recreate(m_windowWidth, m_windowHeight);
    if (renderPassResult.failed()) {
        return renderPassResult.error();
    }

    // 重建天空渲染器
    if (m_skyRendererInitialized && m_skyRendererPtr) {
        auto skyResult = m_skyRendererPtr->onResize(VkExtent2D{m_windowWidth, m_windowHeight});
        if (skyResult.failed()) {
            spdlog::warn("Failed to recreate sky renderer: {}", skyResult.error().toString());
        }
    }

    // 重建云渲染器
    if (m_cloudRendererInitialized && m_cloudRenderer) {
        auto cloudResult = m_cloudRenderer->onResize(VkExtent2D{m_windowWidth, m_windowHeight});
        if (cloudResult.failed()) {
            spdlog::warn("Failed to recreate cloud renderer: {}", cloudResult.error().toString());
        }
    }

    // 重建粒子管理器
    if (m_particleManagerInitialized && m_particleManager) {
        auto particleResult = m_particleManager->onResize(VkExtent2D{m_windowWidth, m_windowHeight});
        if (particleResult.failed()) {
            spdlog::warn("Failed to recreate particle manager: {}", particleResult.error().toString());
        }
    }

    // 重建天气渲染器
    if (m_weatherRendererInitialized && m_weatherRenderer) {
        auto weatherResult = m_weatherRenderer->onResize(VkExtent2D{m_windowWidth, m_windowHeight});
        if (weatherResult.failed()) {
            spdlog::warn("Failed to recreate weather renderer: {}", weatherResult.error().toString());
        }
    }

    return {};
}

// ============================================================================
// 子渲染器访问器
// ============================================================================

ChunkRenderer& TridentEngine::chunkRenderer()
{
    if (!m_chunkRenderer) {
        m_chunkRenderer = std::make_unique<ChunkRenderer>();
    }
    return *m_chunkRenderer;
}

const ChunkRenderer& TridentEngine::chunkRenderer() const
{
    return *m_chunkRenderer;
}

sky::SkyRenderer& TridentEngine::skyRenderer()
{
    if (!m_skyRendererPtr) {
        m_skyRendererPtr = std::make_unique<sky::SkyRenderer>();
    }
    return *m_skyRendererPtr;
}

const sky::SkyRenderer& TridentEngine::skyRenderer() const
{
    return *m_skyRendererPtr;
}

gui::GuiRenderer& TridentEngine::guiRenderer()
{
    if (!m_guiRendererPtr) {
        m_guiRendererPtr = std::make_unique<gui::GuiRenderer>();
    }
    return *m_guiRendererPtr;
}

const gui::GuiRenderer& TridentEngine::guiRenderer() const
{
    return *m_guiRendererPtr;
}

Font& TridentEngine::font()
{
    if (!m_font) {
        m_font = std::make_unique<Font>();
    }
    return *m_font;
}

const Font& TridentEngine::font() const
{
    return *m_font;
}

item::ItemRenderer& TridentEngine::itemRenderer()
{
    if (!m_itemRendererPtr) {
        m_itemRendererPtr = std::make_unique<item::ItemRenderer>();
    }
    return *m_itemRendererPtr;
}

const item::ItemRenderer& TridentEngine::itemRenderer() const
{
    return *m_itemRendererPtr;
}

ItemTextureAtlas& TridentEngine::itemTextureAtlas()
{
    return m_itemTextureAtlas;
}

const ItemTextureAtlas& TridentEngine::itemTextureAtlas() const
{
    return m_itemTextureAtlas;
}

entity::EntityRendererManager& TridentEngine::entityRendererManager()
{
    if (!m_entityRendererManager) {
        m_entityRendererManager = std::make_unique<entity::EntityRendererManager>();
    }
    return *m_entityRendererManager;
}

const entity::EntityRendererManager& TridentEngine::entityRendererManager() const
{
    return *m_entityRendererManager;
}

EntityTextureAtlas& TridentEngine::entityTextureAtlas()
{
    return m_entityTextureAtlas;
}

const EntityTextureAtlas& TridentEngine::entityTextureAtlas() const
{
    return m_entityTextureAtlas;
}

fog::FogManager& TridentEngine::fogManager()
{
    if (!m_fogManager) {
        m_fogManager = std::make_unique<fog::FogManager>();
    }
    return *m_fogManager;
}

const fog::FogManager& TridentEngine::fogManager() const
{
    return *m_fogManager;
}

// ============================================================================
// 子渲染器初始化
// ============================================================================

Result<void> TridentEngine::initializeChunkRenderer()
{
    if (m_chunkRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing chunk renderer...");

    if (!m_chunkRenderer) {
        m_chunkRenderer = std::make_unique<ChunkRenderer>();
    }

    auto result = m_chunkRenderer->initialize(device(),
        physicalDevice(),
        commandPool(),
        graphicsQueue(),
        1024 // max chunks
    );

    if (result.failed()) {
        m_chunkRenderer.reset();
        return result.error();
    }

    // 创建区块图形管线
    if (!m_chunkPipeline) {
        m_chunkPipeline = std::make_unique<TridentPipeline>();
    }

    if (!m_chunkTranslucentPipeline) {
        m_chunkTranslucentPipeline = std::make_unique<TridentPipeline>();
    }

    TridentPipelineConfig pipelineConfig{};
    const auto vertPath = resolveShaderPath("chunk.vert.spv");
    const auto fragPath = resolveShaderPath("chunk.frag.spv");
    if (vertPath.empty() || fragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve chunk shader binaries");
    }

    pipelineConfig.vertexShaderPath = vertPath.string();
    pipelineConfig.fragmentShaderPath = fragPath.string();
    pipelineConfig.renderPass = renderPass();
    pipelineConfig.rasterizationSamples = m_msaaSamples;
    pipelineConfig.cullMode = VK_CULL_MODE_NONE;
    pipelineConfig.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineConfig.depthTestEnable = VK_TRUE;
    pipelineConfig.depthWriteEnable = VK_TRUE;
    pipelineConfig.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineConfig.blendEnable = VK_FALSE;

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    pipelineConfig.vertexBindings.push_back(bindingDesc);

    std::array<VkVertexInputAttributeDescription, 4> attrs{};
    attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<u32>(offsetof(Vertex, x))};
    attrs[1] = {4, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<u32>(offsetof(Vertex, u))};
    attrs[2] = {5, 0, VK_FORMAT_R8G8B8A8_UNORM, static_cast<u32>(offsetof(Vertex, color))};
    attrs[3] = {6, 0, VK_FORMAT_R8_UINT, static_cast<u32>(offsetof(Vertex, light))};
    pipelineConfig.vertexAttributes.assign(attrs.begin(), attrs.end());

    pipelineConfig.descriptorSetLayouts = {
        m_descriptorManager->cameraLayout(), m_descriptorManager->textureLayout(), m_descriptorManager->fogLayout()};

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ChunkPushConstants);
    pipelineConfig.pushConstantRanges.push_back(pushConstantRange);

    auto pipelineResult = m_chunkPipeline->create(m_context.get(), pipelineConfig);
    if (pipelineResult.failed()) {
        m_chunkRenderer->destroy();
        m_chunkRenderer.reset();
        m_chunkPipeline.reset();
        m_chunkTranslucentPipeline.reset();
        return pipelineResult.error();
    }

    TridentPipelineConfig translucentConfig = pipelineConfig;
    translucentConfig.depthWriteEnable = VK_FALSE;
    translucentConfig.blendEnable = VK_TRUE;
    auto translucentPipelineResult = m_chunkTranslucentPipeline->create(m_context.get(), translucentConfig);
    if (translucentPipelineResult.failed()) {
        m_chunkRenderer->destroy();
        m_chunkRenderer.reset();
        m_chunkPipeline.reset();
        m_chunkTranslucentPipeline.reset();
        return translucentPipelineResult.error();
    }

    // 预分配纹理描述符集（纹理上传后再写入）
    auto textureSetResult = m_descriptorManager->allocateTextureSet();
    if (textureSetResult.failed()) {
        m_chunkRenderer->destroy();
        m_chunkRenderer.reset();
        m_chunkPipeline.reset();
        m_chunkTranslucentPipeline.reset();
        return textureSetResult.error();
    }
    m_chunkTextureDescriptorSet = textureSetResult.value();

    m_chunkRendererInitialized = true;
    spdlog::info("Chunk renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeSkyRenderer()
{
    if (m_skyRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing sky renderer...");

    if (!m_skyRendererPtr) {
        m_skyRendererPtr = std::make_unique<sky::SkyRenderer>();
    }

    auto result = m_skyRendererPtr->initialize(
        device(), physicalDevice(), commandPool(), graphicsQueue(), renderPass(), swapchainExtent(), m_msaaSamples);

    if (result.failed()) {
        m_skyRendererPtr.reset();
        return result.error();
    }

    m_skyRendererInitialized = true;
    spdlog::info("Sky renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeGuiRenderer()
{
    if (m_guiRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing GUI renderer...");

    if (!m_guiRendererPtr) {
        m_guiRendererPtr = std::make_unique<GuiRenderer>();
    }

    auto result = m_guiRendererPtr->initialize(device(), physicalDevice(), commandPool(), renderPass(), m_msaaSamples);

    if (result.failed()) {
        m_guiRendererPtr.reset();
        return result.error();
    }

    if (!m_font) {
        m_font = std::make_unique<Font>();
        auto fontResult = DefaultAsciiFont::create(*m_font);
        if (fontResult.failed()) {
            m_guiRendererPtr.reset();
            m_font.reset();
            return fontResult.error();
        }
    }
    m_guiRendererPtr->setFont(m_font.get());

    if (m_itemTextureAtlas.isValid()) {
        m_guiRendererPtr->setItemTextureAtlas(m_itemTextureAtlas.imageView(), m_itemTextureAtlas.sampler());
    }

    m_guiRendererInitialized = true;
    spdlog::info("GUI renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeItemRenderer(ResourceManager* resourceManager)
{
    if (m_itemRendererInitialized) {
        return {};
    }

    if (!resourceManager) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    spdlog::info("Initializing item renderer...");

    if (!m_itemTextureAtlasInitialized) {
        if (!m_itemTextureAtlas.isValid()) {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Rendering.Initialization, "TridentEngine::initializeItemRenderer::CreateAtlas");
            auto createResult =
                m_itemTextureAtlas.create(device(), physicalDevice(), commandPool(), graphicsQueue(), 4096, 4096);
            if (createResult.failed()) {
                return createResult.error();
            }
        }

        {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Rendering.Initialization, "TridentEngine::initializeItemRenderer::LoadFromResourcePacks");
            auto loadResult = m_itemTextureAtlas.loadFromResourcePacks(resourceManager->resourcePacks());
            if (loadResult.failed()) {
                return loadResult.error();
            }
        }

        {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Rendering.Initialization, "TridentEngine::initializeItemRenderer::UploadAtlas");
            auto uploadResult = m_itemTextureAtlas.upload();
            if (uploadResult.failed()) {
                return uploadResult.error();
            }
        }

        m_itemTextureAtlasInitialized = true;
        spdlog::info("Item texture atlas initialized: {} mapped items", m_itemTextureAtlas.textureCount());
    }

    // 创建物品渲染器
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentEngine::initializeItemRenderer::CreateItemRenderer");
        m_itemRendererPtr = std::make_unique<item::ItemRenderer>();
        auto result = m_itemRendererPtr->initialize(resourceManager, &m_itemTextureAtlas);
        if (result.failed()) {
            m_itemRendererPtr.reset();
            return result.error();
        }
    }

    // 设置 ItemMeshBuilder 的纹理图集引用，用于解析物品纹理坐标
    ::mc::client::renderer::entity::item::ItemMeshBuilder::setItemTextureAtlas(&m_itemTextureAtlas);

    // 设置 ItemParticle 的纹理图集引用，用于解析非方块物品粒子的纹理坐标
    ::mc::client::renderer::trident::particle::particles::ItemParticle::setItemTextureAtlas(&m_itemTextureAtlas);

    if (m_guiRendererInitialized && m_guiRendererPtr && m_itemTextureAtlas.isValid()) {
        m_guiRendererPtr->setItemTextureAtlas(m_itemTextureAtlas.imageView(), m_itemTextureAtlas.sampler());
    }

    if (m_firstPersonRendererInitialized && m_firstPersonRenderer && m_itemTextureAtlas.isValid()) {
        m_firstPersonRenderer->setItemTextureAtlas(&m_itemTextureAtlas);
    }

    // 第一人称方块物品 3D 渲染需要方块纹理图集（与区块渲染共用同一图集）。
    if (m_firstPersonRendererInitialized && m_firstPersonRenderer && m_chunkRenderer != nullptr) {
        m_firstPersonRenderer->setChunkTextureAtlas(&m_chunkRenderer->textureAtlas());
    }

    m_itemRendererInitialized = true;
    spdlog::info("Item renderer initialized");
    return {};
}

Result<void> TridentEngine::initializeEntityRenderer()
{
    if (m_entityRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing entity renderer...");

    // 创建实体渲染器管理器
    if (!m_entityRendererManager) {
        m_entityRendererManager = std::make_unique<entity::EntityRendererManager>();
    }

    // 初始化默认实体渲染器
    m_entityRendererManager->initializeDefaults();

    // 设置实体纹理图集（用于UV重映射）
    // 注意：物品纹理图集是 ItemTextureAtlas 类型，而 EntityRendererManager 需要 EntityTextureAtlas 类型
    // 物品实体渲染通过 initializeEntityTextureAtlas() 设置的 entity texture atlas 进行
    // 创建并初始化实体渲染管线
    if (!m_entityPipeline) {
        m_entityPipeline = std::make_unique<EntityPipeline>();
    }

    auto pipelineResult = m_entityPipeline->initialize(device(),
        physicalDevice(),
        graphicsQueue(),
        renderPass(),
        cameraDescriptorLayout(),
        descriptorPool(),
        commandPool(),
        m_msaaSamples,
        maxFramesInFlight());

    if (pipelineResult.failed()) {
        spdlog::error("Failed to initialize entity pipeline: {}", pipelineResult.error().toString());
        m_entityPipeline.reset();
        // 继续初始化，管线失败只记录错误不中断
    } else {
        spdlog::info("Entity pipeline initialized");
        // 设置管线到渲染器管理器
        m_entityRendererManager->setPipeline(m_entityPipeline.get());

        // 初始化世界文本渲染器（用于名称标签等）
        if (m_font && m_font->isValid()) {
            bool textRendererInit = entity::util::WorldTextRenderer::initialize(
                device(), physicalDevice(), commandPool(), graphicsQueue(), *m_entityPipeline, m_font.get());
            if (textRendererInit) {
                spdlog::info("WorldTextRenderer initialized");
            } else {
                spdlog::warn("Failed to initialize WorldTextRenderer");
            }
        } else {
            spdlog::warn("WorldTextRenderer not initialized: font not available");
        }

        // 初始化火焰效果渲染器（仅 Vulkan 句柄与程序化占位纹理）
        // 真实火焰纹理在 initializeEntityTextureAtlas 中通过 loadTexture 注入
        bool fireEffectInit =
            entity::effect::fire::FireEffect::initialize(device(), physicalDevice(), commandPool(), graphicsQueue());
        if (fireEffectInit) {
            spdlog::info("FireEffect initialized");
        } else {
            spdlog::warn("Failed to initialize FireEffect");
        }
    }

    m_entityRendererInitialized = true;
    spdlog::info("Entity renderer initialized");

    // 若方块纹理图集已加载（ChunkRenderer 已初始化），注入到 EntityRendererManager
    // 供末影人手持方块层（HeldBlockLayer）使用
    // 注意：initializeEntityRenderer 可能在 ChunkRenderer 初始化之前或之后调用，
    //       此处处理"之后"的情况；"之前"的情况由加载方块纹理图集的位置处理
    if (m_chunkRendererInitialized && m_chunkRenderer) {
        m_entityRendererManager->setChunkTextureAtlas(&m_chunkRenderer->textureAtlas());
    }

    return {};
}

Result<void> TridentEngine::initializeEntityTextureAtlas(ResourceManager* resourceManager)
{
    if (m_entityTextureAtlasInitialized) {
        return {};
    }

    if (!resourceManager) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    spdlog::info("Initializing entity texture atlas...");

    // 初始化实体纹理图集
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentEngine::initializeEntityTextureAtlas::InitAtlas");
        auto initResult = m_entityTextureAtlas.initialize(device(), physicalDevice(), commandPool(), graphicsQueue());
        if (initResult.failed()) {
            return initResult.error();
        }
    }

    // 构建资源包列表（按优先级从低到高）
    std::vector<IResourcePack*> packs;
    for (size_t i = 0; i < resourceManager->resourcePackCount(); ++i) {
        auto* pack = resourceManager->getResourcePack(i);
        if (pack) {
            packs.push_back(pack);
        }
    }

    spdlog::info("EntityTextureAtlas: Loading textures from {} resource packs", packs.size());

    // 从资源包加载火焰纹理（fire_0.png / fire_1.png）
    if (entity::effect::fire::FireEffect::isInitialized()) {
        if (!entity::effect::fire::FireEffect::loadTexture(packs)) {
            spdlog::warn("Failed to load fire texture from resource packs; using placeholder");
        }
    }

    // 使用新的自动发现方法加载所有实体纹理
    u32 loadedCount = 0;
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentEngine::initializeEntityTextureAtlas::LoadTextures");
        EntityTextureLoader textureLoader;
        auto loadResult = textureLoader.loadAllEntityTextures(packs, m_entityTextureAtlas);
        loadedCount = loadResult.success() ? loadResult.value() : 0;
    }

    // 加载本地玩家皮肤（可选）
    // 优先级高于资源包中的默认 steve/alex 纹理。
    const ResourceLocation localPlayerSkinLocation("minecraft:textures/entity/player/local_player.png");
    const std::array<std::filesystem::path, 4> localSkinCandidates = {
        std::filesystem::path("resources/skins/player.png"),
        std::filesystem::path("resources/skins/local_player.png"),
        std::filesystem::path("resources/skins/steve.png"),
        std::filesystem::path("resources/skins/alex.png")};

    for (const auto& candidate : localSkinCandidates) {
        std::error_code existsError;
        if (!std::filesystem::exists(candidate, existsError) || existsError) {
            continue;
        }

        auto localSkinResult = m_entityTextureAtlas.addTextureFromFile(candidate, localPlayerSkinLocation);
        if (localSkinResult.success()) {
            ++loadedCount;
            m_localPlayerSkinLocation = localPlayerSkinLocation;
            spdlog::info("Loaded local player skin from {}", candidate.string());
            break;
        }

        spdlog::warn(
            "Failed to load local player skin from {}: {}", candidate.string(), localSkinResult.error().toString());
    }

    if (loadedCount > 0) {
        spdlog::info("Total {} entity textures loaded", loadedCount);

        // 构建纹理图集
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentEngine::initializeEntityTextureAtlas::BuildAtlas");
        auto buildResult = m_entityTextureAtlas.build();
        if (buildResult.failed()) {
            spdlog::warn("Failed to build entity texture atlas: {}", buildResult.error().toString());
        } else {
            spdlog::info("Entity texture atlas built: {}x{}", buildResult.value().width, buildResult.value().height);

            // 设置纹理图集到 EntityRendererManager（用于 UV 重映射）
            m_entityRendererManager->setTextureAtlas(&m_entityTextureAtlas);

            // 设置纹理图集到 EntityPipeline
            // 初始化阶段无在飞帧，用 AllFrames 把同一图集写入每帧的纹理描述符集，
            // 保证每帧 set 首次 bind 前都指向有效纹理。运行时切图集走 per-frame setTextureAtlas。
            if (m_entityPipeline && m_entityPipeline->isInitialized()) {
                m_entityPipeline->setTextureAtlasAllFrames(
                    m_entityTextureAtlas.imageView(), m_entityTextureAtlas.sampler());
                spdlog::info("Entity texture atlas bound to pipeline");
            }

            if (m_firstPersonRendererInitialized && m_firstPersonRenderer) {
                m_firstPersonRenderer->setPlayerSkinLocation(m_localPlayerSkinLocation);
            }
        }
    } else {
        spdlog::warn("No entity textures loaded from any resource pack");
    }

    m_entityTextureAtlasInitialized = true;
    spdlog::info("Entity texture atlas initialized");
    return {};
}

Result<void> TridentEngine::initializeFogManager()
{
    if (m_fogManagerInitialized) {
        return {};
    }

    spdlog::info("Initializing fog manager...");

    if (!m_fogManager) {
        m_fogManager = std::make_unique<fog::FogManager>();
    }

    auto result = m_fogManager->initialize(
        device(), physicalDevice(), descriptorPool(), m_descriptorManager->fogLayout(), maxFramesInFlight());

    if (result.failed()) {
        m_fogManager.reset();
        return result.error();
    }

    m_fogManagerInitialized = true;
    spdlog::info("Fog manager initialized");
    return {};
}

Result<void> TridentEngine::initializeCloudRenderer(ResourceManager* resourceManager)
{
    if (m_cloudRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing cloud renderer...");

    if (!m_cloudRenderer) {
        m_cloudRenderer = std::make_unique<cloud::CloudRenderer>();
    }

    auto result = m_cloudRenderer->initialize(device(),
        physicalDevice(),
        commandPool(),
        graphicsQueue(),
        renderPass(),
        swapchainExtent(),
        m_msaaSamples,
        resourceManager);

    if (result.failed()) {
        m_cloudRenderer.reset();
        return result.error();
    }

    m_cloudRendererInitialized = true;
    spdlog::info("Cloud renderer initialized");
    return {};
}

Result<void> TridentEngine::reloadCloudTexture(ResourceManager* resourceManager)
{
    if (!m_cloudRendererInitialized || !m_cloudRenderer) {
        return {};
    }

    return m_cloudRenderer->reloadTexture(resourceManager);
}

Result<void> TridentEngine::reloadFireTexture(ResourceManager* resourceManager)
{
    if (!entity::effect::fire::FireEffect::isInitialized()) {
        return {};
    }

    if (resourceManager == nullptr) {
        // 无资源管理器时回退到程序化纹理
        if (!entity::effect::fire::FireEffect::loadTexture({})) {
            return Error(ErrorCode::OperationFailed, "Failed to reload fire texture with empty packs");
        }
        return {};
    }

    // 构建资源包列表（按优先级从低到高）
    std::vector<IResourcePack*> packs;
    for (size_t i = 0; i < resourceManager->resourcePackCount(); ++i) {
        auto* pack = resourceManager->getResourcePack(i);
        if (pack) {
            packs.push_back(pack);
        }
    }

    if (!entity::effect::fire::FireEffect::loadTexture(packs)) {
        return Error(ErrorCode::OperationFailed, "Failed to reload fire texture from resource packs");
    }
    return {};
}

cloud::CloudRenderer& TridentEngine::cloudRenderer()
{
    if (!m_cloudRenderer) {
        m_cloudRenderer = std::make_unique<cloud::CloudRenderer>();
    }
    return *m_cloudRenderer;
}

const cloud::CloudRenderer& TridentEngine::cloudRenderer() const
{
    return *m_cloudRenderer;
}

// ============================================================================
// 粒子管理器
// ============================================================================

Result<void> TridentEngine::initializeParticleManager()
{
    if (m_particleManagerInitialized) {
        return {};
    }

    spdlog::info("Initializing particle manager...");

    if (!m_particleManager) {
        m_particleManager = std::make_unique<particle::ParticleManager>();
    }

    auto result = m_particleManager->initialize(
        device(), physicalDevice(), commandPool(), graphicsQueue(), renderPass(), swapchainExtent(), m_msaaSamples);

    if (result.failed()) {
        m_particleManager.reset();
        return result.error();
    }

    m_particleManagerInitialized = true;
    spdlog::info("Particle manager initialized");
    return {};
}

particle::ParticleManager& TridentEngine::particleManager()
{
    if (!m_particleManager) {
        m_particleManager = std::make_unique<particle::ParticleManager>();
    }
    return *m_particleManager;
}

const particle::ParticleManager& TridentEngine::particleManager() const
{
    return *m_particleManager;
}

// ============================================================================
// 天气渲染器
// ============================================================================

Result<void> TridentEngine::initializeWeatherRenderer()
{
    if (m_weatherRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing weather renderer...");

    if (!m_weatherRenderer) {
        m_weatherRenderer = std::make_unique<weather::WeatherRenderer>();
    }

    auto result = m_weatherRenderer->initialize(
        device(), physicalDevice(), commandPool(), graphicsQueue(), renderPass(), swapchainExtent(), m_msaaSamples);

    if (result.failed()) {
        m_weatherRenderer.reset();
        return result.error();
    }

    m_weatherRendererInitialized = true;
    spdlog::info("Weather renderer initialized");
    return {};
}

weather::WeatherRenderer& TridentEngine::weatherRenderer()
{
    if (!m_weatherRenderer) {
        m_weatherRenderer = std::make_unique<weather::WeatherRenderer>();
    }
    return *m_weatherRenderer;
}

const weather::WeatherRenderer& TridentEngine::weatherRenderer() const
{
    return *m_weatherRenderer;
}

Result<void> TridentEngine::initializeBreakProgressRenderer(ResourceManager* resourceManager)
{
    if (m_breakProgressRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing break progress renderer...");

    if (!m_breakProgressRenderer) {
        m_breakProgressRenderer = std::make_unique<block::BreakProgressRenderer>();
    }

    block::BreakProgressRenderer::Config config;
    config.device = device();
    config.physicalDevice = physicalDevice();
    config.commandPool = commandPool();
    config.graphicsQueue = graphicsQueue();
    config.renderPass = renderPass();
    config.cameraLayout = cameraDescriptorLayout();
    config.fogLayout = fogDescriptorLayout();
    config.maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
    config.resourceManager = resourceManager;

    if (!m_breakProgressRenderer->initialize(config, m_msaaSamples)) {
        m_breakProgressRenderer.reset();
        return Error(ErrorCode::InitializationFailed, "Failed to initialize break progress renderer");
    }

    m_breakProgressRendererInitialized = true;
    spdlog::info("Break progress renderer initialized");
    return {};
}

block::BreakProgressRenderer& TridentEngine::breakProgressRenderer()
{
    if (!m_breakProgressRenderer) {
        m_breakProgressRenderer = std::make_unique<block::BreakProgressRenderer>();
    }
    return *m_breakProgressRenderer;
}

const block::BreakProgressRenderer& TridentEngine::breakProgressRenderer() const
{
    return *m_breakProgressRenderer;
}

Result<void> TridentEngine::initializeFirstPersonRenderer()
{
    if (m_firstPersonRendererInitialized) {
        return {};
    }

    spdlog::info("Initializing first person renderer...");

    if (!m_firstPersonRenderer) {
        m_firstPersonRenderer = std::make_unique<firstperson::FirstPersonRenderer>();
    }

    auto result = m_firstPersonRenderer->initialize(device(),
        physicalDevice(),
        commandPool(),
        graphicsQueue(),
        renderPass(),
        cameraDescriptorLayout(),
        descriptorPool(),
        &m_entityTextureAtlas,
        maxFramesInFlight(),
        m_msaaSamples);

    if (result.failed()) {
        m_firstPersonRenderer.reset();
        spdlog::error("Failed to initialize first person renderer: {}", result.error().toString());
        return result.error();
    }

    if (m_itemTextureAtlasInitialized && m_itemTextureAtlas.isValid()) {
        m_firstPersonRenderer->setItemTextureAtlas(&m_itemTextureAtlas);
    }

    if (m_chunkRenderer != nullptr) {
        m_firstPersonRenderer->setChunkTextureAtlas(&m_chunkRenderer->textureAtlas());
    }

    m_firstPersonRenderer->setPlayerSkinLocation(m_localPlayerSkinLocation);

    m_firstPersonRendererInitialized = true;
    spdlog::info("First person renderer initialized");
    return {};
}

firstperson::FirstPersonRenderer& TridentEngine::firstPersonRenderer()
{
    if (!m_firstPersonRenderer) {
        m_firstPersonRenderer = std::make_unique<firstperson::FirstPersonRenderer>();
    }
    return *m_firstPersonRenderer;
}

const firstperson::FirstPersonRenderer& TridentEngine::firstPersonRenderer() const
{
    return *m_firstPersonRenderer;
}

Result<void> TridentEngine::updateTextureAtlas(const AtlasBuildResult& atlasResult)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "TridentEngine not initialized");
    }

    if (atlasResult.pixels.empty()) {
        return Error(ErrorCode::InvalidArgument, "Atlas result has no pixel data");
    }

    spdlog::info(
        "Updating texture atlas: {}x{}, {} regions", atlasResult.width, atlasResult.height, atlasResult.regions.size());

    // 保存纹理区域映射
    m_textureRegions = atlasResult.regions;

    // 初始化区块渲染器（如果尚未初始化）
    if (!m_chunkRendererInitialized) {
        auto chunkResult = initializeChunkRenderer();
        if (chunkResult.failed()) {
            spdlog::warn("Failed to initialize chunk renderer: {}", chunkResult.error().toString());
            return chunkResult.error();
        }
    }

    // 加载纹理图集到区块渲染器
    auto loadResult = m_chunkRenderer->loadTextureAtlas(atlasResult.pixels.data(),
        atlasResult.width,
        atlasResult.height,
        static_cast<u32>(::mc::world::CHUNK_WIDTH) // tileSize
    );
    if (loadResult.failed()) {
        spdlog::error("Failed to load texture atlas to chunk renderer: {}", loadResult.error().toString());
        return loadResult.error();
    }

    // 更新区块纹理描述符（set = 1, binding = 0）
    if (m_chunkTextureDescriptorSet != VK_NULL_HANDLE) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_chunkRenderer->textureAtlas().imageView;
        imageInfo.sampler = m_chunkRenderer->textureAtlas().sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_chunkTextureDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device(), 1, &descriptorWrite, 0, nullptr);
    }

    // 更新实体管线的纹理（如果已初始化）
    if (m_entityRendererInitialized && m_entityRendererManager) {
        // 将方块纹理图集注入到 EntityRendererManager，供末影人手持方块层（HeldBlockLayer）使用
        // 方块纹理 UV 基于方块纹理图集（ChunkTextureAtlas），而非实体纹理图集
        m_entityRendererManager->setChunkTextureAtlas(&m_chunkRenderer->textureAtlas());
    }

    // 同步方块纹理图集到第一人称渲染器（方块物品 3D 渲染切换图集用）。
    if (m_firstPersonRendererInitialized && m_firstPersonRenderer && m_chunkRenderer != nullptr) {
        m_firstPersonRenderer->setChunkTextureAtlas(&m_chunkRenderer->textureAtlas());
    }

    // 注册动画精灵
    if (!atlasResult.animations.empty()) {
        // 清除旧的动画 ticker
        m_blockAtlasTicker.clear();

        for (const auto& anim : atlasResult.animations) {
            // 将 AnimationDescriptor 转换为 AnimatedSprite
            std::vector<AnimatedSprite::FrameData> frames;
            frames.reserve(anim.framePixels.size());
            for (const auto& frameData : anim.framePixels) {
                AnimatedSprite::FrameData frame;
                frame.pixels = frameData;
                frame.width = anim.frameWidth;
                frame.height = anim.frameHeight;
                frames.push_back(std::move(frame));
            }

            auto sprite = std::make_shared<AnimatedSprite>(anim.metadata, std::move(frames), anim.atlasX, anim.atlasY);
            sprite->setLocation(anim.location);
            m_blockAtlasTicker.registerAnimatedSprite(std::move(sprite));
        }

        spdlog::info("Registered {} animated textures for block atlas", atlasResult.animations.size());
    }

    spdlog::info("Texture atlas updated successfully");
    return {};
}

const TextureRegion* TridentEngine::getTextureRegion(const ResourceLocation& location) const
{
    auto it = m_textureRegions.find(location);
    if (it != m_textureRegions.end()) {
        return &it->second;
    }
    return nullptr;
}

void TridentEngine::tickTextureAnimations()
{
    m_blockAtlasTicker.tick();
    m_itemAtlasTicker.tick();

    // 火焰纹理动画推进（独立 VkImage，不经过图集，需单独 tick）
    entity::effect::fire::FireEffect::tick();
}

void TridentEngine::uploadAnimationFrames()
{
    if (m_blockAtlasTicker.empty() && m_itemAtlasTicker.empty()) {
        return;
    }

    auto* tridentContext = m_context.get();
    if (!tridentContext) {
        return;
    }

    // 上传方块图集动画帧
    if (!m_blockAtlasTicker.empty() && m_chunkRendererInitialized && m_chunkRenderer) {
        auto& ticker = m_blockAtlasTicker;
        for (size_t i = 0; i < ticker.spriteCount(); ++i) {
            auto* sprite = ticker.getSprite(i);
            if (sprite && sprite->needsUpload()) {
                const auto& frameData = sprite->currentFramePixels();
                if (!frameData.empty()) {
                    auto result = m_chunkRenderer->uploadTextureRegion(frameData.data(),
                        frameData.size(),
                        sprite->atlasX(),
                        sprite->atlasY(),
                        sprite->frameWidth(),
                        sprite->frameHeight(),
                        sprite->frameWidth());
                    if (result.failed()) {
                        spdlog::warn("Failed to upload block atlas animation frame for {}: {}",
                            sprite->location().toString(),
                            result.error().message());
                    }
                    sprite->markUploaded();
                }
            }
        }
    }

    // 上传物品图集动画帧
    if (!m_itemAtlasTicker.empty() && m_itemTextureAtlasInitialized) {
        auto& ticker = m_itemAtlasTicker;
        for (size_t i = 0; i < ticker.spriteCount(); ++i) {
            auto* sprite = ticker.getSprite(i);
            if (sprite && sprite->needsUpload()) {
                const auto& frameData = sprite->currentFramePixels();
                if (!frameData.empty()) {
                    auto result = m_itemTextureAtlas.uploadRegion(frameData.data(),
                        frameData.size(),
                        sprite->atlasX(),
                        sprite->atlasY(),
                        sprite->frameWidth(),
                        sprite->frameHeight(),
                        sprite->frameWidth());
                    if (result.failed()) {
                        spdlog::warn("Failed to upload item atlas animation frame for {}: {}",
                            sprite->location().toString(),
                            result.error().message());
                    }
                    sprite->markUploaded();
                }
            }
        }
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

} // namespace mc::client::renderer::trident

namespace mc::client::renderer::api {

std::unique_ptr<IRenderEngine> createRenderEngine(RenderBackend backend)
{
    switch (backend) {
        case RenderBackend::Vulkan:
            return std::make_unique<trident::TridentEngine>();
        default:
            return nullptr;
    }
}

} // namespace mc::client::renderer::api
