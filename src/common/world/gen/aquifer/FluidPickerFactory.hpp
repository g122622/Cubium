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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "FluidStatus.hpp"

namespace mc::world::gen::aquifer {

/**
 * @brief 创建主世界流体选择器
 *
 * Y < min(-54, seaLevel) 返回熔岩，Y < seaLevel 返回水，否则返回空气。
 *
 * @param seaLevel 海平面高度
 * @param defaultFluid 默认流体（水）
 * @return 流体选择器
 */
[[nodiscard]] FluidPicker createOverworldFluidPicker(i32 seaLevel, const BlockState* defaultFluid);

/**
 * @brief 创建下界流体选择器
 *
 * 全部返回熔岩。
 *
 * @return 流体选择器
 */
[[nodiscard]] FluidPicker createNetherFluidPicker();

/**
 * @brief 创建末地流体选择器
 *
 * 全部返回空气。
 *
 * @return 流体选择器
 */
[[nodiscard]] FluidPicker createEndFluidPicker();

} // namespace mc::world::gen::aquifer
