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

/**
 * @file test_trident_engine.cpp
 * @brief Trident 渲染引擎核心组件测试
 *
 * 测试覆盖：
 * - TridentConfig 配置验证
 * - TridentContext Vulkan 实例/设备/队列
 * - TridentBuffer 缓冲区（顶点/索引/Uniform/暂存）
 * - TridentTexture 纹理
 * - TridentTextureAtlas 纹理图集
 * - FrameManager 帧管理
 * - UniformManager Uniform 缓冲区管理
 * - DimensionRenderSettings 维度渲染设置（云高度等）
 */

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/api/Types.hpp"
#include "client/renderer/trident/cloud/CloudRenderer.hpp"
#include "client/renderer/trident/core/Trident.hpp"
#include "client/renderer/trident/core/buffer/TridentBuffer.hpp"
#include "client/renderer/trident/core/pipeline/TridentPipeline.hpp"
#include "client/renderer/trident/core/render/DescriptorManager.hpp"
#include "client/renderer/trident/core/render/FrameManager.hpp"
#include "client/renderer/trident/core/render/UniformManager.hpp"
#include "client/renderer/trident/core/texture/TridentTexture.hpp"
#include "common/world/dimension/DimensionRenderSettings.hpp"
#include <vector>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::trident;
using namespace mc::client::renderer::trident::cloud;
using namespace mc::client::renderer::api;

// ============================================================================
// 测试基类 - 提供 GLFW 和 Vulkan 环境
// ============================================================================

/**
 * @brief Vulkan 环境测试基类
 *
 * 为需要完整 Vulkan 环境的测试提供统一的初始化和清理。
 */
class TridentTestBase : public ::testing::Test {
protected:
    static GLFWwindow* s_window;
    static TridentContext* s_context;

    static void SetUpTestSuite()
    {
        // 初始化 GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";

        // 设置 GLFW 错误回调
        glfwSetErrorCallback(
            [](int error, const char* description) { FAIL() << "GLFW Error " << error << ": " << description; });

        // 创建无上下文窗口（仅用于 Vulkan surface）
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 隐藏窗口
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        s_window = glfwCreateWindow(800, 600, "TridentTest", nullptr, nullptr);
        ASSERT_NE(s_window, nullptr) << "Failed to create GLFW window";

        // 创建 Trident 上下文
        s_context = new TridentContext();
        TridentConfig config;
        config.enableValidation = true; // 启用验证层
        config.enableVSync = false;

        auto result = s_context->initialize(s_window, config);
        ASSERT_TRUE(result.success()) << "Failed to initialize TridentContext: " << result.error().message();
    }

    static void TearDownTestSuite()
    {
        // 清理上下文
        if (s_context) {
            s_context->destroy();
            delete s_context;
            s_context = nullptr;
        }

        // 清理 GLFW
        if (s_window) {
            glfwDestroyWindow(s_window);
            s_window = nullptr;
        }
        glfwTerminate();
    }

    void SetUp() override
    {
        ASSERT_NE(s_context, nullptr) << "TridentContext not initialized";
        ASSERT_TRUE(s_context->isInitialized()) << "TridentContext not ready";
    }

    void TearDown() override
    {
        // 确保设备空闲
        s_context->waitIdle();
    }

    // 辅助方法：轮询事件
    void pollEvents() { glfwPollEvents(); }
};

// 静态成员定义
GLFWwindow* TridentTestBase::s_window = nullptr;
TridentContext* TridentTestBase::s_context = nullptr;

// ============================================================================
// TridentConfig 测试
// ============================================================================

class TridentConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TridentConfigTest, DefaultConfiguration)
{
    TridentConfig config;

    EXPECT_EQ(config.appName, "Trident");
    EXPECT_EQ(config.engineName, "Trident Engine");
    EXPECT_TRUE(config.enableValidation);
    EXPECT_TRUE(config.enableVSync);
    EXPECT_EQ(config.maxFramesInFlight, 2u);
}

TEST_F(TridentConfigTest, RequiredExtensions)
{
    TridentConfig config;

    // 必须包含交换链扩展
    bool hasSwapchain = false;
    for (const auto& ext : config.requiredDeviceExtensions) {
        if (ext == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
            hasSwapchain = true;
            break;
        }
    }
    EXPECT_TRUE(hasSwapchain) << "Swapchain extension should be required by default";
}

TEST_F(TridentConfigTest, OptionalExtensions)
{
    TridentConfig config;

    // 检查可选扩展
    bool hasTimelineSemaphore = false;
    bool hasSynchronization2 = false;

    for (const auto& ext : config.optionalDeviceExtensions) {
        if (ext == VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) hasTimelineSemaphore = true;
        if (ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) hasSynchronization2 = true;
    }

    EXPECT_TRUE(hasTimelineSemaphore);
    EXPECT_TRUE(hasSynchronization2);
}

TEST_F(TridentConfigTest, CustomConfiguration)
{
    TridentConfig config;
    config.appName = "CustomApp";
    config.engineName = "CustomEngine";
    config.enableValidation = false;
    config.enableVSync = false;
    config.maxFramesInFlight = 3;

    EXPECT_EQ(config.appName, "CustomApp");
    EXPECT_EQ(config.engineName, "CustomEngine");
    EXPECT_FALSE(config.enableValidation);
    EXPECT_FALSE(config.enableVSync);
    EXPECT_EQ(config.maxFramesInFlight, 3u);
}

// ============================================================================
// VulkanVersion 测试
// ============================================================================

class VulkanVersionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(VulkanVersionTest, ToString)
{
    VulkanVersion v1{1, 0, 0};
    EXPECT_EQ(v1.toString(), "1.0.0");

    VulkanVersion v2{1, 2, 3};
    EXPECT_EQ(v2.toString(), "1.2.3");

    VulkanVersion v3{0, 0, 1};
    EXPECT_EQ(v3.toString(), "0.0.1");
}

// ============================================================================
// QueueFamilyIndices 测试
// ============================================================================

class QueueFamilyIndicesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(QueueFamilyIndicesTest, DefaultIncomplete)
{
    QueueFamilyIndices indices;

    EXPECT_FALSE(indices.isComplete());
    EXPECT_FALSE(indices.hasTransfer());
    EXPECT_FALSE(indices.hasCompute());
}

TEST_F(QueueFamilyIndicesTest, CompleteWithGraphicsAndPresent)
{
    QueueFamilyIndices indices;
    indices.graphicsFamily = 0;
    indices.presentFamily = 0;

    EXPECT_TRUE(indices.isComplete());
    EXPECT_FALSE(indices.hasTransfer());
    EXPECT_FALSE(indices.hasCompute());
}

TEST_F(QueueFamilyIndicesTest, CompleteWithAllQueues)
{
    QueueFamilyIndices indices;
    indices.graphicsFamily = 0;
    indices.presentFamily = 0;
    indices.transferFamily = 1;
    indices.computeFamily = 2;

    EXPECT_TRUE(indices.isComplete());
    EXPECT_TRUE(indices.hasTransfer());
    EXPECT_TRUE(indices.hasCompute());
}

// ============================================================================
// TridentContext 测试
// ============================================================================

class TridentContextTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

TEST_F(TridentContextTest, InitializationSuccessful)
{
    EXPECT_TRUE(s_context->isInitialized());
    EXPECT_NE(s_context->instance(), VK_NULL_HANDLE);
    EXPECT_NE(s_context->physicalDevice(), VK_NULL_HANDLE);
    EXPECT_NE(s_context->device(), VK_NULL_HANDLE);
    EXPECT_NE(s_context->surface(), VK_NULL_HANDLE);
}

TEST_F(TridentContextTest, QueuesValid)
{
    EXPECT_NE(s_context->graphicsQueue(), VK_NULL_HANDLE);
    EXPECT_NE(s_context->presentQueue(), VK_NULL_HANDLE);
}

TEST_F(TridentContextTest, QueueFamiliesComplete)
{
    const auto& families = s_context->queueFamilies();

    EXPECT_TRUE(families.isComplete());
    EXPECT_TRUE(families.graphicsFamily.has_value());
    EXPECT_TRUE(families.presentFamily.has_value());
}

TEST_F(TridentContextTest, DevicePropertiesValid)
{
    const auto& props = s_context->deviceProperties();

    // 检查设备属性是否有效
    EXPECT_NE(props.deviceName[0], '\0'); // 设备名称非空
    EXPECT_GT(props.apiVersion, 0u);      // API 版本有效
}

TEST_F(TridentContextTest, MemoryPropertiesValid)
{
    const auto& memProps = s_context->memoryProperties();

    // 至少有一种内存类型
    EXPECT_GT(memProps.memoryTypeCount, 0u);
    // 至少有一个内存堆
    EXPECT_GT(memProps.memoryHeapCount, 0u);
}

TEST_F(TridentContextTest, SwapChainSupportQuery)
{
    auto support = s_context->querySwapChainSupport();

    // 检查表面能力
    EXPECT_GE(support.capabilities.minImageCount, 1u);
    EXPECT_GT(support.capabilities.currentExtent.width, 0u);
    EXPECT_GT(support.capabilities.currentExtent.height, 0u);

    // 检查格式支持
    EXPECT_FALSE(support.formats.empty());

    // 检查呈现模式支持
    EXPECT_FALSE(support.presentModes.empty());
}

TEST_F(TridentContextTest, FindMemoryType)
{
    // 查找设备本地内存
    auto result = s_context->findMemoryType(0xFFFFFFFF, // 所有类型位
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    EXPECT_TRUE(result.success());
    EXPECT_GE(result.value(), 0u);

    // 查找主机可见内存
    result = s_context->findMemoryType(
        0xFFFFFFFF, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    EXPECT_TRUE(result.success());
}

TEST_F(TridentContextTest, FindDepthFormat)
{
    auto result = s_context->findDepthFormat();
    EXPECT_TRUE(result.success());

    VkFormat format = result.value();
    // 常见深度格式
    bool isValidFormat = (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_TRUE(isValidFormat);
}

TEST_F(TridentContextTest, SingleTimeCommands)
{
    // 测试单次命令缓冲区
    VkCommandBuffer cmd = s_context->beginSingleTimeCommands();
    EXPECT_NE(cmd, VK_NULL_HANDLE);

    // 执行一个简单的屏障
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        1,
        &barrier,
        0,
        nullptr,
        0,
        nullptr);

    // 提交命令
    s_context->endSingleTimeCommands(cmd);
}

TEST_F(TridentContextTest, ValidationEnabled)
{
    EXPECT_TRUE(s_context->isValidationEnabled());
}

// ============================================================================
// TridentBuffer 测试
// ============================================================================

class TridentBufferTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

TEST_F(TridentBufferTest, CreateVertexBuffer)
{
    TridentVertexBuffer vbo;

    auto result = vbo.create(s_context, 1024, sizeof(Vertex));
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(vbo.isValid());
    EXPECT_EQ(vbo.size(), 1024u);
    EXPECT_EQ(vbo.vertexStride(), sizeof(Vertex));
    EXPECT_NE(vbo.buffer(), VK_NULL_HANDLE);

    vbo.destroy();
    EXPECT_FALSE(vbo.isValid());
}

TEST_F(TridentBufferTest, VertexBufferUploadViaStaging)
{
    // VertexBuffer 是设备本地内存，需要使用 StagingBuffer 来传输数据
    TridentStagingBuffer staging;
    ASSERT_TRUE(staging.create(s_context, sizeof(Vertex) * 4).success());

    TridentVertexBuffer vbo;
    ASSERT_TRUE(vbo.create(s_context, sizeof(Vertex) * 4, sizeof(Vertex)).success());

    // 创建测试顶点数据
    Vertex vertices[4] = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF, 255},
        {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 255},
        {1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 255},
        {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 255}};

    // 上传到暂存缓冲区
    auto result = staging.upload(vertices, sizeof(vertices));
    EXPECT_TRUE(result.success()) << result.error().message();

    // 使用命令缓冲区复制到顶点缓冲区
    VkCommandBuffer cmd = s_context->beginSingleTimeCommands();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    result = staging.copyTo(cmd, &vbo, sizeof(vertices));
    EXPECT_TRUE(result.success()) << result.error().message();

    s_context->endSingleTimeCommands(cmd);

    staging.destroy();
    vbo.destroy();
}

TEST_F(TridentBufferTest, VertexBufferCannotMapDeviceLocal)
{
    // VertexBuffer 是设备本地内存，不能直接映射
    TridentVertexBuffer vbo;
    ASSERT_TRUE(vbo.create(s_context, 1024, sizeof(Vertex)).success());

    // 设备本地内存不能直接映射，应该返回 nullptr
    void* mapped = vbo.map();
    EXPECT_EQ(mapped, nullptr) << "Device-local vertex buffer should not be directly mappable";

    vbo.destroy();
}

TEST_F(TridentBufferTest, VertexBufferVertexCount)
{
    TridentVertexBuffer vbo;
    const u64 bufferSize = sizeof(Vertex) * 100;
    ASSERT_TRUE(vbo.create(s_context, bufferSize, sizeof(Vertex)).success());

    EXPECT_EQ(vbo.vertexCount(), 100u);

    vbo.destroy();
}

TEST_F(TridentBufferTest, VertexBufferDirectUpload)
{
    // 测试 TridentVertexBuffer::upload() 直接上传方法
    TridentVertexBuffer vbo;
    const u64 bufferSize = sizeof(Vertex) * 4;
    ASSERT_TRUE(vbo.create(s_context, bufferSize, sizeof(Vertex)).success());

    // 创建测试顶点数据
    Vertex vertices[4] = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF, 255},
        {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF, 255},
        {1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 255},
        {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 255}};

    // 使用 upload() 方法直接上传
    auto result = vbo.upload(vertices, sizeof(vertices), 0);
    EXPECT_TRUE(result.success()) << "Direct upload should succeed: " << result.error().message();

    vbo.destroy();
}

TEST_F(TridentBufferTest, VertexBufferDirectUploadWithOffset)
{
    // 测试带偏移的上传
    TridentVertexBuffer vbo;
    const u64 bufferSize = sizeof(Vertex) * 8;
    ASSERT_TRUE(vbo.create(s_context, bufferSize, sizeof(Vertex)).success());

    // 先上传前 4 个顶点
    Vertex vertices1[4] = {};
    auto result = vbo.upload(vertices1, sizeof(vertices1), 0);
    EXPECT_TRUE(result.success()) << "First upload should succeed: " << result.error().message();

    // 再上传后 4 个顶点（带偏移）
    Vertex vertices2[4] = {{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0xFFFFFFFF, 255}};
    result = vbo.upload(vertices2, sizeof(vertices2), sizeof(vertices1));
    EXPECT_TRUE(result.success()) << "Second upload with offset should succeed: " << result.error().message();

    vbo.destroy();
}

TEST_F(TridentBufferTest, VertexBufferUploadInvalidParameters)
{
    TridentVertexBuffer vbo;
    ASSERT_TRUE(vbo.create(s_context, 1024, sizeof(Vertex)).success());

    Vertex vertices[4] = {};

    // 测试空指针
    auto result = vbo.upload(nullptr, sizeof(vertices), 0);
    EXPECT_FALSE(result.success()) << "Should fail with null data pointer";
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);

    // 测试零大小
    result = vbo.upload(vertices, 0, 0);
    EXPECT_FALSE(result.success()) << "Should fail with zero size";
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);

    // 测试超出范围
    result = vbo.upload(vertices, sizeof(vertices), 2000);
    EXPECT_FALSE(result.success()) << "Should fail with out of range offset";
    EXPECT_EQ(result.error().code(), ErrorCode::OutOfRange);

    vbo.destroy();
}

TEST_F(TridentBufferTest, CreateIndexBuffer)
{
    TridentIndexBuffer ibo;

    auto result = ibo.create(s_context, sizeof(u32) * 6, IndexType::U32);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(ibo.isValid());
    EXPECT_EQ(ibo.size(), sizeof(u32) * 6);
    EXPECT_EQ(ibo.indexType(), IndexType::U32);
    EXPECT_EQ(ibo.indexCount(), 6u);
    EXPECT_NE(ibo.buffer(), VK_NULL_HANDLE);

    ibo.destroy();
    EXPECT_FALSE(ibo.isValid());
}

TEST_F(TridentBufferTest, IndexBufferUploadViaStaging)
{
    // IndexBuffer 是设备本地内存，需要使用 StagingBuffer 来传输数据
    TridentStagingBuffer staging;
    ASSERT_TRUE(staging.create(s_context, sizeof(u32) * 6).success());

    TridentIndexBuffer ibo;
    ASSERT_TRUE(ibo.create(s_context, sizeof(u32) * 6, IndexType::U32).success());

    u32 indices[6] = {0, 1, 2, 0, 2, 3};

    // 上传到暂存缓冲区
    auto result = staging.upload(indices, sizeof(indices));
    EXPECT_TRUE(result.success()) << result.error().message();

    // 使用命令缓冲区复制到索引缓冲区
    VkCommandBuffer cmd = s_context->beginSingleTimeCommands();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    result = staging.copyTo(cmd, &ibo, sizeof(indices));
    EXPECT_TRUE(result.success()) << result.error().message();

    s_context->endSingleTimeCommands(cmd);

    staging.destroy();
    ibo.destroy();
}

TEST_F(TridentBufferTest, IndexBufferDirectUpload)
{
    // 测试 TridentIndexBuffer::upload() 直接上传方法
    TridentIndexBuffer ibo;
    ASSERT_TRUE(ibo.create(s_context, sizeof(u32) * 6, IndexType::U32).success());

    u32 indices[6] = {0, 1, 2, 0, 2, 3};

    // 使用 upload() 方法直接上传
    auto result = ibo.upload(indices, sizeof(indices), 0);
    EXPECT_TRUE(result.success()) << "Direct upload should succeed: " << result.error().message();

    ibo.destroy();
}

TEST_F(TridentBufferTest, IndexBufferDirectUploadWithOffset)
{
    // 测试带偏移的上传
    TridentIndexBuffer ibo;
    const u64 bufferSize = sizeof(u32) * 12;
    ASSERT_TRUE(ibo.create(s_context, bufferSize, IndexType::U32).success());

    // 先上传前 6 个索引
    u32 indices1[6] = {0, 1, 2, 0, 2, 3};
    auto result = ibo.upload(indices1, sizeof(indices1), 0);
    EXPECT_TRUE(result.success()) << "First upload should succeed: " << result.error().message();

    // 再上传后 6 个索引（带偏移）
    u32 indices2[6] = {4, 5, 6, 4, 6, 7};
    result = ibo.upload(indices2, sizeof(indices2), sizeof(indices1));
    EXPECT_TRUE(result.success()) << "Second upload with offset should succeed: " << result.error().message();

    ibo.destroy();
}

TEST_F(TridentBufferTest, IndexBufferUploadInvalidParameters)
{
    TridentIndexBuffer ibo;
    ASSERT_TRUE(ibo.create(s_context, 1024, IndexType::U32).success());

    u32 indices[6] = {0, 1, 2, 0, 2, 3};

    // 测试空指针
    auto result = ibo.upload(nullptr, sizeof(indices), 0);
    EXPECT_FALSE(result.success()) << "Should fail with null data pointer";
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);

    // 测试零大小
    result = ibo.upload(indices, 0, 0);
    EXPECT_FALSE(result.success()) << "Should fail with zero size";
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);

    // 测试超出范围
    result = ibo.upload(indices, sizeof(indices), 2000);
    EXPECT_FALSE(result.success()) << "Should fail with out of range offset";
    EXPECT_EQ(result.error().code(), ErrorCode::OutOfRange);

    ibo.destroy();
}

TEST_F(TridentBufferTest, CreateUniformBuffer)
{
    TridentUniformBuffer ubo;

    auto result = ubo.create(s_context, sizeof(CameraUBO), 2);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(ubo.isValid());
    EXPECT_EQ(ubo.size(), sizeof(CameraUBO));
    EXPECT_EQ(ubo.frameCount(), 2u);

    ubo.destroy();
    EXPECT_FALSE(ubo.isValid());
}

TEST_F(TridentBufferTest, UniformBufferFrameAdvancement)
{
    TridentUniformBuffer ubo;
    ASSERT_TRUE(ubo.create(s_context, sizeof(CameraUBO), 2).success());

    EXPECT_EQ(ubo.currentFrameIndex(), 0u);

    ubo.advanceFrame();
    EXPECT_EQ(ubo.currentFrameIndex(), 1u);

    ubo.advanceFrame();
    EXPECT_EQ(ubo.currentFrameIndex(), 0u); // 循环回第 0 帧

    ubo.destroy();
}

TEST_F(TridentBufferTest, UniformBufferUpload)
{
    TridentUniformBuffer ubo;
    ASSERT_TRUE(ubo.create(s_context, sizeof(CameraUBO), 2).success());

    CameraUBO cameraData{};
    cameraData.view = glm::mat4(1.0f);
    cameraData.projection = glm::mat4(1.0f);
    cameraData.viewProjection = glm::mat4(1.0f);

    auto result = ubo.upload(&cameraData, sizeof(CameraUBO));
    EXPECT_TRUE(result.success()) << result.error().message();

    ubo.destroy();
}

TEST_F(TridentBufferTest, CreateStagingBuffer)
{
    TridentStagingBuffer staging;

    auto result = staging.create(s_context, 4096);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(staging.isValid());
    EXPECT_EQ(staging.size(), 4096u);
    EXPECT_EQ(staging.usage(), BufferUsage::Staging);

    staging.destroy();
    EXPECT_FALSE(staging.isValid());
}

TEST_F(TridentBufferTest, StagingBufferUploadAndCopy)
{
    // 创建暂存缓冲区
    TridentStagingBuffer staging;
    ASSERT_TRUE(staging.create(s_context, sizeof(Vertex) * 4).success());

    // 创建目标顶点缓冲区
    TridentVertexBuffer vbo;
    ASSERT_TRUE(vbo.create(s_context, sizeof(Vertex) * 4, sizeof(Vertex)).success());

    // 上传数据到暂存缓冲区
    Vertex vertices[4] = {};
    auto result = staging.upload(vertices, sizeof(vertices));
    EXPECT_TRUE(result.success()) << result.error().message();

    // 开始命令缓冲区
    VkCommandBuffer cmd = s_context->beginSingleTimeCommands();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // 从暂存缓冲区复制到顶点缓冲区
    result = staging.copyTo(cmd, &vbo, sizeof(vertices));
    EXPECT_TRUE(result.success()) << result.error().message();

    // 提交命令
    s_context->endSingleTimeCommands(cmd);

    staging.destroy();
    vbo.destroy();
}

TEST_F(TridentBufferTest, BufferMoveSemantics)
{
    TridentVertexBuffer vbo1;
    ASSERT_TRUE(vbo1.create(s_context, 1024, sizeof(Vertex)).success());

    VkBuffer originalBuffer = vbo1.buffer();

    // 移动构造
    TridentVertexBuffer vbo2(std::move(vbo1));

    EXPECT_EQ(vbo2.buffer(), originalBuffer);
    EXPECT_FALSE(vbo1.isValid()); // 原对象应该无效

    vbo2.destroy();
}

// ============================================================================
// TridentTexture 测试
// ============================================================================

class TridentTextureTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

TEST_F(TridentTextureTest, CreateTexture2D)
{
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 256;
    desc.height = 256;
    desc.format = TextureFormat::R8G8B8A8_SRGB;
    desc.mipLevels = 1;

    auto result = texture.create(s_context, desc);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(texture.isValid());
    EXPECT_EQ(texture.width(), 256u);
    EXPECT_EQ(texture.height(), 256u);
    EXPECT_EQ(texture.mipLevels(), 1u);
    EXPECT_NE(texture.image(), VK_NULL_HANDLE);
    EXPECT_NE(texture.imageView(), VK_NULL_HANDLE);
    EXPECT_NE(texture.sampler(), VK_NULL_HANDLE);

    texture.destroy();
    EXPECT_FALSE(texture.isValid());
}

TEST_F(TridentTextureTest, CreateTextureWithMipmaps)
{
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 256;
    desc.height = 256;
    desc.format = TextureFormat::R8G8B8A8_SRGB;
    desc.mipLevels = 8; // log2(256) + 1 = 8
    desc.generateMipmaps = true;

    auto result = texture.create(s_context, desc);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(texture.mipLevels(), 8u);

    texture.destroy();
}

TEST_F(TridentTextureTest, TextureUpload)
{
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 16;
    desc.height = 16;
    desc.format = TextureFormat::R8G8B8A8_SRGB;
    desc.mipLevels = 1;

    ASSERT_TRUE(texture.create(s_context, desc).success());

    // 创建测试像素数据
    std::vector<u8> pixels(16 * 16 * 4, 255);

    auto result = texture.upload(pixels.data(), pixels.size(), 0);
    EXPECT_TRUE(result.success()) << result.error().message();

    texture.destroy();
}

TEST_F(TridentTextureTest, TextureFormatConversion)
{
    // 测试不同纹理格式
    struct FormatTest {
        TextureFormat apiFormat;
        bool shouldSucceed;
    };

    FormatTest formats[] = {
        {TextureFormat::R8G8B8A8_SRGB, true},
        {TextureFormat::R8G8B8A8_UNORM, true},
        {TextureFormat::R8_UNORM, true},
        {TextureFormat::BC1_RGB_SRGB, false}, // 需要块压缩支持
    };

    for (const auto& test : formats) {
        TridentTexture texture;

        TextureDesc desc{};
        desc.width = 64;
        desc.height = 64;
        desc.format = test.apiFormat;
        desc.mipLevels = 1;

        auto result = texture.create(s_context, desc);
        if (test.shouldSucceed) {
            EXPECT_TRUE(result.success()) << "Format should be supported";
            texture.destroy();
        }
        // 不支持的格式可能创建失败，这是预期行为
    }
}

// ============================================================================
// TridentTextureAtlas 测试
// ============================================================================

class TridentTextureAtlasTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

TEST_F(TridentTextureAtlasTest, CreateAtlas)
{
    TridentTextureAtlas atlas;

    auto result = atlas.create(s_context, 256, 256, 16);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(atlas.isValid());
    EXPECT_EQ(atlas.width(), 256u);
    EXPECT_EQ(atlas.height(), 256u);
    EXPECT_EQ(atlas.tileSize(), 16u);
    EXPECT_EQ(atlas.tilesPerRow(), 16u); // 256 / 16 = 16

    atlas.destroy();
    EXPECT_FALSE(atlas.isValid());
}

TEST_F(TridentTextureAtlasTest, GetRegion)
{
    TridentTextureAtlas atlas;
    ASSERT_TRUE(atlas.create(s_context, 256, 256, 16).success());

    // 获取 (0, 0) 位置的纹理区域
    auto region = atlas.getRegion(0, 0);
    EXPECT_DOUBLE_EQ(region.u0, 0.0);
    EXPECT_DOUBLE_EQ(region.v0, 0.0);
    EXPECT_DOUBLE_EQ(region.u1, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(region.v1, 1.0 / 16.0);

    // 获取 (1, 1) 位置的纹理区域
    region = atlas.getRegion(1, 1);
    EXPECT_DOUBLE_EQ(region.u0, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(region.v0, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(region.u1, 2.0 / 16.0);
    EXPECT_DOUBLE_EQ(region.v1, 2.0 / 16.0);

    atlas.destroy();
}

TEST_F(TridentTextureAtlasTest, GetRegionByIndex)
{
    TridentTextureAtlas atlas;
    ASSERT_TRUE(atlas.create(s_context, 256, 256, 16).success());

    // 索引 0 = (0, 0)
    auto region = atlas.getRegion(0);
    EXPECT_DOUBLE_EQ(region.u0, 0.0);
    EXPECT_DOUBLE_EQ(region.v0, 0.0);

    // 索引 1 = (1, 0)
    region = atlas.getRegion(1);
    EXPECT_DOUBLE_EQ(region.u0, 1.0 / 16.0);
    EXPECT_DOUBLE_EQ(region.v0, 0.0);

    // 索引 16 = (0, 1)
    region = atlas.getRegion(16);
    EXPECT_DOUBLE_EQ(region.u0, 0.0);
    EXPECT_DOUBLE_EQ(region.v0, 1.0 / 16.0);

    atlas.destroy();
}

TEST_F(TridentTextureAtlasTest, UploadAtlasData)
{
    TridentTextureAtlas atlas;
    ASSERT_TRUE(atlas.create(s_context, 64, 64, 16).success());

    // 创建图集构建结果
    AtlasBuildResult buildResult;
    buildResult.width = 64;
    buildResult.height = 64;
    buildResult.tileSize = 16;
    buildResult.pixelData.resize(64 * 64 * 4, 128);

    auto result = atlas.upload(buildResult);
    EXPECT_TRUE(result.success()) << result.error().message();

    atlas.destroy();
}

// ============================================================================
// 纹理子区域上传测试 (Texture Region Upload Tests)
// ============================================================================

class TextureRegionUploadTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

TEST_F(TextureRegionUploadTest, TextureUploadRegion_Basic)
{
    // 测试基本的子区域上传
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 64;
    desc.height = 64;
    desc.format = TextureFormat::R8G8B8A8_UNORM;
    desc.mipLevels = 1;

    ASSERT_TRUE(texture.create(s_context, desc).success());

    // 先上传完整纹理（初始数据）
    std::vector<u8> initialData(64 * 64 * 4, 0);
    ASSERT_TRUE(texture.upload(initialData.data(), initialData.size(), 0).success());

    // 上传子区域（16x16 区域，位于 (32, 32)）
    std::vector<u8> regionData(16 * 16 * 4, 255); // 白色像素
    auto result = texture.uploadRegion(regionData.data(), regionData.size(), 32, 32, 16, 16, 0, 0);
    EXPECT_TRUE(result.success()) << result.error().message();

    texture.destroy();
}

TEST_F(TextureRegionUploadTest, TextureUploadRegion_TopLeft)
{
    // 测试左上角子区域上传
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 32;
    desc.height = 32;
    desc.format = TextureFormat::R8G8B8A8_UNORM;
    desc.mipLevels = 1;

    ASSERT_TRUE(texture.create(s_context, desc).success());

    // 初始化纹理
    std::vector<u8> initialData(32 * 32 * 4, 0);
    ASSERT_TRUE(texture.upload(initialData.data(), initialData.size(), 0).success());

    // 上传到左上角 (0, 0) 的 8x8 区域
    std::vector<u8> regionData(8 * 8 * 4, 128);
    auto result = texture.uploadRegion(regionData.data(), regionData.size(), 0, 0, 8, 8, 0, 0);
    EXPECT_TRUE(result.success()) << result.error().message();

    texture.destroy();
}

TEST_F(TextureRegionUploadTest, TextureUploadRegion_BottomRight)
{
    // 测试右下角子区域上传
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 32;
    desc.height = 32;
    desc.format = TextureFormat::R8G8B8A8_UNORM;
    desc.mipLevels = 1;

    ASSERT_TRUE(texture.create(s_context, desc).success());

    // 初始化纹理
    std::vector<u8> initialData(32 * 32 * 4, 0);
    ASSERT_TRUE(texture.upload(initialData.data(), initialData.size(), 0).success());

    // 上传到右下角 (24, 24) 的 8x8 区域
    std::vector<u8> regionData(8 * 8 * 4, 200);
    auto result = texture.uploadRegion(regionData.data(), regionData.size(), 24, 24, 8, 8, 0, 0);
    EXPECT_TRUE(result.success()) << result.error().message();

    texture.destroy();
}

TEST_F(TextureRegionUploadTest, TextureUploadRegion_OutOfBounds)
{
    // 测试边界检查
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 32;
    desc.height = 32;
    desc.format = TextureFormat::R8G8B8A8_UNORM;
    desc.mipLevels = 1;

    ASSERT_TRUE(texture.create(s_context, desc).success());

    // 初始化纹理
    std::vector<u8> initialData(32 * 32 * 4, 0);
    ASSERT_TRUE(texture.upload(initialData.data(), initialData.size(), 0).success());

    // 尝试上传超出边界的区域
    std::vector<u8> regionData(16 * 16 * 4, 255);
    auto result = texture.uploadRegion(regionData.data(), regionData.size(), 24, 24, 16, 16, 0, 0);
    EXPECT_FALSE(result.success()); // 应该失败：24 + 16 > 32

    texture.destroy();
}

TEST_F(TextureRegionUploadTest, TextureUploadRegion_InvalidParams)
{
    // 测试无效参数
    TridentTexture texture;

    TextureDesc desc{};
    desc.width = 32;
    desc.height = 32;
    desc.format = TextureFormat::R8G8B8A8_UNORM;
    desc.mipLevels = 1;

    ASSERT_TRUE(texture.create(s_context, desc).success());

    // 初始化纹理
    std::vector<u8> initialData(32 * 32 * 4, 0);
    ASSERT_TRUE(texture.upload(initialData.data(), initialData.size(), 0).success());

    std::vector<u8> regionData(16 * 16 * 4, 255);

    // 空数据指针
    auto result = texture.uploadRegion(nullptr, regionData.size(), 0, 0, 16, 16, 0, 0);
    EXPECT_FALSE(result.success());

    // 零大小
    result = texture.uploadRegion(regionData.data(), 0, 0, 0, 16, 16, 0, 0);
    EXPECT_FALSE(result.success());

    // 零宽高
    result = texture.uploadRegion(regionData.data(), regionData.size(), 0, 0, 0, 16, 0, 0);
    EXPECT_FALSE(result.success());

    result = texture.uploadRegion(regionData.data(), regionData.size(), 0, 0, 16, 0, 0, 0);
    EXPECT_FALSE(result.success());

    texture.destroy();
}

TEST_F(TextureRegionUploadTest, AtlasUploadRegion_Basic)
{
    // 测试纹理图集子区域上传
    TridentTextureAtlas atlas;
    ASSERT_TRUE(atlas.create(s_context, 64, 64, 16).success());

    // 先上传完整图集数据
    std::vector<u8> initialData(64 * 64 * 4, 0);
    ASSERT_TRUE(atlas.uploadRegion(initialData.data(), initialData.size(), 0, 0, 64, 64, 0).success());

    // 上传单个瓦片区域 (16x16，位于瓦片坐标 (1, 2))
    std::vector<u8> tileData(16 * 16 * 4, 255);
    auto result = atlas.uploadRegion(tileData.data(), tileData.size(), 16, 32, 16, 16, 0);
    EXPECT_TRUE(result.success()) << result.error().message();

    atlas.destroy();
}

TEST_F(TextureRegionUploadTest, AtlasUploadRegion_MultipleTiles)
{
    // 测试上传多个瓦片区域
    TridentTextureAtlas atlas;
    ASSERT_TRUE(atlas.create(s_context, 64, 64, 16).success());

    // 初始化图集
    std::vector<u8> initialData(64 * 64 * 4, 0);
    ASSERT_TRUE(atlas.uploadRegion(initialData.data(), initialData.size(), 0, 0, 64, 64, 0).success());

    // 上传多个瓦片
    for (u32 y = 0; y < 4; ++y) {
        for (u32 x = 0; x < 4; ++x) {
            std::vector<u8> tileData(16 * 16 * 4, static_cast<u8>((x + y * 4) * 16));
            auto result = atlas.uploadRegion(tileData.data(), tileData.size(), x * 16, y * 16, 16, 16, 0);
            EXPECT_TRUE(result.success()) << "Upload at (" << x << ", " << y << ") should succeed";
        }
    }

    atlas.destroy();
}

// ============================================================================
// FrameManager 测试
// ============================================================================

class FrameManagerTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

TEST_F(FrameManagerTest, CreateFrameManager)
{
    FrameManager frameManager;

    auto result = frameManager.initialize(s_context, 2);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(frameManager.isValid());
    EXPECT_EQ(frameManager.maxFramesInFlight(), 2u);
    EXPECT_NE(frameManager.commandPool(), VK_NULL_HANDLE);

    frameManager.destroy();
    EXPECT_FALSE(frameManager.isValid());
}

TEST_F(FrameManagerTest, FrameIndexCycling)
{
    FrameManager frameManager;
    ASSERT_TRUE(frameManager.initialize(s_context, 2).success());

    EXPECT_EQ(frameManager.currentFrameIndex(), 0u);
    EXPECT_FALSE(frameManager.isFrameStarted());

    // 注意：不能在没有交换链的情况下调用 beginFrame/endFrame

    frameManager.destroy();
}

// ============================================================================
// UniformManager 测试
// ============================================================================

class UniformManagerTest : public TridentTestBase {
protected:
    void SetUp() override { TridentTestBase::SetUp(); }
};

// 注意：UniformManager 需要 DescriptorManager，这里只测试结构大小

TEST_F(UniformManagerTest, CameraUBOSize)
{
    // CameraUBO 应该对齐到 256 字节（Vulkan 要求）
    EXPECT_EQ(sizeof(CameraUBO), sizeof(glm::mat4) * 3);
}

TEST_F(UniformManagerTest, LightingUBOSize)
{
    // LightingUBO 大小检查
    EXPECT_GE(sizeof(LightingUBO),
        sizeof(glm::vec3) + sizeof(f32) +     // sunDirection + sunIntensity
            sizeof(glm::vec3) + sizeof(f32) + // moonDirection + moonIntensity
            sizeof(f32) + sizeof(f32));       // dayTime + gameTime
}

// ============================================================================
// TridentPipeline 配置测试
// ============================================================================

class TridentPipelineConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TridentPipelineConfigTest, DefaultConfiguration)
{
    TridentPipelineConfig config;

    EXPECT_EQ(config.topology, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    EXPECT_EQ(config.primitiveRestartEnable, VK_FALSE);
    EXPECT_TRUE(config.dynamicViewport);
    EXPECT_TRUE(config.dynamicScissor);
    EXPECT_EQ(config.polygonMode, VK_POLYGON_MODE_FILL);
    EXPECT_EQ(config.cullMode, VK_CULL_MODE_BACK_BIT);
    EXPECT_EQ(config.frontFace, VK_FRONT_FACE_CLOCKWISE);
    EXPECT_DOUBLE_EQ(config.lineWidth, 1.0);
}

TEST_F(TridentPipelineConfigTest, DepthStencilDefaults)
{
    TridentPipelineConfig config;

    EXPECT_EQ(config.depthTestEnable, VK_TRUE);
    EXPECT_EQ(config.depthWriteEnable, VK_TRUE);
    EXPECT_EQ(config.depthCompareOp, VK_COMPARE_OP_LESS);
    EXPECT_EQ(config.stencilTestEnable, VK_FALSE);
}

TEST_F(TridentPipelineConfigTest, BlendDefaults)
{
    TridentPipelineConfig config;

    EXPECT_EQ(config.blendEnable, VK_FALSE);
    EXPECT_EQ(config.srcColorBlendFactor, VK_BLEND_FACTOR_SRC_ALPHA);
    EXPECT_EQ(config.dstColorBlendFactor, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(config.colorBlendOp, VK_BLEND_OP_ADD);
}

// ============================================================================
// RenderState 和 RenderType 测试（补充）
// ============================================================================

class ExtendedRenderStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExtendedRenderStateTest, BlendFactorValues)
{
    // 验证 BlendFactor 枚举值与 Vulkan 匹配
    EXPECT_EQ(static_cast<VkBlendFactor>(BlendFactor::Zero), VK_BLEND_FACTOR_ZERO);
    EXPECT_EQ(static_cast<VkBlendFactor>(BlendFactor::One), VK_BLEND_FACTOR_ONE);
    EXPECT_EQ(static_cast<VkBlendFactor>(BlendFactor::SrcAlpha), VK_BLEND_FACTOR_SRC_ALPHA);
    EXPECT_EQ(static_cast<VkBlendFactor>(BlendFactor::OneMinusSrcAlpha), VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    EXPECT_EQ(static_cast<VkBlendFactor>(BlendFactor::DstAlpha), VK_BLEND_FACTOR_DST_ALPHA);
    EXPECT_EQ(static_cast<VkBlendFactor>(BlendFactor::OneMinusDstAlpha), VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
}

TEST_F(ExtendedRenderStateTest, CompareOpValues)
{
    // 验证 CompareOp 枚举值与 Vulkan 匹配
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::Never), VK_COMPARE_OP_NEVER);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::Less), VK_COMPARE_OP_LESS);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::Equal), VK_COMPARE_OP_EQUAL);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::LessEqual), VK_COMPARE_OP_LESS_OR_EQUAL);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::Greater), VK_COMPARE_OP_GREATER);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::NotEqual), VK_COMPARE_OP_NOT_EQUAL);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::GreaterEqual), VK_COMPARE_OP_GREATER_OR_EQUAL);
    EXPECT_EQ(static_cast<VkCompareOp>(CompareOp::Always), VK_COMPARE_OP_ALWAYS);
}

TEST_F(ExtendedRenderStateTest, CullModeValues)
{
    // 验证 CullMode 枚举值与 Vulkan 匹配
    EXPECT_EQ(static_cast<VkCullModeFlags>(CullMode::None), VK_CULL_MODE_NONE);
    EXPECT_EQ(static_cast<VkCullModeFlags>(CullMode::Front), VK_CULL_MODE_FRONT_BIT);
    EXPECT_EQ(static_cast<VkCullModeFlags>(CullMode::Back), VK_CULL_MODE_BACK_BIT);
    EXPECT_EQ(static_cast<VkCullModeFlags>(CullMode::FrontAndBack), VK_CULL_MODE_FRONT_AND_BACK);
}

TEST_F(ExtendedRenderStateTest, RenderStateCopy)
{
    RenderState original = RenderState::solid();
    RenderState copy = original;

    EXPECT_EQ(original.blend.enabled, copy.blend.enabled);
    EXPECT_EQ(original.depth.testEnabled, copy.depth.testEnabled);
    EXPECT_EQ(original.depth.writeEnabled, copy.depth.writeEnabled);
    EXPECT_EQ(original.rasterizer.cullMode, copy.rasterizer.cullMode);
}

TEST_F(ExtendedRenderStateTest, RenderTypePredefined)
{
    // 测试所有预定义渲染类型
    auto solid = RenderType::solid();
    EXPECT_TRUE(solid.isValid());
    EXPECT_EQ(solid.name(), "solid");

    auto cutout = RenderType::cutout();
    EXPECT_TRUE(cutout.isValid());
    EXPECT_EQ(cutout.name(), "cutout");

    auto translucent = RenderType::translucent();
    EXPECT_TRUE(translucent.isValid());
    EXPECT_EQ(translucent.name(), "translucent");

    auto lines = RenderType::lines();
    EXPECT_TRUE(lines.isValid());
    EXPECT_EQ(lines.name(), "lines");
}

// ============================================================================
// MeshData 扩展测试
// ============================================================================

class ExtendedMeshDataTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExtendedMeshDataTest, LargeMeshReserve)
{
    MeshData mesh;
    mesh.reserve(10000, 60000); // 10000 顶点, 60000 索引

    // reserve 不改变 size
    EXPECT_EQ(mesh.vertexCount(), 0u);
    EXPECT_EQ(mesh.indexCount(), 0u);
}

TEST_F(ExtendedMeshDataTest, AddMultipleFaces)
{
    MeshData mesh;

    for (int i = 0; i < 6; ++i) { // 立方体的 6 个面
        std::array<Vertex, 4> faceVertices = {Vertex(0, 0, 0, 0, 0, 0xFFFFFFFF, 255),
            Vertex(1, 0, 0, 1, 0, 0xFFFFFFFF, 255),
            Vertex(1, 1, 0, 1, 1, 0xFFFFFFFF, 255),
            Vertex(0, 1, 0, 0, 1, 0xFFFFFFFF, 255)};
        mesh.addFace(faceVertices, i * 4);
    }

    EXPECT_EQ(mesh.vertexCount(), 24u); // 6 面 * 4 顶点
    EXPECT_EQ(mesh.indexCount(), 36u);  // 6 面 * 6 索引
}

// ============================================================================
// CloudHeight 测试
// ============================================================================

/**
 * @brief 测试云高度设置
 *
 * 参考 MC 1.16.5 DimensionRenderInfo.func_239213_a_()：
 * - 主世界: 有云 (云高度 192.0)
 * - 下界: 无云 (NaN)
 * - 末地: 无云 (NaN)
 */
class CloudHeightTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CloudHeightTest, CloudHeightDefaultValue)
{
    // TridentEngine 默认云高度为主世界值 192.0
    // 由于无法直接实例化 TridentEngine（需要 Vulkan 环境），
    // 这里测试 DimensionRenderSettings 的默认值
    auto overworld = world::DimensionRenderSettings::overworld();
    EXPECT_FLOAT_EQ(overworld.cloudHeight, 192.0f);
    EXPECT_TRUE(overworld.hasClouds);
}

TEST_F(CloudHeightTest, CloudHeightFieldIndicatesNoClouds)
{
    // hasClouds 字段指示该维度是否有云
    // 注意：由于项目使用 -ffast-math，NaN 检测不可靠，
    // 因此使用显式的 hasClouds 布尔字段
    auto nether = world::DimensionRenderSettings::nether();
    auto end = world::DimensionRenderSettings::end();

    EXPECT_FALSE(nether.hasClouds);
    EXPECT_FALSE(end.hasClouds);
}

TEST_F(CloudHeightTest, DimensionRenderSettingsForAllDimensions)
{
    // 验证所有维度的渲染设置
    auto overworld = world::DimensionRenderSettings::overworld();
    auto nether = world::DimensionRenderSettings::nether();
    auto end = world::DimensionRenderSettings::end();

    // 主世界有云
    EXPECT_TRUE(overworld.hasClouds);
    EXPECT_FLOAT_EQ(overworld.cloudHeight, 192.0f);
    EXPECT_TRUE(overworld.hasSky);
    EXPECT_FALSE(overworld.hasCeiling);
    EXPECT_EQ(overworld.fogType, world::FogType::Normal);
    EXPECT_TRUE(overworld.hasNaturalLight);

    // 下界无云
    EXPECT_FALSE(nether.hasClouds);
    EXPECT_FALSE(nether.hasSky);
    EXPECT_TRUE(nether.hasCeiling);
    EXPECT_EQ(nether.fogType, world::FogType::None);
    EXPECT_FALSE(nether.hasNaturalLight);

    // 末地无云
    EXPECT_FALSE(end.hasClouds);
    EXPECT_FALSE(end.hasSky);
    EXPECT_FALSE(end.hasCeiling);
    EXPECT_EQ(end.fogType, world::FogType::End);
    EXPECT_FALSE(end.hasNaturalLight);
}

TEST_F(CloudHeightTest, CloudModeEnumValues)
{
    // 验证 CloudMode 枚举值
    EXPECT_EQ(static_cast<u8>(cloud::CloudMode::Off), 0);
    EXPECT_EQ(static_cast<u8>(cloud::CloudMode::Fast), 1);
    EXPECT_EQ(static_cast<u8>(cloud::CloudMode::Fancy), 2);
}

// main 函数由 gtest_main 库提供
