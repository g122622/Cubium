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
#include "common/entity/entities/passive/horse/HorseEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

using mc::HorseEntity;

HorseRenderer::HorseRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.85f);
    setupLayers();
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
    // 归一化到 -180 到 180
    while (netHeadYaw < -180.0)
        netHeadYaw += 360.0;
    while (netHeadYaw > 180.0)
        netHeadYaw -= 360.0;

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
    // 根据马颜色和花纹选择纹理
    // MC 1.16.5 有 7 种基础颜色和 5 种花纹
    CoatColors color = entity.getColor();
    CoatTypes marking = entity.getMarking();

    // 纹理命名规则: horse_<color><marking>.png
    // 例如: horse_white.png, horse_whitefield.png
    static const char* colorNames[] = {"white", "creamy", "chestnut", "brown", "black", "gray", "darkbrown"};
    static const char* markingNames[] = {"", "white", "whitefield", "whitedots", "blackdots"};

    std::string textureName = "textures/entity/horse/horse_";
    textureName += colorNames[static_cast<i32>(color)];
    if (marking != CoatTypes::None) {
        textureName += markingNames[static_cast<i32>(marking)];
    }
    textureName += ".png";

    return ResourceLocation("minecraft", textureName);
}

ResourceLocation HorseRenderer::getEntityTexture(const ::mc::HorseEntity& entity) const
{
    CoatColors color = entity.getColor();
    CoatTypes marking = entity.getMarking();

    static const char* colorNames[] = {"white", "creamy", "chestnut", "brown", "black", "gray", "darkbrown"};
    static const char* markingNames[] = {"", "white", "whitefield", "whitedots", "blackdots"};

    std::string textureName = "textures/entity/horse/horse_";
    textureName += colorNames[static_cast<i32>(color)];
    if (marking != CoatTypes::None) {
        textureName += markingNames[static_cast<i32>(marking)];
    }
    textureName += ".png";

    return ResourceLocation("minecraft", textureName);
}

void HorseRenderer::setupLayers()
{
    // 层渲染器将在层系统完善后添加
    // - SaddleLayer: 鞍
    // - HorseArmorLayer: 马铠
}

} // namespace mc::client::renderer::entity::renderer::animal
