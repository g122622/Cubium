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

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "../../network/packet/PacketSerializer.hpp"
#include <string>
#include <vector>

namespace mc::network {

/**
 * @brief 记分板目标操作类型
 *
 * 参考 MC 1.16.5: net.minecraft.network.play.server.SScoreboardObjectivePacket
 */
enum class ObjectiveAction : u8 {
    /// 添加目标
    Add = 0,

    /// 移除目标
    Remove = 1,

    /// 更新目标（显示名称或渲染类型）
    Update = 2
};

/**
 * @brief 分数操作类型
 *
 * 参考 MC 1.16.5: net.minecraft.network.play.server.SUpdateScorePacket
 */
enum class ScoreAction : u8 {
    /// 更新/设置分数
    Change = 0,

    /// 移除分数
    Remove = 1
};

/**
 * @brief 队伍操作类型
 *
 * 参考 MC 1.16.5: net.minecraft.network.play.server.STeamsPacket
 */
enum class TeamAction : u8 {
    /// 创建队伍（包含完整信息和成员列表）
    Create = 0,

    /// 移除队伍
    Remove = 1,

    /// 更新队伍信息
    Update = 2,

    /// 添加成员到队伍
    AddMember = 3,

    /// 从队伍移除成员
    RemoveMember = 4
};

/**
 * @brief 记分板目标同步包
 *
 * 用于同步目标的创建、移除和更新。
 * 参考 MC 1.16.5: SScoreboardObjectivePacket (0x3B)
 */
class ScoreboardObjectivePacket {
public:
    /// 目标名称最大长度
    static constexpr size_t MAX_NAME_LENGTH = 16;

    /// 显示名称最大长度
    static constexpr size_t MAX_DISPLAY_NAME_LENGTH = 128;

    /**
     * @brief 默认构造函数
     */
    ScoreboardObjectivePacket() = default;

    /**
     * @brief 构造函数
     *
     * @param objectiveName 目标名称
     * @param action 操作类型
     * @param displayName 显示名称（JSON 格式）
     * @param renderType 渲染类型（"integer" 或 "hearts"）
     */
    ScoreboardObjectivePacket(const std::string& objectiveName,
        ObjectiveAction action,
        const std::string& displayName,
        const std::string& renderType);

    // ========== Getters ==========

    [[nodiscard]] const std::string& objectiveName() const noexcept { return m_objectiveName; }
    [[nodiscard]] ObjectiveAction action() const noexcept { return m_action; }
    [[nodiscard]] const std::string& displayName() const noexcept { return m_displayName; }
    [[nodiscard]] const std::string& renderType() const noexcept { return m_renderType; }

    // ========== Setters ==========

    void setObjectiveName(const std::string& name);
    void setAction(ObjectiveAction action) noexcept { m_action = action; }
    void setDisplayName(const std::string& name);
    void setRenderType(const std::string& type);

    // ========== 序列化 ==========

    /**
     * @brief 序列化到包序列化器
     */
    void serialize(PacketSerializer& ser) const;

    /**
     * @brief 从包反序列化器反序列化
     */
    [[nodiscard]] static Result<ScoreboardObjectivePacket> deserialize(PacketDeserializer& deser);

private:
    std::string m_objectiveName;
    ObjectiveAction m_action = ObjectiveAction::Add;
    std::string m_displayName;
    std::string m_renderType = "integer";
};

/**
 * @brief 分数更新同步包
 *
 * 用于同步分数的设置和移除。
 * 参考 MC 1.16.5: SUpdateScorePacket (0x3C)
 */
class UpdateScorePacket {
public:
    /// 玩家名称最大长度
    static constexpr size_t MAX_NAME_LENGTH = 40;

    /**
     * @brief 默认构造函数
     */
    UpdateScorePacket() = default;

    /**
     * @brief 构造函数
     *
     * @param playerName 玩家/实体名称
     * @param objectiveName 目标名称
     * @param score 分数值
     * @param action 操作类型
     */
    UpdateScorePacket(const std::string& playerName, const std::string& objectiveName, i32 score, ScoreAction action);

    // ========== Getters ==========

    [[nodiscard]] const std::string& playerName() const noexcept { return m_playerName; }
    [[nodiscard]] const std::string& objectiveName() const noexcept { return m_objectiveName; }
    [[nodiscard]] i32 score() const noexcept { return m_score; }
    [[nodiscard]] ScoreAction action() const noexcept { return m_action; }

    // ========== Setters ==========

    void setPlayerName(const std::string& name);
    void setObjectiveName(const std::string& name);
    void setScore(i32 score) noexcept { m_score = score; }
    void setAction(ScoreAction action) noexcept { m_action = action; }

    // ========== 序列化 ==========

    void serialize(PacketSerializer& ser) const;
    [[nodiscard]] static Result<UpdateScorePacket> deserialize(PacketDeserializer& deser);

private:
    std::string m_playerName;
    std::string m_objectiveName;
    i32 m_score = 0;
    ScoreAction m_action = ScoreAction::Change;
};

/**
 * @brief 显示目标同步包
 *
 * 用于设置目标在哪个显示槽位显示。
 * 参考 MC 1.16.5: SDisplayObjectivePacket (0x3D)
 */
class DisplayObjectivePacket {
public:
    /**
     * @brief 默认构造函数
     */
    DisplayObjectivePacket() = default;

    /**
     * @brief 构造函数
     *
     * @param position 显示槽位（0-18）
     * @param objectiveName 目标名称（空字符串表示清除）
     */
    DisplayObjectivePacket(i32 position, const std::string& objectiveName);

    // ========== Getters ==========

    [[nodiscard]] i32 position() const noexcept { return m_position; }
    [[nodiscard]] const std::string& objectiveName() const noexcept { return m_objectiveName; }

    // ========== Setters ==========

    void setPosition(i32 position);
    void setObjectiveName(const std::string& name);

    // ========== 序列化 ==========

    void serialize(PacketSerializer& ser) const;
    [[nodiscard]] static Result<DisplayObjectivePacket> deserialize(PacketDeserializer& deser);

private:
    i32 m_position = 0;
    std::string m_objectiveName;
};

/**
 * @brief 队伍同步包
 *
 * 用于同步队伍的创建、移除、更新和成员变更。
 * 参考 MC 1.16.5: STeamsPacket (0x3E)
 */
class TeamsPacket {
public:
    /// 队伍名称最大长度
    static constexpr size_t MAX_NAME_LENGTH = 16;

    /// 显示名称最大长度
    static constexpr size_t MAX_DISPLAY_NAME_LENGTH = 128;

    /// 前缀/后缀最大长度
    static constexpr size_t MAX_PREFIX_SUFFIX_LENGTH = 16;

    /**
     * @brief 默认构造函数
     */
    TeamsPacket() = default;

    /**
     * @brief 构造函数
     *
     * @param teamName 队伍名称
     * @param action 操作类型
     */
    TeamsPacket(const std::string& teamName, TeamAction action);

    // ========== Getters ==========

    [[nodiscard]] const std::string& teamName() const noexcept { return m_teamName; }
    [[nodiscard]] TeamAction action() const noexcept { return m_action; }
    [[nodiscard]] const std::string& displayName() const noexcept { return m_displayName; }
    [[nodiscard]] const std::string& prefix() const noexcept { return m_prefix; }
    [[nodiscard]] const std::string& suffix() const noexcept { return m_suffix; }
    [[nodiscard]] const std::string& nameTagVisibility() const noexcept { return m_nameTagVisibility; }
    [[nodiscard]] const std::string& collisionRule() const noexcept { return m_collisionRule; }
    [[nodiscard]] const std::string& color() const noexcept { return m_color; }
    [[nodiscard]] u8 friendlyFlags() const noexcept { return m_friendlyFlags; }
    [[nodiscard]] const std::vector<std::string>& players() const noexcept { return m_players; }

    // ========== Setters ==========

    void setTeamName(const std::string& name);
    void setAction(TeamAction action) noexcept { m_action = action; }
    void setDisplayName(const std::string& name);
    void setPrefix(const std::string& prefix);
    void setSuffix(const std::string& suffix);
    void setNameTagVisibility(const std::string& visibility);
    void setCollisionRule(const std::string& rule);
    void setColor(const std::string& color);
    void setFriendlyFlags(u8 flags) noexcept { m_friendlyFlags = flags; }
    void setAllowFriendlyFire(bool allow) noexcept;
    void setSeeFriendlyInvisibles(bool see) noexcept;
    void setPlayers(const std::vector<std::string>& players);

    // ========== 序列化 ==========

    void serialize(PacketSerializer& ser) const;
    [[nodiscard]] static Result<TeamsPacket> deserialize(PacketDeserializer& deser);

private:
    std::string m_teamName;
    TeamAction m_action = TeamAction::Create;

    // Create/Update 时有效
    std::string m_displayName;
    std::string m_prefix;
    std::string m_suffix;
    std::string m_nameTagVisibility = "always";
    std::string m_collisionRule = "always";
    std::string m_color = "white";
    u8 m_friendlyFlags = 0x03; // 默认允许友军伤害 + 能看到隐身队友

    // Create/AddMember/RemoveMember 时有效
    std::vector<std::string> m_players;
};

} // namespace mc::network
