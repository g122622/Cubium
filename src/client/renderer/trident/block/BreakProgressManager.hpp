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
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {
namespace client {
namespace renderer {
namespace trident {
namespace block {

/**
 * @brief 单个方块的破坏进度状态
 */
struct BlockBreakProgress {
    EntityInstanceId breakerId;
    BlockPos position;
    u8 damageStage = 0; // 0-9, 0=刚开始, 9=即将破坏
    u64 creationTick = 0;
    u64 lastUpdateTick = 0;
};

/**
 * @brief 击打音效回调类型
 * @param pos 方块位置
 * @param damageStage 破坏阶段 (0-9)
 */
using HitSoundCallback = std::function<void(const BlockPos& pos, u8 damageStage)>;

/**
 * @brief 客户端挖掘进度管理器
 *
 * 管理所有可见的方块破坏进度状态。
 */
class BreakProgressManager {
public:
    static constexpr size_t MAX_DAMAGE_STAGE = 9;
    static constexpr u8 NO_DAMAGE = 255;                  // 无破坏进度时的返回值
    static constexpr u64 PROGRESS_TIMEOUT_TICKS = 400;    // 20秒
    static constexpr f64 MAX_RENDER_DISTANCE_SQ = 1024.0; // 32格

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
    void updateRemoteProgress(EntityInstanceId breakerId, const BlockPos& pos, i8 stage, u64 currentTick);
    void removeRemoteProgress(EntityInstanceId breakerId);
    void clearRemoteProgress();

    // 查询接口
    [[nodiscard]] u8 getDamageStage(const BlockPos& pos) const;
    [[nodiscard]] std::vector<const BlockBreakProgress*> getProgressAtPos(const BlockPos& pos) const;
    [[nodiscard]] std::vector<std::pair<BlockPos, u8>> getVisibleProgress(const Vector3& cameraPos) const;
    [[nodiscard]] bool hasProgressAt(const BlockPos& pos) const;

    /// 高性能版本：使用预分配缓冲区避免内存分配
    /// @param cameraPos 摄像机位置
    /// @param outProgress 输出缓冲区（会被清空后填充）
    void getVisibleProgress(const Vector3& cameraPos, std::vector<std::pair<BlockPos, u8>>& outProgress) const;

    /**
     * @brief 设置击打音效回调
     *
     * 当破坏阶段变化时调用此回调播放击打音效。
     *
     * @param callback 回调函数
     */
    void setHitSoundCallback(HitSoundCallback callback) { m_hitSoundCallback = std::move(callback); }

private:
    BreakProgressManager() = default;
    ~BreakProgressManager() = default;
    BreakProgressManager(const BreakProgressManager&) = delete;
    BreakProgressManager& operator=(const BreakProgressManager&) = delete;

    void _cleanupStaleProgress(u64 currentTick);
    void _updatePositionIndex(const BlockBreakProgress& progress);
    void _removeFromPositionIndex(const BlockPos& pos, EntityInstanceId breakerId);

    // 本地玩家状态
    bool m_localBreaking = false;
    BlockPos m_localBreakPos;
    f64 m_localProgress = 0.0;
    u8 m_localDamageStage = 0;

    // 远程玩家状态（多人游戏）
    std::unordered_map<EntityInstanceId, BlockBreakProgress> m_remoteProgressByEntity;
    std::unordered_map<BlockPos, std::vector<EntityInstanceId>> m_remoteProgressByPos;
    u64 m_currentTick = 0;

    // 击打音效回调
    HitSoundCallback m_hitSoundCallback;
};

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
