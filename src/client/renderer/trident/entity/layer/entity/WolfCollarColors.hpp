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
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 狼项圈颜色映射工具
 *
 * 将 DyeColor 枚举序数映射为 RGB 颜色值。
 * 对应 MC 1.21.11 DyeColor.getTextureDiffuseColor() 的 RGB 值。
 *
 * 抽离为独立头文件以便单元测试验证颜色映射逻辑，
 * 无需依赖 Vulkan 渲染管线（WolfCollarLayer.cpp 依赖 Vulkan）。
 */
namespace wolf_collar_colors {

/// 项圈颜色 RGB 值数组，索引对应 DyeColor 枚举序数（0-15）
/// 对应 MC 1.21.11 DyeColor.getTextureDiffuseColor() 的 RGB 值
inline const Vector3f COLLAR_COLORS[16] = {
    Vector3f(1.0f, 1.0f, 1.0f),  // 白色 (0)
    Vector3f(0.85f, 0.5f, 0.2f), // 橙色 (1)
    Vector3f(0.8f, 0.2f, 0.6f),  // 品红色 (2)
    Vector3f(0.2f, 0.6f, 0.9f),  // 淡蓝色 (3)
    Vector3f(0.9f, 0.9f, 0.2f),  // 黄色 (4)
    Vector3f(0.4f, 0.8f, 0.2f),  // 黄绿色 (5)
    Vector3f(1.0f, 0.5f, 0.7f),  // 粉红色 (6)
    Vector3f(0.3f, 0.3f, 0.3f),  // 灰色 (7)
    Vector3f(0.5f, 0.5f, 0.5f),  // 淡灰色 (8)
    Vector3f(0.2f, 0.4f, 0.6f),  // 青色 (9)
    Vector3f(0.5f, 0.2f, 0.8f),  // 紫色 (10)
    Vector3f(0.2f, 0.3f, 0.7f),  // 蓝色 (11)
    Vector3f(0.5f, 0.3f, 0.1f),  // 棕色 (12)
    Vector3f(0.2f, 0.5f, 0.2f),  // 绿色 (13)
    Vector3f(0.6f, 0.2f, 0.2f),  // 红色 (14)
    Vector3f(0.1f, 0.1f, 0.1f),  // 黑色 (15)
};

/**
 * @brief 获取指定 DyeColor 对应的项圈 RGB 颜色
 *
 * 索引超出 [0, 15] 范围时回退到默认红色（索引 14）。
 *
 * @param colorIndex DyeColor 枚举序数（0-15）
 * @return RGB 颜色向量
 */
[[nodiscard]] inline Vector3f getCollarColorByIndex(u8 colorIndex)
{
    if (colorIndex < 16) {
        return COLLAR_COLORS[colorIndex];
    }
    return COLLAR_COLORS[14]; // 默认红色
}

/**
 * @brief 获取指定 DyeColor 对应的项圈 RGB 颜色
 *
 * @param color DyeColor 枚举值
 * @return RGB 颜色向量
 */
[[nodiscard]] inline Vector3f getCollarColor(DyeColor color)
{
    return getCollarColorByIndex(static_cast<u8>(color));
}

} // namespace wolf_collar_colors

} // namespace mc::client::renderer::entity::layer::entity
