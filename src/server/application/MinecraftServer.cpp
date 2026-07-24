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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/tag/DamageTypeTagLoader.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
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
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTagLoader.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/load/ChunkLoadTicket.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "common/world/gen/density/DensityFunctionLoader.hpp"
#include "common/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "common/world/gen/feature/FeatureTypeRegistry.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "common/world/gen/noise/NoiseLoader.hpp"
#include "common/world/gen/placement/PlacedFeatureLoader.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorPresetLoader.hpp"
#include "common/world/gen/settings/NoiseSettingsLoader.hpp"
#include "common/world/gen/settings/WorldPresetLoader.hpp"
#include "common/world/gen/structure/StructureDefinitionLoader.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "common/world/gen/structure/StructureSetLoader.hpp"
#include "common/world/gen/structure/StructureTagLoader.hpp"
#include "common/world/gen/structure/StructureTags.hpp"
#include "common/world/gen/structure/pools/Pools.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
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
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include <chrono>
#include <cmath>
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

/// 向字节流追加 VarUInt 编码（与 MC Java VarInt 长度前缀一致）。
void appendVarUInt(std::vector<u8>& out, u32 value)
{
    while ((value & ~0x7Fu) != 0) {
        out.push_back(static_cast<u8>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    out.push_back(static_cast<u8>(value & 0x7F));
}

/// 构造 LevelParticles IR（1.21.11，对齐 ClientboundLevelParticlesPacket）。
///
/// 外层字段取自广播参数：位置/偏移/count；maxSpeed 沿用旧实现固定 0（客户端按
/// 偏移扇出，不消费该字段）。ParticleOptions 由调用方按粒子类型预先填充。
[[nodiscard]] mc::network::ir::IrPacket buildLevelParticlesIr(
    const Vector3& pos, const Vector3& offset, u32 count, mc::network::ir::play::ParticleOptions options)
{
    mc::network::ir::play::LevelParticles pkt;
    pkt.overrideLimiter = false;
    pkt.alwaysShow = false;
    pkt.x = pos.x;
    pkt.y = pos.y;
    pkt.z = pos.z;
    pkt.xDist = offset.x;
    pkt.yDist = offset.y;
    pkt.zDist = offset.z;
    pkt.maxSpeed = 0.0f;
    pkt.count = static_cast<i32>(count);
    pkt.particle = std::move(options);
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

} // namespace

MinecraftServer::MinecraftServer(ServerSettings& settings)
    : m_settings(settings)
    , m_computationWorkerPool(-1, "ServerCompute", 100)
    , m_ioWorkerPool(-1, "ServerIO", 200)
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
    broadcastPacket(serializeDifficultyPacket());
    spdlog::info("Difficulty changed to {}", static_cast<i32>(m_difficulty));
}

mc::network::ir::IrPacket MinecraftServer::serializeDifficultyPacket()
{
    mc::network::ir::play::ChangeDifficulty pkt;
    pkt.difficulty = static_cast<i32>(m_difficulty);
    pkt.locked = false;

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
                Entity* entity = dimension->world()->getEntity(static_cast<EntityInstanceId>(playerId));
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
                Entity* entity = dimension->world()->getEntity(static_cast<EntityInstanceId>(playerId));
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
    world.setOnBroadcastBlockEvent([this](i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId) {
        broadcastBlockEventInRange(x, y, z, paramA, paramB, blockStateId);
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
            m_spawnInitializedThisSession);

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
        // 1.21.11 用 ContainerSetContent(containerId=0) 同步完整玩家物品栏
        // TODO(Phase6): 玩家物品栏的 stateId/动态槽位语义需对齐 1.21.11 PlayerInventoryContents 同步。
        mc::network::ir::play::ContainerSetContent pkt;
        pkt.containerId = 0; // 玩家物品栏
        pkt.stateId = 0;
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

    // 从数据包加载噪声参数（worldgen/noise/*.json）
    // 最底层依赖：density_function / noise_settings 的噪声叶子节点引用噪声名，
    // 故必须先于一切 worldgen Loader 加载。Noises::initialize() 硬编码兜底由
    // NoiseLoader clear() 清空后注入，markLoadedFromDatapack(true) 使 get()/has()
    // 跳过兜底。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Noises");
        auto noiseResult = world::gen::noise::NoiseLoader::loadFromDataPackRepository(m_dataPackList);
        if (noiseResult.failed()) {
            spdlog::error("Failed to load noise parameters from data packs: {}", noiseResult.error().toString());
        } else {
            spdlog::info("Loaded {} noise parameters from data packs", noiseResult.value());
        }
    }

    // 从数据包加载密度函数（worldgen/density_function/*.json）
    // 依赖噪声（noise 叶子节点引用噪声名）。35 个 density_function 经两阶段 Holder
    // 引用解析（前向引用 + 共享子图 + 循环检测）注册到 DensityFunctionRegistry。
    // 噪声叶子节点解析期存 UnboundNoiseLeaf 占位，由 NoiseBindingVisitor 在
    // RandomState 组装 NoiseRouter 时按 name-hash 绑定真实 NormalNoise（阶段3 noise_settings）。
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::DensityFunctions");
        auto dfResult = world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(m_dataPackList);
        if (dfResult.failed()) {
            spdlog::error("Failed to load density functions from data packs: {}", dfResult.error().toString());
        } else {
            spdlog::info("Loaded {} density functions from data packs", dfResult.value());
        }
    }

    // 从数据包加载 noise_settings（worldgen/noise_settings/*.json）
    // 依赖 density_function（noise_router 15 字段是 DF Holder，字符串引用查 DensityFunctionRegistry）。
    // DimensionSettings::fromJson 解析 noise 4 尺寸 + 15 DF 路由模板（m_routerDfs，噪声叶子为
    // UnboundNoiseLeaf 占位）+ surface_rule + spawn_target + 标量字段，注册到 NoiseSettingsRegistry。
    // RandomState::create 据此走数据驱动唯一路径，经 NoiseBindingVisitor 绑定真实 NormalNoise。
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::NoiseSettings");
        auto nsResult = world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(m_dataPackList);
        if (nsResult.failed()) {
            spdlog::error("Failed to load noise_settings from data packs: {}", nsResult.error().toString());
        } else {
            spdlog::info("Loaded {} noise_settings from data packs", nsResult.value());
        }
    }

    // 加载模板池（先注册硬编码基础池，再从数据包加载 JSON 模板池）
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::TemplatePools");
        world::gen::structure::pools::Pools::initialize();
        size_t poolCount = world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(m_dataPackList);
        spdlog::info("Loaded {} template pools from data packs", poolCount);
    }

    // 数据驱动加载结构定义（worldgen/structure/*.json）
    // 顺序：模板池 → 结构定义（jigsaw 结构引用模板池）→ … → 结构标签（依赖结构已注册）。
    // 先 clear() 重置硬编码 initialize() 兜底写入的状态，再由 Loader 按 type 工厂构造并注册。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::Structures");
        world::gen::structure::StructureRegistry::clear();
        auto structureResult =
            world::gen::structure::StructureDefinitionLoader::loadFromDataPackRepository(m_dataPackList);
        if (structureResult.failed()) {
            spdlog::error("Failed to load structures from data packs: {}", structureResult.error().toString());
        } else {
            spdlog::info("Loaded {} structures from data packs", structureResult.value());
        }
        // 数据驱动注册完成后置位，使区块生成器兜底守卫不再触发硬编码注册。
        world::gen::structure::StructureRegistry::markInitialized();
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

    // 从数据包加载生物群系标签（须在 Biome 注册后，结构/结构集合引用标签前）
    // 填充 stronghold_biased_to、has_structure/* 等标签的 BiomeId 集合。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::BiomeTags");
        auto biomeTagResult = world::biome::BiomeTagLoader::loadFromDataPackRepository(m_dataPackList);
        if (biomeTagResult.failed()) {
            spdlog::error("Failed to load biome tags from data packs: {}", biomeTagResult.error().toString());
        } else {
            spdlog::info("Loaded {} biome tags from data packs", biomeTagResult.value());
        }
    }

    // 从数据包加载 flat_level_generator_preset（须在方块/biome 注册后：layers 的 block RL 经
    // BlockRegistry 取默认 BlockState，biome RL 经 BiomeLoader::biomeIdByName 映射 BiomeId）。
    // FlatLevelGeneratorPresetLoader 解析 9 个预设 JSON，注册到 FlatLevelGeneratorPresetRegistry，
    // 供 ServerDimensionManager flat 分支查表构造 FlatChunkGenerator。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::FlatPresets");
        auto flatResult =
            world::gen::settings::FlatLevelGeneratorPresetLoader::loadFromDataPackRepository(m_dataPackList);
        if (flatResult.failed()) {
            spdlog::error(
                "Failed to load flat_level_generator_presets from data packs: {}", flatResult.error().toString());
        } else {
            spdlog::info("Loaded {} flat_level_generator_presets from data packs", flatResult.value());
        }
    }

    // 从数据包加载 world_preset（须在 noise_settings + flat_preset 之后：flat 维度的内联 settings
    // 复用 FlatLevelGeneratorSettings::fromSettingsObject 依赖 BlockRegistry/BiomeLoader；noise 维度
    // 仅存 noise_settings RL，装配期由 RandomState::create 查 NoiseSettingsRegistry）。
    // WorldPresetLoader 解析 6 个预设 JSON，注册到 WorldPresetRegistry，
    // 供 ServerDimensionManager 三维度装配查表。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::WorldPresets");
        auto presetResult = world::gen::settings::WorldPresetLoader::loadFromDataPackRepository(m_dataPackList);
        if (presetResult.failed()) {
            spdlog::error("Failed to load world_presets from data packs: {}", presetResult.error().toString());
        } else {
            spdlog::info("Loaded {} world_presets from data packs", presetResult.value());
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

    // 数据驱动加载结构集合（worldgen/structure_set/*.json）
    // 顺序：结构定义（已注册）→ 生物群系标签（stronghold_biased_to 已填充）→ 结构集合
    // （要塞集合的 preferred_biomes 依赖生物群系标签）。先 clear() 重置硬编码兜底状态，
    // 再由 Loader 按 placement 类型构造并注册，最后 markInitialized() 置位。
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries::StructureSets");
        world::gen::structure::StructureSetRegistry::instance().clear();
        auto setResult = world::gen::structure::StructureSetLoader::loadFromDataPackRepository(m_dataPackList);
        if (setResult.failed()) {
            spdlog::error("Failed to load structure sets from data packs: {}", setResult.error().toString());
        } else {
            spdlog::info("Loaded {} structure sets from data packs", setResult.value());
        }
        world::gen::structure::StructureSetRegistry::instance().markInitialized();
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
        if (auto* es = serverDim->entitySyncManager()) {
            es->setOnEntitySpawn([this, serverDim](EntityInstanceId entityId, const Entity& entity) {
                MC_UNUSED(entityId);
                MC_UNUSED(entity);
                // 实体生成广播由 EntityTracker 处理
            });

            es->setOnEntityRemove([this, serverDim](EntityInstanceId entityId) {
                MC_UNUSED(entityId);
                // 实体移除广播由 EntityTracker 处理
            });

            es->setOnEntityMove([this, serverDim](EntityInstanceId entityId, const Vector3& pos, f32 yaw, f32 pitch) {
                MC_UNUSED(entityId);
                MC_UNUSED(pos);
                MC_UNUSED(yaw);
                MC_UNUSED(pitch);
                // 实体移动广播由 EntityTracker 处理
            });

            es->setOnEntityStatus([this, serverDim](EntityInstanceId entityId, u8 status) {
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
                // 1.21.11 无 UnloadChunk 网络包：区块卸载由客户端按距离启发式自行回收。
                // TODO(Phase6): 若需强制卸载远端区块，需补客户端距离判据或自定义信令。
                MC_UNUSED(playerId);
                MC_UNUSED(x);
                MC_UNUSED(z);
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
        mc::network::ir::play::GameEvent evt;
        evt.event = 7; // RainStrengthChange
        evt.value = rainStrength;
        broadcastPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(evt)},
        });
        m_lastSentRainStrength = rainStrength;
    }

    if (thunderChanged) {
        mc::network::ir::play::GameEvent evt;
        evt.event = 8; // ThunderStrengthChange
        evt.value = thunderStrength;
        broadcastPacket(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(evt)},
        });
        m_lastSentThunderStrength = thunderStrength;
    }

    if (weatherMgr.hasWeatherChanged()) {
        auto weatherType = weatherMgr.weatherType();
        if (weatherType == weather::WeatherType::Clear) {
            mc::network::ir::play::GameEvent evt;
            evt.event = 1; // EndRaining
            evt.value = 0.0f;
            broadcastPacket(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
        } else if (weatherType == weather::WeatherType::Rain || weatherType == weather::WeatherType::Thunder) {
            mc::network::ir::play::GameEvent evt;
            evt.event = 2; // BeginRaining
            evt.value = 0.0f;
            broadcastPacket(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
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
        mc::network::ir::play::GameEvent evt;
        evt.event = 7; // RainStrengthChange
        evt.value = rainStrength;
        sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
    }

    {
        mc::network::ir::play::GameEvent evt;
        evt.event = 8; // ThunderStrengthChange
        evt.value = thunderStrength;
        sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(evt)},
            });
    }
}

void MinecraftServer::sendInitialDifficultyToPlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendInitialDifficulty", "phase", "difficulty_sync");

    sendPacketToPlayer(playerId, serializeDifficultyPacket());
}

void MinecraftServer::sendKeepAliveToAll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "SendKeepAlive", "phase", "keepalive_sync");

    u64 timestamp = util::TimeUtils::getCurrentTimeMs();
    u64 tick = currentTick();

    m_playerManager->forEachPlayer([this, timestamp, tick](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            // 1.21.11 KeepAlive：服务端发 id，客户端原样回 id。用时间戳作为 id。
            mc::network::ir::play::KeepAlive pkt;
            pkt.id = static_cast<i64>(timestamp);
            sendPacketToPlayer(player.playerId,
                mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play,
                    mc::network::ir::PlayPacket{std::move(pkt)},
                });

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
    // 1.21.11 LevelChunkWithLight：chunkData(heightmaps+buffer+blockEntities) + lightData。
    // TODO(Phase6): 旧路径只产生单 blob，光照随 LightUpdate 单独发；此处 lightData 暂空，
    //   后续 ChunkSerializer 应产出分离的 chunk/light buffer 以完整对齐 1.21.11。
    MC_UNUSED(m_dimensionManager);
    mc::network::ir::play::LevelChunkWithLight pkt;
    pkt.x = static_cast<i32>(x);
    pkt.z = static_cast<i32>(z);
    pkt.chunkData = data;
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

void MinecraftServer::sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    // 1.21.11 无 UnloadChunk 网络包：区块卸载由客户端按距离启发式自行回收。
    // TODO(Phase6): 若需强制卸载远端区块，需补客户端距离判据或自定义信令。
    MC_UNUSED(playerId);
    MC_UNUSED(x);
    MC_UNUSED(z);
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

    // 1.21.11 LightUpdate：仅 chunkX/chunkZ + opaque lightData。
    // TODO(Phase6): 真正对齐 1.21.11 ClientboundLightUpdatePacket 的 masks/arrays 编码。
    //   当前 lightData 透传自研格式：sectionY(i32)+flags(u8)+[VarInt len+skyLight]+[VarInt len+blockLight]，
    //   客户端 _handleLightUpdate 按同格式反解（我方互通成立，真 Java 不通）。
    u8 flags = 0;
    if (!skyLight.empty()) flags |= 0x01;
    if (!blockLight.empty()) flags |= 0x02;
    if (trustEdges) flags |= 0x04;

    std::vector<u8> lightData;
    lightData.reserve(sizeof(i32) + sizeof(u8) + skyLight.size() + blockLight.size() + 16);
    // sectionY (i32, 大端)
    {
        i32 sy = sectionY;
        lightData.push_back(static_cast<u8>((sy >> 24) & 0xFF));
        lightData.push_back(static_cast<u8>((sy >> 16) & 0xFF));
        lightData.push_back(static_cast<u8>((sy >> 8) & 0xFF));
        lightData.push_back(static_cast<u8>(sy & 0xFF));
    }
    lightData.push_back(flags);
    // VarUInt 长度前缀的 skyLight
    appendVarUInt(lightData, static_cast<u32>(skyLight.size()));
    lightData.insert(lightData.end(), skyLight.begin(), skyLight.end());
    // VarUInt 长度前缀的 blockLight
    appendVarUInt(lightData, static_cast<u32>(blockLight.size()));
    lightData.insert(lightData.end(), blockLight.begin(), blockLight.end());

    mc::network::ir::play::LightUpdate pkt;
    pkt.x = static_cast<i32>(x);
    pkt.z = static_cast<i32>(z);
    pkt.lightData = std::move(lightData);

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    m_playerManager->forEachPlayer([&packet](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            player.send(mc::network::ir::IrPacket{packet});
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

void MinecraftServer::handlePlayerMovePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    namespace irplay = mc::network::ir::play;
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);

    // 解析四个 MovePlayer 变体之一，统一出位置/朝向/onGround 与"是否含位置变更"
    f64 posX = player->x;
    f64 posY = player->y;
    f64 posZ = player->z;
    f32 yaw = player->yaw;
    f32 pitch = player->pitch;
    bool onGround = player->onGround;
    bool hasPosChange = false;
    bool hasRotOnly = false;

    if (auto* p = std::get_if<irplay::MovePlayerPosRot>(&play)) {
        posX = p->x;
        posY = p->y;
        posZ = p->z;
        yaw = p->yRot;
        pitch = p->xRot;
        onGround = p->flags.onGround;
        hasPosChange = true;
    } else if (auto* p = std::get_if<irplay::MovePlayerPos>(&play)) {
        posX = p->x;
        posY = p->y;
        posZ = p->z;
        onGround = p->flags.onGround;
        hasPosChange = true;
    } else if (auto* p = std::get_if<irplay::MovePlayerRot>(&play)) {
        yaw = p->yRot;
        pitch = p->xRot;
        onGround = p->flags.onGround;
        hasRotOnly = true;
    } else if (auto* p = std::get_if<irplay::MovePlayerStatusOnly>(&play)) {
        onGround = p->flags.onGround;
    } else {
        return;
    }

    // 保存旧位置用于村庄进入检测
    BlockPos prevPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));

    player->x = static_cast<f32>(posX);
    player->y = static_cast<f32>(posY);
    player->z = static_cast<f32>(posZ);
    player->yaw = yaw;
    player->pitch = pitch;
    player->onGround = onGround;

    // 计算新区块坐标
    ChunkCoord newChunkX = math::floorTo<ChunkCoord>(posX / static_cast<f64>(world::CHUNK_WIDTH));
    ChunkCoord newChunkZ = math::floorTo<ChunkCoord>(posZ / static_cast<f64>(world::CHUNK_WIDTH));

    // 检查玩家是否移动到了新区块
    ChunkCoord oldChunkX = math::floorTo<ChunkCoord>(player->x / static_cast<f32>(world::CHUNK_WIDTH));
    ChunkCoord oldChunkZ = math::floorTo<ChunkCoord>(player->z / static_cast<f32>(world::CHUNK_WIDTH));
    bool chunkChanged = (newChunkX != oldChunkX || newChunkZ != oldChunkZ);
    (void)chunkChanged;

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
    if (overworld && overworld->world() && hasPosChange && !hasRotOnly) {
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

void MinecraftServer::handleTeleportConfirmPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::AcceptTeleportation>(&play);
    if (evt == nullptr) {
        return;
    }

    if (m_teleportManager->confirmTeleport(playerId, evt->teleportId)) {
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

void MinecraftServer::handleKeepAlivePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::KeepAlive>(&play);
    if (evt == nullptr) {
        return;
    }

    u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
    m_keepAliveManager->handleKeepAliveResponse(playerId, static_cast<u64>(evt->id), currentTimeMs);
}

void MinecraftServer::handleChatMessagePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::Chat>(&play);
    if (evt == nullptr) {
        return;
    }

    const std::string& message = evt->message;

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

void MinecraftServer::handleUpdateSignPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SignUpdate>(&play);
    if (evt == nullptr) {
        return;
    }

    const BlockPos signPos = BlockPos::fromLong(evt->blockPosPacked);

    // 获取玩家所在维度的世界
    ServerWorld* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::warn("UpdateSign: player {} has no world", playerId);
        return;
    }

    // 获取告示牌方块实体
    BlockEntity* blockEntity = world->getBlockEntity(signPos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Sign) {
        spdlog::warn(
            "UpdateSign: no sign entity at ({}, {}, {}) for player {}", signPos.x, signPos.y, signPos.z, playerId);
        return;
    }

    auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);

    // 安全检查：只有当前编辑者才能更新文本
    // 对应 MC Java 的 SignBlockEntity.setAllowedPlayerEditor 机制
    if (signEntity->getPlayerWhoMayEdit() != player->uuid) {
        spdlog::warn("UpdateSign: player {} is not the allowed editor of sign at ({}, {}, {})",
            playerId,
            signPos.x,
            signPos.y,
            signPos.z);
        return;
    }

    // 涂蜡的告示牌不允许修改文本
    if (signEntity->isWaxed()) {
        spdlog::warn("UpdateSign: sign at ({}, {}, {}) is waxed, ignoring update from player {}",
            signPos.x,
            signPos.y,
            signPos.z,
            playerId);
        signEntity->clearAllowedPlayerEditor();
        return;
    }

    // 更新4行文本
    for (i32 i = 0; i < 4; ++i) {
        signEntity->setLineFromLegacy(i, evt->lines[static_cast<std::size_t>(i)]);
    }

    // 清除编辑锁
    signEntity->clearAllowedPlayerEditor();

    // 标记方块实体已变更，触发区块存档保存
    signEntity->setChanged();

    // 广播 BlockEntity 数据给附近其他玩家，使其能看到更新后的告示牌文本
    // 参考 MC Java: SignBlockEntity.updateSignText -> level.sendBlockUpdated(pos, state, state, 3)
    world->broadcastBlockEntity(signPos);

    spdlog::info("UpdateSign: player {} updated sign at ({}, {}, {})", playerId, signPos.x, signPos.y, signPos.z);
}

// ============================================================================
// 骑乘 / 交互 / 载具移动（C→S）
//
// 以下 6 个处理体对应 MC Java 1.21.11 的 ServerboundPlayerInput /
// ServerboundMoveVehicle / ServerboundPlayerCommand / ServerboundPaddleBoat /
// ServerboundInteract / ServerboundUseItem。可完成项给出实现，依赖未实现
// 子系统（载具物理/反飞行/物品使用）的标 TODO(Phase6)。
// ============================================================================

void MinecraftServer::handlePlayerInputPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlayerInput 位掩码：bit0=forward bit1=backward bit2=left bit3=right
    // bit4=jump bit5=shift bit6=sprint（对齐 MC 1.21.11 net.minecraft.world.entity.player.Input）。
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerInput>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    const u8 input = evt->input;
    const bool forward = (input & 0x01) != 0;
    const bool backward = (input & 0x02) != 0;
    const bool left = (input & 0x04) != 0;
    const bool right = (input & 0x08) != 0;
    const bool jump = (input & 0x10) != 0;
    const bool shift = (input & 0x20) != 0;
    const bool sprint = (input & 0x40) != 0;

    // 疾跑状态由 PlayerCommand 维护，这里仅驱动载具。shift（潜行）在本项目
    // 走 PlayerCommand/PlayerInput 之外的状态链路（见 handlePlayerCommandPacket），
    // 载具侧忽略 shift。TODO(Phase6): 跳跃输入驱动 IJumpingMount 载具的蓄力跳跃。
    (void)jump;
    (void)shift;

    auto* playerEntity = playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    // 玩家骑乘载具时，输入转发给载具。仅 Boat 已实现 handleInput；其它载具
    // （马/骆驼/羊驼等 IJumpingMount 载具）的输入链路 TODO(Phase6)。
    const EntityInstanceId vehicleId = playerEntity->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return;
    }

    auto* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    auto* boat = dynamic_cast<mc::entity::BoatEntity*>(vehicle);
    if (boat != nullptr) {
        // 注意参数顺序：(left, right, forward, backward)（对齐 BoatEntity::handleInput）。
        boat->handleInput(left, right, forward, backward);
        return;
    }

    // 非船载具输入处理 TODO(Phase6)。
    (void)sprint;
}

void MinecraftServer::handleMoveVehiclePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 最小实现：NaN 校验 + 写入载具位置 + 回送 ClientboundMoveVehicle 校正。
    // 完整 moved-too-quickly/wrongly 反飞行检测 TODO(Phase6)。
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundMoveVehicle>(&play);
    if (evt == nullptr) {
        return;
    }

    // 对应 MC Java NetworkValidatorUtils.isInvalidValue：坐标/朝向含 NaN 即拒。
    if (std::isnan(evt->x) || std::isnan(evt->y) || std::isnan(evt->z) || std::isnan(evt->yRot) ||
        std::isnan(evt->xRot)) {
        spdlog::warn("MoveVehicle: player {} sent NaN position, ignoring", playerId);
        return;
    }

    auto* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    const EntityInstanceId vehicleId = playerEntity->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return;
    }

    auto* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    vehicle->setPosition(static_cast<f32>(evt->x), static_cast<f32>(evt->y), static_cast<f32>(evt->z));
    vehicle->setRotation(evt->yRot, evt->xRot);
    vehicle->setOnGround(evt->onGround);

    // 回送校正：服务端权威位置回传客户端，使客户端载具与服务端对齐
    // （对齐 MC Java ServerGamePacketListenerImpl.handleMoveVehicle 发 ClientboundMoveVehicle）。
    mc::network::ir::play::ClientboundMoveVehicle correction;
    correction.x = static_cast<f64>(vehicle->position().x);
    correction.y = static_cast<f64>(vehicle->position().y);
    correction.z = static_cast<f64>(vehicle->position().z);
    correction.yRot = vehicle->yaw();
    correction.xRot = vehicle->pitch();
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(correction)}});
}

void MinecraftServer::handlePlayerCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlayerCommand action（对齐 MC 1.21.11 ServerboundPlayerCommandPacket.Action）：
    // 0=STOP_SLEEPING 1=START_SPRINTING 2=STOP_SPRINTING 3=START_RIDING_JUMP
    // 4=STOP_RIDING_JUMP 5=OPEN_INVENTORY 6=START_FALL_FLYING。
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerCommand>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    switch (evt->action) {
        case 0: { // STOP_SLEEPING
            if (auto* serverPlayer = playerEntity->asServerPlayer()) {
                serverPlayer->stopSleepInBed(true, true);
            } else {
                playerEntity->stopSleeping();
            }
            break;
        }
        case 1: // START_SPRINTING
            playerEntity->setSprinting(true);
            break;
        case 2: // STOP_SPRINTING
            playerEntity->setSprinting(false);
            break;
        case 3: { // START_RIDING_JUMP（data=跳跃强度）
            const EntityInstanceId vehicleId = playerEntity->getVehicle();
            if (vehicleId == INVALID_ENTITY_ID) {
                break;
            }
            auto* vehicle = world->getEntity(vehicleId);
            if (vehicle == nullptr) {
                break;
            }
            auto* jumpingMount = dynamic_cast<mc::entity::IJumpingMount*>(vehicle);
            if (jumpingMount != nullptr) {
                jumpingMount->startJumping(evt->data);
            }
            break;
        }
        case 4: { // STOP_RIDING_JUMP
            const EntityInstanceId vehicleId = playerEntity->getVehicle();
            if (vehicleId == INVALID_ENTITY_ID) {
                break;
            }
            auto* vehicle = world->getEntity(vehicleId);
            if (vehicle == nullptr) {
                break;
            }
            auto* jumpingMount = dynamic_cast<mc::entity::IJumpingMount*>(vehicle);
            if (jumpingMount != nullptr) {
                jumpingMount->stopJumping();
            }
            break;
        }
        case 5: // OPEN_INVENTORY
            // 复用既有 Inventory 开包处理体（IntegratedServer 覆写打开菜单）。
            handleOpenPlayerInventoryPacket(playerId, packet);
            break;
        case 6: // START_FALL_FLYING
            if (!playerEntity->tryToStartFallFlying()) {
                playerEntity->stopFallFlying();
            }
            break;
        default:
            spdlog::info("PlayerCommand: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void MinecraftServer::handlePaddleBoatPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PaddleBoat>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    const EntityInstanceId vehicleId = playerEntity->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return;
    }

    auto* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    auto* boat = dynamic_cast<mc::entity::BoatEntity*>(vehicle);
    if (boat == nullptr) {
        // 非船载具无桨状态，忽略。
        return;
    }

    boat->setPaddleState(evt->left, evt->right);
}

void MinecraftServer::handleInteractPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // Interact action：0=INTERACT 1=ATTACK 2=INTERACT_AT。
    // 成就 player_interacted_with_entity 与严格物品/距离校验 TODO(Phase6)。
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::Interact>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    auto* target = world->getEntity(static_cast<EntityInstanceId>(evt->entityId));
    if (target == nullptr) {
        return;
    }

    // 基本距离校验：超出追踪距离（6 块）拒绝，对齐 MC Java 的 maxInteractDistance 思路。
    // 严格反作弊距离检测 TODO(Phase6)。
    constexpr f32 kMaxInteractDistance = 6.0f;
    if (playerEntity->distanceSqTo(*target) > kMaxInteractDistance * kMaxInteractDistance) {
        return;
    }

    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    switch (evt->action) {
        case 0: // INTERACT
            (void)playerEntity->interactOn(*target, hand);
            break;
        case 1: // ATTACK
            playerEntity->attack(*target);
            break;
        case 2: { // INTERACT_AT（带命中点）
            const Vector3 hitPosition(evt->hitX, evt->hitY, evt->hitZ);
            (void)target->applyPlayerInteraction(*playerEntity, hitPosition, hand);
            break;
        }
        default:
            spdlog::info("Interact: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void MinecraftServer::handleUseItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 客户端暂无出站 UseItem 发送路径，处理骨架 TODO(Phase6)。
    // 完整实现须：取手持物品 → 物品 useOn 空气分支 → 消耗/冷却/同步。
    (void)playerId;
    (void)packet;
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

void MinecraftServer::handleBlockInteractionPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerAction>(&play);
    if (evt == nullptr) {
        return;
    }

    // 1.21.11 PlayerAction：action 0/1/2 对应 Start/Abort/StopDestroy，与旧 BlockInteractionAction 序数一致。
    //   action 3..6（DROP_ALL/DROP_ITEM/SWAP_HANDS 等）由物品逻辑路径单独处理，此处仅转发挖掘相关。
    const BlockPos pos = BlockPos::fromLong(evt->blockPosPacked);

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "MinecraftServer::handleBlockInteractionPacket",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 仅挖掘相关 action 走 MiningManager
    if (evt->action < 0 || evt->action > 2) {
        return;
    }
    const auto action = static_cast<network::BlockInteractionAction>(evt->action);

    // 处理挖掘状态
    miningManager().handleBlockInteraction(playerId, pos, action);

    if (action == network::BlockInteractionAction::StopDestroyBlock) {
        if (!miningManager().tryCompleteMining(playerId, pos)) {
            spdlog::warn("Ignored premature StopDestroyBlock from player {} at {}", playerId, pos.toString());
        }
    }
}

void MinecraftServer::handleBlockPlacementPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::UseItemOn>(&play);
    if (evt == nullptr) {
        return;
    }

    const auto& hit = evt->blockHit;
    const BlockPos pos = BlockPos::fromLong(hit.blockPosPacked);
    auto* playerWorld = getPlayerWorld(playerId);
    const BlockState* clickedState = playerWorld ? playerWorld->getBlockState(pos) : nullptr;
    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;
    const Direction face = static_cast<Direction>(hit.direction);
    const Vector3 hitPosition(hit.hitX, hit.hitY, hit.hitZ);

    const auto tryOpenCrafting = [this, playerId, pos, clickedState]() {
        return isCraftingTableState(clickedState) && tryOpenCraftingContainer(playerId, pos);
    };

    ItemStack heldStack = getHeldItemForPlacement(playerId);
    if (heldStack.isEmpty()) {
        if (!tryOpenCrafting()) {
            (void)blockInteractionManager().handleBlockUse(playerId, pos, hand, hitPosition, face);
        }
        return;
    }

    const Item* heldItem = heldStack.getItem();
    const bool holdingBlockItem =
        heldItem != nullptr && BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId()) != nullptr;

    if (!holdingBlockItem) {
        if (!tryOpenCrafting()) {
            (void)blockInteractionManager().handleBlockUse(playerId, pos, hand, hitPosition, face);
        }
        return;
    }

    auto interactionResult =
        blockInteractionManager().handleBlockPlacement(playerId, pos, hitPosition, face, heldStack);

    if (interactionResult.success() && interactionResult.value().blockPlaced &&
        interactionResult.value().itemConsumed) {
        const i32 selectedSlot = getSelectedHotbarSlot(playerId);
        ItemStack updatedStack = heldStack;
        updatedStack.shrink(1);
        setInventoryItem(playerId, selectedSlot, updatedStack);
        syncPlayerInventory(playerId);
    }
}

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

    mc::network::ir::play::SetTime timePkt;
    timePkt.gameTime = time.gameTime();
    timePkt.dayTime = dayTime;
    timePkt.tickDayTime = time.daylightCycleEnabled();
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(timePkt)},
        });

    // 发送世界出生点（指南针指向位置，从主世界维度获取）
    // 1.21.11 SetDefaultSpawnPosition：dimension + blockPosPacked + yaw + pitch
    Vector3d worldSpawn(0.0, 64.0, 0.0);
    f32 spawnAngle = 0.0f;
    auto* overworld = m_dimensionManager->getOverworld();
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
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(spawnPkt)},
        });

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

    // 1.21.11 Commands：命令树 NBT。当前以 registry 产出的 JSON 文本字节透传。
    // TODO(Phase6): 对齐 1.21.11 ClientboundCommandsPacket 的 CommandNode[] NBT 编码。
    mc::network::ir::play::Commands pkt;
    const std::string& json = m_commandRegistry->getCommandTreeJson();
    pkt.payload = std::vector<u8>(json.begin(), json.end());
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
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

    // 通过 EntityEvent 通知客户端权限等级变更（status byte = 24 + level）。
    // 1.21.11 权限等级走 EntityEvent(OP_PERMISSION_LEVEL_0..3 = 24..27)。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(player->id());
    pkt.eventId = static_cast<u8>(24 + permissionLevel);
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });

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

void MinecraftServer::dispatchPacket(u32 sessionId, const mc::network::ir::IrPacket& packet)
{
    // 新网络层：packet 已是解码后的 IrPacket。解析 PlayerId 后交统一路由入口。
    // sessionId 仅用于解析远程玩家 PlayerId（Local 模式直连 router，不走此入口）。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "DispatchPacketToHandler", "sessionId", sessionId);

    if (packet.phase != mc::network::protocol::ConnectionProtocol::Play) {
        spdlog::warn("dispatchPacket: non-Play packet in dispatch (phase={})", static_cast<int>(packet.phase));
        return;
    }

    PlayerId playerId = getPlayerIdForSession(sessionId);
    if (playerId == 0) {
        return;
    }
    routeInboundPlayPacket(playerId, packet);
}

void MinecraftServer::routeInboundPlayPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 新网络层：按 ir::PlayPacket 变体分发到既有 handle*Packet。
    MC_ASSERT_RELEASE(packet.phase == mc::network::protocol::ConnectionProtocol::Play);
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    namespace irplay = mc::network::ir::play;

    if (std::holds_alternative<irplay::MovePlayerPos>(play) || std::holds_alternative<irplay::MovePlayerPosRot>(play) ||
        std::holds_alternative<irplay::MovePlayerRot>(play) ||
        std::holds_alternative<irplay::MovePlayerStatusOnly>(play)) {
        handlePlayerMovePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::AcceptTeleportation>(play)) {
        handleTeleportConfirmPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::KeepAlive>(play)) {
        handleKeepAlivePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::Chat>(play)) {
        handleChatMessagePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerAction>(play)) {
        handleBlockInteractionPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::UseItemOn>(play)) {
        handleBlockPlacementPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SetCarriedItem>(play)) {
        handleHotbarSelectPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ContainerClick>(play)) {
        handleContainerClickPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ContainerClose>(play)) {
        handleCloseContainerPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerCommand>(play)) {
        // PlayerCommand 全 action 分发（疾跑/潜行/起床/骑乘跳跃/开背包/滑翔）
        handlePlayerCommandPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SignUpdate>(play)) {
        handleUpdateSignPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerInput>(play)) {
        handlePlayerInputPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundMoveVehicle>(play)) {
        handleMoveVehiclePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PaddleBoat>(play)) {
        handlePaddleBoatPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::Interact>(play)) {
        handleInteractPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::UseItem>(play)) {
        handleUseItemPacket(playerId, packet);
    } else {
        // 未覆盖的 C→S 变体（如 SetCreativeModeSlot 等创造模式/命令相关包）
        spdlog::info("routeInboundPlayPacket: unhandled C->S play variant");
    }
}

// ============================================================================
// 声音广播方法
// ============================================================================

void MinecraftServer::broadcastSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    // 1.21.11 PlaySound：Holder<SoundEvent>(opaque) + source + 坐标×8 + volume + pitch + seed。
    // TODO(Phase6): soundHolder 仅以 ResourceLocation 字符串字节承载，未对齐 Holder<SoundEvent> wire；
    //   seed 暂用固定值 0。
    mc::network::ir::play::PlaySound pkt;
    std::string idStr = soundEventId.toString();
    pkt.soundHolder = std::vector<u8>(idStr.begin(), idStr.end());
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;
    broadcastPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    });
}

void MinecraftServer::broadcastSoundInRange(const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 range,
    f32 volume,
    f32 pitch)
{
    // 1.21.11 PlaySound（同上），仅发送给范围内玩家。
    mc::network::ir::play::PlaySound pkt;
    std::string idStr = soundEventId.toString();
    pkt.soundHolder = std::vector<u8>(idStr.begin(), idStr.end());
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_playerManager->forEachPlayer([this, &position, range, &packet, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - position.x;
        f32 dy = player.y - position.y;
        f32 dz = player.z - position.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
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
    // 1.21.11 PlaySound（同上），定向发送。
    mc::network::ir::play::PlaySound pkt;
    std::string idStr = soundEventId.toString();
    pkt.soundHolder = std::vector<u8>(idStr.begin(), idStr.end());
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
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
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    auto irPacket = buildLevelParticlesIr(pos, offset, count, std::move(options));

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_playerManager->forEachPlayer([this, &pos, range, &irPacket, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, irPacket);
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
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    auto irPacket = buildLevelParticlesIr(pos, offset, count, std::move(options));
    sendPacketToPlayer(playerId, irPacket);
}

void MinecraftServer::broadcastVibrationParticleInRange(
    const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks, f32 range)
{
    // VibrationParticleOption.STREAM_CODEC = PositionSource + VAR_INT arrivalInTicks
    // PositionSource = VarInt(kind: 0=Block 1=Entity)
    //   kind=0: i64 packedBlockPos；kind=1: VarInt entityId + FLOAT yOffset
    mc::network::ir::play::ParticleOptions options;
    options.type = particle::ParticleTypeId::Vibration;
    options.arrivalInTicks = arrivalInTicks;
    const std::string sourceType = targetSource.type();
    if (sourceType == "entity") {
        const auto& entitySource = static_cast<const gameevent::EntityPositionSource&>(targetSource);
        options.vibrationSourceKind = 1;
        options.vibrationEntityId = static_cast<i32>(entitySource.entityId());
        options.vibrationYOffset = entitySource.yOffset();
    } else {
        // 默认按方块位置源处理（"block" 或任何未知类型）
        const auto& blockSource = static_cast<const gameevent::BlockPositionSource&>(targetSource);
        options.vibrationSourceKind = 0;
        options.vibrationBlockPosPacked = blockSource.pos().asLong();
    }

    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void MinecraftServer::broadcastTrailParticleInRange(
    const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks, f32 range)
{
    // TrailParticleOption.STREAM_CODEC = Vec3(3×F64 target) + INT color(ARGB) + VAR_INT duration
    mc::network::ir::play::ParticleOptions options;
    options.type = particle::ParticleTypeId::Trail;
    options.trailTargetX = targetPosition.x;
    options.trailTargetY = targetPosition.y;
    options.trailTargetZ = targetPosition.z;
    options.color = color;
    options.trailDuration = durationInTicks;
    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void MinecraftServer::broadcastEntityEffectParticleInRange(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color, f32 range)
{
    // ColorParticleOption(ENTITY_EFFECT).STREAM_CODEC = INT color（ARGB 大端）
    mc::network::ir::play::ParticleOptions options;
    options.type = particle::ParticleTypeId::EntityEffect;
    options.color = color;
    auto irPacket = buildLevelParticlesIr(pos, offset, count, std::move(options));

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void MinecraftServer::broadcastBlockParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, u32 blockStateId, f32 range)
{
    // BlockParticleOption.STREAM_CODEC = VarInt(blockStateId)
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    options.blockStateId = blockStateId;
    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void MinecraftServer::broadcastItemParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack, f32 range)
{
    // ItemParticleOption.STREAM_CODEC = 完整 ItemStack wire（VarInt count + Item holder + DataComponentPatch）
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    options.item = mc::network::ir::toItemStackView(itemStack);
    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_playerManager->forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, irPacket);
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

void MinecraftServer::broadcastEntityStatusInRange(EntityInstanceId entityId, u8 status, const Vector3& pos, f32 range)
{
    // 1.21.11 EntityEvent：entityId + eventId(status)。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(entityId);
    pkt.eventId = status;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void MinecraftServer::broadcastEntityAnimationInRange(
    EntityInstanceId entityId, u8 animation, const Vector3& pos, f32 range)
{
    // 1.21.11 Animate：entityId + action(animation)。
    mc::network::ir::play::Animate pkt;
    pkt.id = static_cast<i32>(entityId);
    pkt.action = animation;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void MinecraftServer::broadcastHurtAnimationInRange(
    EntityInstanceId entityId, f32 hurtDir, const Vector3& pos, f32 range)
{
    // 1.21.11 HurtAnimation：entityId + yaw(hurtDir)。
    // 受害者自身与范围内追踪者均会收到。
    mc::network::ir::play::HurtAnimation pkt;
    pkt.id = static_cast<i32>(entityId);
    pkt.yaw = hurtDir;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void MinecraftServer::broadcastSetEntityLinkInRange(
    EntityInstanceId entityId, EntityInstanceId linkedEntityId, const Vector3& pos, f32 range)
{
    // 1.21.11 SetEntityLink：sourceId + destId（leash/riding 关系）。
    mc::network::ir::play::SetEntityLink pkt;
    pkt.sourceId = static_cast<i32>(entityId);
    pkt.destId = static_cast<i32>(linkedEntityId);
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

// ============================================================================
// 世界事件广播
// ============================================================================

void MinecraftServer::broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    // 1.21.11 LevelEvent：type + blockPosPacked + data + globalEvent。
    mc::network::ir::play::LevelEvent pkt;
    pkt.type = eventId;
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.data = data;
    pkt.globalEvent = false;
    broadcastPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    });
}

void MinecraftServer::broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range)
{
    mc::network::ir::play::LevelEvent pkt;
    pkt.type = eventId;
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.data = data;
    pkt.globalEvent = false;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void MinecraftServer::broadcastBlockEventInRange(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId, f32 range)
{
    // 参考 MC Java: ServerPlayerList.broadcast(null, x, y, z, 64.0, dimension, new ClientboundBlockEventPacket)
    // 1.21.11 BlockEvent：blockPosPacked + b0 + b1 + blockId(此处用 blockStateId)。
    mc::network::ir::play::BlockEvent pkt;
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.b0 = paramA;
    pkt.b1 = paramB;
    pkt.blockId = static_cast<i32>(blockStateId);
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void MinecraftServer::broadcastBlockEntityInRange(
    const BlockPos& pos, BlockEntityType type, std::shared_ptr<nbt::CompoundTag> tag, f32 range)
{
    // 参考 MC Java: PlayerList.broadcast(null, x, y, z, 64.0, dimension,
    //   new ClientboundBlockEntityDataPacket(pos, type, tag))
    // 方块实体数据变化后，将最新 NBT 快照发送给附近客户端。
    // 1.21.11 BlockEntityData：blockPosPacked + blockEntityType + CompoundTag（无长度前缀）。
    mc::network::ir::play::BlockEntityData pkt;
    pkt.blockPosPacked = pos.asLong();
    pkt.blockEntityType = static_cast<i32>(type);
    pkt.tag = std::move(tag);
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    Vector3 fpos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    m_playerManager->forEachPlayer([this, &fpos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, fpos.x, fpos.y, fpos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void MinecraftServer::broadcastBlockBreakProgressInRange(
    EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress, f32 range)
{
    // 对应 MC Java: ServerLevel.destroyBlockProgress()
    // 1.21.11 BlockDestruction：breakerId + blockPosPacked + progress(0-9)。
    // MC Java 原版行为：排除破坏者自身（serverplayer.getId() != breakerId），
    // 只向同维度、32格范围内的其他玩家发送。破坏者自身的动画由客户端本地直接驱动。
    mc::network::ir::play::BlockDestruction pkt;
    pkt.id = static_cast<i32>(breakerId);
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.progress = static_cast<u8>(progress);

    // 将 breakerId (EntityInstanceId) 转换为 PlayerId，用于排除破坏者自身
    PlayerId breakerPlayerId = playerEntityManager().getPlayerIdByEntityId(breakerId);

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, breakerPlayerId, &pos, rangeSq, &pkt](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        // 排除破坏者自身：MC Java 原版中 serverplayer.getId() != breakerId
        if (breakerPlayerId != 0 && player.playerId == breakerPlayerId) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId,
                mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play,
                    mc::network::ir::PlayPacket{pkt},
                });
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
    // 构造 1.21.11 Explosion IR（结构化：中心/半径/方块数/击退/粒子/声音）
    mc::network::ir::play::Explosion pkt;
    pkt.centerX = position.x;
    pkt.centerY = position.y;
    pkt.centerZ = position.z;
    pkt.radius = strength;
    pkt.blockCount = static_cast<i32>(affectedBlocks.size());
    const auto kbIt = playerKnockback.find(static_cast<u64>(playerId));
    pkt.hasPlayerKnockback = (kbIt != playerKnockback.end());
    if (pkt.hasPlayerKnockback) {
        pkt.knockbackX = kbIt->second.x;
        pkt.knockbackY = kbIt->second.y;
        pkt.knockbackZ = kbIt->second.z;
    }
    pkt.explosionParticle.type = particle::ParticleTypeId::Explosion;
    pkt.explosionSound.direct = true;
    pkt.explosionSound.identifier = "minecraft:entity.generic.explode";
    pkt.explosionSound.hasFixedRange = false;

    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

} // namespace mc::server
