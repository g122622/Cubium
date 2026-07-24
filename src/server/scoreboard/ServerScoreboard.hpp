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

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/network/ScoreboardPackets.hpp"
#include <memory>
#include <set>

namespace mc {

// 前向声明
class ServerPlayer;

namespace server {
class IServer;
}

namespace scoreboard {
class ScoreboardDataManager;
}

namespace network {
class PacketSerializer;
}

namespace server {

/**
 * @brief 服务端记分板
 *
 * 扩展 Scoreboard 基类，添加网络同步和持久化功能。
 */
class ServerScoreboard : public mc::scoreboard::Scoreboard {
public:
    /**
     * @brief 构造函数
     *
     * @param server Minecraft 服务器实例
     */
    explicit ServerScoreboard(mc::server::IServer& server);

    /**
     * @brief 析构函数
     *
     * 不在析构中隐式保存。
     * 记分板持久化由服务器关闭流程统一触发。
     */
    ~ServerScoreboard() noexcept override;

    // 禁止拷贝
    ServerScoreboard(const ServerScoreboard&) = delete;
    ServerScoreboard& operator=(const ServerScoreboard&) = delete;

    /**
     * @brief 设置数据管理器
     *
     * @param dataManager 数据管理器（可选）
     */
    void setDataManager(mc::scoreboard::ScoreboardDataManager* dataManager) { m_dataManager = dataManager; }

    // ========== 玩家管理 ==========

    /**
     * @brief 玩家加入时调用
     *
     * 向新玩家发送所有目标、分数和队伍数据。
     *
     * @param player 服务端玩家
     */
    void onPlayerJoin(mc::ServerPlayer& player);

    /**
     * @brief 玩家离开时调用
     *
     * 清理玩家的分数数据。
     *
     * @param playerId 玩家 ID
     * @param playerName 玩家名称
     */
    void onPlayerLeave(PlayerId playerId, const std::string& playerName);

    // ========== 网络同步 ==========

    /**
     * @brief 向所有玩家发送 IR 包
     *
     * @param packet IR 包
     */
    void sendToAllPlayers(const mc::network::ir::IrPacket& packet);

    /**
     * @brief 向指定玩家发送 IR 包
     *
     * @param playerId 玩家 ID
     * @param packet IR 包
     */
    void sendToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /**
     * @brief 发送目标创建包给玩家
     *
     * @param objective 目标
     * @param playerId 玩家 ID
     */
    void sendObjectiveToPlayer(mc::scoreboard::ScoreObjective& objective, PlayerId playerId);

    /**
     * @brief 发送目标移除包给玩家
     *
     * @param objective 目标
     * @param playerId 玩家 ID
     */
    void sendRemoveObjectiveToPlayer(mc::scoreboard::ScoreObjective& objective, PlayerId playerId);

    /**
     * @brief 发送分数更新包给玩家
     *
     * @param score 分数
     * @param playerId 玩家 ID
     */
    void sendScoreToPlayer(mc::scoreboard::Score& score, PlayerId playerId);

    /**
     * @brief 发送分数移除包给玩家
     *
     * @param playerName 玩家名称
     * @param objectiveName 目标名称（空表示移除所有）
     * @param playerId 玩家 ID
     */
    void sendRemoveScoreToPlayer(const std::string& playerName, const std::string& objectiveName, PlayerId playerId);

    /**
     * @brief 发送显示槽位包给玩家
     *
     * @param slot 显示槽位
     * @param objective 目标（nullptr 表示清除）
     * @param playerId 玩家 ID
     */
    void sendDisplayObjectiveToPlayer(
        mc::scoreboard::DisplaySlot slot, mc::scoreboard::ScoreObjective* objective, PlayerId playerId);

    /**
     * @brief 发送队伍创建包给玩家
     *
     * @param team 队伍
     * @param playerId 玩家 ID
     */
    void sendTeamToPlayer(mc::scoreboard::ScorePlayerTeam& team, PlayerId playerId);

    /**
     * @brief 发送队伍移除包给玩家
     *
     * @param team 队伍
     * @param playerId 玩家 ID
     */
    void sendRemoveTeamToPlayer(mc::scoreboard::ScorePlayerTeam& team, PlayerId playerId);

    // ========== 持久化 ==========

    /**
     * @brief 标记数据为脏
     *
     * 数据将在下次保存时写入磁盘。
     */
    void markDirty() { m_dirty = true; }

    /**
     * @brief 检查数据是否为脏
     *
     * @return true 如果需要保存
     */
    [[nodiscard]] bool isDirty() const { return m_dirty; }

    /**
     * @brief 保存数据
     *
     * 将数据写入持久化存储。
     */
    void save();

    /**
     * @brief 加载数据
     *
     * 从持久化存储加载数据。
     */
    void load();

protected:
    // ========== 回调覆写 ==========

    void onObjectiveAdded(mc::scoreboard::ScoreObjective& objective) override;
    void onObjectiveRemoved(mc::scoreboard::ScoreObjective& objective) override;
    void onObjectiveChanged(mc::scoreboard::ScoreObjective& objective) override;
    void onScoreChanged(mc::scoreboard::Score& score) override;
    void onScoreRemoved(mc::scoreboard::Score& score) override;
    void onPlayerRemoved(const std::string& playerName) override;
    void onPlayerScoreRemoved(const std::string& playerName, mc::scoreboard::ScoreObjective& objective) override;
    void onTeamAdded(mc::scoreboard::ScorePlayerTeam& team) override;
    void onTeamChanged(mc::scoreboard::ScorePlayerTeam& team) override;
    void onTeamRemoved(mc::scoreboard::ScorePlayerTeam& team) override;
    void onDisplaySlotChanged(mc::scoreboard::DisplaySlot slot, mc::scoreboard::ScoreObjective* objective) override;

private:
    /**
     * @brief 创建目标数据包
     * @param method 0=Add 1=Remove 2=Change
     */
    [[nodiscard]] mc::network::ir::play::SetObjective _createObjectivePacket(
        mc::scoreboard::ScoreObjective& objective, u8 method);

    /**
     * @brief 创建分数数据包
     * @param change true=变更(填 score)，false=占位
     */
    [[nodiscard]] mc::network::ir::play::SetScore _createSetScorePacket(mc::scoreboard::Score& score, bool change);

    /**
     * @brief 创建显示目标数据包
     */
    [[nodiscard]] mc::network::ir::play::SetDisplayObjective _createDisplayObjectivePacket(
        mc::scoreboard::DisplaySlot slot, mc::scoreboard::ScoreObjective* objective);

    /**
     * @brief 创建队伍数据包
     * @param method 0=Create 1=Remove 2=Change 3=Join 4=Leave
     */
    [[nodiscard]] mc::network::ir::play::SetPlayerTeam _createTeamPacket(
        mc::scoreboard::ScorePlayerTeam& team, u8 method);

    mc::server::IServer& m_server;

    /// 数据管理器（可选，用于持久化）
    mc::scoreboard::ScoreboardDataManager* m_dataManager = nullptr;

    /// 已同步到客户端的目标（用于玩家加入时发送）
    std::set<mc::scoreboard::ScoreObjective*> m_addedObjectives;

    /// 数据是否需要保存
    bool m_dirty = false;
};

} // namespace server
} // namespace mc
