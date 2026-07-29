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
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace mc {
namespace world::chunk {
class ChunkData;
}
} // namespace mc

namespace mc::world::chunk {
class ChunkLoadTicketManager;
}

namespace mc::server {

// 前向声明
class ServerChunkManager;

namespace sync {

/**
 * @brief 区块发送管理器
 *
 * 管理区块数据的发送给客户端：
 * - 区块加载完成时自动发送给追踪该区块的玩家
 * - 玩家追踪变化时发送/卸载区块
 * - 区块卸载前发送卸载通知
 *
 * 与 ChunkLoadTicketManager 协同工作：
 * - 通过 getTrackingPlayers() 获取追踪某区块的玩家
 * - 区块加载完成时自动发送
 * - 追踪变化回调触发发送/卸载
 *
 * 网络发送通过回调实现，由 MinecraftServer/IntegratedServer 设置。
 */
class ChunkSendManager {
public:
    /**
     * @brief 构造函数
     * @param chunkManager 区块管理器引用
     * @param ticketManager 票据管理器引用（用于查询追踪玩家）
     */
    ChunkSendManager(ServerChunkManager& chunkManager, world::chunk::ChunkLoadTicketManager& ticketManager);

    /**
     * @brief 发送区块给指定玩家列表
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param players 玩家ID列表
     * @param validateTracking 发送前是否校验玩家仍在追踪该区块
     *
     * 如果区块已加载，立即序列化并发送；
     * 如果区块未加载，触发异步加载，加载完成后发送。
     */
    void sendChunkToPlayers(
        ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players, bool validateTracking = false);

    /**
     * @brief 发送区块给所有追踪该区块的玩家
     * @param x 区块X坐标
     * @param z 区块Z坐标
     *
     * 从 ChunkLoadTicketManager 查询追踪该区块的玩家。
     */
    void sendChunkToTrackingPlayers(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 发送区块卸载通知给指定玩家列表
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param players 玩家ID列表
     */
    void unloadChunkFromPlayers(ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players);

    /**
     * @brief 发送区块卸载通知给所有追踪该区块的玩家
     * @param x 区块X坐标
     * @param z 区块Z坐标
     */
    void unloadChunkFromTrackingPlayers(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 处理玩家追踪变化
     * @param player 玩家ID
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param isTracking true=玩家开始追踪该区块，false=停止追踪
     *
     * 由 ChunkLoadTicketManager 的追踪变化回调触发。
     * 追踪进入：发送区块（如已加载）或等待加载
     * 追踪离开：发送卸载通知
     */
    void onPlayerTrackingChange(PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking);

    /**
     * @brief 区块卸载前的处理
     * @param x 区块X坐标
     * @param z 区块Z坐标
     *
     * 由 ServerChunkManager 在卸载区块前调用。
     * 向所有追踪该区块的玩家发送卸载通知。
     */
    void onChunkPreUnload(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 移除玩家
     * @param playerId 玩家ID
     *
     * 玩家断开连接时调用。清理相关状态。
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 从 Worker 线程提交已构建的区块 IR
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param ir 已构建的 LevelChunkWithLight IR（vanilla 语义，由 buildLevelChunkWithLightIR 产出）
     * @param players 目标玩家列表
     * @param validateTracking 发送前是否校验玩家仍在追踪该区块
     * @thread-safe
     */
    void submitChunkData(ChunkCoord x,
        ChunkCoord z,
        mc::network::ir::play::LevelChunkWithLight ir,
        std::vector<PlayerId> players,
        bool validateTracking = false);

    /**
     * @brief 主线程处理待发送队列
     */
    void processPendingSends();

    /**
     * @brief 设置区块发送回调
     * @param callback 回调函数，参数为(玩家ID, 区块X, 区块Z, 区块IR)
     *
     * 回调在主线程 processPendingSends 内按 (chunk, player) 对调用，同一 chunk 给 N 个玩家
     * 触发 N 次。IR 已在 worker 线程构建一次，回调侧仅按玩家拷贝进包（与旧 bytes 方案等价）。
     */
    void setOnChunkSend(
        std::function<void(PlayerId, ChunkCoord, ChunkCoord, const mc::network::ir::play::LevelChunkWithLight&)>
            callback);

    /**
     * @brief 设置区块卸载回调
     * @param callback 回调函数，参数为(玩家ID, 区块X, 区块Z)
     */
    void setOnChunkUnload(std::function<void(PlayerId, ChunkCoord, ChunkCoord)> callback);

private:
    /**
     * @brief 生成区块键
     */
    [[nodiscard]] static u64 _chunkKey(ChunkCoord x, ChunkCoord z)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
    }

private:
    struct ReadyChunkData {
        ChunkCoord x = 0;
        ChunkCoord z = 0;
        mc::network::ir::play::LevelChunkWithLight ir;
        std::vector<PlayerId> players;
        bool validateTracking = false;
    };

    ServerChunkManager& m_chunkManager;
    world::chunk::ChunkLoadTicketManager& m_ticketManager;

    // 准备好的区块数据队列（包含目标玩家列表）
    std::vector<ReadyChunkData> m_readyChunks;
    std::mutex m_readyChunksMutex;

    std::function<void(PlayerId, ChunkCoord, ChunkCoord, const mc::network::ir::play::LevelChunkWithLight&)>
        m_onChunkSend;
    std::function<void(PlayerId, ChunkCoord, ChunkCoord)> m_onChunkUnload;
};

} // namespace sync
} // namespace mc::server
