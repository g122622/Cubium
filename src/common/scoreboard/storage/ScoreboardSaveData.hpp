#pragma once

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "../core/Scoreboard.hpp"
#include "../core/ScoreObjective.hpp"
#include "../core/ScorePlayerTeam.hpp"
#include "../core/TeamEnums.hpp"
#include "../core/ScoreCriteriaRenderType.hpp"
#include "../../util/nbt/Nbt.hpp"
#include <string>
#include <vector>
#include <memory>

namespace mc::scoreboard {

/**
 * @brief 记分板持久化数据
 *
 * 用于序列化和反序列化记分板状态。
 * 参考 MC 1.16.5: net.minecraft.world.storage.ScoreboardSaveData
 */
class ScoreboardSaveData {
public:
    // ========== 目标数据 ==========

    /**
     * @brief 目标持久化数据
     */
    struct ObjectiveData {
        std::string name;
        std::string criteriaName;
        std::string displayName;      // JSON 格式
        std::string renderType;       // "integer" 或 "hearts"

        [[nodiscard]] nbt::tags::compound_tag toNbt() const;
        static Result<ObjectiveData> fromNbt(const nbt::tags::compound_tag& tag);
    };

    // ========== 分数数据 ==========

    /**
     * @brief 分数持久化数据
     */
    struct ScoreData {
        std::string playerName;
        std::string objectiveName;
        i32 score = 0;
        bool locked = false;          // 用于 trigger 判据

        [[nodiscard]] nbt::tags::compound_tag toNbt() const;
        static Result<ScoreData> fromNbt(const nbt::tags::compound_tag& tag);
    };

    // ========== 队伍数据 ==========

    /**
     * @brief 队伍持久化数据
     */
    struct TeamData {
        std::string name;
        std::string displayName;      // JSON 格式
        std::string prefix;           // JSON 格式
        std::string suffix;           // JSON 格式
        std::string color;            // 颜色名称
        std::string nameTagVisibility;
        std::string deathMessageVisibility;
        std::string collisionRule;
        bool allowFriendlyFire = true;
        bool seeFriendlyInvisibles = true;
        std::vector<std::string> members;

        [[nodiscard]] nbt::tags::compound_tag toNbt() const;
        static Result<TeamData> fromNbt(const nbt::tags::compound_tag& tag);
    };

    // ========== 显示槽位数据 ==========

    /**
     * @brief 显示槽位持久化数据
     */
    struct DisplaySlotData {
        i32 slot = 0;
        std::string objectiveName;

        [[nodiscard]] nbt::tags::compound_tag toNbt() const;
        static Result<DisplaySlotData> fromNbt(const nbt::tags::compound_tag& tag);
    };

    // ========== 序列化/反序列化 ==========

    /**
     * @brief 从 Scoreboard 提取所有数据
     *
     * @param scoreboard 记分板实例
     * @return 持久化数据
     */
    static ScoreboardSaveData fromScoreboard(const Scoreboard& scoreboard);

    /**
     * @brief 将数据应用到 Scoreboard
     *
     * @param scoreboard 记分板实例
     * @return 成功或错误
     */
    Result<void> applyToScoreboard(Scoreboard& scoreboard) const;

    // ========== NBT 序列化 ==========

    /**
     * @brief 序列化为 NBT 标签
     */
    [[nodiscard]] nbt::tags::compound_tag toNbt() const;

    /**
     * @brief 从 NBT 标签反序列化
     */
    static Result<ScoreboardSaveData> fromNbt(const nbt::tags::compound_tag& tag);

    // ========== 二进制序列化 ==========

    /**
     * @brief 序列化为二进制数据
     */
    [[nodiscard]] Result<std::vector<u8>> serialize() const;

    /**
     * @brief 从二进制数据反序列化
     */
    static Result<ScoreboardSaveData> deserialize(const std::vector<u8>& data);

    // ========== 数据访问 ==========

    [[nodiscard]] const std::vector<ObjectiveData>& objectives() const { return m_objectives; }
    [[nodiscard]] const std::vector<ScoreData>& scores() const { return m_scores; }
    [[nodiscard]] const std::vector<TeamData>& teams() const { return m_teams; }
    [[nodiscard]] const std::vector<DisplaySlotData>& displaySlots() const { return m_displaySlots; }

    void addObjective(ObjectiveData data) { m_objectives.push_back(std::move(data)); }
    void addScore(ScoreData data) { m_scores.push_back(std::move(data)); }
    void addTeam(TeamData data) { m_teams.push_back(std::move(data)); }
    void addDisplaySlot(DisplaySlotData data) { m_displaySlots.push_back(std::move(data)); }

    void clear() {
        m_objectives.clear();
        m_scores.clear();
        m_teams.clear();
        m_displaySlots.clear();
    }

private:
    std::vector<ObjectiveData> m_objectives;
    std::vector<ScoreData> m_scores;
    std::vector<TeamData> m_teams;
    std::vector<DisplaySlotData> m_displaySlots;
};

} // namespace mc::scoreboard
