#pragma once

#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 面剔除模式
 *
 * 定义哪些面应该被剔除。
 * 与 Vulkan VkCullModeFlags 和 OpenGL glCullFace 对应。
 */
enum class CullMode : u8 {
    None,   // 不剔除任何面
    Front,  // 剔除正面
    Back,   // 剔除背面
    FrontAndBack  // 剔除所有面 (通常不使用)
};

/**
 * @brief 正面朝向
 *
 * 定义正面顶点的缠绕顺序。
 * 与 Vulkan VkFrontFace 和 OpenGL glFrontFace 对应。
 */
enum class FrontFace : u8 {
    CounterClockwise,  // 逆时针为正面 (OpenGL 默认)
    Clockwise          // 顺时针为正面 (Vulkan/我们使用)
};

/**
 * @brief 多边形填充模式
 *
 * 定义多边形的渲染方式。
 * 与 Vulkan VkPolygonMode 和 OpenGL glPolygonMode 对应。
 */
enum class PolygonMode : u8 {
    Fill,   // 填充
    Line,   // 线框
    Point   // 点
};

/**
 * @brief 光栅化状态
 *
 * 定义面剔除和填充模式的配置。
 * 参考 MC 1.16.5 RenderState。
 */
struct RasterizerState {
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::Clockwise;
    PolygonMode polygonMode = PolygonMode::Fill;

    /**
     * @brief 创建默认光栅化状态
     *
     * 剔除背面，顺时针为正面
     */
    static RasterizerState defaults() {
        return RasterizerState{};
    }

    /**
     * @brief 创建双面渲染状态
     *
     * 不剔除任何面
     */
    static RasterizerState doubleSided() {
        RasterizerState state;
        state.cullMode = CullMode::None;
        return state;
    }

    /**
     * @brief 创建线框渲染状态
     */
    static RasterizerState wireframe() {
        RasterizerState state;
        state.polygonMode = PolygonMode::Line;
        state.cullMode = CullMode::None;
        return state;
    }

    bool operator==(const RasterizerState& other) const {
        return cullMode == other.cullMode &&
               frontFace == other.frontFace &&
               polygonMode == other.polygonMode;
    }

    bool operator!=(const RasterizerState& other) const {
        return !(*this == other);
    }
};

} // namespace mc::client::renderer::api
