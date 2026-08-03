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

#include "BillboardRenderers.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc::client::renderer::entity::renderer::projectile {

using model::ModelVertex;

// ============================================================================
// ItemBillboardRenderer
// ============================================================================

ItemBillboardRenderer::ItemBillboardRenderer(bool fullbright, f64 scale)
    : m_fullbright(fullbright)
    , m_scale(scale)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void ItemBillboardRenderer::render(Entity& entity, f64 partialTicks)
{
    (void)entity;
    (void)partialTicks;
}

bool ItemBillboardRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    // 全亮光照由 EntityRenderer::isFullbright() 控制，
    // 当 m_fullbright 为 true 时，渲染管线会将光照混合到最大亮度 1.0，
    // 使投掷物在黑暗中也清晰可见（例如末影之眼、火球等）。
    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）：位置(x,y,z) + 纹理坐标(u,v) + 法线(nx,ny,nz)
        ModelVertex(-halfWidth, yOffset, 0.0, 0.0, 1.0, 0.0, 0.0, -1.0),
        ModelVertex(-halfWidth, yOffset + height, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0),
        ModelVertex(halfWidth, yOffset + height, 0.0, 1.0, 0.0, 0.0, 0.0, -1.0),
        ModelVertex(halfWidth, yOffset, 0.0, 1.0, 1.0, 0.0, 0.0, -1.0),
        // 正面（法线 +Z）
        ModelVertex(halfWidth, yOffset, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0),
        ModelVertex(halfWidth, yOffset + height, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0),
        ModelVertex(-halfWidth, yOffset + height, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0),
        ModelVertex(-halfWidth, yOffset, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0),
    };

    indices = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};

    return true;
}

bool ItemBillboardRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

} // namespace mc::client::renderer::entity::renderer::projectile
