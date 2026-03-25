#include "IntegratedServer.hpp"
#include "CoreCommandBridge.hpp"
#include "common/item/BlockItem.hpp"
#include "common/item/Items.hpp"
#include "common/item/BlockItemRegistry.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/BlockItemUseContext.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/BlockBreakAnimPacket.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/util/Direction.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/entity/EntityRegistry.hpp"
#include "common/entity/mob/MobEntity.hpp"
#include "common/entity/loot/LootConditions.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/world/drop/BlockDropHandler.hpp"

#include "common/perfetto/TraceEvents.hpp"

#include <spdlog/spdlog.h>
#include <cmath>
#include <cstring>
#include "common/util/assert/AssertAll.hpp"

namespace mc::server {

// ============================================================================
// ServerCollisionWorld 实现1
// ============================================================================

const BlockState* IntegratedServer::ServerCollisionWorld::getBlockState(i32 x, i32 y, i32 z) const {
    if (!isWithinWorldBounds(x, y, z)) {
        return nullptr;
    }

    ChunkCoord chunkX = world::toChunkCoord(x);
    ChunkCoord chunkZ = world::toChunkCoord(z);

    const ChunkData* chunk = getChunkAt(chunkX, chunkZ);
    if (!chunk) {
        return nullptr;
    }

    i32 localX = world::toLocalCoord(x);
    i32 localZ = world::toLocalCoord(z);

    return chunk->getBlock(localX, y, localZ);
}

bool IntegratedServer::ServerCollisionWorld::isWithinWorldBounds(i32 x, i32 y, i32 z) const {
    (void)x;  // X/Z理论上无限制
    (void)z;
    return y >= getMinBuildHeight() && y < getMaxBuildHeight();
}

const ChunkData* IntegratedServer::ServerCollisionWorld::getChunkAt(ChunkCoord x, ChunkCoord z) const {
    return m_chunkManager.getChunk(x, z);
}

namespace {

Player& getMenuPlayer() {
    static Player player(0, "IntegratedServerMenu");
    return player;
}

/**
 * @brief 发送游戏数据包到本地端点
 * @tparam PacketT 数据包类型
 * @param endpoint 本地端点
 * @param packetType 数据包类型
 * @param packet 数据包实例
 */
template <typename PacketT>
void sendGamePacket(network::LocalEndpoint* endpoint, network::PacketType packetType, const PacketT& packet) {
    if (endpoint == nullptr || !endpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    packet.serialize(payload);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(packetType, payload.buffer());
    endpoint->send(fullPacket.data(), fullPacket.size());
}

/**
 * @brief 检查方块状态是否为工作台
 */
bool isCraftingTableState(const BlockState* state) {
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

/**
 * @brief 服务端区块读取器
 *
 * 用于方块放置/破坏时读取区块数据
 */
class ServerChunkReader final : public IBlockReader {
public:
    explicit ServerChunkReader(ServerChunkManager& chunkManager,
                               u64 seed = 0,
                               u64 currentTick = 0,
                               i64 dayTime = 0)
        : m_chunkManager(chunkManager)
        , m_seed(seed)
        , m_currentTick(currentTick)
        , m_dayTime(dayTime)
    {
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return nullptr;
        }

        const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(x) / 16.0));
        const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(z) / 16.0));
        ChunkData* chunk = m_chunkManager.getChunkSync(chunkX, chunkZ);
        if (chunk == nullptr) {
            return nullptr;
        }

        const i32 localX = x - chunkX * 16;
        const i32 localZ = z - chunkZ * 16;
        return chunk->getBlock(localX, y, localZ);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override
    {
        (void)x;
        (void)z;
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    // IWorld 接口实现
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return fluid::Fluid::getFluidState(0); }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 0; }

private:
    ServerChunkManager& m_chunkManager;
    u64 m_seed = 0;
    u64 m_currentTick = 0;
    i64 m_dayTime = 0;
};

} // namespace

class IntegratedLightingWorld final : public IWorld {
public:
    IntegratedLightingWorld(ServerChunkManager& chunkManager,
                            EntityManager& entityManager,
                            PhysicsEngine* physicsEngine,
                            ServerCore* serverCore,
                            const IntegratedServerConfig& config)
        : m_chunkManager(chunkManager)
        , m_entityManager(entityManager)
        , m_physicsEngine(physicsEngine)
        , m_serverCore(serverCore)
        , m_config(config)
    {
    }

    void setLightManager(WorldLightManager* lightManager) {
        m_lightManager = lightManager;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        if (!isWithinWorldBounds(x, y, z)) {
            return nullptr;
        }

        const ChunkCoord chunkX = world::toChunkCoord(x);
        const ChunkCoord chunkZ = world::toChunkCoord(z);
        const ChunkData* chunk = m_chunkManager.getChunk(chunkX, chunkZ);
        if (!chunk) {
            return nullptr;
        }

        const i32 localX = world::toLocalCoord(x);
        const i32 localZ = world::toLocalCoord(z);
        return chunk->getBlock(localX, y, localZ);
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        if (!isWithinWorldBounds(x, y, z)) {
            return false;
        }

        const ChunkCoord chunkX = world::toChunkCoord(x);
        const ChunkCoord chunkZ = world::toChunkCoord(z);
        ChunkData* chunk = m_chunkManager.getChunkSync(chunkX, chunkZ);
        if (!chunk) {
            return false;
        }

        const i32 localX = world::toLocalCoord(x);
        const i32 localZ = world::toLocalCoord(z);
        chunk->setBlock(localX, y, localZ, state);
        chunk->setDirty(true);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        return state ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override {
        return m_chunkManager.getChunk(x, z);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override {
        return m_chunkManager.getChunk(x, z) != nullptr;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override {
        if (m_lightManager) {
            return m_lightManager->getBlockLight(BlockPos(x, y, z));
        }
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override {
        if (m_lightManager) {
            return m_lightManager->getSkyLight(BlockPos(x, y, z));
        }
        return 15;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override {
        (void)x;
        (void)z;
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }

    [[nodiscard]] PhysicsEngine* physicsEngine() override { return m_physicsEngine; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return m_physicsEngine; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override {
        return {};
    }

    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }

    [[nodiscard]] u64 seed() const override {
        return m_serverCore ? m_serverCore->config().seed : static_cast<u64>(m_config.seed);
    }

    [[nodiscard]] u64 currentTick() const override {
        return m_serverCore ? m_serverCore->currentTick() : 0;
    }

    [[nodiscard]] i64 dayTime() const override {
        return m_serverCore ? m_serverCore->gameTime().dayTime() : 0;
    }

    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 1; }

private:
    ServerChunkManager& m_chunkManager;
    EntityManager& m_entityManager;
    PhysicsEngine* m_physicsEngine = nullptr;
    ServerCore* m_serverCore = nullptr;
    const IntegratedServerConfig& m_config;
    WorldLightManager* m_lightManager = nullptr;
};

class IntegratedLightProvider final : public IChunkLightProvider {
public:
    using LightChangedCallback = std::function<void(LightType, const SectionPos&)>;

    IntegratedLightProvider(IntegratedLightingWorld& world,
                            ServerChunkManager& chunkManager,
                            LightChangedCallback lightChangedCallback)
        : m_world(world)
        , m_chunkManager(chunkManager)
        , m_lightChangedCallback(std::move(lightChangedCallback))
    {
    }

    [[nodiscard]] IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override {
        return m_chunkManager.getChunk(x, z);
    }

    [[nodiscard]] const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override {
        return m_chunkManager.getChunk(x, z);
    }

    [[nodiscard]] const BlockState* getBlockStateForLight(const BlockPos& pos) const override {
        return m_world.getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] IWorld* getWorld() override {
        return &m_world;
    }

    [[nodiscard]] const IWorld* getWorld() const override {
        return &m_world;
    }

    void markLightChanged(LightType type, const SectionPos& pos) override {
        if (m_lightChangedCallback) {
            m_lightChangedCallback(type, pos);
        }
    }

    [[nodiscard]] bool hasSkyLight() const override {
        return true;
    }

    [[nodiscard]] i32 getMinBuildHeight() const override {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] i32 getMaxBuildHeight() const override {
        return world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] i32 getSectionCount() const override {
        return world::CHUNK_SECTIONS;
    }

private:
    IntegratedLightingWorld& m_world;
    ServerChunkManager& m_chunkManager;
    LightChangedCallback m_lightChangedCallback;
};

IntegratedServer::IntegratedServer() = default;

IntegratedServer::~IntegratedServer() {
    if (m_running) {
        stop();
    }
}

Result<void> IntegratedServer::initialize(const IntegratedServerConfig& config) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    m_config = config;

    // 初始化 ServerCore
    ServerCoreConfig coreConfig;
    coreConfig.viewDistance = config.viewDistance;
    coreConfig.defaultGameMode = config.defaultGameMode;
    coreConfig.seed = static_cast<u64>(config.seed);
    coreConfig.maxPlayers = 1;  // 内置服务器只支持单人
    coreConfig.tickRate = config.tickRate;
    m_serverCore = std::make_unique<ServerCore>(coreConfig);

    // 初始化方块注册表（必须在创建地形生成器之前）
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    Items::initialize();
    spdlog::info("Vanilla items initialized");

    // 初始化附魔注册表
    item::enchant::EnchantmentRegistry::initialize();
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    spdlog::info("Block items initialized");

    RecipeLoader recipeLoader;
    auto recipeLoadResult = recipeLoader.loadFromDirectory("data/minecraft/recipes");
    if (recipeLoadResult.failed()) {
        spdlog::warn("Failed to load crafting recipes: {}", recipeLoadResult.error().toString());
    } else {
        spdlog::info("Loaded {} crafting recipes ({} failed)",
                     recipeLoadResult.value().successCount,
                     recipeLoadResult.value().failedCount);
    }

    spdlog::info("Initializing integrated server...");
    spdlog::info("World: {}, Seed: {}, View distance: {}",
                 m_config.worldName, m_config.seed, m_config.viewDistance);

    // 创建本地连接对
    m_connectionPair = std::make_unique<network::LocalConnectionPair>();
    m_connectionPair->connect();
    m_serverEndpoint = &m_connectionPair->serverEndpoint();

    // 创建区块管理器（使用 MC 1.16.5 风格噪声生成器）
    DimensionSettings settings = DimensionSettings::overworld();
    auto generator = std::make_unique<NoiseChunkGenerator>(m_config.seed, std::move(settings));
    m_chunkManager = std::make_unique<ServerChunkManager>(std::move(generator));
    m_chunkManager->setChunkLoadedCallback([this](ChunkCoord chunkX, ChunkCoord chunkZ) {
        enqueueChunkLightingInit(chunkX, chunkZ);
    });
    (void)m_chunkManager->initialize();
    m_chunkManager->startWorkers();
    m_chunkManager->setViewDistance(m_config.viewDistance);

    // 创建物理引擎碰撞世界和物理引擎
    m_collisionWorld = std::make_unique<ServerCollisionWorld>(*m_chunkManager);
    m_physicsEngine = std::make_unique<PhysicsEngine>(*m_collisionWorld);

    // 创建光照世界视图与光照引擎
    m_lightingWorld = std::make_unique<IntegratedLightingWorld>(
        *m_chunkManager,
        m_entityManager,
        m_physicsEngine.get(),
        m_serverCore.get(),
        m_config);
    m_lightProvider = std::make_unique<IntegratedLightProvider>(
        *m_lightingWorld,
        *m_chunkManager,
        [this](LightType type, const SectionPos& pos) {
            markLightChanged(type, pos);
        });
    m_lightManager = std::make_unique<WorldLightManager>(m_lightProvider.get(), true, true);
    m_lightingWorld->setLightManager(m_lightManager.get());

    // 设置实体生成回调
    m_chunkManager->setEntitySpawnCallback(
        [this](const std::vector<SpawnedEntityData>& entities) {
            handleSpawnedEntities(entities);
        });

    // 配置区块管理器的票据管理器
    m_chunkManager->setViewDistance(m_config.viewDistance);

    // 设置票据级别变化回调
    // 注意：这个回调负责两件事：
    // 1. 更新 SingleChunkLifecycleManager 的级别（用于区块卸载检查）
    // 2. 调用 onChunkLevelChanged（用于区块加载/卸载逻辑）
    m_chunkManager->setTicketLevelChangeCallback(
        [this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
            // 更新 SingleChunkLifecycleManager 的级别
            if (newLevel <= world::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
                auto* holder = m_chunkManager->getSingleChunkLifecycleManager(x, z);
                if (holder) {
                    holder->setLevel(newLevel);
                }
            }
            // 调用区块级别变化处理
            onChunkLevelChanged(x, z, oldLevel, newLevel);
        });

    // 启动服务端线程
    m_running = true;
    m_serverThread = std::make_unique<std::thread>([this]() {
        mainLoop();
    });

    m_initialized = true;
    spdlog::info("Integrated server initialized");
    return Result<void>::ok();
}

void IntegratedServer::stop() {
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping integrated server...");
    m_running = false;

    // 1. 停止 Worker 线程（立即取消未完成任务）
    if (m_chunkManager) {
        m_chunkManager->stopWorkers();
    }

    // 2. 断开连接以唤醒可能阻塞的线程
    if (m_connectionPair) {
        m_connectionPair->disconnect();
    }

    // 3. 等待服务端线程结束
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();

    // 4. 清理待发送队列
    {
        std::lock_guard<std::mutex> lock(m_pendingSendsMutex);
        m_pendingSends.clear();
    }

    // 5. 关闭区块管理器
    if (m_chunkManager) {
        m_chunkManager->shutdown();
    }

    shutdown();
    spdlog::info("Integrated server stopped");
}

network::LocalEndpoint* IntegratedServer::getClientEndpoint() {
    if (m_connectionPair) {
        return &m_connectionPair->clientEndpoint();
    }
    return nullptr;
}

u64 IntegratedServer::tickCount() const noexcept {
    return m_serverCore ? m_serverCore->currentTick() : 0;
}

i64 IntegratedServer::dayTime() const noexcept {
    return m_serverCore ? m_serverCore->gameTime().dayTime() : 0;
}

i64 IntegratedServer::gameTime() const noexcept {
    return m_serverCore ? m_serverCore->gameTime().gameTime() : 0;
}

void IntegratedServer::mainLoop() {
    using clock = std::chrono::steady_clock;
    const auto tickDuration = std::chrono::milliseconds(1000 / m_config.tickRate);

    // 设置线程名称
    mc::perfetto::PerfettoManager::instance().setThreadName("IntegratedServerThread");

    spdlog::info("Integrated server started ({} TPS)", m_config.tickRate);

    while (m_running.load(std::memory_order_acquire)) {
        MC_TRACE_EVENT("server.tick", "MainLoopIteration");

        auto startTime = clock::now();

        tick();

        auto elapsed = clock::now() - startTime;
        auto sleepTime = tickDuration - elapsed;

        MC_TRACE_COUNTER("server.tick", "ServerTickTime", static_cast<i64>(elapsed.count()));

        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

void IntegratedServer::tick() {
    MC_TRACE_EVENT("server.tick", "IntegratedServerTick");

    // 如果已停止，跳过处理
    if (!m_running.load()) {
        return;
    }

    // 使用 ServerCore 的 tick
    {
        MC_TRACE_EVENT("server.tick", "CoreTick");
        m_serverCore->tick();
    }

    // 更新所有实体（关键：实体移动需要每帧tick）
    {
        MC_TRACE_EVENT("server.tick", "EntityTick");
        m_entityManager.tick();
    }

    // 同步实体位置到客户端
    syncEntityPositions();

    // 更新玩家挖掘进度（广播破坏动画）
    tickPlayerMining();

    // 处理网络数据包
    {
        MC_TRACE_EVENT("server.network", "ProcessPackets");
        std::vector<u8> packetData;
        while (m_running.load() && m_serverEndpoint && m_serverEndpoint->receive(packetData)) {
            onPacketReceived(packetData.data(), packetData.size());
        }
    }

    // 如果已停止，跳过更新
    if (!m_running.load()) {
        return;
    }

    // 处理待发送区块（从 Worker 线程推送）
    {
        MC_TRACE_EVENT("server.chunk", "processPendingChunkSends");
        processPendingChunkSends();
    }

    // 处理待初始化光照的区块（仅服务端线程执行，避免并发访问光照管理器）
    processPendingChunkLightingInits();

    // 处理延迟卸载（边缘区块防抖）
    {
        MC_TRACE_EVENT("server.chunk", "PendingChunkUnloadTick");
        processPendingChunkUnloads();
    }

    // 更新区块管理器
    if (m_chunkManager) {
        MC_TRACE_EVENT("server.chunk", "ChunkManagerTick");
        m_chunkManager->tick();
    }

    // 更新光照传播队列
    {
        MC_TRACE_EVENT("server.lighting", "LightTick");
        tickLighting();
    }

    // 心跳（每 15 秒）
    u64 tick = m_serverCore->currentTick();
    if (tick % (static_cast<u64>(m_config.tickRate) * 15) == 0) {
        MC_TRACE_EVENT("server.network", "SendKeepAlive");
        u64 timestamp = util::TimeUtils::getCurrentTimeMs();
        sendKeepAlive(timestamp);
    }

    // 日光周期由 ServerCore::tick() 内的 TimeManager::tick() 负责，无需再此重复增加

    // 追踪统计
    MC_TRACE_COUNTER("server.tick", "PlayerCount", static_cast<i64>(m_serverCore->playerCount()));

    // 每 20 tick 同步一次时间到客户端（类似 Java 版，每秒一次）
    if (tick % 20 == 0) {
        sendTimeUpdate();
    }

    // 同步天气变化到客户端
    sendWeatherUpdate();
}

void IntegratedServer::shutdown() {
    // 释放客户端连接（必须在 ServerCore 重置前清空）
    m_clientConnection.reset();

    // 清理光照链路
    m_lightManager.reset();
    m_lightProvider.reset();
    m_lightingWorld.reset();

    // 清理区块管理器
    m_chunkManager.reset();

    m_pendingChunkUnloads.clear();

    // 清理 ServerCore
    m_serverCore.reset();

    if (m_connectionPair) {
        m_connectionPair->disconnect();
        m_connectionPair.reset();
    }

    m_serverEndpoint = nullptr;
    m_initialized = false;
}

void IntegratedServer::requestChunkAsync(ChunkCoord x, ChunkCoord z) {
    if (!m_running.load()) return;

    auto* player = getPlayerData();
    if (!player) return;

    ChunkId id(x, z);
    if (player->loadedChunks.count(id)) return;  // 已发送

    if (!m_chunkManager) return;

    // 异步请求区块
    m_chunkManager->getChunkAsync(x, z,
        [this, x, z, id](bool success, ChunkData* chunk) {
            // Worker 线程回调
            if (!m_running.load() || !success || !chunk) return;

            // 玩家可能已移动到其他区域，避免发送过期区块。
            if (!m_chunkManager || !m_chunkManager->shouldChunkLoad(x, z)) {
                return;
            }

            // 检查是否已发送
            auto* p = getPlayerData();
            if (!p || p->loadedChunks.count(id)) return;

            // 序列化区块数据
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.failed()) {
                spdlog::error("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
                return;
            }

            // 推送到主线程队列
            std::lock_guard<std::mutex> lock(m_pendingSendsMutex);
            m_pendingSends.push_back({x, z, std::move(result.value())});
        },
        &ChunkStatuses::FULL
    );
}

void IntegratedServer::processPendingChunkSends() {
    // 移动待发送队列到本地
    std::vector<PendingChunkSend> sends;
    {
        std::lock_guard<std::mutex> lock(m_pendingSendsMutex);
        sends = std::move(m_pendingSends);
        m_pendingSends.clear();
    }

    // 发送所有待发送区块
    for (const auto& send : sends) {
        if (!m_running.load()) return;

        if (!m_chunkManager || !m_chunkManager->shouldChunkLoad(send.x, send.z)) {
            continue;
        }

        // 检查客户端是否仍然登录
        auto* player = getPlayerData();
        if (!player || !player->loggedIn) return;

        ChunkId id(send.x, send.z);
        if (player->loadedChunks.count(id)) continue;  // 已发送

        // 在服务端线程完成光照初始化，避免 Worker 线程并发访问光照管理器
        initializeChunkLighting(send.x, send.z);

        player->loadedChunks.insert(id);

        spdlog::debug("Sending chunk ({}, {}) to client, size: {} bytes",
                      send.x, send.z, send.serializedData.size());
        sendChunkData(send.x, send.z, send.serializedData);
    }
}

void IntegratedServer::enqueueChunkLightingInit(ChunkCoord x, ChunkCoord z) {
    std::lock_guard<std::mutex> lock(m_pendingLightingInitsMutex);
    m_pendingLightingInits.emplace_back(x, z);
    // spdlog::info("[LightQueue] enqueue chunk=({}, {}), pending_count={}",
    //               x, z, m_pendingLightingInits.size());
    MC_TRACE_EVENT("server.lighting", "EnqueueChunkLightingInit", "ChunkX", x, "ChunkZ", z, "PendingCount", static_cast<i64>(m_pendingLightingInits.size()));
}

void IntegratedServer::processPendingChunkLightingInits() {
    std::vector<ChunkPos> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingLightingInitsMutex);
        pending = std::move(m_pendingLightingInits);
        m_pendingLightingInits.clear();
    }

    if (!pending.empty()) {
        // spdlog::info("[LightQueue] process pending_count={}", pending.size());
        MC_TRACE_EVENT("server.lighting", "ProcessPendingChunkLightingInits", "PendingCount", static_cast<i64>(pending.size()));
    }

    for (const ChunkPos& pos : pending) {
        if (!m_running.load()) {
            spdlog::warn("[LightQueue] stop processing because server is no longer running");
            return;
        }
        // spdlog::info("[LightQueue] init chunk lighting chunk=({}, {})", pos.x, pos.z);
        MC_TRACE_EVENT("server.lighting", "InitializeChunkLighting", "ChunkX", pos.x, "ChunkZ", pos.z);
        initializeChunkLighting(pos.x, pos.z);
    }
}

void IntegratedServer::sendChunkToClient(ChunkCoord x, ChunkCoord z) {
    // 如果服务器已停止，跳过
    if (!m_running.load()) {
        return;
    }

    auto* player = getPlayerData();
    if (!player) return;

    ChunkId id(x, z);

    // 检查是否已发送
    if (player->loadedChunks.find(id) != player->loadedChunks.end()) {
        return;
    }

    // 异步请求区块
    requestChunkAsync(x, z);
}

void IntegratedServer::onPacketReceived(const u8* data, size_t size) {
    if (size < network::PACKET_HEADER_SIZE) {
        spdlog::warn("Packet too small: {} bytes", size);
        return;
    }

    // 解析包头
    network::PacketDeserializer deser(data, size);
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    if (sizeResult.failed() || typeResult.failed()) {
        spdlog::warn("Failed to read packet header");
        return;
    }

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());

    switch (packetType) {
        case network::PacketType::LoginRequest:
            handleLoginRequest(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::PlayerMove:
            handlePlayerMove(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::BlockInteraction:
            handleBlockInteraction(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::PlayerTryUseItemOnBlock:
            handleBlockPlacement(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::HotbarSelect:
            handleHotbarSelect(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::ContainerClick:
            handleContainerClick(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::CloseContainer:
            handleCloseContainer(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::TeleportConfirm:
            handleTeleportConfirm(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::KeepAlive:
            handleKeepAlive(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::ChatMessage:
            handleChatMessage(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        default:
            spdlog::debug("Unhandled packet type: {}", static_cast<int>(packetType));
            break;
    }
}

void IntegratedServer::handleLoginRequest(const u8* data, size_t size) {
    MC_TRACE_EVENT("server.network", "HandleLoginRequest");
    network::PacketDeserializer deser(data, size);
    auto result = network::LoginRequestPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::warn("Failed to parse login request");
        sendLoginResponse(false, 0, "", "Invalid login request");
        return;
    }

    auto& packet = result.value();
    String username = packet.username();

    spdlog::info("Player '{}' attempting to join", username);

    // 创建本地连接，并将 shared_ptr 保存到成员变量，
    // 防止 ServerPlayerData 中的 weak_ptr 失效导致玩家被误删
    m_clientConnection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());

    // 使用 ServerCore 添加玩家
    m_clientPlayerId = m_serverCore->nextPlayerId();
    auto* player = m_serverCore->addPlayer(m_clientPlayerId, username, m_clientConnection);

    if (!player) {
        sendLoginResponse(false, 0, username, "Failed to add player");
        return;
    }

    player->loggedIn = true;
    player->gameMode = m_config.defaultGameMode;
    m_clientData.inventory.clear();
    m_clientData.inventory.setSelectedSlot(0);

    if (player->gameMode == GameMode::Creative) {
        // 第一格放置钻石镐
        if (Items::DIAMOND_PICKAXE != nullptr) {
            m_clientData.inventory.setItem(0, ItemStack(*Items::DIAMOND_PICKAXE, 1));
        }

        // 其余格子填充方块物品
        i32 slot = 1;
        BlockItemRegistry::instance().forEachBlockItem([this, player, &slot](const BlockItem& item) {
            if (slot >= PlayerInventory::TOTAL_SIZE) {
                return;
            }
            m_clientData.inventory.setItem(slot, ItemStack(item, 64));
            ++slot;
        });
    }

    // 设置初始位置（出生点）
    player->x = 0.0f;
    player->y = 90.0f;
    player->z = 0.0f;

    // 发送登录成功响应
    sendLoginResponse(true, m_clientPlayerId, username, "Welcome to singleplayer world!");

    // 发送初始传送
    [[maybe_unused]] u32 teleportId = m_serverCore->teleportPlayer(m_clientPlayerId, player->x, player->y, player->z, player->yaw, player->pitch);
    sendPlayerInventory();

    // 发送初始天气状态
    sendInitialWeatherState();

    // 初始化玩家票据位置
    ChunkCoord spawnChunkX = static_cast<ChunkCoord>(std::floor(player->x / 16.0f));
    ChunkCoord spawnChunkZ = static_cast<ChunkCoord>(std::floor(player->z / 16.0f));
    m_lastPlayerChunkX = spawnChunkX;
    m_lastPlayerChunkZ = spawnChunkZ;

    // 在票据管理器中注册玩家位置
    m_chunkManager->updatePlayerPosition(m_clientPlayerId, spawnChunkX, spawnChunkZ);

    spdlog::info("Player '{}' (ID: {}) joined the game at chunk ({}, {})",
                 username, m_clientPlayerId, spawnChunkX, spawnChunkZ);
}

void IntegratedServer::handlePlayerMove(const u8* data, size_t size) {
    MC_TRACE_EVENT("server.network", "HandlePlayerMove");
    auto* player = getPlayerData();

    MC_ASSERT_RELEASE_MSG(player, "Player data should exist when handling player move");
    MC_ASSERT_RELEASE_MSG(player->loggedIn, "Player should be logged in when handling player move");

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse player move");
        return;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

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

    // 检查是否跨越区块边界
    ChunkCoord newChunkX = static_cast<ChunkCoord>(std::floor(player->x / 16.0f));
    ChunkCoord newChunkZ = static_cast<ChunkCoord>(std::floor(player->z / 16.0f));

    if (newChunkX != m_lastPlayerChunkX || newChunkZ != m_lastPlayerChunkZ) {
        handlePlayerChunkMove(newChunkX, newChunkZ);
        m_lastPlayerChunkX = newChunkX;
        m_lastPlayerChunkZ = newChunkZ;
    }
}

void IntegratedServer::handleBlockInteraction(const u8* data, size_t size) {
    MC_TRACE_EVENT("server.network", "HandleBlockInteraction");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn || !m_chunkManager) {
        spdlog::warn("[Mining] ignore interaction: player={}, loggedIn={}, chunkManager={}",
                     static_cast<void*>(player),
                     player ? player->loggedIn : false,
                     static_cast<void*>(m_chunkManager.get()));
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::BlockInteractionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse block interaction packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    BlockPos pos(packet.x(), packet.y(), packet.z());

    // 处理挖掘动作
    updatePlayerMining(pos, packet.action());

    // 只有 StopDestroyBlock 会实际破坏方块
    if (packet.action() != network::BlockInteractionAction::StopDestroyBlock) {
        // spdlog::trace("[Mining] action is not StopDestroyBlock, only updating progress state");
        return;
    }

    if (packet.y() < world::MIN_BUILD_HEIGHT || packet.y() >= world::MAX_BUILD_HEIGHT) {
        spdlog::warn("[Mining] reject due to Y out of range: y={}, range=[{}, {})",
                     packet.y(), world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);
        return;
    }

    const f64 eyeX = player->x;
    const f64 eyeY = player->y + Player::PLAYER_EYE_HEIGHT;
    const f64 eyeZ = player->z;
    const f64 targetX = static_cast<f64>(packet.x()) + 0.5;
    const f64 targetY = static_cast<f64>(packet.y()) + 0.5;
    const f64 targetZ = static_cast<f64>(packet.z()) + 0.5;
    const f64 dx = targetX - eyeX;
    const f64 dy = targetY - eyeY;
    const f64 dz = targetZ - eyeZ;
    const f64 distanceSquared = dx * dx + dy * dy + dz * dz;

    if (distanceSquared > 36.0) {
        spdlog::warn("[Mining] Out-of-range interaction rejected: distanceSq={}, target=({}, {}, {}), eye=({}, {}, {})",
                     distanceSquared,
                     packet.x(), packet.y(), packet.z(),
                     eyeX, eyeY, eyeZ);
        return;
    }

    ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.x()) / 16.0));
    ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.z()) / 16.0));
    ChunkData* chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
    if (!chunk) {
        spdlog::warn("[Mining] Missing chunk for block interaction at chunk ({}, {})", chunkX, chunkZ);
        return;
    }

    MC_TRACE_INSTANT("server.world.mining", "BlockStateCheck",
                      "Chunk", fmt::format("({}, {})", chunkX, chunkZ),
                      "LocalPos", fmt::format("({}, {}, {})",
                                              packet.x() - chunkX * 16,
                                              packet.y(),
                                              packet.z() - chunkZ * 16));

    const i32 localX = packet.x() - chunkX * 16;
    const i32 localZ = packet.z() - chunkZ * 16;
    const BlockState* state = chunk->getBlock(localX, packet.y(), localZ);
    if (state == nullptr || state->isAir() || state->hardness() < 0.0f) {
        spdlog::warn("[Mining] Block interaction rejected due to invalid target state at ({}, {}, {})",
                     packet.x(), packet.y(), packet.z());
        return;
    }

    MC_TRACE_INSTANT("server.world.mining", "BlockStateInfo",
                      "StateId", state->stateId(),
                      "Hardness", state->hardness(),
                      "LightLevel", state->lightLevel());

    // 获取玩家手持物品
    ItemStack heldItemCopy;
    i32 selectedSlot = 0;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        selectedSlot = m_clientData.inventory.getSelectedSlot();
        heldItemCopy = m_clientData.inventory.getSelectedStack();
    }
    const ItemStack* heldItem = heldItemCopy.isEmpty() ? nullptr : &heldItemCopy;
    MC_TRACE_INSTANT("server.world.mining", "PlayerToolInfo",
                      "SelectedSlot", selectedSlot,
                      "HeldItem", heldItemCopy.isEmpty() ? "<empty>" : heldItemCopy.getItem()->itemLocation().toString(),
                      "Count", heldItemCopy.getCount());

    // 检查是否可以采集方块
    // 使用 ServerPlayerData 的游戏模式检查创造模式
    const Player* playerForHarvest = nullptr;  // 暂时无法获取 Player 对象
    bool canHarvest = BlockDropHandler::canHarvestBlock(*state, playerForHarvest, heldItem);
    MC_TRACE_INSTANT("server.world.mining", "HarvestCheck",
                      "CanHarvest", canHarvest,
                      "GameMode", static_cast<i32>(player->gameMode));

    // 创造模式下不生成掉落物
    bool isCreativeMode = (player->gameMode == GameMode::Creative);

    // 生成掉落物（仅在可以采集且非创造模式时）
    if (canHarvest && !isCreativeMode) {
        const u64 seed = m_serverCore ? m_serverCore->config().seed : static_cast<u64>(m_config.seed);
        const u64 currentTick = m_serverCore ? m_serverCore->currentTick() : 0;
        const i64 dayTime = m_serverCore ? m_serverCore->gameTime().dayTime() : 0;
        ServerChunkReader dropWorld(*m_chunkManager, seed, currentTick, dayTime);

        BlockPos blockPos(packet.x(), packet.y(), packet.z());
        auto drops = BlockDropHandler::generateDrops(
            dropWorld,
            blockPos,
            *state,
            nullptr,  // Player 对象暂时不可用，使用 gameMode 检查创造模式
            heldItem,
            m_lootTableManager);

        if (!drops.empty()) {
            spdlog::debug("[Mining] generated drops count={}", drops.size());
            (void)BlockDropHandler::spawnDrops(
                m_entityManager,
                m_physicsEngine.get(),
                blockPos,
                drops,
                "");
        }

        // 消耗工具耐久度（仅在成功采集时）
        if (heldItemCopy.isDamageable() && state->hardness() > 0.0f) {
            bool broken = heldItemCopy.attemptDamageItem(1);
            spdlog::debug("[Mining] durability consumed, broken={} newDamage={}",
                          broken, heldItemCopy.getDamage());

            // 更新物品栏
            {
                std::lock_guard<std::mutex> lock(m_clientDataMutex);
                m_clientData.inventory.setItem(selectedSlot, heldItemCopy);
            }

            // 如果工具损坏，发送物品更新包到客户端
            if (broken) {
                sendPlayerInventory();
            }
        }
    }

    Block* airBlock = Block::getBlock(ResourceLocation("minecraft:air"));
    if (!airBlock) {
        spdlog::error("Failed to resolve minecraft:air while destroying block");
        return;
    }

    const i32 oldLightLevel = state->lightLevel();
    const i32 newLightLevel = airBlock->defaultState().lightLevel();

    // 写入区块并触发光照更新
    MC_TRACE_INSTANT("server.world.mining", "BlockDestroy",
                      "StateId", state->stateId(),
                      "OldLight", oldLightLevel,
                      "NewLight", newLightLevel);
    chunk->setBlock(localX, packet.y(), localZ, &airBlock->defaultState());
    chunk->setDirty(true);
    onBlockStateChangedForLighting(packet.x(), packet.y(), packet.z(), oldLightLevel, newLightLevel);
    sendBlockUpdate(packet.x(), packet.y(), packet.z(), airBlock->defaultState().stateId());
    MC_TRACE_INSTANT("server.world.mining", "BlockDestroyComplete",
                      "Position", fmt::format("({}, {}, {})", packet.x(), packet.y(), packet.z()));

    // spdlog::info("[Mining] Destroyed block {} at ({}, {}, {})",
    //              state->blockLocation().toString(),
    //              packet.x(), packet.y(), packet.z());
}

void IntegratedServer::handleBlockPlacement(const u8* data, size_t size)
{
    MC_TRACE_EVENT("server.network", "HandleBlockPlacement");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn || !m_chunkManager) {
        spdlog::warn("[Place] ignore placement: player={}, loggedIn={}, chunkManager={}",
                     static_cast<void*>(player),
                     player ? player->loggedIn : false,
                     static_cast<void*>(m_chunkManager.get()));
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    spdlog::info("[Place] Server received placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
                 packet.x(), packet.y(), packet.z(),
                 static_cast<i32>(packet.face()),
                 packet.hitX(), packet.hitY(), packet.hitZ());

    // 验证距离（最大6格）
    const f64 eyeX = player->x;
    const f64 eyeY = player->y + Player::PLAYER_EYE_HEIGHT;
    const f64 eyeZ = player->z;
    const f64 targetX = static_cast<f64>(packet.x()) + 0.5;
    const f64 targetY = static_cast<f64>(packet.y()) + 0.5;
    const f64 targetZ = static_cast<f64>(packet.z()) + 0.5;
    const f64 dx = targetX - eyeX;
    const f64 dy = targetY - eyeY;
    const f64 dz = targetZ - eyeZ;
    const f64 distanceSquared = dx * dx + dy * dy + dz * dz;

    if (distanceSquared > 36.0) {
        spdlog::warn("[Place] Out-of-range placement attempt: distanceSq={:.2f}", distanceSquared);
        return;
    }

    if (player->gameMode == GameMode::Spectator) {
        spdlog::warn("[Place] reject placement in spectator mode");
        return;
    }

    // 检查 Y 范围
    if (packet.y() < world::MIN_BUILD_HEIGHT || packet.y() >= world::MAX_BUILD_HEIGHT) {
        spdlog::warn("[Place] Invalid Y position: {}", packet.y());
        return;
    }

    // 计算放置位置（击中面的相邻方块）
    BlockPos clickedPos(packet.x(), packet.y(), packet.z());
    Direction face = packet.face();

    // 检查区块是否已加载
    ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.x()) / 16.0));
    ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.z()) / 16.0));
    ChunkData* chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
    if (!chunk) {
        spdlog::warn("[Place] Chunk not loaded at ({}, {})", chunkX, chunkZ);
        return;
    }

    const i32 clickedLocalX = packet.x() - chunkX * 16;
    const i32 clickedLocalZ = packet.z() - chunkZ * 16;
    const BlockState* clickedState = chunk->getBlock(clickedLocalX, packet.y(), clickedLocalZ);
    if (clickedState != nullptr) {
        spdlog::debug("[Place] clicked block stateId={} at ({}, {}, {})",
                      clickedState->stateId(), packet.x(), packet.y(), packet.z());
    } else {
        spdlog::warn("[Place] clicked block is nullptr at ({}, {}, {})",
                     packet.x(), packet.y(), packet.z());
    }
    if (isCraftingTableState(clickedState)) {
        spdlog::info("[Use] Opening crafting table at ({}, {}, {})", packet.x(), packet.y(), packet.z());
        openCraftingTableMenu();
        return;
    }

    ItemStack heldStack = m_clientData.inventory.getSelectedStack();
    if (heldStack.isEmpty()) {
        spdlog::debug("[Place] Selected stack is empty");
        return;
    }

    spdlog::info("[Place] selected item={} count={} damage={}",
                  heldStack.getItem() ? heldStack.getItem()->itemLocation().toString() : "<null>",
                  heldStack.getCount(), heldStack.getDamage());

    const Item* heldItem = heldStack.getItem();
    if (heldItem == nullptr) {
        return;
    }

    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId());
    if (blockItem == nullptr) {
        spdlog::debug("[Place] Selected item is not a block item: {}", heldItem->itemLocation().toString());
        return;
    }

    const u64 seed = m_serverCore ? m_serverCore->config().seed : static_cast<u64>(m_config.seed);
    const u64 currentTick = m_serverCore ? m_serverCore->currentTick() : 0;
    const i64 dayTime = m_serverCore ? m_serverCore->gameTime().dayTime() : 0;
    ServerChunkReader worldReader(*m_chunkManager, seed, currentTick, dayTime);
    BlockItemUseContext context(worldReader,
                                nullptr,
                                heldStack,
                                packet.hitPosition(),
                                clickedPos,
                                face,
                                player->yaw);

    if (!blockItem->tryPlace(context)) {
        spdlog::debug("[Place] Block item placement validation failed");
        return;
    }

    const BlockPos placePos = context.placementPos();
    spdlog::info("[Place] computed placementPos=({}, {}, {})",
                  placePos.x, placePos.y, placePos.z);
    const BlockState* newState = blockItem->getStateForPlacement(context);
    if (newState == nullptr) {
        spdlog::debug("[Place] Block item did not provide a placement state");
        return;
    }

    spdlog::info("[Place] new block stateId={} block={}",
                  newState->stateId(), newState->blockLocation().toString());

    // 检查放置位置的区块是否已加载
    ChunkCoord placeChunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(placePos.x) / 16.0));
    ChunkCoord placeChunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(placePos.z) / 16.0));
    ChunkData* placeChunk = (placeChunkX == chunkX && placeChunkZ == chunkZ) ? chunk
        : m_chunkManager->getChunkSync(placeChunkX, placeChunkZ);

    if (!placeChunk) {
        spdlog::warn("[Place] Target chunk not loaded at ({}, {})", placeChunkX, placeChunkZ);
        return;
    }

    const i32 placeLocalX = placePos.x - placeChunkX * 16;
    const i32 placeLocalZ = placePos.z - placeChunkZ * 16;
    const BlockState* oldState = placeChunk->getBlock(placeLocalX, placePos.y, placeLocalZ);
    const i32 oldLightLevel = oldState ? oldState->lightLevel() : 0;
    const i32 newLightLevel = newState->lightLevel();
    spdlog::info("[Place] target oldStateId={} oldLight={} newLight={} targetChunk=({}, {}) local=({}, {})",
                  oldState ? oldState->stateId() : 0,
                  oldLightLevel,
                  newLightLevel,
                  placeChunkX,
                  placeChunkZ,
                  placeLocalX,
                  placeLocalZ);

    // 写入区块并触发光照更新
    placeChunk->setBlock(placeLocalX, placePos.y, placeLocalZ, newState);
    placeChunk->setDirty(true);
    onBlockStateChangedForLighting(placePos.x, placePos.y, placePos.z, oldLightLevel, newLightLevel);
    spdlog::info("[Place] block write + light update finished at ({}, {}, {})", placePos.x, placePos.y, placePos.z);

    if (player->gameMode != GameMode::Creative) {
        const i32 selectedSlot = m_clientData.inventory.getSelectedSlot();
        ItemStack updatedStack = m_clientData.inventory.getItem(selectedSlot);
        updatedStack.shrink(1);
        m_clientData.inventory.setItem(selectedSlot, updatedStack);
        sendPlayerInventory();
    }

    // 广播方块更新
    sendBlockUpdate(placePos.x, placePos.y, placePos.z, newState->stateId());

    spdlog::info("[Place] Placed block {} at ({}, {}, {})",
                 blockItem->block().blockLocation().toString(),
                 placePos.x, placePos.y, placePos.z);
}

void IntegratedServer::handleHotbarSelect(const u8* data, size_t size)
{
    MC_TRACE_EVENT("server.network", "HandleHotbarSelect");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = HotbarSelectPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse hotbar select packet: {}", result.error().message());
        return;
    }

    m_clientData.inventory.setSelectedSlot(result.value().slot());
}

void IntegratedServer::handleContainerClick(const u8* data, size_t size)
{
    MC_TRACE_EVENT("server.network", "HandleContainerClick");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    AbstractContainerMenu* openMenu = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        openMenu = m_clientData.openMenu.get();
    }

    if (!openMenu) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = ContainerClickPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse container click packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    if (packet.containerId() != openMenu->getId()) {
        return;
    }

    ClickType clickType = (packet.button() == 0) ? ClickType::Pick : ClickType::PickSome;
    if (packet.action() == ClickAction::QuickMove) {
        clickType = ClickType::QuickMove;
    }

    Player& menuPlayer = getMenuPlayer();
    openMenu->clicked(packet.slotIndex(), packet.button(), clickType, menuPlayer);

    sendContainerContent(*openMenu);
    sendPlayerInventory();
}

void IntegratedServer::handleCloseContainer(const u8* data, size_t size)
{
    MC_TRACE_EVENT("server.network", "HandleCloseContainer");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    std::unique_ptr<AbstractContainerMenu> openMenu;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        openMenu = std::move(m_clientData.openMenu);
        m_clientData.openContainerType = ContainerType::Player;
    }

    if (!openMenu) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse close container packet: {}", result.error().message());
        return;
    }

    if (result.value().containerId() != openMenu->getId()) {
        return;
    }

    Player& menuPlayer = getMenuPlayer();
    openMenu->removed(menuPlayer);
    sendPlayerInventory();
}

void IntegratedServer::handlePlayerChunkMove(ChunkCoord newChunkX, ChunkCoord newChunkZ) {
    MC_TRACE_EVENT("server.world", "HandlePlayerChunkMove",
               "OldChunkX", m_lastPlayerChunkX, "OldChunkZ", m_lastPlayerChunkZ,
               "NewChunkX", newChunkX, "NewChunkZ", newChunkZ);
    // spdlog::debug("Player crossed chunk boundary: ({}, {}) -> ({}, {})",
    //               m_lastPlayerChunkX, m_lastPlayerChunkZ, newChunkX, newChunkZ);

    // 更新票据管理器中的玩家位置
    // 这会自动触发区块加载/卸载的计算
    m_chunkManager->updatePlayerPosition(m_clientPlayerId, newChunkX, newChunkZ);
}

void IntegratedServer::onChunkLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    // 如果服务器已停止，跳过
    if (!m_running.load()) {
        return;
    }

    auto* player = getPlayerData();
    if (!player) return;

    ChunkId id(x, z);
    bool wasLoaded = world::shouldChunkLoad(oldLevel);
    bool isLoaded = world::shouldChunkLoad(newLevel);

    if (!wasLoaded && isLoaded) {
        // 区块从卸载变为加载 - 异步请求
        spdlog::debug("Chunk ({}, {}) loading: level {} -> {}", x, z, oldLevel, newLevel);
        m_pendingChunkUnloads.erase(chunkKey(x, z));
        requestChunkAsync(x, z);
    } else if (wasLoaded && !isLoaded) {
        // 区块从加载变为卸载：延迟一小段时间，避免边缘抖动导致闪烁
        spdlog::debug("Chunk ({}, {}) scheduled for unload: level {} -> {}", x, z, oldLevel, newLevel);
        const u64 nowTick = m_serverCore ? m_serverCore->currentTick() : 0;
        m_pendingChunkUnloads[chunkKey(x, z)] = nowTick + CHUNK_UNLOAD_GRACE_TICKS;
    }
}

void IntegratedServer::processPendingChunkUnloads() {
    if (!m_chunkManager) {
        m_pendingChunkUnloads.clear();
        return;
    }

    auto* player = getPlayerData();
    if (!player) {
        m_pendingChunkUnloads.clear();
        return;
    }

    const u64 nowTick = m_serverCore ? m_serverCore->currentTick() : 0;

    for (auto it = m_pendingChunkUnloads.begin(); it != m_pendingChunkUnloads.end();) {
        const u64 dueTick = it->second;
        if (nowTick < dueTick) {
            ++it;
            continue;
        }

        ChunkCoord x = 0;
        ChunkCoord z = 0;
        chunkKeyToCoord(it->first, x, z);

        // 经过防抖窗口后再次确认是否仍应卸载
        if (m_chunkManager->shouldChunkLoad(x, z)) {
            it = m_pendingChunkUnloads.erase(it);
            continue;
        }

        const ChunkId id(x, z);
        if (player->loadedChunks.count(id) > 0) {
            if (m_lightManager) {
                m_lightManager->enableLightSources(ChunkPos(x, z), false);
            }
            sendUnloadChunk(x, z);
            player->loadedChunks.erase(id);
            spdlog::debug("Chunk ({}, {}) unloaded after grace window", x, z);
        }

        it = m_pendingChunkUnloads.erase(it);
    }
}

void IntegratedServer::initializeChunkLighting(ChunkCoord x, ChunkCoord z) {
    MC_TRACE_EVENT("server.lighting", "initializeChunkLighting",
               "Chunk", fmt::format("({}, {})", x, z));
    if (!m_lightManager || !m_chunkManager) {
        spdlog::warn("[LightInit] skip: lightManager={} chunkManager={} chunk=({}, {})",
                     static_cast<void*>(m_lightManager.get()),
                     static_cast<void*>(m_chunkManager.get()),
                     x,
                     z);
        return;
    }

    const ChunkData* chunk = m_chunkManager->getChunk(x, z);
    if (!chunk) {
        spdlog::warn("[LightInit] chunk not loaded for chunk=({}, {})", x, z);
        return;
    }

    const ChunkPos chunkPos(x, z);

    for (i32 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk->getSection(sectionY);
        const SectionPos sectionPos(x, sectionY, z);

        const bool isEmpty = (section == nullptr || section->isEmpty());
        m_lightManager->updateSectionStatus(sectionPos, isEmpty);

        if (section != nullptr) {
            if (m_lightManager->getSkyLightEngine()) {
                NibbleArray skyLightCopy = section->skyLightNibble().copy();
                m_lightManager->setData(LightType::SKY, sectionPos, skyLightCopy, false);
            }

            if (m_lightManager->getBlockLightEngine()) {
                NibbleArray blockLightCopy = section->blockLightNibble().copy();
                m_lightManager->setData(LightType::BLOCK, sectionPos, blockLightCopy, false);
            }
        }
    }

    m_lightManager->enableLightSources(chunkPos, true);
}

void IntegratedServer::tickLighting() {
    if (!m_lightManager || !m_lightManager->hasLightWork()) {
        return;
    }

    i32 remaining = m_lightManager->tick(32768, true, true);
    MC_TRACE_INSTANT("server.lighting", "tickLightingEnd", "Remaining", remaining);
    // spdlog::info("[LightTick] processed budget=512 remaining={}", remaining);
}

void IntegratedServer::onBlockStateChangedForLighting(i32 x,
                                                      i32 y,
                                                      i32 z,
                                                      i32 oldLightLevel,
                                                      i32 newLightLevel) {
    if (!m_lightManager) {
        spdlog::warn("[LightBlock] skip because lightManager is null for pos=({}, {}, {})", x, y, z);
        return;
    }

    const BlockPos pos(x, y, z);
    spdlog::info("[LightBlock] checkBlock pos=({}, {}, {}) oldLight={} newLight={}",
                  x, y, z, oldLightLevel, newLightLevel);
    m_lightManager->checkBlock(pos);

    if (newLightLevel > oldLightLevel) {
        spdlog::debug("[LightBlock] emission increased at ({}, {}, {}), level {} -> {}",
                      x, y, z, oldLightLevel, newLightLevel);
        m_lightManager->onBlockEmissionIncrease(pos, newLightLevel);
    }
}

void IntegratedServer::markLightChanged(LightType type, const SectionPos& pos) {
    MC_TRACE_EVENT("server.lighting", "markLightChanged",
               "Type", (type == LightType::SKY) ? "SKY" : "BLOCK",
               "Section", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z));

    if (!m_chunkManager) {
        spdlog::warn("[LightChanged] skip: chunkManager is null");
        return;
    }

    const char* typeName = (type == LightType::SKY) ? "SKY" : "BLOCK";

    ChunkData* chunk = m_chunkManager->getChunk(pos.x, pos.z);
    if (chunk) {
        chunk->setDirty(true);
    } else {
        spdlog::warn("[LightChanged] chunk not loaded for section=({}, {}, {})", pos.x, pos.y, pos.z);
    }

    syncLightDataToChunk(type, pos);
    broadcastLightUpdate(type, pos);
}

void IntegratedServer::syncLightDataToChunk(LightType type, const SectionPos& pos) {
    if (!m_lightManager || !m_chunkManager) {
        spdlog::warn("[LightSync] skip: lightManager={} chunkManager={} section=({}, {}, {})",
                     static_cast<void*>(m_lightManager.get()),
                     static_cast<void*>(m_chunkManager.get()),
                     pos.x,
                     pos.y,
                     pos.z);
        return;
    }

    const char* typeName = (type == LightType::SKY) ? "SKY" : "BLOCK";

    ChunkData* chunk = m_chunkManager->getChunk(pos.x, pos.z);
    if (!chunk) {
        spdlog::warn("[LightSync] chunk not loaded type={} section=({}, {}, {})",
                     typeName, pos.x, pos.y, pos.z);
        return;
    }

    const i32 sectionIndex = pos.y;
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        spdlog::warn("[LightSync] section index out of range: {} for section=({}, {}, {})",
                     sectionIndex, pos.x, pos.y, pos.z);
        return;
    }

    ChunkSection* section = chunk->getSection(sectionIndex);
    if (!section) {
        section = chunk->createSection(sectionIndex);
        if (!section) {
            spdlog::error("[LightSync] failed to create section index={} chunk=({}, {})",
                          sectionIndex, pos.x, pos.z);
            return;
        }
        spdlog::debug("[LightSync] created missing chunk section index={} chunk=({}, {})",
                      sectionIndex, pos.x, pos.z);
    }

    SWMRNibbleArray* lightArray = m_lightManager->getData(type, pos);
    if (!lightArray || lightArray->isNullUpdating()) {
        spdlog::debug("[LightSync] no light array, filling default type={} section=({}, {}, {})",
                      typeName, pos.x, pos.y, pos.z);
        if (type == LightType::SKY) {
            section->fillSkyLight(15);
        } else {
            section->fillBlockLight(0);
        }
        return;
    }

    std::vector<u8> data = lightArray->toByteArray();
    spdlog::trace("[LightSync] apply light array type={} section=({}, {}, {}) size={}",
                  typeName, pos.x, pos.y, pos.z, data.size());
    if (type == LightType::SKY) {
        NibbleArray& skyLight = section->skyLightNibble();
        if (data.empty()) {
            section->fillSkyLight(15);
        } else if (skyLight.data().size() == data.size()) {
            std::memcpy(skyLight.data().data(), data.data(), data.size());
        } else {
            section->skyLightNibble() = NibbleArray(data);
        }
    } else {
        NibbleArray& blockLight = section->blockLightNibble();
        if (data.empty()) {
            section->fillBlockLight(0);
        } else if (blockLight.data().size() == data.size()) {
            std::memcpy(blockLight.data().data(), data.data(), data.size());
        } else {
            section->blockLightNibble() = NibbleArray(data);
        }
    }
}

void IntegratedServer::broadcastLightUpdate(LightType type, const SectionPos& pos) {
    if (!m_chunkManager) {
        spdlog::warn("[LightBroadcast] skip: chunkManager is null");
        return;
    }

    const char* typeName = (type == LightType::SKY) ? "SKY" : "BLOCK";

    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        spdlog::trace("[LightBroadcast] skip: player missing or not logged in");
        return;
    }

    const ChunkId id(pos.x, pos.z);
    if (player->loadedChunks.count(id) == 0) {
        spdlog::trace("[LightBroadcast] skip: chunk not sent to player chunk=({}, {})", pos.x, pos.z);
        return;
    }

    const ChunkData* chunk = m_chunkManager->getChunk(pos.x, pos.z);
    if (!chunk) {
        spdlog::warn("[LightBroadcast] skip: chunk not loaded chunk=({}, {})", pos.x, pos.z);
        return;
    }

    const ChunkSection* section = chunk->getSection(pos.y);
    if (!section) {
        spdlog::trace("[LightBroadcast] skip: section missing section=({}, {}, {})", pos.x, pos.y, pos.z);
        return;
    }

    std::vector<u8> skyLightData;
    std::vector<u8> blockLightData;

    if (type == LightType::SKY) {
        const auto& skyLight = section->skyLightNibble();
        skyLightData = skyLight.data();
        if (skyLightData.empty()) {
            skyLightData.resize(NibbleArray::BYTE_SIZE, 0xFF);
        }
    }

    if (type == LightType::BLOCK) {
        const auto& blockLight = section->blockLightNibble();
        blockLightData = blockLight.data();
        if (blockLightData.empty()) {
            blockLightData.resize(NibbleArray::BYTE_SIZE, 0x00);
        }
    }

    network::LightUpdatePacket packet(
        pos.x,
        pos.z,
        pos.y,
        std::move(skyLightData),
        std::move(blockLightData),
        false);

    network::PacketSerializer payload;
    packet.serialize(payload);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::LightUpdate,
        payload.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
    spdlog::trace("[LightBroadcast] sent type={} section=({}, {}, {}) skyBytes={} blockBytes={}",
                  typeName, pos.x, pos.y, pos.z, skyLightData.size(), blockLightData.size());
}

void IntegratedServer::handleTeleportConfirm(const u8* data, size_t size) {
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        spdlog::warn("Teleport confirm received but client not logged in");
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse teleport confirm: {}", result.error().message());
        return;
    }

    auto& packet = result.value();
    spdlog::debug("Teleport confirm received: id={}, expected={}",
                 packet.teleportId(), player->pendingTeleportId);

    if (m_serverCore->confirmTeleport(m_clientPlayerId, packet.teleportId())) {
        // 传送确认后，触发区块加载
        m_chunkManager->processTicketUpdates();
    } else {
        spdlog::warn("Unexpected teleport confirm: id={}, expected={}",
                     packet.teleportId(), player->pendingTeleportId);
    }
}

void IntegratedServer::handleKeepAlive(const u8* data, size_t size) {
    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
        m_serverCore->keepAliveManager().handleKeepAliveResponse(m_clientPlayerId, packet.timestamp(), currentTimeMs);
        spdlog::trace("KeepAlive received: {}", packet.timestamp());
    }
}

void IntegratedServer::handleChatMessage(const u8* data, size_t size) {
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        return;
    }

    auto& packet = result.value();

    if (!packet.message().empty() && packet.message()[0] == '/') {
        auto& registry = command::CommandRegistry::getGlobal();
        // 从 ServerCore 获取 world 指针
        ServerWorld* world = m_serverCore ? m_serverCore->world() : nullptr;
        CoreCommandBridge bridge(world, m_serverCore.get());
        command::ServerCommandSource source(
            &bridge,
            nullptr,
            nullptr,
            Vector3d(player->x, player->y, player->z),
            Vector2f(player->yaw, player->pitch),
            4,
            m_clientPlayerId,
            player->username
        );

        auto commandResult = registry.execute(packet.message(), source);
        if (commandResult.failed()) {
            spdlog::warn("Integrated command '{}' failed: {}",
                         packet.message(), commandResult.error().toString());
        } else {
            spdlog::info("Integrated command '{}' executed with result {}",
                         packet.message(), commandResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, packet.message());
}

void IntegratedServer::sendLoginResponse(bool success, PlayerId playerId,
                                          const String& username, const String& message) {
    network::LoginResponsePacket response(success, playerId, username, message);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::LoginResponse, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendKeepAlive(u64 timestamp) {
    network::KeepAlivePacket packet;
    packet.setTimestamp(timestamp);

    auto result = packet.serialize();
    if (result.success()) {
        sendToClient(result.value().data(), result.value().size());
    }

    // 记录心跳发送
    if (m_serverCore) {
        m_serverCore->recordKeepAliveSent(m_clientPlayerId, timestamp);
    }
}

void IntegratedServer::sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId) {
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::Teleport, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId) {
    network::BlockUpdatePacket packet(x, y, z, blockStateId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::BlockUpdate, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendPlayerInventory() {
    PlayerInventoryPacket packet(m_clientData.inventory);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::PlayerInventory, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendContainerContent(const AbstractContainerMenu& menu) {
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::ContainerContent,
                   ContainerPacketHandler::createContentPacket(menu));
}

void IntegratedServer::sendOpenContainer(ContainerId containerId, ContainerType type, const String& title, i32 slotCount) {
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::OpenContainer,
                   ContainerPacketHandler::createOpenContainerPacket(containerId,
                                                                     ContainerTypes::toNetworkType(type),
                                                                     title,
                                                                     slotCount));
}

void IntegratedServer::sendCloseContainer(ContainerId containerId) {
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::CloseContainer,
                   CloseContainerPacket(containerId));
}

void IntegratedServer::sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
    MC_TRACE_EVENT("server.chunk", "sendChunkData",
               "Chunk", fmt::format("({}, {})", x, z),
               "DataSize", data.size());
    network::ChunkDataPacket packet(x, z, data);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::ChunkData, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendUnloadChunk(ChunkCoord x, ChunkCoord z) {
    network::UnloadChunkPacket packet(x, z);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::UnloadChunk, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendToClient(const u8* data, size_t size) {
    if (m_serverEndpoint && m_serverEndpoint->isConnected()) {
        m_serverEndpoint->send(data, size);
    }
}

void IntegratedServer::sendBlockBreakAnim(EntityId breakerId, i32 x, i32 y, i32 z, i8 stage) {
    network::BlockBreakAnimPacket packet;
    packet.setBreakerEntityId(breakerId);
    packet.setPosition(BlockPos(x, y, z));
    packet.setStage(stage);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize BlockBreakAnim packet: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::BlockBreakAnim, result.value());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendTimeUpdate() {
    if (!m_serverCore) return;
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) return;

    const auto& time = m_serverCore->gameTime();
    network::TimeUpdatePacket packet(
        time.gameTime(),
        time.dayTime(),
        time.daylightCycleEnabled()
    );

    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::TimeUpdate, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendWeatherUpdate() {
    if (!m_serverCore) return;
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) return;

    const auto& weather = m_serverCore->weatherManager();
    f32 rainStrength = weather.rainStrength();
    f32 thunderStrength = weather.thunderStrength();

    // 检查天气强度是否变化（使用阈值避免频繁发送）
    constexpr f32 STRENGTH_THRESHOLD = 0.001f;
    bool rainChanged = std::abs(rainStrength - m_lastSentRainStrength) > STRENGTH_THRESHOLD;
    bool thunderChanged = std::abs(thunderStrength - m_lastSentThunderStrength) > STRENGTH_THRESHOLD;

    // 发送降雨强度变化
    if (rainChanged) {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            sendToClient(fullPacket.data(), fullPacket.size());
        }
        m_lastSentRainStrength = rainStrength;
    }

    // 发送雷暴强度变化
    if (thunderChanged) {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            sendToClient(fullPacket.data(), fullPacket.size());
        }
        m_lastSentThunderStrength = thunderStrength;
    }

    // 发送天气状态变化（开始下雨/雨停）
    if (weather.hasWeatherChanged()) {
        auto weatherType = weather.weatherType();
        if (weatherType == weather::WeatherType::Clear) {
            // 雨停
            auto packet = network::GameStateChangePacket::endRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::GameStateChange, result.value());
                sendToClient(fullPacket.data(), fullPacket.size());
            }
        } else if (weatherType == weather::WeatherType::Rain ||
                   weatherType == weather::WeatherType::Thunder) {
            // 开始下雨
            auto packet = network::GameStateChangePacket::beginRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::GameStateChange, result.value());
                sendToClient(fullPacket.data(), fullPacket.size());
            }
        }
    }
}

void IntegratedServer::sendInitialWeatherState() {
    if (!m_serverCore) return;

    const auto& weather = m_serverCore->weatherManager();
    f32 rainStrength = weather.rainStrength();
    f32 thunderStrength = weather.thunderStrength();

    // 发送当前降雨强度
    {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            sendToClient(fullPacket.data(), fullPacket.size());
        }
    }

    // 发送当前雷暴强度
    {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            sendToClient(fullPacket.data(), fullPacket.size());
        }
    }

    // 更新上次发送的值
    m_lastSentRainStrength = rainStrength;
    m_lastSentThunderStrength = thunderStrength;
}

void IntegratedServer::openCraftingTableMenu() {
    auto* player = getPlayerData();
    if (!player) return;

    std::unique_ptr<AbstractContainerMenu> existingMenu;
    ContainerId previousId = 0;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        if (m_clientData.openMenu) {
            existingMenu = std::move(m_clientData.openMenu);
            previousId = existingMenu->getId();
        }
    }

    if (existingMenu) {
        Player& menuPlayer = getMenuPlayer();
        existingMenu->removed(menuPlayer);
        sendCloseContainer(previousId);
    }

    ContainerId containerId;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        containerId = m_clientData.nextContainerId++;
    }

    auto menu = std::make_unique<CraftingMenu>(containerId, &m_clientData.inventory, nullptr);
    menu->updateResult();

    sendOpenContainer(containerId,
                      ContainerType::CraftingTable,
                      String(ContainerTypes::getDefaultTitle(ContainerType::CraftingTable)),
                      menu->getSlotCount());
    sendContainerContent(*menu);

    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        m_clientData.openContainerType = ContainerType::CraftingTable;
        m_clientData.openMenu = std::move(menu);
    }
}

void IntegratedServer::handleSpawnedEntities(const std::vector<SpawnedEntityData>& entities) {
    if (entities.empty()) {
        return;
    }

    // 存储实体ID和类型用于发送网络包
    std::vector<std::pair<EntityId, const SpawnedEntityData*>> spawnedEntities;

    // 获取实体注册表
    auto& registry = entity::EntityRegistry::instance();

    for (const auto& entityData : entities) {
        // 获取实体类型
        const entity::EntityType* entityType = registry.getType(entityData.entityTypeId);
        if (!entityType) {
            spdlog::warn("IntegratedServer: Unknown entity type '{}' during chunk generation spawn",
                         entityData.entityTypeId);
            continue;
        }

        // 检查实体类型是否可以生成
        if (!entityType->canSummon()) {
            continue;
        }

        // 创建实体（使用 nullptr 作为世界，因为 IntegratedServer 使用 PhysicsEngine）
        std::unique_ptr<Entity> entity = entityType->create(nullptr);
        if (!entity) {
            continue;
        }

        // 设置实体位置
        entity->setPosition(Vector3(entityData.x, entityData.y, entityData.z));

        // 设置物理引擎（用于碰撞检测）
        if (m_physicsEngine) {
            entity->setPhysicsEngine(m_physicsEngine.get());
        }

        // 添加到实体管理器，获取分配的ID
        EntityId entityId = m_entityManager.addEntity(std::move(entity));

        spawnedEntities.emplace_back(entityId, &entityData);
    }

    // 发送实体生成包到客户端
    sendEntitySpawnPackets(spawnedEntities);
}

void IntegratedServer::sendEntitySpawnPackets(const std::vector<std::pair<EntityId, const SpawnedEntityData*>>& entities) {
    for (const auto& [entityId, entityData] : entities) {
        // 发送 SpawnMobPacket 或 SpawnEntityPacket
        // 对于动物（LivingEntity），使用 SpawnMobPacket
        // 对于其他实体，使用 SpawnEntityPacket

        // 判断是否是生物
        // 这里简化处理：所有动物都是生物
        bool isMob = (entityData->entityTypeId == "minecraft:pig" ||
                      entityData->entityTypeId == "minecraft:cow" ||
                      entityData->entityTypeId == "minecraft:sheep" ||
                      entityData->entityTypeId == "minecraft:chicken");

        if (isMob) {
            network::SpawnMobPacket packet;
            packet.setEntityId(static_cast<u32>(entityId));
            packet.setEntityTypeId(entityData->entityTypeId);
            packet.setPosition(entityData->x, entityData->y, entityData->z);
            packet.setRotation(0.0f, 0.0f, 0.0f);  // yaw, pitch, headYaw
            packet.setVelocity(0, 0, 0);

            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::SpawnMob, result.value());
                sendToClient(fullPacket.data(), fullPacket.size());
                // spdlog::info("IntegratedServer: Sent SpawnMob packet for {} (ID: {}) at ({:.1f}, {:.1f}, {:.1f})",
                //              entityData->entityTypeId, entityId, entityData->x, entityData->y, entityData->z);
            }
        } else {
            network::SpawnEntityPacket packet;
            packet.setEntityId(static_cast<u32>(entityId));
            packet.setEntityTypeId(entityData->entityTypeId);
            packet.setPosition(entityData->x, entityData->y, entityData->z);
            packet.setRotation(0.0f, 0.0f);
            packet.setVelocity(0, 0, 0);

            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::SpawnEntity, result.value());
                sendToClient(fullPacket.data(), fullPacket.size());
                spdlog::info("IntegratedServer: Sent SpawnEntity packet for {} (ID: {}) at ({:.1f}, {:.1f}, {:.1f})",
                             entityData->entityTypeId, entityId, entityData->x, entityData->y, entityData->z);
            }
        }
    }
}

void IntegratedServer::syncEntityPositions() {
    // 遍历所有实体，检查位置变化并发送更新
    m_entityManager.forEachEntity([this](Entity* entityPtr) {
        if (!entityPtr) return true;  // 继续遍历

        Entity& entity = *entityPtr;
        EntityId entityId = entity.id();
        Vector3 currentPos = entity.position();
        f32 currentYaw = entity.yaw();
        f32 currentPitch = entity.pitch();

        // 查找或创建追踪数据
        auto it = m_entityTrackData.find(entityId);
        if (it == m_entityTrackData.end()) {
            // 新实体，记录位置但不发送更新（spawn时已发送）
            m_entityTrackData[entityId] = {currentPos, currentYaw, currentPitch, false};
            return true;  // 继续遍历
        }

        auto& trackData = it->second;

        // 检查位置变化是否超过阈值
        constexpr f32 POSITION_THRESHOLD = 0.1f;  // 位置变化阈值
        constexpr f32 ROTATION_THRESHOLD = 1.0f;  // 旋转变化阈值（度）

        Vector3 delta = currentPos - trackData.lastPosition;
        bool positionChanged = delta.lengthSquared() > (POSITION_THRESHOLD * POSITION_THRESHOLD);
        bool rotationChanged = std::abs(currentYaw - trackData.lastYaw) > ROTATION_THRESHOLD ||
                               std::abs(currentPitch - trackData.lastPitch) > ROTATION_THRESHOLD;

        if (positionChanged || rotationChanged || trackData.needsFullUpdate) {
            // 发送EntityTeleport包
            network::EntityTeleportPacket packet;
            packet.setEntityId(static_cast<u32>(entityId));
            packet.setPosition(currentPos.x, currentPos.y, currentPos.z);
            packet.setRotation(currentYaw, currentPitch);
            packet.setOnGround(entity.onGround());

            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::EntityTeleport, result.value());
                sendToClient(fullPacket.data(), fullPacket.size());
            }

            // 更新追踪数据
            trackData.lastPosition = currentPos;
            trackData.lastYaw = currentYaw;
            trackData.lastPitch = currentPitch;
            trackData.needsFullUpdate = false;
        }

        return true;  // 继续遍历
    });

    // 清理已移除实体的追踪数据
    std::vector<EntityId> toRemove;
    for (const auto& [entityId, _] : m_entityTrackData) {
        if (!m_entityManager.hasEntity(entityId)) {
            toRemove.push_back(entityId);
        }
    }
    for (EntityId id : toRemove) {
        m_entityTrackData.erase(id);
    }
}

// ============================================================================
// 挖掘进度追踪
// ============================================================================

void IntegratedServer::updatePlayerMining(const BlockPos& pos,
                                          network::BlockInteractionAction action) {
    MC_TRACE_EVENT("server.world.mining", "updatePlayerMining",
                "Action", static_cast<i32>(action),
                "Position", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
                "Active", m_playerMining.active,
                "Progress", m_playerMining.progress,
                "Stage", m_playerMining.lastStage);
    // spdlog::info("[MiningState] action={} pos=({}, {}, {}) active={} progress={:.3f} stage={}",
    //               static_cast<i32>(action),
    //               pos.x,
    //               pos.y,
    //               pos.z,
    //               m_playerMining.active,
    //               m_playerMining.progress,
    //               m_playerMining.lastStage);

    switch (action) {
        case network::BlockInteractionAction::StartDestroyBlock:
            // 开始挖掘
            m_playerMining.position = pos;
            m_playerMining.progress = 0.0f;
            m_playerMining.lastStage = 255;  // 255 表示未广播过
            m_playerMining.active = true;
            m_playerMining.startTick = tickCount();
            spdlog::debug("[MiningState] start mining at ({}, {}, {}), startTick={}",
                          pos.x, pos.y, pos.z, m_playerMining.startTick);
            break;

        case network::BlockInteractionAction::AbortDestroyBlock:
            // 中止挖掘
            if (m_playerMining.active) {
                // 发送移除破坏效果的包
                sendBlockBreakAnim(m_clientPlayerId,
                                   m_playerMining.position.x,
                                   m_playerMining.position.y,
                                   m_playerMining.position.z,
                                   -1);  // -1 表示移除
                m_playerMining.active = false;
                m_playerMining.progress = 0.0f;
                m_playerMining.lastStage = 255;
                spdlog::debug("[MiningState] abort mining reset state");
            }
            break;

        case network::BlockInteractionAction::StopDestroyBlock:
            // 完成挖掘
            if (m_playerMining.active) {
                // 发送移除破坏效果的包
                sendBlockBreakAnim(m_clientPlayerId,
                                   m_playerMining.position.x,
                                   m_playerMining.position.y,
                                   m_playerMining.position.z,
                                   -1);
                m_playerMining.active = false;
                m_playerMining.progress = 0.0f;
                m_playerMining.lastStage = 255;
                spdlog::debug("[MiningState] stop mining reset state");
            }
            break;
    }
}

void IntegratedServer::tickPlayerMining() {
    MC_TRACE_EVENT("server.world.mining", "tickPlayerMining");
    if (!m_playerMining.active) {
        return;
    }

    // 检查方块是否仍然存在
    ChunkCoord chunkX = world::toChunkCoord(m_playerMining.position.x);
    ChunkCoord chunkZ = world::toChunkCoord(m_playerMining.position.z);
    const ChunkData* chunk = m_chunkManager ? m_chunkManager->getChunk(chunkX, chunkZ) : nullptr;

    if (!chunk) {
        // 区块未加载，中止挖掘
        spdlog::warn("[MiningTick] cancel because chunk not loaded at chunk=({}, {})", chunkX, chunkZ);
        m_playerMining.active = false;
        return;
    }

    const i32 localX = world::toLocalCoord(m_playerMining.position.x);
    const i32 localZ = world::toLocalCoord(m_playerMining.position.z);
    const BlockState* state = chunk->getBlock(localX, m_playerMining.position.y, localZ);

    if (!state || state->isAir()) {
        // 方块已被破坏或不存在，停止挖掘
        spdlog::debug("[MiningTick] stop because target block missing/air at ({}, {}, {})",
                      m_playerMining.position.x, m_playerMining.position.y, m_playerMining.position.z);
        m_playerMining.active = false;
        return;
    }

    // 获取玩家手持物品以计算挖掘速度
    ItemStack heldItemCopy;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        heldItemCopy = m_clientData.inventory.getSelectedStack();
    }
    const ItemStack* heldItem = heldItemCopy.isEmpty() ? nullptr : &heldItemCopy;

    // 计算挖掘速度（简化版本，后续可以扩展为完整的挖掘速度计算）
    f32 hardness = state->hardness();
    if (hardness < 0.0f) {
        // 不可破坏方块
        spdlog::debug("[MiningTick] unbreakable block stateId={} at ({}, {}, {})",
                      state->stateId(),
                      m_playerMining.position.x,
                      m_playerMining.position.y,
                      m_playerMining.position.z);
        return;
    }

    // 获取挖掘速度倍率
    f32 miningSpeed = 1.0f;
    if (heldItem && heldItem->getItem()) {
        miningSpeed = heldItem->getDestroySpeed(*state);
    }

    // 计算每 tick 的进度增量
    // 参考 MC 1.16.5: damage = miningSpeed / hardness / 30 (如果没有工具则 / 100)
    f32 damagePerTick;
    if (miningSpeed > 1.0f) {
        damagePerTick = miningSpeed / hardness / 30.0f;
    } else {
        damagePerTick = miningSpeed / hardness / 100.0f;
    }

    // 创造模式瞬间破坏
    auto* player = getPlayerData();
    if (player && player->gameMode == GameMode::Creative) {
        damagePerTick = 1.0f;
    }

    // 更新进度
    m_playerMining.progress += damagePerTick;
    spdlog::trace("[MiningTick] stateId={} hardness={:.3f} speed={:.3f} damagePerTick={:.5f} progress={:.3f}",
                  state->stateId(), hardness, miningSpeed, damagePerTick, m_playerMining.progress);

    // 广播破坏进度
    broadcastMiningProgress();
}

void IntegratedServer::broadcastMiningProgress() {
    if (!m_playerMining.active) {
        return;
    }

    // 计算破坏阶段 (0-9)
    u8 stage = static_cast<u8>(std::min(9.0f, m_playerMining.progress * 10.0f));

    // 仅在阶段变化时广播
    if (stage != m_playerMining.lastStage) {
        sendBlockBreakAnim(m_clientPlayerId,
                           m_playerMining.position.x,
                           m_playerMining.position.y,
                           m_playerMining.position.z,
                           static_cast<i8>(stage));
        m_playerMining.lastStage = stage;
    }
}

} // namespace mc::server
