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

#include "PlayerRespawnManager.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/spawn/RespawnData.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/PlayerSpawnFinder.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {

/// 维度 ID → (客户端 dimension_type 注册表 id, 维度 ResourceKey)
struct _DimensionWireInfo {
    i32 typeId;
    const char* key;
};

[[nodiscard]] _DimensionWireInfo _dimensionWireInfo(DimensionId id)
{
    switch (id) {
        case DimensionManager::NETHER:
            return {2, "minecraft:the_nether"};
        case DimensionManager::THE_END:
            return {3, "minecraft:the_end"};
        default:
            return {0, "minecraft:overworld"};
    }
}

/// 从玩家实体复制除生命/饥饿/效果/坐标/维度/蜡烛状态之外的全部状态
void _copyPlayerState(Player& target, const Player& source)
{
    target.inventory().copyInventory(source.inventory());
    target.setGameMode(source.gameMode());
    target.abilities() = source.abilities();
    target.experienceManager().setExperience(source.experienceManager().getLevel(),
        source.experienceManager().getProgress(),
        source.experienceManager().getTotalExperience());
    target.experienceManager().setXpSeed(source.experienceManager().getXpSeed());
    target.attributes().copyFrom(source.attributes());
    target.setScore(source.getScore());
    target.copyRespawnPosition(source);
    if (source.getLastDeathLocation().has_value()) {
        target.setLastDeathLocation(source.getLastDeathLocation());
    }
    if (source.getEnteredNetherPosition().has_value()) {
        target.setEnteredNetherPosition(*source.getEnteredNetherPosition());
    }
    if (source.getRecipeBook() != nullptr && target.getRecipeBook() != nullptr) {
        target.getRecipeBook()->copyFrom(*source.getRecipeBook());
    }
}

} // namespace

PlayerRespawnManager::PlayerRespawnManager(MinecraftServer& server)
    : m_server(server)
{}

bool PlayerRespawnManager::isWaitingForRespawn(PlayerId playerId) const
{
    const auto* data = m_server.playerManager().getPlayer(playerId);
    return data != nullptr && data->loggedIn && data->waitingForRespawn;
}

bool PlayerRespawnManager::respawn(PlayerId playerId, bool keepAllData)
{
    auto* data = m_server.playerManager().getPlayer(playerId);
    if (data == nullptr || !data->loggedIn) {
        return false;
    }

    // 旧实体可能已因死亡计时结束而被移除，取得到时在迁移后统一丢弃。
    ServerWorld* oldWorld = m_server.dimensionManager().getPlayerDimensionWorld(playerId) != nullptr
        ? m_server.dimensionManager().getPlayerDimensionWorld(playerId)->world()
        : m_server.getPlayerWorld(playerId);
    Player* oldPlayer =
        oldWorld != nullptr ? m_server.playerEntityManager().getPlayerEntity(playerId, *oldWorld) : nullptr;

    const bool keepInventoryRule = !keepAllData && oldWorld != nullptr &&
        oldWorld->getGameRules().getBoolean(world::gamerule::GameRuleKeys::KEEP_INVENTORY);

    // 落点计算需要旧实体的重生点配置，因此必须先于实体移除执行。
    mc::ServerPlayer::RespawnLocation location;
    if (oldPlayer != nullptr) {
        auto* oldServerPlayer = oldPlayer->asServerPlayer();
        MC_ASSERT_RELEASE(oldServerPlayer != nullptr);
        location = oldServerPlayer->findRespawnPositionAndUseSpawnBlock(!keepAllData);
    } else {
        // 旧实体已消失：用存档中恢复的共享出生点，个人重生点信息此时不可得。
        // TODO: 将个人重生点配置镜像到 ServerPlayerData，使实体移除后的重生请求也能使用床/锚落点。
        ServerDimension* overworld = m_server.dimensionManager().getOverworld();
        MC_ASSERT_RELEASE(overworld != nullptr && overworld->world() != nullptr);
        ServerWorld* spawnWorld = overworld->world();
        const RespawnData adjusted = spawnWorld->worldBorderAdjustedRespawnData();
        location.world = spawnWorld;
        location.position = Vector3d(static_cast<f64>(adjusted.pos().x) + 0.5,
            static_cast<f64>(adjusted.pos().y + 1),
            static_cast<f64>(adjusted.pos().z) + 0.5);
        if (auto found = PlayerSpawnFinder::findSpawn(*spawnWorld, adjusted.pos()); found.has_value()) {
            location.position = Vector3d(found->x, found->y, found->z);
        }
        location.yaw = adjusted.yaw;
        location.pitch = adjusted.pitch;
    }

    ServerWorld* targetWorld = location.world;
    MC_ASSERT_RELEASE(targetWorld != nullptr);

    Player* newPlayer = _createRespawnedPlayer(playerId, *targetWorld, keepAllData, location.missingRespawnBlock);
    if (newPlayer == nullptr) {
        spdlog::error("PlayerRespawnManager: failed to create respawned player entity for {}", playerId);
        return false;
    }

    newPlayer->setPosition(static_cast<f32>(location.position.x),
        static_cast<f32>(location.position.y),
        static_cast<f32>(location.position.z));
    newPlayer->setRotation(location.yaw, location.pitch);
    newPlayer->setDimension(targetWorld->dimension());

    // 重生后除 keepInventory/旁观者/末地返回外，背包与经验清空、生命回满、饥饿与效果重置。
    if (!keepAllData && !keepInventoryRule && (oldPlayer == nullptr || !oldPlayer->isSpectator())) {
        newPlayer->inventory().clear();
        newPlayer->experienceManager().setExperience(0, 0.0f, 0);
    }
    newPlayer->setHealth(newPlayer->maxHealth());
    newPlayer->foodStats().setFoodLevel(20);
    newPlayer->foodStats().setSaturationLevel(5.0f);
    newPlayer->foodStats().setExhaustionLevel(0.0f);
    newPlayer->effectManager().removeAllEffects(*newPlayer);

    if (location.missingRespawnBlock) {
        newPlayer->setRespawnPosition(std::nullopt, false);
    }

    // 新实体已就位并复制完旧实体状态，回收旧实体：解除映射、取消追踪并静默移除。
    if (oldPlayer != nullptr && oldWorld != nullptr) {
        m_server.playerEntityManager().removePlayerEntity(playerId, *oldWorld);
    }

    m_server.dimensionManager().playerJoinDimension(playerId, targetWorld->dimension());

    {
        auto* playerData = m_server.playerManager().getPlayer(playerId);
        MC_ASSERT_RELEASE(playerData != nullptr);
        playerData->x = static_cast<f32>(location.position.x);
        playerData->y = static_cast<f32>(location.position.y);
        playerData->z = static_cast<f32>(location.position.z);
        playerData->yaw = location.yaw;
        playerData->pitch = location.pitch;
        playerData->gameMode = newPlayer->gameMode();
        playerData->restartClientLoadTimerAfterRespawn();
    }

    _sendRespawnPackets(playerId, *newPlayer, *targetWorld);

    if (auto* serverPlayer = newPlayer->asServerPlayer(); serverPlayer != nullptr) {
        serverPlayer->resetAntiFlightBaseline(location.position.x, location.position.y, location.position.z);
    }

    spdlog::info("PlayerRespawnManager: player {} respawned in dimension {} at ({:.1f}, {:.1f}, {:.1f})",
        newPlayer->username(),
        targetWorld->dimension(),
        location.position.x,
        location.position.y,
        location.position.z);
    return true;
}

Player* PlayerRespawnManager::_createRespawnedPlayer(
    PlayerId playerId, ServerWorld& targetWorld, bool keepAllData, bool missingRespawnBlock)
{
    auto* playerData = m_server.playerManager().getPlayer(playerId);
    MC_ASSERT_RELEASE(playerData != nullptr);

    // 旧实体若仍存活，先把其数据迁移过来（落点在旧实体上计算完毕）。
    ServerWorld* oldWorld = m_server.getPlayerWorld(playerId);
    Player* oldPlayer =
        oldWorld != nullptr ? m_server.playerEntityManager().getPlayerEntity(playerId, *oldWorld) : nullptr;
    if (oldPlayer != nullptr) {
        oldPlayer->stopRiding();
        if (auto* oldServerPlayer = oldPlayer->asServerPlayer(); oldServerPlayer != nullptr) {
            oldServerPlayer->resetCamera();
        }
    }

    // 新实体不持有连接：出站统一走服务器门面按 PlayerId 路由（远程查 PlayerManager 的
    // 连接对象，本地走集成服的本地发送钩子），避免重生流程依赖会话层。
    mc::server::net::ServerClientConnection* reuseConnection = nullptr;

    // 旧实体状态先快照到 ServerPlayer 移植所需的字段由 createPlayerEntity 之后复制；这里保留旧实体到
    // 新实体创建完成后再移除（见 respawn() 的回收步骤），避免提前析构导致的状态丢失。

    Player* newPlayer = m_server.playerEntityManager().createPlayerEntity(playerId,
        playerData->username,
        targetWorld,
        &m_server,
        reuseConnection,
        static_cast<f32>(playerData->x),
        static_cast<f32>(playerData->y),
        static_cast<f32>(playerData->z));
    if (newPlayer == nullptr) {
        return nullptr;
    }

    auto* newServerPlayer = newPlayer->asServerPlayer();
    MC_ASSERT_RELEASE(newServerPlayer != nullptr);
    newServerPlayer->setServer(&m_server);
    newServerPlayer->setConnection(reuseConnection);
    newPlayer->setUuid(playerData->uuid);
    newPlayer->setPermissionLevel(m_server.resolveOpLevel(playerData->uuid));
    newServerPlayer->setOnline(true);

    if (oldPlayer != nullptr) {
        _copyPlayerState(*newPlayer, *oldPlayer);
    } else {
        // 旧实体已消失：从服务器侧的玩家数据恢复可见状态。
        newPlayer->setGameMode(playerData->gameMode);
    }
    MC_UNUSED(keepAllData);
    MC_UNUSED(missingRespawnBlock);
    return newPlayer;
}

void PlayerRespawnManager::_sendRespawnPackets(PlayerId playerId, Player& player, ServerWorld& world)
{
    const DimensionId dimensionId = world.dimension();
    const _DimensionWireInfo wire = _dimensionWireInfo(dimensionId);

    mc::network::ir::play::Respawn respawn;
    respawn.spawnInfo.dimensionType = wire.typeId;
    respawn.spawnInfo.dimension = wire.key;
    respawn.spawnInfo.seed = static_cast<i64>(world.seed());
    respawn.spawnInfo.gameType = player.gameMode();
    respawn.spawnInfo.previousGameType = -1;
    respawn.spawnInfo.isDebug = world.isDebugWorld();
    respawn.spawnInfo.isFlat = dimensionId == DimensionManager::OVERWORLD && world.isFlatWorld();
    if (const auto death = player.getLastDeathLocation(); death.has_value()) {
        respawn.spawnInfo.lastDeathLocation =
            std::make_pair(std::string(dimensionIdToString(death->getDimensionId())), death->getPos().asLong());
    }
    respawn.spawnInfo.portalCooldown = 0;
    respawn.spawnInfo.seaLevel = mc::world::SEA_LEVEL;
    // 普通死亡清空客户端实体数据（位置/速度/朝向由后续包重建）。属性修饰符是否有客户端缓存
    // 语义尚待确认，这里保守地不保留（0），避免把死亡瞬间的瞬时修饰符带到重生后。
    respawn.dataToKeep = 0;

    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(respawn)}});

    // 坐标 + 区块加载起点。
    m_server.teleportManager().requestTeleport(
        playerId, player.position().x, player.position().y, player.position().z, player.yaw(), player.pitch());

    mc::network::ir::play::GameEvent loadStart;
    loadStart.event = 13; // LEVEL_CHUNKS_LOAD_START
    loadStart.value = 0.0f;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(loadStart)}});

    // 世界出生点、经验、药水效果、难度、权限与命令树。
    const RespawnData spawn = world.worldBorderAdjustedRespawnData();
    mc::network::ir::play::SetDefaultSpawnPosition spawnPacket;
    spawnPacket.dimension = std::string(dimensionIdToString(spawn.dimension()));
    spawnPacket.blockPosPacked = spawn.pos().asLong();
    spawnPacket.yaw = spawn.yaw;
    spawnPacket.pitch = spawn.pitch;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(spawnPacket)}});

    world.entityTracker().sendActiveEffects(m_server, playerId, player);
    if (auto* serverPlayer = player.asServerPlayer(); serverPlayer != nullptr) {
        serverPlayer->syncExperience();
    }
    // 区块追踪建立与首包推送由服务器 tick 中的玩家位置更新完成；这里只刷新目标维度的票据来源位置，
    // 使重生点周围的区块按新坐标加载。
    world.chunkManager()->updatePlayerPosition(playerId, player.position().x, player.position().z);
}

} // namespace mc::server