#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../interfaces/IAngerable.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../util/assert/AssertMacros.hpp"
#include <optional>

namespace mc {

// Forward declarations
class Player;

/**
 * @brief 可驯服实体基类
 *
 * 支持被玩家驯服的动物实体基类。
 * 狼、猫、鹦鹉等可驯服动物继承此类。
 *
 * 特性：
 * - 驯服状态（是否被驯服）
 * - 主人ID（驯服后的玩家）
 * - 坐下/站起
 * - 愤怒系统（攻击目标追踪）
 * - 跟随主人行为
 *
 * 参考 MC 1.16.5 TameableEntity
 */
class TameableEntity : public AnimalEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    TameableEntity(LegacyEntityType type, EntityId id);
    ~TameableEntity() override = default;

    // 禁止拷贝
    TameableEntity(const TameableEntity&) = delete;
    TameableEntity& operator=(const TameableEntity&) = delete;

    // 允许移动
    TameableEntity(TameableEntity&&) = default;
    TameableEntity& operator=(TameableEntity&&) = default;

    // ========== 驯服系统 ==========

    /**
     * @brief 检查是否已被驯服
     * @return 如果已被驯服返回true
     */
    [[nodiscard]] bool isTamed() const { return m_tamed; }

    /**
     * @brief 设置驯服状态
     * @param tamed 是否驯服
     *
     * 驯服后会更新实体的AI行为
     */
    virtual void setTamed(bool tamed);

    /**
     * @brief 获取主人ID
     * @return 主人的玩家ID，如果没有主人返回空
     */
    [[nodiscard]] std::optional<u64> getOwnerId() const { return m_ownerId; }

    /**
     * @brief 设置主人ID
     * @param ownerId 主人的玩家ID
     */
    void setOwnerId(u64 ownerId) { m_ownerId = ownerId; }

    /**
     * @brief 清除主人
     */
    void clearOwner() { m_ownerId = std::nullopt; }

    /**
     * @brief 检查指定玩家是否是主人
     * @param playerId 玩家ID
     * @return 如果是主人返回true
     */
    [[nodiscard]] bool isOwner(u64 playerId) const {
        return m_ownerId.has_value() && m_ownerId.value() == playerId;
    }

    /**
     * @brief 获取主人实体
     *
     * MC 1.16.5: TameableEntity.getOwner()
     * 通过主人ID在世界中查找玩家实体。
     * @return 主人实体指针，如果未找到或无主人返回nullptr
     */
    [[nodiscard]] Player* getOwner() const;

    // ========== 坐下/站起 ==========

    /**
     * @brief 检查是否坐下
     * @return 如果坐下返回true
     */
    [[nodiscard]] bool isSitting() const { return m_sitting; }

    /**
     * @brief 设置坐下状态
     * @param sitting 是否坐下
     */
    void setSitting(bool sitting);

    /**
     * @brief 切换坐下状态
     */
    void toggleSitting() { setSitting(!m_sitting); }

    // ========== IAngerable 接口实现 ==========

    void setAttackTarget(LivingEntity* target) override;
    [[nodiscard]] LivingEntity* getAttackTarget() const override { return m_attackTarget; }
    void setRevengeTarget(LivingEntity* target) override;
    [[nodiscard]] bool isAngry() const override { return m_angerTime > 0; }
    void setAngry(bool angry) override;
    [[nodiscard]] i32 getAngerTime() const override { return m_angerTime; }
    void setAngerTime(i32 time) override { m_angerTime = time; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    /**
     * @brief 注册 AI 目标
     *
     * 子类应调用此方法来注册驯服动物的基础行为：
     * - SwimGoal (优先级 0)
     * - SitGoal (优先级 1, 驯服后)
     * - BreedGoal (优先级 2)
     * - FollowOwnerGoal (优先级 3, 驯服后)
     * - TemptGoal (优先级 4)
     * - FollowParentGoal (优先级 5)
     * - WaterAvoidingRandomWalkingGoal (优先级 6)
     * - LookAtGoal (优先级 7)
     * - LookRandomlyGoal (优先级 8)
     */
    void registerGoals() override;

    /**
     * @brief 注册属性
     *
     * 注册驯服动物的基础属性。
     */
    void registerAttributes() override;

    /**
     * @brief 更新愤怒状态
     */
    void updateAnger() override;

    /**
     * @brief 当驯服状态改变时调用
     * @param tamed 是否驯服
     *
     * 子类可重写此方法来处理驯服状态变化的副作用
     */
    virtual void onTamed(bool tamed) { MC_UNUSED(tamed); }

private:
    // 驯服状态
    bool m_tamed = false;
    bool m_sitting = false;
    std::optional<u64> m_ownerId;

    // 愤怒系统
    LivingEntity* m_attackTarget = nullptr;
    i32 m_angerTime = 0;
    std::optional<u64> m_revengeTargetId;

    // 常量
    static constexpr i32 MAX_ANGER_TIME = 600; // 30秒
};

} // namespace mc
