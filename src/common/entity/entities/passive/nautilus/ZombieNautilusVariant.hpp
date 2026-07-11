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

namespace mc {

/**
 * @brief 僵尸鹦鹉螺变体类型
 *
 * 对应 MC 1.21.11 ZombieNautilusVariant 注册表中的气候变体。
 * 简化实现：原版使用 PriorityProvider+SpawnContext 注册表变体，
 * 这里使用枚举并通过生物群系判断选择。
 *
 * 变体由 ZombieNautilusEntity 在 finalizeSpawn 时根据生物群系选择。
 */
enum class ZombieNautilusVariant : i32 {
    /// 温带变体（默认）- 出现在普通海洋、冷水海洋等
    Temperate = 0,
    /// 寒冷变体 - 出现在冻洋、冰冻深海等寒冷生物群系
    Cold = 1,
    /// 温暖变体 - 出现在暖水海洋
    Warm = 2
};

} // namespace mc
