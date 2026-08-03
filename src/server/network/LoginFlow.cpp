/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "LoginFlow.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/backend/java/codecs/CommandTreeEncoder.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/ServerPlayHandler.hpp"
#include "server/sync/WeatherSyncService.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

LoginFlow::PlayerCreationResult LoginFlow::createPlayerForConnection(
    mc::server::net::ServerClientConnection& connection,
    const std::string& username,
    const std::array<u8, 16>& offlineUuid,
    bool hardcore,
    i64 seed,
    bool isFlat)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "LoginFlow::createPlayerForConnection", "username", username);

    PlayerCreationResult result{};
    result.playerId = 0;
    result.entityId = 0;
    result.success = false;

    spdlog::info("Player '{}' attempting to join (handshake complete)", username);

    // 分配玩家ID（握手完成后玩家才真正存在）
    const PlayerId playerId = m_server.playerManager().nextPlayerId();
    result.playerId = playerId;

    const std::string uuidStr = util::uuidToString(offlineUuid);

    // 添加玩家会话信息（连接为该玩家的出站通道，非拥有指针）
    auto* playerData = m_server.playerManager().addPlayer(playerId, uuidStr, username, &connection);
    if (playerData == nullptr) {
        spdlog::error("Failed to add player '{}' to PlayerManager", username);
        return result;
    }

    // 建立 sessionId→playerId 映射并回填 ServerPlayerData::sessionId。
    // 必要性：远程客户端断连回调 RemoteSessionManager::onClientDisconnect 按 sessionId 反查 playerId
    // 来清理玩家；若不建立映射，getPlayerIdBySession 恒返回 0，断连时跳过 removePlayer，玩家记录残留
    // 且 connection 指针随 ServerClientConnection 析构悬垂，下个 tick 广播（如 broadcastLightUpdate）
    // 解悬垂致 ACCESS_VIOLATION 崩溃。
    // sessionId 0 是集成服本地客户端保留值（不触发远程断连回调，其生命周期由 IntegratedServer 直接
    // 管理），跳过映射避免 m_sessionToPlayer 残留无意义条目。
    const u32 sessionId = connection.sessionId();
    playerData->sessionId = sessionId;
    if (sessionId != 0) {
        m_server.playerManager().mapSessionToPlayer(sessionId, playerId);
    }

    // 初始化 KeepAlive 基准时间（wall-clock 毫秒）：
    // - lastKeepAliveSent=now：避免玩家一加入就因 (now - 0) >= 15s 立即发首包（vanilla 是加入后
    //   等一个间隔才发）。tickKeepAlive 每tick 调 getPlayersNeedingKeepAlive，无此初始化则新玩家
    //   首 tick 即被选中发送，与原版"加入 15s 后才发"语义不符。
    // - lastKeepAliveReceived=now：getTimedOutPlayers 的守卫为 lastReceived>0 && (now-lastReceived)
    //   >=30s。若不初始化（保持 0），守卫 lastReceived>0 恒假，玩家即使不回包也永不被判超时（隐患）。
    //   初始化为 now 后，玩家加入 30s 内不误判超时，且若一直不回包则加入 30s 后正确判超时踢出。
    const u64 nowMs = util::TimeUtils::getCurrentTimeMs();
    playerData->lastKeepAliveSent = nowMs;
    playerData->lastKeepAliveReceived = nowMs;

    // 设置玩家初始状态
    setupInitialPlayerState(playerData, static_cast<GameMode>(m_server.settings().defaultGameMode.get()));

    // 创建玩家实体并加入世界（玩家始终在主世界生成）
    auto* overworld = m_server.dimensionManager().getOverworld();
    MC_ASSERT_RELEASE(overworld != nullptr && overworld->world() != nullptr);
    Player* playerEntity = m_server.playerEntityManager().createPlayerEntity(playerId,
        username,
        *overworld->world(),
        &m_server,
        &connection,
        static_cast<f32>(playerData->x),
        static_cast<f32>(playerData->y),
        static_cast<f32>(playerData->z));

    if (playerEntity == nullptr) {
        spdlog::error("Failed to create player entity for {}", username);
        m_server.playerManager().removePlayer(playerId);
        return result;
    }

    // 回填玩家实体的 profile UUID（离线模式 = generateOfflineUuid(username)）。
    // 必要性：TameableEntity::getOwner() 按 UUID 匹配主人（对齐 vanilla TamableAnimal.getOwnerReference），
    // 需 Player::uuid() 等于 profile UUID。createPlayerEntity 构造的实体 m_uuid 是随机值，
    // 不回填则驯服后的狼/猫/鹦鹉螺无法跟随主人、无法继承队伍。PlayerDataManager::applyToPlayer
    // 不涉及 uuid（仅恢复位置/维度/游戏模式等），故此处回填安全不会被覆盖。
    playerEntity->setUuid(uuidStr);

    // 登录阶段必须先建立玩家维度映射，否则 TeleportConfirm 回来后无法解析玩家所在世界。
    m_server.dimensionManager().playerJoinDimension(playerId, overworld->id());
    result.entityId = playerEntity->id();

    // 从 OP 列表设置玩家权限等级 + 从存档加载玩家数据恢复到实体
    const i32 playerPermissionLevel = m_server.resolveOpLevel(playerData->uuid);
    if (auto* world = m_server.getPlayerWorld(playerId)) {
        if (Player* player = m_server.playerEntityManager().getPlayerEntity(playerId, *world)) {
            player->setPermissionLevel(playerPermissionLevel);

            auto* storage = m_server.sharedStorage();
            if (storage != nullptr) {
                auto loadResult = storage->loadPlayer(playerData->uuid);
                if (loadResult.success() && loadResult.value().has_value()) {
                    const auto& saveData = loadResult.value().value();
                    world::storage::PlayerDataManager::applyToPlayer(*player, saveData);
                    spdlog::info("Player '{}' loaded saved data (level {}, gameMode {})",
                        playerData->username,
                        saveData.experienceLevel,
                        static_cast<i32>(saveData.gameMode));
                }
            }
        }
    }

    // 发送 play::Login（post-Configuration S→C，经 sendPacketToPlayer 按 playerId 路由）
    sendLoginResponseForConnection(playerId, hardcore, seed, isFlat);

    // 同步玩家权限等级到客户端（同时发送 EntityEvent 和命令树）
    sendPermissionLevelChange(playerId, playerPermissionLevel);

    // 发送初始游戏状态
    sendInitialGameState(playerId, playerData->x, playerData->y, playerData->z, playerData->yaw, playerData->pitch);

    result.success = true;
    spdlog::info("Player '{}' (PlayerId={}, EntityInstanceId={}) joined the game", username, playerId, result.entityId);
    return result;
}

void LoginFlow::setupInitialPlayerState(ServerPlayerData* player, GameMode gameMode)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Player, "LoginFlow::setupInitialPlayerState", "gameMode", static_cast<i32>(gameMode));

    if (!player) return;

    // 获取世界出生点（从主世界维度获取）
    Vector3d spawnPoint(0.0, 64.0, 0.0); // 默认值
    auto* overworld = m_server.dimensionManager().getOverworld();
    if (overworld && overworld->world()) {
        spawnPoint = overworld->world()->worldSpawnPoint();
    }

    // 设置初始位置
    player->x = static_cast<f32>(spawnPoint.x);
    player->y = static_cast<f32>(spawnPoint.y);
    player->z = static_cast<f32>(spawnPoint.z);

    // 设置游戏状态
    player->loggedIn = true;
    player->gameMode = gameMode;
}

void LoginFlow::sendInitialGameState(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Player, "LoginFlow::sendInitialGameState", "playerId", playerId, "x", x, "y", y, "z", z);

    // TeleportManager::requestTeleport() 内部已经发送过 TeleportPacket。
    // 这里不能重复发送，否则客户端会在登录阶段收到两个相同 teleportId 的传送包，
    // 紧接着回两次 TeleportConfirm，第二次会因为服务端已清除 waitingTeleportConfirm
    // 而被当作无效确认，进而打断首次区块加载时序。
    m_server.teleportManager().requestTeleport(playerId, x, y, z, yaw, pitch);

    // 立即发送时间，避免客户端在首次周期同步前短暂显示默认时间(0)
    // 使用玩家当前维度的时间（下界=18000，末地=6000，主世界=实际时间）
    const auto& time = m_server.timeManager().gameTimeObj();
    i64 dayTime = time.dayTimeOfDay(); // 默认使用主世界时间

    // 获取玩家当前维度并使用维度特定时间
    auto* playerDim = m_server.dimensionManager().getPlayerDimensionWorld(playerId);
    if (playerDim && playerDim->world()) {
        dayTime = playerDim->world()->dayTime();
    }

    mc::network::ir::play::SetTime timePkt;
    timePkt.gameTime = time.gameTime();
    timePkt.dayTime = dayTime;
    timePkt.tickDayTime = time.daylightCycleEnabled();
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(timePkt)},
        });

    // 发送世界出生点（指南针指向位置，从主世界维度获取）
    // 1.21.11 SetDefaultSpawnPosition：dimension + blockPosPacked + yaw + pitch
    Vector3d worldSpawn(0.0, 64.0, 0.0);
    f32 spawnAngle = 0.0f;
    auto* overworld = m_server.dimensionManager().getOverworld();
    if (overworld && overworld->world()) {
        worldSpawn = overworld->world()->worldSpawnPoint();
        spawnAngle = overworld->world()->spawnAngle();
    }
    mc::network::ir::play::SetDefaultSpawnPosition spawnPkt;
    spawnPkt.dimension = "minecraft:overworld";
    spawnPkt.blockPosPacked = BlockPos(static_cast<BlockCoord>(worldSpawn.x),
        static_cast<BlockCoord>(worldSpawn.y),
        static_cast<BlockCoord>(worldSpawn.z))
                                  .asLong();
    spawnPkt.yaw = spawnAngle;
    spawnPkt.pitch = 0.0f;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(spawnPkt)},
        });

    // 发送初始天气状态
    m_server.weatherSyncService().sendInitialWeatherStateToPlayer(playerId);

    // 发送初始难度状态
    sendInitialDifficultyToPlayer(playerId);

    // 通知客户端开始接收区块加载包序列（event=13 LEVEL_CHUNKS_LOAD_START，value=0）。
    // 1.21.11 客户端 LevelLoadTracker 状态机：handleLogin 后处于 WaitingForServer 并显示
    // LevelLoadingScreen（“加载地形中”）；唯有收到此 GameEvent 才调用 loadingPacketsReceived()
    // 转入 WaitingForPlayerChunk，进而等玩家所在 section 编译可见后关屏。漏发则客户端永久卡在
    // WaitingForServer，仅靠 30s 超时兜底强制放行。发送时机位于天气/难度包之后、作为区块加载
    // 序列开始的最后信号（与原版登录序列中该包的位置一致）。
    {
        mc::network::ir::play::GameEvent loadStartEvt;
        loadStartEvt.event = 13; // LEVEL_CHUNKS_LOAD_START
        loadStartEvt.value = 0.0f;
        m_server.sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(loadStartEvt)},
            });
    }

    m_server.playHandler().updateEntityTrackingForPlayer(playerId, x, y, z);

    // 登录时主动建立玩家区块追踪并触发区块推送，不依赖客户端回 AcceptTeleportation。
    // 原版在 PlayerList.placeNewPlayer → serverlevel.addNewPlayer → ChunkMap.updatePlayerStatus
    // 中由“玩家加入世界”事件建立区块追踪（设置 SetChunkCacheCenter 中心、把视野内区块加入发送
    // 队列），与 teleport 确认无关。本项目此前把区块追踪建立推迟到 handleTeleportConfirmPacket
    // （依赖客户端回 AcceptTeleportation 且 teleportId 精确匹配），本地集成客户端因同进程回环
    // 极快能正常建立；真 Java 客户端若回包时序或 id 不匹配则追踪永不建立，服务端一个
    // LevelChunkWithLight 都不发，客户端 LevelLoadTracker 第二闸门（isSectionCompiledAndVisible）
    // 永远过不去，卡在“加载地形中”直至 30s 超时。此处对齐原版在登录末尾主动建立追踪：
    // updatePlayerPosition 构建玩家视野追踪集合 → processTicketUpdatesSync 同步生成/加载 spawn
    // 周围区块 → onPlayerTrackingChange 回调 → ChunkSendManager 把区块加入发送队列，后续 tick
    // 由 processPendingSends 推送。updatePlayerPosition 幂等，后续 AcceptTeleportation 重复触发无害。
    auto* initialWorld = m_server.getPlayerWorld(playerId);
    if (initialWorld && initialWorld->chunkManager()) {
        initialWorld->chunkManager()->updatePlayerPosition(playerId, x, z);
        initialWorld->chunkManager()->processTicketUpdatesSync();
    }
}

void LoginFlow::sendLoginResponseForConnection(PlayerId playerId, bool hardcore, i64 seed, bool isFlat)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Network, "LoginFlow::sendLoginResponseForConnection", "playerId", playerId);

    // 发送 play::Login（post-Configuration S→C，id=48），经 sendPacketToPlayer 按 playerId 路由
    // （本地→m_clientConnection，远程→player->send）。world 参数由调用方提供。
    auto* overworldForLogin = m_server.dimensionManager().getOverworld();
    const bool isDebugWorld =
        overworldForLogin && overworldForLogin->world() && overworldForLogin->world()->isDebugWorld();

    mc::network::ir::play::Login login;
    login.playerId = static_cast<i32>(playerId);
    login.hardcore = hardcore;
    // levels：维度 ResourceKey 列表（主世界/下界/末地）
    login.levels = {"minecraft:overworld", "minecraft:the_nether", "minecraft:the_end"};
    login.maxPlayers = m_server.settings().maxPlayers.get();
    login.chunkRadius = m_server.settings().viewDistance.get();
    login.simulationDistance = m_server.settings().simulationDistance.get();
    login.reducedDebugInfo = false;
    login.showDeathScreen = true;
    login.doLimitedCrafting = false;

    auto& spawn = login.spawnInfo;
    // dimensionType 为 dimension_type 注册表 holder id（纯 VarInt）。Login 初始进入
    // 主世界，overworld 在 RegistryDataBuilder 下发顺序中 id=0。
    spawn.dimensionType = 0;
    spawn.dimension = "minecraft:overworld";
    spawn.seed = seed;
    spawn.gameType = static_cast<GameMode>(m_server.settings().defaultGameMode.get());
    spawn.previousGameType = -1;
    spawn.isDebug = isDebugWorld;
    spawn.isFlat = isFlat;
    spawn.lastDeathLocation = std::nullopt;
    spawn.portalCooldown = 0;
    spawn.seaLevel = mc::world::SEA_LEVEL;
    login.enforcesSecureChat = false;

    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(login)}});
}

void LoginFlow::sendCommandTreePacket(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "LoginFlow::sendCommandTreePacket", "playerId", playerId);

    // commandRegistry() 经 unique_ptr 解引用返回引用（initializeCoreManagers 中 make_unique，
    // 仅 shutdownManagers 中 reset），登录流程阶段恒非空，无需 nullptr 断言。

    // 1.21.11 ClientboundCommandsPacket：二进制 CommandNode 树。对齐 Java 线格式
    // （VarInt(nodeCount) + nodes + VarInt(rootIndex)，每节点 flags/children/redirect/stub）。
    // 旧实现把命令树 JSON 文本当 opaque payload 透传，真 Java 客户端按二进制解码必崩
    // （disconnect-2026-07-29_17.24.34 "Non [a-z0-9_.-] character in namespace"）。
    mc::network::ir::play::Commands pkt;
    auto snapshot = m_server.commandRegistry().getCommandTreeSnapshot();
    auto encoded = mc::network::java::codecs::encodeCommandTree(snapshot);
    if (!encoded.success()) {
        spdlog::error("CommandTreeEncoder: failed to encode command tree: {}", encoded.error().toString());
        return;
    }
    pkt.payload = std::move(encoded.value());
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

void LoginFlow::sendPermissionLevelChange(PlayerId playerId, i32 permissionLevel)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
        "LoginFlow::sendPermissionLevelChange",
        "playerId",
        playerId,
        "permissionLevel",
        permissionLevel);

    // 获取玩家实体 ID 用于 ir::play::EntityEvent
    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* player = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        return;
    }

    // 通过 EntityEvent 通知客户端权限等级变更（status byte = 24 + level）。
    // 1.21.11 权限等级走 EntityEvent(OP_PERMISSION_LEVEL_0..3 = 24..27)。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(player->id());
    pkt.eventId = static_cast<u8>(24 + permissionLevel);
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });

    // 同步更新后的命令树到客户端，以便刷新可用命令列表
    sendCommandTreePacket(playerId);
}

void LoginFlow::sendInitialDifficultyToPlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendInitialDifficulty", "phase", "difficulty_sync");

    m_server.sendPacketToPlayer(playerId, m_server.serializeDifficultyPacket());
}

} // namespace mc::server::net
