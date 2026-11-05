#include "server/interaction/DigSpeed.hpp"

#include <algorithm>

namespace mc::server::interaction {

namespace {

/// 空中挖掘的速度除数（原版 `f /= 5.0F`）
constexpr f32 kOffGroundDivisor = 5.0f;

/// 急迫与潮涌能量每级的速度加成比例（原版 `1 + (amp + 1) * 0.2`）
constexpr f32 kHastePerLevel = 0.2f;

/**
 * @brief 挖掘疲劳各等级的速度乘数
 *
 * 原版是一个四档的 switch：等级 0..2 各有一个精确的十进制字面量（0.3、0.09、0.0027），
 * 高于 2 的等级统一落到 8.1e-4。注意它不是几何级数的近似，而是四个写死的常量，
 * 所以必须逐档照抄，不能按 `0.3^level` 推算。
 */
f32 fatigueMultiplier(i32 amplifier)
{
    switch (amplifier) {
        case 0:
            return 0.3f;
        case 1:
            return 0.09f;
        case 2:
            return 0.0027f;
        default:
            return 8.1e-4f;
    }
}

} // namespace

f32 digSpeed(const DigSpeedInputs& in)
{
    // 本工程在 Release 下开了 -ffast-math，它会把 `speed /= 5.0f` 改写成乘 0.2f。
    // 这两个运算在 IEEE 754 下并不等价：以 1.6f 为输入，除法得 0x3ea3d70a、
    // 乘倒数得 0x3ea3d70b。原版 Java 执行的是真正的除法，所以这里必须局部关闭
    // 倒数优化，否则空中挖掘的速度会与参照实现差一个 ulp。
#pragma clang fp reciprocal(off)
    f32 speed = in.toolSpeed;

    // 效率附魔只在「工具确实比方块快」时生效。空手（toolSpeed = 1.0）挖任何方块都拿不到它。
    if (speed > 1.0f) {
        speed += in.miningEfficiency;
    }

    // 急迫与潮涌能量取等级较高者，两者不叠加。
    const i32 digSpeedAmplifier = std::max(in.hasteAmplifier, in.conduitAmplifier);
    if (digSpeedAmplifier >= 0) {
        speed *= 1.0f + static_cast<f32>(digSpeedAmplifier + 1) * kHastePerLevel;
    }

    if (in.miningFatigueAmplifier >= 0) {
        speed *= fatigueMultiplier(in.miningFatigueAmplifier);
    }

    speed *= in.blockBreakSpeed;

    if (in.eyesInWater) {
        speed *= in.submergedMiningSpeed;
    }

    if (!in.onGround) {
        speed /= kOffGroundDivisor;
    }

    return speed;
}

} // namespace mc::server::interaction
