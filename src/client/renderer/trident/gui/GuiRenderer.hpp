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

#include "GuiAtlasRegistry.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/FontRenderer.hpp"
#include "client/ui/Glyph.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

// 前向声明
namespace mc::client {
class Font;
}

namespace mc::client::renderer::trident::gui {

/**
 * @brief GUI渲染器
 *
 * 负责在屏幕上渲染2D GUI元素，包括：
 * - 文本
 * - 矩形
 * - 纹理
 *
 * 使用正交投影渲染到屏幕空间。
 *
 * ## 多图集支持
 *
 * 渲染器支持多个纹理图集，通过GuiAtlasRegistry管理：
 * - 槽位 0: 字体纹理（固定）
 * - 槽位 1: 物品纹理图集（固定）
 * - 槽位 2+: GUI纹理图集（icons、widgets等）
 *
 * 每个顶点携带atlasSlot字段，指定使用哪个图集纹理。
 */
class GuiRenderer {
public:
    /// 物品纹理颜色（用于向后兼容）
    static constexpr u32 ITEM_TEXTURE_COLOR = 0xFEFFFFFF;

    /// 默认图集槽位常量
    static constexpr u32 FONT_ATLAS_SLOT = 0;
    static constexpr u32 ITEM_ATLAS_SLOT = 1;
    static constexpr u32 DEFAULT_GUI_ATLAS_SLOT = 2;

    GuiRenderer();
    ~GuiRenderer();

    // 禁止拷贝
    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer& operator=(const GuiRenderer&) = delete;

    /**
     * @brief 初始化GUI渲染器
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param renderPass 渲染通道
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkRenderPass renderPass,
        VkSampleCountFlagBits sampleCount);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 设置字体
     * @param font 字体对象
     */
    void setFont(mc::client::Font* font);

    /**
     * @brief 开始帧
     * @param screenW 屏幕宽度
     * @param screenH 屏幕高度
     */
    void beginFrame(f64 screenW, f64 screenH);

    /**
     * @brief 准备帧数据（在渲染通道外调用）
     * 用于更新字体纹理等需要在渲染通道外执行的操作
     * @param commandBuffer 命令缓冲区
     */
    void prepareFrame(VkCommandBuffer commandBuffer);

    /**
     * @brief 渲染GUI（在渲染通道内调用）
     * 绘制所有累积的元素，并上传顶点/索引数据
     * @param commandBuffer 命令缓冲区
     */
    void render(VkCommandBuffer commandBuffer);

    // ==================== 文本绘制 ====================
    /**
     * @brief 绘制文本
     * @param text 文本内容（UTF-8）
     * @param x X坐标
     * @param y Y坐标
     * @param color 颜色（ARGB）
     * @param shadow 是否绘制阴影
     * @return 文本宽度
     */
    f64 drawText(const std::string& text, f64 x, f64 y, u32 color = Colors::WHITE, bool shadow = true);

    /**
     * @brief 绘制居中文本
     * @param text 文本内容
     * @param x 中心X坐标
     * @param y Y坐标
     * @param color 颜色
     * @return 文本宽度
     */
    f64 drawTextCentered(const std::string& text, f64 x, f64 y, u32 color = Colors::WHITE);

    /**
     * @brief 获取文本宽度
     */
    [[nodiscard]] f64 getTextWidth(const std::string& text);

    /**
     * @brief 获取字体高度
     */
    [[nodiscard]] u32 getFontHeight() const;

    /**
     * @brief 获取字体对象
     */
    [[nodiscard]] mc::client::Font* font() const { return m_font; }

    /**
     * @brief 设置字体缩放因子
     * @param scale 缩放因子（默认1.0）
     */
    void setFontScale(f64 scale);

    /**
     * @brief 获取字体缩放因子
     */
    [[nodiscard]] f64 getFontScale() const;

    // ==================== 矩形绘制 ====================

    /**
     * @brief 绘制填充矩形
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param color 颜色（ARGB）
     */
    void fillRect(f64 x, f64 y, f64 width, f64 height, u32 color);

    /**
     * @brief 绘制纹理矩形（使用物品纹理图集）
     *
     * 绘制物品/图标纹理。UV坐标指定纹理图集中的区域。
     *
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param u0 纹理左上角U
     * @param v0 纹理左上角V
     * @param u1 纹理右下角U
     * @param v1 纹理右下角V
     * @param color 颜色（ARGB，默认白色）
     */
    void drawTexturedRect(
        f64 x, f64 y, f64 width, f64 height, f64 u0, f64 v0, f64 u1, f64 v1, u32 color = ITEM_TEXTURE_COLOR);

    /**
     * @brief 绘制纹理矩形（指定图集槽位）
     *
     * 使用指定的图集槽位绘制纹理。
     *
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param u0 纹理左上角U
     * @param v0 纹理左上角V
     * @param u1 纹理右下角U
     * @param v1 纹理右下角V
     * @param color 颜色（ARGB）
     * @param atlasSlot 图集槽位ID
     */
    void drawTexturedRect(f64 x, f64 y, f64 width, f64 height, f64 u0, f64 v0, f64 u1, f64 v1, u32 color, u8 atlasSlot);

    /**
     * @brief 绘制渐变矩形（垂直）
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param colorTop 顶部颜色
     * @param colorBottom 底部颜色
     */
    void fillGradientRect(f64 x, f64 y, f64 width, f64 height, u32 colorTop, u32 colorBottom);

    /**
     * @brief 绘制渐变矩形（水平）
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param colorLeft 左侧颜色
     * @param colorRight 右侧颜色
     */
    void fillGradientRectHorizontal(f64 x, f64 y, f64 width, f64 height, u32 colorLeft, u32 colorRight);

    /**
     * @brief 绘制矩形边框
     * @param x 左上角X
     * @param y 左上角Y
     * @param width 宽度
     * @param height 高度
     * @param color 颜色
     */
    void drawRect(f64 x, f64 y, f64 width, f64 height, u32 color);

    // ==================== 获取器 ====================

    /**
     * @brief 获取屏幕宽度
     */
    [[nodiscard]] f64 screenWidth() const { return m_screenWidth; }

    /**
     * @brief 获取屏幕高度
     */
    [[nodiscard]] f64 screenHeight() const { return m_screenHeight; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 设置物品纹理图集
     *
     * 物品纹理图集用于渲染物品图标。binding=1的纹理采样器。
     *
     * @param itemView 物品纹理图集的图像视图
     * @param itemSampler 物品纹理采样器
     */
    void setItemTextureAtlas(VkImageView itemView, VkSampler itemSampler);

    /**
     * @brief 设置GUI纹理图集
     *
     * GUI纹理图集用于渲染HUD元素、按钮等。binding=2的纹理采样器。
     *
     * @param guiView GUI纹理图集的图像视图
     * @param guiSampler GUI纹理采样器
     * @deprecated 使用 registerAtlas 替代
     */
    void setGuiTextureAtlas(VkImageView guiView, VkSampler guiSampler);

    /**
     * @brief 注册GUI纹理图集
     *
     * 将图集注册到指定槽位，用于渲染HUD元素等。
     *
     * @param name 图集名称（用于调试和查询）
     * @param view Vulkan 图像视图
     * @param sampler Vulkan 采样器
     * @return 分配的槽位ID（>=2），失败返回错误
     */
    [[nodiscard]] Result<u32> registerAtlas(const std::string& name, VkImageView view, VkSampler sampler);

    /**
     * @brief 获取图集槽位ID
     * @param name 图集名称
     * @return 槽位ID，不存在返回 nullopt
     */
    [[nodiscard]] std::optional<u32> getAtlasSlot(const std::string& name) const;

    /**
     * @brief 检查是否有GUI纹理图集
     */
    [[nodiscard]] bool hasGuiTextureAtlas() const { return m_guiTextureView != VK_NULL_HANDLE; }

private:
    /**
     * @brief 创建管线布局
     */
    [[nodiscard]] Result<void> _createPipelineLayout();

    /**
     * @brief 创建图形管线
     */
    [[nodiscard]] Result<void> _createPipeline(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount);

    /**
     * @brief 创建描述符
     */
    [[nodiscard]] Result<void> _createDescriptors();

    /**
     * @brief 创建顶点缓冲和索引缓冲
     */
    [[nodiscard]] Result<void> _createBuffers();

    /**
     * @brief 创建字体纹理
     */
    [[nodiscard]] Result<void> _createFontTexture();

    /**
     * @brief 更新字体纹理
     */
    void _updateFontTexture(VkCommandBuffer commandBuffer);

    /**
     * @brief 初始化纹理布局（在首次使用前调用）
     *
     * 由于纹理创建时布局为 UNDEFINED，需要在首次渲染前转换到 SHADER_READ_ONLY_OPTIMAL
     */
    void _initializeTextureLayouts(VkCommandBuffer commandBuffer);

    /**
     * @brief 上传顶点和索引数据到GPU
     */
    void _uploadBufferData(VkCommandBuffer commandBuffer);

    /**
     * @brief 更新图集描述符
     * @param binding 描述符绑定位点
     * @param view Vulkan 图像视图
     * @param sampler Vulkan 采样器
     */
    void _updateAtlasDescriptor(u32 binding, VkImageView view, VkSampler sampler);

    // ========== Vulkan 辅助函数 ==========

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] Result<void> _createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    // Vulkan资源
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    // 缓冲区
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_vertexBufferSize = 64 * 1024; // 64KB 初始大小
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_indexBufferSize = 128 * 1024; // 128KB 初始大小
    VkBuffer m_fontStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_fontStagingMemory = VK_NULL_HANDLE;

    // 字体纹理
    VkImage m_fontTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_fontTextureMemory = VK_NULL_HANDLE;
    VkImageView m_fontTextureView = VK_NULL_HANDLE;

    // 物品纹理占位符
    VkImage m_itemPlaceholderTexture = VK_NULL_HANDLE;
    VkDeviceMemory m_itemPlaceholderMemory = VK_NULL_HANDLE;
    VkImageView m_itemPlaceholderView = VK_NULL_HANDLE;

    // GUI纹理图集（向后兼容）
    VkImageView m_guiTextureView = VK_NULL_HANDLE;
    VkSampler m_guiSampler = VK_NULL_HANDLE;

    // 多图集支持
    std::unordered_map<std::string, u32> m_atlasSlots;
    u32 m_nextGuiSlot = DEFAULT_GUI_ATLAS_SLOT;

    VkSampler m_sampler = VK_NULL_HANDLE;

    // 字体
    mc::client::Font* m_font = nullptr;
    mc::client::FontRenderer m_fontRenderer;

    // 屏幕尺寸
    f64 m_screenWidth = 0.0;
    f64 m_screenHeight = 0.0;

    // 顶点/索引数据
    std::vector<mc::client::GuiVertex> m_vertices;
    std::vector<u32> m_indices;
    bool m_needsTextureUpdate = false;

    // 状态
    bool m_initialized = false;
    bool m_inFrame = false;
    bool m_textureLayoutsInitialized = false;
    bool m_fontTextureInShaderReadLayout = false;
};

} // namespace mc::client::renderer::trident::gui

// 向后兼容别名
namespace mc::client {
using GuiRenderer = renderer::trident::gui::GuiRenderer;
}
