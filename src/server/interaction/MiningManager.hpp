#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include <unordered_map>
#include <functional>

namespace mc::server {

// 前向声明
class ServerWorld;
namespace core {
class PlayerManager;
class ConnectionManager;
}

namespace interaction {

/**
 * @brief 挖掘进度管理器
 *
 * 追踪玩家的挖掘进度：
 * - 计算挖掘速度（工具、方块硬度）
 * - 广播破坏动画阶段
 * - 处理挖掘中止
 */
class MiningManager {
public:
    /**
     * @brief 构造函数
     */
    MiningManager(core::PlayerManager& playerManager,
                  core::ConnectionManager& connectionManager);

    /**
     * @brief 开始挖掘
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param entityId 用于广播动画的实体ID（破坏者ID）
     */
    void startMining(PlayerId playerId, const BlockPos& pos, EntityId entityId = 0);

    /**
     * @brief 中止挖掘
     * @param playerId 玩家ID
     */
    void abortMining(PlayerId playerId);

    /**
     * @brief 处理方块交互数据包
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param action 交互动作
     */
    void handleBlockInteraction(PlayerId playerId, const BlockPos& pos,
                                network::BlockInteractionAction action);

    /**
     * @brief 每 tick 更新挖掘进度
     * @param world 世界引用（用于获取方块硬度）
     */
    void tick(ServerWorld& world);

    /**
     * @brief 获取挖掘进度 (0.0 - 1.0)
     * @param playerId 玩家ID
     * @return 挖掘进度，如果未在挖掘则返回 0
     */
    [[nodiscard]] f32 getMiningProgress(PlayerId playerId) const;

    /**
     * @brief 检查是否正在挖掘
     * @param playerId 玩家ID
     */
    [[nodiscard]] bool isMining(PlayerId playerId) const;

    /**
     * @brief 获取当前挖掘位置
     * @param playerId 玩家ID
     * @return 挖掘位置，如果未在挖掘则返回空
     */
    [[nodiscard]] std::optional<BlockPos> getMiningPosition(PlayerId playerId) const;

    /**
     * @brief 设置破坏动画广播回调
     * @param callback 回调函数 (playerId, x, y, z, stage)
     */
    void setOnBreakAnimBroadcast(
        std::function<void(PlayerId, i32, i32, i32, i8)> callback);

    /**
     * @brief 设置挖掘完成回调
     * @param callback 回调函数 (playerId, pos)
     */
    void setOnMiningComplete(
        std::function<void(PlayerId, const BlockPos&)> callback);

private:
    /**
     * @brief 计算挖掘速度
     * @param world 世界引用
     * @param pos 方块位置
     * @param playerId 玩家ID
     * @return 每tick的挖掘进度增量
     */
    [[nodiscard]] f32 calculateMiningSpeed(ServerWorld& world,
                                            const BlockPos& pos,
                                            PlayerId playerId) const;

    /**
     * @brief 广播破坏动画
     */
    void broadcastBreakAnim(PlayerId playerId, const BlockPos& pos, i8 stage);

private:
    /**
     * @brief 挖掘状态
     */
    struct MiningState {
        BlockPos position;
        f32 progress = 0.0f;
        u8 lastStage = 255;  // 上次广播的阶段 (0-9)
        bool active = false;
        u64 startTick = 0;
        EntityId breakerId = 0;  // 用于广播动画
    };

    core::PlayerManager& m_playerManager;
    core::ConnectionManager& m_connectionManager;
    std::unordered_map<PlayerId, MiningState> m_miningStates;

    std::function<void(PlayerId, i32, i32, i32, i8)> m_onBreakAnimBroadcast;
    std::function<void(PlayerId, const BlockPos&)> m_onMiningComplete;
};

} // namespace interaction
} // namespace mc::server