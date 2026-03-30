#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 掠夺者基类
 *
 * 灾厄村民的基类，参与袭击事件。
 *
 * 参考 MC 1.16.5 AbstractIllagerEntity
 */
class AbstractIllagerEntity : public MonsterEntity {
public:
    /**
     * @brief 灾厄村民状态
     */
    enum class IllagerState {
        PEACEFUL,    // 和平状态
        CELEBRATING, // 庆祝中
        RAIDING      // 袭击中
    };

    AbstractIllagerEntity(LegacyEntityType type, EntityId id);
    ~AbstractIllagerEntity() override = default;

    // ========== 灾厄村民状态 ==========

    [[nodiscard]] IllagerState getIllagerState() const { return m_illagerState; }
    void setIllagerState(IllagerState state) { m_illagerState = state; }

    [[nodiscard]] bool isCelebrating() const { return m_illagerState == IllagerState::CELEBRATING; }
    [[nodiscard]] bool isRaiding() const { return m_illagerState == IllagerState::RAIDING; }

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;

private:
    IllagerState m_illagerState = IllagerState::PEACEFUL;
};

/**
 * @brief 掠夺者基类（袭击者）
 *
 * 可加入袭击的生物基类。
 *
 * 参考 MC 1.16.5 AbstractRaiderEntity
 */
class AbstractRaiderEntity : public AbstractIllagerEntity {
public:
    AbstractRaiderEntity(LegacyEntityType type, EntityId id);
    ~AbstractRaiderEntity() override = default;

    // ========== 袭击相关 ==========

    [[nodiscard]] i32 getRaidWave() const { return m_raidWave; }
    void setRaidWave(i32 wave) { m_raidWave = wave; }

    [[nodiscard]] bool isRaidCaptain() const { return m_isRaidCaptain; }
    void setRaidCaptain(bool captain) { m_isRaidCaptain = captain; }

protected:
    void registerGoals() override;

private:
    i32 m_raidWave = 0;
    bool m_isRaidCaptain = false;
};

/**
 * @brief 掠夺者实体
 *
 * 使用弩进行远程攻击的灾厄村民。
 *
 * 参考 MC 1.16.5 PillagerEntity
 */
class PillagerEntity : public AbstractRaiderEntity {
public:
    PillagerEntity(LegacyEntityType type, EntityId id);
    ~PillagerEntity() override = default;

    // ========== 武器相关 ==========

    [[nodiscard]] bool isCharging() const { return m_isCharging; }
    void setCharging(bool charging) { m_isCharging = charging; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_isCharging = false;
};

/**
 * @brief 卫道士实体
 *
 * 手持铁斧进行近战攻击的灾厄村民。
 *
 * 参考 MC 1.16.5 VindicatorEntity
 */
class VindicatorEntity : public AbstractRaiderEntity {
public:
    VindicatorEntity(LegacyEntityType type, EntityId id);
    ~VindicatorEntity() override = default;

    [[nodiscard]] bool isAggressive() const { return m_aggressive; }
    void setAggressive(bool aggressive) { m_aggressive = aggressive; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_aggressive = false;
};

} // namespace mc
