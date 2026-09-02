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
 * copies of substantial portions of the Software.
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
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/projectile/ProjectileDeflection.hpp"
#include "common/entity/utils/AnimationState.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

namespace entity::ai::goal {
class BreezeShootGoal;
class BreezeLongJumpGoal;
class BreezeSlideGoal;
class BreezeShootWhenStuckGoal;
} // namespace entity::ai::goal

namespace entity {
class ProjectileEntity;
} // namespace entity

namespace test {
class BreezeEntityTestAccessor; // 测试访问器，声明为 friend 以访问 protected 成员
class BreezeShootTestAccessor;  // shootWindCharge 测试访问器，声明为 friend 以访问 private 成员
} // namespace test

/**
 * @brief 旋风人实体
 *
 * MC 1.21 新增的敌对生物，在试炼密室中生成。
 * 能够投掷风弹攻击目标，具有独特的滑行和长跳移动能力。
 * 可以弹开除风弹外的投射物。
 *
 * 属性：
 * - 生命值：30
 * - 移动速度：0.6
 * - 跟随距离：24
 * - 攻击伤害：3
 * - 击退抗性：0.0
 *
 * AI行为：
 * - Shoot：向目标投掷风弹，范围4-24格
 * - LongJump：长跳移动，跳跃距离3-5格
 * - Slide：在地面上滑行移动
 * - ShootWhenStuck：卡住时紧急射击
 *
 * 动画状态机（基于 Pose 驱动）：
 * - idle：默认空闲动画，每 tick 持续触发
 * - slide：滑行 Pose 时启动
 * - slideBack：从滑行恢复站立时短暂触发的回弹动画
 * - longJump：长跳中 Pose 时启动
 * - shoot：射击 Pose 时启动
 * - inhale：吸气蓄力 Pose 时启动
 *
 * Pose 切换由各 AI Goal 调用 setPose() 触发，tick() 中根据当前 Pose
 * 发射地面/轨迹粒子并播放呼啸音效。AnimationState 字段维护动画时序，
 * 用于 slide→slideBack 等状态机转换判定。
 *
 * TODO(client_renderer): 旋风人客户端模型与渲染器尚未实现。
 * 当前服务端 BreezeEntity 维护了 m_idleAnim/m_slideAnim/m_slideBackAnim/
 * m_longJumpAnim/m_shootAnim/m_inhaleAnim 六个 AnimationState 字段，
 * 这些字段镜像 MC 1.21.11 Breeze.java 的设计，但 Cubium 的客户端
 * ClientEntity 是与服务端实体分离的独立类，目前尚无 BreezeModel/
 * BreezeRenderer 读取这些动画状态。未来实现客户端渲染器时，需要：
 * 1. 在 ClientEntity 中根据同步的 Pose 启动对应的 AnimationState
 *    （参考 MC Breeze.onSyncedDataUpdated 与 resetAnimations）
 * 2. BreezeModel 根据各 AnimationState 的 startTick 计算动画进度
 * 3. 客户端渲染器调用 idleAnimation()/slideAnimation()/等访问器
 *    读取服务端实体（或 ClientEntity 自身维护）的动画状态
 * 在客户端渲染器实现之前，这些 AnimationState 字段暂时没有渲染侧
 * 消费者，但服务端会持续维护它们，确保未来渲染器实现后可直接接入。
 *
 * 掉落：
 * - 狂风杖 1-2（仅被玩家击杀时掉落，受抢夺附魔影响，每级额外+1~2）
 * - 经验值 10
 *
 * 命名空间ID: minecraft:breeze
 */
class BreezeEntity final : public MonsterEntity {
public:
    // AI 目标类需要访问 protected/private 成员
    friend class entity::ai::goal::BreezeShootGoal;
    friend class entity::ai::goal::BreezeLongJumpGoal;
    friend class entity::ai::goal::BreezeSlideGoal;
    friend class entity::ai::goal::BreezeShootWhenStuckGoal;
    // 测试访问器，用于单元测试访问 protected 成员（无需修改生产代码可见性）
    friend class test::BreezeEntityTestAccessor;
    // shootWindCharge 单元测试访问器（访问 private shootWindCharge）
    friend class test::BreezeShootTestAccessor;
    /// 基础生命值
    static constexpr f32 MAX_HEALTH = 30.0f;

    /// 基础移动速度
    static constexpr f32 MOVEMENT_SPEED = 0.6f;

    /// 跟随距离
    static constexpr f32 FOLLOW_RANGE = 24.0f;

    /// 基础攻击伤害
    static constexpr f32 ATTACK_DAMAGE = 3.0f;

    /// 风弹射击最小距离
    static constexpr f32 SHOOT_MIN_RANGE = 4.0f;

    /// 风弹射击最大距离
    static constexpr f32 SHOOT_MAX_RANGE = 24.0f;

    /// 长跳最小距离
    static constexpr f32 LONG_JUMP_MIN_RANGE = 3.0f;

    /// 长跳最大距离
    static constexpr f32 LONG_JUMP_MAX_RANGE = 5.0f;

    /// 滑行速度范围
    static constexpr f32 SLIDE_SPEED_MIN = 0.3f;
    static constexpr f32 SLIDE_SPEED_MAX = 0.6f;

    /**
     * @brief 构造旋风人
     * @param id 实体ID
     */
    explicit BreezeEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~BreezeEntity() override = default;

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 实体尺寸 ==========

    f32 width() const override { return 0.6f; }
    f32 height() const override { return 1.77f; }
    f32 eyeHeight() const override { return 1.52f; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 掉落物 ==========

    /**
     * @brief 重写死亡掉落逻辑
     *
     * Breeze 死亡时掉落狂风杖：
     * - 仅在被玩家击杀时掉落（killed_by_player 条件）
     * - 基础数量 1-2 个
     * - 每级抢夺附魔额外增加 1-2 个
     * - 对应战利品表: minecraft:entities/breeze
     *
     * 当前项目尚未实现通用的实体战利品表掉落流程，
     * 因此在此直接硬编码掉落逻辑，待通用流程实现后迁移到战利品表。
     */
    void die(DamageSource& source) override;

    /**
     * @brief 检查是否可以攻击指定类型的实体
     *
     * Breeze.canAttackType() 仅允许攻击玩家和铁傀儡。
     * 旋风人采用白名单模式，只攻击这两种实体类型。
     */
    [[nodiscard]] bool canAttackType(const entity::EntityType& type) const override;

    /**
     * @brief 重写 Entity::isInvulnerableTo
     *
     * 对齐 MC Java 1.21.11 Breeze.isInvulnerableTo（Breeze.java:267-269）：
     *   return p_312691_.getEntity() instanceof Breeze || super.isInvulnerableTo(p_376278_, p_312691_);
     * 当伤害来源实体（getEntity()，对弹射物是射击者）是旋风人时，目标旋风人免疫该伤害。
     * 这保障旋风人之间的风弹互不伤害——旋风人 A 发射的风弹命中旋风人 B 时，伤害源的
     * getEntity() 是 A（Breeze），B 的 isInvulnerableTo 返回 true，不受伤害。
     *
     * 此前 Cubium 缺此 override，旋风人风弹会正常伤害其他旋风人，与 vanilla 直接冲突。
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const override;

    /**
     * @brief 获取此旋风人对指定弹射物的偏转类型
     *
     * 重写 Entity::deflection()。
     * 旋风人偏转除风弹外的所有投射物，并播放偏转音效。
     * 风弹（包括旋风人风弹和玩家风弹）不被偏转。
     */
    [[nodiscard]] ProjectileDeflection deflection(const entity::ProjectileEntity& projectile) const override;

    // ========== 动画状态访问器 ==========

    /**
     * @brief 空闲动画状态
     */
    [[nodiscard]] const entity::AnimationState& idleAnimation() const noexcept { return m_idleAnim; }

    /**
     * @brief 滑行动画状态
     */
    [[nodiscard]] const entity::AnimationState& slideAnimation() const noexcept { return m_slideAnim; }

    /**
     * @brief 滑行回弹动画状态（从滑行姿态恢复站立时短暂触发）
     */
    [[nodiscard]] const entity::AnimationState& slideBackAnimation() const noexcept { return m_slideBackAnim; }

    /**
     * @brief 长跳动画状态
     */
    [[nodiscard]] const entity::AnimationState& longJumpAnimation() const noexcept { return m_longJumpAnim; }

    /**
     * @brief 射击动画状态
     */
    [[nodiscard]] const entity::AnimationState& shootAnimation() const noexcept { return m_shootAnim; }

    /**
     * @brief 吸气蓄力动画状态
     */
    [[nodiscard]] const entity::AnimationState& inhaleAnimation() const noexcept { return m_inhaleAnim; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 地面/空中分别播放不同环境音，对齐原版 Breeze.getAmbientSound：
     * onGround → IDLE_GROUND，否则 → IDLE_AIR。sounds.json 中无 entity.breeze.ambient，
     * 仅有 idle_ground/idle_air，故不能走默认 makeSoundEventId("ambient")。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    // ========== AI 状态查询与设置 ==========

    /**
     * @brief 获取射击冷却剩余时间
     */
    [[nodiscard]] i32 shootCooldown() const { return m_shootCooldown; }

    /**
     * @brief 设置射击冷却时间
     */
    void setShootCooldown(i32 ticks) { m_shootCooldown = ticks; }

    /**
     * @brief 获取长跳冷却剩余时间
     */
    [[nodiscard]] i32 jumpCooldown() const { return m_jumpCooldown; }

    /**
     * @brief 设置长跳冷却时间
     */
    void setJumpCooldown(i32 ticks) { m_jumpCooldown = ticks; }

    /**
     * @brief 是否有射击许可
     *
     * 射击许可由 Slide/LongJump/ShootWhenStuck 设置，
     * 表示旋风人已准备好进行射击攻击。
     */
    [[nodiscard]] bool hasShootPermit() const { return m_shootPermitTicks > 0; }

    /**
     * @brief 设置射击许可
     * @param ticks 许可持续 ticks 数
     */
    void setShootPermit(i32 ticks) { m_shootPermitTicks = ticks; }

    /**
     * @brief 清除射击许可
     */
    void clearShootPermit() { m_shootPermitTicks = 0; }

    /**
     * @brief 是否正在长跳（吸气或跳跃中）
     */
    [[nodiscard]] bool isLongJumping() const { return m_isLongJumping; }

    /**
     * @brief 设置长跳状态
     */
    void setLongJumping(bool jumping) { m_isLongJumping = jumping; }

    /**
     * @brief 是否正在滑行
     */
    [[nodiscard]] bool isSliding() const { return m_sliding; }

    /**
     * @brief 设置滑行状态
     */
    void setSliding(bool sliding) { m_sliding = sliding; }

private:
    /**
     * @brief 投掷风弹
     *
     * 向当前攻击目标投掷一个风弹弹射物。
     */
    void shootWindCharge();

    // ========== 动画状态机 ==========

    /**
     * @brief 推进滑行回弹动画
     *
     * 当旋风人从滑行姿态恢复站立时，启动 slideBack 动画并停止 slide。
     * 由 tick() 在 Pose != Sliding 且 slide 已启动时调用。
     */
    void updateSlideBackAnimation();

    // ========== 粒子与音效 ==========

    /**
     * @brief 在脚下生成地面方块粒子
     *
     * @param count 粒子数量
     *
     * 在旋风人脚下位置生成 count 个 BLOCK 类型粒子，
     * 粒子携带脚下方块的状态用于纹理渲染。
     * 受骑乘状态抑制（被骑乘时不生成）。
     */
    void emitGroundParticles(i32 count);

    /**
     * @brief 重置跳跃轨迹计时器并返回自身
     *
     * 在 Pose 切换到 LONG_JUMPING 之外时调用，确保下次长跳从 0 开始计数。
     */
    void resetJumpTrail() { m_jumpTrailStartedTick = 0; }

    /**
     * @brief 发射长跳轨迹粒子
     *
     * 长跳中每 tick 调用，前 5 tick 在实体前方稍上方位置生成 3 个 BLOCK 粒子。
     * 粒子携带实体当前穿过/脚下的方块状态。
     */
    void emitJumpTrailParticles();

    /**
     * @brief 播放旋风人呼啸音效
     *
     * 随机间隔触发，音量和音调带有随机扰动。
     */
    void playWhirlSound();

    // ========== 动画状态字段 ==========

    // TODO(client_renderer): 以下六个 AnimationState 字段镜像 MC 1.21.11
    // Breeze.java 的设计，由服务端 BreezeEntity::tick() 与各 AI Goal 维护。
    // 当前尚无客户端 BreezeModel/BreezeRenderer 读取这些字段，存在孤岛代码
    // 风险。未来实现客户端渲染器时，需要在 ClientEntity 中根据同步的 Pose
    // 启动对应动画（参考 MC Breeze.onSyncedDataUpdated），并通过 BreezeModel
    // 根据 startTick 计算动画进度。详见类注释中的 TODO(client_renderer)。

    /// 空闲动画（持续触发，每 tick startIfStopped）
    entity::AnimationState m_idleAnim;
    /// 滑行动画（Sliding Pose 时启动）
    entity::AnimationState m_slideAnim;
    /// 滑行回弹动画（从 Sliding 恢复站立时短暂触发）
    entity::AnimationState m_slideBackAnim;
    /// 长跳动画（LongJumping Pose 时启动）
    entity::AnimationState m_longJumpAnim;
    /// 射击动画（Shooting Pose 时启动）
    entity::AnimationState m_shootAnim;
    /// 吸气蓄力动画（Inhaling Pose 时启动）
    entity::AnimationState m_inhaleAnim;

    // ========== 长跳轨迹与音效计时 ==========

    /// 长跳轨迹已发射的 tick 数（>5 后停止发射，0 表示未启动）
    i32 m_jumpTrailStartedTick = 0;

    /// 呼啸音效下次触发的倒计时（0 表示触发并重新随机化）
    i32 m_soundTick = 0;

    // ========== AI 状态字段 ==========

    /// 是否正在滑行
    bool m_sliding = false;

    /// 风弹射击冷却（ticks）
    i32 m_shootCooldown = 0;

    /// 长跳冷却（ticks）
    i32 m_jumpCooldown = 0;

    /// 射击许可计时器（ticks），>0 表示有射击许可
    i32 m_shootPermitTicks = 0;

    /// 是否正在长跳中（吸气或跳跃阶段）
    bool m_isLongJumping = false;

    // ========== 常量 ==========

    /// 滑行姿态每 tick 发射的地面粒子数
    static constexpr i32 SLIDE_PARTICLES_AMOUNT = 20;
    /// 站立/射击/吸气姿态每 tick 发射的地面粒子数（实际为 1 + nextInt(1)）
    static constexpr i32 IDLE_PARTICLES_AMOUNT = 1;
    /// 长跳轨迹粒子每 tick 发射数量
    static constexpr i32 JUMP_TRAIL_PARTICLES_AMOUNT = 3;
    /// 长跳轨迹粒子持续 tick 数
    static constexpr i32 JUMP_TRAIL_DURATION_TICKS = 5;
    /// 呼啸音效随机间隔下限（ticks）
    static constexpr i32 WHIRL_SOUND_FREQUENCY_MIN = 1;
    /// 呼啸音效随机间隔上限（ticks）
    static constexpr i32 WHIRL_SOUND_FREQUENCY_MAX = 80;
};

} // namespace mc
