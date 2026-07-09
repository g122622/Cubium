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

#include "ParticleManager.hpp"
#include "ParticleRegistry.hpp"
#include "client/renderer/util/ShaderPath.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include <spdlog/spdlog.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace mc::client::renderer::trident::particle {

namespace {

// 粒子纹理尺寸
constexpr u32 PARTICLE_TEXTURE_SIZE = 256;

// 每个粒子使用 4 个顶点（quad）
constexpr u32 VERTICES_PER_PARTICLE = 4;

// 每个粒子使用 6 个索引（2 个三角形）
constexpr u32 INDICES_PER_PARTICLE = 6;

// 生成默认粒子纹理数据（简单白色圆形）
std::vector<u8> generateDefaultParticleTexture(u32 width, u32 height)
{
    std::vector<u8> data(width * height * 4, 0);
    f64 centerX = width / 2.0;
    f64 centerY = height / 2.0;
    f64 radius = std::min(centerX, centerY) * 0.8;

    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            f64 dx = static_cast<f64>(x) - centerX;
            f64 dy = static_cast<f64>(y) - centerY;
            f64 dist = std::sqrt(dx * dx + dy * dy);

            size_t idx = (y * width + x) * 4;
            if (dist <= radius) {
                // 白色，带圆滑边缘 alpha
                const f64 edge0 = radius * 0.7;
                const f64 edge1 = radius;
                const f64 t = std::clamp((dist - edge0) / (edge1 - edge0), 0.0, 1.0);
                const f64 smooth = t * t * (3.0 - 2.0 * t);
                f64 alpha = 1.0 - smooth;
                data[idx + 0] = 255;                          // R
                data[idx + 1] = 255;                          // G
                data[idx + 2] = 255;                          // B
                data[idx + 3] = static_cast<u8>(alpha * 255); // A
            } else {
                data[idx + 0] = 0;
                data[idx + 1] = 0;
                data[idx + 2] = 0;
                data[idx + 3] = 0;
            }
        }
    }

    return data;
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

ParticleManager::ParticleManager() = default;

ParticleManager::~ParticleManager()
{
    destroy();
}

Result<void> ParticleManager::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    VkExtent2D extent,
    VkSampleCountFlagBits sampleCount)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "ParticleManager already initialized");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_extent = extent;

    // 预分配粒子容器
    m_particles.reserve(MAX_PARTICLES);
    m_vertexData.reserve(MAX_PARTICLES * VERTICES_PER_PARTICLE);

    // 创建资源（注意：纹理需要在描述符集之前创建）
    auto result = _createVertexBuffer();
    if (!result.success()) {
        return result.error();
    }

    result = _createUniformBuffers();
    if (!result.success()) {
        return result.error();
    }

    result = _createDescriptorSetLayout();
    if (!result.success()) {
        return result.error();
    }

    result = _createDescriptorPool();
    if (!result.success()) {
        return result.error();
    }

    // 先创建纹理，因为描述符集需要纹理视图和采样器
    result = _createTexture();
    if (!result.success()) {
        return result.error();
    }

    // 然后创建描述符集（需要纹理视图和采样器）
    result = _createDescriptorSets();
    if (!result.success()) {
        return result.error();
    }

    result = _createPipelineLayout();
    if (!result.success()) {
        return result.error();
    }

    result = _createPipelines(sampleCount);
    if (!result.success()) {
        return result.error();
    }

    m_initialized = true;
    spdlog::info("ParticleManager initialized successfully");
    return {};
}

void ParticleManager::destroy()
{
    if (!m_initialized) {
        return;
    }

    // 等待设备空闲
    vkDeviceWaitIdle(m_device);

    // 销毁管线
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
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

    if (m_textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_textureImageView, nullptr);
        m_textureImageView = VK_NULL_HANDLE;
    }

    if (m_textureImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_textureImage, nullptr);
        m_textureImage = VK_NULL_HANDLE;
    }

    if (m_textureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_textureImageMemory, nullptr);
        m_textureImageMemory = VK_NULL_HANDLE;
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

    // 销毁顶点和索引缓冲区
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }

    if (m_indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
        m_indexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    // 清空粒子
    m_particles.clear();
    m_pendingParticles.clear();
    m_vertexData.clear();

    m_initialized = false;
}

Result<void> ParticleManager::onResize(VkExtent2D extent)
{
    m_extent = extent;
    return {};
}

void ParticleManager::addParticle(std::unique_ptr<Particle> particle)
{
    if (m_particles.size() < MAX_PARTICLES && particle) {
        m_particles.push_back(std::move(particle));
    }
}

void ParticleManager::addPendingParticle(
    ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity, ClientWorld* world)
{
    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!shouldShowParticle(type)) {
        return;
    }

    // 粒子将在下一帧 tick 时处理，避免在 tick 中途修改粒子列表
    if (m_pendingParticles.size() < MAX_PARTICLES) {
        m_pendingParticles.push_back({type, pos, velocity, world, nullptr});
    }
}

void ParticleManager::addPendingParticle(ParticleTypeId type,
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world,
    std::unique_ptr<data::ParticleData> data)
{
    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!shouldShowParticle(type)) {
        return;
    }

    // 粒子将在下一帧 tick 时处理，避免在 tick 中途修改粒子列表
    if (m_pendingParticles.size() < MAX_PARTICLES) {
        m_pendingParticles.push_back({type, pos, velocity, world, std::move(data)});
    }
}

void ParticleManager::clear()
{
    m_particles.clear();
}

void ParticleManager::clearPending()
{
    m_pendingParticles.clear();
}

size_t ParticleManager::aliveParticleCount() const
{
    return static_cast<size_t>(std::count_if(
        m_particles.begin(), m_particles.end(), [](const std::unique_ptr<Particle>& p) { return p && p->isAlive(); }));
}

void ParticleManager::tick(mc::client::ClientWorld* world)
{
    // 当调用方未传入 world 时，回退到 setClientWorld() 设置的关联世界
    // 这使得 TridentEngine::render() 在无 world 参数调用 tick() 时，
    // 实体来源的振动粒子等仍能通过 ClientWorld 解析实体位置
    if (world == nullptr) {
        world = m_clientWorld;
    }

    // 首先处理待处理粒子队列
    processPendingParticles();

    // 创建发射回调，允许发射器粒子发射新粒子
    auto emitCallback = [this](ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity) {
        addPendingParticle(type, pos, velocity, nullptr);
    };

    // 更新所有粒子
    for (auto& particle : m_particles) {
        if (particle && particle->isAlive()) {
            // 设置发射回调（用于发射器粒子）
            particle->setEmitCallback(emitCallback);
            particle->tick(world);
        }
    }

    // 移除过期粒子
    m_particles.erase(std::remove_if(m_particles.begin(),
                          m_particles.end(),
                          [](const std::unique_ptr<Particle>& p) { return !p || !p->isAlive(); }),
        m_particles.end());

    // 处理发射器粒子发射的新粒子
    processPendingParticles();
}

bool ParticleManager::shouldShowParticle(ParticleTypeId type) const
{
    // All 模式：显示所有粒子
    if (m_particleMode == client::ParticleMode::All) {
        return true;
    }

    // 检查粒子类型是否为重要粒子（overrideLimiter=true）
    // 重要粒子在任何模式下都显示
    const ParticleTypeInfo* info = ParticleRegistry::instance().getTypeInfo(type);
    if (info != nullptr && info->overrideLimiter) {
        return true;
    }

    // Minimal 模式：跳过所有非重要粒子
    if (m_particleMode == client::ParticleMode::Minimal) {
        return false;
    }

    // Decreased 模式：约 2/3 的普通粒子通过（1/3 概率降级为 Minimal 行为）
    // 使用线程局部随机数，每帧每粒子独立判定
    // MC Java 使用 ClientLevel.random.nextInt(3) == 0 来降级
    // 使用时间+线程ID初始化种子，确保每次运行产生不同序列
    thread_local mc::math::Random s_decreaseRng(
        static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()) ^
        static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    return s_decreaseRng.nextInt(3) != 0;
}

void ParticleManager::processPendingParticles()
{
    // 清空 pending 队列，创建粒子并添加到主列表

    // 交换到临时队列避免迭代时修改
    std::vector<PendingParticle> pending;
    pending.swap(m_pendingParticles);

    for (const auto& pendingParticle : pending) {
        // 距离裁剪检查
        if (m_maxParticleDistance > 0.0f) {
            f32 distSq = glm::distance2(pendingParticle.position, m_cameraPosition);
            f32 maxDistSq = m_maxParticleDistance * m_maxParticleDistance;
            if (distSq > maxDistSq) {
                continue; // 跳过超出距离的粒子
            }
        }

        // 通过 ParticleRegistry 创建粒子
        // 如果有待处理数据，使用数据感知创建方法
        std::unique_ptr<Particle> particle;
        if (pendingParticle.data) {
            particle = ParticleRegistry::instance().createParticleWithData(pendingParticle.type,
                pendingParticle.position,
                pendingParticle.velocity,
                pendingParticle.world,
                pendingParticle.data.get());
        } else {
            particle = ParticleRegistry::instance().createParticle(
                pendingParticle.type, pendingParticle.position, pendingParticle.velocity, pendingParticle.world);
        }

        if (particle && m_particles.size() < MAX_PARTICLES) {
            m_particles.push_back(std::move(particle));
        }
    }
}

void ParticleManager::render(
    VkCommandBuffer cmd, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos, u32 frameIndex)
{
    if (m_particles.empty()) {
        return;
    }

    m_projection = projection;
    m_view = view;
    m_cameraPos = cameraPos;

    // 更新 Uniform 缓冲区
    _updateUniformBuffer(frameIndex);

    // 收集粒子顶点数据（带距离裁剪）
    m_vertexData.clear();
    const f32 maxDistSq = m_maxParticleDistance * m_maxParticleDistance;

    for (const auto& particle : m_particles) {
        if (!particle || !particle->isAlive()) {
            continue;
        }

        // 距离裁剪
        if (m_maxParticleDistance > 0.0f) {
            f32 distSq = glm::distance2(particle->position(), cameraPos);
            if (distSq > maxDistSq) {
                continue;
            }
        }

        particle->buildVertices(cameraPos, m_partialTick, m_textureAtlas, m_vertexData);
    }

    if (m_vertexData.empty()) {
        return;
    }

    // 更新顶点缓冲区
    _updateVertexBuffer();

    // 绑定管线
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // 绑定描述符集
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[frameIndex], 0, nullptr);

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 绘制
    vkCmdDraw(cmd, static_cast<u32>(m_vertexData.size()), 1, 0, 0);
}

void ParticleManager::render(VkCommandBuffer cmd,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos,
    u32 frameIndex,
    const mc::math::frustum::Frustum& frustum)
{
    if (m_particles.empty()) {
        return;
    }

    m_projection = projection;
    m_view = view;
    m_cameraPos = cameraPos;

    // 更新 Uniform 缓冲区
    _updateUniformBuffer(frameIndex);

    // 收集粒子顶点数据（带视锥剔除和距离裁剪）
    m_vertexData.clear();
    const f32 maxDistSq = m_maxParticleDistance * m_maxParticleDistance;

    for (const auto& particle : m_particles) {
        if (!particle || !particle->isAlive()) {
            continue;
        }

        const glm::vec3& particlePos = particle->position();

        // 距离裁剪
        if (m_maxParticleDistance > 0.0f) {
            f32 distSq = glm::distance2(particlePos, cameraPos);
            if (distSq > maxDistSq) {
                continue;
            }
        }

        // 使用球体测试进行视锥剔除
        mc::Vector3 frustumPos(particlePos.x, particlePos.y, particlePos.z);
        f32 particleRadius = static_cast<f32>(particle->size() * 0.5);

        if (!frustum.isSphereVisible(frustumPos, particleRadius)) {
            continue;
        }

        particle->buildVertices(cameraPos, m_partialTick, m_textureAtlas, m_vertexData);
    }

    if (m_vertexData.empty()) {
        return;
    }

    // 更新顶点缓冲区
    _updateVertexBuffer();

    // 绑定管线
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // 绑定描述符集
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[frameIndex], 0, nullptr);

    // 绑定顶点缓冲区
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 绘制
    vkCmdDraw(cmd, static_cast<u32>(m_vertexData.size()), 1, 0, 0);
}

Result<void> ParticleManager::_createVertexBuffer()
{
    // 创建动态顶点缓冲区
    m_vertexBufferSize = sizeof(ParticleVertex) * MAX_PARTICLES * VERTICES_PER_PARTICLE;

    auto result = _createBuffer(m_vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_vertexBuffer,
        m_vertexBufferMemory);

    if (!result.success()) {
        return result.error();
    }

    // 创建索引缓冲区（预定义 quad 索引模式）
    // 每个粒子是 quad，需要 6 个索引
    std::vector<u16> indices;
    indices.reserve(MAX_PARTICLES * INDICES_PER_PARTICLE);

    for (u16 i = 0; i < MAX_PARTICLES; ++i) {
        u16 baseVertex = i * VERTICES_PER_PARTICLE;
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 3);
    }

    VkDeviceSize indexBufferSize = sizeof(u16) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    result = _createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    if (!result.success()) {
        return result.error();
    }

    void* data = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to map particle index staging buffer");
    }
    std::memcpy(data, indices.data(), indexBufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);

    result = _createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_indexBuffer,
        m_indexBufferMemory);

    if (!result.success()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return result.error();
    }

    // 复制到设备本地内存
    VkCommandBuffer cmd = _beginSingleTimeCommands();
    VkBufferCopy copyRegion = {};
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, m_indexBuffer, 1, &copyRegion);
    _endSingleTimeCommands(cmd);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    return {};
}

Result<void> ParticleManager::_createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(ParticleUBO);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto result = _createBuffer(bufferSize,
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
            return Error(ErrorCode::InitializationFailed, "Failed to map particle uniform buffer memory");
        }
        m_uniformBuffersMapped[i] = mapped;
    }

    return {};
}

Result<void> ParticleManager::_createDescriptorSetLayout()
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

Result<void> ParticleManager::_createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create descriptor pool");
    }

    return {};
}

Result<void> ParticleManager::_createDescriptorSets()
{
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = {m_descriptorSetLayout, m_descriptorSetLayout};

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to allocate descriptor sets");
    }

    // 更新描述符集
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ParticleUBO);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(
            m_device, static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return {};
}

Result<void> ParticleManager::_createTexture()
{
    // 生成默认粒子纹理
    auto textureData = generateDefaultParticleTexture(PARTICLE_TEXTURE_SIZE, PARTICLE_TEXTURE_SIZE);

    // 创建 staging buffer
    VkDeviceSize imageSize = PARTICLE_TEXTURE_SIZE * PARTICLE_TEXTURE_SIZE * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    auto result = _createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    if (!result.success()) {
        return result.error();
    }

    void* data = nullptr;
    const VkResult mapResult = vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to map particle texture staging buffer");
    }
    std::memcpy(data, textureData.data(), imageSize);
    vkUnmapMemory(m_device, stagingBufferMemory);

    // 创建图像
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = PARTICLE_TEXTURE_SIZE;
    imageInfo.extent.height = PARTICLE_TEXTURE_SIZE;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_textureImage) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return Error(ErrorCode::InitializationFailed, "Failed to create particle texture image");
    }

    // 分配图像内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_textureImage, &memRequirements);

    auto memoryTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memoryTypeResult.success()) {
        vkDestroyImage(m_device, m_textureImage, nullptr);
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_textureImageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_textureImage, nullptr);
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate particle texture memory");
    }

    vkBindImageMemory(m_device, m_textureImage, m_textureImageMemory, 0);

    // 转换图像布局并复制
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {PARTICLE_TEXTURE_SIZE, PARTICLE_TEXTURE_SIZE, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    _endSingleTimeCommands(cmd);

    // 清理 staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_textureImageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create particle texture image view");
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
        return Error(ErrorCode::InitializationFailed, "Failed to create particle texture sampler");
    }

    return {};
}

Result<void> ParticleManager::_createPipelineLayout()
{
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create particle pipeline layout");
    }

    return {};
}

Result<void> ParticleManager::_createPipelines(VkSampleCountFlagBits sampleCount)
{
    // 加载 shader
    auto vertPath = resolveShaderPath("particle.vert.spv");
    auto fragPath = resolveShaderPath("particle.frag.spv");

    if (vertPath.empty() || fragPath.empty()) {
        return Error(ErrorCode::FileNotFound, "Failed to resolve particle shader binaries");
    }

    // 读取 shader 文件
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

    // Shader stage
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
    bindingDesc.stride = sizeof(ParticleVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> attributeDescs = {};

    // position
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = offsetof(ParticleVertex, position);

    // texCoord
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[1].offset = offsetof(ParticleVertex, texCoord);

    // color
    attributeDescs[2].binding = 0;
    attributeDescs[2].location = 2;
    attributeDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[2].offset = offsetof(ParticleVertex, color);

    // size
    attributeDescs[3].binding = 0;
    attributeDescs[3].location = 3;
    attributeDescs[3].format = VK_FORMAT_R32_SFLOAT;
    attributeDescs[3].offset = offsetof(ParticleVertex, size);

    // alpha
    attributeDescs[4].binding = 0;
    attributeDescs[4].location = 4;
    attributeDescs[4].format = VK_FORMAT_R32_SFLOAT;
    attributeDescs[4].offset = offsetof(ParticleVertex, alpha);

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

    // 视口和裁剪
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(m_extent.width);
    viewport.height = static_cast<f32>(m_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // 粒子需要双面渲染
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;

    // 深度/模板
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE; // 粒子写入深度
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
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
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 创建图形管线
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
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // 清理 shader 模块
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);

    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create particle graphics pipeline");
    }

    return {};
}

void ParticleManager::_updateUniformBuffer(u32 frameIndex)
{
    ParticleUBO ubo = {};
    ubo.projection = m_projection;
    ubo.view = m_view;
    ubo.cameraPos = m_cameraPos;
    ubo.partialTick = static_cast<f32>(m_partialTick);

    if (m_uniformBuffersMapped[frameIndex] != nullptr) {
        std::memcpy(m_uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
    }
}

void ParticleManager::_updateVertexBuffer()
{
    if (m_vertexData.empty()) {
        return;
    }

    void* data = nullptr;
    VkDeviceSize size = m_vertexData.size() * sizeof(ParticleVertex);
    const VkResult mapResult = vkMapMemory(m_device, m_vertexBufferMemory, 0, size, 0, &data);
    if (mapResult != VK_SUCCESS || data == nullptr) {
        return;
    }
    std::memcpy(data, m_vertexData.data(), size);
    vkUnmapMemory(m_device, m_vertexBufferMemory);
}

Result<u32> ParticleManager::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::OutOfMemory, "Failed to find suitable memory type");
}

Result<void> ParticleManager::_createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    auto memoryTypeResult = _findMemoryType(memRequirements.memoryTypeBits, properties);
    if (!memoryTypeResult.success()) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate buffer memory");
    }

    vkBindBufferMemory(m_device, buffer, memory, 0);

    return {};
}

VkCommandBuffer ParticleManager::_beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void ParticleManager::_endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

} // namespace mc::client::renderer::trident::particle
