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

#include "FluidRegistry.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 内置流体静态访问器
 *
 * 提供对内置流体实例的静态访问。
 * 类似于 VanillaBlocks 对方块提供访问。
 *
 * ## 使用示例
 * ```cpp
 * // 获取水源流体
 * Fluid* water = Fluids::WATER();
 *
 * // 获取岩浆源流体
 * Fluid* lava = Fluids::LAVA();
 *
 * // 检查流体状态是否为水
 * if (fluidState.getType() == *Fluids::WATER()) {
 *     // 处理水
 * }
 * ```
 */
namespace Fluids {

/**
 * @brief 初始化内置流体指针
 *
 * 在 FluidRegistry 初始化后调用。
 * 必须在使用任何流体指针之前调用。
 */
void initialize();

/**
 * @brief 空流体（无流体）
 */
[[nodiscard]] Fluid* EMPTY();

/**
 * @brief 水源头
 */
[[nodiscard]] Fluid* WATER();

/**
 * @brief 流动水
 */
[[nodiscard]] Fluid* FLOWING_WATER();

/**
 * @brief 岩浆源头
 */
[[nodiscard]] Fluid* LAVA();

/**
 * @brief 流动岩浆
 */
[[nodiscard]] Fluid* FLOWING_LAVA();

} // namespace Fluids

} // namespace fluid
} // namespace mc
