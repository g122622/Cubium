#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/util/math/Vector3.hpp"
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

/**
 * @brief 单个方块的破坏进度状态
 * 参考 MC 1.16.5 DestroyBlockProgress
 */
struct BlockBreakProgress {
    EntityId breakerId;
    BlockPos position;
    u8 damageStage = 0;           // 0-9, 0=刚开始, 9=即将破坏
    u64 creationTick = 0;
    u64 lastUpdateTick = 0;
};

/**
 * @brief 客户端挖掘进度管理器
 *
 * 管理所有可见的方块破坏进度状态。
 * 参考 MC 1.16.5 WorldRenderer.damagedBlocks
 */
class BreakProgressManager {
public:
    static constexpr size_t MAX_DAMAGE_STAGE = 9;
    static constexpr u64 PROGRESS_TIMEOUT_TICKS = 400;  // 20秒
    static constexpr f64 MAX_RENDER_DISTANCE_SQ = 1024.0f;  // 32格
    static constexpr size_t INITIAL_BUFFER_CAPACITY = 16;  // 预分配缓冲区初始容量

    static BreakProgressManager& instance();

    void initialize();
    void cleanup();
    void tick(f64 deltaTime, u64 currentTick);

    // 本地玩家挖掘进度
    void startBreaking(const BlockPos& pos);
    u8 updateLocalProgress(const BlockPos& pos, f64 progress);
    void stopBreaking();

    [[nodiscard]] bool isBreaking() const { return m_localBreaking; }
    [[nodiscard]] const BlockPos& getLocalBreakPos() const { return m_localBreakPos; }
    [[nodiscard]] f64 getLocalProgress() const { return m_localProgress; }
    [[nodiscard]] u8 getLocalDamageStage() const { return m_localDamageStage; }

    // 远程玩家挖掘进度（多人游戏）
    void updateRemoteProgress(EntityId breakerId, const BlockPos& pos, i8 stage, u64 currentTick);
    void removeRemoteProgress(EntityId breakerId);
    void clearRemoteProgress();

    // 查询接口
    [[nodiscard]] u8 getDamageStage(const BlockPos& pos) const;
    [[nodiscard]] std::vector<const BlockBreakProgress*> getProgressAtPos(const BlockPos& pos) const;
    [[nodiscard]] std::vector<std::pair<BlockPos, u8>> getVisibleProgress(const Vector3& cameraPos) const;
    [[nodiscard]] bool hasProgressAt(const BlockPos& pos) const;

    /// 高性能版本：使用预分配缓冲区避免内存分配
    /// @param cameraPos 摄像机位置
    /// @param outProgress 输出缓冲区（会被清空后填充）
    void getVisibleProgress(const Vector3& cameraPos,
                            std::vector<std::pair<BlockPos, u8>>& outProgress) const;

private:
    BreakProgressManager() = default;
    ~BreakProgressManager() = default;
    BreakProgressManager(const BreakProgressManager&) = delete;
    BreakProgressManager& operator=(const BreakProgressManager&) = delete;

    void cleanupStaleProgress(u64 currentTick);
    void updatePositionIndex(const BlockBreakProgress& progress);
    void removeFromPositionIndex(const BlockPos& pos, EntityId breakerId);

    // 本地玩家状态
    bool m_localBreaking = false;
    BlockPos m_localBreakPos;
    f64 m_localProgress = 0.0f;
    u8 m_localDamageStage = 0;

    // 远程玩家状态（多人游戏）
    std::unordered_map<EntityId, BlockBreakProgress> m_remoteProgressByEntity;
    std::unordered_map<BlockPos, std::vector<EntityId>> m_remoteProgressByPos;
    u64 m_currentTick = 0;
};

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
