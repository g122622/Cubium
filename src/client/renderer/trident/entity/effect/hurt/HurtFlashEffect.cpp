#include "HurtFlashEffect.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <algorithm>

namespace mc::client::renderer::entity::effect::hurt {

bool HurtFlashEffect::s_initialized = false;

void HurtFlashEffect::initialize() {
    if (s_initialized) {
        return;
    }

    // 初始化受伤闪烁系统
    // TODO: 加载受伤闪烁纹理

    s_initialized = true;
}

void HurtFlashEffect::cleanup() {
    if (!s_initialized) {
        return;
    }

    // 清理受伤闪烁系统资源
    // TODO: 释放资源

    s_initialized = false;
}

i32 HurtFlashEffect::getPackedOverlay(::mc::LivingEntity& entity, bool whiteFlash) {
    // 参考 MC 1.16.5 OverlayTexture.java 和 LivingRenderer.java:146-148
    // getPackedUV(getU(uIn), getV(hurtTime > 0 || deathTime > 0))
    //
    // OverlayTexture.getU(uIn) = (int)(uIn * 15.0F)
    // OverlayTexture.getV(hurtIn) = hurtIn ? 3 : 10
    // getPackedUV(u, v) = (u << 16) | (v & 0xFFFF)

    i32 hurtTime = entity.hurtTime();
    i32 deathTime = entity.deathTime();

    // 计算 U 值
    f32 u = 0.0f;
    if (whiteFlash) {
        // 道德影响（如药水效果）使用固定 U 值
        u = 3.0f;
    } else if (hurtTime > 0 || deathTime > 0) {
        // 受伤或死亡时的 U 值
        // MC 1.16.5: uIn 是一个动画进度参数，这里简化为使用 hurtTime
        // LivingRenderer 传入 uIn = partialTicks 或其他值
        // TODO 简化实现：使用 hurtTime 进度
        u = static_cast<f32>(hurtTime) / 10.0f;
    }

    // 打包 U 值 - 使用 15 作为因子，不是 16
    i32 packedU = static_cast<i32>(u * static_cast<f32>(OVERLAY_PACKING));

    // 计算 V 值 - 受伤或死亡时 V=3，正常时 V=10
    i32 packedV = (hurtTime > 0 || deathTime > 0) ? OVERLAY_V_HURT : OVERLAY_V_NORMAL;

    return (packedU << 16) | (packedV & 0xFFFF);
}

f64 HurtFlashEffect::getHurtProgress(::mc::LivingEntity& entity) {
    // 参考 MC 1.16.5 LivingEntity.hurtTime
    // hurtTime 从 10 递减到 0
    // 进度 = 1.0 - (hurtTime / 10.0)

    i32 hurtTime = entity.hurtTime();
    constexpr i32 maxHurtTime = 10;

    if (hurtTime <= 0) {
        return 0.0;
    }

    return 1.0 - static_cast<f64>(hurtTime) / static_cast<f64>(maxHurtTime);
}

bool HurtFlashEffect::isHurt(::mc::LivingEntity& entity) {
    // 参考 MC 1.16.5 LivingEntity.isHurt()
    // 检查 hurtTime > 0
    return entity.hurtTime() > 0;
}

math::Vector4f HurtFlashEffect::applyHurtFlash(::mc::LivingEntity& entity, const math::Vector4f& baseColor) {
    // 参考 MC 1.16.5 受伤闪烁效果
    // 受伤时叠加红色

    if (!isHurt(entity)) {
        return baseColor;
    }

    f64 progress = getHurtProgress(entity);

    // 闪烁强度在受伤开始时最强，逐渐减弱
    f64 flashIntensity = 1.0 - progress;
    flashIntensity = std::max(0.0, std::min(1.0, flashIntensity));

    // 叠加红色
    f32 r = std::min(1.0f, baseColor.x + static_cast<f32>(flashIntensity) * 0.5f);
    f32 g = std::max(0.0f, baseColor.y - static_cast<f32>(flashIntensity) * 0.3f);
    f32 b = std::max(0.0f, baseColor.z - static_cast<f32>(flashIntensity) * 0.3f);
    f32 a = baseColor.w;

    return math::Vector4f(r, g, b, a);
}

} // namespace mc::client::renderer::entity::effect::hurt
