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

#include "common/core/Types.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::gen::density {
class NoiseChunk;
} // namespace mc::world::gen::density

namespace mc::world::chunk {

// ============================================================================
// 区块生成器 (中间状态)
// ============================================================================

/**
 * @brief 区块生成器 (ChunkPrimer)
 *
 * 区块生成过程中的中间状态类。
 * 在区块完全生成之前，使用此类存储临时数据。
 * 生成完成后转换为 ChunkData。
 *
 * 使用方法：
 * 1. 创建 ChunkPrimer
 * 2. 按阶段生成：BIOMES -> NOISE -> SURFACE -> CARVERS -> FEATURES -> HEIGHTMAPS
 * 3. 转换为 ChunkData
 *
 * @note 此类不是线程安全的，应该在单个线程中操作
 */
class ChunkPrimer : public IChunk {
public:
    // ============================================================================
    // 构造函数
    // ============================================================================

    /**
     * @brief 创建空区块生成器
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    explicit ChunkPrimer(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 从现有 ChunkData 创建（用于加载）
     */
    explicit ChunkPrimer(std::unique_ptr<ChunkData> data);

    /**
     * @brief 从共享所有权的 ChunkData 创建（用于存档命中扇出路径）
     *
     * 与 unique_ptr 版本语义一致（chunkStatus=FULL、status=Loaded、重算高度图），但接收
     * shared_ptr，使 primer 与已发布到 m_chunks 的 ChunkData 共享所有权（不拷贝、不独占）。
     * 用于 _fanOutAttachedWaiters 命中分支：owner 已把 ChunkData 存入 m_chunks，attached
     * waiter SCLM 需复用同一份 ChunkData 创建 primer（设 currentChunk），避免重复存储。
     *
     * @param data 共享所有权的 ChunkData（必须非空）
     */
    explicit ChunkPrimer(std::shared_ptr<ChunkData> data);

    ~ChunkPrimer() override;

    // 禁止拷贝
    ChunkPrimer(const ChunkPrimer&) = delete;
    ChunkPrimer& operator=(const ChunkPrimer&) = delete;

    // 允许移动（显式实现：对象追踪守卫不可移动，须在 body 重绑定）
    ChunkPrimer(ChunkPrimer&& other) noexcept;
    ChunkPrimer& operator=(ChunkPrimer&& other) noexcept;

    // ============================================================================
    // IChunk 接口实现
    // ============================================================================

    [[nodiscard]] ChunkCoord x() const override { return m_x; }
    [[nodiscard]] ChunkCoord z() const override { return m_z; }
    [[nodiscard]] ChunkPos pos() const override { return ChunkPos(m_x, m_z); }

    // 方块访问
    [[nodiscard]] const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;
    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) override;

    // 区块段访问
    [[nodiscard]] ChunkSection* getSection(i32 index) override;
    [[nodiscard]] const ChunkSection* getSection(i32 index) const override;
    [[nodiscard]] bool hasSection(i32 index) const override;
    ChunkSection* createSection(i32 index) override;
    [[nodiscard]] std::array<const ChunkSection*, mc::world::CHUNK_SECTIONS> getSections() const override;

    // 高度图
    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override;
    void updateHeightmap(
        HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

    // 高度图原始值（getFirstAvailable 语义）：最高方块 Y+1 或 NO_BLOCK_SENTINEL（空列）。
    // 供需精确识别空列的调用方使用，避免 getTopBlockY 把空列合并为 MIN_BUILD_HEIGHT。
    [[nodiscard]] BlockCoord getHeightmapFirstAvailable(HeightmapType type, BlockCoord x, BlockCoord z) const override;

    // 状态
    [[nodiscard]] ChunkLoadStatus getStatus() const override { return m_status; }
    void setStatus(ChunkLoadStatus status) override { m_status = status; }

    // 标记
    [[nodiscard]] bool isModified() const override { return m_modified; }
    void setModified(bool modified) override { m_modified = modified; }

    // ============================================================================
    // 生成阶段管理
    // ============================================================================

    /**
     * @brief 获取当前生成阶段
     */
    [[nodiscard]] const ChunkStatus& getChunkStatus() const noexcept { return *m_chunkStatus; }

    /**
     * @brief 设置当前生成阶段
     */
    void setChunkStatus(const ChunkStatus& status);

    /**
     * @brief 检查是否已完成指定阶段
     */
    [[nodiscard]] bool hasCompletedStatus(const ChunkStatus& status) const noexcept
    {
        return m_chunkStatus->isAtLeast(status);
    }

    // ============================================================================
    // 持久化状态
    // ============================================================================

    /**
     * @brief 获取持久化状态
     *
     * persistedStatus 记录已完成持久化的阶段，用于决定高度图更新范围。
     * 与 chunkStatus 分离：chunkStatus 是当前正在执行的目标阶段，
     * persistedStatus 是已经确认完成的阶段。
     */
    [[nodiscard]] const ChunkStatus& getPersistedStatus() const noexcept { return *m_persistedStatus; }

    /**
     * @brief 推进持久化状态到目标阶段
     *
     * 只允许向前推进（新状态的 ordinal 必须 >= 当前 persistedStatus）。
     * 同时推进 chunkStatus。
     *
     * @param target 目标阶段
     */
    void setPersistedStatus(const ChunkStatus& target);

    // ============================================================================
    // 生物群系管理
    // ============================================================================

    /**
     * @brief 设置生物群系容器
     */
    void setBiomes(BiomeContainer biomes) noexcept { m_biomes = std::move(biomes); }

    /**
     * @brief 获取生物群系容器
     */
    [[nodiscard]] const BiomeContainer& getBiomes() const noexcept { return m_biomes; }
    [[nodiscard]] BiomeContainer& getBiomes() noexcept { return m_biomes; }

    /**
     * @brief 获取方块位置的生物群系
     */
    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override;

    // ============================================================================
    // 光源位置
    // ============================================================================

    /**
     * @brief 添加光源位置（用于光照计算）
     */
    void addLightPosition(BlockCoord x, BlockCoord y, BlockCoord z);

    /**
     * @brief 获取所有光源位置
     */
    [[nodiscard]] const std::vector<BlockCoord>& getLightPositions() const noexcept { return m_lightPositions; }

    // ============================================================================
    // 高度图管理
    // ============================================================================

    /**
     * @brief 获取高度图
     */
    [[nodiscard]] Heightmap& getHeightmap(HeightmapType type);
    [[nodiscard]] const Heightmap& getHeightmap(HeightmapType type) const;

    /**
     * @brief 更新所有高度图
     */
    void updateAllHeightmaps();

    /**
     * @brief 初始化光源列表
     *
     * INITIALIZE_LIGHT 阶段调用：遍历区块中所有方块，
     * 找到亮度 > 0 的方块（火把、荧石等），注册到光照引擎。
     */
    void initializeLightSources();

    /**
     * @brief 从已有方块数据初始化指定高度图
     *
     * FEATURES 阶段开始前调用，从已放置的方块数据重新计算
     * FINAL_HEIGHTMAPS（OCEAN_FLOOR, WORLD_SURFACE, MOTION_BLOCKING, MOTION_BLOCKING_NO_LEAVES）。
     */
    void primeHeightmaps(HeightmapFlag types);

    // ============================================================================
    // 生成的实体
    // ============================================================================

    /**
     * @brief 获取区块生成时生成的实体列表
     *
     * 区块生成过程中调用 WorldGenSpawner 生成的被动动物会存储在这里。
     * 在区块生成完成后，由 ServerWorld 将这些实体真正创建到世界中。
     */
    [[nodiscard]] std::vector<SpawnedEntityData>& spawnedEntities() { return m_spawnedEntities; }
    [[nodiscard]] const std::vector<SpawnedEntityData>& spawnedEntities() const { return m_spawnedEntities; }

    /**
     * @brief 添加生成的实体数据
     */
    void addSpawnedEntity(SpawnedEntityData data) { m_spawnedEntities.push_back(std::move(data)); }

    /**
     * @brief 清空生成的实体列表
     */
    void clearSpawnedEntities() noexcept { m_spawnedEntities.clear(); }

    /**
     * @brief 获取生成的实体数量
     */
    [[nodiscard]] size_t spawnedEntityCount() const noexcept { return m_spawnedEntities.size(); }

    // ============================================================================
    // 结构起点管理
    // ============================================================================

    /**
     * @brief 添加结构起点
     * @param structureId 结构资源位置 ID
     * @param start 结构起点实例
     */
    void addStructureStart(
        const ResourceLocation& structureId, std::shared_ptr<mc::world::gen::structure::StructureStart> start)
    {
        m_structureStarts[structureId] = std::move(start);
    }

    /**
     * @brief 获取结构起点
     * @param structureId 结构资源位置 ID
     * @return 结构起点指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] mc::world::gen::structure::StructureStart* getStructureStart(
        const ResourceLocation& structureId) noexcept
    {
        auto it = m_structureStarts.find(structureId);
        return it != m_structureStarts.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取结构起点（const 版本）
     * @param structureId 结构资源位置 ID
     * @return 结构起点指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] const mc::world::gen::structure::StructureStart* getStructureStart(
        const ResourceLocation& structureId) const noexcept
    {
        auto it = m_structureStarts.find(structureId);
        return it != m_structureStarts.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 获取所有结构起点（shared_ptr 共享所有权）
     *
     * 结构起点以 shared_ptr 共享：邻居区块在 STRUCTURE_REFERENCES 等阶段通过 WorldGenRegion
     * 读取邻居结构起点（generateStructureReferences 读取邻居 StructureStart 并 incrementRefCount）。
     * shared_ptr 使邻居持有同一份 StructureStart，incrementRefCount 的修改反映到共享对象上。
     */
    [[nodiscard]] const std::unordered_map<ResourceLocation,
        std::shared_ptr<mc::world::gen::structure::StructureStart>>&
    structureStarts() const noexcept
    {
        return m_structureStarts;
    }

    /**
     * @brief 获取与指定区块相交的结构引用
     *
     * 遍历此区块上的所有结构起点，返回边界框与指定区块相交的结构。
     */
    [[nodiscard]] std::vector<std::tuple<ResourceLocation, ChunkCoord, ChunkCoord>> getIntersectingStructures(
        ChunkCoord cx, ChunkCoord cz) const override
    {
        std::vector<std::tuple<ResourceLocation, ChunkCoord, ChunkCoord>> result;
        for (const auto& [id, start] : m_structureStarts) {
            if (start && start->isValid() && start->getBoundingBox().intersectsChunk(cx, cz)) {
                result.emplace_back(id, m_x, m_z);
            }
        }
        return result;
    }

    /**
     * @brief 检查是否有结构起点
     */
    [[nodiscard]] bool hasStructureStarts() const noexcept { return !m_structureStarts.empty(); }

    // ============================================================================
    // 结构引用管理
    // ============================================================================

    /**
     * @brief 添加结构引用
     *
     * 存储"哪些结构可能与此区块相交"的信息。
     * 在 STRUCTURE_REFERENCES 阶段，扫描周围区块的 StructureStart，
     * 如果其边界框与此区块相交，则添加引用。
     *
     * @param structureId 结构资源位置 ID
     * @param referenceChunkX 被引用结构所在区块的 X 坐标
     * @param referenceChunkZ 被引用结构所在区块的 Z 坐标
     */
    void addStructureReference(
        const ResourceLocation& structureId, ChunkCoord referenceChunkX, ChunkCoord referenceChunkZ)
    {
        m_structureReferences[structureId].emplace_back(referenceChunkX, referenceChunkZ);
    }

    /**
     * @brief 获取指定结构的所有引用
     * @param structureId 结构资源位置 ID
     * @return 引用列表（区块坐标），如果不存在返回空列表
     */
    [[nodiscard]] const std::vector<std::pair<ChunkCoord, ChunkCoord>>& getStructureReferences(
        const ResourceLocation& structureId) const
    {
        static const std::vector<std::pair<ChunkCoord, ChunkCoord>> empty;
        auto it = m_structureReferences.find(structureId);
        return it != m_structureReferences.end() ? it->second : empty;
    }

    /**
     * @brief 获取所有结构引用
     */
    [[nodiscard]] const std::unordered_map<ResourceLocation, std::vector<std::pair<ChunkCoord, ChunkCoord>>>&
    structureReferences() const noexcept
    {
        return m_structureReferences;
    }

    /**
     * @brief 检查是否有结构引用
     */
    [[nodiscard]] bool hasStructureReferences() const noexcept { return !m_structureReferences.empty(); }

    // ============================================================================
    // 后处理位置
    // ============================================================================

    /**
     * @brief 标记方块位置为需要后处理
     *
     * 将位置打包为短整型并按区块段索引存储。
     * 用于含水层流体更新调度、地表流体方块、雕刻器流体方块等。
     * 在区块发布后由 postProcessGeneration 遍历这些位置：
     * - 流体方块：调度流体 tick
     * - 非液体方块：通过 updateFromNeighbourShapes 更新方块形状
     *
     * @param x 区块内 X 坐标 (0-15)
     * @param y 方块 Y 坐标
     * @param z 区块内 Z 坐标 (0-15)
     */
    void markPosForPostprocessing(BlockCoord x, BlockCoord y, BlockCoord z);

    /**
     * @brief 获取后处理位置（按区块段索引）
     *
     * 返回按区块段索引组织的打包位置列表数组。
     * 每个短整型编码了段内本地坐标：bits[3:0]=x, bits[7:4]=y, bits[11:8]=z。
     *
     * @return 后处理位置数组的常引用，大小为 CHUNK_SECTIONS
     */
    [[nodiscard]] const std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS>& postProcessingSections() const noexcept
    {
        return m_postProcessingSections;
    }

    /**
     * @brief 从另一个来源合并后处理位置
     *
     * @param packedPositions 打包位置列表
     * @param sectionIndex 区块段索引
     */
    void addPackedPostProcessing(const std::vector<u16>& packedPositions, i32 sectionIndex);

    // ============================================================================
    // 雕刻掩码
    // ============================================================================

    /**
     * @brief 获取雕刻掩码
     *
     * MC原版中，CarvingMask 在 AIR 和 LIQUID 两个雕刻阶段之间共享。
     * 第一次调用时自动创建掩码。
     *
     * @return 雕刻掩码引用
     */
    [[nodiscard]] CarvingMask& carvingMask();

    /**
     * @brief 检查是否已有雕刻掩码
     */
    [[nodiscard]] bool hasCarvingMask() const noexcept { return m_carvingMask != nullptr; }

    // ============================================================================
    // NoiseChunk 缓存
    // ============================================================================

    /**
     * @brief 获取或创建 NoiseChunk
     *
     * NoiseChunk 在 biomes/noise/surface/carvers 阶段共享。
     * 第一次调用时使用 factory 创建，后续调用返回缓存实例。
     *
     * @param factory 创建 NoiseChunk 的工厂函数
     * @return NoiseChunk 引用
     */
    [[nodiscard]] mc::world::gen::density::NoiseChunk& getOrCreateNoiseChunk(
        std::function<std::unique_ptr<mc::world::gen::density::NoiseChunk>()> factory);

    /**
     * @brief 获取 NoiseChunk（可能为 nullptr）
     */
    [[nodiscard]] mc::world::gen::density::NoiseChunk* noiseChunk() noexcept { return m_noiseChunk.get(); }
    [[nodiscard]] const mc::world::gen::density::NoiseChunk* noiseChunk() const noexcept { return m_noiseChunk.get(); }

    /**
     * @brief 检查是否已有 NoiseChunk
     */
    [[nodiscard]] bool hasNoiseChunk() const noexcept { return m_noiseChunk != nullptr; }

    // ============================================================================
    // 转换方法
    // ============================================================================

    /**
     * @brief 共享底层 ChunkData（非破坏性）
     *
     * 对齐 Moonrise：FULL 完成后 ChunkPrimer 仍持有 ChunkData 供邻居引用（直到 holder 卸载），
     * 同时把同一份 ChunkData 发布到内存缓存 m_chunks 供游戏逻辑访问。两者共享所有权。
     *
     * 先完成收尾（高度图、biomes、后处理位置、状态标记），再返回 m_data 的共享副本。
     * 不移走 m_data，ChunkPrimer 仍可正常访问（getChunkData/getBlockState 等）。
     * m_spawnedEntities 仍清空（实体数据已在收尾前由调用方提取）。
     *
     * @return 共享同一份 ChunkData 的 shared_ptr
     */
    [[nodiscard]] std::shared_ptr<ChunkData> toChunkData();

    /**
     * @brief 释放已完成阶段不再需要的生成态数据，降低区块驻留内存
     *
     * 对齐 Moonrise 的阶段性释放策略。在对应生成阶段完成后由 ServerChunkManager 调用。
     *
     * 释放规则（基于对 NoiseChunkGenerator 各阶段数据消费的审计）：
     *   - CARVERS 之后：m_noiseChunk（最后在 applyCarvers 中读取）、m_carvingMask（仅 applyCarvers 使用）
     *   - FEATURES 之后：m_structureReferences（最后在 placeFeatures 中读取）、
     *                     m_postProcessingSections（已由 toChunkData 转移到 ChunkData，但 FEATURES 阶段
     *                     markPosForPostprocessing 仍会写入；FEATURES 完成后不再有写入）
     *   - m_structureStarts 不释放：邻居在 STRUCTURE_REFERENCES/FEATURES/NOISE 阶段通过
     *                     getIntersectingStructures/getStructureStart 读取，必须存活到 holder 卸载
     *   - m_lightPositions 不释放：恒为空（addLightPosition/getLightPositions 在生产代码中无调用）
     *
     * @param afterStatus 刚完成的 ChunkStatus
     */
    void releaseGenOnlyData(const ChunkStatus& afterStatus);

    /**
     * @brief 获取底层 ChunkData（如果存在）
     */
    [[nodiscard]] ChunkData* getChunkData() noexcept { return m_data.get(); }
    [[nodiscard]] const ChunkData* getChunkData() const noexcept { return m_data.get(); }

    /**
     * @brief 共享底层 ChunkData 的所有权（非破坏性，不触发收尾）
     *
     * 与 toChunkData() 的区别：toChunkData() 先完成收尾（setBiomes/setFullyGenerated/
     * setStatus/addPackedPostProcessing 等，会修改 ChunkData），用于生成 FULL 完成路径。
     * 本方法仅返回 m_data 的共享副本，不修改任何状态，用于存档命中路径——存档加载的
     * ChunkData 已是完整持久化状态，不应被 primer 收尾逻辑覆盖（如 setBiomes(m_biomes)
     * 会用 primer 未填充的 m_biomes 清空存档的生物群系）。
     *
     * @return 共享同一份 ChunkData 的 shared_ptr
     */
    [[nodiscard]] std::shared_ptr<ChunkData> shareChunkData() const noexcept { return m_data; }

    // ============================================================================
    // 静态工具方法
    // ============================================================================

    /**
     * @brief 将方块坐标打包为短整型
     */
    [[nodiscard]] static u16 packToLocal(BlockCoord x, BlockCoord y, BlockCoord z) noexcept;

    /**
     * @brief 将短整型解包为方块坐标
     */
    static void unpackFromLocal(u16 packed,
        i32 yOffset,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        BlockCoord& x,
        BlockCoord& y,
        BlockCoord& z) noexcept;

private:
    // 对象级内存追踪守卫：绑定本对象地址，ctor 发 alloc、dtor 发 free。move 时由
    // move ctor/assign 显式「释放旧地址 + 分配新地址」重绑定（守卫不可移动）。
    // 仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件，其余分支空操作。
    ::mc::profiler::TracyObjectTracker<"ChunkPrimer"> m_memTrack;

    ChunkCoord m_x;
    ChunkCoord m_z;

    // 底层数据（shared_ptr：FULL 完成后与内存缓存 m_chunks 共享同一份 ChunkData，
    // 保证邻居引用的 primer 在 holder 卸载前始终有效，对齐 Moonrise currentChunk 生命周期）
    std::shared_ptr<ChunkData> m_data;

    // 生成状态
    const ChunkStatus* m_chunkStatus = &ChunkStatuses::EMPTY;
    const ChunkStatus* m_persistedStatus = &ChunkStatuses::EMPTY;
    ChunkLoadStatus m_status = ChunkLoadStatus::Empty;
    bool m_modified = false;

    // 生物群系
    BiomeContainer m_biomes;

    // 高度图 (按 HeightmapType 枚举索引，O(1) 访问；构造时全量初始化全部类型，
    // 故所有槽位恒存在，无需 find/emplace/回退。与 ChunkData::m_heightmaps 风格一致)
    std::array<Heightmap, HEIGHTMAP_TYPE_COUNT> m_heightmaps;

    // 光源位置
    std::vector<BlockCoord> m_lightPositions;

    // 区块生成时生成的实体
    std::vector<SpawnedEntityData> m_spawnedEntities;

    // 结构起点（用于结构生成）
    // shared_ptr 共享所有权：邻居区块在 STRUCTURE_REFERENCES 等阶段通过 WorldGenRegion 读取邻居
    // 结构起点并 incrementRefCount。shared_ptr 使邻居持有同一份 StructureStart，计数修改反映到共享对象。
    std::unordered_map<ResourceLocation, std::shared_ptr<mc::world::gen::structure::StructureStart>> m_structureStarts;

    // 结构引用（哪些结构的边界框与此区块相交）
    std::unordered_map<ResourceLocation, std::vector<std::pair<ChunkCoord, ChunkCoord>>> m_structureReferences;

    // 雕刻掩码（AIR 和 LIQUID 两个雕刻阶段共享）
    std::unique_ptr<CarvingMask> m_carvingMask;

    /// 后处理位置（按区块段索引存储）
    /// 每个短整型编码段内本地坐标：bits[3:0]=x, bits[7:4]=y, bits[11:8]=z
    std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS> m_postProcessingSections;

    // NoiseChunk 缓存（biomes/noise/surface/carvers 阶段共享）
    std::unique_ptr<mc::world::gen::density::NoiseChunk> m_noiseChunk;

    // 辅助方法
    [[nodiscard]] static bool _isValidBlockCoord(BlockCoord x, BlockCoord y, BlockCoord z) noexcept;

    /**
     * @brief 根据当前 ChunkStatus 的 heightmapsAfter 自动更新高度图
     *
     * 在设置方块后，根据 persistedStatus.heightmapsAfter() 决定更新哪些高度图。
     * 对于尚未创建的高度图类型，先 prime（从方块数据初始化）再增量更新。
     */
    void _updateHeightmapsForCurrentStatus(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state);
};

} // namespace mc::world::chunk
