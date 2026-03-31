#pragma once

#include "common/core/Types.hpp"
#include "common/core/Constants.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include <memory>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace mc {
class AbstractContainerMenu;  // 前向声明
class PlayerInventory;         // 前向声明
}

namespace mc::server {

/**
 * @brief 服务端玩家数据
 *
 * 存储服务端维护的玩家状态信息，使用连接接口而非具体会话类型。
 * 这使得代码可以用于 TCP 远程连接和本地连接两种场景。
 *
 * 使用示例：
 * @code
 * ServerPlayerData player(1, "Steve");
 * player.x = 100.0f;
 * player.y = 64.0f;
 * player.z = 200.0f;
 * @endcode
 */
struct ServerPlayerData {
    /// 玩家ID
    PlayerId playerId = 0;

    /// 用户名
    String username;

    /// 连接接口（可以是 TcpConnection 或 LocalServerConnection）
    network::ConnectionWeakPtr connection;

    /// 会话ID（用于 TCP 连接标识）
    u32 sessionId = 0;

    /// 登录状态
    bool loggedIn = false;

    /// 区块追踪器
    std::shared_ptr<network::PlayerChunkTracker> chunkTracker;

    // 位置（内部使用 f32，网络边界使用 f64）
    f32 x = 0.0f;
    f32 y = 64.0f;
    f32 z = 0.0f;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    bool onGround = true;

    /// 游戏模式
    GameMode gameMode = GameMode::Survival;

    // 传送确认
    u32 pendingTeleportId = 0;
    bool waitingTeleportConfirm = false;

    // 心跳统计
    u64 lastKeepAliveSent = 0;
    u64 lastKeepAliveReceived = 0;
    u64 lastKeepAliveSentTick = 0;  // 发送心跳时的 tick
    u32 ping = 0;  // 延迟（毫秒）

    // 已加载的区块
    std::unordered_set<ChunkId> loadedChunks;

    // 效果系统
    std::vector<entity::effect::EffectInstance> effects;

    // 容器相关（使用原始指针避免 incomplete type 问题）
    mc::AbstractContainerMenu* openMenu = nullptr;
    ContainerType openContainerType = ContainerType::Player;
    ContainerId nextContainerId = 1;

    ServerPlayerData() = default;

    /**
     * @brief 构造玩家数据
     * @param id 玩家ID
     * @param name 用户名
     */
    explicit ServerPlayerData(PlayerId id, const String& name)
        : playerId(id), username(name) {}

    /**
     * @brief 获取连接（如果有效）
     * @return 连接共享指针，如果已断开则返回 nullptr
     */
    [[nodiscard]] network::ConnectionPtr getConnection() const {
        return connection.lock();
    }

    /**
     * @brief 检查连接是否有效
     * @return true 如果连接有效
     */
    [[nodiscard]] bool hasConnection() const {
        auto conn = connection.lock();
        return conn && conn->isConnected();
    }

    /**
     * @brief 发送数据到玩家
     * @param data 数据指针
     * @param size 数据大小
     * @return true 如果发送成功
     */
    bool send(const u8* data, size_t size) const {
        auto conn = connection.lock();
        if (conn && conn->isConnected()) {
            conn->send(data, size);
            return true;
        }
        return false;
    }

    /**
     * @brief 获取区块坐标 X
     */
    [[nodiscard]] ChunkCoord chunkX() const {
        return static_cast<ChunkCoord>(std::floor(x / static_cast<f32>(mc::world::CHUNK_WIDTH)));
    }

    /**
     * @brief 获取区块坐标 Z
     */
    [[nodiscard]] ChunkCoord chunkZ() const {
        return static_cast<ChunkCoord>(std::floor(z / static_cast<f32>(mc::world::CHUNK_WIDTH)));
    }

    /**
     * @brief 获取位置向量
     */
    [[nodiscard]] Vector3f position() const {
        return Vector3f(x, y, z);
    }

    /**
     * @brief 获取旋转向量
     */
    [[nodiscard]] Vector2f rotation() const {
        return Vector2f(yaw, pitch);
    }

    // ========== 效果系统 ==========

    /**
     * @brief 添加效果
     * @param effect 效果实例
     * @return true 如果成功添加或合并
     */
    bool addEffect(const entity::effect::EffectInstance& effect) {
        // 查找是否已有同类型效果
        for (auto& existing : effects) {
            if (existing.type() == effect.type()) {
                // 尝试合并效果
                return existing.merge(effect);
            }
        }

        // 没有找到同类型效果，添加新效果
        effects.push_back(effect);
        return true;
    }

    /**
     * @brief 移除效果
     * @param type 效果类型
     */
    void removeEffect(entity::effect::EffectType type) {
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                [type](const entity::effect::EffectInstance& e) {
                    return e.type() == type;
                }),
            effects.end()
        );
    }

    /**
     * @brief 移除所有效果
     */
    void removeAllEffects() {
        effects.clear();
    }

    /**
     * @brief 检查是否有指定效果
     * @param type 效果类型
     * @return true 如果有该效果
     */
    [[nodiscard]] bool hasEffect(entity::effect::EffectType type) const {
        for (const auto& effect : effects) {
            if (effect.type() == type) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 获取效果实例
     * @param type 效果类型
     * @return 效果实例指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const entity::effect::EffectInstance* getEffect(entity::effect::EffectType type) const {
        for (const auto& effect : effects) {
            if (effect.type() == type) {
                return &effect;
            }
        }
        return nullptr;
    }

    /**
     * @brief 获取所有效果
     */
    [[nodiscard]] const std::vector<entity::effect::EffectInstance>& getAllEffects() const {
        return effects;
    }
};

} // namespace mc::server
