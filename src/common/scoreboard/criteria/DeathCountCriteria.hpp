#pragma once

#include "../core/ScoreCriteria.hpp"

namespace mc::scoreboard {

/**
 * @brief 死亡计数判据
 *
 * 当玩家死亡时自动增加分数。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.DEATH_COUNT
 */
class DeathCountCriteria : public ScoreCriteria {
public:
    /// 判据名称
    static constexpr const char* NAME = "deathCount";

    /**
     * @brief 构造函数
     */
    DeathCountCriteria();

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

    void onPlayerDeath(const std::string& playerName, Scoreboard& scoreboard) override;

private:
    std::string m_name;
};

} // namespace mc::scoreboard
