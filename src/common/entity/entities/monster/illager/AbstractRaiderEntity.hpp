#pragma once

#include "AbstractIllagerEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 掠夺者抽象基类
 *
 * 参与掠夺事件的敌对生物的共同基类。
 *
 * 特性：
 * - 参与掠夺事件
 * - 有掠夺ID和波次信息
 * - 胜利时庆祝
 *
 * 参考 MC 1.16.5 AbstractRaiderEntity
 */
class AbstractRaiderEntity : public AbstractIllagerEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractRaiderEntity(LegacyEntityType type, EntityId id);
    ~AbstractRaiderEntity() override = default;

    // 禁止拷贝
    AbstractRaiderEntity(const AbstractRaiderEntity&) = delete;
    AbstractRaiderEntity& operator=(const AbstractRaiderEntity&) = delete;

    // 允许移动
    AbstractRaiderEntity(AbstractRaiderEntity&&) = default;
    AbstractRaiderEntity& operator=(AbstractRaiderEntity&&) = default;

    // ========== 掠夺系统 ==========

    /**
     * @brief 是否有掠夺首领奖励
     */
    [[nodiscard]] bool hasRaidLeaderBonus() const { return m_hasLeaderBonus; }

    /**
     * @brief 设置掠夺首领奖励
     */
    void setRaidLeaderBonus(bool bonus) { m_hasLeaderBonus = bonus; }

    /**
     * @brief 是否可以加入掠夺
     */
    [[nodiscard]] bool canJoinRaid() const { return m_canJoinRaid; }

    /**
     * @brief 设置是否可以加入掠夺
     */
    void setCanJoinRaid(bool canJoin) { m_canJoinRaid = canJoin; }

    /**
     * @brief 获取庆祝时间
     */
    [[nodiscard]] i32 getCelebrationTime() const { return m_celebrationTime; }

    /**
     * @brief 开始庆祝
     */
    void startCelebrating();

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // 掠夺相关
    bool m_hasLeaderBonus = false;
    bool m_canJoinRaid = true;
    i32 m_celebrationTime = 0;

    // 常量
    static constexpr i32 CELEBRATION_DURATION = 200;
};

} // namespace mc
