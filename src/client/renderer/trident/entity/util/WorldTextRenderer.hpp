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

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::util {

/**
 * @brief 字形网格数据（世界空间版本）
 */
struct WorldGlyphMesh {
    std::vector<model::ModelVertex> vertices; // 顶点数据 (position xyz, texcoord uv, normal xyz)
    std::vector<u32> indices;
    f32 advanceX; // 绘制此字符后光标前进的距离
    f32 width;    // 字符宽度
    f32 height;   // 字符高度
};

/**
 * @brief 世界空间文本渲染器
 *
 * 在 3D 世界中渲染文本（如名称标签）。
 * 使用 billboard 技术使文本始终面向相机。
 *
 * 集成现有 FontTextureAtlas 和 Font 系统。
 */
class WorldTextRenderer {
public:
    /**
     * @brief 初始化文本渲染器
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param pipeline 实体渲染管线
     * @param font 字体对象（用于获取字形UV）
     * @return 成功或错误
     */
    static bool initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        pipeline::EntityPipeline& pipeline,
        Font* font);

    /**
     * @brief 清理资源
     */
    static void cleanup();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * @brief 设置相机位置（用于 billboard 计算）
     */
    static void setCameraPosition(const Vector3d& position);

    /**
     * @brief 设置视图矩阵（用于 billboard 计算）
     */
    static void setViewMatrix(const std::array<f64, 16>& viewMatrix);

    /**
     * @brief 设置视锥体（用于视锥剔除）
     *
     * @param frustum 视锥体对象
     */
    static void setFrustum(const mc::math::frustum::Frustum& frustum);

    /**
     * @brief 设置相机前向方向（用于背面剔除）
     *
     * @param forward 相机前向向量（归一化）
     */
    static void setCameraForward(const Vector3f& forward);

    /**
     * @brief 渲染世界空间文本
     *
     * @param cmd Vulkan 命令缓冲区
     * @param text 文本内容
     * @param position 世界位置
     * @param scale 缩放因子
     * @param color 文本颜色 (RGBA)
     * @param showBackground 是否显示背景
     * @param pipeline 实体渲染管线
     */
    static void renderText(VkCommandBuffer cmd,
        const std::string& text,
        const Vector3f& position,
        f32 scale,
        const Vector4f& color,
        bool showBackground,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 渲染名称标签（带 billboard 行为）
     *
     * @param cmd Vulkan 命令缓冲区
     * @param name 玩家名称
     * @param entityPosition 实体世界位置
     * @param entityHeight 实体高度
     * @param pipeline 实体渲染管线
     */
    static void renderNameTag(VkCommandBuffer cmd,
        const std::string& name,
        const Vector3f& entityPosition,
        f32 entityHeight,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 设置最大可见距离
     */
    static void setMaxDistance(f32 distance);

    /**
     * @brief 获取最大可见距离
     */
    [[nodiscard]] static f32 maxDistance();

    /**
     * @brief 设置背景颜色
     */
    static void setBackgroundColor(u8 r, u8 g, u8 b, u8 a);

    /**
     * @brief 设置是否显示背景
     */
    static void setShowBackground(bool show);

    /**
     * @brief 获取字形网格（用于渲染）
     * @param codepoint Unicode码点
     * @return 字形网格指针，如果不存在返回nullptr
     */
    [[nodiscard]] static const WorldGlyphMesh* getGlyphMesh(u32 codepoint);

    /**
     * @brief 获取字体纹理视图（用于绑定到管线）
     */
    [[nodiscard]] static VkImageView fontTextureView() { return s_fontTextureView; }

    /**
     * @brief 获取字体采样器
     */
    [[nodiscard]] static VkSampler fontSampler() { return s_fontSampler; }

private:
    /**
     * @brief 创建字形网格（从Font获取字形数据）
     * @param glyph 字形数据
     * @return 字形网格
     */
    static WorldGlyphMesh createGlyphMeshFromGlyph(const Glyph& glyph);

    /**
     * @brief 创建字体纹理（上传到GPU）
     * @return 成功或错误
     */
    static bool createFontTexture();

    /**
     * @brief 创建背景网格
     */
    static void createBackgroundMesh(f32 width, f32 height);

    /**
     * @brief 计算 billboard 矩阵
     */
    static void computeBillboardMatrix(const Vector3f& position, std::array<f64, 16>& outMatrix);

    /**
     * @brief 计算文本宽度
     */
    [[nodiscard]] static f32 calculateTextWidth(const std::string& text, f32 scale);

    /**
     * @brief 检查是否应该渲染文本
     *
     * @param position 文本世界位置
     * @param distance 到相机的距离
     * @return true 如果应该渲染
     */
    [[nodiscard]] static bool shouldRenderText(const Vector3f& position, f32 distance);

    /**
     * @brief 检查文本是否在相机背后（背面剔除）
     *
     * @param textPosition 文本世界位置
     * @return true 如果文本在相机背后
     */
    [[nodiscard]] static bool isBackFacing(const Vector3f& textPosition);

    /**
     * @brief 从UTF-8字符串解码码点
     */
    [[nodiscard]] static u32 decodeCodepoint(const std::string& text, size_t& pos);

    // 静态成员
    static bool s_initialized;
    static Font* s_font; // 字体引用
    static f32 s_maxDistance;
    static f32 s_scale;
    static bool s_showBackground;
    static u8 s_bgColorR;
    static u8 s_bgColorG;
    static u8 s_bgColorB;
    static u8 s_bgColorA;
    static Vector3d s_cameraPosition;
    static std::array<f64, 16> s_viewMatrix;
    static mc::math::frustum::Frustum s_frustum; // 视锥体（用于视锥剔除）
    static Vector3f s_cameraForward;             // 相机前向向量（用于背面剔除）

    // Vulkan 资源
    static VkDevice s_device;
    static VkPhysicalDevice s_physicalDevice;
    static VkCommandPool s_commandPool;
    static VkQueue s_graphicsQueue;
    static VkImage s_fontTexture;
    static VkDeviceMemory s_fontTextureMemory;
    static VkImageView s_fontTextureView;
    static VkSampler s_fontSampler;

    // 字形网格缓存（从Font的Glyph创建）
    static std::unordered_map<u32, WorldGlyphMesh> s_glyphMeshCache;

    // 背景网格
    static pipeline::EntityMesh* s_backgroundMesh;

    // 常量
    static constexpr f32 DEFAULT_MAX_DISTANCE = 64.0f;
    static constexpr f32 DEFAULT_SCALE = 0.025f;
    static constexpr f32 DEFAULT_CHAR_WIDTH = 0.5f; // 每个字符的默认宽度
    static constexpr f32 CHAR_HEIGHT = 1.0f;        // 每个字符的默认高度
    static constexpr f32 BACKGROUND_PADDING = 0.25f;
    static constexpr f32 HEIGHT_OFFSET = 0.3f; // 名称标签在头顶上方的偏移
};

} // namespace mc::client::renderer::entity::util
