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
    // 参考 MC 1.16.5 LivingRenderer.getPackedOverlay()
    // 计算覆盖层 UV：
    // - U = hurtTime / 10.0F * 16.0F
    // - V = 0
    // - 道德影响时 U 固定为 3.0F

    i32 hurtTime = entity.hurtTime();

    f32 u = 0.0f;
    if (whiteFlash) {
        // 道德影响（如药水效果）使用白色闪烁
        u = 3.0f;
    } else if (hurtTime > 0) {
        // 受伤闪烁
        u = static_cast<f32>(hurtTime) / 10.0f * 16.0f;
    }

    // V 始终为 0
    f32 v = 0.0f;

    // 打包 UV 为 16 位整数
    // packed = (int)(u * 16) << 16 | (int)(v * 16)
    i32 packedU = static_cast<i32>(u * OVERLAY_PACKING);
    i32 packedV = static_cast<i32>(v * OVERLAY_PACKING);

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
