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

#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include <array>

namespace mc::client::ui::kagero::paint {

// 引入 Rect 类型（定义在 kagero 命名空间）
using mc::client::ui::kagero::Rect;

/**
 * @brief 2D仿射矩阵（3x3）
 *
 * 存储顺序为行主序：
 * | m[0] m[1] m[2] |   | sx  kx  tx |
 * | m[3] m[4] m[5] | = | ky  sy  ty |
 * | m[6] m[7] m[8] |   | 0   0   1  |
 *
 * 对于2D仿射变换，最后一行始终为 [0, 0, 1]。
 */
struct Matrix {
    std::array<f32, 9> m = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    /**
     * @brief 创建单位矩阵
     */
    [[nodiscard]] static Matrix identity() { return Matrix{}; }

    /**
     * @brief 创建平移矩阵
     * @param dx X方向平移
     * @param dy Y方向平移
     */
    [[nodiscard]] static Matrix makeTranslate(f32 dx, f32 dy);

    /**
     * @brief 创建缩放矩阵
     * @param sx X方向缩放
     * @param sy Y方向缩放
     */
    [[nodiscard]] static Matrix makeScale(f32 sx, f32 sy);

    /**
     * @brief 创建旋转矩阵
     * @param degrees 旋转角度（度数，顺时针为正）
     */
    [[nodiscard]] static Matrix makeRotate(f32 degrees);

    /**
     * @brief 创建倾斜矩阵（用于斜体效果）
     * @param angleX X方向倾斜角度（度数）
     * @param angleY Y方向倾斜角度（度数）
     */
    [[nodiscard]] static Matrix makeSkew(f32 angleX, f32 angleY);

    /**
     * @brief 矩阵乘法
     * @param other 右乘矩阵
     * @return 结果矩阵
     */
    [[nodiscard]] Matrix operator*(const Matrix& other) const;

    /**
     * @brief 就地平移
     * @param dx X方向平移
     * @param dy Y方向平移
     */
    void translate(f32 dx, f32 dy);

    /**
     * @brief 就地缩放
     * @param sx X方向缩放
     * @param sy Y方向缩放
     */
    void scale(f32 sx, f32 sy);

    /**
     * @brief 就地旋转
     * @param degrees 旋转角度（度数，顺时针为正）
     */
    void rotate(f32 degrees);

    /**
     * @brief 就地倾斜
     * @param angleX X方向倾斜角度（度数）
     * @param angleY Y方向倾斜角度（度数）
     */
    void skew(f32 angleX, f32 angleY);

    /**
     * @brief 变换点
     * @param x X坐标（输入输出）
     * @param y Y坐标（输入输出）
     */
    void transformPoint(f32& x, f32& y) const;

    /**
     * @brief 计算行列式
     */
    [[nodiscard]] f32 determinant() const;

    /**
     * @brief 计算逆矩阵
     * @return 逆矩阵，若不可逆则返回单位矩阵
     */
    [[nodiscard]] Matrix inverse() const;
};

/**
 * @brief 圆角矩形
 */
struct RRect {
    Rect rect;
    f32 radiusX = 0.0f;
    f32 radiusY = 0.0f;
};

} // namespace mc::client::ui::kagero::paint
