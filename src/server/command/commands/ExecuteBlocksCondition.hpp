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

#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

#include <functional>

namespace mc::command {

/// 按世界坐标查方块状态的回调；返回 nullptr 表示该位置尚未加载/无数据。
using BlockStateLookup = std::function<const BlockState*(const BlockPos&)>;

/**
 * @brief 判断两个长方体区域内对应方块是否一致（`/execute if blocks`）
 *
 * 两个区域以各自的**最小角**（start）给定，尺寸由 `start`~`end` 的闭区间决定；
 * 目标区域从 `destination` 开始按同样尺寸逐格比较。
 *
 * 两种模式（对齐原版）：
 * - `masked = false`（`all`）：**逐格严格比较**，源区域里的空气也必须在目标区域对应位置是空气；
 * - `masked = true`：**只比较源区域的非空气方块**，源为空气的位置直接跳过。
 *
 * 任意一侧查询到 nullptr（区块未加载）即视为不匹配——宁可条件不成立，
 * 也不要把"读不到"当成"相同"。
 *
 * @param start 源区域最小角
 * @param end 源区域最大角（与 start 一起构成闭区间，顺序无关）
 * @param destination 目标区域最小角
 * @param masked true 走 masked 语义，false 走 all 语义
 * @param lookup 方块状态查询回调
 * @return 两个区域是否一致
 */
[[nodiscard]] bool blocksMatch(const BlockPos& start,
    const BlockPos& end,
    const BlockPos& destination,
    bool masked,
    const BlockStateLookup& lookup);

} // namespace mc::command
