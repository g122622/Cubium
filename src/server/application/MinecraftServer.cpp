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
#include "common/advancement/AdvancementLoader.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/damage/tag/DamageTypeTagLoader.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/tag/EntityTypeTagLoader.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/special/ArmorDyeRecipe.hpp"
#include "common/item/crafting/special/BookCloningRecipe.hpp"
#include "common/item/crafting/special/DecoratedPotRecipe.hpp"
#include "common/item/crafting/special/MapCloningRecipe.hpp"
#include "common/item/crafting/special/MapExtendingRecipe.hpp"
#include "common/item/crafting/special/RepairItemRecipe.hpp"
#include "common/item/crafting/special/TippedArrowRecipe.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/loot/LootPredicateLoader.hpp"
#include "common/item/loot/LootTableLoader.hpp"
#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/packet/BlockBreakAnimPacket.hpp"
#include "common/network/packet/BlockEventPacket.hpp"
#include "common/network/packet/CommandTreePacket.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/ServerDifficultyPacket.hpp"
#include "common/network/packet/SetEntityLinkPacket.hpp"
#include "common/network/packet/SpawnPositionPacket.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/load/ChunkLoadTicket.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "common/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "common/world/gen/feature/FeatureTypeRegistry.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "common/world/gen/placement/PlacedFeatureLoader.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureTagLoader.hpp"
#include "common/world/gen/structure/StructureTags.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/storage/db/ConsistencyMode.hpp"
#include "common/world/storage/save/AutoSave.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/ServerDragonBossBar.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/function/FunctionLoader.hpp"
#include "server/mod/bedrock/addon/ServerScriptManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/sync/BlockUpdateSyncManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include "server/sync/EntitySyncManager.hpp"
#include "server/sync/LightSyncManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include <chrono>
#include <filesystem>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

namespace {

[[nodiscard]] bool isCraftingTableState(const BlockState* state)
{
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

[[nodiscard]] bool isReadonlyForeignStorage(const world::storage::SingleLevelStorageManager* storage)
{
    return storage != nullptr && storage->isOpen() && storage->config().readonly && storage->isForeignFormat();
}

} // namespace

MinecraftServer::MinecraftServer(ServerSettings& settings)
    : m_settings(settings)
    , m_computationWorkerPool(-1, "ServerCompute")
    , m_ioWorkerPool(-1, "ServerIO")
    , m_lootTableManager()
{}

void MinecraftServer::setDifficulty(Difficulty difficulty)
{
    if (m_difficulty == difficulty) {
        return; // 难度未变化，无需同步
    }
    m_difficulty = difficulty;

    // 广播难度变更给所有玩家
    broadcastDifficultyChange();
}

void MinecraftServer::broadcastDifficultyChange()
{
    auto fullPacket = serializeDifficultyPacket();
    if (fullPacket.empty()) {
        return;
    }

    broadcastPacket(fullPacket.data(), fullPacket.size());
    spdlog::info("Difficulty changed to {}", static_cast<i32>(m_difficulty));
}

std::vector<u8> MinecraftServer::serializeDifficultyPacket()
{
    network::ServerDifficultyPacket packet(m_difficulty, false);
    auto serializeResult = packet.serialize();
    if (serializeResult.failed()) {
        spdlog::error("Failed to serialize ServerDifficultyPacket");
        return {};
    }

    return core::ConnectionManager::encapsulatePacket(network::PacketType::ServerDifficulty, serializeResult.value());
}

void MinecraftServer::setDefaultGameMode(GameMode mode)
{
    m_settings.defaultGameMode.set(static_cast<i32>(mode));
}

void MinecraftServer::setPlayerIdleTimeoutMinutes(i32 timeoutMinutes)
{
    m_playerIdleTimeoutMinutes = timeoutMinutes;
}

void MinecraftServer::broadcastServerMessage(std::string_view message)
{
    const std::string text(message);
    spdlog::info("[System] {}", text);
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

    // 检查心跳超时
    if (m_tickCounter % KEEPALIVE_INTERVAL == 0) {
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

    // 心跳（每 15 秒）
    tickKeepAlive();

    // 每 20 tick 同步一次时间
    u64 tick = currentTick();
    if (tick % 20 == 0) {
        sendTimeUpdate();
    }

    // 同步天气变化
    sendWeatherUpdate();

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
    m_keepAliveManager = std::make_unique<core::KeepAliveManager>(
        *m_playerManager, m_settings.keepAliveInterval.get(), m_settings.keepAliveTimeout.get());
    m_positionTracker = std::make_unique<core::PositionTracker>(*m_playerManager, m_settings.viewDistance.get());
    m_packetHandler = std::make_unique<core::PacketHandler>(*m_playerManager,
        *m_connectionManager,
        *m_teleportManager,
        *m_keepAliveManager,
        *m_positionTracker,
        *m_timeManager,
        static_cast<GameMode>(m_settings.defaultGameMode.get()));
    m_gameModeManager = std::make_unique<core::GameModeManager>(*m_playerManager, *m_connectionManager);

    // 注册游戏模式变化回调：当从旁观者模式切换到其他模式时，重置旁观目标
    m_gameModeManager->setOnGameModeChange([this](PlayerId playerId, GameMode oldMode, GameMode newMode) {
        if (oldMode == GameMode::Spectator && newMode != GameMode::Spectator) {
            // 从旁观者模式切换出来时，重置旁观目标并发送 SetCameraPacket
            // 需要找到 ServerPlayer 实体来操作
            auto* playerData = m_playerManager->getPlayer(playerId);
            if (playerData == nullptr) {
                return;
            }

            // 遍历所有维度世界寻找玩家实体
            for (DimensionId dimId : m_dimensionManager->getDimensionIds()) {
                auto* dimension = m_dimensionManager->getDimension(dimId);
                if (dimension == nullptr || dimension->world() == nullptr) {
                    continue;
                }
                Entity* entity = dimension->world()->getEntity(static_cast<EntityId>(playerId));
                if (entity == nullptr) {
                    continue;
                }
                auto* serverPlayer = dynamic_cast<mc::ServerPlayer*>(entity);
                if (serverPlayer != nullptr) {
                    serverPlayer->resetCamera();
                    // 同时更新 Player 实体的游戏模式、能力和 noclip 状态
                    serverPlayer->setGameMode(newMode);
                    break;
                }
            }
        } else if (newMode == GameMode::Spectator) {
            // 切换到旁观者模式时，更新 Player 实体的游戏模式和 noclip
            auto* playerData = m_playerManager->getPlayer(playerId);
            if (playerData == nullptr) {
                return;
            }
            for (DimensionId dimId : m_dimensionManager->getDimensionIds()) {
                auto* dimension = m_dimensionManager->getDimension(dimId);
                if (dimension == nullptr || dimension->world() == nullptr) {
                    continue;
                }
                Entity* entity = dimension->world()->getEntity(static_cast<EntityId>(playerId));
                if (entity == nullptr) {
                    continue;
                }
                auto* serverPlayer = dynamic_cast<mc::ServerPlayer*>(entity);
                if (serverPlayer != nullptr) {
                    serverPlayer->setGameMode(newMode);
                    break;
                }
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
    m_dimensionManager->setDimensionChangeCallback(
        [this](PlayerId playerId, DimensionId fromDim, DimensionId toDim, const Vector3d& position) {
            auto* player = m_playerManager->getPlayer(playerId);
            if (!player) {
                return;
            }

            m_positionTracker->updatePosition(
                playerId, position.x, position.y, position.z, player->yaw, player->pitch, player->onGround);
            updateEntityTrackingForPlayer(playerId, position.x, position.y, position.z);

            // 发送维度特定时间更新
            // 当玩家切换到下界或末地时，需要发送固定时间
            auto* targetDim = m_dimensionManager->getDimension(toDim);
            if (targetDim && targetDim->world()) {
                const auto& time = timeManager().gameTimeObj();
                i64 dayTime = targetDim->world()->dayTime();

                network::TimeUpdatePacket timePacket(time.gameTime(), dayTime, time.daylightCycleEnabled());
                network::PacketSerializer timeSer;
                timePacket.serialize(timeSer);
                auto fullTimePacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::TimeUpdate, timeSer.buffer());
                sendPacketToPlayer(playerId, fullTimePacket.data(), fullTimePacket.size());
            }
        });

    // Worker 池由服务器统一管理，初始化阶段在这里启动。
    m_computationWorkerPool.start();
    m_ioWorkerPool.start();
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
    world.setOnBroadcastEntityStatus([this, &world](EntityId entityId, u8 status) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastEntityStatusInRange(entityId, status, entity->position());
        }
    });
    world.setOnBroadcastEntityAnimation([this, &world](EntityId entityId, u8 animation) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastEntityAnimationInRange(entityId, animation, entity->position());
        }
    });
    world.setOnBroadcastSetEntityLink([this, &world](EntityId entityId, EntityId linkedEntityId) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastSetEntityLinkInRange(entityId, linkedEntityId, entity->position());
        }
    });
    world.setOnBroadcastWorldEvent(
        [this](i32 eventId, i32 x, i32 y, i32 z, i32 data) { broadcastWorldEventInRange(eventId, x, y, z, data); });
    world.setOnBroadcastBlockEvent([this](i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId) {
        broadcastBlockEventInRange(x, y, z, paramA, paramB, blockStateId);
    });
    world.setOnDestroyBlockProgress([this](EntityId breakerId, i32 x, i32 y, i32 z, i32 progress) {
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
    world.setTimeManager(m_timeManager.get());
    world.setDifficultyCallback([this]() { return this->difficulty(); });
    world.setLootTableManager(&m_lootTableManager);
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
        Vector3d spawnPoint = world->worldSpawnPoint();
        i32 spawnX = static_cast<i32>(std::floor(spawnPoint.x));
        i32 spawnY = static_cast<i32>(std::floor(spawnPoint.y));
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
            thundering);

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

            m_dimensionManager->forEachDimension([&runtimeData](Dimension& dim) {
                auto* serverDim = static_cast<ServerDimension*>(&dim);
                auto* world = serverDim->world();
                if (world == nullptr) {
                    return;
                }

                if (serverDim->id() == 0) {
                    world->applyLevelRuntimeData(runtimeData);
                } else {
                    world->initializeWorldSpawn();
                }
            });
        } else {
            spdlog::warn("Failed to load level runtime data: {}", runtimeDataResult.error().message());
            m_dimensionManager->forEachDimension([](Dimension& dim) {
                auto* serverDim = static_cast<ServerDimension*>(&dim);
                auto* world = serverDim->world();
                if (world != nullptr) {
                    world->initializeWorldSpawn();
                }
            });
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
        PlayerInventoryPacket packet(inventory);
        network::PacketSerializer payload;
        packet.serialize(payload);

        const auto fullPacket =
            core::ConnectionManager::encapsulatePacket(network::PacketType::PlayerInventory, payload.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
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

    // 设置 EntityId 解析器：将 PlayerId 转换为正确的 EntityId
    // MiningManager 内部只有 PlayerId，但广播破坏动画需要 EntityId 作为 breakerId
    m_miningManager->setEntityIdResolver(
        [this](PlayerId playerId) -> EntityId { return playerEntityManager().getPlayerEntityId(playerId); });

    // 设置破坏动画广播回调：将挖掘进度通过 ServerWorld::destroyBlockProgress 广播给其他玩家
    // 对应 MC Java: ServerPlayerGameMode 中调用 level.destroyBlockProgress(entityId, pos, stage)
    m_miningManager->setOnBreakAnimBroadcast([this](PlayerId playerId, i32 x, i32 y, i32 z, i8 stage) {
        EntityId entityId = playerEntityManager().getPlayerEntityId(playerId);
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

void MinecraftServer::registerSpecialRecipes()
{
    using namespace crafting;

    // 注册物品修复配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<RepairItemRecipe>(ResourceLocation("minecraft", "repair_item")));

    // 注册盔甲染色配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<ArmorDyeRecipe>(ResourceLocation("minecraft", "armor_dye")));

    // 注册书复制配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<BookCloningRecipe>(ResourceLocation("minecraft", "book_cloning")));

    // 注册地图复制配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<MapCloningRecipe>(ResourceLocation("minecraft", "map_cloning")));

    // 注册地图扩展配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<MapExtendingRecipe>(ResourceLocation("minecraft", "map_extending")));

    // 注册药水箭配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<TippedArrowRecipe>(ResourceLocation("minecraft", "tipped_arrow")));

    // 注册饰纹陶罐配方
    RecipeManager::instance().registerRecipe(
        std::make_unique<DecoratedPotRecipe>(ResourceLocation("minecraft", "decorated_pot")));

    spdlog::info("Special recipes registered (7 recipes)");
}

void MinecraftServer::initializeRegistries(bool registerEntities)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries");

    // 初始化方块注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Blocks");
        VanillaBlocks::initialize();
    }
    spdlog::info("Vanilla blocks initialized");

    // 初始化物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Items");
        Items::initialize();
    }
    spdlog::info("Vanilla items initialized");

    // 初始化唱片机歌曲注册表（必须在 SoundEvents 初始化后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::JukeboxSongs");
        JukeboxSongs::initialize();
    }
    spdlog::info("Jukebox songs initialized");

    // 初始化附魔注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Enchantments");
        item::enchant::EnchantmentRegistry::initialize();
    }
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::BlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
    spdlog::info("Block items initialized");

    // 初始化物品标签（必须在所有物品注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::ItemTags");
        item::tag::ItemTags::initialize();
    }
    spdlog::info("Item tags initialized");

    // 从数据包加载物品标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::ItemTagLoader");
        auto dataPackLoadResult = item::tag::ItemTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load item tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} item tags from data packs", dataPackLoadResult.value());
        }
    }

    // 初始化发射器行为注册表
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::DispenseBehaviors");
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
    }
    spdlog::info("Dispense item behaviors initialized");

    // 初始化战利品表管理器（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::LootTables");
        loot::LootTableLoader lootLoader(m_lootTableManager);
        auto dataPackLoadResult = lootLoader.loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load loot tables from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            const auto& result = dataPackLoadResult.value();
            spdlog::info("Loaded {} loot tables from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Loot table error: {}", err);
            }
        }
    }

    // 初始化战利品谓词管理器（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Predicates");
        loot::LootPredicateLoader predicateLoader(m_predicateManager);
        auto dataPackLoadResult = predicateLoader.loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load predicates from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            const auto& result = dataPackLoadResult.value();
            spdlog::info("Loaded {} predicates from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Predicate error: {}", err);
            }
        }
        // 将谓词管理器关联到掉落表管理器，使 LootContext 可通过掉落表管理器解析命名谓词
        m_lootTableManager.setPredicateManager(&m_predicateManager);
    }

    // 加载配方（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Recipes");
        RecipeLoader recipeLoader;
        auto dataPackLoadResult = recipeLoader.loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load crafting recipes from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} crafting recipes from data packs ({} failed)",
                dataPackLoadResult.value().successCount,
                dataPackLoadResult.value().failedCount);
        }

        // 注册特殊配方（动态配方，不从数据包加载）
        registerSpecialRecipes();
    }

    // 加载函数（从数据包加载 .mcfunction 文件）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Functions");
        function::FunctionLoader functionLoader(m_functionManager);
        auto funcLoadResult = functionLoader.loadFromDataPackRepository(m_dataPackList);
        if (funcLoadResult.failed()) {
            spdlog::error("Failed to load functions from data packs: {}", funcLoadResult.error().toString());
        } else {
            const auto& result = funcLoadResult.value();
            spdlog::info("Loaded {} functions from data packs ({} failed, {} macros skipped)",
                result.successCount,
                result.failedCount,
                result.skippedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Function error: {}", err);
            }
        }
        m_functionManager.notifyReload();
    }

    // 加载进度（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Advancements");
        mc::advancement::AdvancementLoader advancementLoader;
        auto advancementLoadResult = advancementLoader.loadFromDataPackRepository(m_dataPackList);
        if (advancementLoadResult.failed()) {
            spdlog::error("Failed to load advancements from data packs: {}", advancementLoadResult.error().toString());
        } else {
            const auto& result = advancementLoadResult.value();
            spdlog::info("Loaded {} advancements from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Advancement error: {}", err);
            }
        }
    }

    // 加载模板池（从数据包加载）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::TemplatePools");
        size_t poolCount = world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(m_dataPackList);
        spdlog::info("Loaded {} template pools from data packs", poolCount);
    }

    // 加载处理器列表（从数据包加载，补充硬编码注册未覆盖的列表）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::ProcessorLists");
        auto processorResult = world::gen::jigsaw::ProcessorListLoader::loadFromDataPackRepository(m_dataPackList);
        if (processorResult.success()) {
            spdlog::info("Loaded {} processor lists from data packs", processorResult.value());
        } else {
            spdlog::warn("Failed to load processor lists from data packs: {}", processorResult.error().message());
        }
    }

    // 设置 JigsawAssembler 的 TemplateManager 数据包列表（用于加载结构模板 .nbt 文件）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::JigsawTemplateManager");
        world::gen::jigsaw::JigsawAssembler::getTemplateManager().setDataPackRepository(&m_dataPackList);
        spdlog::info("Jigsaw TemplateManager configured with data pack list");
    }

    // ============================================================================
    // 数据驱动世界生成管线
    // ============================================================================
    // 顺序：放置器类型 → 特征类型 → configured_feature → placed_feature →
    //       configured_carver → biome（biome 必须最后，引用上述注册表）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::PlacementRegistry");
        PlacementRegistry::instance().initialize();
    }
    spdlog::info("Placement registry initialized");

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::FeatureTypeRegistry");
        world::gen::feature::initializeBuiltinFeatureTypes();
    }
    spdlog::info("Builtin feature types initialized");

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::ConfiguredFeatures");
        auto featureResult = world::gen::feature::ConfiguredFeatureLoader::loadFromDataPackRepository(m_dataPackList);
        if (featureResult.failed()) {
            spdlog::error("Failed to load configured features from data packs: {}", featureResult.error().toString());
        } else {
            spdlog::info("Loaded {} configured features from data packs", featureResult.value());
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::PlacedFeatures");
        auto placedResult = world::gen::placement::PlacedFeatureLoader::loadFromDataPackRepository(m_dataPackList);
        if (placedResult.failed()) {
            spdlog::error("Failed to load placed features from data packs: {}", placedResult.error().toString());
        } else {
            spdlog::info("Loaded {} placed features from data packs", placedResult.value());
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::ConfiguredCarvers");
        auto carverResult = world::gen::carver::ConfiguredCarverLoader::loadFromDataPackRepository(m_dataPackList);
        if (carverResult.failed()) {
            spdlog::error("Failed to load configured carvers from data packs: {}", carverResult.error().toString());
        } else {
            spdlog::info("Loaded {} configured carvers from data packs", carverResult.value());
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Biomes");
        // 确保 BiomeFactory 构造的默认 Biome 已注册（BiomeLoader 在其上叠加 JSON 字段）
        BiomeRegistry::instance().initialize();
        auto biomeResult = world::biome::BiomeLoader::loadFromDataPackRepository(m_dataPackList);
        if (biomeResult.failed()) {
            spdlog::error("Failed to load biomes from data packs: {}", biomeResult.error().toString());
        } else {
            spdlog::info("Loaded {} biomes from data packs", biomeResult.value());
        }
    }

    // 初始化结构标签（必须在结构集合注册后，海豚寻宝等玩法依赖此标签）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::StructureTags");
        world::gen::structure::StructureTags::initialize();
    }
    spdlog::info("Structure tags initialized");

    // 从数据包加载结构标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::StructureTagLoader");
        auto dataPackLoadResult = world::gen::structure::StructureTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load structure tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} structure tags from data packs", dataPackLoadResult.value());
        }
    }

    // 注册实体类型（可选）
    if (registerEntities) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Entities");
        entity::VanillaEntities::registerAll();
    }

    // 初始化实体类型标签（必须在所有实体类型注册后）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::EntityTypeTags");
        EntityTypeTags::initialize();
    }
    spdlog::info("Entity type tags initialized");

    // 从数据包加载实体类型标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::EntityTypeTagLoader");
        auto dataPackLoadResult = EntityTypeTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load entity type tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} entity type tags from data packs", dataPackLoadResult.value());
        }
    }

    // 初始化伤害类型标签（用于狼铠吸收判定、伤害分类等）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::DamageTypeTags");
        DamageTypeTags::initialize();
    }
    spdlog::info("Damage type tags initialized");

    // 从数据包加载伤害类型标签（追加到或替换内置默认值）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::DamageTypeTagLoader");
        auto dataPackLoadResult = DamageTypeTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load damage type tags from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} damage type tags from data packs", dataPackLoadResult.value());
        }
    }

    // 初始化预定义日程（村民AI行为日程）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Schedules");
        entity::ai::brain::schedule::Schedule::initialize();
    }
    spdlog::info("Schedules initialized");

    // 初始化记忆模块类型
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::MemoryModules");
        entity::ai::brain::memory::MemoryModuleTypes::initialize();
    }
    spdlog::info("Memory module types initialized");

    // 初始化村民交易配方表
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::VillagerTrades");
        world::village::trade::VillagerTrades::initialize();
    }
}

void MinecraftServer::setupWorldCallbacks()
{
    // 为所有维度设置回调
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (!world || !world->chunkManager()) {
            return;
        }

        // 设置区块加载回调 - 当区块加载/生成完成时触发
        // 注：onChunkLoaded（加载区块内实体）由调用方负责，不在此回调内调用：
        //   - 存档加载路径：ServerChunkManager::_resolveChunkSourceSync 在 m_chunkLoadedCallback 之前
        //     已直接调用 m_world->onChunkLoaded。
        //   - 生成路径：ServerChunkManager::_drainPendingPostProcess 在 m_chunkLoadedCallback 之前
        //     已直接调用 m_world->onChunkLoaded。
        //   此回调仅做光照初始化与区块发送，重复调用 onChunkLoaded 会导致实体重复生成。
        world->chunkManager()->setChunkLoadedCallback([this, serverDim, world](ChunkCoord x, ChunkCoord z) {
            // 初始化区块光照
            if (auto* ls = serverDim->lightSyncManager()) {
                ls->initializeChunkLighting(x, z);
            }
            // 发送区块给追踪玩家
            if (auto* cs = serverDim->chunkSendManager()) {
                cs->sendChunkToTrackingPlayers(x, z);
            }
        });

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

                const EntityId spawnedId = world->spawnEntity(std::move(entity));
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

            // 通知光照同步管理器
            if (auto* ls = serverDim->lightSyncManager()) {
                ls->markLightChanged(type, pos);
            }

            // 广播光照更新给客户端
            auto* lightManager = serverDim->lightManager();
            if (!lightManager) {
                return;
            }

            std::vector<u8> skyLight;
            std::vector<u8> blockLight;

            // 获取光照数据
            if (type == LightType::SKY && lightManager->getSkyLightEngine()) {
                auto* data = lightManager->getData(LightType::SKY, pos);
                if (data) {
                    skyLight = data->toByteArray();
                }
            } else if (type == LightType::BLOCK && lightManager->getBlockLightEngine()) {
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
                network::BlockUpdatePacket packet(x, y, z, blockStateId);
                network::PacketSerializer ser;
                packet.serialize(ser);

                auto fullPacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::BlockUpdate, ser.buffer());
                sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
            });
        }

        // 设置实体同步回调
        if (auto* es = serverDim->entitySyncManager()) {
            es->setOnEntitySpawn([this, serverDim](EntityId entityId, const Entity& entity) {
                MC_UNUSED(entityId);
                MC_UNUSED(entity);
                // 实体生成广播由 EntityTracker 处理
            });

            es->setOnEntityRemove([this, serverDim](EntityId entityId) {
                MC_UNUSED(entityId);
                // 实体移除广播由 EntityTracker 处理
            });

            es->setOnEntityMove([this, serverDim](EntityId entityId, const Vector3& pos, f32 yaw, f32 pitch) {
                MC_UNUSED(entityId);
                MC_UNUSED(pos);
                MC_UNUSED(yaw);
                MC_UNUSED(pitch);
                // 实体移动广播由 EntityTracker 处理
            });

            es->setOnEntityStatus([this, serverDim](EntityId entityId, u8 status) {
                MC_UNUSED(entityId);
                MC_UNUSED(status);
                // 实体状态广播由 EntityTracker 处理
            });
        }

        // 设置区块发送回调
        if (auto* cs = serverDim->chunkSendManager()) {
            cs->setOnChunkSend([this](PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
                sendChunkDataToPlayer(playerId, x, z, data);
            });

            cs->setOnChunkUnload([this](PlayerId playerId, ChunkCoord x, ChunkCoord z) {
                network::UnloadChunkPacket packet(x, z, 0); // dimension will be set per-context
                network::PacketSerializer ser;
                packet.serialize(ser);

                auto fullPacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::UnloadChunk, ser.buffer());
                sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
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

void MinecraftServer::setupChunkSendCallback()
{
    // ChunkSendManager 已移入 ServerDimension，回调由 attachWorldBindings 设置
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
            m_packetHandler.reset();
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
    u64 tick = currentTick();
    if (tick - m_lastKeepAliveTick >= KEEPALIVE_INTERVAL) {
        m_lastKeepAliveTick = tick;
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "SendKeepAlive", "phase", "keepalive_send");
        sendKeepAliveToAll();
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

        network::TimeUpdatePacket packet(gameTime, tod, daylightCycleEnabled);
        network::PacketSerializer ser;
        packet.serialize(ser);

        auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::TimeUpdate, ser.buffer());

        // 发送给该维度的所有玩家
        for (PlayerId playerId : playerIds) {
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }
}

void MinecraftServer::sendWeatherUpdate()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "sendWeatherUpdate", "phase", "weather_sync");

    // 天气仅存在于主世界
    auto* overworld = m_dimensionManager->getOverworld();
    if (!overworld || !overworld->world() || !overworld->world()->weatherManager()) return;

    auto& weatherMgr = *overworld->world()->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    constexpr f32 STRENGTH_THRESHOLD = 0.001f;
    bool rainChanged = std::abs(rainStrength - m_lastSentRainStrength) > STRENGTH_THRESHOLD;
    bool thunderChanged = std::abs(thunderStrength - m_lastSentThunderStrength) > STRENGTH_THRESHOLD;

    if (rainChanged) {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            broadcastPacket(fullPacket.data(), fullPacket.size());
        }
        m_lastSentRainStrength = rainStrength;
    }

    if (thunderChanged) {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            broadcastPacket(fullPacket.data(), fullPacket.size());
        }
        m_lastSentThunderStrength = thunderStrength;
    }

    if (weatherMgr.hasWeatherChanged()) {
        auto weatherType = weatherMgr.weatherType();
        if (weatherType == weather::WeatherType::Clear) {
            auto packet = network::GameStateChangePacket::endRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
                broadcastPacket(fullPacket.data(), fullPacket.size());
            }
        } else if (weatherType == weather::WeatherType::Rain || weatherType == weather::WeatherType::Thunder) {
            auto packet = network::GameStateChangePacket::beginRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
                broadcastPacket(fullPacket.data(), fullPacket.size());
            }
        }
    }
}

void MinecraftServer::sendInitialWeatherStateToPlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendInitialWeatherState", "phase", "weather_sync");

    // 天气仅存在于主世界
    auto* overworld = m_dimensionManager->getOverworld();
    if (!overworld || !overworld->world() || !overworld->world()->weatherManager()) return;

    auto& weatherMgr = *overworld->world()->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }

    {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }
}

void MinecraftServer::sendInitialDifficultyToPlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendInitialDifficulty", "phase", "difficulty_sync");

    auto fullPacket = serializeDifficultyPacket();
    if (fullPacket.empty()) {
        spdlog::error("Failed to serialize ServerDifficultyPacket for player {}", playerId);
        return;
    }

    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendKeepAliveToAll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendKeepAlive", "phase", "keepalive_sync");

    u64 timestamp = util::TimeUtils::getCurrentTimeMs();
    u64 tick = currentTick();

    m_playerManager->forEachPlayer([this, timestamp, tick](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            network::KeepAlivePacket packet;
            packet.setTimestamp(timestamp);

            auto result = packet.serialize();
            if (result.success()) {
                sendPacketToPlayer(player.playerId, result.value().data(), result.value().size());
            }

            m_keepAliveManager->recordKeepAliveSent(player.playerId, timestamp, tick);
        }
    });
}

void MinecraftServer::initializeCreativeInventory(PlayerInventory& inventory)
{
    inventory.clear();
    inventory.setSelectedSlot(0);
    fillCreativeModeInventory(inventory);
}

void MinecraftServer::sendChunkDataToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data)
{
    DimensionId dimension = 0;
    if (m_dimensionManager) {
        const DimensionId resolvedDimension = m_dimensionManager->getPlayerDimension(playerId);
        if (resolvedDimension >= 0) {
            dimension = resolvedDimension;
        }
    }

    network::ChunkDataPacket packet(x, z, dimension, data);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::ChunkData, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    DimensionId dimension = 0;
    if (m_dimensionManager) {
        const DimensionId resolvedDimension = m_dimensionManager->getPlayerDimension(playerId);
        if (resolvedDimension >= 0) {
            dimension = resolvedDimension;
        }
    }

    network::UnloadChunkPacket packet(x, z, dimension);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::UnloadChunk, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastLightUpdate(ChunkCoord x,
    ChunkCoord z,
    i32 sectionY,
    const std::vector<u8>& skyLight,
    const std::vector<u8>& blockLight,
    bool trustEdges)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "BroadcastLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", x, sectionY, z),
        "SkyLightSize",
        skyLight.size(),
        "BlockLightSize",
        blockLight.size(),
        [flow = ::perfetto::Flow::ProcessScoped(SectionPos(x, sectionY, z).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    network::LightUpdatePacket packet(
        x, z, sectionY, std::vector<u8>(skyLight), std::vector<u8>(blockLight), trustEdges);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::LightUpdate, ser.buffer());
    m_playerManager->forEachPlayer([&fullPacket](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            player.send(fullPacket.data(), fullPacket.size());
        }
    });
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
// 数据包处理方法
// ============================================================================

void MinecraftServer::handlePlayerMovePacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse player move from player {}", playerId);
        return;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

    // 保存旧位置用于村庄进入检测
    BlockPos prevPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));

    // 计算新区块坐标
    ChunkCoord newChunkX = math::floorTo<ChunkCoord>(pos.x / static_cast<f64>(world::CHUNK_WIDTH));
    ChunkCoord newChunkZ = math::floorTo<ChunkCoord>(pos.z / static_cast<f64>(world::CHUNK_WIDTH));

    // 检查玩家是否移动到了新区块
    ChunkCoord oldChunkX = math::floorTo<ChunkCoord>(player->x / static_cast<f32>(world::CHUNK_WIDTH));
    ChunkCoord oldChunkZ = math::floorTo<ChunkCoord>(player->z / static_cast<f32>(world::CHUNK_WIDTH));
    bool chunkChanged = (newChunkX != oldChunkX || newChunkZ != oldChunkZ);

    switch (packet.type()) {
        case network::PlayerMovePacket::MoveType::Full:
            player->x = static_cast<f32>(pos.x);
            player->y = static_cast<f32>(pos.y);
            player->z = static_cast<f32>(pos.z);
            player->yaw = pos.yaw;
            player->pitch = pos.pitch;
            break;
        case network::PlayerMovePacket::MoveType::Position:
            player->x = static_cast<f32>(pos.x);
            player->y = static_cast<f32>(pos.y);
            player->z = static_cast<f32>(pos.z);
            break;
        case network::PlayerMovePacket::MoveType::Rotation:
            player->yaw = pos.yaw;
            player->pitch = pos.pitch;
            break;
        case network::PlayerMovePacket::MoveType::GroundOnly:
            player->onGround = pos.onGround;
            break;
    }

    auto* world = getPlayerWorld(playerId);
    if (world) {
        world->entityManager().forEachEntity([playerId, player](Entity* entity) {
            auto* playerEntity = dynamic_cast<Player*>(entity);
            if (playerEntity == nullptr || playerEntity->playerId() != playerId) {
                return true;
            }

            playerEntity->setPosition(player->x, player->y, player->z);
            playerEntity->setRotation(player->yaw, player->pitch);
            playerEntity->setOnGround(player->onGround);
            return false;
        });
    }

    m_positionTracker->updatePosition(
        playerId, player->x, player->y, player->z, player->yaw, player->pitch, player->onGround);
    updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

    // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
    // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
    if (world && world->chunkManager()) {
        world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
        world->chunkManager()->processTicketUpdatesSync();
    }

    // 村庄进入检测（用于触发袭击）
    // 仅在位置实际改变时检测，村庄/袭击仅存在于主世界
    auto* overworld = m_dimensionManager->getOverworld();
    if (overworld && overworld->world() && packet.type() != network::PlayerMovePacket::MoveType::Rotation &&
        packet.type() != network::PlayerMovePacket::MoveType::GroundOnly) {
        auto* villageManager = overworld->world()->villageManager();
        auto* raidManager = overworld->world()->raidManager();
        if (villageManager && raidManager) {
            BlockPos currentPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));
            world::village::Village* enteredVillage = villageManager->checkPlayerEnterVillage(currentPos, prevPos);
            if (enteredVillage) {
                // 玩家进入了村庄，使用回调检查不祥之兆并触发袭击
                raidManager->onPlayerEnterVillageWithCallback(
                    [player](BlockPos) -> i32 {
                        if (player->hasEffect(entity::effect::EffectType::BadOmen)) {
                            const entity::effect::EffectInstance* effect =
                                player->getEffect(entity::effect::EffectType::BadOmen);
                            if (effect != nullptr) {
                                i32 level = effect->getEffectLevel();
                                // 移除不祥之兆效果
                                player->removeEffect(entity::effect::EffectType::BadOmen);
                                return level;
                            }
                        }
                        return 0;
                    },
                    enteredVillage);
            }
        }
    }
}

void MinecraftServer::handleTeleportConfirmPacket(PlayerId playerId, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse teleport confirm from player {}", playerId);
        return;
    }

    auto& packet = result.value();

    if (m_teleportManager->confirmTeleport(playerId, packet.teleportId())) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (!player) {
            return;
        }

        updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

        // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
        // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
        auto* world = getPlayerWorld(playerId);
        if (world && world->chunkManager()) {
            world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
            world->chunkManager()->processTicketUpdatesSync();
        }
    }
}

void MinecraftServer::handleKeepAlivePacket(PlayerId playerId, const u8* data, size_t size)
{
    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
        m_keepAliveManager->handleKeepAliveResponse(playerId, packet.timestamp(), currentTimeMs);
    } else {
        spdlog::error("Failed to parse keep alive packet from player {}: {}", playerId, result.error().message());
    }
}

void MinecraftServer::handleChatMessagePacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse chat message packet from player {}: {}", playerId, result.error().message());
        return;
    }

    auto& packet = result.value();
    std::string message = packet.message();

    if (!message.empty() && message[0] == '/') {
        // 执行命令
        DimensionId commandDimension = 0;
        if (auto* playerWorld = getPlayerWorld(playerId)) {
            commandDimension = playerWorld->dimension();
        }
        // 从玩家管理器查找 Player 实体作为命令执行实体。
        mc::Entity* commandEntity = nullptr;
        if (auto* cmdDim = dimensionManager().getDimension(commandDimension)) {
            if (auto* cmdWorld = cmdDim->world()) {
                commandEntity = playerEntityManager().getPlayerEntity(playerId, *cmdWorld);
            }
        }

        mc::command::ServerCommandSource source(this,
            nullptr,
            commandDimension,
            Vector3d(player->x, player->y, player->z),
            Vector2f(player->yaw, player->pitch),
            static_cast<i32>(resolveOpLevel(player->uuid)),
            playerId,
            player->username,
            commandEntity);
        auto cmdResult = m_commandRegistry->execute(message, source);
        if (cmdResult.failed()) {
            spdlog::warn("Command '{}' failed for {}: {}", message, player->username, cmdResult.error().toString());
        } else {
            spdlog::info("Command '{}' executed for {} with result {}", message, player->username, cmdResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);
}

void MinecraftServer::updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "MinecraftServer::updateEntityTrackingForPlayer",
        "playerId",
        playerId,
        "x",
        x,
        "y",
        y,
        "z",
        z);

    auto* world = getPlayerWorld(playerId);
    if (!world) {
        return;
    }

    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    world->entityTracker().updatePlayerTracking(
        *this, *world, playerId, Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)));
}

void MinecraftServer::handleBlockInteractionPacket(PlayerId playerId, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::BlockInteractionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse block interaction packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    BlockPos pos(packet.x(), packet.y(), packet.z());

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "MinecraftServer::handleBlockInteractionPacket",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 处理挖掘状态
    miningManager().handleBlockInteraction(playerId, pos, packet.action());

    if (packet.action() == network::BlockInteractionAction::StopDestroyBlock) {
        if (!miningManager().tryCompleteMining(playerId, pos)) {
            spdlog::warn("Ignored premature StopDestroyBlock from player {} at {}", playerId, pos.toString());
        }
    }
}

void MinecraftServer::handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    const BlockPos pos(packet.x(), packet.y(), packet.z());
    auto* playerWorld = getPlayerWorld(playerId);
    const BlockState* clickedState = playerWorld ? playerWorld->getBlockState(pos) : nullptr;
    const Hand hand = (packet.hand() == static_cast<u8>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    const auto tryOpenCrafting = [this, playerId, pos, clickedState]() {
        return isCraftingTableState(clickedState) && tryOpenCraftingContainer(playerId, pos);
    };

    ItemStack heldStack = getHeldItemForPlacement(playerId);
    if (heldStack.isEmpty()) {
        if (!tryOpenCrafting()) {
            (void)blockInteractionManager().handleBlockUse(playerId, pos, hand, packet.hitPosition(), packet.face());
        }
        return;
    }

    const Item* heldItem = heldStack.getItem();
    const bool holdingBlockItem =
        heldItem != nullptr && BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId()) != nullptr;

    if (!holdingBlockItem) {
        if (!tryOpenCrafting()) {
            (void)blockInteractionManager().handleBlockUse(playerId, pos, hand, packet.hitPosition(), packet.face());
        }
        return;
    }

    auto interactionResult =
        blockInteractionManager().handleBlockPlacement(playerId, pos, packet.hitPosition(), packet.face(), heldStack);

    if (interactionResult.success() && interactionResult.value().blockPlaced &&
        interactionResult.value().itemConsumed) {
        const i32 selectedSlot = getSelectedHotbarSlot(playerId);
        ItemStack updatedStack = heldStack;
        updatedStack.shrink(1);
        setInventoryItem(playerId, selectedSlot, updatedStack);
        syncPlayerInventory(playerId);
    }
}

void MinecraftServer::handleCreativeInventoryActionPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn || player->gameMode != GameMode::Creative) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = CreativeInventoryActionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse creative inventory action packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    const i32 slotIndex = packet.slotIndex();
    if (slotIndex < 0 || slotIndex >= PlayerInventory::TOTAL_SIZE) {
        spdlog::warn("Ignoring creative inventory action with invalid slot {}", slotIndex);
        return;
    }

    setInventoryItem(playerId, slotIndex, packet.item());
    syncPlayerInventory(playerId);
}

// ============================================================================
// 数据包发送辅助方法
// ============================================================================

void MinecraftServer::sendTeleportPacket(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Teleport, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory)
{
    MC_UNUSED(playerId);
    MC_UNUSED(inventory);
}

void MinecraftServer::setupInitialPlayerState(ServerPlayerData* player, GameMode gameMode)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Player, "MinecraftServer::setupInitialPlayerState", "gameMode", static_cast<i32>(gameMode));

    if (!player) return;

    // 获取世界出生点（从主世界维度获取）
    Vector3d spawnPoint(0.0, 64.0, 0.0); // 默认值
    auto* overworld = m_dimensionManager->getOverworld();
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

void MinecraftServer::sendInitialGameState(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player,
        "MinecraftServer::sendInitialGameState",
        "playerId",
        playerId,
        "x",
        x,
        "y",
        y,
        "z",
        z);

    // TeleportManager::requestTeleport() 内部已经发送过 TeleportPacket。
    // 这里不能重复发送，否则客户端会在登录阶段收到两个相同 teleportId 的传送包，
    // 紧接着回两次 TeleportConfirm，第二次会因为服务端已清除 waitingTeleportConfirm
    // 而被当作无效确认，进而打断首次区块加载时序。
    m_teleportManager->requestTeleport(playerId, x, y, z, yaw, pitch);

    // 立即发送时间，避免客户端在首次周期同步前短暂显示默认时间(0)
    // 使用玩家当前维度的时间（下界=18000，末地=6000，主世界=实际时间）
    const auto& time = timeManager().gameTimeObj();
    i64 dayTime = time.dayTimeOfDay(); // 默认使用主世界时间

    // 获取玩家当前维度并使用维度特定时间
    auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(playerId);
    if (playerDim && playerDim->world()) {
        dayTime = playerDim->world()->dayTime();
    }

    network::TimeUpdatePacket timePacket(time.gameTime(), dayTime, time.daylightCycleEnabled());
    network::PacketSerializer timeSer;
    timePacket.serialize(timeSer);
    auto fullTimePacket = core::ConnectionManager::encapsulatePacket(network::PacketType::TimeUpdate, timeSer.buffer());
    sendPacketToPlayer(playerId, fullTimePacket.data(), fullTimePacket.size());

    // 发送世界出生点（指南针指向位置，从主世界维度获取）
    Vector3d worldSpawn(0.0, 64.0, 0.0);
    f32 spawnAngle = 0.0f;
    auto* overworld = m_dimensionManager->getOverworld();
    if (overworld && overworld->world()) {
        worldSpawn = overworld->world()->worldSpawnPoint();
        spawnAngle = overworld->world()->spawnAngle();
    }
    network::SpawnPositionPacket spawnPosPacket(BlockPos(static_cast<BlockCoord>(worldSpawn.x),
                                                    static_cast<BlockCoord>(worldSpawn.y),
                                                    static_cast<BlockCoord>(worldSpawn.z)),
        spawnAngle);
    auto spawnPosResult = spawnPosPacket.serialize();
    if (spawnPosResult.success()) {
        auto fullSpawnPacket =
            core::ConnectionManager::encapsulatePacket(network::PacketType::SpawnPosition, spawnPosResult.value());
        sendPacketToPlayer(playerId, fullSpawnPacket.data(), fullSpawnPacket.size());
    }

    // 发送初始天气状态
    sendInitialWeatherStateToPlayer(playerId);

    // 发送初始难度状态
    sendInitialDifficultyToPlayer(playerId);

    updateEntityTrackingForPlayer(playerId, x, y, z);
}

void MinecraftServer::sendCommandTreePacket(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "MinecraftServer::sendCommandTreePacket", "playerId", playerId);

    MC_ASSERT_RELEASE(m_commandRegistry != nullptr);

    network::CommandTreePacket packet(m_commandRegistry->getCommandTreeJson());
    auto serializeResult = packet.serialize();
    MC_ASSERT_RELEASE(serializeResult.success());

    auto fullPacket =
        core::ConnectionManager::encapsulatePacket(network::PacketType::CommandTree, serializeResult.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendPermissionLevelChange(PlayerId playerId, i32 permissionLevel)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
        "MinecraftServer::sendPermissionLevelChange",
        "playerId",
        playerId,
        "permissionLevel",
        permissionLevel);

    // 获取玩家实体 ID 用于 EntityStatusPacket
    auto* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* player = playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        return;
    }

    // 通过 EntityStatusPacket 通知客户端权限等级变更（status byte = 24 + level）
    network::EntityStatusPacket packet;
    packet.setEntityId(static_cast<u32>(player->id()));
    packet.setStatus(network::EntityStatusPacket::permissionLevel(permissionLevel));

    auto payloadResult = packet.serialize();
    if (payloadResult.success()) {
        auto fullPacket =
            core::ConnectionManager::encapsulatePacket(network::PacketType::EntityStatus, payloadResult.value());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    }

    // 同步更新后的命令树到客户端，以便刷新可用命令列表
    sendCommandTreePacket(playerId);
}

i32 MinecraftServer::resolveOpLevel(const std::string& uuid) const noexcept
{
    return static_cast<i32>(m_opListManager->getLevel(uuid));
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

void MinecraftServer::dispatchPacket(u32 sessionId, const u8* data, size_t size)
{
    if (size < network::PACKET_HEADER_SIZE) {
        spdlog::warn("Packet too small: {} bytes", size);
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    if (sizeResult.failed() || typeResult.failed()) {
        spdlog::warn("Failed to read packet header");
        return;
    }

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());
    const u8* payload = data + network::PACKET_HEADER_SIZE;
    size_t payloadSize = size - network::PACKET_HEADER_SIZE;

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
        "DispatchPacketToHandler",
        "sessionId",
        sessionId,
        "packetType",
        static_cast<int>(packetType),
        "payloadSize",
        payloadSize);

    switch (packetType) {
        case network::PacketType::LoginRequest: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleLoginRequestPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            handleLoginRequestPacket(sessionId, payload, payloadSize);
            break;
        }

        case network::PacketType::PlayerMove: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandlePlayerMovePacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handlePlayerMovePacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::BlockInteraction: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleBlockInteractionPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleBlockInteractionPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::PlayerTryUseItemOnBlock: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandlePlayerTryUseItemOnBlockPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleBlockPlacementPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::HotbarSelect: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleHotbarSelectPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleHotbarSelectPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::CreativeInventoryAction: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleCreativeInventoryActionPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleCreativeInventoryActionPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::ContainerClick: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleContainerClickPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleContainerClickPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::CloseContainer: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleCloseContainerPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleCloseContainerPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::TeleportConfirm: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleTeleportConfirmPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleTeleportConfirmPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::KeepAlive: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleKeepAlivePacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleKeepAlivePacket(playerId, data, size);
            }
            break;
        }

        case network::PacketType::ChatMessage: {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
                "HandleChatMessagePacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleChatMessagePacket(playerId, payload, payloadSize);
            }
            break;
        }

        default:
            spdlog::warn("Unhandled packet type: {}", static_cast<int>(packetType));
            break;
    }
}

// ============================================================================
// 声音广播方法
// ============================================================================

void MinecraftServer::broadcastSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::warn("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlaySound, result.value());
    broadcastPacket(fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastSoundInRange(const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 range,
    f32 volume,
    f32 pitch)
{
    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlaySound, result.value());

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_playerManager->forEachPlayer([this, &position, range, &fullPacket, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - position.x;
        f32 dy = player.y - position.y;
        f32 dz = player.z - position.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
            playersNotified++;
        }
    });
}

void MinecraftServer::sendSoundToPlayer(PlayerId playerId,
    const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 volume,
    f32 pitch)
{
    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlaySound, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
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
    network::ParticlePacket packet(type, pos, velocity, offset, count);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
            playersNotified++;
        }
    });
}

void MinecraftServer::sendParticleToPlayer(PlayerId playerId,
    particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    network::ParticlePacket packet(type, pos, velocity, offset, count);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastVibrationParticleInRange(
    const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks, f32 range)
{
    // 根据 PositionSource 类型选择对应的 createVibration 重载，
    // 以确保网络序列化格式与 MC Java 1.21.11 VibrationParticleOption.STREAM_CODEC 一致：
    // - BlockPositionSource -> VarInt(0) + i64 packedBlockPos
    // - EntityPositionSource -> VarInt(1) + VarInt entityId + f32 yOffset
    network::ParticlePacket packet;
    const std::string sourceType = targetSource.type();
    if (sourceType == "entity") {
        const auto& entitySource = static_cast<const gameevent::EntityPositionSource&>(targetSource);
        packet = network::ParticlePacket::createVibration(
            pos, entitySource.entityId(), entitySource.yOffset(), arrivalInTicks);
    } else {
        // 默认按方块位置源处理（"block" 或任何未知类型）
        const auto& blockSource = static_cast<const gameevent::BlockPositionSource&>(targetSource);
        packet = network::ParticlePacket::createVibration(pos, blockSource.pos(), arrivalInTicks);
    }

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize VibrationParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastTrailParticleInRange(
    const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks, f32 range)
{
    network::ParticlePacket packet = network::ParticlePacket::createTrail(pos, targetPosition, color, durationInTicks);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize TrailParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastEntityEffectParticleInRange(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color, f32 range)
{
    network::ParticlePacket packet = network::ParticlePacket::createEntityEffect(pos, velocity, offset, count, color);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize EntityEffectParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastBlockParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, u32 blockStateId, f32 range)
{
    network::ParticlePacket packet =
        network::ParticlePacket::createBlock(type, pos, velocity, Vector3(0.0f, 0.0f, 0.0f), 1, blockStateId);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize BlockParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastItemParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack, f32 range)
{
    network::ParticlePacket packet =
        network::ParticlePacket::createItem(type, pos, velocity, Vector3(0.0f, 0.0f, 0.0f), 1, itemStack);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ItemParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastParticleInRange(u32 type,
    f64 x,
    f64 y,
    f64 z,
    f32 velocityX,
    f32 velocityY,
    f32 velocityZ,
    f32 offsetX,
    f32 offsetY,
    f32 offsetZ,
    u32 count,
    f32 range)
{
    // 转换为强类型枚举并调用现有的实现
    auto particleType = static_cast<particle::ParticleTypeId>(type);
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    Vector3 velocity(velocityX, velocityY, velocityZ);
    Vector3 offset(offsetX, offsetY, offsetZ);

    broadcastParticleInRange(particleType, pos, velocity, offset, count, range);
}

void MinecraftServer::broadcastEntityStatusInRange(EntityId entityId, u8 status, const Vector3& pos, f32 range)
{
    network::EntityStatusPacket packet;
    packet.setEntityId(static_cast<u32>(entityId));
    packet.setStatus(static_cast<network::EntityStatusPacket::Status>(status));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize EntityStatusPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::EntityStatus, result.value());

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastEntityAnimationInRange(EntityId entityId, u8 animation, const Vector3& pos, f32 range)
{
    network::EntityAnimationPacket packet;
    packet.setEntityId(static_cast<u32>(entityId));
    packet.setAnimation(static_cast<network::EntityAnimationPacket::Animation>(animation));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize EntityAnimationPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::EntityAnimation, result.value());

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastSetEntityLinkInRange(
    EntityId entityId, EntityId linkedEntityId, const Vector3& pos, f32 range)
{
    network::SetEntityLinkPacket packet(static_cast<u32>(entityId), static_cast<u32>(linkedEntityId));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize SetEntityLinkPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::SetEntityLink, result.value());

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

// ============================================================================
// 世界事件广播
// ============================================================================

void MinecraftServer::broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    sound::WorldEventPacket packet(eventId, x, y, z, data);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize WorldEventPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::WorldEvent, result.value());
    broadcastPacket(fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range)
{
    sound::WorldEventPacket packet(eventId, x, y, z, data);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize WorldEventPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::WorldEvent, result.value());

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastBlockEventInRange(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId, f32 range)
{
    // 参考 MC Java: ServerPlayerList.broadcast(null, x, y, z, 64.0, dimension, new ClientboundBlockEventPacket)
    network::BlockEventPacket packet =
        network::BlockEventPacket::create(BlockPos(x, y, z), paramA, paramB, blockStateId);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize BlockEventPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::BlockEvent, result.value());

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

void MinecraftServer::broadcastBlockBreakProgressInRange(
    EntityId breakerId, i32 x, i32 y, i32 z, i32 progress, f32 range)
{
    // 对应 MC Java: ServerLevel.destroyBlockProgress()
    // 发送 BlockBreakAnimPacket 给范围内的玩家
    // MC Java 原版行为：排除破坏者自身（serverplayer.getId() != breakerId），
    // 只向同维度、32格范围内的其他玩家发送 ClientboundBlockDestructionPacket。
    // 破坏者自身的动画由客户端本地直接驱动，不需要服务端发包。

    network::BlockBreakAnimPacket packet;
    packet.setBreakerEntityId(breakerId);
    packet.setPosition(BlockPos(x, y, z));
    packet.setStage(static_cast<i8>(progress));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize BlockBreakAnimPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::BlockBreakAnim, result.value());

    // 将 breakerId (EntityId) 转换为 PlayerId，用于排除破坏者自身
    PlayerId breakerPlayerId = playerEntityManager().getPlayerIdByEntityId(breakerId);

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, breakerPlayerId, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        // 排除破坏者自身：MC Java 原版中 serverplayer.getId() != breakerId
        if (breakerPlayerId != 0 && player.playerId == breakerPlayerId) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

// ============================================================================
// 爆炸广播
// ============================================================================

void MinecraftServer::broadcastExplosionInRange(const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback,
    f32 range)
{
    // 发送给爆炸点 64 格范围内的玩家
    // 每个玩家收到的击退向量不同，需要为每个玩家单独构建数据包

    f32 rangeSq = range * range;

    m_playerManager->forEachPlayer(
        [this, &position, strength, &affectedBlocks, &playerKnockback, rangeSq](ServerPlayerData& player) {
            if (!player.loggedIn || !player.hasConnection()) {
                return;
            }

            // 检查玩家是否在范围内
            f32 dx = player.x - position.x;
            f32 dy = player.y - position.y;
            f32 dz = player.z - position.z;
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq <= rangeSq) {
                // 为每个玩家创建单独的爆炸包（击退向量不同）
                sendExplosionToPlayer(player.playerId, position, strength, affectedBlocks, playerKnockback);
            }
        });
}

void MinecraftServer::sendExplosionToPlayer(PlayerId playerId,
    const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback)
{
    // 创建爆炸包，包含该玩家的击退向量
    network::ExplosionPacket packet(position, strength, affectedBlocks, playerKnockback, static_cast<u64>(playerId));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ExplosionPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Explosion, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

} // namespace mc::server
