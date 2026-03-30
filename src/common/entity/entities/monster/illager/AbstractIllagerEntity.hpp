#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 灾厄村民抽象基类
 *
 * 灾厄村民（Illager）的共同基类，包括掠夺者、唤魔者、幻术师等。
 *
 * 特性：
 * - 敌对玩家和村民
 * - 参与掠夺事件
 * - 有团队协作能力
 *
 * 参考 MC 1.16.5 AbstractIllagerEntity
 */
class AbstractIllagerEntity : public MonsterEntity {
public:
    /**
     * @brief 灾厄村民状态
     */
    enum class IllagerState : u8 {
        Neutral = 0,    // 中立
        Aggressive = 1, // 攻击
        Celebrating = 2 // 庆祝
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractIllagerEntity(LegacyEntityType type, EntityId id);
    ~AbstractIllagerEntity() override = default;

    // 禁止拷贝
    AbstractIllagerEntity(const AbstractIllagerEntity&) = delete;
    AbstractIllagerEntity& operator=(const AbstractIllagerEntity&) = delete;

    // 允许移动
    AbstractIllagerEntity(AbstractIllagerEntity&&) = default;
    AbstractIllagerEntity& operator=(AbstractIllagerEntity&&) = default;

    // ========== 状态系统 ==========

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] IllagerState getState() const { return m_state; }

    /**
     * @brief 设置状态
     */
    void setState(IllagerState state) { m_state = state; }

    /**
     * @brief 是否正在庆祝
     */
    [[nodiscard]] bool isCelebrating() const { return m_state == IllagerState::Celebrating; }

    // ========== 掠夺系统 ==========

    /**
     * @brief 是否参与掠夺
     */
    [[nodiscard]] bool isRaidActive() const { return m_raidActive; }

    /**
     * @brief 设置掠夺状态
     */
    void setRaidActive(bool active) { m_raidActive = active; }

    /**
     * @brief 获取掠夺波次
     */
    [[nodiscard]] i32 getRaidWave() const { return m_raidWave; }

    /**
     * @brief 设置掠夺波次
     */
    void setRaidWave(i32 wave) { m_raidWave = wave; }

protected:
    // 状态
    IllagerState m_state = IllagerState::Neutral;

    // 掠夺相关
    bool m_raidActive = false;
    i32 m_raidWave = 0;
};

} // namespace mc
