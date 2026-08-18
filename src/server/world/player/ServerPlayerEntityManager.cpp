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

#include "ServerPlayerEntityManager.hpp"
#include "../../world/ServerWorld.hpp"
#include "../../world/entity/EntityTracker.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IServer.hpp"
#include "server/network/ServerNetwork.hpp"
#include "server/player/ServerPlayer.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::server {

Player* ServerPlayerEntityManager::createPlayerEntity(PlayerId playerId,
    const std::string& username,
    ServerWorld& world,
    IServer* server,
    mc::server::net::ServerClientConnection* connection,
    f32 spawnX,
    f32 spawnY,
    f32 spawnZ)
{
    MC_ASSERT_RELEASE(playerId != 0);
    MC_ASSERT_RELEASE(server != nullptr);

    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    if (m_playerToEntity.find(playerId) != m_playerToEntity.end()) {
        spdlog::warn("ServerPlayerEntityManager: Player {} already exists", playerId);
        return nullptr;
    }

    // 创建服务端玩家实体（EntityInstanceId 暂时为 0，由 EntityManager 分配）
    // 使用 mc::ServerPlayer（而非基类 Player），以便：
    // - PlayerAdvancements、StatisticsManager、ServerRecipeBook 等服务端状态就位
    // - Player::asServerPlayer() 返回有效指针，打通成就触发、命令系统、选择器等路径
    // 注意：必须使用 mc::ServerPlayer 全限定名，避免被 mc::server::ServerPlayer
    // （StatisticsManager.hpp 中的前向声明）错误遮蔽。
    // ECS 迁移：ServerPlayer 构造需要 registry 句柄，从传入的 ServerWorld 取。
    auto* ecsRegistry = world.entityRegistry();
    if (ecsRegistry == nullptr) {
        spdlog::error("ServerPlayerEntityManager: World has no entity registry for {}", username);
        return nullptr;
    }
    auto player = std::make_unique<mc::ServerPlayer>(0, username, *ecsRegistry);
    if (!player) {
        spdlog::error("ServerPlayerEntityManager: Failed to create ServerPlayer for {}", username);
        return nullptr;
    }

    // 设置玩家ID
    player->setPlayerId(playerId);

    // 设置初始位置
    player->setPosition(spawnX, spawnY, spawnZ);

    // 注入服务端上下文：必须在 spawnEntity 之前完成，
    // 这样实体一进入 EntityManager 就是完整初始化状态，
    // 避免其他系统在 setServer/setWorld/setConnection 完成前访问到未就绪的 ServerPlayer。
    // - setServer：末影箱自动保存回调依赖 m_server（见 ServerPlayer::setupInventoryCallback）
    // - setWorld：ServerPlayer::m_world 独立于 Entity::m_world，需显式设置
    // - setConnection：网络发包方法依赖 m_connection
    player->setServer(server);
    player->setWorld(&world);
    player->setConnection(connection);

    // 加入世界实体池（EntityManager 会分配 EntityInstanceId）
    EntityInstanceId entityId = world.spawnEntity(std::move(player));
    if (entityId == INVALID_ENTITY_ID) {
        spdlog::error("ServerPlayerEntityManager: Failed to spawn player entity for {}", username);
        return nullptr;
    }

    // 获取加入后的实体指针
    Entity* entity = world.entityManager().getEntity(entityId);
    if (!entity) {
        spdlog::error("ServerPlayerEntityManager: Entity not found after spawn for {}", username);
        return nullptr;
    }

    Player* playerPtr = dynamic_cast<Player*>(entity);
    if (!playerPtr) {
        spdlog::error("ServerPlayerEntityManager: Entity is not a Player for {}", username);
        world.removeEntity(entityId);
        return nullptr;
    }

    // 加入实体追踪（开始同步给其他玩家）
    world.entityTracker().trackEntity(playerPtr);

    // 建立映射
    m_playerToEntity[playerId] = entityId;
    m_entityToPlayer[entityId] = playerId;

    spdlog::info(
        "ServerPlayerEntityManager: Created player {} (PlayerId={}, EntityInstanceId={}) at ({:.1f}, {:.1f}, {:.1f})",
        username,
        playerId,
        entityId,
        spawnX,
        spawnY,
        spawnZ);

    return playerPtr;
}

bool ServerPlayerEntityManager::registerExistingPlayerEntity(
    PlayerId playerId, EntityInstanceId entityId, ServerWorld& world)
{
    MC_ASSERT_RELEASE(playerId != 0);

    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查 PlayerId 是否已注册
    if (m_playerToEntity.find(playerId) != m_playerToEntity.end()) {
        spdlog::warn("ServerPlayerEntityManager: Player {} already exists", playerId);
        return false;
    }

    // 确认实体存在且是 Player（SimulatedPlayer 是 ServerPlayer 子类，dynamic_cast 成功）
    Entity* entity = world.entityManager().getEntity(entityId);
    if (entity == nullptr) {
        spdlog::error("ServerPlayerEntityManager: Entity {} not found for player {}", entityId, playerId);
        return false;
    }
    Player* playerPtr = dynamic_cast<Player*>(entity);
    if (playerPtr == nullptr) {
        spdlog::error("ServerPlayerEntityManager: Entity {} is not a Player for player {}", entityId, playerId);
        return false;
    }

    // 加入实体追踪（与其他玩家同步；SimulatedPlayer 无连接时发包路径 no-op，安全）
    world.entityTracker().trackEntity(playerPtr);

    // 建立双向映射
    m_playerToEntity[playerId] = entityId;
    m_entityToPlayer[entityId] = playerId;

    spdlog::info("ServerPlayerEntityManager: Registered existing player {} (PlayerId={}, EntityInstanceId={})",
        playerPtr->username(),
        playerId,
        entityId);

    return true;
}

void ServerPlayerEntityManager::removePlayerEntity(PlayerId playerId, ServerWorld& world)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playerToEntity.find(playerId);
    if (it == m_playerToEntity.end()) {
        spdlog::warn("ServerPlayerEntityManager: Player {} not found for removal", playerId);
        return;
    }

    EntityInstanceId entityId = it->second;

    // 从实体追踪器移除
    world.entityTracker().untrackEntity(entityId);

    // 从世界实体池移除
    world.removeEntity(entityId);

    // 清除映射
    m_playerToEntity.erase(it);
    m_entityToPlayer.erase(entityId);

    spdlog::info("ServerPlayerEntityManager: Removed player {} (EntityInstanceId={})", playerId, entityId);
}

void ServerPlayerEntityManager::clearAll(ServerWorld& world)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 移除所有玩家实体
    for (const auto& [playerId, entityId] : m_playerToEntity) {
        world.entityTracker().untrackEntity(entityId);
        world.removeEntity(entityId);
    }

    m_playerToEntity.clear();
    m_entityToPlayer.clear();

    spdlog::info("ServerPlayerEntityManager: Cleared all player entities");
}

EntityInstanceId ServerPlayerEntityManager::getPlayerEntityId(PlayerId playerId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playerToEntity.find(playerId);
    if (it == m_playerToEntity.end()) {
        return INVALID_ENTITY_ID;
    }
    return it->second;
}

PlayerId ServerPlayerEntityManager::getPlayerIdByEntityId(EntityInstanceId entityId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entityToPlayer.find(entityId);
    if (it == m_entityToPlayer.end()) {
        return 0;
    }
    return it->second;
}

Player* ServerPlayerEntityManager::getPlayerEntity(PlayerId playerId, ServerWorld& world) const
{
    EntityInstanceId entityId = getPlayerEntityId(playerId);
    if (entityId == INVALID_ENTITY_ID) {
        return nullptr;
    }

    Entity* entity = world.entityManager().getEntity(entityId);
    if (!entity) {
        return nullptr;
    }

    return dynamic_cast<Player*>(entity);
}

bool ServerPlayerEntityManager::hasPlayer(PlayerId playerId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playerToEntity.find(playerId) != m_playerToEntity.end();
}

size_t ServerPlayerEntityManager::playerCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playerToEntity.size();
}

std::vector<PlayerId> ServerPlayerEntityManager::getPlayerIds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<PlayerId> ids;
    ids.reserve(m_playerToEntity.size());
    for (const auto& [playerId, entityId] : m_playerToEntity) {
        ids.push_back(playerId);
    }
    return ids;
}

} // namespace mc::server
