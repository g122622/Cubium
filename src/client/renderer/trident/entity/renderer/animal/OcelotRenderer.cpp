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

#include "OcelotRenderer.hpp"
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

using mc::OcelotEntity;

OcelotRenderer::OcelotRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.5f);
}

void OcelotRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& ocelot = static_cast<OcelotEntity&>(entity);

    // 选择模型（幼体或成体）
    bool isChild = ocelot.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置状态（蹲伏/奔跑/站立）
    model.setCrouching(ocelot.isFleeing());
    model.setSprinting(ocelot.isFleeing());

    // 计算动画参数
    // TODO: 实现豹猫的动画参数计算（肢体摆动、头部朝向等），当前均为默认值
    f64 limbSwing = 0.0;
    f64 limbSwingAmount = 0.0;
    f64 ageInTicks = static_cast<f64>(ocelot.ticksExisted());
    f64 headYaw = 0.0;
    f64 headPitch = 0.0;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation OcelotRenderer::getEntityTexture(OcelotEntity& entity)
{
    // TODO: 根据豹猫变种返回不同纹理，当前忽略变种信息
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/cat/ocelot.png");
}

ResourceLocation OcelotRenderer::getEntityTexture(const OcelotEntity& entity) const
{
    // TODO: 根据豹猫变种返回不同纹理，当前忽略变种信息
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/cat/ocelot.png");
}

} // namespace mc::client::renderer::entity::renderer::animal
