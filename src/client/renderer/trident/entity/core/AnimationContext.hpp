#pragma once

#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::core {

/**
 * @brief 动画上下文
 *
 * 存储实体的动画状态，用于传递给渲染器和网格更新器。
 * 参考 MC 1.16.5 LivingRenderer.render() 中的动画参数计算
 */
struct AnimationContext {
    // ========== 部分 Tick ==========

    /**
     * @brief 部分 Tick 值
     *
     * 用于在帧之间插值，范围 [0, 1)
     */
    f64 partialTicks = 0.0;

    // ========== 动画参数 ==========

    /**
     * @brief 步态动画周期
     *
     * 实体移动时的腿部摆动周期。
     * 在 MC 1.16.5 中计算为：
     * limbSwing = entity.limbSwing - entity.limbSwingAmount * (1 - partialTicks)
     */
    f64 limbSwing = 0.0;

    /**
     * @brief 步态动画强度
     *
     * 实体移动速度的表示，范围 [0, 1]。
     * 在 MC 1.16.5 中计算为：
     * limbSwingAmount = lerp(prevLimbSwingAmount, limbSwingAmount, partialTicks)
     */
    f64 limbSwingAmount = 0.0;

    /**
     * @brief 年龄（Tick）
     *
     * 实体存活时间，用于空闲动画（如呼吸、摆动）。
     * 在 MC 1.16.5 中计算为：ticksExisted + partialTicks
     */
    f64 ageInTicks = 0.0;

    /**
     * @brief 头部偏航角（相对于身体）
     *
     * 头部相对于身体的旋转角度，范围 [-180, 180] 度。
     * 在 MC 1.16.5 中计算为：
     * netHeadYaw = interpolateAngle(prevRotationYawHead, rotationYawHead, partialTicks) - bodyYaw
     */
    f64 netHeadYaw = 0.0;

    /**
     * @brief 头部俯仰角
     *
     * 头部上下旋转角度，范围 [-90, 90] 度。
     * 在 MC 1.16.5 中计算为：
     * headPitch = lerp(prevRotationPitch, rotationPitch, partialTicks)
     */
    f64 headPitch = 0.0;

    /**
     * @brief 缩放因子
     *
     * 实体模型的缩放比例，通常为 1.0/16.0。
     * 幼体实体使用更小的缩放（0.5）。
     */
    f64 scale = 1.0 / 16.0;

    // ========== 状态哈希 ==========

    /**
     * @brief 状态哈希值
     *
     * 用于快速比较动画状态是否变化。
     * 当状态变化时需要重新生成网格。
     */
    u32 stateHash = 0;

    // ========== 特殊状态 ==========

    /**
     * @brief 是否正在坐下
     */
    bool isSitting = false;

    /**
     * @brief 是否是幼体
     */
    bool isChild = false;

    /**
     * @brief 是否正在蹲伏
     */
    bool isSneaking = false;

    /**
     * @brief 是否正在游泳
     */
    bool isSwimming = false;

    /**
     * @brief 是否正在骑乘
     */
    bool isRiding = false;

    /**
     * @brief 挥手进度（攻击动画）
     *
     * 范围 [0, 1)，0 表示无挥手，>0 表示正在挥手
     */
    f32 swingProgress = 0.0f;

    // ========== 方法 ==========

    /**
     * @brief 计算状态哈希
     *
     * 基于动画参数计算哈希值，用于快速检测状态变化。
     */
    void computeHash();

    /**
     * @brief 比较两个动画上下文是否相等
     *
     * 基于状态哈希进行比较
     */
    bool operator==(const AnimationContext& other) const { return stateHash == other.stateHash; }

    /**
     * @brief 比较两个动画上下文是否不相等
     */
    bool operator!=(const AnimationContext& other) const { return stateHash != other.stateHash; }

    /**
     * @brief 检查动画状态是否有显著变化
     *
     * @param other 要比较的上下文
     * @param threshold 角度变化阈值（默认 0.01 弧度）
     * @return 是否有显著变化
     */
    [[nodiscard]] bool hasSignificantChange(const AnimationContext& other, f64 threshold = 0.01) const;
};

} // namespace mc::client::renderer::entity::core
