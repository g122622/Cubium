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
#include "../../../../entity/effect/EffectType.hpp"
#include "../../../../entity/interfaces/IRangedAttackMob.hpp"
#include "AbstractRaiderEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
namespace entity::effect {
class EffectInstance;
}

/**
 * @brief 女巫实体
 *
 * 使用药水的敌对生物，可参与掠夺事件。
 *
 * 特性：
 * - 药水攻击：向玩家投掷负面药水
 * - 治疗：受伤时会使用治疗药水
 * - 掉落：药水材料
 * - 生成：在沼泽小屋
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity -> WitchEntity
 */
class WitchEntity : public AbstractRaiderEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    WitchEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~WitchEntity() override = default;

    // 禁止拷贝
    WitchEntity(const WitchEntity&) = delete;
    WitchEntity& operator=(const WitchEntity&) = delete;

    // 允许移动
    WitchEntity(WitchEntity&&) = delete;
    WitchEntity& operator=(WitchEntity&&) = delete;

    /// 本类继承链标识（parent = AbstractRaiderEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建女巫实体
     * @param world 世界实例
     * @return 新的女巫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 药水系统 ==========

    /**
     * @brief 是否正在喝药水
     */
    [[nodiscard]] bool isDrinking() const { return m_drinking; }

    /**
     * @brief 设置喝药水状态
     */
    void setDrinking(bool drinking) { m_drinking = drinking; }

    /**
     * @brief 获取喝药水计时器
     */
    [[nodiscard]] i32 getDrinkTimer() const { return m_drinkTimer; }

    /**
     * @brief 设置喝药水计时器
     */
    void setDrinkTimer(i32 timer) { m_drinkTimer = timer; }

    // ========== 攻击 ==========

    /**
     * @brief 获取攻击冷却
     */
    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }

    /**
     * @brief 重置攻击冷却
     */
    void resetAttackCooldown() { m_attackCooldown = ATTACK_COOLDOWN; }

    // ========== 常量 ==========

    /**
     * @brief 喝药水时移动速度减益的 UUID
     *
     * MC 1.16.5: 女巫喝药水时移动速度减少 0.25（Addition 操作）
     * UUID: "5CD17E52-A79A-43D3-A529-90FDE04B181E"
     */
    static constexpr const char* DRINKING_SPEED_PENALTY_UUID = "5CD17E52-A79A-43D3-A529-90FDE04B181E";

    // ========== 阳光燃烧 ==========

    /**
     * @brief 女巫不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.95f; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    // ========== 防御 ==========

    /**
     * @brief 魔法伤害减免
     *
     * 女巫对魔法伤害有85%减免，同时免疫自己造成的伤害。
     *
     * @param source 伤害来源
     * @param amount 原始伤害量
     * @return 减免后的伤害量
     */
    [[nodiscard]] f32 applyMagicDamageReduction(DamageSource& source, f32 amount);

    // ========== 远程攻击 (IRangedAttackMob) ==========

    /**
     * @brief 对目标进行远程攻击（投掷药水）
     *
     * 根据目标状态选择不同的药水类型：
     * - 对掠夺者同伴：治疗/再生药水
     * - 距离>=8且无缓慢：缓慢药水
     * - 生命>=8且无中毒：中毒药水
     * - 距离<=3且无虚弱（25%概率）：虚弱药水
     * - 默认：伤害药水
     *
     * @param target 目标实体
     * @param charge 蓄力程度（未使用，女巫攻击固定参数）
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    /**
     * @brief 获取攻击间隔时间
     * @return 60 ticks (3秒)
     */
    [[nodiscard]] i32 getAttackInterval() const override { return ATTACK_COOLDOWN; }

    /**
     * @brief 检查是否可以进行远程攻击
     * @return 如果不在喝药水状态返回true
     */
    [[nodiscard]] bool canRangedAttack() const override { return !m_drinking; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // ========== 药水决策逻辑 ==========

    /**
     * @brief 检查是否需要治疗
     */
    [[nodiscard]] bool _needsHealing() const;

    /**
     * @brief 检查是否需要水肺药水
     */
    [[nodiscard]] bool _needsWaterBreathing() const;

    /**
     * @brief 检查是否需要抗火药水
     */
    [[nodiscard]] bool _needsFireResistance() const;

    /**
     * @brief 检查最后一次伤害来源是否是火焰
     */
    [[nodiscard]] bool _lastDamageSourceWasFire() const;

    /**
     * @brief 检查是否需要速度药水
     */
    [[nodiscard]] bool _needsSwiftness() const;

    /**
     * @brief 决定使用哪种药水
     *
     * 按优先级检查：水肺 > 抗火 > 治疗 > 速度
     *
     * @return 效果类型，如果不需要药水返回空
     */
    [[nodiscard]] std::optional<entity::effect::EffectType> _decidePotionToDrink();

    /**
     * @brief 开始喝药水
     *
     * 设置喝药水状态、播放音效、应用移动速度减益
     *
     * @param effectType 要喝的药水效果类型
     */
    void _startDrinkingPotion(entity::effect::EffectType effectType);

    /**
     * @brief 完成喝药水
     *
     * 应用效果、移除移动速度减益、清空状态
     */
    void _finishDrinkingPotion();

    /**
     * @brief 应用喝药水的效果
     *
     * 根据药水类型应用相应效果：
     * - 瞬间治疗：直接恢复生命值
     * - 其他效果：添加到效果管理器
     *
     * @param effectType 效果类型
     */
    void _applyDrankPotionEffect(entity::effect::EffectType effectType);

    /**
     * @brief 选择对目标的攻击药水类型
     *
     * 根据目标状态选择药水：
     * - 如果目标是掠夺者同伴（AbstractRaiderEntity）：
     *   - 生命值<=4：治疗药水
     *   - 否则：再生药水
     * - 距离>=8格且目标无缓慢效果：缓慢药水
     * - 目标生命>=8且无中毒效果：中毒药水
     * - 距离<=3格且无虚弱效果（25%概率）：虚弱药水
     * - 默认：伤害药水
     *
     * @param target 目标实体
     * @return 药水效果类型
     */
    [[nodiscard]] entity::effect::EffectType _selectAttackPotionType(LivingEntity* target) const;

    /**
     * @brief 投掷药水到目标位置
     *
     * @param target 目标实体
     * @param potionType 药水效果类型
     */
    void _throwPotionAt(LivingEntity* target, entity::effect::EffectType potionType);

private:
    // 药水状态
    bool m_drinking = false;
    i32 m_drinkTimer = 0;

    // 当前正在喝的药水效果类型
    entity::effect::EffectType m_currentPotionType = entity::effect::EffectType::InstantHealth;

    // 攻击冷却
    i32 m_attackCooldown = 0;

    // 常量
    static constexpr i32 ATTACK_COOLDOWN = 60; // 3秒攻击冷却
    static constexpr i32 DRINK_DURATION = 32;  // 喝药水时间（ticks）

    // 药水攻击常量
    static constexpr f32 ATTACK_RADIUS = 10.0f;       // 攻击半径
    static constexpr f32 ATTACK_RADIUS_SQ = 100.0f;   // 攻击半径平方
    static constexpr f32 POTION_VELOCITY = 0.75f;     // 药水投掷速度
    static constexpr f32 POTION_INACCURACY = 8.0f;    // 药水散射度
    static constexpr f32 CLOSE_RANGE_DISTANCE = 3.0f; // 近距离阈值
    static constexpr f32 FAR_RANGE_DISTANCE = 8.0f;   // 远距离阈值

    // 药水触发概率
    static constexpr f32 WATER_BREATHING_CHANCE = 0.15f; // 水肺药水概率 15%
    static constexpr f32 FIRE_RESISTANCE_CHANCE = 0.15f; // 抗火药水概率 15%
    static constexpr f32 HEALING_CHANCE = 0.05f;         // 治疗药水概率 5%
    static constexpr f32 SWIFTNESS_CHANCE = 0.5f;        // 速度药水概率 50%
    static constexpr f32 SWIFTNESS_DISTANCE_SQ = 121.0f; // 速度药水距离阈值 11^2

    // 攻击药水选择概率
    static constexpr f32 WEAKNESS_CHANCE = 0.25f; // 虚弱药水概率 25%
};

} // namespace mc
