#pragma once

#include "common/core/Types.hpp"
#include <array>

namespace mc::ecs {

/**
 * @brief 盔甲架姿态（六部位欧拉角，单位度）
 *
 * 对应 vanilla ArmorStand 通过 Pose 复合标签持久化的六个部位旋转。
 */
struct ArmorStandPose {
    std::array<f32, 3> head{};     ///< 头部姿态（x, y, z）
    std::array<f32, 3> body{};     ///< 身体姿态
    std::array<f32, 3> leftArm{};  ///< 左臂姿态
    std::array<f32, 3> rightArm{}; ///< 右臂姿态
    std::array<f32, 3> leftLeg{};  ///< 左腿姿态
    std::array<f32, 3> rightLeg{}; ///< 右腿姿态
};

/**
 * @brief 盔甲架状态组件
 *
 * 承载 ArmorStandEntity 的特有状态字段：
 * - m_invisible：不可见（存盘键 Invisible）
 * - m_small：小型盔甲架
 * - m_arms：显示手臂（为 false 时手部槽位不可交互）
 * - m_disabledSlots：槽位禁用掩码（低半字节禁用放入、高半字节禁用取出）
 * - m_basePlate：是否显示底座（仅渲染标志）
 * - m_pose：六部位姿态
 *
 * 不可见同时驱动"是否可见"与"是否忽略爆炸"两条逻辑（vanilla ArmorStand），
 * 是盔甲架独占的状态，与 Entity 级通用标志位（游泳/发光/鞘翅飞行等）无对应关系。
 * 仅 ArmorStandEntity 构造时 attach；NBT 序列化经 ArmorStandComponentSerialization 注册。
 */
struct ArmorStandStateComponent {
    bool m_invisible = false;
    bool m_small = false;
    bool m_arms = false;
    i32 m_disabledSlots = 0;
    bool m_basePlate = true;
    ArmorStandPose m_pose{};
};

} // namespace mc::ecs
