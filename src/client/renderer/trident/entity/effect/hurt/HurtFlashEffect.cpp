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

#include "HurtFlashEffect.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector4.hpp"

namespace mc::client::renderer::entity::effect::hurt {

bool HurtFlashEffect::s_initialized = false;

void HurtFlashEffect::initialize()
{
    if (s_initialized) {
        return;
    }

    // 受伤闪烁效果通过着色器实现，不需要加载纹理资源。
    //
    // 实现方式：
    // 1. EntityPipeline 通过 push constant 传递 hurtTime 和 deathTime 到着色器
    // 2. entity.frag 中的 shouldApplyHurtEffect() 和 computeHurtFlashIntensity()
    //    直接计算红色闪烁效果，使用 mix() 与基础颜色混合
    //
    // 参考：shaders/entity.frag 中的受伤效果实现

    s_initialized = true;
}

void HurtFlashEffect::cleanup()
{
    if (!s_initialized) {
        return;
    }

    // 无需清理资源，受伤闪烁效果通过着色器实现，不持有任何 GPU 资源。

    s_initialized = false;
}

i32 HurtFlashEffect::getPackedOverlay(::mc::LivingEntity& entity, bool whiteFlash)
{
    // OverlayTexture 格式: packedUV = u | (v << 16)
    // U 在低 16 位，V 在高 16 位

    i32 hurtTime = entity.hurtTime();
    i32 deathTime = entity.deathTime();

    // 计算 U 值
    f32 u = 0.0f;
    if (whiteFlash) {
        u = 3.0f;
    } else if (hurtTime > 0 || deathTime > 0) {
        u = static_cast<f32>(hurtTime) / 10.0f;
    }

    // 打包 U 值
    i32 packedU = static_cast<i32>(u * static_cast<f32>(OVERLAY_PACKING));

    // 受伤或死亡时 V=3，正常时 V=10
    i32 packedV = (hurtTime > 0 || deathTime > 0) ? OVERLAY_V_HURT : OVERLAY_V_NORMAL;

    return packedU | (packedV << 16);
}

f64 HurtFlashEffect::getHurtProgress(::mc::LivingEntity& entity)
{
    // hurtTime 从 10 递减到 0，进度 = 1.0 - (hurtTime / 10.0)
    i32 hurtTime = entity.hurtTime();
    constexpr i32 maxHurtTime = 10;

    if (hurtTime <= 0) {
        return 0.0;
    }

    return 1.0 - static_cast<f64>(hurtTime) / static_cast<f64>(maxHurtTime);
}

bool HurtFlashEffect::isHurt(::mc::LivingEntity& entity)
{
    return entity.hurtTime() > 0;
}

math::Vector4f HurtFlashEffect::applyHurtFlash(::mc::LivingEntity& entity, const math::Vector4f& baseColor)
{
    if (!isHurt(entity)) {
        return baseColor;
    }

    f64 progress = getHurtProgress(entity);

    // 闪烁强度在受伤开始时最强，逐渐减弱
    f64 flashIntensity = math::clamp(1.0 - progress, 0.0, 1.0);

    // 叠加红色
    f32 r = math::clamp(baseColor.x + static_cast<f32>(flashIntensity) * 0.5f, 0.0f, 1.0f);
    f32 g = math::clamp(baseColor.y - static_cast<f32>(flashIntensity) * 0.3f, 0.0f, 1.0f);
    f32 b = math::clamp(baseColor.z - static_cast<f32>(flashIntensity) * 0.3f, 0.0f, 1.0f);
    f32 a = baseColor.w;

    return math::Vector4f(r, g, b, a);
}

} // namespace mc::client::renderer::entity::effect::hurt
