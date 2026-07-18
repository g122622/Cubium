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

#include "BlendMode.hpp"
#include "CompareOp.hpp"
#include "CullMode.hpp"
#include "Types.hpp"
#include "buffer/IBuffer.hpp"
#include "camera/ICamera.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "pipeline/IPipeline.hpp"
#include "pipeline/RenderState.hpp"
#include "pipeline/RenderType.hpp"
#include "texture/ITexture.hpp"
#include "texture/ITextureAtlas.hpp"
#include "texture/TextureRegion.hpp"
#include <functional>
#include <memory>

namespace mc::client::renderer::api {

/**
 * @brief 渲染引擎配置
 */
struct RenderEngineConfig {
    std::string appName = "Trident";
    bool enableValidation = true;   // 启用验证层
    bool enableVSync = true;        // 垂直同步
    bool enableAntiAliasing = true; // 抗锯齿开关
    u32 msaaSamples = 4;            // MSAA 采样数（当前用于配置与日志）
    u32 maxFramesInFlight = 2;      // 最大帧在飞数
    u32 initialWindowWidth = 1280;  // 初始窗口宽度
    u32 initialWindowHeight = 720;  // 初始窗口高度
};

/**
 * @brief 帧上下文
 *
 * 包含当前帧渲染所需的所有信息。
 */
struct FrameContext {
    u32 frameIndex = 0;              // 当前帧索引 (用于多帧资源轮换)
    u32 imageIndex = 0;              // 当前交换链图像索引
    f64 deltaTime = 0.0;             // 帧时间（秒）
    f64 totalTime = 0.0;             // 总运行时间（秒）
    const ICamera* camera = nullptr; // 当前相机

    // 矩阵缓存
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::mat4 viewProjectionMatrix{1.0f};
};

/**
 * @brief 渲染引擎接口
 *
 * 平台无关的渲染引擎抽象接口。
 * 这是渲染系统的主入口点，负责：
 * - 初始化和销毁渲染资源
 * - 管理帧渲染生命周期
 * - 创建和管理 GPU 资源
 * - 渲染状态管理
 *
 * 使用示例：
 * @code
 * auto engine = createRenderEngine(RenderBackend::Vulkan);
 * engine->initialize(window, config);
 *
 * while (running) {
 *     engine->beginFrame();
 *     // 设置相机、绑定管线、绘制...
 *     engine->endFrame();
 *     engine->present();
 * }
 *
 * engine->destroy();
 * @endcode
 */
class IRenderEngine {
public:
    virtual ~IRenderEngine() = default;

    // ========================================================================
    // 生命周期管理
    // ========================================================================

    /**
     * @brief 初始化渲染引擎
     * @param window 平台窗口句柄 (GLFWwindow*)
     * @param config 渲染引擎配置
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> initialize(void* window, const RenderEngineConfig& config) = 0;

    /**
     * @brief 销毁渲染引擎
     *
     * 释放所有渲染资源。
     */
    virtual void destroy() = 0;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] virtual bool isInitialized() const = 0;

    // ========================================================================
    // 帧渲染
    // ========================================================================

    /**
     * @brief 开始新帧
     *
     * 必须在每帧开始时调用。
     * 获取下一个交换链图像，准备命令缓冲区。
     *
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> beginFrame() = 0;

    /**
     * @brief 结束帧
     *
     * 完成命令录制，提交到 GPU。
     *
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> endFrame() = 0;

    /**
     * @brief 呈现帧
     *
     * 将渲染结果呈现到屏幕。
     *
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> present() = 0;

    // ========================================================================
    // 窗口和相机
    // ========================================================================

    /**
     * @brief 处理窗口大小变化
     * @param width 新宽度
     * @param height 新高度
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> onResize(u32 width, u32 height) = 0;

    /**
     * @brief 设置当前相机
     * @param camera 相机指针
     */
    virtual void setCamera(const ICamera* camera) = 0;

    /**
     * @brief 获取当前相机
     */
    [[nodiscard]] virtual const ICamera* camera() const = 0;

    // ========================================================================
    // 资源创建
    // ========================================================================

    /**
     * @brief 创建顶点缓冲区
     * @param size 缓冲区大小（字节）
     * @param vertexStride 顶点步长（字节）
     * @return 顶点缓冲区或错误
     */
    [[nodiscard]] virtual Result<std::unique_ptr<IVertexBuffer>> createVertexBuffer(u64 size, u32 vertexStride) = 0;

    /**
     * @brief 创建索引缓冲区
     * @param size 缓冲区大小（字节）
     * @param type 索引类型
     * @return 索引缓冲区或错误
     */
    [[nodiscard]] virtual Result<std::unique_ptr<IIndexBuffer>> createIndexBuffer(u64 size, IndexType type) = 0;

    /**
     * @brief 创建 Uniform 缓冲区
     * @param size 缓冲区大小（字节）
     * @param frameCount 帧数（用于多帧轮换）
     * @return Uniform 缓冲区或错误
     */
    [[nodiscard]] virtual Result<std::unique_ptr<IUniformBuffer>> createUniformBuffer(u64 size, u32 frameCount = 2) = 0;

    /**
     * @brief 创建纹理
     * @param desc 纹理描述
     * @return 纹理或错误
     */
    [[nodiscard]] virtual Result<std::unique_ptr<ITexture>> createTexture(const TextureDesc& desc) = 0;

    /**
     * @brief 创建纹理图集
     * @param width 图集宽度
     * @param height 图集高度
     * @param tileSize 瓦片大小
     * @return 纹理图集或错误
     */
    [[nodiscard]] virtual Result<std::unique_ptr<ITextureAtlas>> createTextureAtlas(
        u32 width, u32 height, u32 tileSize) = 0;

    // ========================================================================
    // 渲染状态
    // ========================================================================

    /**
     * @brief 设置渲染类型
     * @param type 渲染类型
     */
    virtual void setRenderType(const RenderType& type) = 0;

    /**
     * @brief 获取当前渲染类型
     */
    [[nodiscard]] virtual const RenderType& currentRenderType() const = 0;

    /**
     * @brief 绑定纹理到指定绑定槽
     * @param binding 绑定槽索引
     * @param texture 纹理
     */
    virtual void bindTexture(u32 binding, const ITexture* texture) = 0;

    /**
     * @brief 绑定 Uniform 缓冲区到指定绑定槽
     * @param binding 绑定槽索引
     * @param buffer Uniform 缓冲区
     */
    virtual void bindUniformBuffer(u32 binding, const IUniformBuffer* buffer) = 0;

    // ========================================================================
    // 绘制
    // ========================================================================

    /**
     * @brief 绘制索引几何体
     * @param indexCount 索引数量
     * @param firstIndex 起始索引
     * @param vertexOffset 顶点偏移
     */
    virtual void drawIndexed(u32 indexCount, u32 firstIndex = 0, i32 vertexOffset = 0) = 0;

    /**
     * @brief 绘制非索引几何体
     * @param vertexCount 顶点数量
     * @param firstVertex 起始顶点
     */
    virtual void draw(u32 vertexCount, u32 firstVertex = 0) = 0;

    /**
     * @brief 绘制实例化索引几何体
     * @param indexCount 索引数量
     * @param instanceCount 实例数量
     * @param firstIndex 起始索引
     * @param vertexOffset 顶点偏移
     * @param firstInstance 起始实例
     */
    virtual void drawIndexedInstanced(
        u32 indexCount, u32 instanceCount, u32 firstIndex = 0, i32 vertexOffset = 0, u32 firstInstance = 0) = 0;

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 获取当前帧索引
     */
    [[nodiscard]] virtual u32 currentFrameIndex() const = 0;

    /**
     * @brief 获取当前交换链图像索引
     */
    [[nodiscard]] virtual u32 currentImageIndex() const = 0;

    /**
     * @brief 获取帧上下文
     */
    [[nodiscard]] virtual const FrameContext& frameContext() const = 0;

    /**
     * @brief 获取最大帧在飞数
     */
    [[nodiscard]] virtual u32 maxFramesInFlight() const = 0;

    /**
     * @brief 检查窗口是否最小化
     */
    [[nodiscard]] virtual bool isMinimized() const = 0;

    /**
     * @brief 获取当前窗口宽度
     */
    [[nodiscard]] virtual u32 windowWidth() const = 0;

    /**
     * @brief 获取当前窗口高度
     */
    [[nodiscard]] virtual u32 windowHeight() const = 0;
};

/**
 * @brief 渲染后端类型
 */
enum class RenderBackend : u8 {
    Vulkan,  // Vulkan 渲染后端
    OpenGL,  // OpenGL 渲染后端 (未来支持)
    DirectX, // DirectX 渲染后端 (未来支持)
    Metal    // Metal 渲染后端 (未来支持)
};

/**
 * @brief 创建渲染引擎
 * @param backend 渲染后端类型
 * @return 渲染引擎实例
 */
[[nodiscard]] std::unique_ptr<IRenderEngine> createRenderEngine(RenderBackend backend = RenderBackend::Vulkan);

} // namespace mc::client::renderer::api
