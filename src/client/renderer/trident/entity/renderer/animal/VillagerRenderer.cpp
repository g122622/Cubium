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

#include "VillagerRenderer.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

VillagerRenderer::VillagerRenderer()
    : m_model(0.0f)
{
    setShadowSize(0.5f);
}

void VillagerRenderer::render(Entity& entity, f64 partialTicks)
{
    // 计算动画参数
    f64 limbSwing = 0.0;
    f64 limbSwingAmount = 0.0;
    f64 ageInTicks = static_cast<f64>(entity.ticksExisted());
    f64 headYaw = entity.prevYaw() + (entity.yaw() - entity.prevYaw()) * partialTicks;
    f64 headPitch = entity.prevPitch() + (entity.pitch() - entity.prevPitch()) * partialTicks;

    m_model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, 1.0);
    m_model.render(1.0 / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation VillagerRenderer::getEntityTexture(::mc::VillagerEntity& entity)
{
    // TODO: 根据村民职业和生物群系选择纹理
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/villager/villager.png");
}

ResourceLocation VillagerRenderer::getEntityTexture(const ::mc::VillagerEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/villager/villager.png");
}

void registerVillagerRenderer(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:villager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<VillagerRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::animal
