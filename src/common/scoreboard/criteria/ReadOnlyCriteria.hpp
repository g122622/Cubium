#pragma once

#include "../core/ScoreCriteria.hpp"
#include "../core/ScoreCriteriaRenderType.hpp"

namespace mc::scoreboard {

/**
 * @brief 只读判据基类
 *
 * 只读判据的分数由游戏自动更新，不能通过命令手动设置。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria
 */
class ReadOnlyCriteria : public ScoreCriteria {
public:
    /**
     * @brief 构造函数
     *
     * @param name 判据名称
     * @param renderType 默认渲染类型
     */
    ReadOnlyCriteria(const std::string& name, RenderType renderType);

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return true; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return m_renderType; }

protected:
    std::string m_name;
    RenderType m_renderType;
};

/**
 * @brief 生命值判据
 *
 * 显示玩家的当前生命值（心）。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.HEALTH
 */
class HealthCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "health";

    HealthCriteria();
};

/**
 * @brief 饥饿值判据
 *
 * 显示玩家的当前饥饿值。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.FOOD
 */
class FoodCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "food";

    FoodCriteria();
};

/**
 * @brief 氧气值判据
 *
 * 显示玩家的当前氧气值。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.AIR
 */
class AirCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "air";

    AirCriteria();
};

/**
 * @brief 护甲值判据
 *
 * 显示玩家的当前护甲值。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.ARMOR
 */
class ArmorCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "armor";

    ArmorCriteria();
};

/**
 * @brief 经验值判据
 *
 * 显示玩家的当前经验值。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.XP
 */
class XpCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "xp";

    XpCriteria();
};

/**
 * @brief 等级判据
 *
 * 显示玩家的当前等级。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreCriteria.LEVEL
 */
class LevelCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "level";

    LevelCriteria();
};

} // namespace mc::scoreboard
