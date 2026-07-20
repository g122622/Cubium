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
#include "../../../core/DataParameter.hpp"
#include "../../../core/EntitySize.hpp"
#include "../../../interfaces/IAngerable.hpp"
#include "entity/entities/passive/basic/AnimalEntity.hpp"
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
 */
class PolarBearEntity : public AnimalEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    PolarBearEntity(EntityInstanceId id);
    ~PolarBearEntity() override = default;

    // 禁止拷贝
    PolarBearEntity(const PolarBearEntity&) = delete;
    PolarBearEntity& operator=(const PolarBearEntity&) = delete;

    // 允许移动
    PolarBearEntity(PolarBearEntity&&) = delete;
    PolarBearEntity& operator=(PolarBearEntity&&) = delete;

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
    [[nodiscard]] bool isStanding() const { return m_dataManager.get<bool>(DATA_STANDING_PARAM); }

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

    /**
     * @brief 获取站立动画缩放值
     *
     * 用于模型渲染时计算站立姿态的插值。
     *
     * @param partialTick 部分 Tick 值
     * @return 动画进度 [0.0, 1.0]，0 表示四足站立，1 表示后腿站立
     */
    [[nodiscard]] f32 getStandingAnimationScale(f32 partialTick) const;

    /**
     * @brief 获取站立状态数据参数ID
     *
     * 用于客户端从元数据中读取站立状态。
     */
    [[nodiscard]] static u16 getStandingParamId() { return DATA_STANDING_PARAM.id(); }

    /**
     * @brief 设置客户端站立动画值
     *
     * 用于测试和调试，直接设置动画进度值。
     * 正常情况下由 tick() 在客户端自动更新。
     *
     * @param animation 动画进度值 [0.0, STAND_ANIMATION_TICKS]
     */
    void setClientSideStandAnimation(f32 animation) { m_clientSideStandAnimation = animation; }

    // ========== 属性 ==========

    /**
     * @brief 获取基础宽度
     * 北极熊的基础宽度为 1.4 格
     */
    [[nodiscard]] f32 getBaseWidth() const override { return POLAR_BEAR_WIDTH; }

    /**
     * @brief 获取基础高度
     * 北极熊的基础高度为 1.4 格（四足站立）
     */
    [[nodiscard]] f32 getBaseHeight() const override { return POLAR_BEAR_HEIGHT; }

    /**
     * @brief 根据姿态和站立动画状态获取碰撞箱尺寸
     *
     * 站立动画期间，北极熊的高度会随动画进度逐渐增大。
     * 完全站立时高度为基础高度的 2 倍（1.4 * 2.0 = 2.8）。
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;

    /**
     * @brief 获取眼睛高度
     * 成年北极熊眼高为 1.19（1.4 * 0.85），幼熊为 0.595（0.7 * 0.85）
     */
    [[nodiscard]] f32 eyeHeight() const override
    {
        return isChild() ? POLAR_BEAR_HEIGHT * AgeableEntity::BABY_SCALE * 0.85f : POLAR_BEAR_HEIGHT * 0.85f;
    }

    // ========== IAngerable 接口实现 ==========

    /**
     * @brief 设置攻击目标 (IAngerable)
     */
    void setAttackTarget(LivingEntity* target) override { MobEntity::setAttackTarget(target); }

    /**
     * @brief 获取攻击目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const override
    {
        return const_cast<PolarBearEntity*>(this)->MobEntity::attackTarget();
    }

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
     */
    bool attackEntityAsMob(LivingEntity& target) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据同步 ==========
    void registerData() override;

    // ========== 刻更新 ==========
    void tick() override;

private:
    // ========== 数据同步 ==========
    static entity::DataParameter<bool> DATA_STANDING_PARAM;

    // ========== 站立状态 ==========
    bool m_standing = false;
    bool m_warning = false;
    i32 m_standTimer = 0;
    i32 m_warningSoundTicks = 0;

    // ========== 客户端站立动画 ==========
    // 这些字段仅在客户端使用，用于平滑站立动画
    mutable f32 m_clientSideStandAnimation0 = 0.0f;
    mutable f32 m_clientSideStandAnimation = 0.0f;

    // ========== 愤怒系统（m_attackTarget 使用 MobEntity::m_attackTarget，不重复声明） ==========
    std::optional<u64> m_revengeTargetId;
    i32 m_revengeTimer = 0;
    i32 m_angerTime = 0;

    // ========== 常量 ==========
    static constexpr f32 POLAR_BEAR_WIDTH = 1.4f;      // 基础宽度
    static constexpr f32 POLAR_BEAR_HEIGHT = 1.4f;     // 基础高度（四足站立）
    static constexpr f32 STAND_ANIMATION_TICKS = 6.0f; // 站立动画过渡帧数
    static constexpr i32 STAND_DURATION_MIN = 100;     // 最小站立时间 (ticks)
    static constexpr i32 STAND_DURATION_MAX = 400;     // 最大站立时间 (ticks)
    static constexpr i32 WARNING_SOUND_COOLDOWN = 40;  // 警告声音冷却 (ticks)
    static constexpr i32 MAX_ANGER_TIME = 600;         // 最大愤怒时间 (30秒)
    static constexpr i32 ANGER_TIME_MIN = 20;          // 最小愤怒时间 (ticks)
    static constexpr i32 ANGER_TIME_MAX = 39;          // 最大愤怒时间 (ticks)

    // 友元类声明（AI目标类需要访问私有成员）
    friend class PolarBearMeleeAttackGoal;
    friend class PolarBearPanicGoal;
    friend class PolarBearHurtByTargetGoal;
    friend class PolarBearAttackPlayerGoal;
};

} // namespace mc
