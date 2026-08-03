/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the following conditions:
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

#include "VillagerRenderer.hpp"
#include "client/renderer/trident/entity/layer/entity/VillagerLayer.hpp"
#include "client/renderer/trident/entity/model/animal/VillagerModel.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc::client::renderer::entity::renderer::animal {

VillagerRenderer::VillagerRenderer()
{
    m_shadowSize = 0.5f;
    _initLayers();
}

void VillagerRenderer::_initLayers()
{
    // 该层负责渲染类型层、职业层和等级徽章层
    m_villagerLayer =
        addLayer<layer::entity::VillagerLayer<::mc::entity::VillagerEntity, model::animal::VillagerModel>>(
            *this, "villager");
}

void VillagerRenderer::setTextureAtlas(const pipeline::EntityTextureAtlas* atlas)
{
    m_villagerLayer->setTextureAtlas(atlas);
}

ResourceLocation VillagerRenderer::getEntityTexture(::mc::entity::VillagerEntity& entity)
{
    // 只返回基础纹理，类型层和职业层由 VillagerLayer 渲染
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/villager/villager.png");
}

ResourceLocation VillagerRenderer::getEntityTexture(const ::mc::entity::VillagerEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/villager/villager.png");
}

} // namespace mc::client::renderer::entity::renderer::animal
