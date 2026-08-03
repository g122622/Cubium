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

#include "CatRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <array>

namespace mc::client::renderer::entity::renderer::animal {

using mc::CatEntity;

namespace {
// 猫类型纹理
// 0: Tabby, 1: Black, 2: Red, 3: Siamese, 4: British Shorthair
// 5: Calico, 6: Persian, 7: Ragdoll, 8: White, 9: Jellie, 10: All Black
constexpr std::array CAT_TEXTURES = {"textures/entity/cat/tabby.png",
    "textures/entity/cat/black.png",
    "textures/entity/cat/red.png",
    "textures/entity/cat/siamese.png",
    "textures/entity/cat/british_shorthair.png",
    "textures/entity/cat/calico.png",
    "textures/entity/cat/persian.png",
    "textures/entity/cat/ragdoll.png",
    "textures/entity/cat/white.png",
    "textures/entity/cat/jellie.png",
    "textures/entity/cat/all_black.png"};
} // namespace

CatRenderer::CatRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.4f);
}

void CatRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& cat = static_cast<CatEntity&>(entity);

    // 选择模型（幼体或成体）
    bool isChild = cat.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置猫特有动画状态
    f32 lieDownAmount = cat.getLieDownAmount(static_cast<f32>(partialTicks));
    f32 relaxStateAmount = cat.getRelaxStateOneAmount(static_cast<f32>(partialTicks));
    // sleepPoseAmount 使用 lieDownAmount：当猫完全躺下且在睡眠玩家上方时，应用睡眠姿势
    f32 sleepPoseAmount = (cat.isLieDown() && cat.isLyingOnTopOfSleepingPlayer()) ? lieDownAmount : 0.0f;
    model.setCatAnimState(lieDownAmount, relaxStateAmount, sleepPoseAmount);
    model.setSitting(cat.isSitting());

    // 在设置状态之后、设置角度之前，调用 setLivingAnimations 来根据状态调整模型部件位置
    // CatModel::setLivingAnimations 会调用 OcelotModel::setLivingAnimations 处理蹲伏/奔跑/坐下姿态
    f64 limbSwingForAnim = static_cast<f64>(cat.prevLimbSwing()) +
        (static_cast<f64>(cat.limbSwing()) - static_cast<f64>(cat.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmountForAnim = static_cast<f64>(cat.prevLimbSwingAmount()) +
        (static_cast<f64>(cat.limbSwingAmount()) - static_cast<f64>(cat.prevLimbSwingAmount())) * partialTicks;
    model.setLivingAnimations(limbSwingForAnim, limbSwingAmountForAnim, partialTicks);

    // 计算动画参数 - 从 TameableEntity（继承自 AnimalEntity -> LivingEntity）获取
    f64 limbSwing = static_cast<f64>(cat.prevLimbSwing()) +
        (static_cast<f64>(cat.limbSwing()) - static_cast<f64>(cat.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmount = static_cast<f64>(cat.prevLimbSwingAmount()) +
        (static_cast<f64>(cat.limbSwingAmount()) - static_cast<f64>(cat.prevLimbSwingAmount())) * partialTicks;
    f64 ageInTicks = static_cast<f64>(cat.ticksExisted());

    // 头部旋转
    f64 bodyYaw = static_cast<f64>(cat.prevRenderYawOffset()) +
        (static_cast<f64>(cat.renderYawOffset()) - static_cast<f64>(cat.prevRenderYawOffset())) * partialTicks;
    f64 headYaw = static_cast<f64>(cat.prevRotationYawHead()) +
        (static_cast<f64>(cat.rotationYawHead()) - static_cast<f64>(cat.prevRotationYawHead())) * partialTicks;
    f64 netHeadYaw = headYaw - bodyYaw;
    while (netHeadYaw < -180.0)
        netHeadYaw += 360.0;
    while (netHeadYaw > 180.0)
        netHeadYaw -= 360.0;

    f64 headPitch = static_cast<f64>(cat.prevPitch()) +
        (static_cast<f64>(cat.pitch()) - static_cast<f64>(cat.prevPitch())) * partialTicks;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation CatRenderer::getEntityTexture(CatEntity& entity)
{
    u32 catType = static_cast<u32>(entity.getCatType());
    return _getCatTexture(catType);
}

ResourceLocation CatRenderer::getEntityTexture(const CatEntity& entity) const
{
    u32 catType = static_cast<u32>(entity.getCatType());
    return _getCatTexture(catType);
}

ResourceLocation CatRenderer::_getCatTexture(u32 catType)
{
    if (catType >= CAT_TEXTURES.size()) {
        catType = 0;
    }
    return ResourceLocation("minecraft", CAT_TEXTURES[catType]);
}

} // namespace mc::client::renderer::entity::renderer::animal
