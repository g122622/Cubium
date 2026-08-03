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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

/**
 * @brief 图集槽位信息
 *
 * 描述一个已注册的图集在渲染管线中的位置。
 */
struct AtlasSlot {
    u32 id;                ///< 槽位ID（0-MAX_ATLAS_SLOTS-1）
    VkImageView imageView; ///< Vulkan 图像视图
    VkSampler sampler;     ///< Vulkan 采样器
    std::string name;      ///< 图集名称（调试用）

    [[nodiscard]] bool isValid() const { return imageView != VK_NULL_HANDLE && sampler != VK_NULL_HANDLE; }
};

/**
 * @brief GUI图集注册表
 *
 * 管理多个GUI纹理图集的注册、查询和描述符更新。
 * 支持运行时动态注册图集，自动分配槽位。
 *
 * ## 设计理念
 *
 * 考虑到扩展性，本系统采用槽位注册模式：
 * - 预定义固定数量的描述符槽位（最大16个）
 * - 图集通过名称注册到槽位
 * - 渲染时通过槽位ID选择正确的纹理
 *
 * ## 槽位分配策略
 *
 * 槽位 0: 字体纹理（固定，由GuiRenderer管理）
 * 槽位 1: 物品纹理图集（固定，由ItemRenderer管理）
 * 槽位 2-15: GUI图集（可动态分配）
 *   - icons 图集（心形、饥饿等）
 *   - widgets 图集（按钮、快捷栏等）
 *   - container 图集（容器背景等）
 *   - 其他资源包图集
 *
 * ## 使用示例
 *
 * ```cpp
 * GuiAtlasRegistry registry(device);
 * registry.initialize(descriptorSetLayout, descriptorPool);
 *
 * // 注册图集
 * u32 iconsSlot = registry.registerAtlas("icons", iconsView, iconsSampler);
 * u32 widgetsSlot = registry.registerAtlas("widgets", widgetsView, widgetsSampler);
 *
 * // 在渲染时获取槽位ID
 * u32 slot = registry.getSlotId("icons"); // 返回分配的槽位ID
 * ```
 *
 * ## 线程安全性
 *
 * 注册操作不是线程安全的，应在初始化阶段完成。
 * 查询操作（getSlotId、getAtlas）是线程安全的。
 */
class GuiAtlasRegistry {
public:
    /// 最大图集槽位数量（不包括固定的字体和物品槽位）
    static constexpr u32 MAX_GUI_ATLAS_SLOTS = 14;

    /// 固定槽位ID
    static constexpr u32 FONT_SLOT = 0;
    static constexpr u32 ITEM_SLOT = 1;
    static constexpr u32 FIRST_GUI_SLOT = 2;

    /**
     * @brief 构造函数
     * @param device Vulkan 逻辑设备
     */
    explicit GuiAtlasRegistry(VkDevice device);

    /**
     * @brief 析构函数
     */
    ~GuiAtlasRegistry();

    // 禁止拷贝
    GuiAtlasRegistry(const GuiAtlasRegistry&) = delete;
    GuiAtlasRegistry& operator=(const GuiAtlasRegistry&) = delete;

    /**
     * @brief 初始化注册表
     * @param descriptorSetLayout 描述符集布局
     * @param descriptorPool 描述符池
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);

    /**
     * @brief 注册图集
     *
     * 将图集注册到一个槽位，并更新描述符集。
     * 如果同名图集已存在，则更新其纹理。
     *
     * @param name 图集名称（用于查询）
     * @param imageView Vulkan 图像视图
     * @param sampler Vulkan 采样器
     * @return 分配的槽位ID，失败返回错误
     */
    [[nodiscard]] Result<u32> registerAtlas(const std::string& name, VkImageView imageView, VkSampler sampler);

    /**
     * @brief 注销图集
     *
     * 从注册表中移除图集，释放槽位。
     * 注意：不会销毁Vulkan资源。
     *
     * @param name 图集名称
     * @return 成功或错误
     */
    Result<void> unregisterAtlas(const std::string& name);

    /**
     * @brief 获取图集槽位ID
     * @param name 图集名称
     * @return 槽位ID，不存在返回nullopt
     */
    [[nodiscard]] std::optional<u32> getSlotId(const std::string& name) const;

    /**
     * @brief 获取图集信息
     * @param slotId 槽位ID
     * @return 图集信息，不存在返回nullptr
     */
    [[nodiscard]] const AtlasSlot* getAtlas(u32 slotId) const;

    /**
     * @brief 获取描述符集
     */
    [[nodiscard]] VkDescriptorSet descriptorSet() const { return m_descriptorSet; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取已注册图集数量
     */
    [[nodiscard]] u32 atlasCount() const { return m_atlasCount; }

    /**
     * @brief 更新描述符集中的图集
     *
     * 在Vulkan资源重建后（如窗口resize）调用此方法更新描述符。
     *
     * @param slotId 槽位ID
     * @param imageView 新的图像视图
     * @param sampler 新的采样器
     * @return 成功或错误
     */
    Result<void> updateAtlas(u32 slotId, VkImageView imageView, VkSampler sampler);

    /**
     * @brief 更新字体纹理（槽位0）
     *
     * 这是固定槽位，用于字体渲染。
     *
     * @param imageView 字体纹理图像视图
     * @param sampler 字体纹理采样器
     */
    void updateFontTexture(VkImageView imageView, VkSampler sampler);

    /**
     * @brief 更新物品纹理图集（槽位1）
     *
     * 这是固定槽位，用于物品图标渲染。
     *
     * @param imageView 物品纹理图像视图
     * @param sampler 物品纹理采样器
     */
    void updateItemAtlas(VkImageView imageView, VkSampler sampler);

private:
    /**
     * @brief 创建描述符集布局
     */
    // TODO: 当前仅接受外部提供的布局，此方法暂未实现
    [[nodiscard]] Result<void> _createDescriptorSetLayout();

    /**
     * @brief 分配描述符集
     */
    [[nodiscard]] Result<void> _allocateDescriptorSet();

    /**
     * @brief 更新单个描述符
     */
    void _writeDescriptor(u32 binding, VkImageView imageView, VkSampler sampler);

    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    /// 图集槽位数组（索引 = binding point）
    std::array<AtlasSlot, FONT_SLOT + MAX_GUI_ATLAS_SLOTS + 1> m_slots{};

    /// 名称到槽位ID的映射
    std::unordered_map<std::string, u32> m_nameToSlot;

    /// 已使用的GUI槽位计数
    u32 m_nextGuiSlot = FIRST_GUI_SLOT;

    /// 已注册图集数量
    u32 m_atlasCount = 0;

    bool m_initialized = false;
    bool m_ownsLayout = false; ///< 是否拥有描述符集布局
    bool m_ownsPool = false;   ///< 是否拥有描述符池
};

} // namespace mc::client::renderer::trident::gui
