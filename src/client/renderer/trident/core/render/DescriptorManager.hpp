#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;

/**
 * @brief 描述符管理器
 *
 * 管理描述符集布局、描述符池和描述符集分配。
 * 从 VulkanRenderer 拆分，职责单一化。
 */
class DescriptorManager {
public:
    DescriptorManager();
    ~DescriptorManager();

    // 禁止拷贝
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;

    // 允许移动
    DescriptorManager(DescriptorManager&& other) noexcept;
    DescriptorManager& operator=(DescriptorManager&& other) noexcept;

    /**
     * @brief 初始化描述符管理器
     * @param context Trident 上下文
     * @param maxFramesInFlight 最大同时在飞帧数
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(TridentContext* context, u32 maxFramesInFlight = 2);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 分配相机描述符集
     * @param frameIndex 帧索引
     * @return 描述符集或错误
     */
    [[nodiscard]] Result<VkDescriptorSet> allocateCameraSet(u32 frameIndex);

    /**
     * @brief 分配纹理描述符集
     * @return 描述符集或错误
     */
    [[nodiscard]] Result<VkDescriptorSet> allocateTextureSet();

    /**
     * @brief 分配雾效果描述符集
     * @return 描述符集或错误
     */
    [[nodiscard]] Result<VkDescriptorSet> allocateFogSet();

    // 访问器
    [[nodiscard]] VkDescriptorSetLayout cameraLayout() const { return m_cameraLayout; }
    [[nodiscard]] VkDescriptorSetLayout textureLayout() const { return m_textureLayout; }
    [[nodiscard]] VkDescriptorSetLayout fogLayout() const { return m_fogLayout; }
    [[nodiscard]] VkDescriptorPool pool() const { return m_pool; }
    [[nodiscard]] VkPipelineLayout pipelineLayout() const { return m_pipelineLayout; }
    [[nodiscard]] bool isValid() const { return m_pool != VK_NULL_HANDLE; }

private:
    // 创建方法
    [[nodiscard]] Result<void> createDescriptorSetLayouts();
    [[nodiscard]] Result<void> createPipelineLayout();
    [[nodiscard]] Result<void> createDescriptorPool();

    // 销毁方法
    void destroyDescriptorSetLayouts();
    void destroyPipelineLayout();
    void destroyDescriptorPool();

    // 外部依赖
    TridentContext* m_context = nullptr;

    // 描述符集布局
    VkDescriptorSetLayout m_cameraLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_textureLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_fogLayout = VK_NULL_HANDLE;

    // 管线布局
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    // 描述符池
    VkDescriptorPool m_pool = VK_NULL_HANDLE;

    // 配置
    u32 m_maxFramesInFlight = 2;
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
