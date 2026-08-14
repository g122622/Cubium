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
#include "../Goal.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

namespace mc {

// 前向声明
class MobEntity;
class LivingEntity;
class Player;
class IWorld;

namespace entity {
class EntityPredicate;
}

namespace entity::ai::goal {

/**
 * @brief 类型过滤标记结构
 *
 * 用于 LookAtGoal 的模板构造函数，指定要看向的实体类型。
 *
 * @tparam T 目标实体类型（必须继承自 LivingEntity）
 */
template <typename T>
struct TypeFilter {
    static_assert(std::is_base_of_v<LivingEntity, T>, "T must derive from LivingEntity");
};

/**
 * @brief 看向实体目标
 *
 * 使生物看向附近的指定类型实体。
 *
 * 支持三种构造方式：
 * 1. 看向任意 LivingEntity：LookAtGoal(mob, maxDistance)
 * 2. 看向特定类型实体：LookAtGoal(mob, maxDistance, chance, TypeFilter<T>{})
 * 3. 看向自定义过滤实体：LookAtGoal(mob, maxDistance, chance, filter)
 */
class LookAtGoal : public Goal {
public:
    /**
     * @brief 实体过滤函数类型
     */
    using EntityFilter = std::function<bool(const LivingEntity*)>;

    /**
     * @brief 默认看向概率 (2%)
     */
    static constexpr f32 DEFAULT_LOOK_CHANCE = 0.02f;

    /**
     * @brief 构造函数（看向任意LivingEntity）
     * @param mob 拥有此目标的生物
     * @param maxDistance 最大观看距离
     */
    LookAtGoal(MobEntity* mob, f32 maxDistance);

    /**
     * @brief 构造函数（看向任意LivingEntity，带概率）
     * @param mob 拥有此目标的生物
     * @param maxDistance 最大观看距离
     * @param chance 每tick执行的概率（0-1）
     */
    LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance);

    /**
     * @brief 构造函数（带自定义过滤条件）
     * @param mob 拥有此目标的生物
     * @param maxDistance 最大观看距离
     * @param chance 每tick执行的概率（0-1）
     * @param filter 实体过滤函数
     */
    LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance, EntityFilter filter);

    /**
     * @brief 构造函数（看向特定类型的实体）
     * @tparam T 目标实体类型（必须继承自 LivingEntity）
     * @param mob 拥有此目标的生物
     * @param maxDistance 最大观看距离
     * @param chance 每tick执行的概率（0-1）
     * @param typeFilter 类型过滤标记（用于模板特化）
     *
     * 使用示例：
     * @code
     * // 看向附近的炽足兽
     * m_goalSelector.addGoal(9, std::make_unique<LookAtGoal>(
     *     this, 8.0f, LookAtGoal::DEFAULT_LOOK_CHANCE, TypeFilter<StriderEntity>{}));
     * @endcode
     */
    template <typename T, typename = std::enable_if_t<std::is_base_of_v<LivingEntity, T>>>
    LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance, TypeFilter<T> typeFilter);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "LookAtGoal"; }

protected:
    /**
     * @brief 寻找要看的实体
     * @return 找到的实体，如果没有返回nullptr
     */
    [[nodiscard]] virtual LivingEntity* findTarget();

    MobEntity* m_mob;
    LivingEntity* m_lookTarget = nullptr;
    EntityFilter m_filter; // 实体过滤函数
    f32 m_maxDistance;
    f32 m_chance;
    i32 m_lookTime = 0;

    static constexpr i32 LOOK_AT_MIN_TIME = 40; // 2秒
    static constexpr i32 LOOK_AT_MAX_TIME = 80; // 4秒
};

template <typename T, typename>
LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance, TypeFilter<T> /*typeFilter*/)
    : m_mob(mob)
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
    // 设置类型过滤谓词：使用 dynamic_cast 检查类型
    m_filter = [](const LivingEntity* entity) -> bool { return dynamic_cast<const T*>(entity) != nullptr; };
}

/**
 * @brief 随机看向目标
 *
 * 使生物随机看向往某个方向。
 */
class LookRandomlyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     */
    explicit LookRandomlyGoal(MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    // 对齐 vanilla RandomLookAroundGoal.requiresUpdateEveryTick()=true：
    // 该 goal 每 tick 评估（不走 GoalSelector 半 tick 节流），故 shouldExecute 概率
    // 与 startExecuting 的 lookTime 均用裸值，不经 adjustedTickDelay 减半。
    [[nodiscard]] bool requiresUpdateEveryTick() const override { return true; }

    [[nodiscard]] std::string getTypeName() const override { return "LookRandomlyGoal"; }

private:
    MobEntity* m_mob;
    f64 m_lookX = 0.0;  // 方向向量的 X 分量 (cos(angle))
    f64 m_lookZ = 0.0;  // 方向向量的 Z 分量 (sin(angle))
    i32 m_idleTime = 0; // 剩余看向时间（ticks）

    static constexpr f32 RANDOM_LOOK_CHANCE = 0.02f; // 2%
    static constexpr i32 RANDOM_LOOK_MIN_TIME = 20;  // 1秒
    static constexpr i32 RANDOM_LOOK_MAX_TIME = 40;  // 2秒
};

} // namespace entity::ai::goal
} // namespace mc
