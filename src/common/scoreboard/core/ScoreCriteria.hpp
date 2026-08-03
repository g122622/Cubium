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

#include "ScoreCriteriaRenderType.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::scoreboard {

// 前向声明
class Score;
class Scoreboard;

/**
 * @brief 判据基类
 *
 * 所有记分板判据的抽象基类。判据定义了分数的来源和更新方式。
 *
 * 内置判据类型：
 * - dummy: 手动设置分数
 * - trigger: 玩家可触发（需管理员 enable）
 * - deathCount: 死亡计数
 * - playerKillCount: 击杀玩家计数
 * - totalKillCount: 总击杀计数
 * - health: 生命值（只读）
 * - food: 饥饿值（只读）
 * - air: 氧气值（只读）
 * - armor: 护甲值（只读）
 * - xp: 经验值（只读）
 * - level: 等级（只读）
 * - teamkill.{color}: 队伍击杀
 * - killedByTeam.{color}: 被队伍击杀
 */
class ScoreCriteria {
public:
    virtual ~ScoreCriteria() = default;

    /**
     * @brief 获取判据名称
     *
     * 名称用于命令中引用判据，如 "dummy"、"deathCount" 等。
     *
     * @return 判据名称
     */
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;

    /**
     * @brief 判断是否为只读判据
     *
     * 只读判据的分数不能通过命令修改，只能由游戏自动更新。
     * 例如 health、food、air 等判据是只读的。
     *
     * @return true 如果是只读判据
     */
    [[nodiscard]] virtual bool isReadOnly() const noexcept { return false; }

    /**
     * @brief 获取默认渲染类型
     *
     * 定义使用此判据的目标默认如何显示分数。
     * 例如 health 判据默认使用 Hearts 渲染类型。
     *
     * @return 默认渲染类型
     */
    [[nodiscard]] virtual RenderType getDefaultRenderType() const noexcept { return RenderType::Integer; }

    /**
     * @brief 分数变更时调用
     *
     * 当分数被修改时调用此方法。子类可以覆盖此方法来自动更新其他判据。
     * 只读判据应忽略此调用。
     *
     * @param score 分数对象
     * @param oldScore 变更前的分数值
     */
    virtual void onScoreChanged(Score& score, i32 oldScore)
    {
        MC_UNUSED(score);
        MC_UNUSED(oldScore);
    }

    /**
     * @brief 玩家死亡时调用
     *
     * 当玩家死亡时，此方法会被调用以更新相关判据的分数。
     * 默认实现不执行任何操作。
     *
     * @param playerName 玩家名称
     * @param scoreboard 记分板
     */
    virtual void onPlayerDeath(const std::string& playerName, Scoreboard& scoreboard)
    {
        MC_UNUSED(playerName);
        MC_UNUSED(scoreboard);
    }

    /**
     * @brief 玩家击杀实体时调用
     *
     * 当玩家击杀实体时，此方法会被调用以更新相关判据的分数。
     *
     * @param playerName 玩家名称
     * @param victimType 受害者实体类型
     * @param isPlayer 受害者是否为玩家
     * @param scoreboard 记分板
     */
    virtual void onPlayerKill(
        const std::string& playerName, const std::string& victimType, bool isPlayer, Scoreboard& scoreboard)
    {
        MC_UNUSED(playerName);
        MC_UNUSED(victimType);
        MC_UNUSED(isPlayer);
        MC_UNUSED(scoreboard);
    }

    /**
     * @brief 玩家被击杀时调用
     *
     * 当玩家被实体击杀时，此方法会被调用。
     *
     * @param playerName 玩家名称
     * @param killerType 击杀者实体类型
     * @param isPlayer 击杀者是否为玩家
     * @param scoreboard 记分板
     */
    virtual void onPlayerKilled(
        const std::string& playerName, const std::string& killerType, bool isPlayer, Scoreboard& scoreboard)
    {
        MC_UNUSED(playerName);
        MC_UNUSED(killerType);
        MC_UNUSED(isPlayer);
        MC_UNUSED(scoreboard);
    }

    /**
     * @brief 玩家 Tick 时调用
     *
     * 每游戏刻调用，用于更新只读判据（如 health、food 等）。
     *
     * @param playerName 玩家名称
     * @param scoreboard 记分板
     */
    virtual void onPlayerTick(const std::string& playerName, Scoreboard& scoreboard)
    {
        MC_UNUSED(playerName);
        MC_UNUSED(scoreboard);
    }

protected:
    /**
     * @brief 构造函数
     *
     * 子类调用此构造函数来初始化判据。
     */
    ScoreCriteria() = default;
};

/**
 * @brief 判据注册表
 *
 * 管理所有判据类型的注册和查找。
 * 使用单例模式。
 *
 * 使用示例：
 * @code
 * // 初始化时注册内置判据
 * ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
 *
 * // 获取判据
 * auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
 *
 * // 列出所有判据
 * auto names = ScoreCriteriaRegistry::instance().getAllCriteriaNames();
 * @endcode
 */
class ScoreCriteriaRegistry {
public:
    /**
     * @brief 获取单例实例
     *
     * @return 注册表实例引用
     */
    static ScoreCriteriaRegistry& instance();

    // 禁止拷贝
    ScoreCriteriaRegistry(const ScoreCriteriaRegistry&) = delete;
    ScoreCriteriaRegistry& operator=(const ScoreCriteriaRegistry&) = delete;

    /**
     * @brief 注册判据
     *
     * @param criteria 判据实例
     * @return 成功或错误（如果判据名称已存在）
     */
    Result<void> registerCriteria(std::unique_ptr<ScoreCriteria> criteria);

    /**
     * @brief 获取判据
     *
     * @param name 判据名称
     * @return 判据指针，如果不存在返回 nullptr
     */
    [[nodiscard]] ScoreCriteria* getCriteria(const std::string& name);

    /**
     * @brief 获取判据（const 版本）
     *
     * @param name 判据名称
     * @return 判据指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const ScoreCriteria* getCriteria(const std::string& name) const;

    /**
     * @brief 检查判据是否存在
     *
     * @param name 判据名称
     * @return true 如果存在
     */
    [[nodiscard]] bool hasCriteria(const std::string& name) const;

    /**
     * @brief 获取所有判据名称
     *
     * @return 判据名称列表
     */
    [[nodiscard]] std::vector<std::string> getAllCriteriaNames() const;

    /**
     * @brief 注册所有内置判据
     *
     * 包括：dummy、trigger、deathCount、playerKillCount、totalKillCount、
     * health、food、air、armor、xp、level、teamkill.*、killedByTeam.*
     */
    void registerBuiltinCriteria();

    /**
     * @brief 清空所有判据
     *
     * 用于测试或重新初始化。
     */
    void clear();

private:
    ScoreCriteriaRegistry() = default;

    std::unordered_map<std::string, std::unique_ptr<ScoreCriteria>> m_criteria;
};

} // namespace mc::scoreboard
