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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"
#include "common/resource/ResourceLocation.hpp"

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

    // 设置模型动画状态（对齐 MC Java OcelotRenderer.extractRenderState）
    // isCrouching: 被诱惑时潜行（对应 EntityPose::Crouching）
    // isSprinting: 逃避玩家时冲刺（对应 isFleeing()）
    bool isCrouching = ocelot.pose() == mc::entity::EntityPose::Crouching;
    bool isSprinting = ocelot.isFleeing();
    model.setCrouching(isCrouching);
    model.setSprinting(isSprinting);

    // 在设置状态之后、设置角度之前，调用 setLivingAnimations 来根据状态调整模型部件位置
    f64 limbSwingForAnim = static_cast<f64>(ocelot.prevLimbSwing()) +
        (static_cast<f64>(ocelot.limbSwing()) - static_cast<f64>(ocelot.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmountForAnim = static_cast<f64>(ocelot.prevLimbSwingAmount()) +
        (static_cast<f64>(ocelot.limbSwingAmount()) - static_cast<f64>(ocelot.prevLimbSwingAmount())) * partialTicks;
    model.setLivingAnimations(limbSwingForAnim, limbSwingAmountForAnim, partialTicks);

    // 计算动画参数（对齐 MC Java LivingEntityRenderer.extractRenderState）
    // limbSwing: 步态动画周期，幼体 3 倍速
    f64 limbSwingAmount = static_cast<f64>(ocelot.prevLimbSwingAmount()) +
        (static_cast<f64>(ocelot.limbSwingAmount()) - static_cast<f64>(ocelot.prevLimbSwingAmount())) * partialTicks;
    if (limbSwingAmount > 1.0) {
        limbSwingAmount = 1.0;
    }
    f64 limbSwing = static_cast<f64>(ocelot.limbSwing()) - limbSwingAmount * (1.0 - partialTicks);
    if (isChild) {
        limbSwing *= 3.0;
    }

    f64 ageInTicks = static_cast<f64>(ocelot.ticksExisted()) + partialTicks;

    // headYaw: 头部相对于身体的偏航角，归一化到 [-180, 180]
    f64 bodyYaw = static_cast<f64>(ocelot.prevRenderYawOffset()) +
        (static_cast<f64>(ocelot.renderYawOffset()) - static_cast<f64>(ocelot.prevRenderYawOffset())) * partialTicks;
    f64 headYawRaw = static_cast<f64>(ocelot.prevRotationYawHead()) +
        (static_cast<f64>(ocelot.rotationYawHead()) - static_cast<f64>(ocelot.prevRotationYawHead())) * partialTicks;
    f64 netHeadYaw = headYawRaw - bodyYaw;
    while (netHeadYaw < -180.0)
        netHeadYaw += 360.0;
    while (netHeadYaw > 180.0)
        netHeadYaw -= 360.0;

    // headPitch: 头部俯仰角
    f64 headPitch = static_cast<f64>(ocelot.prevPitch()) +
        (static_cast<f64>(ocelot.pitch()) - static_cast<f64>(ocelot.prevPitch())) * partialTicks;

    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation OcelotRenderer::getEntityTexture(OcelotEntity& entity)
{
    // MC 1.14+ 中猫从豹猫拆分为独立实体（CatEntity），豹猫仅保留野生皮肤
    // OcelotType 枚举中 1-10 为历史兼容类型（已废弃），无对应纹理
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/cat/ocelot.png");
}

ResourceLocation OcelotRenderer::getEntityTexture(const OcelotEntity& entity) const
{
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/cat/ocelot.png");
}

} // namespace mc::client::renderer::entity::renderer::animal
