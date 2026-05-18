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
* THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
#include "../../../interfaces/IAngerable.hpp"
#include "../basic/AnimalEntity.hpp"
#include <memory>
#include <optional>
#include <string>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {
class MeleeAttackGoal;
class PanicGoal;
class HurtByTargetGoal;
template <typename T>
class NearestAttackableTargetGoal;
} // namespace entity::ai::goal

/**
 * @brief 北极熊实体
 *
 * 生活在冰原的大型中立动物。
 *
 * 特性：
 * - 保护幼崽：幼熊附近有成年熊时会攻击玩家
 * - 游泳：擅长游泳
 * - 站立：可以后腿站立
 * - 攻击：被攻击后会反击
 * - 幼崽：小北极熊
 * - 愤怒：实现 IAngerable 接口，被攻击后会记住攻击者
 *
 * 参考 MC 1.16.5 PolarBearEntity
 */
class PolarBearEntity : public AnimalEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    PolarBearEntity(EntityId id);
    ~PolarBearEntity() override = default;

    // 禁止拷贝
    PolarBearEntity(const PolarBearEntity&) = delete;
    PolarBearEntity& operator=(const PolarBearEntity&) = delete;

    // 允许移动
    PolarBearEntity(PolarBearEntity&&) = default;
    PolarBearEntity& operator=(PolarBearEntity&&) = default;

    /**
     * @brief 创建北极熊实体
     * @param world 世界实例
     * @return 新的北极熊实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 状态 ==========

    /**
     * @brief 是否正在站立
     */
    [[nodiscard]] bool isStanding() const { return m_standing; }

    /**
     * @brief 设置站立状态
     */
    void setStanding(bool standing);

    /**
     * @brief 是否正在警告
     * 站立并张开前爪
     */
    [[nodiscard]] bool isWarning() const { return m_warning; }

    /**
     * @brief 设置警告状态
     */
    void setWarning(bool warning);

    // ========== 繁殖 ==========

    /**
     * @brief 北极熊不可繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 北极熊不可繁殖
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        (void)partner;
        return nullptr;
    }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.7f : 1.4f; }

    // ========== IAngerable 接口实现 ==========

    /**
     * @brief 设置攻击目标 (IAngerable)
     */
    void setAttackTarget(LivingEntity* target) override { m_attackTarget = target; }

    /**
     * @brief 获取攻击目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const override { return m_attackTarget; }

    /**
     * @brief 设置复仇目标 (IAngerable)
     */
    void setRevengeTarget(LivingEntity* target) override;

    /**
     * @brief 获取复仇目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;

    /**
     * @brief 获取复仇计时器 (IAngerable)
     */
    [[nodiscard]] i32 getRevengeTimer() const override { return m_revengeTimer; }

    /**
     * @brief 是否愤怒 (IAngerable)
     */
    [[nodiscard]] bool isAngry() const override { return m_angerTime > 0; }

    /**
     * @brief 设置愤怒状态 (IAngerable)
     */
    void setAngry(bool angry) override;

    /**
     * @brief 获取愤怒时间 (IAngerable)
     */
    [[nodiscard]] i32 getAngerTime() const override { return m_angerTime; }

    /**
     * @brief 设置愤怒时间 (IAngerable)
     */
    void setAngerTime(i32 time) override { m_angerTime = time; }

    /**
     * @brief 更新愤怒计时器 (IAngerable)
     */
    void updateAnger() override;

    // ========== 声音事件 ==========

    /**
     * @brief 获取环境音效
     * 幼熊使用不同的音效
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
     * @brief 播放脚步声
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    /**
     * @brief 播放警告声
     * 站立时播放的警告音效
     */
    void playWarningSound();

    // ========== 攻击 ==========

    /**
     * @brief 作为生物攻击目标
     * MC 1.16.5: attackEntityAsMob
     */
    bool attackEntityAsMob(LivingEntity& target) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 刻更新 ==========
    void tick() override;

private:
    // ========== 站立状态 ==========
    bool m_standing = false;
    bool m_warning = false;
    i32 m_standTimer = 0;
    i32 m_warningSoundTicks = 0;

    // ========== 愤怒系统 ==========
    LivingEntity* m_attackTarget = nullptr;
    std::optional<u64> m_revengeTargetId;
    i32 m_revengeTimer = 0;
    i32 m_angerTime = 0;

    // ========== 常量 ==========
    static constexpr i32 STAND_DURATION_MIN = 100;    // 最小站立时间 (ticks)
    static constexpr i32 STAND_DURATION_MAX = 400;    // 最大站立时间 (ticks)
    static constexpr i32 WARNING_SOUND_COOLDOWN = 40; // 警告声音冷却 (ticks)
    static constexpr i32 MAX_ANGER_TIME = 600;        // 最大愤怒时间 (30秒)
    static constexpr i32 ANGER_TIME_MIN = 20;         // 最小愤怒时间 (ticks)
    static constexpr i32 ANGER_TIME_MAX = 39;         // 最大愤怒时间 (ticks)

    // 友元类声明（AI目标类需要访问私有成员）
    friend class PolarBearMeleeAttackGoal;
    friend class PolarBearPanicGoal;
    friend class PolarBearHurtByTargetGoal;
    friend class PolarBearAttackPlayerGoal;
};

} // namespace mc
