#pragma once

#include "../../../../core/Types.hpp"
#include "../Goal.hpp"
#include <functional>

namespace mc {

// 前向声明
class MobEntity;
class LivingEntity;
class IWorld;

namespace entity {
class EntityPredicate;
}

namespace entity::ai::goal {

/**
 * @brief 看向实体目标
 *
 * 使生物看向附近的指定类型实体。
 *
 * 参考 MC 1.16.5 LookAtGoal
 */
class LookAtGoal : public Goal {
public:
    /**
     * @brief 实体过滤函数类型
     */
    using EntityFilter = std::function<bool(const LivingEntity*)>;

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

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "LookAtGoal"; }

protected:
    /**
     * @brief 寻找要看的实体
     * MC 1.16.5: 使用 getClosestPlayer 或 getEntitiesWithinAABB
     * @return 找到的实体，如果没有返回nullptr
     */
    [[nodiscard]] virtual LivingEntity* findTarget();

    MobEntity* m_mob;
    LivingEntity* m_lookTarget = nullptr;
    EntityFilter m_filter; // 实体过滤函数
    f32 m_maxDistance;
    f32 m_chance;
    i32 m_lookTime = 0;

    static constexpr i32 LOOK_AT_MIN_TIME = 40;       // 2秒
    static constexpr i32 LOOK_AT_MAX_TIME = 80;       // 4秒
    static constexpr f32 DEFAULT_LOOK_CHANCE = 0.02f; // 2%
};

/**
 * @brief 随机看向目标
 *
 * 使生物随机看向往某个方向。
 *
 * 参考 MC 1.16.5 LookRandomlyGoal
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

    [[nodiscard]] std::string getTypeName() const override { return "LookRandomlyGoal"; }

private:
    MobEntity* m_mob;
    f64 m_lookX = 0.0;  // MC 1.16.5: 方向向量的 X 分量 (cos(angle))
    f64 m_lookZ = 0.0;  // MC 1.16.5: 方向向量的 Z 分量 (sin(angle))
    i32 m_idleTime = 0; // MC 1.16.5: idleTime

    static constexpr f32 RANDOM_LOOK_CHANCE = 0.02f; // 2%
    static constexpr i32 RANDOM_LOOK_MIN_TIME = 20;  // 1秒
    static constexpr i32 RANDOM_LOOK_MAX_TIME = 40;  // 2秒
};

} // namespace entity::ai::goal
} // namespace mc
