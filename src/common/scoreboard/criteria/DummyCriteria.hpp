#pragma once

#include "../core/ScoreCriteria.hpp"

namespace mc::scoreboard {

/**
 * @brief 手动设置判据
 *
 * 最基础的判据类型，分数只能通过命令手动设置。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.DUMMY
 */
class DummyCriteria : public ScoreCriteria {
public:
    /// 判据名称
    static constexpr const char* NAME = "dummy";

    /**
     * @brief 构造函数
     */
    DummyCriteria();

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

private:
    std::string m_name;
};

} // namespace mc::scoreboard
