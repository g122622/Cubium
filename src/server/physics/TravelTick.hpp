#pragma once

#include "common/core/Types.hpp"

namespace mc::server::physics {

/**
 * @brief 影响一次空中移动 tick 的全部输入
 *
 * 对应原版 `LivingEntity.travelInAir` 及其调用链上的每一个量。收进一个纯数据结构体，
 * 是为了让这条链能被单独验证——差分测试需要覆盖摩擦、重力、悬浮、缓降的各种组合，
 * 而 `LivingEntity` 的对应成员函数依赖完整的世界与实体状态。
 */
struct TravelTickInputs {
    /// 水平输入速度（原版 getSpeed()，即 MOVEMENT_SPEED 属性值）
    f32 horizontalSpeed;

    /// 脚下方块的滑度（原版 Block.getFriction；站着时为该值，空中为 1.0）
    f32 slipperiness;

    /// 上一次 tick 结束时的竖直速度（决定缓降是否生效、落地判定）
    f32 verticalVelocity;

    /// 实体自身的重力（原版 getGravity；玩家 0.08）
    f32 gravity;

    /// 是否站在地面上
    bool onGround;

    /// 悬浮效果的 amplifier（没有该效果时为 -1）
    i32 levitationAmplifier;

    /// 是否带有缓降效果
    bool slowFalling;

    /// 跳跃提升的额外初速度（原版 getJumpBoostPower：每级 0.1）
    f32 jumpBoostPower;

    /// 脚下方块的跳跃因子（原版 getBlockJumpFactor；蜂蜜块 0.5，其它 1.0）
    f32 blockJumpFactor;

    /// jump_strength 属性值（默认 0.42）
    f32 jumpStrength;
};

/**
 * @brief 一次空中移动 tick 的结果
 */
struct TravelTickResult {
    /// 本 tick 用于水平位移的移动因子
    f32 moveFactor;

    /// 本 tick 结束时的竖直速度
    f32 verticalVelocity;

    /// 本 tick 的跳跃初速度（未跳跃时为 0）
    f32 jumpPower;
};

/**
 * @brief 计算一次空中移动 tick
 *
 * 逐环对应原版 `LivingEntity.travelInAir`：
 *
 *   1. 滑度：站在地面时取方块滑度，空中一律为 1.0；
 *   2. 移动因子：在地面为 `speed * 0.21600002 / (f * f * f)`（**除法**，不是乘倒数，
 *      两者在 IEEE 754 下差 1 ulp），在空中为固定的 0.02；
 *   3. 竖直速度：有悬浮效果时按 `(0.05 * (amp + 1) - v) * 0.2` 向该速度收敛，
 *      否则减去重力；缓降在**下落时**把重力压到 `min(gravity, 0.01)`；
 *   4. 跳跃初速度：`jump_strength * blockJumpFactor + jumpBoostPower`。
 *
 * 第 2 环的除法是本函数存在的主要原因：工程在 Release 下开了 `-ffast-math`，
 * 它会把除以 `f*f*f` 改写成乘倒数，与原版差 1 ulp。
 *
 * @param in 全部输入
 * @return 本 tick 的移动因子、竖直速度与跳跃初速度
 */
[[nodiscard]] TravelTickResult travelInAir(const TravelTickInputs& in);

} // namespace mc::server::physics
