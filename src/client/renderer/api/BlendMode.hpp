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

#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 混合因子
 *
 * 定义源和目标颜色的混合因子，与 Vulkan 和 OpenGL 兼容。
 */
enum class BlendFactor : u8 {
    Zero,             // 0
    One,              // 1
    SrcColor,         // 源颜色
    OneMinusSrcColor, // 1 - 源颜色
    DstColor,         // 目标颜色
    OneMinusDstColor, // 1 - 目标颜色
    SrcAlpha,         // 源透明度
    OneMinusSrcAlpha, // 1 - 源透明度
    DstAlpha,         // 目标透明度
    OneMinusDstAlpha, // 1 - 目标透明度
    ConstantColor,    // 常量颜色
    OneMinusConstantColor,
    ConstantAlpha, // 常量透明度
    OneMinusConstantAlpha,
    SrcAlphaSaturate // 源透明度饱和
};

/**
 * @brief 混合操作
 *
 * 定义源和目标颜色如何组合。
 */
enum class BlendOp : u8 {
    Add,             // 源 + 目标
    Subtract,        // 源 - 目标
    ReverseSubtract, // 目标 - 源
    Min,             // min(源, 目标)
    Max              // max(源, 目标)
};

/**
 * @brief 混合状态
 *
 * 定义颜色混合的完整配置。
 */
struct BlendState {
    bool enabled = false;
    BlendFactor srcColor = BlendFactor::SrcAlpha;
    BlendFactor dstColor = BlendFactor::OneMinusSrcAlpha;
    BlendFactor srcAlpha = BlendFactor::One;
    BlendFactor dstAlpha = BlendFactor::Zero;
    BlendOp colorOp = BlendOp::Add;
    BlendOp alphaOp = BlendOp::Add;
    u8 colorWriteMask = 0xF; // RGBA 各通道写入掩码

    /**
     * @brief 创建禁用混合的状态
     */
    static BlendState disabled() { return BlendState{}; }

    /**
     * @brief 创建标准 Alpha 混合状态
     *
     * 用于半透明物体 (src=SRC_ALPHA, dst=ONE_MINUS_SRC_ALPHA)
     */
    static BlendState alpha()
    {
        BlendState state;
        state.enabled = true;
        state.srcColor = BlendFactor::SrcAlpha;
        state.dstColor = BlendFactor::OneMinusSrcAlpha;
        state.srcAlpha = BlendFactor::One;
        state.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        return state;
    }

    /**
     * @brief 创建加法混合状态
     *
     * 用于发光效果、粒子等 (src=SRC_ALPHA, dst=ONE)
     */
    static BlendState additive()
    {
        BlendState state;
        state.enabled = true;
        state.srcColor = BlendFactor::SrcAlpha;
        state.dstColor = BlendFactor::One;
        state.srcAlpha = BlendFactor::One;
        state.dstAlpha = BlendFactor::One;
        return state;
    }

    /**
     * @brief 创建预乘 Alpha 混合状态
     *
     * 用于预乘 Alpha 纹理 (src=ONE, dst=ONE_MINUS_SRC_ALPHA)
     */
    static BlendState premultiplied()
    {
        BlendState state;
        state.enabled = true;
        state.srcColor = BlendFactor::One;
        state.dstColor = BlendFactor::OneMinusSrcAlpha;
        state.srcAlpha = BlendFactor::One;
        state.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        return state;
    }

    /**
     * @brief 创建颜色叠加混合状态
     *
     * 用于颜色调制效果 (src=DST_COLOR, dst=SRC_COLOR)
     */
    static BlendState multiply()
    {
        BlendState state;
        state.enabled = true;
        state.srcColor = BlendFactor::DstColor;
        state.dstColor = BlendFactor::SrcColor;
        state.srcAlpha = BlendFactor::DstAlpha;
        state.dstAlpha = BlendFactor::SrcAlpha;
        return state;
    }

    bool operator==(const BlendState& other) const
    {
        return enabled == other.enabled && srcColor == other.srcColor && dstColor == other.dstColor &&
            srcAlpha == other.srcAlpha && dstAlpha == other.dstAlpha && colorOp == other.colorOp &&
            alphaOp == other.alphaOp && colorWriteMask == other.colorWriteMask;
    }

    bool operator!=(const BlendState& other) const { return !(*this == other); }
};

} // namespace mc::client::renderer::api
