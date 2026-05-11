#pragma once

#include "../core/ScoreCriteria.hpp"

namespace mc::scoreboard {

/**
 * @brief 触发器判据
 *
 * 允许玩家通过 /trigger 命令触发分数变更。
 * 玩家触发后分数会被锁定，需要管理员重新 enable。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.TRIGGER
 */
class TriggerCriteria : public ScoreCriteria {
public:
    /// 判据名称
    static constexpr const char* NAME = "trigger";

    /**
     * @brief 构造函数
     */
    TriggerCriteria();

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

private:
    std::string m_name;
};

} // namespace mc::scoreboard
