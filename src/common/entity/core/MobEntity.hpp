#pragma once

#include "LivingEntity.hpp"
#include "../ai/goal/GoalSelector.hpp"
#include "../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

// 前向声明
namespace entity::ai::controller {
    class LookController;
    class MovementController;
    class JumpController;
}

namespace entity::ai {
    class EntitySenses;
}

namespace entity::ai::pathfinding {
    class PathNavigator;
}

/**
 * @brief Mob 实体基类
 *
 * 所有 AI 生物的基类，包括怪物和动物。
 * 提供 AI 目标系统、控制器、寻路等功能。
 *
 * 参考 MC 1.16.5 MobEntity
 */
class MobEntity : public LivingEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    MobEntity(LegacyEntityType type, EntityId id);

    ~MobEntity() override;

    // 禁止拷贝
    MobEntity(const MobEntity&) = delete;
    MobEntity& operator=(const MobEntity&) = delete;

    // 允许移动
    MobEntity(MobEntity&&) = default;
    MobEntity& operator=(MobEntity&&) = default;

    // ========== AI 目标系统 ==========

    /**
     * @brief 获取行为目标选择器
     */
    [[nodiscard]] entity::ai::GoalSelector& goalSelector() { return m_goalSelector; }
    [[nodiscard]] const entity::ai::GoalSelector& goalSelector() const { return m_goalSelector; }

    /**
     * @brief 获取目标选择器（攻击目标等）
     */
    [[nodiscard]] entity::ai::GoalSelector& targetSelector() { return m_targetSelector; }
    [[nodiscard]] const entity::ai::GoalSelector& targetSelector() const { return m_targetSelector; }

    /**
     * @brief 注册 AI 目标
     *
     * 子类应重写此方法来注册自己的 AI 目标。
     */
    virtual void registerGoals() {}

    /**
     * @brief 获取环境声音间隔
     */
    [[nodiscard]] virtual i32 getTalkInterval() const { return 80; }

    /**
     * @brief 播放环境声音
     */
    void playAmbientSound();

    /**
     * @brief 播放近战攻击声音
     */
    virtual void playAttackSound(LivingEntity& target);

    // ========== 控制器 ==========

    /**
     * @brief 获取视线控制器
     */
    [[nodiscard]] entity::ai::controller::LookController* lookController();
    [[nodiscard]] const entity::ai::controller::LookController* lookController() const;

    /**
     * @brief 获取移动控制器
     */
    [[nodiscard]] entity::ai::controller::MovementController* moveController();
    [[nodiscard]] const entity::ai::controller::MovementController* moveController() const;

    /**
     * @brief 获取跳跃控制器
     */
    [[nodiscard]] entity::ai::controller::JumpController* jumpController();
    [[nodiscard]] const entity::ai::controller::JumpController* jumpController() const;

    // ========== 目标 ==========

    /**
     * @brief 获取攻击目标
     */
    [[nodiscard]] LivingEntity* attackTarget() { return m_attackTarget; }
    [[nodiscard]] const LivingEntity* attackTarget() const { return m_attackTarget; }

    /**
     * @brief 设置攻击目标
     */
    void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    // ========== 刻更新 ==========

    void tick() override;

    // ========== AI 辅助方法 ==========

    /**
     * @brief 获取空闲时间
     */
    [[nodiscard]] i32 idleTime() const { return m_idleTime; }

    /**
     * @brief 设置空闲时间
     */
    void setIdleTime(i32 time) { m_idleTime = time; }

    /**
     * @brief 获取随机数生成器（基于实体ID和tick）
     */
    [[nodiscard]] math::Random getRandom() const;

    /**
     * @brief 检查是否被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const;

    /**
     * @brief 获取导航器
     */
    [[nodiscard]] entity::ai::pathfinding::PathNavigator* navigator();
    [[nodiscard]] const entity::ai::pathfinding::PathNavigator* navigator() const;
    [[nodiscard]] entity::ai::EntitySenses* senses();
    [[nodiscard]] const entity::ai::EntitySenses* senses() const;

    // ========== AI 便捷方法 ==========

    /**
     * @brief 获取水平面部旋转速度
     *
     * 参考 MC 1.16.5 MobEntity.getHorizontalFaceSpeed()
     * 用于LookController的默认旋转速度。
     */
    [[nodiscard]] virtual f32 getHorizontalFaceSpeed() const { return 10.0f; }

    /**
     * @brief 获取垂直面部旋转速度
     *
     * 参考 MC 1.16.5 MobEntity.getVerticalFaceSpeed()
     * 用于LookController的默认旋转速度。
     */
    [[nodiscard]] virtual f32 getVerticalFaceSpeed() const { return 10.0f; }

    /**
     * @brief 获取面部旋转速度
     *
     * 参考 MC 1.16.5 MobEntity.getFaceRotSpeed()
     * 用于LookController的默认偏航角旋转速度。
     */
    [[nodiscard]] virtual f32 getFaceRotSpeed() const { return 10.0f; }

    /**
     * @brief 清除导航路径
     *
     * 安全地清除导航器的路径，内部处理空指针检查。
     */
    void clearNavigation();

    /**
     * @brief 看向指定实体
     *
     * 使用视线控制器看向目标实体的眼睛位置。
     * @param target 目标实体
     * @param deltaYaw 最大偏航角变化速度（默认10）
     * @param deltaPitch 最大俯仰角变化速度（默认10）
     */
    void lookAt(const Entity& target, f32 deltaYaw = 10.0f, f32 deltaPitch = 10.0f);

    /**
     * @brief 看向指定位置
     *
     * 使用视线控制器看向指定位置。
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param deltaYaw 最大偏航角变化速度（默认10）
     * @param deltaPitch 最大俯仰角变化速度（默认10）
     */
    void lookAt(f64 x, f64 y, f64 z, f32 deltaYaw = 10.0f, f32 deltaPitch = 10.0f);

    // ========== 经验值 ==========

    /**
     * @brief 获取经验值
     *
     * 死亡时掉落的经验值数量。
     */
    [[nodiscard]] i32 experienceValue() const { return m_experienceValue; }

    /**
     * @brief 设置经验值
     * @param value 经验值
     */
    void setExperienceValue(i32 value) { m_experienceValue = value; }

    // ========== 掉落 ==========

    /**
     * @brief 掉落经验
     *
     * 重写 LivingEntity::dropExperience()，在死亡时生成经验球。
     */
    void dropExperience() override;

protected:
    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getAmbientSound() const;

    void playHurtSound(DamageSource& source) override;

    // AI 目标选择器
    entity::ai::GoalSelector m_goalSelector;
    entity::ai::GoalSelector m_targetSelector;

    // 控制器
    std::unique_ptr<entity::ai::controller::LookController> m_lookController;
    std::unique_ptr<entity::ai::controller::MovementController> m_moveController;
    std::unique_ptr<entity::ai::controller::JumpController> m_jumpController;
    std::unique_ptr<entity::ai::EntitySenses> m_senses;

    // 寻路器
    std::unique_ptr<entity::ai::pathfinding::PathNavigator> m_navigator;

    // 攻击目标
    LivingEntity* m_attackTarget = nullptr;

    // AI 状态
    bool m_aiEnabled = true;
    i32 m_idleTime = 0;  // 空闲时间（用于随机漫步等）
    i32 m_livingSoundTime = 0;

    // 经验值（死亡时掉落）
    i32 m_experienceValue = 0;
};

} // namespace mc
