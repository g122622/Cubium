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

#include "WolfRenderer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/layer/entity/WolfCollarLayer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::renderer::animal {

using mc::WolfEntity;

WolfRenderer::WolfRenderer()
    : m_model()
    , m_modelBaby()
{
    setShadowSize(0.5f);
    _setupLayers();
}

void WolfRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& wolf = static_cast<WolfEntity&>(entity);

    // 设置模型动画状态
    bool isSitting = wolf.isSitting();
    bool isAngry = wolf.isAngry();
    bool isWet = wolf.isWet(); // 使用 isWet 而非 isInWater()，对应 MC Wolf.isWet
    f32 tailRotation = wolf.getTailAngle();
    // 甩水动画进度（插值后）：对应 MC WolfRenderState.shakeAnim
    f32 shakeAnim = wolf.getShakeAnim(static_cast<f32>(partialTicks));
    // 乞求食物头部角度（插值后）：对应 MC Wolf.getHeadRollAngle()
    f32 interestedAngle = wolf.getHeadRollAngle(static_cast<f32>(partialTicks));

    // 选择模型（幼体或成体）
    bool isChild = wolf.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    model.setAnimState(isSitting, isAngry, isWet, tailRotation, shakeAnim, interestedAngle);

    // 在设置状态之后、设置角度之前，调用 setLivingAnimations 来根据状态调整模型部件位置
    // WolfModel::setLivingAnimations 处理坐下/站立姿态、步态动画和抖水动画
    f64 limbSwingForAnim = static_cast<f64>(wolf.prevLimbSwing()) +
        (static_cast<f64>(wolf.limbSwing()) - static_cast<f64>(wolf.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmountForAnim = static_cast<f64>(wolf.prevLimbSwingAmount()) +
        (static_cast<f64>(wolf.limbSwingAmount()) - static_cast<f64>(wolf.prevLimbSwingAmount())) * partialTicks;
    model.setLivingAnimations(limbSwingForAnim, limbSwingAmountForAnim, partialTicks);

    // 设置湿状态着色（对应 MC Wolf.getWetShade()）
    f32 wetShade = wolf.getWetShade(static_cast<f32>(partialTicks));
    model.setTint(wetShade, wetShade, wetShade);

    // 计算动画参数（从LivingEntity获取）
    f64 limbSwing = static_cast<f64>(wolf.prevLimbSwing()) +
        (static_cast<f64>(wolf.limbSwing()) - static_cast<f64>(wolf.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmount = static_cast<f64>(wolf.prevLimbSwingAmount()) +
        (static_cast<f64>(wolf.limbSwingAmount()) - static_cast<f64>(wolf.prevLimbSwingAmount())) * partialTicks;
    f64 ageInTicks = static_cast<f64>(wolf.ticksExisted());
    f64 headYaw = static_cast<f64>(wolf.prevRotationYawHead()) +
        (static_cast<f64>(wolf.rotationYawHead()) - static_cast<f64>(wolf.prevRotationYawHead())) * partialTicks;
    f64 headPitch = static_cast<f64>(wolf.prevPitch()) +
        (static_cast<f64>(wolf.pitch()) - static_cast<f64>(wolf.prevPitch())) * partialTicks;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation WolfRenderer::getEntityTexture(WolfEntity& entity)
{
    // 根据狼的状态选择纹理：驯服、愤怒、普通
    if (entity.isTamed()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_tame.png");
    }
    if (entity.isAngry()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_angry.png");
    }
    return ResourceLocation("minecraft", "textures/entity/wolf/wolf.png");
}

ResourceLocation WolfRenderer::getEntityTexture(const WolfEntity& entity) const
{
    if (entity.isTamed()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_tame.png");
    }
    if (entity.isAngry()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_angry.png");
    }
    return ResourceLocation("minecraft", "textures/entity/wolf/wolf.png");
}

void WolfRenderer::_setupLayers()
{
    // 注册项圈层（对应 MC 1.21.11 WolfRenderer 构造函数中的 addLayer(new WolfCollarLayer(this))）
    // WolfCollarLayer 通过 ClientEntity 的元数据镜像字段读取驯服状态和颈圈颜色，
    // 仅驯服的狼渲染项圈，颜色由 WolfEntity::DATA_COLLAR_COLOR_PARAM 同步。
    m_collarLayer = std::make_unique<layer::entity::WolfCollarLayer>();

    // TODO: 狼铠层待实现后注册：
    // WolfArmorLayer - 狼铠层（需要狼铠纹理资源和裂纹覆盖纹理）
    //
    // 狼铠渲染需要以下纹理资源（MC 1.21.11）：
    // - textures/entity/wolf/wolf_armor.png （狼铠基础纹理）
    // - textures/entity/wolf/wolf_armor_crackiness_low.png
    // - textures/entity/wolf/wolf_armor_crackiness_medium.png
    // - textures/entity/wolf/wolf_armor_crackiness_high.png
    //
    // 渲染逻辑参考 net.minecraft.client.renderer.entity.layers.WolfArmorLayer:
    // - 检查实体是否穿戴狼铠（getBodyArmorItem 非空且为 WolfArmorItem）
    // - 使用 WolfModel 渲染狼铠模型
    // - 根据 Crackiness::WOLF_ARMOR 裂纹等级渲染裂纹覆盖纹理
}

void WolfRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 分发到已注册的层
    // 对应 MC 1.21.11 RenderLayer 遍历调用 submit() 的逻辑
    if (m_collarLayer != nullptr) {
        if (m_collarLayer->shouldRender(entity)) {
            m_collarLayer->renderPipeline(entity, cmd, context, pipeline);
        }
    }
}

} // namespace mc::client::renderer::entity::renderer::animal
