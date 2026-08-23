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

#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../MonsterEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declaration
class DamageSource;

/**
 * @brief 守卫者实体
 *
 * 生活在海底神殿的海洋怪物。
 *
 * 特性：
 * - 激光攻击：向玩家发射激光
 * - 尖刺：被攻击时会伤害攻击者
 * - 游泳：在水中游泳
 * - 陆地挣扎：在陆地上会挣扎
 */
class GuardianEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    GuardianEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~GuardianEntity() override = default;

    // 水生生物归类（对齐 Java Guardian.getMobType()==WATER）。基类默认 Undefined，未覆写会导致
    // 穿刺(Impaling)附魔无加成（ImpalingEnchantment 对 Water 属性额外伤害）等失效。
    // 子类 ElderGuardianEntity 继承此覆写自动归类 Water。
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Water; }

    // 禁止拷贝
    GuardianEntity(const GuardianEntity&) = delete;
    GuardianEntity& operator=(const GuardianEntity&) = delete;

    // 允许移动
    GuardianEntity(GuardianEntity&&) = delete;
    GuardianEntity& operator=(GuardianEntity&&) = delete;

    /**
     * @brief 创建守卫者实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 激光攻击 ==========

    /**
     * @brief 是否正在发射激光
     */
    [[nodiscard]] bool isLaserCharging() const { return m_laserCharging; }

    /**
     * @brief 设置激光充能状态
     */
    void setLaserCharging(bool charging) { m_laserCharging = charging; }

    /**
     * @brief 获取激光充能时间
     */
    [[nodiscard]] i32 getLaserChargeTime() const { return m_laserChargeTime; }

    /**
     * @brief 设置激光充能时间
     */
    void setLaserChargeTime(i32 time) { m_laserChargeTime = time; }

    /**
     * @brief 获取目标实体ID
     */
    [[nodiscard]] EntityInstanceId getTargetEntity() const { return m_targetEntity; }

    /**
     * @brief 设置目标实体
     */
    void setTargetEntity(EntityInstanceId id) { m_targetEntity = id; }

    // ========== 尖刺 ==========

    /**
     * @brief 尖刺是否伸出
     */
    [[nodiscard]] bool areSpikesRetracted() const { return m_spikesRetracted; }

    /**
     * @brief 设置尖刺状态
     */
    void setSpikesRetracted(bool retracted) { m_spikesRetracted = retracted; }

    // ========== 水中状态 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    // ========== 寻路权重 ==========

    /**
     * @brief 获取路径权重
     *
     * 守卫者偏好水中位置：在水中返回 10.0f + lightCost，否则返回父类值。
     * 对应 MC Guardian.getWalkTargetValue。
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 阳光燃烧 ==========

    /**
     * @brief 守卫者不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.5f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 陆地和水中使用不同音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 播放激光攻击音效
     */
    void playLaserSound();

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 受伤入口（对齐 vanilla Guardian.hurtServer:314-326）
     *
     * 守卫者受伤时，若非移动状态且伤害源不属于 AVOIDS_GUARDIAN_THORNS / THORNS，且直接来源是
     * LivingEntity，则对直接攻击者造成 2.0 荆棘反伤（damageSources().thorns(this)）。
     * 荆棘伤害 type=Thorns 会被 !source.is(THORNS) 门控挡住，反伤链不递归。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 是否正在移动（对齐 vanilla Guardian.isMoving:101-103）
     *
     * vanilla 读 DATA_ID_MOVING 同步参数，由 GuardianMoveControl.tick 在 MoveControl 为
     * MOVE_TO 且导航未完成时设 true（Guardian.java:451/482-485）。Cubium 未实现 GuardianMoveControl，
     * 此处用 moveController()->isUpdating() && !navigator()->noPath() 近似（对齐 vanilla 判定
     * operation==MOVE_TO && !navigation.isDone()）。
     * TODO: 完整对齐需实现 GuardianMoveControl + DATA_ID_MOVING 同步参数体系（当前用 moveControl
     *       状态近似，静态场景（无 AI 驱动移动）下返 false 与 vanilla 一致）。
     */
    [[nodiscard]] bool isMoving() const;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 激光攻击
    bool m_laserCharging = false;
    i32 m_laserChargeTime = 0;
    EntityInstanceId m_targetEntity = 0;

    // 尖刺
    bool m_spikesRetracted = false;
    i32 m_spikeTimer = 0;

    // 常量
    static constexpr i32 LASER_CHARGE_DURATION = 60; // 3秒充能
    static constexpr f32 LASER_DAMAGE = 4.0f;
    static constexpr f32 SPIKE_DAMAGE = 2.0f;
};

} // namespace mc
