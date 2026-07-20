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

#include "../../../resource/ResourceLocation.hpp"
#include "../../core/DataParameter.hpp"
#include "../../core/MobEntity.hpp"
#include "../../interfaces/IRangedAttackMob.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class DamageSource;
class LivingEntity;

namespace entity {

// 前向声明
class WitherEntity;

namespace ai::controller {
class FlyingMovementController;
}

/**
 * @brief 凋灵随机飞行目标
 *
 * 当凋灵没有其他高优先级目标时，随机选择一个飞行目标位置。
 * 由于 WitherEntity 继承自 MobEntity 而非 CreatureEntity，
 * 不能直接使用 WaterAvoidingRandomFlyingGoal（它要求 CreatureEntity*），
 * 因此创建了此专用目标类。
 */
class WitherRandomFlyGoal : public ai::Goal {
public:
    explicit WitherRandomFlyGoal(WitherEntity* wither);

    bool shouldExecute() override;
    bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
    std::string getTypeName() const override { return "WitherRandomFlyGoal"; }

private:
    /**
     * @brief 生成飞行目标位置
     *
     * 在凋灵前方 PI/2 弧度锥形范围内随机选择一个避水避岩浆的空气位置。
     */
    bool _generateFlightTarget();

    WitherEntity* m_wither;
    f64 m_speed = 1.0;
    f32 m_targetX = 0.0f;
    f32 m_targetY = 0.0f;
    f32 m_targetZ = 0.0f;
    bool m_hasTarget = false;
};

/**
 * @brief 凋灵无敌阶段目标
 *
 * 在凋灵的无敌阶段（生成后220 ticks）阻止所有行动。
 * 阻止移动、跳跃和看向。
 */
class WitherDoNothingGoal : public ai::Goal {
public:
    explicit WitherDoNothingGoal(WitherEntity* wither);

    ~WitherDoNothingGoal() noexcept override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "WitherDoNothingGoal"; }

private:
    WitherEntity* m_wither;
};

/**
 * @brief 凋灵Boss实体
 *
 * 地狱Boss，具有三个头和多种攻击模式。
 *
 * 特性：
 * - 三头：主头和两个侧头独立追踪目标
 * - 无敌阶段：生成后220 ticks无敌
 * - 充能阶段：生命值低于一半时充能发射蓝色凋灵之首
 * - 方块破坏：能够破坏周围方块
 * - Boss条：显示Boss生命值（紫色）
 */
class WitherEntity : public MobEntity, public IRangedAttackMob {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit WitherEntity(EntityInstanceId id);

    ~WitherEntity() noexcept override = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.9f; }
    [[nodiscard]] f32 height() const override { return 3.5f; }
    [[nodiscard]] f32 eyeHeight() const override { return 2.0f; }

    void tick() override;

    // ========== LivingEntity 接口重写 ==========

    /**
     * @brief AI步进，在LivingEntity::tick()中调用
     *
     * 重写以在控制器更新之前执行飞行追踪行为，
     * 使 FlyingMovementController 的旋转限制能正确生效。
     */
    void aiStep() override;

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 凋灵免疫火焰、溺水、凋零伤害
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const override;

    /**
     * @brief 受伤处理
     * 凋灵受伤后会触发方块破坏
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 是否为亡灵生物
     */
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }

    /**
     * @brief 是否为Boss
     */
    [[nodiscard]] bool isNonBoss() const override { return false; }

    /**
     * @brief 摔落伤害免疫
     */
    [[nodiscard]] bool onLivingFall(f32 distance, f32 damageMultiplier) override;

    /**
     * @brief 凋灵永不自然消失
     *
     * 返回 true 使 DespawnManager 跳过距离判定并重置 idleTime。
     */
    [[nodiscard]] bool preventDespawn() const override { return true; }

    /**
     * @brief 和平难度下消失
     *
     * 返回 true 使 DespawnManager 在和平难度下移除此实体。
     */
    [[nodiscard]] bool isDespawnPeaceful() const override { return true; }

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 40; }
    [[nodiscard]] bool canRangedAttack() const override;

    // ========== Boss 功能 ==========

    /**
     * @brief 获取Boss名称
     */
    [[nodiscard]] std::string getBossName() const;

    /**
     * @brief 是否显示生命条
     */
    [[nodiscard]] bool shouldDisplayHealthBar() const { return true; }

    // ========== 凋灵特有 ==========

    /**
     * @brief 获取无敌时间
     */
    [[nodiscard]] i32 getInvulTime() const { return m_dataManager.get<i32>(INVULNERABILITY_TIME); }

    /**
     * @brief 设置无敌时间
     */
    void setInvulTime(i32 time) { m_dataManager.set(INVULNERABILITY_TIME, time); }

    /**
     * @brief 是否处于无敌阶段
     */
    [[nodiscard]] bool isInvulnerablePhase() const { return getInvulTime() > 0; }

    /**
     * @brief 是否充能（生命值低于一半）
     */
    [[nodiscard]] bool isCharged() const { return health() <= maxHealth() / 2.0f; }

    /**
     * @brief 获取指定头的目标实体ID
     * @param head 头索引 (0=主头, 1=左头, 2=右头)
     */
    [[nodiscard]] i32 getWatchedTargetId(i32 head) const;

    /**
     * @brief 更新指定头的目标
     * @param head 头索引
     * @param targetId 目标实体ID
     */
    void updateWatchedTargetId(i32 head, i32 targetId);

    /**
     * @brief 发射凋灵之首到指定位置
     * @param head 头索引 (0=主头, 1=左头, 2=右头)
     * @param targetX 目标X坐标
     * @param targetY 目标Y坐标
     * @param targetZ 目标Z坐标
     * @param isBlue 是否为蓝色凋灵之首
     */
    void launchWitherSkullToPosition(i32 head, f64 targetX, f64 targetY, f64 targetZ, bool isBlue);

    /**
     * @brief 发射凋灵之首到实体目标
     * @param head 头索引 (0=主头, 1=左头, 2=右头)
     * @param target 目标实体
     */
    void launchWitherSkullToEntity(i32 head, LivingEntity* target);

    /**
     * @brief 开始生成序列
     */
    void ignite();

    // ========== 侧头朝向（用于渲染） ==========

    /**
     * @brief 获取指定侧头的当前俯仰角（度）
     * @param index 侧头索引 (0=左头, 1=右头)
     * @return 当前 tick 的俯仰角
     *
     * 对应 MC 1.21.11 WitherBoss.xRotHeads[index]。
     * 在 aiStep() 中由 rotlerp 逐步逼近目标俯仰角。
     */
    [[nodiscard]] f32 sideHeadPitch(i32 index) const { return m_headXRot[index]; }

    /**
     * @brief 获取指定侧头的上一 tick 俯仰角（度，用于插值）
     * @param index 侧头索引 (0=左头, 1=右头)
     */
    [[nodiscard]] f32 prevSideHeadPitch(i32 index) const { return m_prevHeadXRot[index]; }

    /**
     * @brief 获取指定侧头的当前偏航角（度）
     * @param index 侧头索引 (0=左头, 1=右头)
     *
     * 对应 MC 1.21.11 WitherBoss.yRotHeads[index]。
     * 在 aiStep() 中由 rotlerp 逐步逼近目标偏航角（或身体偏航角，当无目标时）。
     */
    [[nodiscard]] f32 sideHeadYaw(i32 index) const { return m_headYRot[index]; }

    /**
     * @brief 获取指定侧头的上一 tick 偏航角（度，用于插值）
     * @param index 侧头索引 (0=左头, 1=右头)
     */
    [[nodiscard]] f32 prevSideHeadYaw(i32 index) const { return m_prevHeadYRot[index]; }

    // ========== DataParameter ID 访问器（供 ClientEntity 读取元数据） ==========

    /**
     * @brief 获取主头目标 DataParameter ID
     * ClientEntity::syncMetadataFromDataManager 通过此 ID 读取 HEAD_TARGET_1。
     */
    [[nodiscard]] static u16 getHeadTarget1ParamId() { return HEAD_TARGET_1.id(); }

    /**
     * @brief 获取左头目标 DataParameter ID
     * ClientEntity::syncMetadataFromDataManager 通过此 ID 读取 HEAD_TARGET_2。
     */
    [[nodiscard]] static u16 getHeadTarget2ParamId() { return HEAD_TARGET_2.id(); }

    /**
     * @brief 获取右头目标 DataParameter ID
     * ClientEntity::syncMetadataFromDataManager 通过此 ID 读取 HEAD_TARGET_3。
     */
    [[nodiscard]] static u16 getHeadTarget3ParamId() { return HEAD_TARGET_3.id(); }

    /**
     * @brief 获取无敌时间 DataParameter ID
     */
    [[nodiscard]] static u16 getInvulTimeParamId() { return INVULNERABILITY_TIME.id(); }

protected:
    void registerData() override;
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 死亡时掉落物品
     */
    void die(DamageSource& source) override;

    /**
     * @brief 药水效果免疫检查
     */
    [[nodiscard]] bool isPotionApplicable(const entity::effect::EffectInstance& effect) const override;

private:
    // ========== 数据参数 ==========
    // 头部追踪目标实体ID
    static entity::DataParameter<i32> HEAD_TARGET_1; // 主头目标
    static entity::DataParameter<i32> HEAD_TARGET_2; // 左头目标
    static entity::DataParameter<i32> HEAD_TARGET_3; // 右头目标
    // 无敌时间
    static entity::DataParameter<i32> INVULNERABILITY_TIME;

    // 头部旋转角度（用于渲染）
    f32 m_headXRot[2] = {0.0f, 0.0f};     // 侧头俯仰角
    f32 m_headYRot[2] = {0.0f, 0.0f};     // 侧头偏航角
    f32 m_prevHeadXRot[2] = {0.0f, 0.0f}; // 上一帧俯仰角
    f32 m_prevHeadYRot[2] = {0.0f, 0.0f}; // 上一帧偏航角

    // 头部攻击相关
    i32 m_nextHeadUpdate[2] = {0, 0};  // 下次攻击更新tick
    i32 m_idleHeadUpdates[2] = {0, 0}; // 空闲头部更新计数
    i32 m_blockBreakCounter = 0;       // 方块破坏计数器

    // 常量
    static constexpr i32 INVULNERABILITY_TIME_CONST = 220; // 生成无敌时间 (11秒)
    static constexpr i32 BLOCK_BREAK_COOLDOWN = 20;        // 方块破坏冷却
    static constexpr f32 HEAD_TRACK_RANGE = 20.0f;         // 头部追踪范围
    static constexpr i32 ATTACK_COOLDOWN = 40;             // 攻击冷却 (2秒)

    /**
     * @brief 更新AI任务
     *
     * 包含无敌阶段处理、头部目标追踪、方块破坏逻辑和充能状态回血。
     */
    void _updateAITasks();

    /**
     * @brief 更新飞行追踪行为
     *
     * 当主头有目标时，凋灵会主动飞向目标：
     * - Y轴速度保留60%（阻尼）
     * - 低于目标时附加上升推力
     * - 水平距离大于3格时附加追踪推力
     * - 有水平速度时自动面向运动方向
     */
    void _updateFlightBehavior();

    /**
     * @brief 更新头部目标追踪
     */
    void _updateHeadTargets();

    /**
     * @brief 更新侧头朝向角度
     *
     * 对应 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 循环：
     * - 若侧头有追踪目标，计算目标相对头部位置的 yaw/pitch，
     *   用 rotlerp 逐步逼近（yaw 最大 10°/tick，pitch 最大 40°/tick）。
     * - 若无目标，yaw 用 rotlerp 逐步逼近身体偏航角（renderYawOffset）。
     *
     * 在 aiStep() 中于 LivingEntity::aiStep() 之后调用，
     * 以确保 renderYawOffset 已由 LookController 更新。
     */
    void _updateSideHeadRotations();

    /**
     * @brief 角度逐步逼近（rotlerp）
     *
     * 对应 MC 1.21.11 WitherBoss.rotlerp(current, target, maxStep)：
     *   diff = wrapDegrees(target - current)
     *   diff = clamp(diff, -maxStep, maxStep)
     *   return current + diff
     *
     * @param current 当前角度（度）
     * @param target 目标角度（度）
     * @param maxStep 单 tick 最大变化量（度）
     * @return 逼近后的角度（度，不包装到 [-180,180]）
     */
    [[nodiscard]] static f32 _rotLerp(f32 current, f32 target, f32 maxStep);

    /**
     * @brief 获取头的X坐标
     */
    [[nodiscard]] f32 _getHeadX(i32 head) const;

    /**
     * @brief 获取头的Y坐标
     */
    [[nodiscard]] f32 _getHeadY(i32 head) const;

    /**
     * @brief 获取头的Z坐标
     */
    [[nodiscard]] f32 _getHeadZ(i32 head) const;

    /**
     * @brief 破坏周围方块
     */
    void _breakNearbyBlocks();

    /**
     * @brief 在生成时创建爆炸
     */
    void _explodeOnSpawn();

    /**
     * @brief 生成粒子效果
     */
    void _spawnParticles();
};

} // namespace entity
} // namespace mc
