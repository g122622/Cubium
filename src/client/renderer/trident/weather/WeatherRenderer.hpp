#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::weather {

// 天气渲染常量
namespace WeatherRenderConstants {
/// 最小渲染强度阈值（低于此值不渲染）
constexpr f64 MIN_RENDER_STRENGTH = 0.001;

/// 雨顶点最大数量（每个渲染位置 6 个顶点）
/// 参考 MC 1.16.5: 渲染半径 10 格，约 21x21 个位置
constexpr size_t MAX_RAIN_VERTICES = 21 * 21 * 6;

/// 天气纹理尺寸
constexpr u32 TEXTURE_SIZE = 64;

/// 温度阈值：低于此值为雪，高于此值为雨
/// 参考 MC 1.16.5 Biome.getTemperature()
constexpr f32 SNOW_TEMPERATURE_THRESHOLD = 0.15f;

/// 云层高度（雨雪渲染顶部）
constexpr f64 CLOUD_HEIGHT = 192.0;

/// 雨柱高度
constexpr f64 RAIN_PILLAR_HEIGHT = 20.0;
}  // namespace WeatherRenderConstants

/**
 * @brief 天气渲染器
 *
 * 负责渲染雨滴和雪花效果。
 * 参考 MC 1.16.5 WorldRenderer.renderRainSnow()
 *
 * 渲染方式：
 * - 直接渲染雨/雪纹理层（高效，不需要单独粒子）
 * - 根据生物群系温度决定降水类型（雨/雪）
 * - 在玩家附近渲染，远处渐隐
 *
 * 与 ParticleManager 的区别：
 * - WeatherRenderer：渲染大量的雨/雪纹理层（主要视觉效果）
 * - ParticleManager：渲染溅落粒子等效果粒子
 */
class WeatherRenderer {
public:
    WeatherRenderer();
    ~WeatherRenderer();

    // 禁止拷贝
    WeatherRenderer(const WeatherRenderer&) = delete;
    WeatherRenderer& operator=(const WeatherRenderer&) = delete;

    // ========================================================================
    // 初始化与销毁
    // ========================================================================

    /**
     * @brief 初始化天气渲染器
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param renderPass 渲染通道
     * @param extent 交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkExtent2D extent,
        VkSampleCountFlagBits sampleCount);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 窗口大小变化时重新创建资源
     * @param extent 新的交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> onResize(VkExtent2D extent);

    // ========================================================================
    // 更新与渲染
    // ========================================================================

    /**
     * @brief 更新天气状态
     *
     * @param rainStrength 降雨强度 (0.0 - 1.0)
     * @param thunderStrength 雷暴强度 (0.0 - 1.0)
     * @param ticks 游戏帧数（用于动画）
     * @param partialTick 部分 tick（用于插值）
     */
    void update(f64 rainStrength, f64 thunderStrength, i64 ticks, f64 partialTick);

    /**
     * @brief 渲染天气效果
     *
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置
     * @param frameIndex 当前帧索引
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                u32 frameIndex);

    /**
     * @brief 渲染天气效果（带世界信息）
     *
     * 参考 MC 1.16.5 WorldRenderer.renderRainSnow()
     * 根据生物群系温度决定降水类型，根据地形高度决定渲染范围。
     *
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置
     * @param frameIndex 当前帧索引
     * @param world 客户端世界（用于查询生物群系和高度）
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                u32 frameIndex,
                mc::client::ClientWorld& world);

    /**
     * @brief 渲染天气效果（带视锥剔除）
     *
     * 使用视锥剔除优化天气渲染，只渲染视锥内的雨滴/雪花。
     *
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置
     * @param frameIndex 当前帧索引
     * @param frustum 视锥体（用于剔除）
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                u32 frameIndex,
                const mc::math::frustum::Frustum& frustum);

    /**
     * @brief 渲染天气效果（带世界信息和视锥剔除）
     *
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置
     * @param frameIndex 当前帧索引
     * @param world 客户端世界
     * @param frustum 视锥体
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                u32 frameIndex,
                mc::client::ClientWorld& world,
                const mc::math::frustum::Frustum& frustum);

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 是否正在下雨
     */
    [[nodiscard]] bool isRaining() const { return m_rainStrength > 0.01; }

    /**
     * @brief 是否正在雷暴
     */
    [[nodiscard]] bool isThundering() const { return m_thunderStrength > 0.9; }

private:
    // ========================================================================
    // 内部渲染方法
    // ========================================================================

    /**
     * @brief 内部渲染方法（带世界指针）
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                u32 frameIndex,
                mc::client::ClientWorld* world);

    // ========================================================================
    // 顶点数据结构
    // ========================================================================

    /**
     * @brief 天气层顶点数据
     *
     * 每个顶点包含位置、纹理坐标、颜色和光照信息。
     * 参考 MC 1.16.5 DefaultVertexFormats.PARTICLE_POSITION_TEX_COLOR_LMAP
     */
    struct WeatherVertex {
        float x, y, z;          ///< 位置（相对于相机）
        float u, v;             ///< 纹理坐标
        float r, g, b, a;       ///< RGBA 颜色
        u16 lightU, lightV;     ///< 光照贴图坐标
    };

    // ========================================================================
    // 资源创建
    // ========================================================================

    [[nodiscard]] Result<void> createVertexBuffer();
    [[nodiscard]] Result<void> createUniformBuffers();
    [[nodiscard]] Result<void> createDescriptorSetLayout();
    [[nodiscard]] Result<void> createDescriptorPool();
    [[nodiscard]] Result<void> createDescriptorSets();
    [[nodiscard]] Result<void> createPipelineLayout();
    [[nodiscard]] Result<void> createPipelines(VkSampleCountFlagBits sampleCount);
    [[nodiscard]] Result<void> createTextures();

    void updateUniformBuffer(u32 frameIndex);

    /**
     * @brief 生成雨/雪层顶点数据
     *
     * 参考 MC 1.16.5 WorldRenderer.renderRainSnow()
     * 根据相机位置和生物群系生成雨/雪层。
     *
     * @param world 客户端世界（可为 nullptr，此时使用默认值）
     */
    void generateWeatherGeometry(mc::client::ClientWorld* world);

    /**
     * @brief 从数据创建纹理
     */
    [[nodiscard]] Result<void> createTextureFromData(const std::vector<u8>& data,
                                                     u32 width, u32 height,
                                                     VkImage& image,
                                                     VkDeviceMemory& memory,
                                                     VkImageView& imageView);

    /**
     * @brief 生成程序化雨纹理
     */
    [[nodiscard]] std::vector<u8> generateRainTexture(u32 width, u32 height);

    /**
     * @brief 生成程序化雪纹理
     */
    [[nodiscard]] std::vector<u8> generateSnowTexture(u32 width, u32 height);

private:
    // Vulkan 设备
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;

    // 描述符
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // 管线
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_rainPipeline = VK_NULL_HANDLE;  // 雨滴管线
    VkPipeline m_snowPipeline = VK_NULL_HANDLE;  // 雪花管线

    // 纹理
    VkImage m_rainTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_rainTextureMemory = VK_NULL_HANDLE;
    VkImageView m_rainTextureView = VK_NULL_HANDLE;

    VkImage m_snowTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_snowTextureMemory = VK_NULL_HANDLE;
    VkImageView m_snowTextureView = VK_NULL_HANDLE;

    VkSampler m_textureSampler = VK_NULL_HANDLE;

    // 顶点缓冲区（动态更新）
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_vertexBufferSize = 0;
    void* m_vertexBufferMapped = nullptr;

    // Uniform 缓冲区（每帧一个）
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    VkBuffer m_uniformBuffers[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    void* m_uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT] = {nullptr, nullptr};
    VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    // 天气状态
    f64 m_rainStrength = 0.0f;
    f64 m_thunderStrength = 0.0f;
    i64 m_ticks = 0;
    f64 m_partialTick = 0.0f;
    glm::vec3 m_cameraPos = glm::vec3(0.0f);

    // 当前渲染参数
    glm::mat4 m_currentProjection = glm::mat4(1.0f);
    glm::mat4 m_currentView = glm::mat4(1.0f);

    // 顶点数据
    std::vector<WeatherVertex> m_rainVertices;
    std::vector<WeatherVertex> m_snowVertices;
    u32 m_rainVertexCount = 0;
    u32 m_snowVertexCount = 0;

    // 渲染范围（参考 MC 的 l 变量）
    i32 m_renderRadius = 5;  // Fast 模式: 5, Fancy 模式: 10

    // 随机偏移数组（参考 MC 的 rainSizeX/rainSizeZ）
    static constexpr i32 RAIN_SIZE = 32;
    f64 m_rainOffsetX[RAIN_SIZE * RAIN_SIZE] = {};
    f64 m_rainOffsetZ[RAIN_SIZE * RAIN_SIZE] = {};

    // 视锥体（用于剔除）
    const mc::math::frustum::Frustum* m_frustum = nullptr;
};

} // namespace mc::client::renderer::trident::weather
