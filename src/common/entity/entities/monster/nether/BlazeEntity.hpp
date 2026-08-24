/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "common/entity/core/Entity.hpp"
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
 * - 悬浮飞行：通过缓降和有条件的上升推力实现伪悬浮
 * - 火球：发射小火球
 * - 火焰免疫：免疫火焰伤害
 * - 弱水：接触水或雨每tick受伤
 * - 燃烧：攻击时全身冒火
 *
 * 悬浮机制（对齐 MC 1.21.11 Blaze）：
 * - aiStep 中缓降：不在地面且下落时 velocityY *= 0.6
 * - customServerAiStep 中上升推力：目标在上方时施加 (0.3 - velocityY) * 0.3 的Y轴加速
 * - allowedHeightOffset 每100 tick 重新随机，控制悬浮目标高度
 */
class BlazeEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    BlazeEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~BlazeEntity() override = default;

    // 禁止拷贝
    BlazeEntity(const BlazeEntity&) = delete;
    BlazeEntity& operator=(const BlazeEntity&) = delete;

    // 允许移动
    BlazeEntity(BlazeEntity&&) = delete;
    BlazeEntity& operator=(BlazeEntity&&) = delete;

    /**
     * @brief 创建烈焰人实体
     * @param world 世界实例
     * @return 新的烈焰人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 声音 ==========

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

    // ========== IRangedAttackMob 接口实现 ==========

    /**
     * @brief 远程攻击接口的必要实现
     *
     * 烈焰人使用专用的 BlazeFireballAttackGoal 管理火球攻击，
     * 而非通用的 RangedAttackGoal，因此此方法不会被外部调用。
     * 保留此空实现仅为满足 IRangedAttackMob 纯虚接口要求。
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 火球攻击 ==========

    /**
     * @brief 是否正在充能（全身冒火）
     */
    [[nodiscard]] bool isCharged() const { return m_charging; }

    /**
     * @brief 设置充能状态
     */
    void setCharging(bool charging) { m_charging = charging; }

    // ========== 悬浮高度 ==========

    /**
     * @brief 获取允许的高度偏移
     *
     * 控制烈焰人悬浮的目标高度。当攻击目标的眼高超过
     * 烈焰人眼高 + allowedHeightOffset 时，烈焰人会上升。
     * 每 100 tick 通过三角分布重新随机化。
     */
    [[nodiscard]] f32 allowedHeightOffset() const { return m_allowedHeightOffset; }

    // ========== 水敏感性 ==========

    /**
     * @brief 烈焰人对水敏感
     *
     * 对齐 MC 1.21.11 Blaze.isSensitiveToWater() 返回 true。
     * tick() 中水伤害条件为 isWaterSensitive() && isWet()，
     * 对齐 MC 原版 LivingEntity.baseTick() 的逻辑模式。
     * 同时被 PotionEntity::onHitAsWater 查询：水瓶命中范围内水敏感实体
     * 受 1.0 indirectMagic 伤害（AbstractThrownPotion.onHitAsWater:93-95）。
     */
    [[nodiscard]] bool isWaterSensitive() const override { return true; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 烈焰人不在阳光下燃烧
     *
     * MC 1.21.11 中烈焰人不在 BURN_IN_DAYLIGHT 实体标签中，
     * 且注册为 fireImmune()，因此不会在阳光下燃烧。
     * 即使 burnUndead() 被调用，fireImmune 也会在 baseTick 中立即清除火焰。
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 亮度 ==========

    /**
     * @brief 获取亮度（充能时全身冒火，亮度为1）
     */
    [[nodiscard]] f32 getBrightness() const { return 1.0f; }

    // ========== 摔落伤害 ==========

    /**
     * @brief 烈焰人免疫摔落伤害
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

    // ========== 行为常量 ==========
    // 对齐 MC 1.21.11 Blaze 行为参数，公开以便测试引用

    static constexpr f32 HEIGHT_OFFSET_MODE = 0.5f;           ///< 高度偏移三角分布的众数
    static constexpr f32 HEIGHT_OFFSET_DEVIATION = 6.891f;    ///< 高度偏移三角分布的偏差
    static constexpr i32 HEIGHT_OFFSET_CHANGE_INTERVAL = 100; ///< 高度偏移重新随机化的间隔（ticks）
    static constexpr f32 ASCEND_TARGET_SPEED = 0.3f;          ///< 上升时收敛的目标Y轴速度
    static constexpr f32 ASCEND_ACCELERATION = 0.3f;          ///< 上升推力系数
    static constexpr f32 FALL_DAMPING = 0.6f;                 ///< 下落缓降系数
    static constexpr f32 WATER_DAMAGE_AMOUNT = 1.0f;          ///< 水伤害量

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 更新AI任务
     *
     * 实现烈焰人特有的悬浮上升推力和高度偏移随机化，
     * 对齐 MC 1.21.11 Blaze.customServerAiStep()。
     */
    void updateAITasks() override;

private:
    // 攻击状态
    bool m_charging = false;

    // 悬浮高度偏移（对齐 MC Blaze.allowedHeightOffset / nextHeightOffsetChangeTick）
    f32 m_allowedHeightOffset = 0.5f;
    i32 m_nextHeightOffsetChangeTick = 0;
};

} // namespace mc
