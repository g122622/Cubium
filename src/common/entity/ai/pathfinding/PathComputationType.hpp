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

/**
 * @file PathComputationType.hpp
 * @brief 寻路计算类型（对齐原版 `PathComputationType`）。
 */

#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 寻路时「穿过这一格」的方式
 *
 * 同一个方块在不同计算类型下的可穿过性可以不同：雪层在陆地上层数 >= 5 就过不去，
 * 但水中的水草对水生生物而言是通的。原版因此把「能不能穿过」拆成一个
 * `Block#isPathfindable(state, type)` 的二元查询，而不是直接看碰撞形状。
 */
enum class PathComputationType : u8 {
    /// 陆地行走（`WalkNodeEvaluator` 用这个）
    Land,
    /// 水中游动
    Water,
    /// 空中飞行
    Air,
};

} // namespace mc
