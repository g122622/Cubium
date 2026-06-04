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
 */

#pragma once

#include "common/core/Types.hpp"
#include <memory>

namespace mc::world::gen::density {

/**
 * @brief 密度函数接口
 *
 * MC 1.18+ 引入的密度函数系统，用于地形生成和气候参数计算。
 * 每个密度函数接收方块坐标 (x, y, z)，返回一个密度值。
 *
 * 密度函数可以组合（加、乘、钳制等），形成表达式树。
 * NoiseRouter 持有 15 个密度函数引用，其中 6 个用于 Climate.Sampler。
 */
class DensityFunction {
public:
    virtual ~DensityFunction() = default;

    /**
     * @brief 在指定方块坐标处计算密度值
     *
     * @param blockX 方块 X 坐标
     * @param blockY 方块 Y 坐标
     * @param blockZ 方块 Z 坐标
     * @return 密度值
     */
    [[nodiscard]] virtual f64 compute(i32 blockX, i32 blockY, i32 blockZ) const = 0;

    /** 密度函数的最小可能值 */
    [[nodiscard]] virtual f64 minValue() const = 0;

    /** 密度函数的最大可能值 */
    [[nodiscard]] virtual f64 maxValue() const = 0;
};

} // namespace mc::world::gen::density
