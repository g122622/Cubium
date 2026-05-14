#pragma once

#include "../../../../../core/Types.hpp"
#include "../../Goal.hpp"

namespace mc {

// Forward declarations
class CreatureEntity;
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 避开水随机行走目标
 *
 * 与RandomWalkingGoal类似，但会避开水域。
 * 适用于大多数陆地生物。
 *
 * 参考 MC 1.16.5 WaterAvoidingRandomWalkingGoal
 */
class WaterAvoidingRandomWalkingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     */
    WaterAvoidingRandomWalkingGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（带概率）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param chance 每tick执行的概率（0.0-1.0）
     */
    WaterAvoidingRandomWalkingGoal(CreatureEntity* creature, f64 speed, f32 chance);

    ~WaterAvoidingRandomWalkingGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    /**
     * @brief 获取随机目标位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool getRandomPosition();

    /**
     * @brief 检查位置是否在水或岩浆中
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 如果在水中或岩浆中返回true
     */
    [[nodiscard]] bool isInWaterOrLava(f64 x, f64 y, f64 z) const;

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_chance;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_timeout = 0;
    bool m_isRunning = false;

    static constexpr i32 MAX_TIMEOUT = 600; // 最大行走时间（30秒）
};

/**
 * @brief 跳跃攻击目标
 *
 * 使实体跳跃向目标攻击。
 * 适用于蜘蛛等会跳跃攻击的生物。
 *
 * 参考 MC 1.16.5 LeapAtTargetGoal
 */
class LeapAtTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param leapHeight 跳跃高度
     */
    LeapAtTargetGoal(MobEntity* mob, f32 leapHeight);

    ~LeapAtTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

private:
    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    f32 m_leapHeight;
    bool m_leaped = false;

    static constexpr f32 MIN_DISTANCE = 4.0f; // 最小跳跃距离
    static constexpr f32 MAX_DISTANCE = 8.0f; // 最大跳跃距离
};

} // namespace entity::ai::goal
} // namespace mc
