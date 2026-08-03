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

#include "ScoreboardSaveData.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::world::storage {
class SingleLevelStorageManager;
}

namespace mc::scoreboard {

/**
 * @brief 记分板数据管理器
 *
 * 负责记分板数据的持久化存储和加载。
 * 使用 SingleLevelStorageManager 作为后端存储入口，采用缓存 + 脏标记模式。
 *
 * 键设计：
 * - "objectives:{name}" - 目标数据
 * - "scores:{objective}:{player}" - 分数数据
 * - "teams:{name}" - 队伍数据
 * - "displayslots" - 显示槽位数据
 */
class ScoreboardDataManager {
public:
    /**
     * @brief 构造函数
     *
     * @param storage 世界存储门面
     */
    explicit ScoreboardDataManager(world::storage::SingleLevelStorageManager& storage);

    /**
     * @brief 析构函数
     *
     * 自动保存脏数据
     */
    ~ScoreboardDataManager() noexcept;

    // 禁止拷贝
    ScoreboardDataManager(const ScoreboardDataManager&) = delete;
    ScoreboardDataManager& operator=(const ScoreboardDataManager&) = delete;

    // ========== 目标操作 ==========

    /**
     * @brief 保存目标
     *
     * @param objective 目标数据
     * @return 成功或错误
     */
    Result<void> saveObjective(const ScoreboardSaveData::ObjectiveData& objective);

    /**
     * @brief 加载目标
     *
     * @param name 目标名称
     * @return 目标数据（如果存在）
     */
    [[nodiscard]] Result<std::optional<ScoreboardSaveData::ObjectiveData>> loadObjective(const std::string& name);

    /**
     * @brief 删除目标
     *
     * @param name 目标名称
     * @return 成功或错误
     */
    Result<void> deleteObjective(const std::string& name);

    /**
     * @brief 加载所有目标
     *
     * @return 所有目标数据
     */
    [[nodiscard]] Result<std::vector<ScoreboardSaveData::ObjectiveData>> loadAllObjectives();

    // ========== 分数操作 ==========

    /**
     * @brief 保存分数
     *
     * @param objectiveName 目标名称
     * @param playerName 玩家名称
     * @param score 分数值
     * @param locked 是否锁定（用于 trigger）
     * @return 成功或错误
     */
    Result<void> saveScore(
        const std::string& objectiveName, const std::string& playerName, i32 score, bool locked = false);

    /**
     * @brief 加载分数
     *
     * @param objectiveName 目标名称
     * @param playerName 玩家名称
     * @return 分数数据（如果存在）
     */
    [[nodiscard]] Result<std::optional<ScoreboardSaveData::ScoreData>> loadScore(
        const std::string& objectiveName, const std::string& playerName);

    /**
     * @brief 删除分数
     *
     * @param objectiveName 目标名称
     * @param playerName 玩家名称
     * @return 成功或错误
     */
    Result<void> deleteScore(const std::string& objectiveName, const std::string& playerName);

    /**
     * @brief 删除玩家的所有分数
     *
     * @param playerName 玩家名称
     * @return 成功或错误
     */
    Result<void> deletePlayerScores(const std::string& playerName);

    /**
     * @brief 加载指定目标的所有分数
     *
     * @param objectiveName 目标名称
     * @return 分数数据列表
     */
    [[nodiscard]] Result<std::vector<ScoreboardSaveData::ScoreData>> loadScoresForObjective(
        const std::string& objectiveName);

    // ========== 队伍操作 ==========

    /**
     * @brief 保存队伍
     *
     * @param team 队伍数据
     * @return 成功或错误
     */
    Result<void> saveTeam(const ScoreboardSaveData::TeamData& team);

    /**
     * @brief 加载队伍
     *
     * @param name 队伍名称
     * @return 队伍数据（如果存在）
     */
    [[nodiscard]] Result<std::optional<ScoreboardSaveData::TeamData>> loadTeam(const std::string& name);

    /**
     * @brief 删除队伍
     *
     * @param name 队伍名称
     * @return 成功或错误
     */
    Result<void> deleteTeam(const std::string& name);

    /**
     * @brief 加载所有队伍
     *
     * @return 所有队伍数据
     */
    [[nodiscard]] Result<std::vector<ScoreboardSaveData::TeamData>> loadAllTeams();

    // ========== 显示槽位操作 ==========

    /**
     * @brief 保存显示槽位
     *
     * @param slot 槽位索引
     * @param objectiveName 目标名称（空表示清除）
     * @return 成功或错误
     */
    Result<void> saveDisplaySlot(i32 slot, const std::string& objectiveName);

    /**
     * @brief 加载所有显示槽位
     *
     * @return 显示槽位数据列表
     */
    [[nodiscard]] Result<std::vector<ScoreboardSaveData::DisplaySlotData>> loadDisplaySlots();

    // ========== 批量操作 ==========

    /**
     * @brief 保存整个记分板
     *
     * @param scoreboard 记分板实例
     * @return 成功或错误
     */
    Result<void> saveScoreboard(const Scoreboard& scoreboard);

    /**
     * @brief 加载整个记分板
     *
     * @param scoreboard 记分板实例
     * @return 成功或错误
     */
    Result<void> loadScoreboard(Scoreboard& scoreboard);

    /**
     * @brief 保存所有脏数据
     *
     * @return 保存的数据项数量
     */
    Result<size_t> saveAllDirty();

    /**
     * @brief 清空缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存大小
     */
    [[nodiscard]] size_t cacheSize() const;

    /**
     * @brief 获取脏数据数量
     */
    [[nodiscard]] size_t dirtyCount() const;

private:
    // 键前缀
    static constexpr const char* KEY_PREFIX_OBJECTIVES = "obj:";
    static constexpr const char* KEY_PREFIX_SCORES = "score:";
    static constexpr const char* KEY_PREFIX_TEAMS = "team:";
    static constexpr const char* KEY_DISPLAY_SLOTS = "displayslots";

    /**
     * @brief 生成目标键
     */
    [[nodiscard]] static std::vector<u8> makeObjectiveKey(const std::string& name);

    /**
     * @brief 生成分数键
     */
    [[nodiscard]] static std::vector<u8> makeScoreKey(const std::string& objectiveName, const std::string& playerName);

    /**
     * @brief 生成队伍键
     */
    [[nodiscard]] static std::vector<u8> makeTeamKey(const std::string& name);

    /**
     * @brief 从分数键解析目标名和玩家名
     */
    [[nodiscard]] static Result<std::pair<std::string, std::string>> parseScoreKey(const std::vector<u8>& key);

    world::storage::SingleLevelStorageManager& m_storage;

    mutable std::mutex m_cacheMutex;

    // 缓存
    std::unordered_map<std::string, ScoreboardSaveData::ObjectiveData> m_objectiveCache;
    std::unordered_map<std::string, std::unordered_map<std::string, ScoreboardSaveData::ScoreData>> m_scoreCache;
    std::unordered_map<std::string, ScoreboardSaveData::TeamData> m_teamCache;
    std::vector<ScoreboardSaveData::DisplaySlotData> m_displaySlotCache;

    // 脏数据追踪
    std::unordered_set<std::string> m_dirtyObjectives;
    std::unordered_set<std::string> m_dirtyScores; // 格式: "objective:player"
    std::unordered_set<std::string> m_dirtyTeams;
    bool m_dirtyDisplaySlots = false;

    bool m_displaySlotsLoaded = false;
};

} // namespace mc::scoreboard
