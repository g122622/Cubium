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

#include "client/resource/BlockModelLoader.hpp"
#include "common/core/Types.hpp"
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace mc::client::renderer::entity::item {

/**
 * @brief 计算旋转后各轴的 rescale 缩放因子
 *
 * 对于每个坐标轴，取该轴单位向量经过旋转矩阵变换后的最大绝对分量，
 * 其倒数即为缩放因子。这补偿了旋转导致的轴向投影收缩。
 *
 * 例如：Y轴旋转45度时，X单位向量 (1,0,0) 变换为 (cos45, 0, sin45)
 * 最大绝对分量为 cos45 ≈ 0.707，缩放因子为 1/0.707 ≈ 1.414
 *
 * 参考 MC BlockElementRotation.scaleFactorForAxis()
 *
 * @param rotMatrix 旋转矩阵的3x3部分
 * @return 各轴缩放因子 (sx, sy, sz)
 */
inline glm::vec3 computeRescaleFactors(const glm::mat3& rotMatrix)
{
    auto computeAxisScale = [&rotMatrix](const glm::vec3& axisUnit) -> f32 {
        glm::vec3 transformed = rotMatrix * axisUnit;
        f32 maxAbs = glm::max(glm::abs(transformed.x), glm::max(glm::abs(transformed.y), glm::abs(transformed.z)));
        return (maxAbs > 0.0001f) ? (1.0f / maxAbs) : 1.0f;
    };

    return glm::vec3(computeAxisScale(glm::vec3(1.0f, 0.0f, 0.0f)),
        computeAxisScale(glm::vec3(0.0f, 1.0f, 0.0f)),
        computeAxisScale(glm::vec3(0.0f, 0.0f, 1.0f)));
}

/**
 * @brief 构建元素旋转矩阵（含 rescale 缩放）
 *
 * 参考 MC FaceBakery.bakeQuad() 的元素旋转逻辑：
 * 1. 构建绕指定轴的旋转矩阵
 * 2. 若 rescale 为 true，计算各轴缩放因子补偿旋转投影收缩
 * 3. 组合为 T(origin) * R(可选S) * T(-origin) 的变换矩阵
 *
 * @param rotation 模型元素旋转参数（origin/axis/angle/rescale）
 * @param scale 顶点缩放因子（用于将像素坐标转为世界坐标，通常为 1/16）
 * @return 旋转+平移组合矩阵，若无旋转则返回单位矩阵
 */
inline glm::mat4 buildElementRotationMatrix(const ModelRotation& rotation, f64 scale)
{
    // TODO: MC 1.21.11 新增了 EulerXYZ 旋转格式（x/y/z 三轴欧拉角），
    // 当前 ModelRotation 仅支持 axis+angle 单轴旋转格式。当资源包使用 x/y/z 字段时，
    // 需要扩展 ModelRotation 结构体并在此处支持 ZYX 欧拉角旋转矩阵构建，
    // 参考 MC BlockElementRotation.EulerXYZRotation.computeTransform()。

    f32 angleRad = glm::radians(rotation.angle);
    glm::vec3 axisVec;

    if (rotation.axis == "x") {
        axisVec = glm::vec3(1.0f, 0.0f, 0.0f);
    } else if (rotation.axis == "z") {
        axisVec = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        // 默认为 Y 轴
        axisVec = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // 构建纯旋转矩阵
    glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), angleRad, axisVec);

    // 若 rescale 为 true，计算各轴缩放因子并应用
    // 参考 MC BlockElementRotation.computeRescale() / scaleFactorForAxis()
    if (rotation.rescale && rotation.angle != 0.0f) {
        glm::vec3 rescaleFactors = computeRescaleFactors(glm::mat3(rotMatrix));

        // 将缩放应用到旋转矩阵: rotMatrix = rotMatrix * scale(rescaleFactors)
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), rescaleFactors);
        rotMatrix = rotMatrix * scaleMatrix;
    }

    // 旋转中心：MC 原版在 JSON 解析时将 origin 除以 16 转为方块坐标系
    // 此处顶点坐标已经乘以 scale (=1/16)，所以 origin 也需要乘以 scale
    glm::vec3 origin(rotation.origin.x * static_cast<f32>(scale),
        rotation.origin.y * static_cast<f32>(scale),
        rotation.origin.z * static_cast<f32>(scale));

    // 组合变换：T(origin) * rotMatrix * T(-origin)
    glm::mat4 translateToOrigin = glm::translate(glm::mat4(1.0f), -origin);
    glm::mat4 translateBack = glm::translate(glm::mat4(1.0f), origin);

    return translateBack * rotMatrix * translateToOrigin;
}

/**
 * @brief 获取面UV旋转后的顶点UV坐标
 *
 * UV旋转通过顶点索引排列实现，参考 MC Quadrant.rotateVertexIndex()。
 * rotation 为 0/90/180/270 度，对应 shift 为 0/1/2/3。
 * 每个顶点 i 使用无旋转时顶点 (i + shift) % 4 的 UV 坐标。
 *
 * 无旋转时各顶点的 UV 映射：
 *   顶点0: (u0, v1)  -- 左下
 *   顶点1: (u1, v1)  -- 右下
 *   顶点2: (u1, v0)  -- 右上
 *   顶点3: (u0, v0)  -- 左上
 *
 * @param vertexIndex 顶点索引 (0-3)
 * @param uvRotation UV旋转角度 (0/90/180/270)
 * @param u0, v0, u1, v1 UV坐标范围
 * @return 该顶点的 (u, v) 坐标
 */
inline std::pair<f32, f32> getRotatedUV(int vertexIndex, i32 uvRotation, f32 u0, f32 v0, f32 u1, f32 v1)
{
    int shift = uvRotation / 90; // 0, 1, 2, 3
    int uvCorner = (vertexIndex + shift) % 4;

    switch (uvCorner) {
        case 0:
            return {u0, v1}; // 左下
        case 1:
            return {u1, v1}; // 右下
        case 2:
            return {u1, v0}; // 右上
        case 3:
            return {u0, v0}; // 左上
        default:
            return {u0, v1}; // 不应到达
    }
}

} // namespace mc::client::renderer::entity::item
