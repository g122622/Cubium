#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"
#include <functional>

namespace mc {

// 前向声明
class CreatureEntity;
class LivingEntity;
class Player;
class MobEntity;
class AgeableEntity;

namespace entity::ai::goal {

/**
 * @brief 吃草目标
 *
 * 使羊等动物吃草，用于羊毛重新生长。
 *
 * 参考 MC 1.16.5 EatGrassGoal
 */
class EatGrassGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     */
    explicit EatGrassGoal(CreatureEntity* creature);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "EatGrassGoal"; }

    /**
     * @brief 获取吃草动画计时器
     */
    [[nodiscard]] i32 getEatAnimationTick() const { return m_eatAnimationTick; }

private:
    CreatureEntity* m_creature;
    i32 m_eatAnimationTick = 0;
    BlockCoord m_targetBlock;

    static constexpr i32 EAT_DURATION = 40;  // 吃草持续时间（ticks）
    static constexpr i32 EAT_COOLDOWN = 100;  // 吃草冷却（ticks）
};

/**
 * @brief 飞行目标
 *
 * 使生物能够在空中飞行。
 *
 * 参考 MC 1.16.5 FlyGoal
 */
class FlyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 飞行速度倍率
     */
    FlyGoal(CreatureEntity* creature, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "FlyGoal"; }

protected:
    /**
     * @brief 寻找飞行目标位置
     * @return 如果找到返回true
     */
    bool findFlightTarget();

    CreatureEntity* m_creature;
    f64 m_speed;
    Vector3 m_targetPos;
    i32 m_flightTime = 0;

    static constexpr i32 MAX_FLIGHT_TIME = 200;  // 最大飞行时间
    static constexpr f32 FLIGHT_HEIGHT_MIN = 10.0f;  // 最小飞行高度
    static constexpr f32 FLIGHT_HEIGHT_MAX = 30.0f;  // 最大飞行高度
};

/**
 * @brief 睡觉目标
 *
 * 使生物在天黑时睡觉。
 * 主要用于村民。
 *
 * 参考 MC 1.16.5 SleepGoal
 */
class SleepGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     */
    explicit SleepGoal(CreatureEntity* creature);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] String getTypeName() const override { return "SleepGoal"; }

private:
    CreatureEntity* m_creature;
    BlockCoord m_bedPos;

    /**
     * @brief 检查是否是睡觉时间
     */
    [[nodiscard]] bool isSleepTime() const;

    /**
     * @brief 寻找床
     */
    [[nodiscard]] bool findBed();
};

/**
 * @brief 工作目标
 *
 * 使村民在工作时间前往工作站点工作。
 *
 * 参考 MC 1.16.5 WorkAtComposterGoal / WorkAtPoiGoal
 */
class WorkAtPoiGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体（村民）
     */
    explicit WorkAtPoiGoal(CreatureEntity* creature);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "WorkAtPoiGoal"; }

private:
    CreatureEntity* m_creature;
    BlockCoord m_workStation;
    i32 m_workTime = 0;

    /**
     * @brief 检查是否是工作时间
     */
    [[nodiscard]] bool isWorkTime() const;

    /**
     * @brief 执行工作
     */
    void performWork();
};

/**
 * @brief 寻找遮蔽目标
 *
 * 在雨天或夜晚寻找遮蔽处。
 *
 * 参考 MC 1.16.5 FindShelterGoal
 */
class FindShelterGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     */
    FindShelterGoal(CreatureEntity* creature, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "FindShelterGoal"; }

private:
    /**
     * @brief 寻找遮蔽位置
     */
    [[nodiscard]] bool findShelterPosition();

    CreatureEntity* m_creature;
    f64 m_speed;
    BlockCoord m_shelterPos;
    i32 m_timeout = 0;

    static constexpr i32 SEARCH_RADIUS = 32;
    static constexpr i32 TIMEOUT_MAX = 1200;
};

/**
 * @brief 逃离阳光目标
 *
 * 使亡灵生物在阳光下寻找阴影。
 *
 * 参考 MC 1.16.5 FleeSunGoal
 */
class FleeSunGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     */
    FleeSunGoal(CreatureEntity* creature, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] String getTypeName() const override { return "FleeSunGoal"; }

private:
    /**
     * @brief 寻找阴影位置
     */
    [[nodiscard]] bool findShadePosition();

    CreatureEntity* m_creature;
    f64 m_speed;
    BlockCoord m_targetPos;

    static constexpr i32 SEARCH_RADIUS = 16;
};

/**
 * @brief 返回家的目标
 *
 * 使生物在特定时间返回家中。
 *
 * 参考 MC 1.16.5 ReturnToVillageGoal
 */
class ReturnToHomeGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     * @param homeRadius 家的范围半径
     */
    ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius = 16.0f);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "ReturnToHomeGoal"; }

protected:
    /**
     * @brief 寻找回家路径
     */
    [[nodiscard]] bool findPathToHome();

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_homeRadius;
    Vector3 m_homePos;
    i32 m_timeout = 0;

    static constexpr i32 TIMEOUT_MAX = 600;
};

/**
 * @brief 交易目标
 *
 * 使村民能够与玩家交易。
 *
 * 参考 MC 1.16.5 TradeWithPlayerGoal
 */
class TradeWithPlayerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体（村民）
     */
    explicit TradeWithPlayerGoal(CreatureEntity* creature);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "TradeWithPlayerGoal"; }

private:
    CreatureEntity* m_creature;
    Player* m_customer = nullptr;
};

/**
 * @brief 展示交易目标
 *
 * 使村民向玩家展示交易物品。
 *
 * 参考 MC 1.16.5 ShowWaresGoal
 */
class ShowWaresGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体（村民）
     */
    explicit ShowWaresGoal(CreatureEntity* creature);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "ShowWaresGoal"; }

private:
    CreatureEntity* m_creature;
    Player* m_targetPlayer = nullptr;
    i32 m_displayTime = 0;

    static constexpr i32 DISPLAY_DURATION = 100;  // 展示持续时间
};

/**
 * @brief 攻击玩家目标
 *
 * 当被玩家攻击后攻击玩家。
 *
 * 参考 MC 1.16.5 HurtByTargetGoal
 */
class HurtByTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 生物实体
     */
    explicit HurtByTargetGoal(MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] String getTypeName() const override { return "HurtByTargetGoal"; }

private:
    MobEntity* m_mob;
    LivingEntity* m_attacker = nullptr;
    i32 m_timestamp = 0;
};

/**
 * @brief 最近目标目标
 *
 * 寻找并攻击最近的攻击目标。
 *
 * 参考 MC 1.16.5 NearestAttackableTargetGoal
 */
class NearestAttackableTargetGoal : public Goal {
public:
    /**
     * @brief 目标选择函数
     */
    using TargetSelector = std::function<bool(LivingEntity*)>;

    /**
     * @brief 构造函数
     * @param mob 生物实体
     * @param targetClass 目标类型名称（用于过滤）
     * @param checkSight 是否检查视线
     * @param nearbyOnly 是否只攻击附近
     */
    NearestAttackableTargetGoal(MobEntity* mob, const String& targetClass,
                                  bool checkSight = true, bool nearbyOnly = false);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] String getTypeName() const override { return "NearestAttackableTargetGoal"; }

    /**
     * @brief 设置目标选择器
     */
    void setTargetSelector(TargetSelector selector) { m_targetSelector = std::move(selector); }

protected:
    /**
     * @brief 寻找最近的攻击目标
     */
    [[nodiscard]] LivingEntity* findNearestTarget();

    MobEntity* m_mob;
    String m_targetClass;
    bool m_checkSight;
    bool m_nearbyOnly;
    LivingEntity* m_target = nullptr;
    TargetSelector m_targetSelector;
    i32 m_searchCooldown = 0;

    static constexpr i32 SEARCH_INTERVAL = 20;  // 搜索间隔
    static constexpr f32 TARGET_RANGE = 16.0f;  // 目标搜索范围
};

} // namespace entity::ai::goal
} // namespace mc
