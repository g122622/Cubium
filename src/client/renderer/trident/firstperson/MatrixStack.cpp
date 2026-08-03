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

#include "MatrixStack.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer {

namespace {

/**
 * @brief 以后乘方式应用变换矩阵
 *
 * current = current * transform
 */
void applyPostMultiply(Matrix4f& current, const Matrix4f& transform)
{
    current = current * transform;
}

} // namespace

// ============================================================================
// MatrixStack 实现
// ============================================================================
//
// Minecraft 使用右乘语义：变换矩阵右乘到当前矩阵上
// 这意味着调用顺序与变换顺序一致：
//   stack.translate(x, y, z);  // 先平移
//   stack.rotateZ(90);         // 后旋转
// 结果：先平移后旋转
//
// 矩阵乘法：M' = M * T
// 对于变换点 v：v' = M * T * v
// ============================================================================

MatrixStack::MatrixStack()
{
    // 初始化栈底为单位矩阵
    m_stack.push(Matrix4f::identity());
}

void MatrixStack::push()
{
    // 压入当前矩阵的副本
    m_stack.push(m_stack.top());
}

void MatrixStack::pop()
{
    MC_ASSERT_RELEASE(m_stack.size() > 1);
    m_stack.pop();
}

void MatrixStack::translate(f32 x, f32 y, f32 z)
{
    Matrix4f translation = Matrix4f::identity();
    translation.data[3] = x;
    translation.data[7] = y;
    translation.data[11] = z;

    applyPostMultiply(m_stack.top(), translation);
}

void MatrixStack::rotateX(f32 degrees)
{
    f32 radians = math::toRadians(degrees);
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);

    Matrix4f rotation = Matrix4f::identity();
    rotation.data[5] = c;
    rotation.data[6] = -s;
    rotation.data[9] = s;
    rotation.data[10] = c;

    applyPostMultiply(m_stack.top(), rotation);
}

void MatrixStack::rotateY(f32 degrees)
{
    f32 radians = math::toRadians(degrees);
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);

    Matrix4f rotation = Matrix4f::identity();
    rotation.data[0] = c;
    rotation.data[2] = s;
    rotation.data[8] = -s;
    rotation.data[10] = c;

    applyPostMultiply(m_stack.top(), rotation);
}

void MatrixStack::rotateZ(f32 degrees)
{
    f32 radians = math::toRadians(degrees);
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);

    Matrix4f rotation = Matrix4f::identity();
    rotation.data[0] = c;
    rotation.data[1] = -s;
    rotation.data[4] = s;
    rotation.data[5] = c;

    applyPostMultiply(m_stack.top(), rotation);
}

void MatrixStack::rotate(f32 axisX, f32 axisY, f32 axisZ, f32 degrees)
{
    f32 radians = math::toRadians(degrees);
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);
    f32 t = 1.0f - c;

    // 归一化轴
    f32 len = std::sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
    if (len > 0.0f) {
        axisX /= len;
        axisY /= len;
        axisZ /= len;
    }

    // 创建轴旋转矩阵
    Matrix4f rot = Matrix4f::identity();

    rot.data[0] = t * axisX * axisX + c;
    rot.data[1] = t * axisX * axisY - s * axisZ;
    rot.data[2] = t * axisX * axisZ + s * axisY;

    rot.data[4] = t * axisX * axisY + s * axisZ;
    rot.data[5] = t * axisY * axisY + c;
    rot.data[6] = t * axisY * axisZ - s * axisX;

    rot.data[8] = t * axisX * axisZ - s * axisY;
    rot.data[9] = t * axisY * axisZ + s * axisX;
    rot.data[10] = t * axisZ * axisZ + c;

    applyPostMultiply(m_stack.top(), rot);
}

void MatrixStack::scale(f32 x, f32 y, f32 z)
{
    Matrix4f scaleMatrix = Matrix4f::identity();
    scaleMatrix.data[0] = x;
    scaleMatrix.data[5] = y;
    scaleMatrix.data[10] = z;

    applyPostMultiply(m_stack.top(), scaleMatrix);
}

void MatrixStack::clear()
{
    // 清空栈
    while (!m_stack.empty()) {
        m_stack.pop();
    }
    // 重新压入单位矩阵
    m_stack.push(Matrix4f::identity());
}

} // namespace mc::client::renderer
