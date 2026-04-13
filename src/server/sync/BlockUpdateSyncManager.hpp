#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

#include <functional>
#include <unordered_map>

namespace mc::world {
class ChunkLoadTicketManager;
}

namespace mc::server::sync {

/**
 * @brief 方块更新同步管理器
 *
 * 收集同一 tick 内的方块更新，并在服务器主循环末尾统一发送。
 * 同一坐标只保留最后一次写入的状态，不做跨坐标合并。
 */
class BlockUpdateSyncManager {
public:
    /**
     * @brief 构造函数
     * @param ticketManager 区块追踪票据管理器
     */
    explicit BlockUpdateSyncManager(world::ChunkLoadTicketManager& ticketManager);

    /**
     * @brief 记录方块更新
     * @param pos 方块位置
     * @param blockStateId 方块状态ID
     */
    void queueBlockUpdate(const BlockPos& pos, u32 blockStateId);

    /**
     * @brief 记录方块更新
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param blockStateId 方块状态ID
     */
    void queueBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId) {
        queueBlockUpdate(BlockPos(x, y, z), blockStateId);
    }

    /**
     * @brief 统一发送待处理的方块更新
     */
    void flushPendingUpdates();

    /**
     * @brief 设置方块更新发送回调
     * @param callback 回调函数，参数为(玩家ID, x, y, z, stateId)
     */
    void setOnBlockUpdate(std::function<void(PlayerId, i32, i32, i32, u32)> callback);

private:
    struct PendingBlockUpdate {
        BlockPos pos;
        u32 blockStateId = 0;
    };

    [[nodiscard]] static u64 chunkKey(ChunkCoord x, ChunkCoord z);

private:
    world::ChunkLoadTicketManager& m_ticketManager;
    std::unordered_map<BlockPos, u32> m_pendingBlockUpdates;
    std::function<void(PlayerId, i32, i32, i32, u32)> m_onBlockUpdate;
};

} // namespace mc::server::sync
