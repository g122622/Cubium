#pragma once

#include "TameableEntity.hpp"

namespace mc {

/**
 * @brief 可停在玩家肩膀上的可驯服实体
 *
 * 对齐 1.16.5 `ShoulderRidingEntity` 的最小职责，
 * 负责肩膀乘坐冷却和肩膀挂靠状态。
 */
class ShoulderRidingEntity : public TameableEntity {
public:
    /**
     * @brief 构造肩膀乘坐实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    ShoulderRidingEntity(LegacyEntityType type, EntityId id)
        : TameableEntity(type, id)
    {}

    ~ShoulderRidingEntity() override = default;

    ShoulderRidingEntity(const ShoulderRidingEntity&) = delete;
    ShoulderRidingEntity& operator=(const ShoulderRidingEntity&) = delete;
    ShoulderRidingEntity(ShoulderRidingEntity&&) = default;
    ShoulderRidingEntity& operator=(ShoulderRidingEntity&&) = default;

    /**
     * @brief 当前是否挂在玩家肩膀上
     */
    [[nodiscard]] bool isOnShoulder() const { return m_onShoulder; }

    /**
     * @brief 获取肩膀所属玩家 ID
     */
    [[nodiscard]] u64 getShoulderPlayerId() const { return m_shoulderPlayerId; }

    /**
     * @brief 当前是否满足停到肩膀上的冷却条件
     */
    [[nodiscard]] bool canSitOnShoulder() const { return m_rideCooldownCounter > 100; }

    /**
     * @brief 挂到玩家肩膀上
     */
    bool mountShoulder(u64 playerId)
    {
        if (!isTamed() || isSitting() || !canSitOnShoulder()) {
            return false;
        }

        m_onShoulder = true;
        m_shoulderPlayerId = playerId;
        return true;
    }

    /**
     * @brief 从肩膀上离开
     */
    void dismountShoulder()
    {
        m_onShoulder = false;
        m_shoulderPlayerId = 0;
        m_rideCooldownCounter = 0;
    }

    void tick() override
    {
        ++m_rideCooldownCounter;
        TameableEntity::tick();
    }

private:
    i32 m_rideCooldownCounter = 0;
    bool m_onShoulder = false;
    u64 m_shoulderPlayerId = 0;
};

} // namespace mc
