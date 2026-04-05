#pragma once

#include "common/core/Types.hpp"

namespace mc::client::renderer {

/**
 * @brief 手臂姿态
 *
 * 定义手臂在不同使用状态下的姿态。
 * 用于动画系统决定手臂如何摆放。
 *
 * 参考 MC 1.16.5 BipedModel.ArmPose
 */
enum class ArmPose : u8 {
    /// 空手（无物品）
    Empty = 0,

    /// 持有普通物品
    Item = 1,

    /// 格挡（盾牌）
    Block = 2,

    /// 拉弓
    BowAndArrow = 3,

    /// 投掷三叉戟
    ThrowSpear = 4,

    /// 装填弩
    CrossbowCharge = 5,

    /// 持有已装填的弩
    CrossbowHold = 6,

    /// 吃食物/喝药水
    EatOrDrink = 7,

    /// 使用地图
    Map = 8,

    /// 鞘翅飞行
    FallFlying = 9,

    /// 游泳
    Swimming = 10,

    /// 睡眠
    Sleeping = 11,

    /// 潜行
    Sneaking = 12
};

/**
 * @brief 判断手臂姿态是否需要两手持握
 *
 * 某些物品（如弓、三叉戟）需要双手操作，
 * 当主手使用这些物品时，副手不应渲染。
 *
 * @param pose 手臂姿态
 * @return true 如果需要两手持握
 */
[[nodiscard]] inline bool isTwoHanded(ArmPose pose) {
    return pose == ArmPose::BowAndArrow ||
           pose == ArmPose::ThrowSpear ||
           pose == ArmPose::CrossbowCharge;
}

/**
 * @brief 判断手臂姿态是否阻止副手渲染
 *
 * 当主手使用某些物品时，副手应该隐藏或不渲染。
 *
 * @param pose 手臂姿态
 * @return true 如果副手不应渲染
 */
[[nodiscard]] inline bool blocksOffHand(ArmPose pose) {
    return pose == ArmPose::BowAndArrow ||
           pose == ArmPose::ThrowSpear ||
           pose == ArmPose::CrossbowCharge ||
           pose == ArmPose::CrossbowHold;
}

} // namespace mc::client::renderer
