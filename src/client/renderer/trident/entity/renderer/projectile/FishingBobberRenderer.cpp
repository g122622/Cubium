/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FishingBobberRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

using model::ModelVertex;

FishingBobberRenderer::FishingBobberRenderer()
{
    m_shadowSize = 0.0;
}

void FishingBobberRenderer::render(Entity& entity, f64 partialTicks)
{
    // Vulkan 管线路径处理渲染，此方法为传统渲染路径保留
    (void)entity;
    (void)partialTicks;
}

bool FishingBobberRenderer::generateMesh(
    ClientEntity& entity, std::vector<ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 钓鱼浮标渲染：浮标四边形 + 钓线
    // 目前仅生成浮标四边形；钓线需要从浮标到玩家手持位置计算，
    // 但 generateMesh 无法访问其他实体，钓线在 renderWithPipeline 中单独处理
    _generateBobberQuad(vertices, indices);
    return !vertices.empty() && !indices.empty();
}

bool FishingBobberRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

void FishingBobberRenderer::_generateBobberQuad(std::vector<ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 浮标尺寸：0.25 x 0.25 的小四边形
    // 参考 MC 1.16.5 FishingBobberEntity 的大小
    const f32 halfWidth = 0.125f;
    const f32 height = 0.25f;
    const f32 yOffset = 0.25f; // 浮标浮在水面上的高度偏移

    // 前面 (Z+)
    vertices.emplace_back(ModelVertex{Vector3f(-halfWidth, yOffset, 0.0f), Vector2f(0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f)});
    vertices.emplace_back(ModelVertex{Vector3f(halfWidth, yOffset, 0.0f), Vector2f(1.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f)});
    vertices.emplace_back(ModelVertex{Vector3f(halfWidth, yOffset + height, 0.0f), Vector2f(1.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f)});
    vertices.emplace_back(ModelVertex{Vector3f(-halfWidth, yOffset + height, 0.0f), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f)});

    // 背面 (Z-)
    vertices.emplace_back(ModelVertex{Vector3f(halfWidth, yOffset, 0.0f), Vector2f(0.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f)});
    vertices.emplace_back(ModelVertex{Vector3f(-halfWidth, yOffset, 0.0f), Vector2f(1.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f)});
    vertices.emplace_back(
        ModelVertex{Vector3f(-halfWidth, yOffset + height, 0.0f), Vector2f(1.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f)});
    vertices.emplace_back(
        ModelVertex{Vector3f(halfWidth, yOffset + height, 0.0f), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f)});

    // 前面索引
    indices.emplace_back(0);
    indices.emplace_back(1);
    indices.emplace_back(2);
    indices.emplace_back(0);
    indices.emplace_back(2);
    indices.emplace_back(3);

    // 背面索引
    indices.emplace_back(4);
    indices.emplace_back(5);
    indices.emplace_back(6);
    indices.emplace_back(4);
    indices.emplace_back(6);
    indices.emplace_back(7);
}

void FishingBobberRenderer::_generateFishingLine(const Vector3f& bobberPos,
    const Vector3f& playerHandPos,
    std::vector<ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 参考 MC 1.16.5 FishRenderer：16 段抛物线
    constexpr i32 SEGMENTS = 16;

    Vector3f start = playerHandPos;
    Vector3f end = bobberPos;

    // 计算钓线的中点下垂量
    f32 dx = end.x - start.x;
    f32 dy = end.y - start.y;
    f32 dz = end.z - start.z;
    f32 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 下垂量取决于水平距离，最大约 0.25 格
    f32 sag = horizontalDist * 0.1f + 0.15f;

    for (i32 i = 0; i <= SEGMENTS; ++i) {
        f32 t = static_cast<f32>(i) / static_cast<f32>(SEGMENTS);

        // 线性插值 + 抛物线下垂
        f32 x = start.x + (end.x - start.x) * t;
        f32 z = start.z + (end.z - start.z) * t;
        // Y 轴：线性插值 + sin 曲线模拟重力下垂
        f32 y = start.y + (end.y - start.y) * t - sag * std::sin(t * static_cast<f32>(math::PI));

        vertices.emplace_back(ModelVertex{Vector3f(x, y, z), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});

        if (i > 0) {
            indices.emplace_back(static_cast<u32>(vertices.size() - 2));
            indices.emplace_back(static_cast<u32>(vertices.size() - 1));
        }
    }
}

} // namespace mc::client::renderer::entity::renderer::projectile
