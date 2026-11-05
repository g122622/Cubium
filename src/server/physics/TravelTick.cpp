#include "server/physics/TravelTick.hpp"

#include <algorithm>

namespace mc::server::physics {

namespace {

/// 地面移动因子的系数（原版 getFrictionInfluencedSpeed 里的 0.21600002F）
constexpr f32 kGroundMoveCoefficient = 0.21600002f;

/// 空中移动因子（原版 getFlyingSpeed 在非玩家操控时的 0.02F）
constexpr f32 kAirMoveFactor = 0.02f;

/// 缓降把重力压到的上限
constexpr f32 kSlowFallingGravity = 0.01f;

/// 悬浮每级提供的上升速度
constexpr f32 kLevitationPerLevel = 0.05f;

/// 悬浮向目标速度收敛的比例
constexpr f32 kLevitationBlend = 0.2f;

/// 空中竖直速度的保留比例（原版 travelInAir 里 d0 * 0.98F）
constexpr f32 kAirVerticalRetention = 0.98f;

} // namespace

TravelTickResult travelInAir(const TravelTickInputs& in)
{
    // 工程在 Release 下开了 -ffast-math，它会把除以 (f*f*f) 改写成乘倒数。两者在
    // IEEE 754 下不等价（除法与倒数乘法会差 1 ulp），而原版 Java 执行的是真正的除法，
    // 所以这里必须局部关闭倒数优化。
#pragma clang fp reciprocal(off)
    // 站在地面时用方块滑度，空中一律按 1.0 处理——原版把这一步写成
    // `onGround ? getFriction() : 1.0F`，空中不受方块滑度影响。
    const f32 friction = in.onGround ? in.slipperiness : 1.0f;

    // 移动因子：地面走滑度公式，空中走固定值。
    const f32 moveFactor =
        in.onGround ? in.horizontalSpeed * kGroundMoveCoefficient / (friction * friction * friction) : kAirMoveFactor;

    // 竖直速度：悬浮优先于重力。
    f32 vertical = in.verticalVelocity;
    if (in.levitationAmplifier >= 0) {
        const f32 target = kLevitationPerLevel * static_cast<f32>(in.levitationAmplifier + 1);
        vertical += (target - vertical) * kLevitationBlend;
    } else {
        // 缓降只在**下落**时生效：上升途中仍按原重力减速，否则跳跃高度会被削掉一半。
        f32 gravity = in.gravity;
        if (in.slowFalling && vertical <= 0.0f) {
            gravity = std::min(gravity, kSlowFallingGravity);
        }
        vertical -= gravity;
    }

    // 竖直速度无条件乘一次保留比例——原版把 `d0 * f2` 写在悬浮分支之后，悬浮也照乘。
    vertical *= kAirVerticalRetention;

    // 跳跃初速度：jump_strength 乘方块跳跃因子，再加上跳跃提升的额外部分（相加，不是相乘）。
    const f32 jumpPower = in.jumpStrength * in.blockJumpFactor + in.jumpBoostPower;

    return TravelTickResult{moveFactor, vertical, jumpPower};
}

} // namespace mc::server::physics
