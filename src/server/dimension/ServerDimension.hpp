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
#include "common/world/dimension/Dimension.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

namespace mc {

// 前向声明
namespace server {
class ServerWorld;
class ServerChunkManager;
} // namespace server

namespace server {
namespace sync {
class EntitySyncManager;
class ChunkSendManager;
class BlockUpdateSyncManager;
} // namespace sync
} // namespace server

namespace world {
namespace spawn {
class NaturalSpawner;
class DespawnManager;
} // namespace spawn
} // namespace world

class WorldLightManager;

namespace server {
class MinecraftServer;
}

/**
 * @brief 服务端维度实例
 *
 * 继承 Dimension，添加服务端特有的功能：
 * - ServerWorld 管理
 * - 运行时世界引用
 * - 玩家追踪
 * - 传送门位置记录
 */
class ServerDimension : public Dimension {
public:
    /**
     * @brief 构造服务端维度
     *
     * @param id 维度ID
     * @param type 维度类型
     * @param generator 区块生成器
     * @param seed 世界种子
     * @param viewDistance 视野距离
     */
    ServerDimension(
        DimensionId id, DimensionType type, std::unique_ptr<IChunkGenerator> generator, u64 seed, i32 viewDistance);

    ~ServerDimension() override;

    // 禁止拷贝
    ServerDimension(const ServerDimension&) = delete;
    ServerDimension& operator=(const ServerDimension&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化维度
     *
     * 创建 ServerWorld、区块管理器、光照管理器等。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize();

    /**
     * @brief 关闭维度
     *
     * 清理所有资源。
     */
    void shutdown();

    // ========== 更新 ==========

    /**
     * @brief 维度刻更新
     *
     * 更新区块管理器、实体、天气等。
     */
    void tick() override;

    // ========== 世界访问 ==========

    /**
     * @brief 获取服务端世界
     */
    [[nodiscard]] server::ServerWorld* world() { return m_world.get(); }
    [[nodiscard]] const server::ServerWorld* world() const { return m_world.get(); }

    void setWorld(std::unique_ptr<server::ServerWorld> world);

    /**
     * @brief 获取区块管理器
     */
    [[nodiscard]] server::ServerChunkManager* chunkManager();
    [[nodiscard]] const server::ServerChunkManager* chunkManager() const;

    /**
     * @brief 获取光照管理器
     */
    [[nodiscard]] WorldLightManager* lightManager();
    [[nodiscard]] const WorldLightManager* lightManager() const;

    // ========== 同步管理器 ==========

    /**
     * @brief 获取实体同步管理器
     */
    [[nodiscard]] server::sync::EntitySyncManager* entitySyncManager() { return m_entitySyncManager.get(); }
    [[nodiscard]] const server::sync::EntitySyncManager* entitySyncManager() const { return m_entitySyncManager.get(); }

    /**
     * @brief 获取区块发送管理器
     */
    [[nodiscard]] server::sync::ChunkSendManager* chunkSendManager() { return m_chunkSendManager.get(); }
    [[nodiscard]] const server::sync::ChunkSendManager* chunkSendManager() const { return m_chunkSendManager.get(); }

    /**
     * @brief 获取方块更新同步管理器
     */
    [[nodiscard]] server::sync::BlockUpdateSyncManager* blockUpdateSyncManager()
    {
        return m_blockUpdateSyncManager.get();
    }
    [[nodiscard]] const server::sync::BlockUpdateSyncManager* blockUpdateSyncManager() const
    {
        return m_blockUpdateSyncManager.get();
    }

    // ========== 生物生成 ==========

    /**
     * @brief 获取自然生成管理器
     */
    [[nodiscard]] world::spawn::NaturalSpawner* naturalSpawner() { return m_naturalSpawner.get(); }
    [[nodiscard]] const world::spawn::NaturalSpawner* naturalSpawner() const { return m_naturalSpawner.get(); }

    /**
     * @brief 获取生物消失管理器
     */
    [[nodiscard]] world::spawn::DespawnManager* despawnManager() { return m_despawnManager.get(); }
    [[nodiscard]] const world::spawn::DespawnManager* despawnManager() const { return m_despawnManager.get(); }

    // ========== 玩家追踪 ==========

    /**
     * @brief 添加玩家到维度
     */
    void addPlayer(PlayerId playerId);

    /**
     * @brief 从维度移除玩家
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 检查玩家是否在维度中
     */
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;

    /**
     * @brief 获取维度中的所有玩家
     */
    [[nodiscard]] const std::vector<PlayerId>& players() const { return m_players; }

    /**
     * @brief 检查维度是否有玩家
     */
    [[nodiscard]] bool hasPlayers() const { return !m_players.empty(); }

    /**
     * @brief 获取玩家数量
     */
    [[nodiscard]] size_t playerCount() const { return m_players.size(); }

    // ========== 传送门追踪 ==========

    /**
     * @brief 记录传送门位置
     *
     * 用于传送门搜索，避免重复创建传送门。
     */
    void recordPortalPosition(const BlockPos& pos);

    /**
     * @brief 忘记传送门位置
     */
    void forgetPortalPosition(const BlockPos& pos);

    /**
     * @brief 检查位置是否有传送门
     */
    [[nodiscard]] bool hasPortalAt(const BlockPos& pos) const;

    /**
     * @brief 获取最近的传送门位置
     *
     * @param pos 搜索中心位置
     * @param radius 搜索半径
     * @return 最近的传送门位置，如果没有则返回空
     */
    [[nodiscard]] std::optional<BlockPos> findNearestPortal(const BlockPos& pos, i32 radius) const;

    // ========== 配置 ==========

    /**
     * @brief 获取视野距离
     */
    [[nodiscard]] i32 viewDistance() const { return m_viewDistance; }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 seed() const { return m_seed; }

private:
    std::unique_ptr<server::ServerWorld> m_world;

    // 同步管理器（每个维度各自持有）
    std::unique_ptr<server::sync::EntitySyncManager> m_entitySyncManager;
    std::unique_ptr<server::sync::ChunkSendManager> m_chunkSendManager;
    std::unique_ptr<server::sync::BlockUpdateSyncManager> m_blockUpdateSyncManager;

    // 生物生成管理器
    std::unique_ptr<world::spawn::NaturalSpawner> m_naturalSpawner;
    std::unique_ptr<world::spawn::DespawnManager> m_despawnManager;

    std::vector<PlayerId> m_players;
    std::unordered_set<u64> m_portalPositions; // 使用哈希的 BlockPos

    u64 m_seed;
    i32 m_viewDistance;
    bool m_initialized = false;

    /**
     * @brief 计算 BlockPos 的哈希值
     */
    [[nodiscard]] static u64 _hashBlockPos(const BlockPos& pos);
};

} // namespace mc
