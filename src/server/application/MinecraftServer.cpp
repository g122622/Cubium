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

#include "MinecraftServer.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/dimension/Dimension.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/storage/GlobalStorageManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/storage/db/ConsistencyMode.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "common/world/storage/save/AutoSave.hpp"
#include "common/world/village/raid/Raid.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/bossbar/ServerDragonBossBar.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/WhitelistManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/function/FunctionLoader.hpp"
#include "server/function/TimerQueue.hpp"
#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/interaction/MiningManager.hpp"
#include "server/mod/bedrock/addon/ServerScriptManager.hpp"
#include "server/network/LoginFlow.hpp"
#include "server/network/PlayerBroadcaster.hpp"
#include "server/network/RemoteSessionManager.hpp"
#include "server/network/ServerNetwork.hpp"
#include "server/network/ServerPlayHandler.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/registry/RegistryBootstrap.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/sync/BlockUpdateSyncManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include "server/sync/WeatherSyncService.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

namespace {

[[nodiscard]] bool isReadonlyForeignStorage(const world::storage::SingleLevelStorageManager* storage)
{
    return storage != nullptr && storage->isOpen() && storage->config().readonly && storage->isForeignFormat();
}

// tick 耗时指数移动平均平滑因子（与 MC 一致），_updateTickDebugStats 使用。
constexpr f32 kTickTimeSmoothFactor = 0.2f;

} // namespace

MinecraftServer::MinecraftServer(ServerSettings& settings)
    : m_settings(settings)
    , m_computationWorkerPool(-1, "ServerCompute", 100)
    , m_ioWorkerPool(-1, "ServerIO", 200)
    , m_lootTableManager()
{}

void MinecraftServer::setDifficulty(Difficulty difficulty)
{
    // 难度被锁定时拒绝切换，对齐 Java setDifficulty(diff, false) 的 !isDifficultyLocked() 守卫。
    if (m_difficultyLocked) {
        return;
    }
    if (m_difficulty == difficulty) {
        return; // 难度未变化，无需同步
    }
    m_difficulty = difficulty;

    // 广播难度变更给所有玩家
    broadcastDifficultyChange();
}

void MinecraftServer::setDifficultyLocked(bool locked)
{
    if (m_difficultyLocked == locked) {
        return; // 锁定状态未变化，无需同步
    }
    m_difficultyLocked = locked;

    // 锁定状态变更立即广播 cb:10（携带新 locked 值），客户端据此禁用/启用难度按钮。
    broadcastDifficultyChange();
    spdlog::info("Difficulty locked = {}", locked);
}

void MinecraftServer::broadcastDifficultyChange()
{
    broadcastPacket(serializeDifficultyPacket());
    spdlog::info("Difficulty changed to {} (locked={})", static_cast<i32>(m_difficulty), m_difficultyLocked);
}

mc::network::ir::IrPacket MinecraftServer::serializeDifficultyPacket()
{
    mc::network::ir::play::ChangeDifficulty pkt;
    pkt.difficulty = static_cast<i32>(m_difficulty);
    pkt.locked = m_difficultyLocked;

    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

void MinecraftServer::setDefaultGameMode(GameMode mode)
{
    m_settings.defaultGameMode.set(static_cast<i32>(mode));
}

void MinecraftServer::setPlayerIdleTimeoutMinutes(i32 timeoutMinutes)
{
    m_playerIdleTimeoutMinutes = timeoutMinutes;
}

void MinecraftServer::requestStop()
{
    m_running = false;
}

PlayerInventory* MinecraftServer::playerInventory(PlayerId playerId)
{
    MC_ASSERT_RELEASE(m_inventoryManager != nullptr);
    return m_inventoryManager->getInventory(playerId);
}

const PlayerInventory* MinecraftServer::playerInventory(PlayerId playerId) const
{
    MC_ASSERT_RELEASE(m_inventoryManager != nullptr);
    return m_inventoryManager->getInventory(playerId);
}

MinecraftServer::~MinecraftServer()
{
    if (m_running) {
        shutdown();
    }
}

bool MinecraftServer::isSharedStorageReadonlyForeignWorld() const
{
    return isReadonlyForeignStorage(m_storage.get());
}

void MinecraftServer::shutdown()
{
    m_running = false;
    shutdownManagers();
}

void MinecraftServer::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "MinecraftServerTick");

    if (!m_running.load()) {
        return;
    }

    // 更新时间 - 根据 doDaylightCycle 游戏规则决定是否推进日光周期
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "TickTime");

        // 从主世界获取 doDaylightCycle 游戏规则
        // MC 1.16.5: 只有主世界的时间会受 doDaylightCycle 影响
        bool daylightCycleEnabled = true; // 默认启用
        auto* overworld = m_dimensionManager ? m_dimensionManager->getOverworld() : nullptr;
        if (overworld && overworld->world()) {
            daylightCycleEnabled =
                overworld->world()->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_DAYLIGHT_CYCLE);
        }

        // 更新 TimeManager 的日光周期状态
        m_timeManager->setDaylightCycleEnabled(daylightCycleEnabled);
        m_timeManager->tick();
    }

    // 自然刷怪和生物消失由 ServerDimension::tick() 处理

    // 清理断开连接的玩家
    if (m_tickCounter % CLEANUP_INTERVAL == 0) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "CleanupDisconnected", "phase", "cleanup");

        std::vector<PlayerId> removedPlayers;
        m_connectionManager->cleanupDisconnectedPlayers(&removedPlayers);
        // 清理玩家相关的追踪和票据
        for (PlayerId playerId : removedPlayers) {
            // 从维度同步管理器中移除玩家
            auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(playerId);
            if (playerDim) {
                if (playerDim->world() && playerDim->world()->chunkManager()) {
                    playerDim->world()->chunkManager()->removePlayer(playerId);
                }
                if (auto* cs = playerDim->chunkSendManager()) {
                    cs->removePlayer(playerId);
                }
            }
        }
    }

    ++m_tickCounter;

    // 更新所有维度
    if (m_dimensionManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "TickAllDimensions");
        m_dimensionManager->tick();
    }

    // 全员睡眠跨维度聚合判定（维度 tick 完成后、实体 tick 之前）。
    // 睡眠状态变化由本帧轮询捕获，最多 1 tick 延迟。
    checkAllPlayersSleeping();

    // 函数系统 tick：执行 minecraft:tick 标签中的函数和处理调度的函数
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "TickFunctions");
        // 创建游戏循环命令源（权限等级2，抑制输出，无关联玩家）
        command::ServerCommandSource gameLoopSource(this, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "");

        // 执行 tick 和 load 标签中的函数
        m_functionManager.tick(gameLoopSource);

        // 处理到期的调度事件
        auto dueEvents = m_functionTimerQueue.tick(currentTick());
        for (const auto& event : dueEvents) {
            if (event.type == function::TimerQueue::EventType::Function) {
                // 执行单个函数
                m_functionManager.execute(event.loc, gameLoopSource);
            } else if (event.type == function::TimerQueue::EventType::FunctionTag) {
                // 执行函数标签中的所有函数
                const auto& functionIds = m_functionManager.getTag(event.loc);
                for (const auto& funcId : functionIds) {
                    m_functionManager.execute(funcId, gameLoopSource);
                }
            }
        }
    }

    if (m_storage) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "TickSharedStorageAutoSave");
        m_storage->tickAutoSave(currentTick());
    }

    // 执行实体 tick
    tickEntities();

    // 实体同步由 ServerDimension::tick() 处理

    // 更新挖掘进度（遍历所有维度）
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world) {
            miningManager().tick(*world);
        }
    });

    // 处理网络事件（子类实现）
    pollNetwork();

    if (!m_running.load()) {
        return;
    }

    // 检查心跳超时（每 tick 判定：开销极小，每玩家一次减法比较；vanilla 硬编码 30s 阈值）。
    // 必须 wall-clock 每tick，避免低 TPS 下漏判导致超时玩家滞留。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "CheckKeepAliveTimeout", "phase", "keepalive_timeout");

        u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
        auto timedOutPlayers = m_keepAliveManager->getTimedOutPlayers(currentTimeMs);
        for (PlayerId playerId : timedOutPlayers) {
            spdlog::warn("MinecraftServer: Player {} timed out", playerId);
            // 从维度同步管理器中移除玩家
            auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(playerId);
            if (playerDim) {
                if (playerDim->world() && playerDim->world()->chunkManager()) {
                    playerDim->world()->chunkManager()->removePlayer(playerId);
                }
                if (auto* cs = playerDim->chunkSendManager()) {
                    cs->removePlayer(playerId);
                }
            }
            m_connectionManager->disconnectPlayer(playerId, "Connection timed out");
        }
    }

    // 区块发送由 ServerDimension::tick() 处理

    // 方块更新同步由 ServerDimension::tick() 处理

    // 心跳发送（wall-clock 按需，间隔为 vanilla 硬编码 15s）
    tickKeepAlive();

    // 每 20 tick 同步一次时间
    u64 tick = currentTick();
    if (tick % 20 == 0) {
        sendTimeUpdate();
    }

    // 同步天气变化
    if (m_weatherSyncService) {
        m_weatherSyncService->tick();
    }

    // 驱动脚本系统tick
    if (m_scriptManager && m_scriptManager->isInitialized()) {
        m_scriptManager->tick(currentTick());
    }
}

void MinecraftServer::initializeCoreManagers()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeCoreManagers");

    // 创建核心管理器
    m_playerManager = std::make_unique<core::PlayerManager>(m_settings.maxPlayers.get());
    m_connectionManager = std::make_unique<core::ConnectionManager>(*m_playerManager);
    // MC 1.16.5: 新世界初始 dayTime = 0（日出时刻）
    m_timeManager = std::make_unique<core::TimeManager>(0, 0);
    m_teleportManager = std::make_unique<core::TeleportManager>(*m_playerManager);
    m_keepAliveManager = std::make_unique<core::KeepAliveManager>(*m_playerManager);
    m_positionTracker = std::make_unique<core::PositionTracker>(*m_playerManager, m_settings.viewDistance.get());
    m_gameModeManager = std::make_unique<core::GameModeManager>(*m_playerManager, *m_connectionManager);

    // 注册游戏模式变化回调：当玩家切入/切出旁观者模式时，更新 ServerPlayer 实体的
    // 游戏模式（能力/noclip）。切出旁观者额外 resetCamera() 发送 SetCameraPacket。
    // 原实现两分支各写一遍“遍历维度寻找 ServerPlayer 实体”循环，此处合并为单循环，
    // 仅在切出旁观者时多调一次 resetCamera()。
    m_gameModeManager->setOnGameModeChange([this](PlayerId playerId, GameMode oldMode, GameMode newMode) {
        const bool leavingSpectator = (oldMode == GameMode::Spectator && newMode != GameMode::Spectator);
        const bool enteringSpectator = (newMode == GameMode::Spectator);
        if (!leavingSpectator && !enteringSpectator) {
            return;
        }

        // 遍历所有维度世界寻找玩家实体（ServerPlayer）
        for (DimensionId dimId : m_dimensionManager->getDimensionIds()) {
            auto* dimension = m_dimensionManager->getDimension(dimId);
            if (dimension == nullptr || dimension->world() == nullptr) {
                continue;
            }
            Entity* entity = dimension->world()->getEntity(static_cast<EntityInstanceId>(playerId));
            if (entity == nullptr) {
                continue;
            }
            auto* serverPlayer = dynamic_cast<mc::ServerPlayer*>(entity);
            if (serverPlayer != nullptr) {
                if (leavingSpectator) {
                    serverPlayer->resetCamera();
                }
                serverPlayer->setGameMode(newMode);
                break;
            }
        }
    });
    m_whitelistManager = std::make_unique<core::WhitelistManager>();
    m_bannedPlayerList = std::make_unique<core::BannedPlayerList>();
    m_bannedIpList = std::make_unique<core::BannedIpList>();
    m_opListManager = std::make_unique<core::OpListManager>();

    // 创建记分板
    m_scoreboard = std::make_unique<ServerScoreboard>(*this);

    // 创建 Boss 栏管理器
    m_bossBarManager = std::make_unique<CustomServerBossInfoManager>(*this);

    // 创建脚本系统管理器
    m_scriptManager = std::make_unique<ServerScriptManager>();

    // 创建维度管理器
    m_dimensionManager = std::make_unique<ServerDimensionManager>(this);

    // 创建天气同步服务（方法内部对主世界 WeatherManager 未就绪做早退守卫，
    // 故可在维度世界创建前构造；tick/登录调用时主世界已就绪）
    m_weatherSyncService = std::make_unique<sync::WeatherSyncService>(*this);

    // 创建玩家广播门面（承接全部 broadcast*/send* 转发型网络广播，仅持 *this 引用）
    m_broadcaster = std::make_unique<net::PlayerBroadcaster>(*this);

    // 创建登录流程门面（玩家创建 + 初始游戏状态推送整簇，仅持 *this 引用；运行期各 manager
    // 已就绪时 createPlayerForConnection 才被调用，构造时机无依赖）
    m_loginFlow = std::make_unique<net::LoginFlow>(*this);

    // 创建 Play 包处理门面（routeInboundPlayPacket + 13 个非纯虚 handle*Packet +
    // updateEntityTrackingForPlayer 整簇，仅持 *this 引用）。须先于下方维度切换回调构造：
    // 回调内调 m_playHandler->updateEntityTrackingForPlayer。
    m_playHandler = std::make_unique<net::ServerPlayHandler>(*this);
    m_dimensionManager->setDimensionChangeCallback(
        [this](PlayerId playerId, DimensionId fromDim, DimensionId toDim, const Vector3d& position) {
            auto* player = m_playerManager->getPlayer(playerId);
            if (!player) {
                return;
            }

            m_positionTracker->updatePosition(
                playerId, position.x, position.y, position.z, player->yaw, player->pitch, player->onGround);
            m_playHandler->updateEntityTrackingForPlayer(playerId, position.x, position.y, position.z);

            // 发送维度特定时间更新
            // 当玩家切换到下界或末地时，需要发送固定时间
            auto* targetDim = m_dimensionManager->getDimension(toDim);
            if (targetDim && targetDim->world()) {
                const auto& time = timeManager().gameTimeObj();
                i64 dayTime = targetDim->world()->dayTime();

                mc::network::ir::play::SetTime timePkt;
                timePkt.gameTime = time.gameTime();
                timePkt.dayTime = dayTime;
                timePkt.tickDayTime = time.daylightCycleEnabled();
                sendPacketToPlayer(playerId,
                    mc::network::ir::IrPacket{
                        mc::network::protocol::ConnectionProtocol::Play,
                        mc::network::ir::PlayPacket{std::move(timePkt)},
                    });
            }
        });

    // Worker 池由服务器统一管理，初始化阶段在这里启动。
    m_computationWorkerPool.start();
    m_ioWorkerPool.start();

    // 注册 viewDistance / simulationDistance 运行时变更回调（须在 m_dimensionManager /
    // m_playerManager 就绪后）。初始值已由 LoginFlow 登录时下发，此处仅处理运行时变更。
    _registerDistanceCallbacks();
}

void MinecraftServer::attachWorldBindings(ServerWorld& world)
{
    world.setOnPlaySound([this](const ResourceLocation& soundEventId,
                             sound::SoundCategory category,
                             const Vector3& position,
                             f32 volume,
                             f32 pitch) { broadcastSound(soundEventId, category, position, volume, pitch); });
    world.setOnBroadcastParticle([this](particle::ParticleTypeId type,
                                     const Vector3& pos,
                                     const Vector3& velocity,
                                     const Vector3& offset,
                                     u32 count) { broadcastParticleInRange(type, pos, velocity, offset, count); });
    world.setOnBroadcastVibrationParticle(
        [this](const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks) {
            broadcastVibrationParticleInRange(pos, targetSource, arrivalInTicks);
        });
    world.setOnBroadcastTrailParticle(
        [this](const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks) {
            broadcastTrailParticleInRange(pos, targetPosition, color, durationInTicks);
        });
    world.setOnBroadcastEntityEffectParticle(
        [this](const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color) {
            broadcastEntityEffectParticleInRange(pos, velocity, offset, count, color);
        });
    world.setOnBroadcastBlockParticle(
        [this](particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, u32 blockStateId) {
            broadcastBlockParticleInRange(type, pos, velocity, blockStateId);
        });
    world.setOnBroadcastItemParticle(
        [this](particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack) {
            broadcastItemParticleInRange(type, pos, velocity, itemStack);
        });
    world.setOnBroadcastEntityStatus([this, &world](EntityInstanceId entityId, u8 status) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastEntityStatusInRange(entityId, status, entity->position());
        }
    });
    world.setOnBroadcastEntityAnimation([this, &world](EntityInstanceId entityId, u8 animation) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastEntityAnimationInRange(entityId, animation, entity->position());
        }
    });
    world.setOnBroadcastHurtAnimation([this, &world](EntityInstanceId entityId, f32 hurtDir) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastHurtAnimationInRange(entityId, hurtDir, entity->position());
        }
    });
    world.setOnBroadcastSetEntityLink([this, &world](EntityInstanceId entityId, EntityInstanceId linkedEntityId) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastSetEntityLinkInRange(entityId, linkedEntityId, entity->position());
        }
    });
    world.setOnBroadcastWorldEvent(
        [this](i32 eventId, i32 x, i32 y, i32 z, i32 data) { broadcastWorldEventInRange(eventId, x, y, z, data); });
    world.setOnBroadcastBlockEvent([this](i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockId) {
        broadcastBlockEventInRange(x, y, z, paramA, paramB, blockId);
    });
    world.setOnBroadcastBlockEntity([this, &world](const BlockPos& pos) {
        // 方块实体数据变化后，获取最新 NBT 快照并广播给附近客户端
        // 参考 MC Java: ServerLevel.sendBlockUpdated -> PlayerList.broadcast(
        //   null, x, y, z, 64.0, dimension, new ClientboundBlockEntityDataPacket)
        const BlockEntity* entity = world.getBlockEntity(pos);
        if (entity == nullptr) {
            return;
        }

        // 1.21.11 BlockEntityData：blockPosPacked + blockEntityType + CompoundTag（无长度前缀）。
        auto tag = std::make_shared<nbt::CompoundTag>(entity->getUpdateTag());
        broadcastBlockEntityInRange(pos, entity->getType(), std::move(tag));
    });
    world.setOnDestroyBlockProgress([this](EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress) {
        broadcastBlockBreakProgressInRange(breakerId, x, y, z, progress);
    });
    world.setOnBroadcastExplosion([this](const Vector3& position,
                                      f32 strength,
                                      const std::vector<BlockPos>& affectedBlocks,
                                      const std::unordered_map<u64, Vector3>& playerKnockback) {
        broadcastExplosionInRange(position, strength, affectedBlocks, playerKnockback);
    });
}

void MinecraftServer::attachWorldCommandBindings(ServerWorld& world)
{
    // 注：setTimeManager / setDifficultyCallback / setLootTableManager 三项注入已由
    // ServerDimensionManager::_createServerWorld 在每个维度世界创建时统一完成（见
    // ServerDimensionManager.cpp:565-567）。批5a 去重：此处不再重复注入，避免装配
    // 胶水与维度管理器双写。保留空函数体供子类 attachWorldBindings/attachWorldCommandBindings
    // 成对装配调用点结构稳定，便于将来若需补充维度无关命令绑定在此扩展。
    (void)world;
}

Result<void> MinecraftServer::initializeSharedStorage(const GameDirectory& gameDirectory, const std::string& levelId)
{
    world::storage::SingleLevelStorageConfig storageConfig;
    storageConfig.consistencyMode = world::storage::ConsistencyMode::Eventual;
    storageConfig.sectionCacheCapacity = 2048;
    storageConfig.enableBackup = true;

    m_globalStorage = world::storage::GlobalStorageManager(gameDirectory);
    auto storageResult = m_globalStorage.openLevel(levelId, storageConfig);
    if (storageResult.failed()) {
        spdlog::error("Failed to open world storage: {}", storageResult.error().message());
        return storageResult.error();
    }
    m_storage = storageResult.value();
    m_storage->setIoWorkerPool(&m_ioWorkerPool);
    m_storage->setComputeWorkerPool(&m_computationWorkerPool);
    spdlog::info("World storage opened at {}", m_storage->worldPath().string());

    world::storage::AutoSaveConfig saveConfig;
    m_storage->initializeAutoSave(saveConfig);
    if (isSharedStorageReadonlyForeignWorld()) {
        spdlog::info("World storage is a readonly foreign world (format: {}); autosave remains disabled",
            m_storage->formatInfo().formatName);
    } else {
        m_storage->startAutoSave();
    }
    m_scoreboard->setDataManager(m_storage->scoreboardDataManager());
    m_scoreboard->load();
    return Result<void>::ok();
}

void MinecraftServer::shutdownSharedStorage()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::shutdownSharedStorage");

    if (m_storage && m_storage->isOpen()) {
        m_storage->close();
    }
    m_storage.reset();
}

Result<size_t> MinecraftServer::saveAllWorldData()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::saveAllWorldData");

    if (!m_storage || !m_storage->isOpen()) {
        return Error(ErrorCode::InvalidState, "Shared storage not open");
    }

    if (isSharedStorageReadonlyForeignWorld()) {
        spdlog::info("Skipping saveAllWorldData persistence for readonly foreign world (format: {})",
            m_storage->formatInfo().formatName);
        return 0;
    }

    // 保存区块和玩家数据
    auto result = m_storage->saveAll();
    if (result.failed()) {
        return result.error();
    }

    // 保存运行时数据到 level.dat（时间、天气、出生点等）
    // 仅保存主世界数据
    auto* overworld = m_dimensionManager->getOverworld();
    if (overworld && overworld->world()) {
        ServerWorld* world = overworld->world();

        // 获取时间和天气数据
        i64 gameTime = m_timeManager->gameTime();
        i64 dayTime = m_timeManager->dayTime();

        // 获取出生点
        // worldSpawnPoint 是玩家脚位置（方块上方），level.dat 的 SpawnY 语义为脚下方块 Y，需 -1。
        Vector3d spawnPoint = world->worldSpawnPoint();
        i32 spawnX = static_cast<i32>(std::floor(spawnPoint.x));
        i32 spawnY = static_cast<i32>(std::floor(spawnPoint.y)) - 1;
        i32 spawnZ = static_cast<i32>(std::floor(spawnPoint.z));
        f32 spawnAngle = world->spawnAngle();

        // 获取天气数据
        i32 clearWeatherTime = 0;
        i32 rainTime = 0;
        bool raining = false;
        i32 thunderTime = 0;
        bool thundering = false;

        if (world->weatherManager()) {
            const auto& weatherState = world->weatherManager()->state();
            clearWeatherTime = weatherState.clearWeatherTime;
            rainTime = weatherState.rainTime;
            raining = weatherState.raining;
            thunderTime = weatherState.thunderTime;
            thundering = weatherState.thundering;
        }

        // 保存到 level.dat
        auto levelResult = m_storage->saveLevelData(gameTime,
            dayTime,
            spawnX,
            spawnY,
            spawnZ,
            spawnAngle,
            clearWeatherTime,
            rainTime,
            raining,
            thunderTime,
            thundering,
            m_spawnInitializedThisSession,
            m_difficulty,
            m_difficultyLocked);

        if (levelResult.failed()) {
            spdlog::error("Failed to save level.dat: {}", levelResult.error().message());
        }

        // 保存调度事件到 level.dat
        auto serializedEvents = m_functionTimerQueue.serialize();
        auto eventsResult = m_storage->saveScheduledEvents(*serializedEvents);
        if (eventsResult.failed()) {
            spdlog::error("Failed to save scheduled events: {}", eventsResult.error().message());
        }
    }

    // 保存末影龙战斗数据（仅末地维度）。
    // 末影龙战斗状态是末地维度专属的世界级数据，落盘归属服务器级全量保存入口，
    // 而非任何单一 ServerWorld（原 ServerWorld::saveAll 越权方法已删除，该路径此前是
    // saveDragonFightData 的唯一调用方，生产侧关服从未落盘末影龙数据——本次一并补全）。
    if (auto* theEnd = m_dimensionManager->getTheEnd(); theEnd && theEnd->world()) {
        if (auto* dragonFight = theEnd->world()->dragonFight()) {
            auto dragonFightResult = m_storage->saveDragonFightData(dragonFight->saveData().toJson());
            if (dragonFightResult.failed()) {
                spdlog::warn("Failed to save dragon fight data: {}", dragonFightResult.error().message());
            }
        }
    }

    spdlog::info("Saved {} cached sections and player data during shutdown", result.value());
    return result.value();
}

Result<void> MinecraftServer::initializeWorld()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeWorld");

    // 检查维度管理器是否已初始化且主世界维度存在
    auto* overworld = m_dimensionManager->getOverworld();
    if (!overworld || !overworld->world()) {
        return Error(ErrorCode::NotInitialized, "Overworld dimension not initialized");
    }

    // 物理系统当前通过各维度 world 自身暴露的碰撞查询能力工作，这里不再保留未接入占位。

    // 初始化命令注册表
    m_commandRegistry = std::make_unique<command::CommandRegistry>();

    // 初始化命令存储（/data storage 命令使用的持久化 NBT 存储）
    m_commandStorage = std::make_unique<command::CommandStorage>();

    if (m_storage && m_storage->isOpen()) {
        auto runtimeDataResult = m_storage->loadLevelData();
        if (runtimeDataResult.success()) {
            const auto& runtimeData = runtimeDataResult.value();
            m_timeManager->setGameTime(runtimeData.gameTime);
            m_timeManager->setDayTime(runtimeData.dayTime);

            // 恢复 level.dat 中的难度与锁定状态（此前读出即丢弃，导致老存档重启难度不恢复）。
            // 直接置字段而非调 setDifficulty/setDifficultyLocked：避免触发广播（此时玩家尚未加入）。
            m_difficulty = runtimeData.summary.difficulty;
            m_difficultyLocked = runtimeData.difficultyLocked;

            m_dimensionManager->forEachDimension([&](Dimension& dim) {
                auto* serverDim = static_cast<ServerDimension*>(&dim);
                auto* world = serverDim->world();
                if (world == nullptr) {
                    return;
                }

                // 仅主世界持有世界出生点（消费方只读主世界 worldSpawnPoint）。
                // 下界/末地无独立 spawn，不再调 initializeWorldSpawn。
                if (serverDim->id() != 0) {
                    return;
                }

                // 先恢复 level.dat 中的运行时数据（时间/天气/spawnAngle/出生点占位等）
                world->applyLevelRuntimeData(runtimeData);

                // level.dat 未初始化时，计算真实出生点覆盖新世界模板的 (0,0,0) 占位。
                // 老存档（initialized=true）直接信任存档中的 SpawnX/Y/Z。
                if (!runtimeData.initialized) {
                    world->initializeWorldSpawn();
                }
                m_spawnInitializedThisSession = true;
            });
        } else {
            spdlog::warn("Failed to load level runtime data: {}", runtimeDataResult.error().message());
            // level.dat 读取失败：视为未初始化，仅主世界计算出生点。
            m_dimensionManager->forEachDimension([&](Dimension& dim) {
                auto* serverDim = static_cast<ServerDimension*>(&dim);
                auto* world = serverDim->world();
                if (world != nullptr && serverDim->id() == 0) {
                    world->initializeWorldSpawn();
                }
            });
            m_spawnInitializedThisSession = true;
        }

        // 加载调度事件
        auto eventsResult = m_storage->loadScheduledEvents();
        if (eventsResult.success() && !eventsResult.value()->value.empty()) {
            m_functionTimerQueue.deserialize(*eventsResult.value());
            spdlog::info("Loaded {} scheduled event(s) from level.dat", m_functionTimerQueue.size());
        } else if (eventsResult.failed()) {
            spdlog::warn("Failed to load scheduled events: {}", eventsResult.error().message());
        }
    }

    // 初始化脚本系统
    if (m_scriptManager) {
        // 桥接脚本API到游戏对象
        m_scriptManager->setServer(this);

        auto scriptResult = m_scriptManager->initialize();
        if (scriptResult.failed()) {
            spdlog::warn("[Server] Failed to initialize script system: {}", scriptResult.error().message());
        }
    }

    return Result<void>::ok();
}

void MinecraftServer::initializeInteractionManagers()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeInteractionManagers");

    m_blockInteractionManager =
        std::make_unique<interaction::BlockInteractionManager>(*m_playerManager, m_lootTableManager);

    m_miningManager = std::make_unique<interaction::MiningManager>(*m_playerManager, *m_connectionManager);

    m_containerManager = std::make_unique<interaction::ContainerManager>(*m_playerManager);

    m_inventoryManager = std::make_unique<interaction::InventoryManager>(*m_playerManager);

    m_inventoryManager->setOnInventoryUpdate([this](PlayerId playerId, const PlayerInventory& inventory) {
        // 1.21.11 用 ContainerSetContent(containerId=0) 同步完整玩家物品栏。
        // stateId 取自玩家数据（containerId=0 在服务端无独立 AbstractContainerMenu 实例，
        // 由 ServerPlayerData::playerInventoryStateId 承载，每次下发自增）。
        mc::network::ir::play::ContainerSetContent pkt;
        pkt.containerId = 0; // 玩家物品栏
        auto* playerData = m_playerManager->getPlayer(playerId);
        pkt.stateId = (playerData != nullptr) ? playerData->incrementPlayerInventoryStateId() : 0;
        const i32 totalSlots = inventory.getContainerSize();
        pkt.items.reserve(static_cast<size_t>(totalSlots));
        for (i32 slot = 0; slot < totalSlots; ++slot) {
            pkt.items.push_back(mc::network::ir::toItemStackView(inventory.getItem(slot)));
        }
        pkt.carriedItem = mc::network::ir::play::ItemStackView{0, 0, {}}; // 空 carried

        sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(pkt)},
            });
    });

    // 设置 InventoryManager 到其他管理器
    m_containerManager->setInventoryManager(m_inventoryManager.get());
    m_blockInteractionManager->setInventoryManager(m_inventoryManager.get());
    m_miningManager->setInventoryManager(m_inventoryManager.get());
    m_miningManager->setOnMiningComplete([this](PlayerId playerId, const BlockPos& pos) {
        MC_ASSERT_RELEASE(m_blockInteractionManager != nullptr);
        auto result = m_blockInteractionManager->handleBlockBreak(playerId, pos);
        // 即使失败也不影响流程，handleBlockBreak 内部已处理
        (void)result;
    });

    // 设置 EntityInstanceId 解析器：将 PlayerId 转换为正确的 EntityInstanceId
    // MiningManager 内部只有 PlayerId，但广播破坏动画需要 EntityInstanceId 作为 breakerId
    m_miningManager->setEntityIdResolver(
        [this](PlayerId playerId) -> EntityInstanceId { return playerEntityManager().getPlayerEntityId(playerId); });

    // 设置破坏动画广播回调：将挖掘进度通过 ServerWorld::destroyBlockProgress 广播给其他玩家
    // 对应 MC Java: ServerPlayerGameMode 中调用 level.destroyBlockProgress(entityId, pos, stage)
    m_miningManager->setOnBreakAnimBroadcast([this](PlayerId playerId, i32 x, i32 y, i32 z, i8 stage) {
        EntityInstanceId entityId = playerEntityManager().getPlayerEntityId(playerId);
        if (entityId == INVALID_ENTITY_ID) {
            return;
        }
        // 获取玩家所在维度的世界，向该维度广播破坏动画
        // 对应 MC Java: serverplayer.level() == this 维度检查
        ServerDimension* dim = dimensionManager().getPlayerDimensionWorld(playerId);
        if (dim != nullptr) {
            auto* world = dim->world();
            if (world != nullptr) {
                world->destroyBlockProgress(entityId, BlockPos(x, y, z), static_cast<i32>(stage));
            }
        }
    });

    // 设置服务器接口到 BlockInteractionManager（用于告示牌命令执行等）
    m_blockInteractionManager->setServer(this);

    // 初始化成就事件处理器
    // 先注册所有内置触发器（必须在成就加载之前完成）
    mc::advancement::CriterionTriggers::instance().registerBuiltinTriggers();

    // 设置服务器接口以允许从 PlayerId 获取 ServerPlayer
    m_advancementEventHandler.setServer(this);
    m_advancementEventHandler.initialize();
    spdlog::info("AdvancementEventHandler initialized");
}

void MinecraftServer::initializeSyncManagers()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeSyncManagers");

    // 同步管理器已移入 ServerDimension，由 ServerDimension::initialize() 创建
}

void MinecraftServer::initializeChunkSyncManagers()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeChunkSyncManagers");

    // 区块同步管理器已移入 ServerDimension，由 ServerDimension::initialize() 创建
}

void MinecraftServer::initializeRegistries(bool registerEntities)
{
    // 注册表装配已下沉到 RegistryBootstrap 门面（server/registry/RegistryBootstrap.cpp）。
    // 此处仅构造门面并转调，保持原调用顺序与行为逐字节一致。
    RegistryBootstrap bootstrap(m_dataPackList, m_lootTableManager, m_predicateManager, m_functionManager);
    bootstrap.initializeAll(registerEntities);
}

void MinecraftServer::setupWorldCallbacks()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::setupWorldCallbacks");

    // 为所有维度设置回调
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (!world || !world->chunkManager()) {
            return;
        }

        // 注入服务器接口，供 ServerWorld 在主动移除实体（removeEntity / 区块卸载）时
        // 通过 EntityTracker 向追踪玩家发送 destroy 包。
        world->setServer(this);
        // 设置区块加载回调 - 当区块加载/生成完成时触发
        // 注：onChunkLoaded（加载区块内实体）由调用方负责，不在此回调内调用：
        //   - 存档加载路径：ServerChunkManager::_resolveChunkSourceSync 在 m_chunkLoadedCallback 之前
        //     已直接调用 m_world->onChunkLoaded。
        //   - 生成路径：ServerChunkManager::_drainPendingPostProcess 在 m_chunkLoadedCallback 之前
        //     已直接调用 m_world->onChunkLoaded。
        //   此回调仅入队区块加载光照任务（③-2b 统一异步调度），重复调用 onChunkLoaded 会导致实体重复生成。
        //   光照由 worker 完成（ChunkLoadLightTask），完成后经发送续延队列在主线程 send——
        //   保证 serialize 读到已光照 nibble，客户端不收全黑区块。
        world->chunkManager()->setChunkLoadedCallback(
            [world](ChunkCoord x, ChunkCoord z) { world->enqueueChunkLoadLight(x, z); });

        // 设置区块卸载回调 - 当区块即将卸载时触发
        world->chunkManager()->setChunkUnloadedCallback([this, serverDim, world](ChunkCoord x, ChunkCoord z) {
            // 保存并移除区块内实体
            world->onChunkUnloading(x, z);
            if (auto* cs = serverDim->chunkSendManager()) {
                cs->onChunkPreUnload(x, z);
            }
        });

        // 设置追踪变化回调 - 当玩家进入/离开区块视距时触发
        world->chunkManager()->ticketManager().setTrackingChangeCallback(
            [serverDim](PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
                if (auto* cs = serverDim->chunkSendManager()) {
                    cs->onPlayerTrackingChange(player, x, z, isTracking);
                }
            });

        // 设置区块缓存中心变化回调 - 玩家所在区块中心变化时发送 SetChunkCacheCenter。
        // 对齐 vanilla ChunkMap.applyChunkTrackingView：中心变化先于区块数据下发。
        // 客户端 ClientChunkCache.Storage 的 viewCenterX/Z 默认 (0,0)，未收到此包则
        // inRange（Chebyshev ≤ chunkRadius）恒以原点为中心，出生点远离原点的玩家收到的
        // 全部区块被 “Ignoring chunk since it's not in the view range” 丢弃，LevelLoadTracker
        // 第二闸门 isSectionCompiledAndVisible 永远过不去，卡 “加载地形中” 直至心跳超时。
        world->chunkManager()->setChunkCacheCenterCallback([this](PlayerId player, ChunkCoord x, ChunkCoord z) {
            mc::network::ir::play::SetChunkCacheCenter pkt;
            pkt.x = static_cast<i32>(x);
            pkt.z = static_cast<i32>(z);
            sendPacketToPlayer(player,
                mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play,
                    mc::network::ir::PlayPacket{std::move(pkt)},
                });
        });

        // 设置实体生成回调
        world->chunkManager()->setEntitySpawnCallback([this, world](const std::vector<SpawnedEntityData>& entities) {
            for (const auto& entityData : entities) {
                const auto* entityType = entity::EntityRegistry::instance().getType(entityData.entityTypeId);
                if (!entityType || !entityType->canSummon()) {
                    continue;
                }
                auto entity = entityType->create(world);
                if (!entity) {
                    continue;
                }
                entity->setPosition(Vector3(entityData.x, entityData.y, entityData.z));
                if (world->physicsEngine()) {
                    entity->setPhysicsEngine(world->physicsEngine());
                }

                // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
                auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
                if (mobEntity != nullptr) {
                    entity::combat::DifficultyInstance difficultyInstance =
                        entity::combat::DifficultyInstance::at(*world,
                            BlockPos(static_cast<i32>(std::floor(entityData.x)),
                                static_cast<i32>(entityData.y),
                                static_cast<i32>(std::floor(entityData.z))));
                    mobEntity->finalizeSpawn(*world, difficultyInstance, world::spawn::SpawnReason::ChunkGeneration);
                }

                const EntityInstanceId spawnedId = world->spawnEntity(std::move(entity));
                MC_UNUSED(spawnedId);
            }
        });

        // 设置光照变化回调：同步数据到 ChunkSection + 广播给客户端
        world->setOnLightChanged([this, serverDim](LightType type, const SectionPos& pos) {
            MC_TRACE_INSTANT_EVENT(TraceEvents.Server.Lighting,
                "ServerWorld::OnLightChangedCallback.START",
                "Type",
                (type == LightType::SKY) ? "Sky" : "Block",
                "Section",
                fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
                [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) { flow(ctx); });

            // markLightChanged 已在主线程 tick _drainPendingLightFlushes 中由 ServerWorld 完成
            // （_syncLightDataToChunk 写 ChunkSection nibble + setDirty），此处只负责广播给客户端。

            // 广播光照更新给客户端
            auto* lightManager = serverDim->lightManager();
            if (!lightManager) {
                return;
            }

            std::vector<u8> skyLight;
            std::vector<u8> blockLight;

            // 获取光照数据（主线程读 visible 侧）
            if (type == LightType::SKY && lightManager->hasSkyLight()) {
                auto* data = lightManager->getData(LightType::SKY, pos);
                if (data) {
                    skyLight = data->toByteArray();
                }
            } else if (type == LightType::BLOCK && lightManager->hasBlockLight()) {
                auto* data = lightManager->getData(LightType::BLOCK, pos);
                if (data) {
                    blockLight = data->toByteArray();
                }
            }

            // 发送光照更新包
            if (!skyLight.empty() || !blockLight.empty()) {
                broadcastLightUpdate(pos.x, pos.z, pos.y, skyLight, blockLight, false);
            }

            MC_TRACE_INSTANT_EVENT(TraceEvents.Server.Lighting,
                "ServerWorld::OnLightChangedCallback.END",
                "Type",
                (type == LightType::SKY) ? "Sky" : "Block",
                "Section",
                fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
                [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) { flow(ctx); });
        });

        world->setOnOpenContainer([this](ContainerType type, const BlockPos& pos, Player& player) {
            return openContainerRequest(type, pos, player);
        });

        // 设置方块变化回调：写入后记录到同步管理器，统一在 tick 末发送
        world->setOnBlockChanged([serverDim](const BlockPos& pos, u32 blockStateId) {
            if (auto* bus = serverDim->blockUpdateSyncManager()) {
                bus->queueBlockUpdate(pos, blockStateId);
            }
        });

        // 设置方块同步回调
        if (auto* bus = serverDim->blockUpdateSyncManager()) {
            bus->setOnBlockUpdate([this](PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId) {
                mc::network::ir::play::BlockUpdate pkt;
                pkt.blockPosPacked = BlockPos(x, y, z).asLong();
                pkt.blockStateId = static_cast<i32>(blockStateId);
                sendPacketToPlayer(playerId,
                    mc::network::ir::IrPacket{
                        mc::network::protocol::ConnectionProtocol::Play,
                        mc::network::ir::PlayPacket{std::move(pkt)},
                    });
            });
        }

        // 设置实体同步回调
        // 实体同步/广播全部由 EntityTracker + ServerWorld 广播回调完成，
        // 原 EntitySyncManager 幽灵同步层已删除（spawnEntity/removeEntity/
        // broadcastEntityStatus 零调用、回调空操作、tick 空转）。

        // 设置区块发送回调
        if (auto* cs = serverDim->chunkSendManager()) {
            cs->setOnChunkSend([this](PlayerId playerId,
                                   ChunkCoord x,
                                   ChunkCoord z,
                                   const mc::network::ir::play::LevelChunkWithLight& ir) {
                sendChunkDataToPlayer(playerId, x, z, ir);
            });

            cs->setOnChunkUnload([this](PlayerId playerId, ChunkCoord x, ChunkCoord z) {
                // 1.21.11 客户端按 SetChunkCacheCenter 中心 + render distance 自行回收远端区块，
                // 但服务端在玩家走远/区块卸载时仍显式发送 ForgetLevelChunk(cb:37)，确保客户端
                // 立即释放对应区块的 mesh 与缓存（早于客户端自主回收），与 vanilla
                // ServerPlayerGameModeifier / chunk 卸载链路一致。
                sendUnloadChunkToPlayer(playerId, x, z);
            });
        }
    });

    // 方块交互回调（全局，不按维度区分）
    m_blockInteractionManager->setOnBlockBreak([this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
        const auto& soundType = state.getSoundType();
        Vector3 position(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
        broadcastSoundInRange(soundType.getBreakSound(),
            sound::SoundCategory::Blocks,
            position,
            16.0f * soundType.getVolume(),
            soundType.getVolume(),
            soundType.getPitch());
    });

    m_blockInteractionManager->setOnBlockPlace([this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
        const auto& soundType = state.getSoundType();
        Vector3 position(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
        broadcastSoundInRange(soundType.getPlaceSound(),
            sound::SoundCategory::Blocks,
            position,
            16.0f * soundType.getVolume(),
            soundType.getVolume(),
            soundType.getPitch());
    });
}

void MinecraftServer::setupRaidManagerCallbacks()
{
    auto* overworld = m_dimensionManager->getOverworld();
    if (!overworld || !overworld->world() || !overworld->world()->raidManager()) {
        return;
    }

    auto* world = overworld->world();
    auto* raidManager = world->raidManager();
    world::village::raid::RaidCallbacks callbacks;

    callbacks.onRaidStarted = [this](const world::village::raid::Raid& raid, BlockPos center) {
        broadcastSound(SoundEvents::EVENT_RAID_HORN,
            sound::SoundCategory::Neutral,
            Vector3(static_cast<f32>(center.x) + 0.5f, static_cast<f32>(center.y), static_cast<f32>(center.z) + 0.5f),
            64.0f,
            1.0f);

        spdlog::info("Raid {} started at village center ({}, {}, {})", raid.id(), center.x, center.y, center.z);
    };

    callbacks.onRaidVictory =
        [this, world](const world::village::raid::Raid& raid, const std::vector<Uuid>& heroes, i32 badOmenLevel) {
            constexpr i32 HERO_EFFECT_DURATION = 48000;

            for (const auto& heroUuid : heroes) {
                const std::string heroUuidStr = util::uuidToString(heroUuid);

                m_playerManager->forEachPlayer(
                    [this, &heroUuidStr, badOmenLevel, &raid, world](ServerPlayerData& playerData) {
                        Player* player = playerEntityManager().getPlayerEntity(playerData.playerId, *world);
                        if (player == nullptr || player->uuid() != heroUuidStr) {
                            return;
                        }

                        entity::effect::EffectInstance heroEffect(entity::effect::EffectType::HeroOfTheVillage,
                            HERO_EFFECT_DURATION,
                            badOmenLevel - 1,
                            false,
                            true,
                            true);
                        player->addEffect(std::move(heroEffect));

                        spdlog::info(
                            "Player '{}' (UUID: {}) received Hero of the Village effect (level {}) for raid {} victory",
                            playerData.username,
                            heroUuidStr,
                            badOmenLevel,
                            raid.id());
                    });
            }

            BlockPos center = raid.center();
            spdlog::info("Raid {} victory at ({}, {}, {}) - {} heroes rewarded",
                raid.id(),
                center.x,
                center.y,
                center.z,
                heroes.size());
        };

    callbacks.onRaidLoss = [](const world::village::raid::Raid& raid) {
        BlockPos center = raid.center();
        spdlog::info("Raid {} failed at ({}, {}, {})", raid.id(), center.x, center.y, center.z);
    };

    callbacks.onWaveStarted = [](const world::village::raid::Raid& raid, i32 wave, BlockPos spawnPos) {
        spdlog::info("Raid {} wave {} started at ({}, {}, {})", raid.id(), wave, spawnPos.x, spawnPos.y, spawnPos.z);
    };

    raidManager->setCallbacks(std::move(callbacks));
}

void MinecraftServer::setupDragonFightBossBar()
{
    // 获取末地维度
    auto* theEnd = m_dimensionManager ? m_dimensionManager->getTheEnd() : nullptr;
    if (theEnd == nullptr || theEnd->world() == nullptr) {
        return;
    }

    auto* world = theEnd->world();
    EndDragonFight* fight = world->dragonFight();
    if (fight == nullptr) {
        return;
    }

    // 创建服务端末影龙 Boss 栏并注入 EndDragonFight
    // 对应 MC Java: EndDragonFight.dragonEvent = new ServerBossEvent(
    //     Component.translatable("entity.minecraft.ender_dragon"),
    //     BossBarColor.PINK, BossBarOverlay.PROGRESS)
    //     .setPlayBossMusic(true).setCreateWorldFog(true);

    // 使用世界种子 + 当前时间生成 UUID 随机数种子，保证每次启动生成不同的 Boss 栏 UUID
    math::Random uuidRng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));

    auto bossBar = std::make_unique<ServerDragonBossBar>(*this,
        util::generateRandomUuid(uuidRng),
        EndDragonFight::createDefaultBossName(),
        BossInfoColor::Pink,
        BossInfoOverlay::Progress);

    fight->setDragonBossBar(std::move(bossBar));
    spdlog::info("Dragon fight boss bar injected into EndDragonFight");
}

bool MinecraftServer::openContainerRequest(ContainerType type, const BlockPos& pos, Player& player)
{
    return containerManager().openContainer(player.playerId(), type, pos).success();
}

void MinecraftServer::shutdownManagers()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::shutdownManagers");

    // 1. 保存世界数据（落盘，整个 stop 流程最重的 I/O 阶段）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::shutdownManagers::SaveWorldData");

        if (m_storage && m_storage->isOpen()) {
            if (isSharedStorageReadonlyForeignWorld()) {
                m_storage->stopAutoSave();
                spdlog::info("Shutdown skipped persistence for readonly foreign world (format: {})",
                    m_storage->formatInfo().formatName);
            } else {
                // 注意：savePlayerRuntimeState() 由子类在 stop() 中调用，
                // 必须在 clearAll() 之前、维度管理器 shutdown 之前执行，
                // 以保证遍历玩家实体时它们仍存在于世界中。
                m_storage->shutdownAutoSave();
                auto saveResult = saveAllWorldData();
                if (saveResult.failed()) {
                    spdlog::error("Failed to save world during shutdown: {}", saveResult.error().message());
                }
            }
        }
    }

    // 2. 关闭成就事件处理器与脚本系统
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::shutdownManagers::ShutdownAdvancementAndScript");
        m_advancementEventHandler.shutdown();

        if (m_scriptManager) {
            m_scriptManager->shutdown();
        }
    }

    // 3. reset 交互与命令管理器
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::shutdownManagers::ResetInteractionManagers");
        m_inventoryManager.reset();
        m_containerManager.reset();
        m_miningManager.reset();
        m_blockInteractionManager.reset();
        m_commandRegistry.reset();
        m_commandStorage.reset();
    }

    // 4. 维度 shutdown + 共享存储关闭 + reset 剩余核心管理器
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::shutdownManagers::ResetCoreManagers");

        // 4a. 维度管理器 shutdown（可能涉及卸载维度/世界资源）+ reset
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization,
                "MinecraftServer::shutdownManagers::ResetCoreManagers::ShutdownDimensionManager");
            if (m_dimensionManager) {
                m_dimensionManager->shutdown();
            }
            m_dimensionManager.reset();
        }

        // 4b. 关闭共享存储（shutdownSharedStorage 内部已有独立 trace，会自动嵌套）
        shutdownSharedStorage();

        // 4c. reset 剩余核心管理器（轻量 unique_ptr 释放）
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization,
                "MinecraftServer::shutdownManagers::ResetCoreManagers::ResetRemainingManagers");
            m_gameModeManager.reset();
            m_positionTracker.reset();
            m_keepAliveManager.reset();
            m_teleportManager.reset();
            m_timeManager.reset();
            m_connectionManager.reset();
            m_playerManager.reset();
        }
    }
}

void MinecraftServer::tickEntities()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "MinecraftServer::tickEntities()", "phase", "entities");

    // 遍历所有维度执行实体 tick
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "MinecraftServer::tickEntities().Dim", "dim", dim.type().name());

        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (!world) {
            return;
        }

        world->entityManager().tick();

        // 物品拾取处理
        world->itemPickupManager().tick(*world, *this);

        // 实体追踪更新
        world->entityTracker().tick(*this, *world);
    });
}

void MinecraftServer::tickKeepAlive()
{
    // wall-clock 按需发送：getPlayersNeedingKeepAlive 用 (currentMs - lastSent) >= 15s 判定，
    // 与 TPS 无关，低 TPS 下发送间隔仍稳定 15s。一次 tick 内所有到期玩家共用同一 timestamp 作 id。
    u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
    auto players = m_keepAliveManager->getPlayersNeedingKeepAlive(currentTimeMs);
    if (players.empty()) {
        return;
    }
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "SendKeepAlive", "phase", "keepalive_send");
    u64 tick = currentTick();
    for (PlayerId playerId : players) {
        sendKeepAliveToPlayer(playerId, currentTimeMs, tick);
    }
}

void MinecraftServer::checkAllPlayersSleeping()
{
    // 跨维度聚合所有非旁观者玩家，判定是否全员睡熟。
    // 对应 MC Java 服务器级睡眠判定：判定范围（所有维度玩家）与副作用范围
    // （服务器级共享 TimeManager 跳夜）一致，修正原 ServerWorld 单维度判定却写
    // 全局 TimeManager 的层级错配。
    bool anySleeping = false;
    bool allSleeping = true;

    const auto dimIds = m_dimensionManager->getDimensionIds();
    for (DimensionId dimId : dimIds) {
        auto* dim = m_dimensionManager->getDimension(dimId);
        if (dim == nullptr || dim->world() == nullptr) {
            continue;
        }

        auto players = dim->world()->entityManager().getEntitiesByType(entity::EntityTypeKeys::PLAYER);
        for (Entity* entity : players) {
            auto* player = dynamic_cast<Player*>(entity);
            if (player == nullptr || player->isSpectator()) {
                continue;
            }

            if (player->isSleeping()) {
                anySleeping = true;
                if (!player->isPlayerFullyAsleep()) {
                    allSleeping = false;
                }
            } else {
                allSleeping = false;
            }
        }
    }

    // 没有玩家在睡，或并非全员睡熟：不跳夜。
    if (!anySleeping || !allSleeping) {
        return;
    }

    // 跳到下一个早晨（服务器级共享 TimeManager）。
    if (m_timeManager && m_timeManager->daylightCycleEnabled()) {
        i64 currentDayTime = m_timeManager->dayTime();
        m_timeManager->setDayTime(((currentDayTime / 24000) + 1) * 24000);
    }

    // 按维度清天气并唤醒玩家。
    for (DimensionId dimId : dimIds) {
        auto* dim = m_dimensionManager->getDimension(dimId);
        if (dim == nullptr || dim->world() == nullptr) {
            continue;
        }

        auto* world = dim->world();
        if (auto* weather = world->weatherManager(); weather != nullptr && weather->weatherCycleEnabled()) {
            weather->resetWeather();
        }

        auto players = world->entityManager().getEntitiesByType(entity::EntityTypeKeys::PLAYER);
        for (Entity* entity : players) {
            auto* player = dynamic_cast<Player*>(entity);
            if (player != nullptr && player->isSleeping()) {
                // 直接停止睡眠状态并重置计时器，与原 ServerWorld::wakeUpAllPlayers 行为一致。
                // 网络包同步由 Player::stopSleeping 内部链路处理。
                player->stopSleeping();
                player->setSleepTimer(0);
            }
        }
    }
}

void MinecraftServer::sendTimeUpdate()
{
    // MC 1.16.5: 按维度发送时间更新
    // - 主世界: 使用实际时间
    // - 下界: 固定为 18000 (午夜)
    // - 末地: 固定为 6000 (正午)

    const auto& time = timeManager().gameTimeObj();
    const i64 gameTime = time.gameTime();
    const bool daylightCycleEnabled = time.daylightCycleEnabled();

    // 按维度发送时间更新
    // 使用维度管理器获取每个维度的玩家列表
    constexpr DimensionId dimensions[] = {
        DimensionManager::OVERWORLD, DimensionManager::NETHER, DimensionManager::THE_END};
    for (DimensionId dimId : dimensions) {
        auto* dim = m_dimensionManager->getDimension(dimId);
        if (!dim || !dim->world()) {
            continue;
        }

        const auto& playerIds = dim->players();
        if (playerIds.empty()) {
            continue;
        }

        // 获取该维度的一天内时间 (0-23999)
        // dayTimeOfDay() 对于下界/末地会返回固定时间
        i64 tod = dim->world()->dayTimeOfDay();

        // 1.21.11 SetTime：gameTime + dayTime + tickDayTime(是否推进昼夜)
        mc::network::ir::play::SetTime pkt;
        pkt.gameTime = gameTime;
        pkt.dayTime = tod;
        pkt.tickDayTime = daylightCycleEnabled;

        mc::network::ir::IrPacket packet{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        };

        // 发送给该维度的所有玩家
        for (PlayerId playerId : playerIds) {
            sendPacketToPlayer(playerId, packet);
        }
    }
}

void MinecraftServer::sendKeepAliveToPlayer(PlayerId playerId, u64 timestamp, u64 tick)
{
    // 仅对已登录且连接有效的玩家发送。本地客户端（playerManager 中同样存在）经
    // sendPacketToPlayer 内部 m_localClientSender 直传，此守卫对其亦无害。
    auto* player = m_playerManager->getPlayer(playerId);
    if (player == nullptr || !player->loggedIn || !player->hasConnection()) {
        return;
    }

    // 1.21.11 KeepAlive：服务端发 id，客户端原样回 id。用时间戳作为 id。
    mc::network::ir::play::KeepAlive pkt;
    pkt.id = static_cast<i64>(timestamp);
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });

    m_keepAliveManager->recordKeepAliveSent(playerId, timestamp, tick);
}

void MinecraftServer::initializeCreativeInventory(PlayerInventory& inventory)
{
    inventory.clear();
    inventory.setSelectedSlot(0);
    fillCreativeModeInventory(inventory);
}

void MinecraftServer::sendChunkDataToPlayer(
    PlayerId playerId, ChunkCoord x, ChunkCoord z, const mc::network::ir::play::LevelChunkWithLight& ir)
{
    // 1.21.11 LevelChunkWithLight：IR 已由 ChunkSendManager 在 worker 线程经
    // buildLevelChunkWithLightIR 构建完成（vanilla 语义字段：heightmaps/sections/blockEntities/
    // lightMasks/lightUpdates）。此处仅按玩家拷贝进 IrPacket 发送：
    //   - 本地客户端经 LocalTransport 零拷贝直传 IR 结构体；
    //   - 远程 Java 客户端经 JavaBackend→levelChunkWithLightCodec 编码成 vanilla wire。
    MC_UNUSED(m_dimensionManager);
    mc::network::ir::play::LevelChunkWithLight pkt = ir;
    pkt.x = static_cast<i32>(x);
    pkt.z = static_cast<i32>(z);
    // 诊断日志：确认区块 LevelChunkWithLight 实际发往客户端（排查真 Java 客户端卡 loading 时
    // 区块是否送达）。排查完成后可降级或移除。
    spdlog::info("Sent chunk ({}, {}) to player {}", x, z, playerId);
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

void MinecraftServer::sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    // 1.21.11 ForgetLevelChunk(cb:37)：单个 ChunkPos，经 FriendlyByteBuf.writeChunkPos →
    // writeLong(ChunkPos.toLong) 编码为固定 8 字节大端 Long。ChunkPos.asLong 位布局：
    // X 低 32 位 @ bit0，Z 低 32 位 @ bit32。服务端在玩家走远/区块卸载时发送，通知客户端
    // 立即卸载对应区块（mesh 取消 + 缓存失效 + m_chunks erase）。客户端消费复用
    // ClientWorld::onChunkUnload。触发链为 ChunkSendManager::unloadChunkFromPlayers →
    // m_onChunkUnload 回调（见 initializeDimensions 内 setOnChunkUnload 注册）。
    mc::network::ir::play::ForgetLevelChunk pkt;
    const u64 ux = static_cast<u64>(static_cast<u32>(static_cast<i32>(x)));
    const u64 uz = static_cast<u64>(static_cast<u32>(static_cast<i32>(z)));
    pkt.packedPos = static_cast<i64>((uz << 32) | ux); // ChunkPos.asLong：X@bit0, Z@bit32
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

void MinecraftServer::broadcastLightUpdate(ChunkCoord x,
    ChunkCoord z,
    i32 sectionY,
    const std::vector<u8>& skyLight,
    const std::vector<u8>& blockLight,
    bool trustEdges)
{
    m_broadcaster->broadcastLightUpdate(x, z, sectionY, skyLight, blockLight, trustEdges);
}

u64 MinecraftServer::currentTick() const
{
    return m_timeManager ? m_timeManager->currentTick() : m_tickCounter;
}

// ============================================================================
// 维度感知世界访问
// ============================================================================

ServerWorld* MinecraftServer::getPlayerWorld(PlayerId playerId)
{
    auto* dim = m_dimensionManager->getPlayerDimensionWorld(playerId);
    return dim ? dim->world() : nullptr;
}

// ============================================================================
// Play 包处理：仅留 handleOpenPlayerInventoryPacket 基类默认实现（子类覆写打开菜单）。
// routeInboundPlayPacket 及 13 个非纯虚 handle*Packet 已于批7 下沉至 net::ServerPlayHandler。
// ============================================================================

void MinecraftServer::handleOpenPlayerInventoryPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 默认实现：仅校验包为 PlayerCommand{action=OPEN_INVENTORY}。具体打开逻辑由
    // IntegratedServer（本地客户端）覆写；StandaloneServer 远程 TCP 玩家路径暂未接入。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerCommand>(&play);
    if (evt == nullptr) {
        return;
    }
    (void)playerId;
    (void)evt;
}

// ============================================================================
// 数据包发送辅助方法
// ============================================================================

void MinecraftServer::sendTeleportPacket(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    // 1.21.11 PlayerPosition（S→C 传送）：teleportId + 坐标 + delta + 旋转 + relatives flag。
    mc::network::ir::play::PlayerPosition pkt;
    pkt.teleportId = static_cast<i32>(teleportId);
    pkt.x = x;
    pkt.y = y;
    pkt.z = z;
    pkt.deltaX = 0.0;
    pkt.deltaY = 0.0;
    pkt.deltaZ = 0.0;
    pkt.yRot = yaw;
    pkt.xRot = pitch;
    pkt.relatives = 0;
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

void MinecraftServer::onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory)
{
    MC_UNUSED(playerId);
    MC_UNUSED(inventory);
}

// 注：登录流程整簇（createPlayerForConnection/setupInitialPlayerState/
// sendLoginResponseForConnection/sendPermissionLevelChange/sendCommandTreePacket/
// sendInitialGameState/sendInitialDifficultyToPlayer）已于批6 下沉至 LoginFlow 门面。
// 调用方（IntegratedServer::_onClientPlayerReady、RemoteSessionManager::onPlayerReady）
// 经 m_loginFlow->createPlayerForConnection 进入；本类经 updateEntityTrackingForPlayer/
// serializeDifficultyPacket/sendPacketToPlayer 等 public 原语配合 LoginFlow。

i32 MinecraftServer::resolveOpLevel(const std::string& uuid) const noexcept
{
    return static_cast<i32>(m_opListManager->getLevel(uuid));
}

void MinecraftServer::savePlayerRuntimeState()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::savePlayerRuntimeState");

    // 外来只读存档不写盘，直接跳过
    if (isSharedStorageReadonlyForeignWorld()) {
        return;
    }

    auto* storage = sharedStorage();
    if (storage == nullptr || !storage->isOpen()) {
        return;
    }
    auto* pdm = storage->playerDataManager();
    if (pdm == nullptr) {
        return;
    }

    // 遍历所有维度，对每个在线 Player 实体回写运行时状态到 PlayerDataManager 缓存。
    // 必须在 stopCore()（含 shutdownManagers）之前调用，否则维度已卸载、玩家实体已销毁。
    size_t savedCount = 0;
    m_dimensionManager->forEachDimension([&](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world == nullptr) {
            return;
        }

        const auto playerIds = m_playerEntityManager.getPlayerIds();
        for (PlayerId playerId : playerIds) {
            Player* player = m_playerEntityManager.getPlayerEntity(playerId, *world);
            if (player == nullptr) {
                continue;
            }

            // fromPlayer() 提取位置、生命、饥饿、经验、背包、效果等运行时状态；
            // savePlayer() 同时更新缓存并标记脏，后续 saveAllWorldData() 会落盘。
            auto saveData = world::storage::PlayerDataManager::fromPlayer(*player);

            // Player 实体的 m_uuid 由登录流程计算离线 UUID 后存入 ServerPlayerData，
            // 但未回写到实体本身。这里用 PlayerManager 中的权威 UUID 覆盖 saveData.uuid，
            // 避免以空字符串作为键落盘导致数据丢失。
            if (auto* playerData = m_playerManager->getPlayer(playerId)) {
                if (!playerData->uuid.empty()) {
                    saveData.uuid = playerData->uuid;
                }
            }

            auto result = pdm->savePlayer(saveData);
            if (result.success()) {
                ++savedCount;
            } else {
                spdlog::warn(
                    "Failed to save player runtime state for playerId={}: {}", playerId, result.error().message());
            }
        }
    });

    if (savedCount > 0) {
        spdlog::info("Saved runtime state for {} player(s) before shutdown", savedCount);
    }
}

void MinecraftServer::pollNetwork()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "PollNetwork");
    // 主线程驱动：tick pump Local（集成服本地客户端）+ drain Wire 入站队列（接收线程
    // enqueueInbound，主线程 drainInbound 派发握手/Play）+ 派发延迟断开。批2a 统一自
    // 两子类（实现完全一致）。远程玩家的玩家创建/断开清理在 _onRemotePlayerReady/
    // _onRemoteClientDisconnect 内（经 tick 回调，主线程）。
    if (m_serverNetwork) {
        m_serverNetwork->tick();
        _drainDisconnectedSessions();
    }
}

void MinecraftServer::broadcastPacket(const mc::network::ir::IrPacket& packet)
{
    // 本地客户端（若注入钩子）：经 m_localClientSender 直传（LocalTransport 零拷贝）。
    if (m_localClientSender) {
        m_localClientSender(packet);
    }

    // 远程玩家：经 PlayerManager 遍历，player.send 走各自 ServerClientConnection。
    // 跳过本地客户端（m_localClientPlayerId，已由钩子发送），否则双重发送。
    const std::optional<PlayerId> localPlayerId = m_localClientPlayerId;
    m_playerManager->forEachPlayer([&packet, localPlayerId](ServerPlayerData& player) {
        if (localPlayerId.has_value() && player.playerId == *localPlayerId) {
            return;
        }
        if (player.loggedIn && player.hasConnection()) {
            player.send(mc::network::ir::IrPacket{packet});
        }
    });
}

PlayerId MinecraftServer::getPlayerIdForSession(u32 sessionId) const
{
    // sessionId == 0 且注入本地客户端钩子：返回本地客户端 playerId。
    if (sessionId == 0 && m_localClientPlayerId.has_value()) {
        return *m_localClientPlayerId;
    }
    return m_playerManager->getPlayerIdBySession(sessionId);
}

void MinecraftServer::sendPacketToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 本地客户端（若注入钩子）：经 m_localClientSender 直传（LocalTransport 零拷贝）。
    if (m_localClientPlayerId.has_value() && playerId == *m_localClientPlayerId) {
        if (m_localClientSender) {
            m_localClientSender(packet);
        }
        return;
    }

    // 远程 TCP 玩家：经 ServerPlayerData::send 走其 ServerClientConnection。
    auto* player = m_playerManager->getPlayer(playerId);
    if (player != nullptr) {
        player->send(mc::network::ir::IrPacket{packet});
    }
}

void MinecraftServer::stopCore()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::stopCore");

    spdlog::info("Stopping server core...");

    // 停止 Worker 线程池，避免后续关服过程中继续提交任务
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::stopCore::ShutdownWorkerPools");
        m_computationWorkerPool.shutdown();
        m_ioWorkerPool.shutdown();
    }

    // 断开所有玩家
    if (m_connectionManager) {
        m_connectionManager->disconnectAll("Server shutting down");
    }

    // 关闭管理器
    shutdownManagers();

    spdlog::info("Server core stopped.");
}

// ============================================================================
// 声音广播方法
// ============================================================================

// ============================================================================
// 广播方法（薄转调：逻辑已下沉至 net::PlayerBroadcaster）
// ============================================================================

void MinecraftServer::broadcastSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    m_broadcaster->broadcastSound(soundEventId, category, position, volume, pitch);
}

void MinecraftServer::broadcastSoundInRange(const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 range,
    f32 volume,
    f32 pitch)
{
    m_broadcaster->broadcastSoundInRange(soundEventId, category, position, range, volume, pitch);
}

// ============================================================================
// 粒子广播
// ============================================================================

void MinecraftServer::broadcastParticleInRange(particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    f32 range)
{
    m_broadcaster->broadcastParticleInRange(type, pos, velocity, offset, count, range);
}

void MinecraftServer::sendParticleToPlayer(PlayerId playerId,
    particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    m_broadcaster->sendParticleToPlayer(playerId, type, pos, velocity, offset, count);
}

void MinecraftServer::broadcastVibrationParticleInRange(
    const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks, f32 range)
{
    m_broadcaster->broadcastVibrationParticleInRange(pos, targetSource, arrivalInTicks, range);
}

void MinecraftServer::broadcastTrailParticleInRange(
    const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks, f32 range)
{
    m_broadcaster->broadcastTrailParticleInRange(pos, targetPosition, color, durationInTicks, range);
}

void MinecraftServer::broadcastEntityEffectParticleInRange(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color, f32 range)
{
    m_broadcaster->broadcastEntityEffectParticleInRange(pos, velocity, offset, count, color, range);
}

void MinecraftServer::broadcastBlockParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, u32 blockStateId, f32 range)
{
    m_broadcaster->broadcastBlockParticleInRange(type, pos, velocity, blockStateId, range);
}

void MinecraftServer::broadcastItemParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack, f32 range)
{
    m_broadcaster->broadcastItemParticleInRange(type, pos, velocity, itemStack, range);
}

// ============================================================================
// 实体事件/动画
// ============================================================================

void MinecraftServer::broadcastEntityStatusInRange(EntityInstanceId entityId, u8 status, const Vector3& pos, f32 range)
{
    m_broadcaster->broadcastEntityStatusInRange(entityId, status, pos, range);
}

void MinecraftServer::broadcastEntityAnimationInRange(
    EntityInstanceId entityId, u8 animation, const Vector3& pos, f32 range)
{
    m_broadcaster->broadcastEntityAnimationInRange(entityId, animation, pos, range);
}

void MinecraftServer::broadcastHurtAnimationInRange(
    EntityInstanceId entityId, f32 hurtDir, const Vector3& pos, f32 range)
{
    m_broadcaster->broadcastHurtAnimationInRange(entityId, hurtDir, pos, range);
}

void MinecraftServer::broadcastSetEntityLinkInRange(
    EntityInstanceId entityId, EntityInstanceId linkedEntityId, const Vector3& pos, f32 range)
{
    m_broadcaster->broadcastSetEntityLinkInRange(entityId, linkedEntityId, pos, range);
}

// ============================================================================
// 世界事件
// ============================================================================

void MinecraftServer::broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    m_broadcaster->broadcastWorldEvent(eventId, x, y, z, data);
}

void MinecraftServer::broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range)
{
    m_broadcaster->broadcastWorldEventInRange(eventId, x, y, z, data, range);
}

void MinecraftServer::broadcastBlockEventInRange(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockId, f32 range)
{
    m_broadcaster->broadcastBlockEventInRange(x, y, z, paramA, paramB, blockId, range);
}

void MinecraftServer::broadcastBlockEntityInRange(
    const BlockPos& pos, BlockEntityType type, std::shared_ptr<nbt::CompoundTag> tag, f32 range)
{
    m_broadcaster->broadcastBlockEntityInRange(pos, type, std::move(tag), range);
}

void MinecraftServer::broadcastBlockBreakProgressInRange(
    EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress, f32 range)
{
    m_broadcaster->broadcastBlockBreakProgressInRange(breakerId, x, y, z, progress, range);
}

// ============================================================================
// 爆炸
// ============================================================================

void MinecraftServer::broadcastExplosionInRange(const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback,
    f32 range)
{
    m_broadcaster->broadcastExplosionInRange(position, strength, affectedBlocks, playerKnockback, range);
}

void MinecraftServer::sendExplosionToPlayer(PlayerId playerId,
    const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback)
{
    m_broadcaster->sendExplosionToPlayer(playerId, position, strength, affectedBlocks, playerKnockback);
}

// ============================================================================
// 共享子逻辑（批9 下沉自 IntegratedServer/StandaloneServer 重复实现）
// ============================================================================

void MinecraftServer::_updateTickDebugStats(f32 tickTimeMs)
{
    // 指数移动平均平滑 tick 耗时（首帧直接取本次值）
    if (m_smoothedTickTimeMs == 0.0f) {
        m_smoothedTickTimeMs = tickTimeMs;
    } else {
        m_smoothedTickTimeMs =
            m_smoothedTickTimeMs * (1.0f - kTickTimeSmoothFactor) + tickTimeMs * kTickTimeSmoothFactor;
    }
    m_debugStats.smoothedTickTimeMs.store(m_smoothedTickTimeMs, std::memory_order::relaxed);

    // 更新强制区块计数（从主世界 chunkManager ticketManager 取）
    if (m_dimensionManager != nullptr) {
        if (auto* overworld = m_dimensionManager->getOverworld(); overworld != nullptr) {
            if (auto* world = overworld->world(); world != nullptr) {
                if (auto* chunkMgr = world->chunkManager(); chunkMgr != nullptr) {
                    const auto& ticketManager = chunkMgr->ticketManager();
                    m_debugStats.forcedChunkCount.store(
                        static_cast<i32>(ticketManager.getForcedChunks().size()), std::memory_order::relaxed);
                }
            }
        }
    }
}

Result<void> MinecraftServer::_setupRemoteSessions(std::string_view logPrefix,
    i32 compressionThreshold,
    std::function<mc::server::net::RemoteWorldParams()> worldParamsProvider,
    u16 port,
    u32 maxConnections)
{
    // 批2c：远程会话四件套下沉至 RemoteSessionManager 门面。worldParams 由调用方经
    // provider 注入（StandaloneServer 取 m_settings，IntegratedServer 取 m_params）。
    m_remoteSessionManager = std::make_unique<mc::server::net::RemoteSessionManager>(
        *this, std::string(logPrefix), compressionThreshold, std::move(worldParamsProvider));
    m_serverNetwork->onClientConnect(
        [this](mc::server::net::ServerClientConnection& conn) { m_remoteSessionManager->onClientConnect(conn); });
    m_serverNetwork->onClientDisconnect(
        [this](mc::server::net::ServerClientConnection& conn) { m_remoteSessionManager->onClientDisconnect(conn); });

    // startAccept 失败时直接透传其原始 Error，由调用方按各自场景包装日志前缀
    // （StandaloneServer "Failed to start server" / IntegratedServer "Failed to start LAN server"）。
    return m_serverNetwork->startAccept(port, maxConnections);
}

void MinecraftServer::_shutdownRemoteSessions() noexcept
{
    // 先清远程会话（session 持 ServerClientConnection& 引用，须先于连接销毁），
    // 再 reset 网络门面（关 acceptor + join accept 线程 + 析构各 ServerClientConnection）。
    // 对未装配的空 unique_ptr reset 幂等（IntegratedServer 单机未发布 LAN 时为 nullptr）。
    m_remoteSessionManager.reset();
    if (m_serverNetwork) {
        m_serverNetwork.reset();
    }
}

void MinecraftServer::_registerDistanceCallbacks()
{
    // viewDistance 运行时变更：下发各维度 ServerWorld（setConfig 触发 ChunkManager setViewDistance）
    // + 广播 SetChunkCacheRadius(id=93)。原 StandaloneServer 实现仅更新主世界且不广播，此处修正为
    // 全维度覆盖 + 广播，对齐原版 PlayerList.setViewDistance。
    m_settings.viewDistance.onChange([this](i32 value) {
        spdlog::info("View distance changed to: {}", value);
        m_dimensionManager->forEachDimension([value](Dimension& dim) {
            auto* serverDim = static_cast<ServerDimension*>(&dim);
            if (serverDim->world() != nullptr) {
                auto config = serverDim->world()->config();
                config.viewDistance = value;
                serverDim->world()->setConfig(config);
            }
        });
        mc::network::ir::play::SetChunkCacheRadius pkt;
        pkt.radius = value;
        broadcastPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
    });

    // simulationDistance 运行时变更：下发各维度 EntityManager（实体激活范围）+ 广播
    // SetSimulationDistance(id=109)。对齐原版 PlayerList.setSimulationDistance。
    m_settings.simulationDistance.onChange([this](i32 value) {
        spdlog::info("Simulation distance changed to: {}", value);
        m_dimensionManager->forEachDimension([value](Dimension& dim) {
            auto* serverDim = static_cast<ServerDimension*>(&dim);
            if (serverDim->world() != nullptr) {
                serverDim->world()->entityManager().setSimulationDistance(value);
            }
        });
        mc::network::ir::play::SetSimulationDistance pkt;
        pkt.simulationDistance = value;
        broadcastPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
    });
}

void MinecraftServer::_handleHotbarSelectRemote(PlayerId playerId, i32 slot)
{
    inventoryManager().setSelectedSlot(playerId, slot);

    // 服务端回送确认包（1.21.11 SetHeldSlot）
    mc::network::ir::play::SetHeldSlot resp;
    resp.slot = slot;
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(resp)},
        });
}

void MinecraftServer::_handleCloseContainerRemote(PlayerId playerId)
{
    containerManager().closeContainer(playerId);
    inventoryManager().syncToClient(playerId);
}

void MinecraftServer::_handleContainerClickRemote(
    PlayerId playerId, const mc::network::ir::play::ContainerClick& evt, const ItemStack& cursorItem)
{
    // stateId 一致性校验：boolean flag = pkt.stateId != menu.getStateId()；
    // 不一致→broadcastFullState，一致→broadcastChanges。
    // 本项目容器同步采用全量重发模型（ContainerManager::handleClick 内部 m_onContainerUpdate
    // 已全量下发 ContainerSetContent），故无论是否一致都走全量重发，校验仅记录诊断日志。
    if (const auto* menu = containerManager().getOpenMenu(playerId);
        menu != nullptr && menu->getId() == static_cast<mc::ContainerId>(evt.containerId)) {
        if (evt.stateId != menu->getStateId()) {
            spdlog::warn("ContainerClick stateId mismatch (playerId={} cid={} client={} server={}): full resync",
                playerId,
                evt.containerId,
                evt.stateId,
                menu->getStateId());
        }
    }

    auto clickResult = containerManager().handleClick(playerId,
        static_cast<mc::ContainerId>(evt.containerId),
        evt.slotNum,
        static_cast<u8>(evt.buttonNum),
        static_cast<u8>(evt.clickType),
        cursorItem);

    if (clickResult.success()) {
        // 同步物品栏到客户端
        inventoryManager().syncToClient(playerId);
    }
}

ItemStack MinecraftServer::hashedStackToItemStack(const mc::network::ir::play::HashedStack& hashed)
{
    if (!hashed.present || hashed.itemId == 0 || hashed.count <= 0) {
        return ItemStack();
    }
    // wire 上的 hashed.itemId 是 vanilla registry id，须经 JavaItemIdMap 反查为项目内部 ItemId。
    const mc::ItemId internalItemId =
        mc::network::backend::java::JavaItemIdMap::instance().fromJavaRegistryId(hashed.itemId);
    auto* item = mc::ItemRegistry::instance().getItem(internalItemId);
    if (item == nullptr) {
        return ItemStack();
    }
    return ItemStack(*item, hashed.count);
}

// ============================================================================
// 数据包处理 / inventory 查询：基类默认实现（远程玩家路径）
// 两子类原纯虚，去纯虚后基类提供远程默认；IntegratedServer 覆写追加本地客户端分支，
// StandaloneServer 不再覆写继承基类默认（纯远程行为）。
// ============================================================================

void MinecraftServer::handleHotbarSelectPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 1.21.11 SetCarriedItem（C→S）：玩家切换快捷栏选中槽位。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SetCarriedItem>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* player = m_playerManager->getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }

    _handleHotbarSelectRemote(playerId, evt->slot);
}

void MinecraftServer::handleContainerClickPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 1.21.11 ContainerClick（C→S）：stateId + slot + button + clickType + carriedItem(HashedStack)。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ContainerClick>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* player = m_playerManager->getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }

    // 还原光标物品：HashedStack 仅 itemId+count（组件哈希不可逆），还原为基础物品+数量。
    // 远程玩家（StandaloneServer / IntegratedServer 远程分支）走此基类默认实现。
    const ItemStack cursorItem = hashedStackToItemStack(evt->carriedItem);
    _handleContainerClickRemote(playerId, *evt, cursorItem);
}

void MinecraftServer::handleCloseContainerPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 1.21.11 ContainerClose（C→S）：containerId。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ContainerClose>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* player = m_playerManager->getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }

    _handleCloseContainerRemote(playerId);
}

ItemStack MinecraftServer::getHeldItemForPlacement(PlayerId playerId)
{
    return inventoryManager().getHeldItem(playerId);
}

i32 MinecraftServer::getSelectedHotbarSlot(PlayerId playerId)
{
    return inventoryManager().getSelectedSlot(playerId);
}

void MinecraftServer::setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack)
{
    inventoryManager().setItem(playerId, slotIndex, stack);
}

void MinecraftServer::syncPlayerInventory(PlayerId playerId)
{
    inventoryManager().syncToClient(playerId);
}

bool MinecraftServer::tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos)
{
    auto openResult = containerManager().openContainer(playerId, mc::ContainerType::Crafting, pos);
    return openResult.success();
}

} // namespace mc::server
