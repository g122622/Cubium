#include "BeaconBeamModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cmath>
#include <cstdlib>

namespace mc::client::renderer::blockentity::model {

BeaconBeamModel::BeaconBeamModel()
    : m_segments()
{
}

f32 BeaconBeamModel::calculateBeamRotation(i64 gameTime, f32 partialTick) {
    // MC 1.16.5: f = Math.floorMod(totalWorldTime, 40L) + partialTicks
    // rotation = f * 2.25F - 45.0F
    const f32 f = static_cast<f32>(math::floorMod(gameTime, ROTATION_PERIOD)) + partialTick;
    return f * ROTATION_SPEED + ROTATION_OFFSET;
}

void BeaconBeamModel::generateMesh(
    std::vector<entity::model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    i64 gameTime,
    f32 partialTick) const
{
    if (m_segments.empty()) {
        return;
    }

    // MC 1.16.5 BeaconTileEntityRenderer.render():
    // 计算纹理 V 坐标偏移
    // float f = (float)Math.floorMod(totalWorldTime, 40L) + partialTicks;
    // float f1 = height < 0 ? f : -f;
    // float f2 = MathHelper.frac(f1 * 0.2F - (float)MathHelper.floor(f1 * 0.1F));
    const f32 f = static_cast<f32>(math::floorMod(gameTime, ROTATION_PERIOD)) + partialTick;
    const f32 vOffset = std::fmod(f * 0.2f - static_cast<f32>(std::floor(f * 0.1f)), 1.0f);

    // 渲染所有光束段
    i32 yOffset = 0;
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const BeamSegment& segment = m_segments[i];

        // MC 1.16.5: 最后一段高度为 1024
        // k == list.size() - 1 ? 1024 : beacontileentity$beamsegment.getHeight()
        const i32 height = (i == m_segments.size() - 1) ? MAX_BEAM_HEIGHT : segment.height;

        // 渲染内层光束（不透明）
        renderSegment(vertices, indices, yOffset, height, segment.colors, vOffset, false);

        // 渲染外层光晕（半透明）
        renderSegment(vertices, indices, yOffset, height, segment.colors, vOffset, true);

        yOffset += segment.height;
    }
}

void BeaconBeamModel::renderSegment(
    std::vector<entity::model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    i32 yOffset,
    i32 height,
    const std::array<f32, 3>& colors,
    f32 vOffset,
    bool isGlow) const
{
    // MC 1.16.5 BeaconTileEntityRenderer.renderBeamSegment():
    // 内层光束: radius = 0.2F, alpha = 1.0F
    // 外层光晕: radius = 0.25F, alpha = 0.125F
    const f32 radius = isGlow ? GLOW_RADIUS : BEAM_RADIUS;
    const f32 alpha = isGlow ? 0.125f : 1.0f;

    // 纹理参数
    // MC 1.16.5: textureScale = 1.0F (参数)
    const f32 textureScale = 1.0f;

    // V 纹理坐标
    // MC 1.16.5:
    // float f15 = -1.0F + f2;
    // float f16 = (float)height * textureScale * (0.5F / beamRadius) + f15;
    const f32 v1 = -1.0f + vOffset;
    const f32 v2 = static_cast<f32>(height) * textureScale * (0.5f / radius) + v1;

    // 渲染四个面（四个方向）
    // MC 1.16.5 renderPart():
    // addQuad(..., yMin, yMax, x1, z1, x2, z2, u1, u2, v1, v2)
    // 注意：MC 中 U 坐标是 0.0 和 1.0

    // 面1: 背面 (Z-)
    // MC: renderPart(..., 0.0F, beamRadius, beamRadius, 0.0F, ...)
    // 参数: x1=0.0, z1=-beamRadius, x2=beamRadius, z2=0.0 (等价)
    // 实际: p_228840_8_=0.0F, p_228840_9_=-beamRadius, p_228840_10_=beamRadius, p_228840_11_=-beamRadius
    // 这是一个面，从 (0, -radius) 到 (radius, -radius)，即 Z 负方向的背面
    addQuad(vertices, indices,
            static_cast<f32>(yOffset), static_cast<f32>(yOffset + height),
            0.0f, -radius, radius, -radius,
            0.0f, 1.0f, v2, v1,
            colors[0], colors[1], colors[2], alpha);

    // 面2: 正面 (Z+)
    // MC: p_228840_8_=radius, p_228840_9_=-radius, p_228840_10_=radius, p_228840_11_=radius
    addQuad(vertices, indices,
            static_cast<f32>(yOffset), static_cast<f32>(yOffset + height),
            radius, -radius, radius, radius,
            0.0f, 1.0f, v2, v1,
            colors[0], colors[1], colors[2], alpha);

    // 面3: 右面 (X+)
    // MC: p_228840_10_=radius, p_228840_11_=-radius, p_228840_14_=radius, p_228840_15_=-radius
    addQuad(vertices, indices,
            static_cast<f32>(yOffset), static_cast<f32>(yOffset + height),
            radius, -radius, -radius, -radius,
            0.0f, 1.0f, v2, v1,
            colors[0], colors[1], colors[2], alpha);

    // 面4: 左面 (X-)
    // MC: p_228840_12_=-radius, p_228840_13_=radius, p_228840_8_=0.0F, p_228840_9_=-radius
    addQuad(vertices, indices,
            static_cast<f32>(yOffset), static_cast<f32>(yOffset + height),
            -radius, radius, -radius, -radius,
            0.0f, 1.0f, v2, v1,
            colors[0], colors[1], colors[2], alpha);
}

void BeaconBeamModel::addQuad(
    std::vector<entity::model::ModelVertex>& vertices,
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
    // MC 1.16.5 BeaconTileEntityRenderer.addQuad():
    // addVertex(..., yMax, x1, z1, u2, v1);
    // addVertex(..., yMin, x1, z1, u2, v2);
    // addVertex(..., yMin, x2, z2, u1, v2);
    // addVertex(..., yMax, x2, z2, u1, v1);
    //
    // addVertex():
    // bufferIn.pos(matrixPos, x, (float)y, z)
    //           .color(red, green, blue, alpha)
    //           .tex(texU, texV)
    //           .overlay(OverlayTexture.NO_OVERLAY)
    //           .lightmap(15728880)
    //           .normal(matrixNormal, 0.0F, 1.0F, 0.0F)
    //           .endVertex();

    const u32 baseIndex = static_cast<u32>(vertices.size());

    // 四个顶点（顺时针，从右上角开始）
    // 顶点0: (x1, yMax, z1), tex(u2, v1)
    // 顶点1: (x1, yMin, z1), tex(u2, v2)
    // 顶点2: (x2, yMin, z2), tex(u1, v2)
    // 顶点3: (x2, yMax, z2), tex(u1, v1)

    // 法线朝上 (0, 1, 0)
    const f32 nx = 0.0f;
    const f32 ny = 1.0f;
    const f32 nz = 0.0f;

    // 顶点0: 右上
    vertices.emplace_back(
        entity::model::ModelVertex(
            x1, yMax, z1,
            u2, v1,
            nx, ny, nz));

    // 顶点1: 右下
    vertices.emplace_back(
        entity::model::ModelVertex(
            x1, yMin, z1,
            u2, v2,
            nx, ny, nz));

    // 顶点2: 左下
    vertices.emplace_back(
        entity::model::ModelVertex(
            x2, yMin, z2,
            u1, v2,
            nx, ny, nz));

    // 顶点3: 左上
    vertices.emplace_back(
        entity::model::ModelVertex(
            x2, yMax, z2,
            u1, v1,
            nx, ny, nz));

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
    // 临时抑制未使用变量警告
    (void)r;
    (void)g;
    (void)b;
    (void)alpha;
}

} // namespace mc::client::renderer::blockentity::model
