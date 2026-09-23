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

// MathUtils.cpp - 数学工具函数（头文件中的函数都是内联的）
// 本文件只保留无法内联的实现（需要静态初始化表的查表函数）。

#include "MathUtils.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace mc::math {

namespace {

// MC Mth 的量化参数（见 net/minecraft/util/Mth.java）
constexpr i32 SIN_QUANTIZATION = 65536;
constexpr i32 SIN_MASK = 65535;
constexpr i32 COS_OFFSET = 16384;
constexpr f64 SIN_SCALE = 10430.378350470453;

/// 预计算正弦表。表项为 (f32)std::sin(i / SIN_SCALE)，与 MC 的 SIN 表逐项同源。
/// 表在首次使用时构造（C++ 静态局部变量保证线程安全），此后为只读。
const std::array<f32, SIN_QUANTIZATION>& sinTable()
{
    static const std::array<f32, SIN_QUANTIZATION> kTable = [] {
        std::array<f32, SIN_QUANTIZATION> table{};
        for (i32 i = 0; i < SIN_QUANTIZATION; ++i) {
            table[static_cast<size_t>(i)] = static_cast<f32>(std::sin(static_cast<f64>(i) / SIN_SCALE));
        }
        return table;
    }();
    return kTable;
}

/// 把已缩放的角度量化为表索引：截断为 long 再取低 16 位。
/// 截断（而非取模）是必要的：负角度截断后与 65535 按位与会得到与原版一致的正索引。
i32 tableIndex(f64 scaledValue) noexcept
{
    return static_cast<i32>(static_cast<i64>(scaledValue) & SIN_MASK);
}

} // namespace

f32 mthSin(f64 value) noexcept
{
    return sinTable()[static_cast<size_t>(tableIndex(value * SIN_SCALE))];
}

f32 mthCos(f64 value) noexcept
{
    // 注意偏移必须在**截断之前**加上：Java 写的是 (long)(v * SCALE + 16384.0) & 65535L，
    // 截断不满足分配律，先截断再加偏移会与整数边界上的原版结果不一致。
    return sinTable()[static_cast<size_t>(tableIndex(value * SIN_SCALE + COS_OFFSET))];
}

} // namespace mc::math
