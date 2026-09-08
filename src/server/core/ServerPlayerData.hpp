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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include "server/network/IServerClientConnection.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
class AbstractContainerMenu; // 前向声明
class PlayerInventory;       // 前向声明
} // namespace mc

namespace mc::server {
class PlayerAdvancements; // 前向声明
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
    /// 玩家ID（服务器分配的会话ID）
    PlayerId playerId = 0;

    /// 玩家唯一标识符（持久化ID，用于存档）
    std::string uuid;

    /// 用户名
    std::string username;

    /// 连接（业务侧仅依赖 IServerClientConnection 接口；非拥有，由
    /// ServerNetwork/PlayerManager 持有具体 ServerClientConnection）。
    /// nullptr 表示本地玩家（IntegratedServer 单玩家优化：连接由 IntegratedServer 直接持有）
    mc::server::net::IServerClientConnection* connection = nullptr;

    /// 会话ID（用于 TCP 连接标识）
    u32 sessionId = 0;

    /// IP 地址（从连接获取，本地连接为空字符串）
    std::string ipAddress;

    /// 登录状态
    bool loggedIn = false;

    /// 区块追踪器
    std::shared_ptr<network::PlayerChunkTracker> chunkTracker;

    // 位置（内部使用 f32，网络边界使用 f64）
    f32 x = 0.0f;
    f32 y = static_cast<f32>(world::SEA_LEVEL) + 1.0f; // 默认出生高度：海平面+1
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
    u64 lastKeepAliveSentTick = 0; // 发送心跳时的 tick
    u32 ping = 0;                  // 延迟（毫秒）

    // ========== 客户端加载状态（对齐 ServerGamePacketListenerImpl） ==========
    /// 玩家死亡后等待重生（对齐 waitingForRespawn）。
    /// 置位后 hasClientLoaded() 返回 false，直到 PERFORM_RESPAWN 触发 restartClientLoadTimerAfterRespawn()。
    /// 对齐 vanilla ServerGamePacketListenerImpl.markClientUnloadedAfterDeath()。
    bool waitingForRespawn = false;
    /// 客户端加载超时计时器（对齐 clientLoadedTimeoutTimer），每 tick 递减。
    i32 clientLoadedTimeoutTimer = 0;

    // 已加载的区块
    std::unordered_set<ChunkId> loadedChunks;

    // 效果系统
    std::vector<entity::effect::EffectInstance> effects;

    // 容器相关（使用原始指针避免 incomplete type 问题）
    mc::AbstractContainerMenu* openMenu = nullptr;
    ContainerType openContainerType = ContainerType::Player;
    ContainerId nextContainerId = 1;

    // 玩家物品栏（containerId=0）的 stateId 计数器。containerId=0 在服务端无独立
    // AbstractContainerMenu 实例（直接用 PlayerInventory），故 stateId 在此承载。
    // 每次 syncToClient/_sendPlayerInventory 自增并随 ContainerSetContent(containerId=0) 下发，
    // 客户端收包写回其 InventoryCraftingMenu/ItemPickerMenu 的 stateId，出站点击回填。
    // mutable：同步令牌，PlayerManager::getPlayer 返回 const 指针，出站回调须能自增。
    mutable i32 playerInventoryStateId = 0;

    // 成就进度（弃用：请通过 ServerPlayer::getAdvancements() 获取 PlayerAdvancements，
    // 该路径通过 ServerPlayerEntityManager → Player::asServerPlayer() 正确获取。
    // 此字段始终为 nullptr，将在未来版本中移除。）
    std::shared_ptr<PlayerAdvancements> advancements;

    ServerPlayerData() = default;

    /**
     * @brief 构造玩家数据
     * @param id 玩家ID
     * @param name 用户名
     */
    explicit ServerPlayerData(PlayerId id, const std::string& name)
        : playerId(id)
        , username(name)
    {}

    /**
     * @brief 获取连接（如果有效）
     * @return 连接指针，如果无连接返回 nullptr
     */
    [[nodiscard]] mc::server::net::IServerClientConnection* getConnection() const noexcept { return connection; }

    /**
     * @brief 检查连接是否有效
     * @return true 如果连接非空且未断开
     */
    [[nodiscard]] bool hasConnection() const { return connection != nullptr && connection->isConnected(); }

    /**
     * @brief 自增并返回玩家物品栏（containerId=0）的 stateId（& 32767 环绕）
     * @return 自增后的 stateId
     *
     * 服务端在构造 ContainerSetContent(containerId=0) 出站包时调用。返回值填入包的 stateId 字段下发。
     * const 方法：stateId 是 mutable 同步令牌，const 玩家数据引用下出站回调须能自增。
     */
    [[nodiscard]] i32 incrementPlayerInventoryStateId() const noexcept
    {
        playerInventoryStateId = (playerInventoryStateId + 1) & 32767;
        return playerInventoryStateId;
    }

    /**
     * @brief 获取玩家物品栏（containerId=0）的当前 stateId
     */
    [[nodiscard]] i32 getPlayerInventoryStateId() const noexcept { return playerInventoryStateId; }

    /**
     * @brief 发送 IR 包到玩家
     * @param packet IR 包（按值移动）
     * @return true 如果发送成功（连接有效且 send 成功）
     */
    bool send(mc::network::ir::IrPacket packet) const
    {
        if (connection != nullptr && connection->isConnected()) {
            auto r = connection->send(std::move(packet));
            return r.success();
        }
        return false;
    }

    /**
     * @brief 获取区块坐标 X
     */
    [[nodiscard]] ChunkCoord chunkX() const noexcept
    {
        return static_cast<ChunkCoord>(std::floor(x / static_cast<f32>(mc::world::CHUNK_WIDTH)));
    }

    /**
     * @brief 获取区块坐标 Z
     */
    [[nodiscard]] ChunkCoord chunkZ() const noexcept
    {
        return static_cast<ChunkCoord>(std::floor(z / static_cast<f32>(mc::world::CHUNK_WIDTH)));
    }

    // ========== 客户端加载状态（对齐 ServerGamePacketListenerImpl） ==========

    /**
     * @brief 玩家死亡后标记客户端已卸载，等待重生
     *
     * 对齐 vanilla ServerGamePacketListenerImpl.markClientUnloadedAfterDeath()：
     * 仅置 waitingForRespawn = true。该标志使 hasClientLoaded() 返回 false，
     * 直到玩家执行 PERFORM_RESPAWN 触发 restartClientLoadTimerAfterRespawn() 才被清除。
     */
    void markClientUnloadedAfterDeath() noexcept { waitingForRespawn = true; }

    /**
     * @brief 重生后重启客户端加载超时计时器
     *
     * 对齐 vanilla ServerGamePacketListenerImpl.restartClientLoadTimerAfterRespawn()：
     * 清除 waitingForRespawn，并启动 60 tick 的客户端加载超时计时器。
     */
    void restartClientLoadTimerAfterRespawn() noexcept
    {
        waitingForRespawn = false;
        clientLoadedTimeoutTimer = 60;
    }

    /**
     * @brief 每 tick 递减客户端加载超时计时器
     *
     * 对齐 vanilla ServerGamePacketListenerImpl.tickClientLoadTimeout()。
     */
    void tickClientLoadTimeout() noexcept
    {
        if (clientLoadedTimeoutTimer > 0) {
            --clientLoadedTimeoutTimer;
        }
    }

    /**
     * @brief 检查客户端是否已加载完成
     *
     * 对齐 vanilla ServerGamePacketListenerImpl.hasClientLoaded()：
     * 未等待重生（!waitingForRespawn）且加载超时计时器已归零。
     */
    [[nodiscard]] bool hasClientLoaded() const noexcept { return !waitingForRespawn && clientLoadedTimeoutTimer <= 0; }

    /**
     * @brief 获取位置向量
     */
    [[nodiscard]] Vector3f position() const noexcept { return Vector3f(x, y, z); }

    /**
     * @brief 获取旋转向量
     */
    [[nodiscard]] Vector2f rotation() const noexcept { return Vector2f(yaw, pitch); }

    // ========== 效果系统 ==========

    /**
     * @brief 添加效果
     * @param effect 效果实例
     * @return true 如果成功添加或合并
     */
    bool addEffect(const entity::effect::EffectInstance& effect)
    {
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
    void removeEffect(entity::effect::EffectType type)
    {
        effects.erase(std::remove_if(effects.begin(),
                          effects.end(),
                          [type](const entity::effect::EffectInstance& e) { return e.type() == type; }),
            effects.end());
    }

    /**
     * @brief 移除所有效果
     */
    void removeAllEffects() noexcept { effects.clear(); }

    /**
     * @brief 检查是否有指定效果
     * @param type 效果类型
     * @return true 如果有该效果
     */
    [[nodiscard]] bool hasEffect(entity::effect::EffectType type) const
    {
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
    [[nodiscard]] const entity::effect::EffectInstance* getEffect(entity::effect::EffectType type) const
    {
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
    [[nodiscard]] const std::vector<entity::effect::EffectInstance>& getAllEffects() const noexcept { return effects; }
};

} // namespace mc::server
