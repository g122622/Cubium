#pragma once

#include "../BlendMode.hpp"
#include "../CompareOp.hpp"
#include "../CullMode.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 渲染状态
 *
 * 定义渲染管线的完整状态配置。
 * 参考 MC 1.16.5 RenderState 系统。
 */
struct RenderState {
    BlendState blend;
    DepthState depth;
    RasterizerState rasterizer;

    /**
     * @brief 创建不透明物体的渲染状态
     *
     * - 无混合
     * - 深度读写
     * - 背面剔除
     */
    static RenderState solid()
    {
        RenderState state;
        state.blend = BlendState::disabled();
        state.depth = DepthState::readWrite();
        state.rasterizer = RasterizerState::defaults();
        return state;
    }

    /**
     * @brief 创建镂空物体的渲染状态 (cutout)
     *
     * - 无混合
     * - 深度读写
     * - 双面渲染
     */
    static RenderState cutout()
    {
        RenderState state;
        state.blend = BlendState::disabled();
        state.depth = DepthState::readWrite();
        state.rasterizer = RasterizerState::doubleSided();
        return state;
    }

    /**
     * @brief 创建镂空+Mipmap的渲染状态
     *
     * 与 cutout 相同，但使用 mipmap
     */
    static RenderState cutoutMipped()
    {
        return cutout(); // 状态相同，区别在纹理采样
    }

    /**
     * @brief 创建半透明物体的渲染状态
     *
     * - Alpha 混合
     * - 深度测试但不写入
     * - 双面渲染
     */
    static RenderState translucent()
    {
        RenderState state;
        state.blend = BlendState::alpha();
        state.depth = DepthState::readOnly();
        state.rasterizer = RasterizerState::doubleSided();
        return state;
    }

    /**
     * @brief 创建线条渲染状态
     *
     * - Alpha 混合
     * - 深度读写
     * - 无剔除
     * - 线框模式
     */
    static RenderState lines()
    {
        RenderState state;
        state.blend = BlendState::alpha();
        state.depth = DepthState::readWrite();
        state.rasterizer = RasterizerState::wireframe();
        return state;
    }

    /**
     * @brief 创建加法混合渲染状态
     *
     * 用于发光效果
     */
    static RenderState additive()
    {
        RenderState state;
        state.blend = BlendState::additive();
        state.depth = DepthState::readOnly();
        state.rasterizer = RasterizerState::doubleSided();
        return state;
    }

    bool operator==(const RenderState& other) const
    {
        return blend == other.blend && depth == other.depth && rasterizer == other.rasterizer;
    }

    bool operator!=(const RenderState& other) const { return !(*this == other); }
};

} // namespace mc::client::renderer::api
