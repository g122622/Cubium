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

#include "LlamaRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include <string>

namespace mc::client::renderer::entity::renderer::animal {

using mc::LlamaEntity;

LlamaRenderer::LlamaRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.7f);
}

void LlamaRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& llama = static_cast<LlamaEntity&>(entity);

    // 选择模型（幼体或成体）
    bool isChild = llama.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置箱子状态
    model.setHasChest(llama.hasChest());

    // 设置幼体状态
    model.setChild(isChild);

    // 计算动画参数 - 从 LivingEntity 获取
    f64 limbSwing = static_cast<f64>(llama.prevLimbSwing()) +
        (static_cast<f64>(llama.limbSwing()) - static_cast<f64>(llama.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmount = static_cast<f64>(llama.prevLimbSwingAmount()) +
        (static_cast<f64>(llama.limbSwingAmount()) - static_cast<f64>(llama.prevLimbSwingAmount())) * partialTicks;
    f64 ageInTicks = static_cast<f64>(llama.ticksExisted());

    // 头部旋转
    f64 bodyYaw = static_cast<f64>(llama.prevRenderYawOffset()) +
        (static_cast<f64>(llama.renderYawOffset()) - static_cast<f64>(llama.prevRenderYawOffset())) * partialTicks;
    f64 headYaw = static_cast<f64>(llama.prevRotationYawHead()) +
        (static_cast<f64>(llama.rotationYawHead()) - static_cast<f64>(llama.prevRotationYawHead())) * partialTicks;
    f64 netHeadYaw = headYaw - bodyYaw;
    netHeadYaw = static_cast<f64>(mc::math::wrapDegrees(static_cast<f32>(netHeadYaw)));

    f64 headPitch = static_cast<f64>(llama.prevPitch()) +
        (static_cast<f64>(llama.pitch()) - static_cast<f64>(llama.prevPitch())) * partialTicks;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation LlamaRenderer::getEntityTexture(::mc::LlamaEntity& entity)
{
    return static_cast<const LlamaRenderer*>(this)->getEntityTexture(static_cast<const ::mc::LlamaEntity&>(entity));
}

ResourceLocation LlamaRenderer::getEntityTexture(const ::mc::LlamaEntity& entity) const
{
    static const char* colorNames[] = {"creamy", "white", "brown", "gray"};

    i32 variant = static_cast<i32>(entity.getColor());
    MC_ASSERT_RELEASE_MSG(variant >= 0 && variant <= 3, "Invalid llama color variant");

    std::string textureName = "textures/entity/llama/";
    textureName += colorNames[variant];
    textureName += ".png";

    return ResourceLocation("minecraft", textureName);
}

} // namespace mc::client::renderer::entity::renderer::animal
