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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"

// macOS系统头文件<mach/arm/vm_param.h>定义了BYTE_SIZE宏(=8)，
// 与NibbleArray::BYTE_SIZE静态常量冲突，在此push/undef屏蔽。
// 不在include后pop_macro，确保后续代码不受影响。
#ifdef __APPLE__
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE
#endif
#include "common/profiler/MemoryTracking.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include <array>
#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 区块数据
// ============================================================================

class ChunkData : public IChunk {
public:
    ChunkData();
    ChunkData(ChunkCoord x, ChunkCoord z);
    ~ChunkData() override;

    // 禁止拷贝
    ChunkData(const ChunkData&) = delete;
    ChunkData& operator=(const ChunkData&) = delete;

    // 允许移动（显式实现：m_lightCorrect 为 atomic 不可默认移动，须 load/store）
    ChunkData(ChunkData&& other) noexcept;
    ChunkData& operator=(ChunkData&& other) noexcept;

    // 位置 (IChunk 接口)
    [[nodiscard]] ChunkCoord x() const override { return m_x; }
    [[nodiscard]] ChunkCoord z() const override { return m_z; }
    [[nodiscard]] ChunkPos pos() const override { return ChunkPos(m_x, m_z); }

    // 方块访问 (IChunk 接口 - 使用 BlockState 指针)
    [[nodiscard]] const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

    // 方块访问 (IChunk 接口 - 使用状态ID，更高效)
    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) override;

    // 高度图
    [[nodiscard]] BlockCoord getHighestBlock(BlockCoord x, BlockCoord z) const;
    void updateHeightMap(BlockCoord x, BlockCoord z);

    // 生物群系 (IChunk 接口)
    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    [[nodiscard]] const BiomeContainer& getBiomes() const { return m_biomes; }
    [[nodiscard]] BiomeContainer& getBiomes() { return m_biomes; }
    void setBiomes(BiomeContainer biomes) { m_biomes = std::move(biomes); }

    // 区块段访问 (IChunk 接口)
    [[nodiscard]] ChunkSection* getSection(i32 index) override;
    [[nodiscard]] const ChunkSection* getSection(i32 index) const override;
    [[nodiscard]] bool hasSection(i32 index) const override;
    ChunkSection* createSection(i32 index) override;

    // 获取所有区块段（用于光照引擎缓存）
    [[nodiscard]] const ChunkSection* const* getSections() const override;

    // 高度图 (IChunk 接口)
    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override;
    void updateHeightmap(
        HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

    // 高度图原始值（getFirstAvailable 语义）：最高方块 Y+1 或 NO_BLOCK_SENTINEL（空列）。
    // 未初始化的类型回退到 WorldSurface 槽位（与 getTopBlockY 一致），供需精确识别空列的
    // 调用方使用。
    [[nodiscard]] BlockCoord getHeightmapFirstAvailable(HeightmapType type, BlockCoord x, BlockCoord z) const override;

    /**
     * @brief 从存档数据直接恢复指定类型的高度图（绕过 _isOpaque 判定）
     *
     * 用于 Java/Bedrock 存档读取、原生 serialize/deserialize 等场景，将存档中
     * 已持久化的高度值整列写回 m_heightmaps[type]，并标记该槽位为已初始化。
     *
     * 注意：data 必须按 Heightmap 内部存储顺序（z-major：index = z * CHUNK_WIDTH + x）
     * 排列，且每个元素为 Heightmap 内部存储语义（最高方块 Y+1，0 表示无方块）。
     *
     * @param type 高度图类型
     * @param data 高度数据数组（大小必须为 Heightmap::SIZE）
     */
    void setHeightmapFromStorage(HeightmapType type, const std::array<BlockCoord, Heightmap::SIZE>& data);

    /**
     * @brief 读取指定类型高度图的原始存储数据（Y+1 或 NO_BLOCK_SENTINEL）
     *
     * 与 getTopBlockY 不同，此方法返回 Heightmap 内部存储语义（最高方块 Y+1，
     * NO_BLOCK_SENTINEL 表示无方块），不做"无方块与 MIN_BUILD_HEIGHT 处有方块"的歧义合并，
     * 供序列化/网络同步无损还原使用。未初始化的类型返回其当前槽位内容（通常为哨兵）。
     */
    [[nodiscard]] const std::array<BlockCoord, Heightmap::SIZE>& getHeightmapData(HeightmapType type) const
    {
        return m_heightmaps[static_cast<size_t>(type)].getData();
    }

    /**
     * @brief 查询指定类型高度图槽位是否已被显式填充
     *
     * 序列化侧据此决定是否写该类型（未初始化的类型无有效数据，跳过以节省带宽）。
     */
    [[nodiscard]] bool isHeightmapInitialized(HeightmapType type) const
    {
        return m_heightmapInitialized[static_cast<size_t>(type)];
    }

    // 区块状态 (IChunk 接口)
    [[nodiscard]] ChunkLoadStatus getStatus() const override { return m_status; }
    void setStatus(ChunkLoadStatus status) override { m_status = status; }

    // 修改标记 (IChunk 接口)
    [[nodiscard]] bool isModified() const override { return m_dirty; }
    void setModified(bool modified) override { m_dirty = modified; }

    // 其他状态
    [[nodiscard]] bool isFullyGenerated() const { return m_fullyGenerated; }
    void setFullyGenerated(bool value) { m_fullyGenerated = value; }

    [[nodiscard]] bool isDirty() const { return m_dirty; }
    void setDirty(bool value) { m_dirty = value; }

    [[nodiscard]] bool isLoaded() const { return m_loaded; }
    void setLoaded(bool value) { m_loaded = value; }

    // 居住时间（区块内有玩家附近时的累计刻数，用于计算区域难度）
    [[nodiscard]] i64 inhabitedTime() const { return m_inhabitedTime; }
    void setInhabitedTime(i64 value) { m_inhabitedTime = value; }
    void incrementInhabitedTime(i64 ticks) { m_inhabitedTime += ticks; }

    // 序列化
    [[nodiscard]] std::vector<u8> serialize() const;
    [[nodiscard]] static Result<std::unique_ptr<ChunkData>> deserialize(const u8* data, size_t size);

    // 填充
    void fill(BlockCoord minY, BlockCoord maxY, u32 stateId);

    // 光照访问
    [[nodiscard]] u8 getSkyLight(BlockCoord x, BlockCoord y, BlockCoord z) const;
    void setSkyLight(BlockCoord x, BlockCoord y, BlockCoord z, u8 light);
    [[nodiscard]] u8 getBlockLight(BlockCoord x, BlockCoord y, BlockCoord z) const;
    void setBlockLight(BlockCoord x, BlockCoord y, BlockCoord z, u8 light);

    // ========================================================================
    // Starlight 光照数据接口 (IChunk 接口实现)
    // ========================================================================

    // 光照段数量（包含上下缓冲区）
    static constexpr i32 LIGHT_SECTIONS = mc::world::CHUNK_SECTIONS + 2; // CHUNK_SECTIONS + 2 缓冲段

    /**
     * @brief 获取天空光照空映射
     */
    [[nodiscard]] const bool* getSkyEmptinessMap() const override;

    /**
     * @brief 设置天空光照空映射
     */
    void setSkyEmptinessMap(const bool* map) override;

    /**
     * @brief 获取方块光照空映射
     */
    [[nodiscard]] const bool* getBlockEmptinessMap() const override;

    /**
     * @brief 设置方块光照空映射
     */
    void setBlockEmptinessMap(const bool* map) override;

    /**
     * @brief 获取天空光照 Nibble 数组
     */
    [[nodiscard]] SWMRNibbleArray* const* getSkyNibbles() const override;

    /**
     * @brief 设置天空光照 Nibble 数组
     */
    void setSkyNibbles(SWMRNibbleArray* const* nibbles) override;

    /**
     * @brief 获取方块光照 Nibble 数组
     */
    [[nodiscard]] SWMRNibbleArray* const* getBlockNibbles() const override;

    /**
     * @brief 设置方块光照 Nibble 数组
     */
    void setBlockNibbles(SWMRNibbleArray* const* nibbles) override;

    /**
     * @brief 检查区块光照是否正确
     *
     * ③-2b：m_lightCorrect 改 atomic——worker（ChunkLoadLightTask/LIGHT 生成阶段）写、
     * 主线程不再读（initializeChunkLighting 已搬 worker），但 SkyLightEngine::canTickChunk
     * 在 worker 读。atomic 防御未来主线程读路径，IChunk 虚签名不变（仍 bool）。
     */
    [[nodiscard]] bool isLightCorrect() const override { return m_lightCorrect.load(std::memory_order::acquire); }

    /**
     * @brief 设置区块光照正确状态
     */
    void setLightCorrect(bool correct) override { m_lightCorrect.store(correct, std::memory_order::release); }

    // Starlight 光照数据存储（内部使用）
    /**
     * @brief 初始化光照数据
     */
    void initLightData();

    /**
     * @brief 获取可变的天空光照 Nibble 数组
     */
    [[nodiscard]] std::array<SWMRNibbleArray, LIGHT_SECTIONS>& skyNibbles() { return m_skyNibbles; }
    [[nodiscard]] const std::array<SWMRNibbleArray, LIGHT_SECTIONS>& skyNibbles() const { return m_skyNibbles; }

    /**
     * @brief 获取可变的方块光照 Nibble 数组
     */
    [[nodiscard]] std::array<SWMRNibbleArray, LIGHT_SECTIONS>& blockNibbles() { return m_blockNibbles; }
    [[nodiscard]] const std::array<SWMRNibbleArray, LIGHT_SECTIONS>& blockNibbles() const { return m_blockNibbles; }

    // ========================================================================
    // 方块实体管理
    // ========================================================================

    /**
     * @brief 获取指定位置的方块实体
     * @param pos 方块位置
     * @return 方块实体指针，如果不存在返回 nullptr
     */
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos);

    /**
     * @brief 获取指定位置的方块实体（const 版本）
     * @param pos 方块位置
     * @return 方块实体指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const;

    /**
     * @brief 设置指定位置的方块实体
     * @param pos 方块位置
     * @param entity 方块实体（转移所有权）
     * @return 之前在该位置的方块实体，如果没有则返回 nullptr
     *
     * 如果位置不在当前区块内，返回传入的 entity。
     * 如果该位置已有方块实体，则替换并返回旧的方块实体。
     */
    std::unique_ptr<BlockEntity> setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity);

    /**
     * @brief 移除指定位置的方块实体
     * @param pos 方块位置
     * @return 被移除的方块实体，如果不存在则返回 nullptr
     */
    std::unique_ptr<BlockEntity> removeBlockEntity(const BlockPos& pos);

    /**
     * @brief 检查指定位置是否有方块实体
     * @param pos 方块位置
     * @return 如果有方块实体返回 true
     */
    [[nodiscard]] bool hasBlockEntity(const BlockPos& pos) const;

    /**
     * @brief 获取所有方块实体
     * @return 方块实体指针列表
     */
    [[nodiscard]] std::vector<BlockEntity*> getAllBlockEntities();

    /**
     * @brief 获取所有方块实体（const 版本）
     * @return 方块实体指针列表
     */
    [[nodiscard]] std::vector<const BlockEntity*> getAllBlockEntities() const;

    /**
     * @brief 获取方块实体数量
     * @return 当前区块内的方块实体数量
     */
    [[nodiscard]] size_t blockEntityCount() const;

    // ========================================================================
    // 已加载实体承载
    // ========================================================================

    void addLoadedEntity(std::unique_ptr<Entity> entity);
    [[nodiscard]] std::vector<std::unique_ptr<Entity>>& loadedEntities() { return m_loadedEntities; }
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& loadedEntities() const { return m_loadedEntities; }
    [[nodiscard]] bool hasLoadedEntities() const { return !m_loadedEntities.empty(); }
    [[nodiscard]] size_t loadedEntityCount() const { return m_loadedEntities.size(); }
    std::vector<std::unique_ptr<Entity>> takeLoadedEntities();

    // ---- 已加载实体 NBT（Java 存档路径）----
    // Java 区块里的实体在 JavaColumnReader::_readEntities 阶段仅以原始 NBT 形式暂存于此，
    // 不立即反序列化——反序列化需要所在维度的 ecs::EntityRegistry（Entity 构造时即 attach
    // 高频组件，entt 实体不可跨 registry 迁移），而 storage 层不持有 world。故推迟到
    // ServerWorld::onChunkLoaded（持有 *entityRegistry()）spawn 点再反序列化并注入世界。
    void addLoadedEntityNbt(std::unique_ptr<nbt::tags::compound_tag> tag);
    [[nodiscard]] bool hasLoadedEntityNbt() const { return !m_loadedEntityNbt.empty(); }
    std::vector<std::unique_ptr<nbt::tags::compound_tag>> takeLoadedEntityNbt();

    // ========================================================================
    // 后处理位置
    // ========================================================================

    /**
     * @brief 获取后处理位置（按区块段索引）
     *
     * 每个 ShortList 存储打包的段内本地坐标。
     *
     * @return 后处理位置数组的常指针，大小为 CHUNK_SECTIONS
     */
    [[nodiscard]] const std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS>& postProcessingSections() const noexcept
    {
        return m_postProcessingSections;
    }

    /**
     * @brief 获取后处理位置（可变版本）
     */
    [[nodiscard]] std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS>& postProcessingSections() noexcept
    {
        return m_postProcessingSections;
    }

    /**
     * @brief 从 ChunkPrimer 合并后处理位置
     *
     * @param sections 源后处理位置数组（按段索引）
     */
    void addPackedPostProcessing(const std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS>& sections);

    /**
     * @brief 清空指定段的后处理位置
     *
     * @param sectionIndex 区块段索引
     */
    void clearPostProcessingForSection(i32 sectionIndex);

    /**
     * @brief 清空所有段的后处理位置
     */
    void clearAllPostProcessing();

    /**
     * @brief 后处理是否已完成
     *
     * 标记 _postProcessChunk 是否已对该区块执行过。保证 onChunkLoaded / 区块加载回调 /
     * 实体生成 / _postProcessChunk 至多执行一次，防止重复入队或 worker/主线程路径竞态
     * 导致实体重复生成、区块重复发送、光照重复初始化。
     *
     * 不持久化：存档区块后处理位置为空且 needsPostProcess=false，重新加载后为 false 安全。
     *
     * @return 是否已完成后处理
     */
    [[nodiscard]] bool isPostProcessingDone() const noexcept { return m_postProcessingDone; }

    /**
     * @brief 设置后处理完成标志
     *
     * @param done 是否已完成后处理
     */
    void setPostProcessingDone(bool done) noexcept { m_postProcessingDone = done; }

    // ========================================================================
    // 游戏事件监听器注册表（按区块段）
    // ========================================================================

    /**
     * @brief 获取指定段的监听器注册表
     *
     * 返回该段对应的注册表指针。如果该段没有注册表（没有监听器），
     * 返回 nullptr。
     *
     * @param sectionY 段Y坐标（世界坐标，非数组索引）
     * @return 监听器注册表指针，如果不存在返回 nullptr
     */
    [[nodiscard]] gameevent::GameEventListenerRegistry* getGameEventListenerRegistry(i32 sectionY);

    /**
     * @brief 获取指定段的监听器注册表（const 版本）
     * @param sectionY 段Y坐标
     * @return 监听器注册表指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const gameevent::GameEventListenerRegistry* getGameEventListenerRegistry(i32 sectionY) const;

    /**
     * @brief 获取或创建指定段的监听器注册表
     *
     * 如果该段没有注册表，则使用提供的工厂函数创建一个新的。
     * 当注册表变为空时，通过 OnEmptyAction 自动从映射中移除。
     *
     * @param sectionY 段Y坐标
     * @param factory 创建注册表的工厂函数，接收段Y坐标，返回注册表 unique_ptr
     * @return 监听器注册表引用
     */
    [[nodiscard]] gameevent::GameEventListenerRegistry& getOrCreateGameEventListenerRegistry(
        i32 sectionY, std::function<std::unique_ptr<gameevent::EuclideanGameEventListenerRegistry>(i32)> factory);

    /**
     * @brief 移除指定段的监听器注册表
     *
     * 当注册表变为空时由 OnEmptyAction 回调调用，从映射中移除该段的注册表，
     * 防止空注册表长期累积导致内存泄漏。
     *
     * @param sectionY 段Y坐标
     */
    void removeGameEventListenerRegistry(i32 sectionY);

private:
    // 对象级内存追踪守卫：绑定本对象地址，ctor 发 alloc、dtor 发 free。move 时由
    // move ctor/assign 显式「释放旧地址 + 分配新地址」重绑定（守卫不可移动），避免
    // move 后旧地址仍留在 Tracy 活跃集、被堆复用时触发 MemAllocTwice 硬失败。
    // 仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件，其余分支空操作。
    // 详见 common/profiler/MemoryTracking.hpp。
    ::mc::profiler::TracyObjectTracker<"ChunkCache"> m_memTrack;

    ChunkCoord m_x = 0;
    ChunkCoord m_z = 0;

    // 区块段 (可以为空)
    std::array<std::unique_ptr<ChunkSection>, mc::world::CHUNK_SECTIONS> m_sections;

    // 区块段指针数组（用于 getSections() 接口，mutable 允许 const 方法更新）
    mutable std::array<const ChunkSection*, mc::world::CHUNK_SECTIONS> m_sectionPtrs{};

    // 高度图 (按 HeightmapType 枚举索引，O(1) 访问；WorldSurface 槽位作为快速查询缓存)
    std::array<Heightmap, HEIGHTMAP_TYPE_COUNT> m_heightmaps;

    // 标记对应类型的高度图槽位是否已被显式填充（updateHeightmap 或反序列化）。
    // 未填充的类型在 getTopBlockY 查询时回退到 WorldSurface 槽位。
    std::array<bool, HEIGHTMAP_TYPE_COUNT> m_heightmapInitialized{};

    // 生物群系采样数据
    BiomeContainer m_biomes;

    // 居住时间（区块内有玩家附近时的累计刻数）
    i64 m_inhabitedTime = 0;

    // 状态
    ChunkLoadStatus m_status = ChunkLoadStatus::Empty;
    bool m_fullyGenerated = false;
    bool m_dirty = false;
    bool m_loaded = false;
    // 光照正确标志（③-2b：atomic，worker 写 / 主线程未来可能读，见 isLightCorrect）
    std::atomic<bool> m_lightCorrect{false};

    // ========================================================================
    // Starlight 光照数据
    // ========================================================================

    // 天空光照 Nibble 数组 (每个区块段一个)
    std::array<SWMRNibbleArray, LIGHT_SECTIONS> m_skyNibbles;

    // 方块光照 Nibble 数组 (每个区块段一个)
    std::array<SWMRNibbleArray, LIGHT_SECTIONS> m_blockNibbles;

    // 天空光照空映射 (每个区块段一个)
    std::array<bool, LIGHT_SECTIONS> m_skyEmptinessMap{};
    bool m_hasSkyEmptinessMap = false;

    // 方块光照空映射 (每个区块段一个)
    std::array<bool, LIGHT_SECTIONS> m_blockEmptinessMap{};
    bool m_hasBlockEmptinessMap = false;

    // Nibble 数组指针数组（用于 getSkyNibbles/getBlockNibbles 接口）
    mutable std::array<SWMRNibbleArray*, LIGHT_SECTIONS> m_skyNibblePtrs{};
    mutable std::array<SWMRNibbleArray*, LIGHT_SECTIONS> m_blockNibblePtrs{};
    mutable bool m_nibblePtrsInitialized = false;

    // ========================================================================
    // 方块实体存储
    // ========================================================================

    // 使用位置哈希存储方块实体
    std::unordered_map<i64, std::unique_ptr<BlockEntity>> m_blockEntities;

    // 从存储加载出的运行时实体，先挂在 chunk 上，后续由世界层统一注入 EntityManager
    std::vector<std::unique_ptr<Entity>> m_loadedEntities;

    // Java 存档路径的实体原始 NBT（反序列化推迟到 onChunkLoaded，见上方 addLoadedEntityNbt 注释）
    std::vector<std::unique_ptr<nbt::tags::compound_tag>> m_loadedEntityNbt;

    // 后处理位置（按区块段索引存储）
    // 每个短整型编码段内本地坐标：bits[3:0]=x, bits[7:4]=y, bits[11:8]=z
    std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS> m_postProcessingSections;

    // 后处理是否已完成（_postProcessChunk 执行后置 true）。保证主线程后处理至多执行一次。
    // 不持久化：存档区块后处理位置为空且 needsPostProcess=false，重新加载后为 false 安全。
    bool m_postProcessingDone = false;

    // 游戏事件监听器注册表（按段Y坐标索引，惰性创建）
    // 当注册表为空时自动从映射中移除，节省内存
    std::unordered_map<i32, std::unique_ptr<gameevent::EuclideanGameEventListenerRegistry>>
        m_gameEventListenerRegistries;

    /**
     * @brief 初始化 Nibble 指针数组
     */
    void _ensureNibblePtrs() const;

    // 按枚举顺序为 m_heightmaps 每个槽位设置正确的 HeightmapType
    void _initHeightmaps();
};

// ============================================================================
// 区块数据引用 (轻量级访问)
// ============================================================================

class ChunkDataRef {
public:
    ChunkDataRef(ChunkData* data, bool writeAccess = false);
    ~ChunkDataRef();

    // 禁止拷贝
    ChunkDataRef(const ChunkDataRef&) = delete;
    ChunkDataRef& operator=(const ChunkDataRef&) = delete;

    // 允许移动
    ChunkDataRef(ChunkDataRef&& other) noexcept;
    ChunkDataRef& operator=(ChunkDataRef&& other) noexcept;

    // 访问
    [[nodiscard]] ChunkData* get() const { return m_data; }
    [[nodiscard]] ChunkData* operator->() const { return m_data; }
    [[nodiscard]] ChunkData& operator*() const { return *m_data; }

    [[nodiscard]] bool isValid() const { return m_data != nullptr; }
    [[nodiscard]] bool hasWriteAccess() const { return m_writeAccess; }

private:
    ChunkData* m_data = nullptr;
    bool m_writeAccess = false;
};

} // namespace mc::world::chunk

namespace mc {
using ChunkData = mc::world::chunk::ChunkData;
using ChunkSection = mc::world::chunk::ChunkSection;
using ChunkDataRef = mc::world::chunk::ChunkDataRef;
} // namespace mc
