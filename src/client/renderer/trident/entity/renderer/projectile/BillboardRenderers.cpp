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
#include "client/world/entity/ClientEntity.hpp"

namespace mc::client::renderer::entity::renderer::projectile {

using model::ModelVertex;

// ============================================================================
// SnowballRenderer
// ============================================================================

SnowballRenderer::SnowballRenderer()
    : m_fullbright(false)
    , m_scale(1.0)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void SnowballRenderer::render(Entity& entity, f64 partialTicks)
{
    // 由 Vulkan 管线路径处理渲染
    (void)entity;
    (void)partialTicks;
}

bool SnowballRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    // 生成双面 billboard 四边形
    // 中心位于 Y=0.25，宽度/高度由缩放决定
    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）
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

    indices = {
        // 背面
        0,
        1,
        2,
        0,
        2,
        3,
        // 正面
        4,
        5,
        6,
        4,
        6,
        7,
    };

    return true;
}

bool SnowballRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

// ============================================================================
// EggRenderer
// ============================================================================

EggRenderer::EggRenderer()
    : m_fullbright(false)
    , m_scale(1.0)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void EggRenderer::render(Entity& entity, f64 partialTicks)
{
    (void)entity;
    (void)partialTicks;
}

bool EggRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）
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

bool EggRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

// ============================================================================
// EnderPearlRenderer
// ============================================================================

EnderPearlRenderer::EnderPearlRenderer()
    : m_fullbright(false)
    , m_scale(1.0)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void EnderPearlRenderer::render(Entity& entity, f64 partialTicks)
{
    (void)entity;
    (void)partialTicks;
}

bool EnderPearlRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）
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

bool EnderPearlRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

// ============================================================================
// PotionRenderer
// ============================================================================

PotionRenderer::PotionRenderer()
    : m_fullbright(false)
    , m_scale(1.0)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void PotionRenderer::render(Entity& entity, f64 partialTicks)
{
    (void)entity;
    (void)partialTicks;
}

bool PotionRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）
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

bool PotionRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

// ============================================================================
// ExperienceBottleRenderer
// ============================================================================

ExperienceBottleRenderer::ExperienceBottleRenderer()
    : m_fullbright(false)
    , m_scale(1.0)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void ExperienceBottleRenderer::render(Entity& entity, f64 partialTicks)
{
    (void)entity;
    (void)partialTicks;
}

bool ExperienceBottleRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）
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

bool ExperienceBottleRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

// ============================================================================
// EyeOfEnderRenderer
// ============================================================================

EyeOfEnderRenderer::EyeOfEnderRenderer()
    : m_fullbright(true)
    , m_scale(1.0)
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void EyeOfEnderRenderer::render(Entity& entity, f64 partialTicks)
{
    (void)entity;
    (void)partialTicks;
}

bool EyeOfEnderRenderer::generateMesh(
    ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    (void)entity;

    const f64 baseSize = 0.25;
    const f64 halfWidth = baseSize * m_scale * 0.5;
    const f64 height = baseSize * m_scale;
    const f64 yOffset = 0.25;

    vertices = {
        // 背面（法线 -Z）
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

bool EyeOfEnderRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    (void)entity;
    return false;
}

} // namespace mc::client::renderer::entity::renderer::projectile
