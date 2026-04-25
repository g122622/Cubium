#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::util {

/**
 * @brief 字形网格数据
 */
struct GlyphMesh {
    std::vector<f32> vertices;  // 顶点数据 (position xyz, texcoord uv, normal xyz, color rgba)
    std::vector<u32> indices;
    f32 advanceX;  // 绘制此字符后光标前进的距离
    f32 width;     // 字符宽度
    f32 height;    // 字符高度
};

/**
 * @brief 世界空间文本渲染器
 *
 * 在 3D 世界中渲染文本（如名称标签）。
 * 使用 billboard 技术使文本始终面向相机。
 *
 * 参考 MC 1.16.5 NameTagRenderer
 */
class WorldTextRenderer {
public:
    /**
     * @brief 初始化文本渲染器
     * @param pipeline 实体渲染管线
     * @return 成功或错误
     */
    static bool initialize(pipeline::EntityPipeline& pipeline);

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
    static void renderText(
        VkCommandBuffer cmd,
        const String& text,
        const Vector3f& position,
        f32 scale,
        const Vector4f& color,
        bool showBackground,
        pipeline::EntityPipeline& pipeline
    );

    /**
     * @brief 渲染名称标签（带 billboard 行为）
     *
     * @param cmd Vulkan 命令缓冲区
     * @param name 玩家名称
     * @param entityPosition 实体世界位置
     * @param entityHeight 实体高度
     * @param pipeline 实体渲染管线
     */
    static void renderNameTag(
        VkCommandBuffer cmd,
        const String& name,
        const Vector3f& entityPosition,
        f32 entityHeight,
        pipeline::EntityPipeline& pipeline
    );

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

private:
    /**
     * @brief 创建字形网格
     */
    static GlyphMesh createGlyphMesh(char c, f32 size);

    /**
     * @brief 创建背景网格
     */
    static void createBackgroundMesh(f32 width, f32 height);

    /**
     * @brief 计算 billboard 矩阵
     */
    static void computeBillboardMatrix(
        const Vector3f& position,
        std::array<f64, 16>& outMatrix
    );

    /**
     * @brief 计算文本宽度
     */
    [[nodiscard]] static f32 calculateTextWidth(const String& text, f32 scale);

    /**
     * @brief 检查是否应该渲染文本
     */
    [[nodiscard]] static bool shouldRenderText(
        const Vector3f& position,
        f32 distance
    );

    // 静态成员
    static bool s_initialized;
    static f32 s_maxDistance;
    static f32 s_scale;
    static bool s_showBackground;
    static u8 s_bgColorR;
    static u8 s_bgColorG;
    static u8 s_bgColorB;
    static u8 s_bgColorA;
    static Vector3d s_cameraPosition;
    static std::array<f64, 16> s_viewMatrix;

    // 字形缓存
    static std::unordered_map<char, GlyphMesh> s_glyphCache;

    // 背景网格
    static pipeline::EntityMesh* s_backgroundMesh;

    // 常量
    static constexpr f32 DEFAULT_MAX_DISTANCE = 64.0f;
    static constexpr f32 DEFAULT_SCALE = 0.025f;
    static constexpr f32 CHAR_WIDTH = 0.5f;   // 每个字符的默认宽度
    static constexpr f32 CHAR_HEIGHT = 1.0f;  // 每个字符的默认高度
    static constexpr f32 BACKGROUND_PADDING = 0.25f;
    static constexpr f32 HEIGHT_OFFSET = 0.3f;  // 名称标签在头顶上方的偏移
};

} // namespace mc::client::renderer::entity::util
