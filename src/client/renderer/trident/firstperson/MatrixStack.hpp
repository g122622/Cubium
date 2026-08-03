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
#include "common/util/math/Vector3.hpp"
#include <array>
#include <cstddef>
#include <stack>
#include <vector>

namespace mc::client::renderer {

/**
 * @brief 4x4 变换矩阵
 *
 * 使用行主序存储，与 OpenGL/Vulkan 约定一致。
 *
 * 矩阵布局：
 * ```
 * [ m0  m1  m2  m3  ]   [ 0  1  2  3 ]
 * [ m4  m5  m6  m7  ] = [ 4  5  6  7 ]
 * [ m8  m9  m10 m11 ]   [ 8  9  10 11 ]
 * [ m12 m13 m14 m15 ]   [ 12 13 14 15 ]
 * ```
 *
 * 变换公式：
 * - 平移：position = m3, m7, m11
 * - 缩放：scale = length(m0-2), length(m4-6), length(m8-10)
 */
struct Matrix4f {
    std::array<f32, 16> data;

    /**
     * @brief 创建单位矩阵
     */
    static Matrix4f identity()
    {
        return Matrix4f{
            {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
    }

    /**
     * @brief 访问元素（行主序）
     */
    f32& operator()(i32 row, i32 col) { return data[static_cast<size_t>(row) * 4 + static_cast<size_t>(col)]; }

    /**
     * @brief 访问元素（行主序，const 版本）
     */
    [[nodiscard]] f32 operator()(i32 row, i32 col) const
    {
        return data[static_cast<size_t>(row) * 4 + static_cast<size_t>(col)];
    }

    /**
     * @brief 矩阵乘法
     */
    [[nodiscard]] Matrix4f operator*(const Matrix4f& other) const
    {
        Matrix4f result;
        for (i32 row = 0; row < 4; ++row) {
            for (i32 col = 0; col < 4; ++col) {
                f32 sum = 0.0f;
                for (i32 k = 0; k < 4; ++k) {
                    sum += (*this)(row, k) * other(k, col);
                }
                result(row, col) = sum;
            }
        }
        return result;
    }

    /**
     * @brief 获取平移分量
     */
    [[nodiscard]] Vector3f translation() const { return Vector3f(data[3], data[7], data[11]); }

    /**
     * @brief 设置平移分量
     */
    void setTranslation(f32 x, f32 y, f32 z)
    {
        data[3] = x;
        data[7] = y;
        data[11] = z;
    }
};

/**
 * @brief 矩阵栈
 *
 * 管理变换层级，用于渲染时的矩阵变换累积。
 *
 * 用法：
 * ```cpp
 * MatrixStack stack;
 * stack.push();
 *     stack.translate(0.0f, 1.0f, 0.0f);
 *     stack.rotateX(45.0f);
 *     stack.scale(0.5f, 0.5f, 0.5f);
 *     // 使用 stack.last() 渲染
 * stack.pop();
 * ```
 */
class MatrixStack {
public:
    MatrixStack();

    /**
     * @brief 将当前矩阵压入栈
     *
     * 创建当前矩阵的副本压入栈顶。
     * 后续变换将应用于副本。
     */
    void push();

    /**
     * @brief 从栈弹出矩阵
     *
     * 恢复到上一个矩阵状态。
     * 前置条件：栈深度必须大于1（不能弹出栈底单位矩阵）。
     */
    void pop();

    /**
     * @brief 平移变换
     */
    void translate(f32 x, f32 y, f32 z);

    /**
     * @brief 平移变换（Vector3 版本）
     */
    void translate(const Vector3f& v) { translate(v.x, v.y, v.z); }

    /**
     * @brief 绕 X 轴旋转
     * @param degrees 旋转角度（度）
     */
    void rotateX(f32 degrees);

    /**
     * @brief 绕 Y 轴旋转
     * @param degrees 旋转角度（度）
     */
    void rotateY(f32 degrees);

    /**
     * @brief 绕 Z 轴旋转
     * @param degrees 旋转角度（度）
     */
    void rotateZ(f32 degrees);

    /**
     * @brief 绕任意轴旋转
     * @param axis 旋转轴（需要归一化）
     * @param degrees 旋转角度（度）
     */
    void rotate(f32 axisX, f32 axisY, f32 axisZ, f32 degrees);

    /**
     * @brief 缩放变换
     */
    void scale(f32 x, f32 y, f32 z);

    /**
     * @brief 均匀缩放
     */
    void scale(f32 s) { scale(s, s, s); }

    /**
     * @brief 获取当前矩阵
     */
    [[nodiscard]] const Matrix4f& last() const { return m_stack.top(); }

    /**
     * @brief 获取当前矩阵（可修改）
     */
    Matrix4f& last() { return m_stack.top(); }

    /**
     * @brief 获取栈深度
     */
    [[nodiscard]] size_t depth() const { return m_stack.size(); }

    /**
     * @brief 清空栈，重置为单位矩阵
     */
    void clear();

private:
    std::stack<Matrix4f> m_stack;
};

} // namespace mc::client::renderer
