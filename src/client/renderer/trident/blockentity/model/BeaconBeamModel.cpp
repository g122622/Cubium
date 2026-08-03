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

#include "BeaconBeamModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace mc::client::renderer::blockentity::model {

BeaconBeamModel::BeaconBeamModel()
    : m_segments()
{}

f32 BeaconBeamModel::calculateBeamRotation(i64 gameTime, f32 partialTick)
{
    // floorMod(totalWorldTime, 40) + partialTick
    // rotation = f * 2.25 - 45.0
    const f32 f = static_cast<f32>(math::floorMod(gameTime, ROTATION_PERIOD)) + partialTick;
    return f * ROTATION_SPEED + ROTATION_OFFSET;
}

void BeaconBeamModel::generateMesh(
    std::vector<entity::model::ModelVertex>& vertices, std::vector<u32>& indices, i64 gameTime, f32 partialTick) const
{
    if (m_segments.empty()) {
        return;
    }

    // 计算纹理 V 坐标偏移
    // vOffset = frac(f * 0.2 - floor(f * 0.1))
    const f32 f = static_cast<f32>(math::floorMod(gameTime, ROTATION_PERIOD)) + partialTick;
    const f32 vOffset = std::fmod(f * 0.2f - static_cast<f32>(std::floor(f * 0.1f)), 1.0f);

    // 渲染所有光束段
    i32 yOffset = 0;
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const BeamSegment& segment = m_segments[i];

        // 最后一段高度固定为 MAX_BEAM_HEIGHT
        const i32 height = (i == m_segments.size() - 1) ? MAX_BEAM_HEIGHT : segment.height;

        // 渲染内层光束（不透明）
        _renderSegment(vertices, indices, yOffset, height, segment.colors, vOffset, false);

        // 渲染外层光晕（半透明）
        _renderSegment(vertices, indices, yOffset, height, segment.colors, vOffset, true);

        yOffset += segment.height;
    }
}

void BeaconBeamModel::_renderSegment(std::vector<entity::model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    i32 yOffset,
    i32 height,
    const std::array<f32, 3>& colors,
    f32 vOffset,
    bool isGlow) const
{
    // 内层光束: radius = 0.2, alpha = 1.0
    // 外层光晕: radius = 0.25, alpha = 0.125
    const f32 radius = isGlow ? GLOW_RADIUS : BEAM_RADIUS;
    const f32 alpha = isGlow ? 0.125f : 1.0f;

    // 纹理参数
    const f32 textureScale = 1.0f;

    // V 纹理坐标
    // v1 = -1.0 + vOffset
    // v2 = height * textureScale * (0.5 / radius) + v1
    const f32 v1 = -1.0f + vOffset;
    const f32 v2 = static_cast<f32>(height) * textureScale * (0.5f / radius) + v1;

    // 渲染四个面（四个方向）

    // 面1: 背面 (Z-)
    _addQuad(vertices,
        indices,
        static_cast<f32>(yOffset),
        static_cast<f32>(yOffset + height),
        0.0f,
        -radius,
        radius,
        -radius,
        0.0f,
        1.0f,
        v2,
        v1,
        colors[0],
        colors[1],
        colors[2],
        alpha);

    // 面2: 正面 (Z+)
    _addQuad(vertices,
        indices,
        static_cast<f32>(yOffset),
        static_cast<f32>(yOffset + height),
        radius,
        -radius,
        radius,
        radius,
        0.0f,
        1.0f,
        v2,
        v1,
        colors[0],
        colors[1],
        colors[2],
        alpha);

    // 面3: 右面 (X+)
    _addQuad(vertices,
        indices,
        static_cast<f32>(yOffset),
        static_cast<f32>(yOffset + height),
        radius,
        -radius,
        -radius,
        -radius,
        0.0f,
        1.0f,
        v2,
        v1,
        colors[0],
        colors[1],
        colors[2],
        alpha);

    // 面4: 左面 (X-)
    _addQuad(vertices,
        indices,
        static_cast<f32>(yOffset),
        static_cast<f32>(yOffset + height),
        -radius,
        radius,
        -radius,
        -radius,
        0.0f,
        1.0f,
        v2,
        v1,
        colors[0],
        colors[1],
        colors[2],
        alpha);
}

void BeaconBeamModel::_addQuad(std::vector<entity::model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    f32 yMin,
    f32 yMax,
    f32 x1,
    f32 z1,
    f32 x2,
    f32 z2,
    f32 u1,
    f32 u2,
    f32 v1,
    f32 v2,
    f32 r,
    f32 g,
    f32 b,
    f32 alpha) const
{
    const u32 baseIndex = static_cast<u32>(vertices.size());

    // 四个顶点：右上 → 右下 → 左下 → 左上（顺时针绕序）
    // 法线朝上 (0, 1, 0)
    const f32 nx = 0.0f;
    const f32 ny = 1.0f;
    const f32 nz = 0.0f;

    // 顶点0: 右上
    vertices.emplace_back(entity::model::ModelVertex(x1, yMax, z1, u2, v1, nx, ny, nz));

    // 顶点1: 右下
    vertices.emplace_back(entity::model::ModelVertex(x1, yMin, z1, u2, v2, nx, ny, nz));

    // 顶点2: 左下
    vertices.emplace_back(entity::model::ModelVertex(x2, yMin, z2, u1, v2, nx, ny, nz));

    // 顶点3: 左上
    vertices.emplace_back(entity::model::ModelVertex(x2, yMax, z2, u1, v1, nx, ny, nz));

    // 两个三角形（顺时针绕序）
    // 三角形1: 0-1-2
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);

    // 三角形2: 0-2-3
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    // 注意：颜色和透明度需要在渲染时通过顶点颜色或着色器传递
    // 当前的 ModelVertex 不包含颜色分量，需要在渲染管线中处理
    MC_UNUSED(r);
    MC_UNUSED(g);
    MC_UNUSED(b);
    MC_UNUSED(alpha);
}

} // namespace mc::client::renderer::blockentity::model
