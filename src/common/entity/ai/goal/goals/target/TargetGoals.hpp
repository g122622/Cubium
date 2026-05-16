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

#include "core/Types.hpp"
#include "entity/interfaces/IAngerable.hpp"
#include "../../Goal.hpp"
#include <functional>

namespace mc {

// Forward declarations
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 目标筛选谓词类型
 *
 * 用于筛选可以作为攻击目标的实体。
 * 返回 true 表示该实体可以作为目标，false 表示不能。
 */
using TargetPredicate = std::function<bool(const LivingEntity*)>;

/**
 * @brief 目标选择目标基类
 *
 * 用于选择攻击目标的目标类型。
 * 与普通Goal不同，TargetGoal专门用于targetSelector。
 *
 * 参考 MC 1.16.5 TargetGoal
 */
class TargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param checkSight 是否需要视线检查
     */
    TargetGoal(MobEntity* mob, bool checkSight);

    ~TargetGoal() override = default;

    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

protected:
    /**
     * @brief 检查目标是否适合攻击
     * @param target 目标实体
     * @return 如果适合攻击返回true
     */
    [[nodiscard]] bool isSuitableTarget(LivingEntity* target) const;

    /**
     * @brief 检查视线
     * @return 如果能看到目标返回true
     */
    [[nodiscard]] bool checkSight() const;

    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    bool m_checkSight;
    i32 m_unseenTicks = 0;

    // 看不到目标后的记忆时间
    static constexpr i32 MAX_UNSEEN_TICKS = 60; // 3秒
};

/**
 * @brief 最近可攻击目标选择
 *
 * 选择最近的符合条件的目标进行攻击。
 *
 * 参考 MC 1.16.5 NearestAttackableTargetGoal
 */
template <typename T>
class NearestAttackableTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param checkSight 是否需要视线检查
     * @param chance 每tick检查的概率倒数（0=每tick都检查）
     */
    NearestAttackableTargetGoal(MobEntity* mob, bool checkSight, i32 chance = 0);

    /**
     * @brief 构造函数（带目标筛选谓词）
     * @param mob 拥有此目标的生物
     * @param checkSight 是否需要视线检查
     * @param chance 每tick检查的概率倒数（0=每tick都检查）
     * @param predicate 目标筛选谓词
     */
    NearestAttackableTargetGoal(MobEntity* mob, bool checkSight, i32 chance, TargetPredicate predicate);

    ~NearestAttackableTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

private:
    i32 m_chance;
    T* m_targetEntity = nullptr;
    TargetPredicate m_predicate; // 目标筛选谓词（可选）
};

/**
 * @brief 被攻击后反击目标
 *
 * 当实体被攻击时，记住攻击者并反击。
 *
 * 参考 MC 1.16.5 HurtByTargetGoal
 */
class HurtByTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param alertAllies 是否警醒盟友
     */
    HurtByTargetGoal(MobEntity* mob, bool alertAllies = false);

    ~HurtByTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;

private:
    bool m_alertAllies;
    i32 m_timestamp = 0;
};

/**
 * @brief 主人被攻击时反击目标
 *
 * 当驯服动物的主人被攻击时，反击攻击者。
 * 需要配合TameableEntity使用。
 *
 * 参考 MC 1.16.5 OwnerHurtByTargetGoal
 */
class OwnerHurtByTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的驯服动物
     */
    explicit OwnerHurtByTargetGoal(MobEntity* mob);

    ~OwnerHurtByTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
};

/**
 * @brief 攻击主人正在攻击的目标
 *
 * 当驯服动物的主人攻击某实体时，也攻击该实体。
 *
 * 参考 MC 1.16.5 OwnerHurtTargetGoal
 */
class OwnerHurtTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的驯服动物
     */
    explicit OwnerHurtTargetGoal(MobEntity* mob);

    ~OwnerHurtTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
};

/**
 * @brief 未驯服状态目标选择
 *
 * 驯服动物在未驯服状态下选择攻击目标。
 * 用于狼等动物。
 *
 * 参考 MC 1.16.5 NonTamedTargetGoal
 */
template <typename T>
class NonTamedTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的驯服动物
     * @param checkSight 是否需要视线检查
     */
    NonTamedTargetGoal(MobEntity* mob, bool checkSight);

    /**
     * @brief 构造函数（带目标筛选谓词）
     * @param mob 拥有此目标的驯服动物
     * @param checkSight 是否需要视线检查
     * @param predicate 目标筛选谓词
     */
    NonTamedTargetGoal(MobEntity* mob, bool checkSight, TargetPredicate predicate);

    ~NonTamedTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

private:
    T* m_targetEntity = nullptr;
    TargetPredicate m_predicate; // 目标筛选谓词（可选）
};

/**
 * @brief 重置愤怒目标
 *
 * 当 UNIVERSAL_ANGER 游戏规则启用时，检查并处理愤怒目标。
 * 用于实现了 IAngerable 接口的实体（如铁傀儡、末影人等）。
 *
 * 参考 MC 1.16.5 ResetAngerGoal
 */
template <typename T>
class ResetAngerGoal : public Goal {
public:
    static_assert(std::is_base_of<MobEntity, T>::value && std::is_base_of<entity::IAngerable, T>::value,
        "ResetAngerGoal<T> requires T to be derived from both MobEntity and IAngerable");

    /**
     * @brief 构造函数
     * @param mob 拥有此目标的实体
     * @param alertOthers 是否警醒附近同类实体
     */
    ResetAngerGoal(T* mob, bool alertOthers);

    ~ResetAngerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] std::string getTypeName() const override { return "ResetAngerGoal"; }

private:
    T* m_mob;
    bool m_alertOthers;
    i32 m_revengeTimer = 0;

    /**
     * @brief 检查是否应该对玩家复仇
     * @return 如果应该复仇则返回true
     */
    [[nodiscard]] bool shouldGetRevengeOnPlayer() const;

    /**
     * @brief 获取附近的同类实体
     * @return 同类实体列表
     */
    [[nodiscard]] std::vector<T*> getNearbySameTypeEntities() const;
};

} // namespace entity::ai::goal
} // namespace mc
