#pragma once

#include "../core/ScoreCriteria.hpp"

namespace mc::scoreboard {

/**
 * @brief 击杀计数判据基类
 *
 * 当玩家击杀实体时自动增加分数。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria
 */
class KillCountCriteria : public ScoreCriteria {
public:
    /**
     * @brief 构造函数
     *
     * @param name 判据名称
     * @param playersOnly 是否只统计击杀玩家
     */
    KillCountCriteria(const std::string& name, bool playersOnly);

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

    void onPlayerKill(
        const std::string& playerName, const std::string& victimType, bool isPlayer, Scoreboard& scoreboard) override;

protected:
    std::string m_name;
    bool m_playersOnly;
};

/**
 * @brief 玩家击杀计数判据
 *
 * 当玩家击杀其他玩家时自动增加分数。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.PLAYER_KILL_COUNT
 */
class PlayerKillCountCriteria : public KillCountCriteria {
public:
    static constexpr const char* NAME = "playerKillCount";

    PlayerKillCountCriteria();
};

/**
 * @brief 总击杀计数判据
 *
 * 当玩家击杀任何实体时自动增加分数。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.TOTAL_KILL_COUNT
 */
class TotalKillCountCriteria : public KillCountCriteria {
public:
    static constexpr const char* NAME = "totalKillCount";

    TotalKillCountCriteria();
};

} // namespace mc::scoreboard
