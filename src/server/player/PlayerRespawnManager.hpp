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

namespace mc {
class Player;
}

namespace mc::server {

class MinecraftServer;
class ServerWorld;

/**
 * @brief 玩家重生管理器
 *
 * 统一处理"玩家在死亡界面确认重生"的完整流程：按个人重生点/共享出生点计算落点，
 * 在目标维度为玩家重建实体，迁移除生命/饥饿/效果之外的全部玩家状态，并把
 * Respawn、出生点、经验、药水效果、难度、权限、坐标、区块加载序列发送给客户端。
 *
 * 死亡后客户端会在玩家实体被移除（约 1 秒）之前或之后发送重生请求，两条路径都由
 * 本类处理：旧实体已消失时从连接数据恢复必要状态，旧实体尚存时从其复制。
 */
class PlayerRespawnManager {
public:
    /**
     * @brief 构造重生管理器
     * @param server 服务器（不拥有）
     */
    explicit PlayerRespawnManager(MinecraftServer& server);

    /**
     * @brief 执行玩家重生
     *
     * @param playerId 玩家 ID
     * @param keepAllData 是否保留全部数据（末地通关返回主世界为 true；普通死亡为 false，
     *        仅当 keepInventory 规则开启或原游戏模式为旁观者时才保留背包与经验）
     * @return 重生成功返回 true；玩家不存在、未登录或目标世界不可用返回 false
     */
    bool respawn(PlayerId playerId, bool keepAllData);

    /**
     * @brief 玩家是否处于"已死亡、等待重生"状态
     *
     * 判定依据是连接的 waitingForRespawn 标志，而不是实体存活——实体在死亡 1 秒后即被移除。
     */
    [[nodiscard]] bool isWaitingForRespawn(PlayerId playerId) const;

private:
    /**
     * @brief 为玩家在目标世界创建新实体并完成状态迁移
     *
     * @param playerId 玩家 ID
     * @param targetWorld 目标世界
     * @param keepAllData 是否保留全部数据
     * @param missingRespawnBlock 个人重生点是否已失效
     * @return 新实体指针
     */
    [[nodiscard]] Player* _createRespawnedPlayer(
        PlayerId playerId, ServerWorld& targetWorld, bool keepAllData, bool missingRespawnBlock);

    /**
     * @brief 向重生后的客户端发送完整的 Play 阶段重入序列
     *
     * 对齐 vanilla PlayerList.respawn 的包序：Respawn → 传送 → （缺失出生方块时）
     * NO_RESPAWN_BLOCK_AVAILABLE → 区块加载起点 → 世界出生点 → 效果 → 经验。
     *
     * @param missingRespawnBlock 个人重生点是否已失效（失效时额外发送 GameEvent 0）
     */
    void _sendRespawnPackets(PlayerId playerId, Player& player, ServerWorld& world, bool missingRespawnBlock);

    MinecraftServer& m_server;
};

} // namespace mc::server