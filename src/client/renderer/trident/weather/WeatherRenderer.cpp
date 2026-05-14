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

#include "WeatherRenderer.hpp"
#include "../../util/ShaderPath.hpp"
#include "../util/VulkanUtils.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::weather {

namespace {

using namespace WeatherRenderConstants;

// 天气 UBO 结构
struct WeatherUBO {
    alignas(16) glm::mat4 projection;
    alignas(16) glm::mat4 view;
    alignas(16) glm::vec3 cameraPos;
    alignas(4) f32 partialTick;
    alignas(4) f32 rainStrength;
    alignas(4) f32 thunderStrength;
};

// 初始化随机偏移数组（参考 MC 1.16.5 WorldRenderer 构造函数）
// MC原版算法：计算从中心向外的归一化方向向量
// rainSizeX[i << 5 | j] = -f1 / f2;  rainSizeZ[i << 5 | j] = f / f2;
// 其中 f = j - 16, f1 = i - 16, f2 = sqrt(f*f + f1*f1)
void initRainOffsets(f64* offsetX, f64* offsetZ, i32 size)
{
    // MC原版使用32x32网格，size应该为32
    MC_ASSERT_RELEASE(size == 32);

    for (i32 i = 0; i < 32; ++i) {
        for (i32 j = 0; j < 32; ++j) {
            // MC原版：f = j - 16, f1 = i - 16
            f32 f = static_cast<f32>(j - 16);    // X方向分量
            f32 f1 = static_cast<f32>(i - 16);   // Z方向分量
            f32 f2 = std::sqrt(f * f + f1 * f1); // 向量长度

            // 避免除以零（中心点）
            if (f2 < 0.0001f) {
                f2 = 1.0f;
            }

            // MC原版索引：i << 5 | j
            i32 idx = (i << 5) | j;
            // MC原版：rainSizeX = -f1 / f2, rainSizeZ = f / f2
            // 这产生从中心向外的归一化方向向量
            offsetX[idx] = -f1 / f2;
            offsetZ[idx] = f / f2;
        }
    }
}

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

Result<VkShaderModule> createShaderModule(VkDevice device, const std::vector<u8>& code)
{
    if (code.size() % 4 != 0) {
        return Error(ErrorCode::InvalidData, "Invalid SPIR-V file size");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create shader module");
    }

    return shaderModule;
}

} // namespace

WeatherRenderer::WeatherRenderer()
{
    // 初始化随机偏移数组
    initRainOffsets(m_rainOffsetX, m_rainOffsetZ, RAIN_SIZE);
}

WeatherRenderer::~WeatherRenderer()
{
    destroy();
}

Result<void> WeatherRenderer::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkExtent2D extent,
    VkSampleCountFlagBits sampleCount)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "WeatherRenderer already initialized");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_extent = extent;

    // 创建资源
    auto result = createVertexBuffer();
    if (!result.success()) {
        return result.error();
    }

    result = createUniformBuffers();
    if (!result.success()) {
        return result.error();
    }

    result = createDescriptorSetLayout();
    if (!result.success()) {
        return result.error();
    }

    result = createDescriptorPool();
    if (!result.success()) {
        return result.error();
    }

    result = createDescriptorSets();
    if (!result.success()) {
        return result.error();
    }

    result = createTextures();
    if (!result.success()) {
        return result.error();
    }

    result = createPipelineLayout();
    if (!result.success()) {
        return result.error();
    }

    result = createPipelines(sampleCount);
    if (!result.success()) {
        return result.error();
    }

    m_initialized = true;
    spdlog::info("WeatherRenderer initialized successfully");
    return {};
}

void WeatherRenderer::destroy()
{
    if (!m_initialized) {
        return;
    }

    vkDeviceWaitIdle(m_device);

    // 销毁管线
    if (m_rainPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_rainPipeline, nullptr);
        m_rainPipeline = VK_NULL_HANDLE;
    }

    if (m_snowPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_snowPipeline, nullptr);
        m_snowPipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // 销毁纹理
    if (m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }

    if (m_rainTextureView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_rainTextureView, nullptr);
        m_rainTextureView = VK_NULL_HANDLE;
    }

    if (m_rainTexture != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_rainTexture, nullptr);
        m_rainTexture = VK_NULL_HANDLE;
    }

    if (m_rainTextureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_rainTextureMemory, nullptr);
        m_rainTextureMemory = VK_NULL_HANDLE;
    }

    if (m_snowTextureView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_snowTextureView, nullptr);
        m_snowTextureView = VK_NULL_HANDLE;
    }

    if (m_snowTexture != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_snowTexture, nullptr);
        m_snowTexture = VK_NULL_HANDLE;
    }

    if (m_snowTextureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_snowTextureMemory, nullptr);
        m_snowTextureMemory = VK_NULL_HANDLE;
    }

    // 销毁描述符
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    // 销毁 Uniform 缓冲区
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (m_uniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(m_device, m_uniformBuffersMemory[i]);
            m_uniformBuffersMapped[i] = nullptr;
        }

        if (m_uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
            m_uniformBuffers[i] = VK_NULL_HANDLE;
        }

        if (m_uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
            m_uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }

    // 销毁顶点缓冲区
    if (m_vertexBufferMapped != nullptr) {
        vkUnmapMemory(m_device, m_vertexBufferMemory);
        m_vertexBufferMapped = nullptr;
    }

    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    m_initialized = false;
}

Result<void> WeatherRenderer::onResize(VkExtent2D extent)
{
    m_extent = extent;
    return {};
}

void WeatherRenderer::setFancyGraphics(bool isFancy)
{
    // 参考 MC 1.16.5 WorldRenderer.renderRainSnow()
    // int l = 5; if (Minecraft.isFancyGraphicsEnabled()) { l = 10; }
    m_renderRadius = isFancy ? 10 : 5;
}

void WeatherRenderer::update(f64 rainStrength, f64 thunderStrength, i64 ticks, f64 partialTick)
{
    m_rainStrength = rainStrength;
    m_thunderStrength = thunderStrength;
    m_ticks = ticks;
    m_partialTick = partialTick;
}

void WeatherRenderer::render(
    VkCommandBuffer cmd, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos, u32 frameIndex)
{
    // 无 World 的简化渲染
    mc::client::ClientWorld* nullWorld = nullptr;
    render(cmd, projection, view, cameraPos, frameIndex, nullWorld);
}

void WeatherRenderer::render(VkCommandBuffer cmd,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos,
    u32 frameIndex,
    mc::client::ClientWorld& world)
{
    render(cmd, projection, view, cameraPos, frameIndex, &world);
}

void WeatherRenderer::render(VkCommandBuffer cmd,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos,
    u32 frameIndex,
    const mc::math::frustum::Frustum& frustum)
{
    m_frustum = &frustum;
    render(cmd, projection, view, cameraPos, frameIndex, static_cast<mc::client::ClientWorld*>(nullptr));
    m_frustum = nullptr;
}

void WeatherRenderer::render(VkCommandBuffer cmd,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos,
    u32 frameIndex,
    mc::client::ClientWorld& world,
    const mc::math::frustum::Frustum& frustum)
{
    m_frustum = &frustum;
    render(cmd, projection, view, cameraPos, frameIndex, &world);
    m_frustum = nullptr;
}

void WeatherRenderer::render(VkCommandBuffer cmd,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos,
    u32 frameIndex,
    mc::client::ClientWorld* world)
{
    if (m_rainStrength <= WeatherRenderConstants::MIN_RENDER_STRENGTH) {
        return; // 不下雨/雪，不渲染
    }

    if (!m_initialized) {
        return;
    }

    MC_TRACE_EVENT_BEGIN("rendering.weather", "WeatherRenderer::render");

    m_cameraPos = cameraPos;
    m_currentProjection = projection;
    m_currentView = view;

    // 更新 Uniform 缓冲区
    updateUniformBuffer(frameIndex);

    // 生成天气几何
    generateWeatherGeometry(world);

    if (m_rainVertexCount == 0 && m_snowVertexCount == 0) {
        MC_TRACE_EVENT_END("rendering.weather");
        return;
    }

    // 更新顶点缓冲区
    size_t totalVertices = m_rainVertices.size() + m_snowVertices.size();
    if (totalVertices > 0) {
        VkDeviceSize size = totalVertices * sizeof(WeatherVertex);

#ifdef __APPLE__
        // macOS + MoltenVK 上不要重复映射同一段内存，直接使用初始化阶段建立的持久映射。
        void* data = m_vertexBufferMapped;
        if (data == nullptr) {
            spdlog::error("WeatherRenderer: vertex buffer is not persistently mapped on Apple");
            MC_TRACE_EVENT_END("rendering.weather");
            return;
        }
#else
        void* data = nullptr;
        const VkResult mapResult = vkMapMemory(m_device, m_vertexBufferMemory, 0, size, 0, &data);
        if (mapResult != VK_SUCCESS || data == nullptr) {
            spdlog::error("WeatherRenderer: failed to map vertex buffer memory: {}", static_cast<i32>(mapResult));
            MC_TRACE_EVENT_END("rendering.weather");
            return;
        }
#endif

        // 复制雨顶点
        if (!m_rainVertices.empty()) {
            std::memcpy(data, m_rainVertices.data(), m_rainVertices.size() * sizeof(WeatherVertex));
        }

        // 复制雪顶点
        if (!m_snowVertices.empty()) {
            std::memcpy(static_cast<u8*>(data) + m_rainVertices.size() * sizeof(WeatherVertex),
                m_snowVertices.data(),
                m_snowVertices.size() * sizeof(WeatherVertex));
        }

#ifndef __APPLE__
        vkUnmapMemory(m_device, m_vertexBufferMemory);
#endif
    }

    // 渲染雨
    if (m_rainVertexCount > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_rainPipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[frameIndex], 0, nullptr);

        VkBuffer vertexBuffers[] = {m_vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(cmd, m_rainVertexCount, 1, 0, 0);
    }

    // 渲染雪
    if (m_snowVertexCount > 0) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_snowPipeline);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[frameIndex], 0, nullptr);

        VkBuffer vertexBuffers[] = {m_vertexBuffer};
        VkDeviceSize offsets[] = {m_rainVertices.size() * sizeof(WeatherVertex)};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(cmd, m_snowVertexCount, 1, 0, 0);
    }

    MC_TRACE_EVENT_END("rendering.weather");
}

void WeatherRenderer::generateWeatherGeometry(mc::client::ClientWorld* world)
{
    m_rainVertices.clear();
    m_snowVertices.clear();
    m_rainVertexCount = 0;
    m_snowVertexCount = 0;

    if (m_rainStrength <= WeatherRenderConstants::MIN_RENDER_STRENGTH) {
        return;
    }

    // 参考 MC 1.16.5 WorldRenderer.renderRainSnow()
    // 遍历玩家周围的区块

    i32 camX = static_cast<i32>(std::floor(m_cameraPos.x));
    i32 camZ = static_cast<i32>(std::floor(m_cameraPos.z));
    i32 camY = static_cast<i32>(std::floor(m_cameraPos.y));

    f64 f1 = static_cast<f64>(m_ticks) + m_partialTick;

    // 渲染范围
    i32 radius = m_renderRadius;

    for (i32 z = camZ - radius; z <= camZ + radius; ++z) {
        for (i32 x = camX - radius; x <= camX + radius; ++x) {
            // 计算偏移索引
            i32 idx = ((z - camZ + 16) * RAIN_SIZE + (x - camX + 16)) % (RAIN_SIZE * RAIN_SIZE);
            if (idx < 0) idx += RAIN_SIZE * RAIN_SIZE;

            // MC原版: double d0 = (double)this.rainSizeX[l1] * 0.5D;
            // 偏移值需要乘以0.5进行缩放
            f64 offsetX = m_rainOffsetX[idx] * 0.5;
            f64 offsetZ = m_rainOffsetZ[idx] * 0.5;

            // 视锥剔除：使用球体测试检查位置是否可见
            if (m_frustum && m_frustum->isValid()) {
                glm::vec3 center(
                    static_cast<f32>(x) + 0.5f, static_cast<f32>(m_cameraPos.y), static_cast<f32>(z) + 0.5f);
                f32 sphereRadius = 25.0f;

                if (!m_frustum->isSphereVisible(center, sphereRadius)) {
                    continue;
                }
            }

            // 获取生物群系温度决定雨/雪
            f32 temperature = 0.5f; // 默认温度
            i32 groundY = 64;       // 默认地面高度

            if (world) {
                // 查询生物群系
                const mc::Biome* biome = world->getBiomeAtBlock(x, camY, z);
                if (biome) {
                    // 检查生物群系是否允许降水
                    if (biome->climate().precipitation == mc::BiomeClimate::Precipitation::None) {
                        continue; // 该生物群系不降水（如沙漠）
                    }
                }

                // 查询地形高度（使用 MOTION_BLOCKING 高度图）
                groundY = world->getHeight(x, z);
            }

            // MC原版高度计算:
            // int i2 = world.getHeight(Heightmap.Type.MOTION_BLOCKING, pos).getY();
            // int j2 = j - l; (相机Y - 半径)
            // int k2 = j + l; (相机Y + 半径)
            // if (j2 < i2) j2 = i2; (下边界不低于地形)
            // if (k2 < i2) k2 = i2; (上边界不低于地形)
            // int l2 = i2; if (i2 < j) l2 = j; (光照采样高度 = max(地形高度, 相机Y))
            i32 j2 = camY - radius; // 下边界初始值
            i32 k2 = camY + radius; // 上边界初始值

            if (j2 < groundY) j2 = groundY; // 下边界不低于地形
            if (k2 < groundY) k2 = groundY; // 上边界不低于地形

            // 如果上下边界相同，跳过（没有渲染空间）
            if (j2 == k2) {
                continue;
            }

            // l2 = 光照采样高度 = max(groundY, cameraY)，也用于温度采样
            i32 l2 = groundY;
            if (groundY < camY) l2 = camY;

            // 使用位置相关温度（考虑海拔影响）
            if (world) {
                const mc::Biome* biome = world->getBiomeAtBlock(x, camY, z);
                if (biome) {
                    temperature = biome->getTemperature(l2);
                }
            }

            // 计算到相机的距离，用于淡出
            // MC原版: float f4 = MathHelper.sqrt(d2*d2 + d4*d4) / (float)l;
            // d2 = k1 - d0, d4 = j1 - d2 (相对于相机的距离)
            f64 dx = static_cast<f64>(x) + 0.5 - m_cameraPos.x;
            f64 dz = static_cast<f64>(z) + 0.5 - m_cameraPos.z;
            f64 dist = std::sqrt(dx * dx + dz * dz);
            f64 f4 = dist / static_cast<f64>(radius); // 归一化距离比 (0~1)

            // MC原版 alpha 计算:
            // 雨: float f5 = ((1.0F - f4*f4) * 0.5F + 0.5F) * f;
            // 雪: float f10 = ((1.0F - f9*f9) * 0.3F + 0.5F) * f;
            f64 rainFade = (1.0 - f4 * f4) * 0.5 + 0.5;
            f64 alpha = rainFade * m_rainStrength;

            // MC原版位置种子计算（雨和雪共用）
            i64 positionSeed = static_cast<i64>(x) * static_cast<i64>(x) * 3121 + static_cast<i64>(x) * 45238971 +
                static_cast<i64>(z) * static_cast<i64>(z) * 418711 + static_cast<i64>(z) * 13761;

            // 温度阈值判断：低于 SNOW_TEMPERATURE_THRESHOLD 为雪，高于等于为雨
            // 参考 MC 1.16.5 Biome.getPrecipitation()
            if (temperature >= SNOW_TEMPERATURE_THRESHOLD) {
                // 雨
                // MC原版 UV动画计算:
                // int i3 = this.ticks + k1*k1*3121 + k1*45238971 + j1*j1*418711 + j1*13761 & 31;
                // float f3 = -((float)i3 + partialTicks) / 32.0F * (3.0F + random.nextFloat());
                mc::math::Random rng(static_cast<u64>(positionSeed));
                i32 i3 = (static_cast<i32>(m_ticks) + static_cast<i32>(positionSeed & 0x7FFFFFFF)) & 31;
                f32 texOffset =
                    -((static_cast<f32>(i3) + static_cast<f32>(m_partialTick)) / 32.0f) * (3.0f + rng.nextFloat());

                // 光照采样：参考 MC 1.16.5 WorldRenderer.renderRainSnow()
                // MC使用 l2 = max(groundY, cameraY) 作为采样高度
                u16 lightU = 240;
                u16 lightV = 240;

                if (world) {
                    u8 skyLight = world->getSkyLight(x, l2, z);
                    u8 blockLight = world->getBlockLight(x, l2, z);

                    // MC原版: lightmap U = blockLight, V = skyLight
                    // 我们存储 0-15 范围的光照值，需要乘以 16 得到 0-240 范围
                    lightU = static_cast<u16>(blockLight) << 4; // U = blockLight
                    lightV = static_cast<u16>(skyLight) << 4;   // V = skyLight
                }

                // 使用MC原版的高度计算 (j2 = 下边界, k2 = 上边界)
                f64 topY = static_cast<f64>(k2);
                f64 bottomY = static_cast<f64>(j2);

                WeatherVertex v0, v1, v2, v3;

                v0.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x - offsetX + 0.5);
                v0.y = static_cast<f32>(topY - m_cameraPos.y);
                v0.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z - offsetZ + 0.5);
                v0.u = 0.0f;
                v0.v = static_cast<f32>(bottomY * 0.25 + texOffset);
                v0.r = 1.0f;
                v0.g = 1.0f;
                v0.b = 1.0f;
                v0.a = static_cast<f32>(alpha);
                v0.lightU = lightU;
                v0.lightV = lightV;

                v1.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x + offsetX + 0.5);
                v1.y = static_cast<f32>(topY - m_cameraPos.y);
                v1.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z + offsetZ + 0.5);
                v1.u = 1.0f;
                v1.v = static_cast<f32>(bottomY * 0.25 + texOffset);
                v1.r = 1.0f;
                v1.g = 1.0f;
                v1.b = 1.0f;
                v1.a = static_cast<f32>(alpha);
                v1.lightU = lightU;
                v1.lightV = lightV;

                v2.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x + offsetX + 0.5);
                v2.y = static_cast<f32>(bottomY - m_cameraPos.y);
                v2.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z + offsetZ + 0.5);
                v2.u = 1.0f;
                v2.v = static_cast<f32>(topY * 0.25 + texOffset);
                v2.r = 1.0f;
                v2.g = 1.0f;
                v2.b = 1.0f;
                v2.a = static_cast<f32>(alpha);
                v2.lightU = lightU;
                v2.lightV = lightV;

                v3.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x - offsetX + 0.5);
                v3.y = static_cast<f32>(bottomY - m_cameraPos.y);
                v3.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z - offsetZ + 0.5);
                v3.u = 0.0f;
                v3.v = static_cast<f32>(topY * 0.25 + texOffset);
                v3.r = 1.0f;
                v3.g = 1.0f;
                v3.b = 1.0f;
                v3.a = static_cast<f32>(alpha);
                v3.lightU = lightU;
                v3.lightV = lightV;

                m_rainVertices.push_back(v0);
                m_rainVertices.push_back(v1);
                m_rainVertices.push_back(v2);
                m_rainVertices.push_back(v0);
                m_rainVertices.push_back(v2);
                m_rainVertices.push_back(v3);
            } else {
                // 雪
                // MC原版 UV动画计算:
                // float f6 = -((float)(this.ticks & 511) + partialTicks) / 512.0F;
                // float f7 = (float)(random.nextDouble() + (double)f1 * 0.01D *
                // (double)((float)random.nextGaussian())); float f8 = (float)(random.nextDouble() + (double)(f1 *
                // (float)random.nextGaussian()) * 0.001D);
                f32 texOffsetY = -((static_cast<f32>(m_ticks & 511) + static_cast<f32>(m_partialTick)) / 512.0f);

                // 重用已计算的位置种子
                mc::math::Random rng(static_cast<u64>(positionSeed));
                f32 texOffsetX = static_cast<f32>(
                    rng.nextDouble() + static_cast<f64>(f1) * 0.01 * static_cast<f64>(rng.nextGaussian()));
                f32 texOffsetYExtra = static_cast<f32>(
                    rng.nextDouble() + static_cast<f64>(f1 * static_cast<f32>(rng.nextGaussian())) * 0.001);

                // MC原版 alpha 计算:
                // float f9 = MathHelper.sqrt(d3*d3 + d5*d5) / (float)l;
                // float f10 = ((1.0F - f9*f9) * 0.3F + 0.5F) * f;
                f64 snowFade = (1.0 - f4 * f4) * 0.3 + 0.5;
                f64 snowAlpha = snowFade * m_rainStrength;

                // 光照采样：参考 MC 1.16.5 WorldRenderer.renderRainSnow()
                // 雪花需要更亮的光照效果
                // MC原版: int k3 = getCombinedLight(world, blockpos$mutable);
                // int l3 = k3 >> 16 & '￿';  (skyLight 0-240)
                // int i4 = (k3 & '￿') * 3;  (blockLight * 3)
                // int j4 = (l3 * 3 + 240) / 4;  (增强 skyLight)
                // int k4 = (i4 * 3 + 240) / 4;  (blockLight * 3 * 3 + 240) / 4
                u16 lightU = 240;
                u16 lightV = 240;

                if (world) {
                    u8 skyLight = world->getSkyLight(x, l2, z);
                    u8 blockLight = world->getBlockLight(x, l2, z);

                    u16 rawSkyLight = static_cast<u16>(skyLight) << 4;     // 0-240
                    u16 rawBlockLight = static_cast<u16>(blockLight) << 4; // 0-240

                    // MC原版公式:
                    // j4 = (l3 * 3 + 240) / 4  用于 V (skyLight)
                    // k4 = (i4 * 3 + 240) / 4  用于 U (blockLight * 9 + 240) / 4
                    // 其中 i4 = blockLight * 3, 所以 k4 = (blockLight * 9 + 240) / 4
                    lightV = static_cast<u16>((rawSkyLight * 3 + 240) / 4);
                    lightU = static_cast<u16>((rawBlockLight * 9 + 240) / 4);
                }

                // 使用MC原版的高度计算 (j2 = 下边界, k2 = 上边界)
                f64 topY = static_cast<f64>(k2);
                f64 bottomY = static_cast<f64>(j2);

                WeatherVertex v0, v1, v2, v3;

                v0.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x - offsetX + 0.5);
                v0.y = static_cast<f32>(topY - m_cameraPos.y);
                v0.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z - offsetZ + 0.5);
                v0.u = static_cast<f32>(0.0 + texOffsetX);
                v0.v = static_cast<f32>(bottomY * 0.25 + texOffsetY + texOffsetYExtra);
                v0.r = 1.0f;
                v0.g = 1.0f;
                v0.b = 1.0f;
                v0.a = static_cast<f32>(snowAlpha);
                v0.lightU = lightU;
                v0.lightV = lightV;

                v1.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x + offsetX + 0.5);
                v1.y = static_cast<f32>(topY - m_cameraPos.y);
                v1.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z + offsetZ + 0.5);
                v1.u = static_cast<f32>(1.0 + texOffsetX);
                v1.v = static_cast<f32>(bottomY * 0.25 + texOffsetY + texOffsetYExtra);
                v1.r = 1.0f;
                v1.g = 1.0f;
                v1.b = 1.0f;
                v1.a = static_cast<f32>(snowAlpha);
                v1.lightU = lightU;
                v1.lightV = lightV;

                v2.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x + offsetX + 0.5);
                v2.y = static_cast<f32>(bottomY - m_cameraPos.y);
                v2.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z + offsetZ + 0.5);
                v2.u = static_cast<f32>(1.0 + texOffsetX);
                v2.v = static_cast<f32>(topY * 0.25 + texOffsetY + texOffsetYExtra);
                v2.r = 1.0f;
                v2.g = 1.0f;
                v2.b = 1.0f;
                v2.a = static_cast<f32>(snowAlpha);
                v2.lightU = lightU;
                v2.lightV = lightV;

                v3.x = static_cast<f32>(static_cast<f64>(x) - m_cameraPos.x - offsetX + 0.5);
                v3.y = static_cast<f32>(bottomY - m_cameraPos.y);
                v3.z = static_cast<f32>(static_cast<f64>(z) - m_cameraPos.z - offsetZ + 0.5);
                v3.u = static_cast<f32>(0.0 + texOffsetX);
                v3.v = static_cast<f32>(topY * 0.25 + texOffsetY + texOffsetYExtra);
                v3.r = 1.0f;
                v3.g = 1.0f;
                v3.b = 1.0f;
                v3.a = static_cast<f32>(snowAlpha);
                v3.lightU = lightU;
                v3.lightV = lightV;

                m_snowVertices.push_back(v0);
                m_snowVertices.push_back(v1);
                m_snowVertices.push_back(v2);
                m_snowVertices.push_back(v0);
                m_snowVertices.push_back(v2);
                m_snowVertices.push_back(v3);
            }
        }
    }

    m_rainVertexCount = static_cast<u32>(m_rainVertices.size());
    m_snowVertexCount = static_cast<u32>(m_snowVertices.size());
}

Result<void> WeatherRenderer::createVertexBuffer()
{
    // 创建动态顶点缓冲区（足够大以容纳最大顶点数）
    m_vertexBufferSize = sizeof(WeatherVertex) * MAX_RAIN_VERTICES;

    auto result = ::mc::client::renderer::VulkanUtils::createBuffer(m_device,
        m_physicalDevice,
        m_vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory);

    if (!result.success()) {
        return result.error();
    }

    // 持久映射
    void* data = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, m_vertexBufferMemory, 0, m_vertexBufferSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
        return Error(ErrorCode::InitializationFailed, "Failed to map weather vertex buffer memory");
    }
    m_vertexBufferMapped = data;

    return {};
}

Result<void> WeatherRenderer::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(WeatherUBO);

    u32 createdBuffers = 0;
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto result = ::mc::client::renderer::VulkanUtils::createBuffer(m_device,
            m_physicalDevice,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i],
            m_uniformBuffersMemory[i]);

        if (!result.success()) {
            return result.error();
        }

        void* mapped = nullptr;
        const VkResult mapResult = vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0, &mapped);
        if (mapResult != VK_SUCCESS || mapped == nullptr) {
            for (u32 j = 0; j < createdBuffers; ++j) {
                if (m_uniformBuffersMapped[j] != nullptr) {
                    vkUnmapMemory(m_device, m_uniformBuffersMemory[j]);
                    m_uniformBuffersMapped[j] = nullptr;
                }

                if (m_uniformBuffers[j] != VK_NULL_HANDLE) {
                    vkDestroyBuffer(m_device, m_uniformBuffers[j], nullptr);
                    m_uniformBuffers[j] = VK_NULL_HANDLE;
                }

                if (m_uniformBuffersMemory[j] != VK_NULL_HANDLE) {
                    vkFreeMemory(m_device, m_uniformBuffersMemory[j], nullptr);
                    m_uniformBuffersMemory[j] = VK_NULL_HANDLE;
                }
            }

            vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
            vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
            m_uniformBuffers[i] = VK_NULL_HANDLE;
            m_uniformBuffersMemory[i] = VK_NULL_HANDLE;
            return Error(ErrorCode::InitializationFailed, "Failed to map weather uniform buffer memory");
        }

        m_uniformBuffersMapped[i] = mapped;
        ++createdBuffers;
    }

    return {};
}

Result<void> WeatherRenderer::createDescriptorSetLayout()
{
    // Binding 0: Uniform Buffer
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    // Binding 1: Texture Sampler
    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, samplerBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor set layout");
    }

    return {};
}

Result<void> WeatherRenderer::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * 2; // 雨和雪两个纹理

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT * 2;
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor pool");
    }

    return {};
}

Result<void> WeatherRenderer::createDescriptorSets()
{
    // 创建雨和雪两套描述符集
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT * 2> layouts = {
        m_descriptorSetLayout, m_descriptorSetLayout, m_descriptorSetLayout, m_descriptorSetLayout};

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT; // 先创建第一套
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate descriptor sets");
    }

    // 更新描述符集（先更新 Uniform Buffer，纹理在创建后再更新）
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(WeatherUBO);

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

    return {};
}

Result<void> WeatherRenderer::createPipelineLayout()
{
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create pipeline layout");
    }

    return {};
}

Result<void> WeatherRenderer::createPipelines(VkSampleCountFlagBits sampleCount)
{
    // 加载 shader
    auto vertPath = resolveShaderPath("weather.vert.spv");
    auto fragPath = resolveShaderPath("weather.frag.spv");

    if (vertPath.empty() || fragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve weather shader binaries");
    }

    auto vertShaderResult = readBinaryFile(vertPath.string().c_str());
    if (!vertShaderResult.success()) {
        return vertShaderResult.error();
    }

    auto fragShaderResult = readBinaryFile(fragPath.string().c_str());
    if (!fragShaderResult.success()) {
        return fragShaderResult.error();
    }

    const auto& vertShaderCode = vertShaderResult.value();
    const auto& fragShaderCode = fragShaderResult.value();

    // 创建 shader 模块
    auto vertShaderModuleResult = createShaderModule(m_device, vertShaderCode);
    if (!vertShaderModuleResult.success()) {
        return vertShaderModuleResult.error();
    }
    VkShaderModule vertShaderModule = vertShaderModuleResult.value();

    auto fragShaderModuleResult = createShaderModule(m_device, fragShaderCode);
    if (!fragShaderModuleResult.success()) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        return fragShaderModuleResult.error();
    }
    VkShaderModule fragShaderModule = fragShaderModuleResult.value();

    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 顶点输入
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(WeatherVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 4> attributeDescs = {};

    // position
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = offsetof(WeatherVertex, x);

    // texCoord
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[1].offset = offsetof(WeatherVertex, u);

    // color
    attributeDescs[2].binding = 0;
    attributeDescs[2].location = 2;
    attributeDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[2].offset = offsetof(WeatherVertex, r);

    // lightmap
    attributeDescs[3].binding = 0;
    attributeDescs[3].location = 3;
    attributeDescs[3].format = VK_FORMAT_R16G16_UNORM;
    attributeDescs[3].offset = offsetof(WeatherVertex, lightU);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态设置）
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // 双面渲染
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;

    // 深度/模板
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE; // 天气不写入深度
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 颜色混合（半透明）
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 动态状态
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 创建雨管线
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
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

    // 先创建雨管线
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_rainPipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create rain pipeline");
    }

    // 雪管线使用相同的着色器和配置
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_snowPipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create snow pipeline");
    }

    // 清理 shader 模块
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);

    return {};
}

Result<void> WeatherRenderer::createTextures()
{
    // 生成雨纹理
    auto rainData = generateRainTexture(TEXTURE_SIZE, TEXTURE_SIZE);
    auto result = createTextureFromData(
        rainData, TEXTURE_SIZE, TEXTURE_SIZE, m_rainTexture, m_rainTextureMemory, m_rainTextureView);
    if (!result.success()) {
        return result.error();
    }

    // 生成雪纹理
    auto snowData = generateSnowTexture(TEXTURE_SIZE, TEXTURE_SIZE);
    result = createTextureFromData(
        snowData, TEXTURE_SIZE, TEXTURE_SIZE, m_snowTexture, m_snowTextureMemory, m_snowTextureView);
    if (!result.success()) {
        return result.error();
    }

    // 创建采样器
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create weather texture sampler");
    }

    // 更新描述符集绑定纹理
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_rainTextureView; // 默认绑定雨纹理
        imageInfo.sampler = m_textureSampler;

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }

    return {};
}

void WeatherRenderer::updateUniformBuffer(u32 frameIndex)
{
    WeatherUBO ubo = {};
    ubo.projection = m_currentProjection;
    ubo.view = m_currentView;
    ubo.cameraPos = m_cameraPos;
    ubo.partialTick = static_cast<f32>(m_partialTick);
    ubo.rainStrength = static_cast<f32>(m_rainStrength);
    ubo.thunderStrength = static_cast<f32>(m_thunderStrength);

    std::memcpy(m_uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}

std::vector<u8> WeatherRenderer::generateRainTexture(u32 width, u32 height)
{
    std::vector<u8> data(width * height * 4, 0);

    // 生成简单的雨滴纹理（细长条纹）
    mc::math::Random rng(12345);

    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * 4;

            // 雨滴是垂直条纹
            f64 xNorm = static_cast<f64>(x) / width;
            f64 yNorm = static_cast<f64>(y) / height;

            // 创建多个垂直条纹
            f64 stripe = 0.0f;
            for (int i = 0; i < 4; ++i) {
                f64 stripeX = 0.2f + i * 0.2f + rng.nextFloat() * 0.05f;
                f64 distance = std::abs(xNorm - stripeX);
                if (distance < 0.02f) {
                    stripe = 1.0f - distance / 0.02f;
                    break;
                }
            }

            // 渐变效果（从上到下）
            f64 gradient = 1.0f - yNorm * 0.3f;

            f64 alpha = stripe * gradient * 0.7f;

            data[idx + 0] = 200;                          // R
            data[idx + 1] = 220;                          // G
            data[idx + 2] = 255;                          // B
            data[idx + 3] = static_cast<u8>(alpha * 255); // A
        }
    }

    return data;
}

std::vector<u8> WeatherRenderer::generateSnowTexture(u32 width, u32 height)
{
    std::vector<u8> data(width * height * 4, 0);

    // 生成雪花纹理（圆形斑点）
    mc::math::Random rng(54321);

    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * 4;

            f64 xNorm = static_cast<f64>(x) / width;
            f64 yNorm = static_cast<f64>(y) / height;

            // 创建多个圆形雪花
            f64 snow = 0.0f;
            for (int i = 0; i < 5; ++i) {
                f64 cx = rng.nextFloat();
                f64 cy = rng.nextFloat();
                f64 radius = 0.05f + rng.nextFloat() * 0.1f;

                f64 dx = xNorm - cx;
                f64 dy = yNorm - cy;
                f64 distance = std::sqrt(dx * dx + dy * dy);

                if (distance < radius) {
                    snow = std::max(snow, 1.0f - distance / radius);
                }
            }

            f64 alpha = snow * 0.9f;

            data[idx + 0] = 255;                          // R
            data[idx + 1] = 255;                          // G
            data[idx + 2] = 255;                          // B
            data[idx + 3] = static_cast<u8>(alpha * 255); // A
        }
    }

    return data;
}

Result<void> WeatherRenderer::createTextureFromData(
    const std::vector<u8>& data, u32 width, u32 height, VkImage& image, VkDeviceMemory& memory, VkImageView& imageView)
{
    // 创建 staging buffer
    VkDeviceSize imageSize = width * height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    auto result = ::mc::client::renderer::VulkanUtils::createBuffer(m_device,
        m_physicalDevice,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);
    if (!result.success()) {
        return result.error();
    }

    void* mappedData = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
    if (mapResult != VK_SUCCESS || mappedData == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to map weather texture staging buffer");
    }
    std::memcpy(mappedData, data.data(), imageSize);
    vkUnmapMemory(m_device, stagingBufferMemory);

    // 创建图像
    result = ::mc::client::renderer::VulkanUtils::createImage(m_device,
        m_physicalDevice,
        width,
        height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory);
    if (!result.success()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return result.error();
    }

    // 转换图像布局并复制
    VkCommandBuffer cmd = ::mc::client::renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);

    VulkanUtils::transitionImageLayout(cmd,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    VulkanUtils::copyBufferToImage(cmd, stagingBuffer, image, width, height);

    VulkanUtils::transitionImageLayout(cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);

    // 清理 staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // 创建图像视图
    result = ::mc::client::renderer::VulkanUtils::createImageView(
        m_device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, imageView);
    if (!result.success()) {
        return result.error();
    }

    return {};
}

} // namespace mc::client::renderer::trident::weather
