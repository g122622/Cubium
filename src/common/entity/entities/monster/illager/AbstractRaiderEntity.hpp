#pragma once

#include "PatrollerEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../entity/damage/DamageSource.hpp"

namespace mc {
namespace world::village::raid {
class Raid;
}

/**
 * @brief 袭击者抽象基类。
 *
 * 为所有可参与村庄袭击的实体提供统一的 Raid 关联与庆祝状态管理。
 */
class AbstractRaiderEntity : public PatrollerEntity {
public:
    /**
     * @brief 构造袭击者基类。
     *
     * @param type 实体类型。
     * @param id 实体 ID。
     */
    AbstractRaiderEntity(LegacyEntityType type, EntityId id);

    ~AbstractRaiderEntity() override = default;
    AbstractRaiderEntity(const AbstractRaiderEntity&) = delete;
    AbstractRaiderEntity& operator=(const AbstractRaiderEntity&) = delete;
    AbstractRaiderEntity(AbstractRaiderEntity&&) = default;
    AbstractRaiderEntity& operator=(AbstractRaiderEntity&&) = default;

    /**
     * @brief 判断是否拥有袭击队长加成。
     */
    [[nodiscard]] bool hasRaidLeaderBonus() const { return m_hasLeaderBonus; }

    /**
     * @brief 设置袭击队长加成状态。
     *
     * @param bonus 是否拥有加成。
     */
    void setRaidLeaderBonus(bool bonus) { m_hasLeaderBonus = bonus; }

    /**
     * @brief 判断是否允许加入袭击。
     */
    [[nodiscard]] bool canJoinRaid() const { return m_canJoinRaid; }

    /**
     * @brief 设置是否允许加入袭击。
     *
     * @param canJoin 是否允许加入。
     */
    void setCanJoinRaid(bool canJoin) { m_canJoinRaid = canJoin; }

    /**
     * @brief 获取当前所属袭击。
     *
     * @return 当前 Raid 指针；若未加入则返回 `nullptr`。
     */
    [[nodiscard]] world::village::raid::Raid* getCurrentRaid() const { return m_raid; }

    /**
     * @brief 加入指定袭击。
     *
     * @param raid 目标袭击。
     * @param wave 当前波次。
     */
    void joinRaid(world::village::raid::Raid* raid, i32 wave);

    /**
     * @brief 离开当前袭击。
     */
    void leaveRaid();

    /**
     * @brief 获取当前所属波次。
     */
    [[nodiscard]] i32 getRaidWave() const { return m_wave; }

    /**
     * @brief 设置当前所属波次。
     *
     * @param wave 波次编号。
     */
    void setRaidWave(i32 wave) { m_wave = wave; }

    /**
     * @brief 获取庆祝剩余时间。
     */
    [[nodiscard]] i32 getCelebrationTime() const { return m_celebrationTime; }

    /**
     * @brief 进入庆祝状态。
     *
     * @note 仅更新本地状态，不会自动广播动画或音效。
     */
    void startCelebrating();

    /**
     * @brief 执行实体 tick。
     *
     * @note 会在常规巡逻者逻辑后更新 Raid 关联状态。
     */
    void tick() override;

    /**
     * @brief 处理死亡。
     *
     * @param cause 伤害来源。
     *
     * @warning 调用顺序很重要，必须先通知 Raid，再交给父类完成死亡流程。
     */
    void die(DamageSource& cause) override;

protected:
    bool m_hasLeaderBonus = false;
    bool m_canJoinRaid = true;
    i32 m_celebrationTime = 0;
    world::village::raid::Raid* m_raid = nullptr;
    i32 m_wave = 0;

    static constexpr i32 CELEBRATION_DURATION = 200;
};

} // namespace mc
