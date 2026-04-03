#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "../../common/world/chunk/ChunkData.hpp"
#include "../../common/world/biome/Biome.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../renderer/MeshTypes.hpp"
#include "../../common/network/sync/ChunkSync.hpp"
#include "../../common/physics/PhysicsEngine.hpp"
#include "../renderer/Camera.hpp"
#include "../renderer/mesh/MeshWorkerPool.hpp"
#include "entity/ClientEntityManager.hpp"
#include "ClientWeather.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <queue>
#include <functional>

namespace mc::client {

/**
 * @brief 客户端区块数据
 */
struct ClientChunk {
    ChunkId chunkId;
    std::shared_ptr<ChunkData> data;  // 使用 shared_ptr 以支持异步网格构建
    MeshData solidMesh;       // 实心方块网格
    MeshData transparentMesh;  // 透明方块网格
    bool needsMeshUpdate = true;
    bool isGenerating = false;
    bool isLoaded = false;
    bool meshBuilding = false;  // 是否正在异步构建网格
    bool needsNeighborRebuild = false;  // 是否需要因邻居加载而重建网格
    u64 activeMeshTaskId = 0;  // 当前有效的网格构建任务 ID
};

/**
 * @brief 区块加载请求
 */
struct ChunkLoadRequest {
    ChunkId chunkId;
    world::ChunkLoadPriority priority;
    bool operator<(const ChunkLoadRequest& other) const {
        return static_cast<i32>(priority) > static_cast<i32>(other.priority);
    }
};

/**
 * @brief 客户端世界管理器
 *
 * 管理区块的加载、卸载和渲染。
 * 实现 ICollisionWorld 接口以支持物理碰撞检测。
 */
class ClientWorld : public ICollisionWorld {
public:
    ClientWorld();
    ~ClientWorld() override;

    // 禁止拷贝
    ClientWorld(const ClientWorld&) = delete;
    ClientWorld& operator=(const ClientWorld&) = delete;

    /**
     * @brief 初始化世界
     */
    [[nodiscard]] Result<void> initialize(u64 seed = 0);

    /**
     * @brief 清理世界
     */
    void destroy();

    /**
     * @brief 更新世界（每帧调用）
     * @param cameraPosition 相机位置
     * @param renderDistance 渲染距离（区块数）
     */
    void update(const glm::vec3& cameraPosition, i32 renderDistance);

    /**
     * @brief 获取区块
     */
    [[nodiscard]] ClientChunk* getChunk(const ChunkId& id);
    [[nodiscard]] const ClientChunk* getChunk(const ChunkId& id) const;

    /**
     * @brief 获取方块
     */
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 获取方块所在生物群系
     */
    [[nodiscard]] const Biome* getBiomeAtBlock(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取天空光照值 (0-15)
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @return 天空光照值，如果区块未加载返回15
     */
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取方块光照值 (0-15)
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @return 方块光照值，如果区块未加载返回0
     */
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;

    /**
     * @brief 设置方块
     */
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state);

    // ========== ICollisionWorld 接口实现 ==========

    /**
     * @brief 检查位置是否在世界范围内
     */
    [[nodiscard]] bool isWithinWorldBounds(i32 /*x*/, i32 y, i32 /*z*/) const override {
        return y >= m_minBuildHeight && y < m_maxBuildHeight;
    }

    /**
     * @brief 获取区块数据
     */
    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const override;

    /**
     * @brief 获取世界最小Y坐标
     */
    [[nodiscard]] i32 getMinBuildHeight() const override { return m_minBuildHeight; }

    /**
     * @brief 获取世界最大Y坐标
     */
    [[nodiscard]] i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }

    // ========== 区块管理 ==========

    /**
     * @brief 获取所有已加载的区块
     */
    void forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func);

    /**
     * @brief 获取需要更新网格的区块
     */
    void forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func);

    /**
     * @brief 获取区块数量
     */
    [[nodiscard]] size_t chunkCount() const { return m_chunks.size(); }

    /**
     * @brief 获取渲染距离内的区块坐标
     */
    void getChunksInRange(const glm::vec3& position, i32 range,
                          std::vector<ChunkId>& outChunks) const;

    /**
     * @brief 强制加载区块
     */
    void loadChunk(const ChunkId& id);

    /**
     * @brief 强制卸载区块
     * @param id 区块 ID
     * @param outUnloadedChunkIds 如果提供，将被填充为实际卸载的区块 ID 列表
     */
    void unloadChunk(const ChunkId& id, std::vector<ChunkId>* outUnloadedChunkIds = nullptr);

    /**
     * @brief 设置区块卸载回调
     *
     * 当区块从世界中卸载时调用此回调，用于通知外部系统（如 ChunkRenderer）释放资源。
     *
     * @param callback 回调函数，参数为被卸载的区块 ID
     */
    void setChunkUnloadCallback(std::function<void(const ChunkId&)> callback) {
        m_chunkUnloadCallback = std::move(callback);
    }

    /**
     * @brief 重新生成区块网格
     */
    void rebuildChunkMesh(const ChunkId& id);

    /**
     * @brief 标记区块需要网格更新
     */
    void markChunkDirty(const ChunkId& id);

    /**
     * @brief 设置渲染距离
     */
    void setRenderDistance(i32 distance) { m_renderDistance = distance; }
    [[nodiscard]] i32 renderDistance() const { return m_renderDistance; }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 seed() const { return m_seed; }

    /**
     * @brief 接收服务端区块数据
     */
    void onChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8>&& data);

    /**
     * @brief 卸载区块（服务端通知）
     */
    void onChunkUnload(ChunkCoord x, ChunkCoord z);

    // ========== 时间管理 ==========

    /**
     * @brief 处理服务端时间更新
     * @param gameTime 游戏总 tick 数
     * @param dayTime 一天内的时间 (0-23999)
     * @param daylightCycleEnabled 日光周期是否启用
     */
    void onTimeUpdate(i64 gameTime, i64 dayTime, bool daylightCycleEnabled);

    /**
     * @brief 获取插值后的天体角度 (用于平滑渲染)
     * @param partialTick 部分 tick (0.0-1.0)
     * @return 天体角度 (0.0-1.0)
     */
    [[nodiscard]] f32 getInterpolatedCelestialAngle(f32 partialTick) const;

    /**
     * @brief 获取当前一天内的时间
     */
    [[nodiscard]] i64 dayTime() const { return m_dayTime; }

    /**
     * @brief 获取游戏总 tick 数
     */
    [[nodiscard]] i64 gameTime() const { return m_gameTime; }

    /**
     * @brief 获取日光周期是否启用
     */
    [[nodiscard]] bool daylightCycleEnabled() const { return m_daylightCycleEnabled; }

    // ========== 网格构建线程池 ==========

    /**
     * @brief 初始化网格构建线程池
     * @param threadCount 线程数量（-1 表示自动检测）
     */
    void initializeMeshWorkerPool(i32 threadCount = -1);

    /**
     * @brief 关闭网格构建线程池
     */
    void shutdownMeshWorkerPool();

    /**
     * @brief 处理已完成的网格构建结果
     *
     * 在主线程每帧调用，处理 Worker 线程完成的结果。
     * 使用 maxPerFrame 限制每帧处理数量，避免帧卡顿。
     *
     * @param maxPerFrame 每帧最多处理数量（默认 4）
     */
    void processMeshBuildResults(u32 maxPerFrame = 4);

    /**
     * @brief 获取网格构建线程池（只读访问）
     */
    [[nodiscard]] const MeshWorkerPool* meshWorkerPool() const { return m_meshWorkerPool.get(); }

    // ========== 实体管理 ==========

    /**
     * @brief 获取实体管理器
     */
    [[nodiscard]] ClientEntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const ClientEntityManager& entityManager() const { return m_entityManager; }

    // ========== 天气管理 ==========

    /**
     * @brief 获取客户端天气状态
     */
    [[nodiscard]] ClientWeather& weather() { return m_weather; }
    [[nodiscard]] const ClientWeather& weather() const { return m_weather; }

    /**
     * @brief 处理服务端天气同步 - 降雨强度变化
     */
    void onRainStrengthChange(f32 strength);

    /**
     * @brief 处理服务端天气同步 - 雷暴强度变化
     */
    void onThunderStrengthChange(f32 strength);

    /**
     * @brief 处理服务端天气同步 - 开始下雨
     */
    void onBeginRaining();

    /**
     * @brief 处理服务端天气同步 - 雨停
     */
    void onEndRaining();

    // ========== 光照更新 ==========

    /**
     * @brief 处理服务端光照更新
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param sectionY 区块段Y坐标
     * @param skyLight 天空光照数据 (NibbleArray格式的原始字节)
     * @param blockLight 方块光照数据 (NibbleArray格式的原始字节)
     * @param trustEdges 是否信任边缘光照
     */
    void onLightUpdate(i32 chunkX, i32 chunkZ, i32 sectionY,
                       const std::vector<u8>& skyLight,
                       const std::vector<u8>& blockLight,
                       bool trustEdges = false);

private:
    // 区块卸载回调
    std::function<void(const ChunkId&)> m_chunkUnloadCallback;

    // 区块加载/卸载
    void loadChunksInRange(const glm::vec3& position, i32 range);
    void unloadChunksOutOfRange(const glm::vec3& position, i32 range);
    void processLoadQueue();

    // 区块生成
    void generateChunk(ClientChunk& chunk);
    void rebuildMesh(ClientChunk& chunk);
    void scheduleChunkMeshRebuild(const ChunkId& id);

    // 异步重建区块网格（使用线程池）
    void scheduleChunkMeshRebuildAsync(const ChunkId& id, i32 priority = static_cast<i32>(world::ChunkLoadPriority::Normal));

    // 通知已存在的邻居区块重建网格（在新区块加载后调用）
    void scheduleNeighborMeshRebuild(const ChunkId& id);

    // 获取相邻区块
    void getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6]);

    // 获取相邻区块数据（shared_ptr 版本，用于异步网格构建）
    std::array<std::shared_ptr<const ChunkData>, 6> getNeighborChunkData(const ChunkId& id);

    // 计算区块优先级
    world::ChunkLoadPriority calculatePriority(const ChunkId& id, const glm::vec3& cameraPos) const;

private:
    std::unordered_map<ChunkId, std::unique_ptr<ClientChunk>> m_chunks;

    // 加载队列
    std::priority_queue<ChunkLoadRequest> m_loadQueue;
    std::unordered_set<ChunkId> m_queuedChunks;

    // 网格构建线程池
    std::unique_ptr<MeshWorkerPool> m_meshWorkerPool;

    // 配置
    i32 m_renderDistance = 12;
    i32 m_maxChunksPerFrame = 4;  // 每帧最多加载的区块数
    u64 m_seed = 0;

    // 相机位置（用于优先级计算）
    glm::vec3 m_cameraPosition{0.0f, 0.0f, 0.0f};

    // 世界边界
    i32 m_minBuildHeight = 0;
    i32 m_maxBuildHeight = 256;

    // 统计
    u32 m_chunksLoaded = 0;
    u32 m_chunksUnloaded = 0;

    // 网格任务代际计数器（避免旧任务结果覆盖新状态）
    u64 m_nextMeshTaskId = 1;

    // 时间 (从服务端同步)
    i64 m_gameTime = 0;
    i64 m_dayTime = 0;
    i64 m_prevDayTime = 0;       // 上一帧的 dayTime (用于插值)
    i64 m_targetDayTime = 0;     // 目标 dayTime (从服务端接收)
    bool m_daylightCycleEnabled = true;

    // 实体管理器
    ClientEntityManager m_entityManager;

    // 客户端天气状态
    ClientWeather m_weather;

    bool m_destroyed = false;
};

} // namespace mc::client
