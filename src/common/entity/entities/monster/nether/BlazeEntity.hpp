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
#include "../../../interfaces/IRangedAttackMob.hpp"
#include "../MonsterEntity.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class IWorld;
class DamageSource;

/**
 * @brief 烈焰人实体
 *
 * 生活在下界的火焰怪物。
 *
 * 特性：
 * - 飞行：可以飞行并悬浮
 * - 火球：发射小火球
 * - 火焰免疫：免疫火焰伤害
 * - 弱水：接触水会受伤
 * - 燃烧：攻击时全身冒火
 *
 * 参考 MC 1.16.5 BlazeEntity
 */
class BlazeEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    BlazeEntity(EntityId id);
    ~BlazeEntity() override = default;

    // 禁止拷贝
    BlazeEntity(const BlazeEntity&) = delete;
    BlazeEntity& operator=(const BlazeEntity&) = delete;

    // 允许移动
    BlazeEntity(BlazeEntity&&) = default;
    BlazeEntity& operator=(BlazeEntity&&) = default;

    /**
     * @brief 创建烈焰人实体
     * @param world 世界实例
     * @return 新的烈焰人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * MC 1.16.5: entity.blaze.ambient
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * MC 1.16.5: entity.blaze.hurt
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * MC 1.16.5: entity.blaze.death
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 火球攻击 ==========

    /**
     * @brief 是否正在发射火球（全身冒火）
     * MC 1.16.5: isCharged() / isBurning()
     */
    [[nodiscard]] bool isCharged() const { return m_charging; }

    /**
     * @brief 设置发射状态
     */
    void setCharging(bool charging) { m_charging = charging; }

    /**
     * @brief 获取火球数量
     */
    [[nodiscard]] i32 getFireballCount() const { return m_fireballCount; }

    /**
     * @brief 设置火球数量
     */
    void setFireballCount(i32 count) { m_fireballCount = count; }

    // ========== 悬浮高度 ==========

    /**
     * @brief 获取高度偏移
     * MC 1.16.5: 用于悬浮行为
     */
    [[nodiscard]] f32 heightOffset() const { return m_heightOffset; }

    // ========== 水敏感性 ==========

    /**
     * @brief 烈焰人对水敏感
     * MC 1.16.5: isWaterSensitive() -> true
     */
    [[nodiscard]] bool isWaterSensitive() const { return true; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 烈焰人不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 亮度 ==========

    /**
     * @brief 获取亮度
     * MC 1.16.5: getBrightness() -> 1.0F
     */
    [[nodiscard]] f32 getBrightness() const { return 1.0f; }

    // ========== 摔落伤害 ==========

    /**
     * @brief 烈焰人免疫摔落伤害
     * MC 1.16.5: onLivingFall() -> false
     */
    [[nodiscard]] bool canTakeFallDamage() const { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.0f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.8f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 更新AI任务
     */
    void updateAITasks() override;

private:
    // 攻击状态
    bool m_charging = false;
    i32 m_fireballCount = 0;
    i32 m_attackStep = 0; // MC 1.16.5: attackStep
    i32 m_attackTime = 0; // MC 1.16.5: attackTime

    // 悬浮高度
    f32 m_heightOffset = 0.5f;        // MC 1.16.5: heightOffset
    i32 m_heightOffsetUpdateTime = 0; // MC 1.16.5: heightOffsetUpdateTime

    // MC 1.16.5 常量
    static constexpr f32 FIREBALL_DAMAGE = 5.0f;
    static constexpr i32 ATTACK_CHARGE_TIME = 60; // 充能时间
    static constexpr i32 ATTACK_COOLDOWN = 100;   // 攻击冷却
    static constexpr i32 FIREBALL_INTERVAL = 6;   // 火球间隔
    static constexpr i32 MAX_FIREBALLS = 3;       // 最多连发3个火球
};

} // namespace mc
