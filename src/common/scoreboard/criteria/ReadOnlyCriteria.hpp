/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include <string>

namespace mc::scoreboard {

/**
 * @brief 只读判据基类
 *
 * 只读判据的分数由游戏自动更新，不能通过命令手动设置。
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
 */
class LevelCriteria : public ReadOnlyCriteria {
public:
    static constexpr const char* NAME = "level";

    LevelCriteria();
};

} // namespace mc::scoreboard
