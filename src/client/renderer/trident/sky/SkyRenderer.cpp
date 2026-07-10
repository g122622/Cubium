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

#include "SkyRenderer.hpp"
#include "CelestialCalculations.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include "common/core/Constants.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client::renderer::trident::sky {

namespace {

constexpr f64 SKY_CLIP_SCALE = 0.0075;

Result<std::vector<u8>> readBinaryFile(const char* path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, "Failed to open shader file: " + std::string(path));
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        return Error(ErrorCode::InvalidData, std::string("Shader file is empty: ") + path);
    }

    std::vector<u8> data(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    if (!file.good()) {
        return Error(ErrorCode::Unknown, "Failed to read shader file: " + std::string(path));
    }

    return data;
}

Result<VkShaderModule> createShaderModule(VkDevice device, const char* path)
{
    auto codeResult = readBinaryFile(path);
    if (codeResult.failed()) {
        return codeResult.error();
    }

    const auto& code = codeResult.value();
    if (code.size() % 4 != 0) {
        return Error(ErrorCode::InvalidData, "Invalid SPIR-V file size: " + std::string(path));
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create shader module: " + std::string(path));
    }

    return shaderModule;
}

} // namespace

// ============================================================================
// 顶点结构
// ============================================================================

struct SkyVertex {
    f32 x, y, z;
};

struct SkyPushConstants {
    glm::mat4 viewProjection;
};

// ============================================================================
// 构造/析构
// ============================================================================

SkyRenderer::SkyRenderer() = default;

SkyRenderer::~SkyRenderer()
{
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> SkyRenderer::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkExtent2D extent,
    VkSampleCountFlagBits sampleCount)
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_extent = extent;

    // 创建顶点缓冲区
    auto result1 = _createSkyDomeVBO();
    if (result1.failed()) {
        return result1;
    }

    auto result2 = _createStarVBO();
    if (result2.failed()) {
        return result2;
    }

    auto result3 = _createSunMoonVBO();
    if (result3.failed()) {
        return result3;
    }

    // 创建 Uniform 缓冲区
    auto result4 = _createUniformBuffers();
    if (result4.failed()) {
        return result4;
    }

    // 创建描述符
    auto result5 = _createDescriptorSetLayout();
    if (result5.failed()) {
        return result5;
    }

    auto result6 = _createDescriptorSets();
    if (result6.failed()) {
        return result6;
    }

    // 创建管线
    auto result7 = _createPipelineLayout();
    if (result7.failed()) {
        return result7;
    }

    auto result8 = _createPipelines(sampleCount);
    if (result8.failed()) {
        return result8;
    }

    m_initialized = true;
    spdlog::info("SkyRenderer initialized");
    return Result<void>::ok();
}

void SkyRenderer::destroy()
{
    if (m_device == VK_NULL_HANDLE) return;

    VkDevice device = m_device;

    // 销毁管线
    if (m_skyPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_skyPipeline, nullptr);
        m_skyPipeline = VK_NULL_HANDLE;
    }
    if (m_sunPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_sunPipeline, nullptr);
        m_sunPipeline = VK_NULL_HANDLE;
    }
    if (m_moonPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_moonPipeline, nullptr);
        m_moonPipeline = VK_NULL_HANDLE;
    }
    if (m_starPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_starPipeline, nullptr);
        m_starPipeline = VK_NULL_HANDLE;
    }

    // 销毁管线布局
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // 销毁描述符池
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    // 销毁描述符集布局
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    // 销毁 Uniform 缓冲区
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (m_uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_uniformBuffers[i], nullptr);
            m_uniformBuffers[i] = VK_NULL_HANDLE;
        }
        if (m_uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_uniformBuffersMemory[i], nullptr);
            m_uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
        m_uniformBuffersMapped[i] = nullptr;
    }

    // 销毁顶点缓冲区
    if (m_skyDomeVBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_skyDomeVBO, nullptr);
        m_skyDomeVBO = VK_NULL_HANDLE;
    }
    if (m_skyDomeVBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_skyDomeVBOMemory, nullptr);
        m_skyDomeVBOMemory = VK_NULL_HANDLE;
    }
    if (m_skyDomeIBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_skyDomeIBO, nullptr);
        m_skyDomeIBO = VK_NULL_HANDLE;
    }
    if (m_skyDomeIBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_skyDomeIBOMemory, nullptr);
        m_skyDomeIBOMemory = VK_NULL_HANDLE;
    }
    if (m_starVBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_starVBO, nullptr);
        m_starVBO = VK_NULL_HANDLE;
    }
    if (m_starVBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_starVBOMemory, nullptr);
        m_starVBOMemory = VK_NULL_HANDLE;
    }
    if (m_sunMoonVBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_sunMoonVBO, nullptr);
        m_sunMoonVBO = VK_NULL_HANDLE;
    }
    if (m_sunMoonVBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_sunMoonVBOMemory, nullptr);
        m_sunMoonVBOMemory = VK_NULL_HANDLE;
    }

    m_initialized = false;
    spdlog::info("SkyRenderer destroyed");
}

Result<void> SkyRenderer::onResize(VkExtent2D extent)
{
    m_extent = extent;
    // 不需要重建资源，因为天空渲染使用的是视图投影矩阵
    return Result<void>::ok();
}

// ============================================================================
// 更新
// ============================================================================

void SkyRenderer::update(i64 dayTime, i64 gameTime, f64 partialTick, f64 rainStrength, f64 thunderStrength)
{
    MC_ASSERT_RELEASE_MSG(partialTick >= 0.0 && partialTick <= 1.0, "partialTick must be in range [0, 1]");

    m_dayTime = dayTime;
    m_gameTime = gameTime;
    m_rainStrength = rainStrength;
    m_thunderStrength = thunderStrength;

    // 计算天体角度 (插值)
    m_celestialAngle = client::CelestialCalculations::calculateCelestialAngleInterpolated(dayTime, partialTick);

    // 计算月相
    m_moonPhase = client::CelestialCalculations::calculateMoonPhase(gameTime);

    // 计算天空颜色
    m_skyColor = client::CelestialCalculations::calculateSkyColor(m_celestialAngle, rainStrength, thunderStrength);
    m_fogColor = client::CelestialCalculations::calculateFogColor(m_celestialAngle, rainStrength, thunderStrength);
    m_sunriseSunsetColor =
        client::CelestialCalculations::calculateSunriseSunsetColor(m_celestialAngle, rainStrength, thunderStrength);

    // 计算太阳方向和强度
    m_sunDirection = client::CelestialCalculations::calculateSunDirection(m_celestialAngle);
    m_sunIntensity = client::CelestialCalculations::calculateSunIntensity(m_celestialAngle);

    // 日出日落中心方向（始终在水平面）
    glm::dvec2 sunriseXZ(static_cast<f64>(m_sunDirection.x), static_cast<f64>(m_sunDirection.z));
    const f64 sunriseLen2 = glm::dot(sunriseXZ, sunriseXZ);
    if (sunriseLen2 > 1e-6) {
        sunriseXZ = sunriseXZ / std::sqrt(sunriseLen2);
        m_sunriseDirection = glm::vec3(static_cast<f32>(sunriseXZ.x), 0.0f, static_cast<f32>(sunriseXZ.y));
    }

    // 计算星星亮度
    m_starBrightness = client::CelestialCalculations::calculateStarBrightness(m_celestialAngle);
}

// ============================================================================
// 渲染
// ============================================================================

void SkyRenderer::render(VkCommandBuffer cmd,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos,
    const glm::vec3& cameraForward,
    u32 frameIndex)
{
    (void)cameraPos;

    if (!m_initialized || m_pipelineLayout == VK_NULL_HANDLE) {
        return;
    }

    m_cameraForward = cameraForward;

    m_currentFrame = frameIndex % MAX_FRAMES_IN_FLIGHT;

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "UpdateUBO");
        _updateUniformBuffer(m_currentFrame);
    }

    if (m_descriptorSets[m_currentFrame] == VK_NULL_HANDLE) {
        return;
    }

    // 绑定描述符集
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "SkyDescriptor");
        vkCmdBindDescriptorSets(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &m_descriptorSets[m_currentFrame],
            0,
            nullptr);
    }

    // 天空渲染关键：移除视图矩阵的平移分量，只保留旋转。
    // 这使得天体（太阳/月亮/星星）始终位于"无限远"处，不会随相机移动而改变位置。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "SkyViewProjection");
        SkyPushConstants pushConstants{};

        // 从视图矩阵中移除平移分量，只保留旋转
        // 这相当于将相机"移动到原点"，但保持其朝向不变
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

        // 组合投影矩阵和无平移视图矩阵
        // 由于视图矩阵没有平移，天体位置相对于相机旋转固定，不会随相机移动
        pushConstants.viewProjection = projection * viewNoTranslation * static_cast<f32>(SKY_CLIP_SCALE);

        vkCmdPushConstants(
            cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkyPushConstants), &pushConstants);
    }

    // 渲染天空穹顶 (最优先，写入深度为远平面)
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "SkyDome");
        _renderSkyDome(cmd);
    }

    // 渲染太阳
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "Sun");
        _renderSun(cmd);
    }

    // 渲染月亮
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "Moon");
        _renderMoon(cmd);
    }

    // 渲染星星 (夜晚可见)
    if (m_starBrightness > 0.005) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Sky, "Stars");
        _renderStars(cmd);
    }
}

void SkyRenderer::_renderSkyDome(VkCommandBuffer cmd)
{
    if (m_skyPipeline == VK_NULL_HANDLE || m_skyDomeVBO == VK_NULL_HANDLE || m_skyDomeIBO == VK_NULL_HANDLE ||
        m_skyDomeIndexCount == 0) {
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyPipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_skyDomeVBO, &offset);
    vkCmdBindIndexBuffer(cmd, m_skyDomeIBO, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd, m_skyDomeIndexCount, 1, 0, 0, 0);
}

void SkyRenderer::_renderSun(VkCommandBuffer cmd)
{
    // 太阳强度太低时不渲染
    if (m_sunIntensity < 0.03f || m_sunPipeline == VK_NULL_HANDLE || m_sunMoonVBO == VK_NULL_HANDLE) {
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_sunPipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_sunMoonVBO, &offset);

    // 太阳使用 2 个三角形（6 顶点）
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void SkyRenderer::_renderMoon(VkCommandBuffer cmd)
{
    // 白天不渲染月亮（晨昏允许短暂过渡）
    if (m_sunIntensity > 0.18f || m_moonPipeline == VK_NULL_HANDLE || m_sunMoonVBO == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_moonPipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_sunMoonVBO, &offset);

    // 月亮使用 2 个三角形（6 顶点，从索引 6 开始）
    vkCmdDraw(cmd, 6, 1, 6, 0);
}

void SkyRenderer::_renderStars(VkCommandBuffer cmd)
{
    if (m_starVertexCount == 0 || m_starPipeline == VK_NULL_HANDLE || m_starVBO == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_starPipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_starVBO, &offset);

    // 星星使用点渲染
    vkCmdDraw(cmd, m_starVertexCount, 1, 0, 0);
}

// ============================================================================
// 资源创建
// ============================================================================

Result<void> SkyRenderer::_createSkyDomeVBO()
{
    // 创建天空球网格。
    // 说明：
    // - 原先的平面天空无法区分"上半天空/下半天空"，无法实现晨昏时下半天空填充。
    // - 使用球面后，可基于方向向量按半球分别着色并叠加日出日落扇形效果。
    constexpr f64 SKY_RADIUS = 384.0;
    constexpr i32 STACK_COUNT = 32;
    constexpr i32 SLICE_COUNT = 64;

    std::vector<SkyVertex> vertices;
    std::vector<u16> indices;

    vertices.reserve(static_cast<size_t>((STACK_COUNT + 1) * (SLICE_COUNT + 1)));
    indices.reserve(static_cast<size_t>(STACK_COUNT * SLICE_COUNT * 6));

    // 生成球面顶点（纬度-经度）
    for (i32 stack = 0; stack <= STACK_COUNT; ++stack) {
        const f64 v = static_cast<f64>(stack) / static_cast<f64>(STACK_COUNT);
        const f64 phi = v * mc::math::PI_DOUBLE; // [0, PI]

        const f64 y = std::cos(phi) * SKY_RADIUS;
        const f64 ringRadius = std::sin(phi) * SKY_RADIUS;

        for (i32 slice = 0; slice <= SLICE_COUNT; ++slice) {
            const f64 u = static_cast<f64>(slice) / static_cast<f64>(SLICE_COUNT);
            const f64 theta = u * 2.0 * mc::math::PI_DOUBLE; // [0, 2PI]

            const f64 x = std::cos(theta) * ringRadius;
            const f64 z = std::sin(theta) * ringRadius;
            vertices.push_back({static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)});
        }
    }

    // 生成三角形索引
    const i32 stride = SLICE_COUNT + 1;
    for (i32 stack = 0; stack < STACK_COUNT; ++stack) {
        const i32 row0 = stack * stride;
        const i32 row1 = (stack + 1) * stride;

        for (i32 slice = 0; slice < SLICE_COUNT; ++slice) {
            const u16 i0 = static_cast<u16>(row0 + slice);
            const u16 i1 = static_cast<u16>(row1 + slice);
            const u16 i2 = static_cast<u16>(row0 + slice + 1);
            const u16 i3 = static_cast<u16>(row1 + slice + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i3);
        }
    }

    m_skyDomeIndexCount = static_cast<u32>(indices.size());

    // 创建 VBO
    VkDeviceSize vertexSize = vertices.size() * sizeof(SkyVertex);
    auto result = _createBuffer(vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_skyDomeVBO,
        m_skyDomeVBOMemory);
    if (result.failed()) {
        return result;
    }

    // 上传数据
    void* data = nullptr;
    VkResult mapResult = vkMapMemory(m_device, m_skyDomeVBOMemory, 0, vertexSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        return Error(ErrorCode::InitializationFailed, "Failed to map sky dome vertex buffer memory");
    }
    std::memcpy(data, vertices.data(), vertexSize);
    vkUnmapMemory(m_device, m_skyDomeVBOMemory);

    // 创建 IBO
    VkDeviceSize indexSize = indices.size() * sizeof(u16);
    result = _createBuffer(indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_skyDomeIBO,
        m_skyDomeIBOMemory);
    if (result.failed()) {
        return result;
    }

    // 上传数据
    data = nullptr;
    mapResult = vkMapMemory(m_device, m_skyDomeIBOMemory, 0, indexSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        return Error(ErrorCode::InitializationFailed, "Failed to map sky dome index buffer memory");
    }
    std::memcpy(data, indices.data(), indexSize);
    vkUnmapMemory(m_device, m_skyDomeIBOMemory);

    return Result<void>::ok();
}

Result<void> SkyRenderer::_createStarVBO()
{
    // 使用固定种子生成星星位置
    math::Random rng(client::CelestialCalculations::getStarSeed());

    std::vector<SkyVertex> vertices;
    vertices.reserve(client::CelestialCalculations::getStarCount());

    // 在单位球面上均匀分布星星
    for (i32 i = 0; i < client::CelestialCalculations::getStarCount(); ++i) {
        // 使用球面均匀分布算法（Random.nextDouble() 返回 f64，转换为 f64 用于内部计算）
        f64 theta = 2.0 * mc::math::PI_DOUBLE * rng.nextDouble();
        f64 phi = std::acos(2.0 * rng.nextDouble() - 1.0);
        constexpr f64 radius = 100.0; // 星星距离

        f64 x = radius * std::sin(phi) * std::cos(theta);
        f64 y = radius * std::sin(phi) * std::sin(theta);
        f64 z = radius * std::cos(phi);

        // 只保留 y > 0 的星星 (地平线以上)
        if (y > 0.0) {
            vertices.push_back({static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)});
        }
    }

    m_starVertexCount = static_cast<u32>(vertices.size());

    // 创建 VBO
    VkDeviceSize vertexSize = vertices.size() * sizeof(SkyVertex);
    auto result = _createBuffer(vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_starVBO,
        m_starVBOMemory);
    if (result.failed()) {
        return result;
    }

    // 上传数据
    void* data = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, m_starVBOMemory, 0, vertexSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        return Error(ErrorCode::InitializationFailed, "Failed to map star vertex buffer memory");
    }
    std::memcpy(data, vertices.data(), vertexSize);
    vkUnmapMemory(m_device, m_starVBOMemory);

    return Result<void>::ok();
}

Result<void> SkyRenderer::_createSunMoonVBO()
{
    // 太阳和月亮都使用单位四边形 [-1, 1]，并展开为 TriangleList（6 顶点）。
    // 使用 TriangleList 可避免 TriangleStrip 在某些姿态下出现的裂缝/缺口伪影。
    std::vector<SkyVertex> vertices = {
        // 太阳（2 三角形）
        {-1.0f, -1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},

        // 月亮（2 三角形）
        {-1.0f, -1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
    };

    // 创建 VBO
    VkDeviceSize vertexSize = vertices.size() * sizeof(SkyVertex);
    auto result = _createBuffer(vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_sunMoonVBO,
        m_sunMoonVBOMemory);
    if (result.failed()) {
        return result;
    }

    // 上传数据
    void* data = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, m_sunMoonVBOMemory, 0, vertexSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        return Error(ErrorCode::InitializationFailed, "Failed to map sun/moon vertex buffer memory");
    }
    std::memcpy(data, vertices.data(), vertexSize);
    vkUnmapMemory(m_device, m_sunMoonVBOMemory);

    return Result<void>::ok();
}

Result<void> SkyRenderer::_createUniformBuffers()
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto result = _createBuffer(sizeof(SkyUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i],
            m_uniformBuffersMemory[i]);
        if (result.failed()) {
            return result;
        }

        // 持久映射
        void* mapped = nullptr;
        const VkResult mapResult = vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, sizeof(SkyUBO), 0, &mapped);
        if (mapResult != VK_SUCCESS || mapped == nullptr) {
            return Error(ErrorCode::InitializationFailed, "Failed to map sky uniform buffer memory");
        }
        m_uniformBuffersMapped[i] = mapped;
    }
    return Result<void>::ok();
}

Result<void> SkyRenderer::_createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sky descriptor set layout");
    }

    return Result<void>::ok();
}

Result<void> SkyRenderer::_createDescriptorSets()
{
    // 创建描述符池
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sky descriptor pool");
    }

    // 创建描述符集
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    result = vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate sky descriptor sets");
    }

    // 更新描述符集
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyUBO);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }

    return Result<void>::ok();
}

Result<void> SkyRenderer::_createPipelineLayout()
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sky pipeline layout");
    }

    return Result<void>::ok();
}

Result<void> SkyRenderer::_createPipelines(VkSampleCountFlagBits sampleCount)
{
    const VkDevice device = m_device;

    const auto skyVertPath = resolveShaderPath("sky.vert.spv");
    const auto skyFragPath = resolveShaderPath("sky.frag.spv");
    const auto sunVertPath = resolveShaderPath("sun.vert.spv");
    const auto sunFragPath = resolveShaderPath("sun.frag.spv");
    const auto moonVertPath = resolveShaderPath("moon.vert.spv");
    const auto moonFragPath = resolveShaderPath("moon.frag.spv");
    const auto starVertPath = resolveShaderPath("star.vert.spv");
    const auto starFragPath = resolveShaderPath("star.frag.spv");

    if (skyVertPath.empty() || skyFragPath.empty() || sunVertPath.empty() || sunFragPath.empty() ||
        moonVertPath.empty() || moonFragPath.empty() || starVertPath.empty() || starFragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve one or more sky shader binaries");
    }

    auto createPipeline = [this, device, sampleCount](const std::filesystem::path& vertPath,
                              const std::filesystem::path& fragPath,
                              VkPrimitiveTopology topology,
                              VkBool32 blendEnable,
                              VkBool32 depthTestEnable,
                              VkBool32 depthWriteEnable,
                              VkPipeline* outPipeline) -> Result<void> {
        const auto vertPathString = vertPath.string();
        const auto fragPathString = fragPath.string();

        auto vertResult = createShaderModule(device, vertPathString.c_str());
        if (vertResult.failed()) {
            return vertResult.error();
        }

        auto fragResult = createShaderModule(device, fragPathString.c_str());
        if (fragResult.failed()) {
            vkDestroyShaderModule(device, vertResult.value(), nullptr);
            return fragResult.error();
        }

        const VkShaderModule vertModule = vertResult.value();
        const VkShaderModule fragModule = fragResult.value();

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertModule;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragModule;
        shaderStages[1].pName = "main";

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(SkyVertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrDesc{};
        attrDesc.binding = 0;
        attrDesc.location = 0;
        attrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
        attrDesc.offset = 0;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = 1;
        vertexInputInfo.pVertexAttributeDescriptions = &attrDesc;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = sampleCount;
        multisampling.sampleShadingEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = depthTestEnable;
        depthStencil.depthWriteEnable = depthWriteEnable;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = blendEnable;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;

        const VkResult pipelineResult =
            vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, outPipeline);

        vkDestroyShaderModule(device, fragModule, nullptr);
        vkDestroyShaderModule(device, vertModule, nullptr);

        if (pipelineResult != VK_SUCCESS) {
            return Error(ErrorCode::InitializationFailed,
                std::string("Failed to create sky graphics pipeline for shaders: ") + vertPathString + " / " +
                    fragPathString);
        }

        return Result<void>::ok();
    };

    // 天空穹顶
    auto skyResult = createPipeline(
        skyVertPath, skyFragPath, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE, VK_FALSE, VK_FALSE, &m_skyPipeline);
    if (skyResult.failed()) {
        return skyResult.error();
    }

    // 太阳
    auto sunResult = createPipeline(
        sunVertPath, sunFragPath, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_TRUE, VK_FALSE, VK_FALSE, &m_sunPipeline);
    if (sunResult.failed()) {
        return sunResult.error();
    }

    // 月亮
    auto moonResult = createPipeline(
        moonVertPath, moonFragPath, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_TRUE, VK_FALSE, VK_FALSE, &m_moonPipeline);
    if (moonResult.failed()) {
        return moonResult.error();
    }

    // 星星
    auto starResult = createPipeline(
        starVertPath, starFragPath, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_TRUE, VK_FALSE, VK_FALSE, &m_starPipeline);
    if (starResult.failed()) {
        return starResult.error();
    }

    spdlog::info("SkyRenderer pipelines created");
    return Result<void>::ok();
}

void SkyRenderer::_updateUniformBuffer(u32 frameIndex)
{
    SkyUBO ubo = {};

    // 闪电闪烁时天空变亮
    // 当 lightningFlashBrightness > 0 时，天空颜色向白色混合
    glm::vec4 finalSkyColor = m_skyColor;
    if (m_lightningFlashBrightness > 0.0) {
        // 闪电闪烁时，天空颜色向白色混合
        f64 flashFactor = glm::clamp(m_lightningFlashBrightness, 0.0, 1.0);
        finalSkyColor = glm::mix(finalSkyColor, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), static_cast<f32>(flashFactor));
    }

    ubo.skyColor = finalSkyColor;
    ubo.fogColor = m_fogColor;

    // 闪电闪烁时雾颜色也变亮
    glm::vec4 finalFogColor = m_fogColor;
    if (m_lightningFlashBrightness > 0.0) {
        f64 flashFactor = glm::clamp(m_lightningFlashBrightness, 0.0, 1.0);
        finalFogColor = glm::mix(finalFogColor, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), static_cast<f32>(flashFactor));
    }
    ubo.fogColor = finalFogColor;

    ubo.sunriseColor = m_sunriseSunsetColor;
    ubo.sunriseDirection = glm::vec4(m_sunriseDirection, 0.0f);

    glm::dvec3 cameraForward(m_cameraForward.x, m_cameraForward.y, m_cameraForward.z);
    const f64 cameraForwardLen2 = glm::dot(cameraForward, cameraForward);
    if (cameraForwardLen2 > 1e-6) {
        cameraForward /= std::sqrt(cameraForwardLen2);
    } else {
        cameraForward = glm::dvec3(0.0, 0.0, -1.0);
    }
    ubo.cameraForward = glm::vec4(
        static_cast<f32>(cameraForward.x), static_cast<f32>(cameraForward.y), static_cast<f32>(cameraForward.z), 0.0f);

    // sun.vert 在太阳接近天顶/天底时会出现 right=normalize(cross(up, sunDir)) 退化。
    // 这里做极小偏移，避免精确零向量导致太阳四边形退化不可见。
    f64 adjustedAngle = m_celestialAngle;
    const f64 angleRad = adjustedAngle * 2.0 * mc::math::PI_DOUBLE;
    if (std::abs(std::sin(angleRad)) < 1e-4) {
        adjustedAngle += 1e-4;
        if (adjustedAngle >= 1.0) {
            adjustedAngle -= 1.0;
        }
    }

    ubo.celestialAngle = static_cast<f32>(adjustedAngle);
    ubo.starBrightness = static_cast<f32>(m_starBrightness);
    ubo.moonPhase = m_moonPhase;
    ubo.padding = 0.0f;

    // 使用持久映射的内存
    if (m_uniformBuffersMapped[frameIndex] != nullptr) {
        std::memcpy(m_uniformBuffersMapped[frameIndex], &ubo, sizeof(SkyUBO));
    }
}

// ============================================================================
// Vulkan 辅助函数
// ============================================================================

Result<u32> SkyRenderer::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

Result<void> SkyRenderer::_createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    return renderer::VulkanUtils::createBuffer(m_device, m_physicalDevice, size, usage, properties, buffer, memory);
}

VkCommandBuffer SkyRenderer::_beginSingleTimeCommands()
{
    return renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void SkyRenderer::_endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
}

} // namespace mc::client::renderer::trident::sky
