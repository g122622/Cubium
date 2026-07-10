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

#pragma once

#include "common/core/Types.hpp"
#include <array>

namespace mc::client::renderer::entity::core {

/**
 * @brief 动画上下文
 *
 * 存储实体的动画状态，用于传递给渲染器和网格更新器。
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
     * 实体移动时的腿部摆动周期
     */
    f64 limbSwing = 0.0;

    /**
     * @brief 步态动画强度
     *
     * 实体移动速度的表示，范围 [0, 1]
     */
    f64 limbSwingAmount = 0.0;

    /**
     * @brief 年龄（Tick）
     *
     * 实体存活时间，用于空闲动画（如呼吸、摆动）
     */
    f64 ageInTicks = 0.0;

    /**
     * @brief 头部偏航角（相对于身体）
     *
     * 范围 [-180, 180] 度
     */
    f64 netHeadYaw = 0.0;

    /**
     * @brief 头部俯仰角
     *
     * 范围 [-90, 90] 度
     */
    f64 headPitch = 0.0;

    /**
     * @brief 缩放因子
     *
     * 实体模型的缩放比例，幼体实体使用更小的缩放
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
     * @brief 游泳动画渐变量（已插值）
     *
     * 对应 MC 1.21.11 HumanoidRenderState.swimAmount（由 LivingEntity.getSwimAmount(partialTick) 填充）。
     * 范围 [0, 1]，0 表示完全直立，1 表示完全游泳姿态。
     * DrownedModel::setAngles 读取此值驱动手臂/腿部的游泳覆盖动画。
     *
     * 数据流：服务端 DrownedEntity::updateSwimming 设置 Swimming 标志位
     * → EntityTracker 广播 EntityMetadataPacket
     * → ClientEntity::syncMetadataFromDataManager 调用 setSwimming
     * → ClientEntity::tick 推进 m_swimAmount/m_swimAmountO（±0.09/tick）
     * → EntityRendererManager::updateAnimationContext 插值写入此字段
     * → DrownedModel::setAngles 读取并应用游泳手臂/腿部覆盖。
     */
    f32 swimAmount = 0.0f;

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

    // ========== 实体特定动画参数 ==========

    /**
     * @brief 北极熊站立动画进度
     *
     * 范围 [0, 1]，0 表示四足站立，1 表示后腿站立
     */
    f32 standingProgress = 0.0f;

    /**
     * @brief 河豚膨胀状态
     *
     * 0 = 未膨胀, 1 = 半膨胀, 2 = 完全膨胀
     * 参考 MC 1.16.5 PufferfishEntity.PUFF_STATE
     */
    i32 puffState = 0;

    /**
     * @brief 吃草动画计时器
     *
     * 羊等实体的低头吃草动画计时器。
     * 收到 EatBlock(10) 状态包时设为 40，每tick递减，0表示动画结束。
     * 参考 MC 1.16.5 Sheep.eatAnimationTick。
     *
     * 在 EntityRendererManager 中传递给 SheepModel::setEatAnimationTimer()，
     * 由 SheepModel 根据 timer 值计算头部低头/摆动动画。
     */
    i32 eatAnimationTimer = 0;

    /**
     * @brief 撞飞攻击动画剩余 tick
     *
     * 疣猪兽/僵尸疣兽甩头攻击动画计时器。
     * 收到 HoglinAttack(4) 状态包时设为 10，每tick递减，0表示动画结束。
     * 对应 MC 原版 HoglinBase.getAttackAnimationRemainingTicks()。
     *
     * 在 EntityRendererManager 中传递给 BoarModel::setAttackAnimationTicks()，
     * 由 BoarModel 根据计时器值计算头部 X 旋转插值（甩头动画）。
     */
    i32 attackAnimationTicks = 0;

    /**
     * @brief 狼甩水动画进度（插值后）
     *
     * 对应 MC 1.21.11 Wolf.shakeAnim（已插值）。
     * 范围 [0, 2]，由 EntityRendererManager 从 ClientEntity 读取并插值。
     * WolfModel 通过 _getBodyRollAngle(offset) 使用此值计算各部件 Z 旋转。
     *
     * 数据流：服务端 WolfEntity.tick() 推进 shakeAnim
     * → broadcastEntityStatus(ShakeOffWater=8) → ClientEntity.setWolfShaking(true)
     * → ClientEntity::tick() 推进 wolfShakeAnim → AnimationContext.wolfShakeAnim
     * → WolfModel::setAnimState → _getBodyRollAngle
     */
    f32 wolfShakeAnim = 0.0f;

    /**
     * @brief 狼乞求食物头部角度（插值后）
     *
     * 对应 MC 1.21.11 Wolf.interestedAngle（已插值）。
     * WolfModel 将此值加到头部 Z 旋转上。
     *
     * 数据流：服务端 WolfEntity::setInterested 写入 DATA_INTERESTED_PARAM
     * → EntityTracker 广播 EntityMetadataPacket
     * → ClientEntity::syncMetadataFromDataManager 调用 setWolfIsInterested
     * → ClientEntity::tick 推进 wolfInterestedAngle 向 1.0/0.0 插值
     * → EntityRendererManager::updateAnimationContext 写入此字段
     * → WolfModel::setAnimState 读取并应用到头部 Z 旋转。
     */
    f32 wolfInterestedAngle = 0.0f;

    /**
     * @brief 狼湿润着色值
     *
     * 对应 MC 1.21.11 Wolf.getWetShade()。范围 [0.75, 1.0]。
     * 1.0 表示完全干燥，0.75 表示刚接触水（最暗）。
     * WolfRenderer 用于设置模型 tint。
     */
    f32 wolfWetShade = 1.0f;

    /**
     * @brief 是否处于愤怒状态（狼专用）
     *
     * 对应 MC 1.21.11 Wolf.isAngry()（由 NeutralMob 默认方法计算）。
     * 由 EntityRendererManager 从 ClientEntity::wolfIsAngry() 读取填充。
     *
     * 数据流：服务端 WolfEntity::setAngry/setAngerTime 写入 DATA_ANGER_TIME_PARAM
     * → EntityTracker 广播 EntityMetadataPacket
     * → ClientEntity::syncMetadataFromDataManager 调用 setWolfIsAngry
     * → EntityRendererManager::updateAnimationContext 写入此字段
     * → WolfModel::setAnimState 读取以决定尾巴 Y 旋转（愤怒时锁 0）。
     *
     * 注意：此字段同时影响 wolf 模型分支中的 tailAngle 计算（愤怒时 1.539f ≈ 88°）。
     */
    bool isAngry = false;

    /**
     * @brief 凋灵侧头偏航角（度，已减去身体偏航角，插值后）
     *
     * 对应 MC 1.21.11 WitherRenderState.yHeadRots[2] - bodyRot。
     * index 0 = 左头，index 1 = 右头。
     *
     * 数据流：服务端 WitherEntity::aiStep() 调用 _updateSideHeadRotations()
     * 用 rotlerp 计算 m_headYRot[2]
     * → （客户端不运行 aiStep，由 ClientEntity::tickWitherSideHeads 本地镜像计算）
     * → ClientEntity 存储 witherSideHeadYaw/Pitch + prev 变体
     * → EntityRendererManager 在 wither 分支插值并写入此字段
     * → WitherModel::setSideHeadRotations 应用到 m_heads[1]/m_heads[2]
     */
    std::array<f32, 2> witherSideHeadYaw = {0.0f, 0.0f};

    /**
     * @brief 凋灵侧头俯仰角（度，插值后）
     *
     * 对应 MC 1.21.11 WitherRenderState.xHeadRots[2]。
     * index 0 = 左头，index 1 = 右头。
     */
    std::array<f32, 2> witherSideHeadPitch = {0.0f, 0.0f};

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
     * @param threshold 角度变化阈值
     * @return 是否有显著变化
     */
    [[nodiscard]] bool hasSignificantChange(const AnimationContext& other, f64 threshold) const;
};

} // namespace mc::client::renderer::entity::core
