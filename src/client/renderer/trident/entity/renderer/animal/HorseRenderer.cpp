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

#include "HorseRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/horse/CoatColors.hpp"
#include "common/entity/entities/passive/horse/CoatTypes.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include <string>

namespace mc::client::renderer::entity::renderer::animal {

using mc::HorseEntity;

HorseRenderer::HorseRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.85f);
    _setupLayers();
}

void HorseRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& horse = static_cast<HorseEntity&>(entity);

    // 选择模型（幼体或成体）
    bool isChild = horse.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置鞍和骑乘状态
    model.setSaddled(horse.hasSaddle());
    model.setRidden(horse.isBeingRidden());

    // 计算动画参数 - 从 LivingEntity 获取
    f64 limbSwing = static_cast<f64>(horse.prevLimbSwing()) +
        (static_cast<f64>(horse.limbSwing()) - static_cast<f64>(horse.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmount = static_cast<f64>(horse.prevLimbSwingAmount()) +
        (static_cast<f64>(horse.limbSwingAmount()) - static_cast<f64>(horse.prevLimbSwingAmount())) * partialTicks;
    f64 ageInTicks = static_cast<f64>(horse.ticksExisted());

    // 头部旋转
    f64 bodyYaw = static_cast<f64>(horse.prevRenderYawOffset()) +
        (static_cast<f64>(horse.renderYawOffset()) - static_cast<f64>(horse.prevRenderYawOffset())) * partialTicks;
    f64 headYaw = static_cast<f64>(horse.prevRotationYawHead()) +
        (static_cast<f64>(horse.rotationYawHead()) - static_cast<f64>(horse.prevRotationYawHead())) * partialTicks;
    f64 netHeadYaw = headYaw - bodyYaw;
    netHeadYaw = static_cast<f64>(mc::math::wrapDegrees(static_cast<f32>(netHeadYaw)));

    f64 headPitch = static_cast<f64>(horse.prevPitch()) +
        (static_cast<f64>(horse.pitch()) - static_cast<f64>(horse.prevPitch())) * partialTicks;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation HorseRenderer::getEntityTexture(::mc::HorseEntity& entity)
{
    return static_cast<const HorseRenderer*>(this)->getEntityTexture(static_cast<const ::mc::HorseEntity&>(entity));
}

ResourceLocation HorseRenderer::getEntityTexture(const ::mc::HorseEntity& entity) const
{
    CoatColors color = entity.getColor();
    CoatTypes marking = entity.getMarking();

    std::string textureName = "textures/entity/horse/horse_";
    textureName += getCoatColorName(color);
    if (marking != CoatTypes::None) {
        textureName += getCoatTypeName(marking);
    }
    textureName += ".png";

    return ResourceLocation("minecraft", textureName);
}

void HorseRenderer::_setupLayers()
{
    // TODO: 层渲染器（鞍、马铠）将在层系统完善后添加
}

} // namespace mc::client::renderer::entity::renderer::animal
