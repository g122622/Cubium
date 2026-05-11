#pragma once

#include "ScoreCriteriaRenderType.hpp"
#include "ScoreObjective.hpp"
#include "Score.hpp"
#include "ScorePlayerTeam.hpp"
#include "../../util/text/ITextComponentFwd.hpp"
#include "../../core/Result.hpp"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <array>
#include <functional>

namespace mc::scoreboard {

/**
 * @brief 分数比较器
 *
 * 用于对分数进行排序。按分数降序排列，分数相同则按名称升序排列。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.Score.SCORE_COMPARATOR
 */
struct ScoreComparator {
    bool operator()(const Score* a, const Score* b) const;
};

/**
 * @brief 记分板核心类
 *
 * 管理目标、分数和队伍。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.Scoreboard
 *
 * 这是客户端和服务端共用的基类。服务端应使用 ServerScoreboard 子类，
 * 它添加了网络同步功能。
 *
 * 使用示例：
 * @code
 * Scoreboard scoreboard;
 *
 * // 创建目标
 * auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
 * auto* objective = scoreboard.addObjective("kills", *criteria,
 *     std::make_unique<StringTextComponent>("Kills"));
 *
 * // 设置分数
 * auto* score = scoreboard.getOrCreateScore("Steve", *objective);
 * score->setScorePoints(10);
 *
 * // 创建队伍
 * auto* team = scoreboard.createTeam("red");
 * team->addMember("Steve");
 * team->setColor(TextFormatting::Red);
 *
 * // 设置显示槽位
 * scoreboard.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, objective);
 * @endcode
 */
class Scoreboard {
public:
    /**
     * @brief 构造函数
     */
    Scoreboard();

    /**
     * @brief 析构函数
     */
    virtual ~Scoreboard() = default;

    // 禁止拷贝
    Scoreboard(const Scoreboard&) = delete;
    Scoreboard& operator=(const Scoreboard&) = delete;

    // 允许移动
    Scoreboard(Scoreboard&&) noexcept = default;
    Scoreboard& operator=(Scoreboard&&) noexcept = default;

    // ========== 目标管理 ==========

    /**
     * @brief 添加新目标
     *
     * @param name 目标名称（最大16字符）
     * @param criteria 判据
     * @param displayName 显示名称（可为空，默认使用目标名称）
     * @return 创建的目标指针，如果目标名称已存在返回 nullptr
     */
    ScoreObjective* addObjective(const std::string& name,
                                  ScoreCriteria& criteria,
                                  std::unique_ptr<text::ITextComponent> displayName = nullptr);

    /**
     * @brief 获取目标
     *
     * @param name 目标名称
     * @return 目标指针，如果不存在返回 nullptr
     */
    [[nodiscard]] ScoreObjective* getObjective(const std::string& name);

    /**
     * @brief 获取目标（const 版本）
     *
     * @param name 目标名称
     * @return 目标指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const ScoreObjective* getObjective(const std::string& name) const;

    /**
     * @brief 检查目标是否存在
     *
     * @param name 目标名称
     * @return true 如果存在
     */
    [[nodiscard]] bool hasObjective(const std::string& name) const;

    /**
     * @brief 移除目标
     *
     * 移除目标时，会同时移除所有相关的分数。
     *
     * @param objective 目标引用
     */
    void removeObjective(ScoreObjective& objective);

    /**
     * @brief 获取所有目标
     *
     * @return 目标指针列表
     */
    [[nodiscard]] std::vector<ScoreObjective*> getObjectives();

    /**
     * @brief 获取所有目标（const 版本）
     *
     * @return 目标指针列表
     */
    [[nodiscard]] std::vector<const ScoreObjective*> getObjectives() const;

    /**
     * @brief 获取指定判据的所有目标
     *
     * @param criteria 判据
     * @return 目标指针列表
     */
    [[nodiscard]] std::vector<ScoreObjective*> getObjectivesByCriteria(ScoreCriteria& criteria);

    // ========== 分数管理 ==========

    /**
     * @brief 获取或创建分数
     *
     * 如果分数不存在，会自动创建。玩家/实体名称最大 40 字符。
     *
     * @param playerName 玩家/实体名称
     * @param objective 目标
     * @return 分数对象指针
     */
    Score* getOrCreateScore(const std::string& playerName, ScoreObjective& objective);

    /**
     * @brief 获取分数
     *
     * @param playerName 玩家/实体名称
     * @param objective 目标
     * @return 分数对象指针，如果不存在返回 nullptr
     */
    [[nodiscard]] Score* getScore(const std::string& playerName, ScoreObjective& objective);

    /**
     * @brief 获取分数（const 版本）
     *
     * @param playerName 玩家/实体名称
     * @param objective 目标
     * @return 分数对象指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const Score* getScore(const std::string& playerName,
                                         ScoreObjective& objective) const;

    /**
     * @brief 移除分数
     *
     * @param playerName 玩家/实体名称
     * @param objective 目标（如果为 nullptr，移除该玩家的所有分数）
     */
    void removeScore(const std::string& playerName, ScoreObjective* objective = nullptr);

    /**
     * @brief 检查玩家是否在目标上有分数
     *
     * @param playerName 玩家/实体名称
     * @param objective 目标
     * @return true 如果有分数
     */
    [[nodiscard]] bool entityHasObjective(const std::string& playerName,
                                           ScoreObjective& objective) const;

    /**
     * @brief 获取目标的所有分数（按分数排序）
     *
     * @param objective 目标
     * @return 排序后的分数列表
     */
    [[nodiscard]] std::vector<Score*> getSortedScores(ScoreObjective& objective);

    /**
     * @brief 获取玩家的所有目标名称
     *
     * @param playerName 玩家/实体名称
     * @return 目标名称列表
     */
    [[nodiscard]] std::vector<std::string> getPlayerObjectives(const std::string& playerName) const;

    // ========== 显示槽位 ==========

    /**
     * @brief 设置显示槽位的目标
     *
     * @param slot 显示槽位
     * @param objective 目标指针（nullptr 表示清除）
     */
    void setObjectiveInDisplaySlot(DisplaySlot slot, ScoreObjective* objective);

    /**
     * @brief 获取显示槽位的目标
     *
     * @param slot 显示槽位
     * @return 目标指针，如果未设置返回 nullptr
     */
    [[nodiscard]] ScoreObjective* getObjectiveInDisplaySlot(DisplaySlot slot);

    /**
     * @brief 获取显示槽位的目标（const 版本）
     *
     * @param slot 显示槽位
     * @return 目标指针，如果未设置返回 nullptr
     */
    [[nodiscard]] const ScoreObjective* getObjectiveInDisplaySlot(DisplaySlot slot) const;

    /**
     * @brief 获取目标所在的显示槽位
     *
     * @param objective 目标
     * @return 显示槽位，如果未在任何槽位显示返回空
     */
    [[nodiscard]] std::vector<DisplaySlot> getDisplaySlotsForObject(ScoreObjective& objective) const;

    // ========== 队伍管理 ==========

    /**
     * @brief 创建队伍
     *
     * @param name 队伍名称（最大16字符）
     * @return 创建的队伍指针，如果队伍名称已存在返回 nullptr
     */
    ScorePlayerTeam* createTeam(const std::string& name);

    /**
     * @brief 移除队伍
     *
     * @param team 队伍引用
     */
    void removeTeam(ScorePlayerTeam& team);

    /**
     * @brief 获取队伍
     *
     * @param name 队伍名称
     * @return 队伍指针，如果不存在返回 nullptr
     */
    [[nodiscard]] ScorePlayerTeam* getTeam(const std::string& name);

    /**
     * @brief 获取队伍（const 版本）
     *
     * @param name 队伍名称
     * @return 队伍指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const ScorePlayerTeam* getTeam(const std::string& name) const;

    /**
     * @brief 检查队伍是否存在
     *
     * @param name 队伍名称
     * @return true 如果存在
     */
    [[nodiscard]] bool hasTeam(const std::string& name) const;

    /**
     * @brief 将玩家加入队伍
     *
     * 如果玩家已经在其他队伍，会先离开原队伍。
     *
     * @param playerName 玩家名称
     * @param team 队伍
     * @return true 如果成功加入
     */
    bool addPlayerToTeam(const std::string& playerName, ScorePlayerTeam& team);

    /**
     * @brief 将玩家移出队伍
     *
     * @param playerName 玩家名称
     * @param team 队伍
     * @return true 如果成功移出
     */
    bool removePlayerFromTeam(const std::string& playerName, ScorePlayerTeam& team);

    /**
     * @brief 获取玩家所在的队伍
     *
     * @param playerName 玩家名称
     * @return 队伍指针，如果玩家不在任何队伍返回 nullptr
     */
    [[nodiscard]] ScorePlayerTeam* getPlayersTeam(const std::string& playerName) const;

    /**
     * @brief 获取所有队伍
     *
     * @return 队伍指针列表
     */
    [[nodiscard]] std::vector<ScorePlayerTeam*> getTeams();

    /**
     * @brief 获取所有队伍（const 版本）
     *
     * @return 队伍指针列表
     */
    [[nodiscard]] std::vector<const ScorePlayerTeam*> getTeams() const;

    // ========== 回调（子类重写）==========

    /**
     * @brief 目标添加时调用
     */
    virtual void onObjectiveAdded(ScoreObjective& objective);

    /**
     * @brief 目标移除时调用
     */
    virtual void onObjectiveRemoved(ScoreObjective& objective);

    /**
     * @brief 目标变更时调用
     */
    virtual void onObjectiveChanged(ScoreObjective& objective);

    /**
     * @brief 分数变更时调用
     */
    virtual void onScoreChanged(Score& score);

    /**
     * @brief 分数移除时调用
     */
    virtual void onScoreRemoved(Score& score);

    /**
     * @brief 玩家分数移除时调用
     */
    virtual void onPlayerRemoved(const std::string& playerName);

    /**
     * @brief 玩家分数重置时调用
     */
    virtual void onPlayerScoreRemoved(const std::string& playerName, ScoreObjective& objective);

    /**
     * @brief 队伍添加时调用
     */
    virtual void onTeamAdded(ScorePlayerTeam& team);

    /**
     * @brief 队伍变更时调用
     */
    virtual void onTeamChanged(ScorePlayerTeam& team);

    /**
     * @brief 队伍移除时调用
     */
    virtual void onTeamRemoved(ScorePlayerTeam& team);

    /**
     * @brief 显示槽位变更时调用
     */
    virtual void onDisplaySlotChanged(DisplaySlot slot, ScoreObjective* objective);

    // ========== 批判据查询 ==========

    /**
     * @brief 对所有使用指定判据的目标执行操作
     *
     * @param criteria 判据
     * @param playerName 玩家名称
     * @param action 操作函数
     */
    void forAllObjectives(ScoreCriteria& criteria,
                          const std::string& playerName,
                          std::function<void(Score&)> action);

protected:
    /// 目标映射：名称 -> 目标
    std::unordered_map<std::string, std::unique_ptr<ScoreObjective>> m_objectives;

    /// 判据到目标的映射
    std::unordered_map<ScoreCriteria*, std::vector<ScoreObjective*>> m_objectivesByCriteria;

    /// 玩家分数映射：玩家名 -> 目标名 -> 分数
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Score>>> m_playerScores;

    /// 显示槽位数组（19个槽位）
    std::array<ScoreObjective*, DISPLAY_SLOT_COUNT> m_displaySlots{};

    /// 队伍映射：名称 -> 队伍
    std::unordered_map<std::string, std::unique_ptr<ScorePlayerTeam>> m_teams;

    /// 玩家所属队伍映射：玩家名 -> 队伍指针
    std::unordered_map<std::string, ScorePlayerTeam*> m_teamMemberships;

private:
    /**
     * @brief 检查目标名称是否有效
     */
    [[nodiscard]] static bool isValidObjectiveName(const std::string& name);

    /**
     * @brief 检查玩家名称是否有效
     */
    [[nodiscard]] static bool isValidPlayerName(const std::string& name);
};

} // namespace mc::scoreboard
