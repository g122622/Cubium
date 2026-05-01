#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "../block/Block.hpp"

// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE
#include "../../util/NibbleArray.hpp"
#pragma pop_macro("BYTE_SIZE")
#include "../lighting/storage/SWMRNibbleArray.hpp"
#include "ChunkPos.hpp"
#include "IChunk.hpp"
#include "../WorldConstants.hpp"
#include <vector>
#include <memory>
#include <array>
#include <cstring>
#include <unordered_map>

namespace mc {

// ============================================================================
// 区块段 (16x16x16 方块)
// ============================================================================

class ChunkSection {
public:
    static constexpr i32 SIZE = world::CHUNK_SECTION_HEIGHT; // 16
    static constexpr i32 VOLUME = SIZE * SIZE * SIZE;        // 4096

    ChunkSection();
    ~ChunkSection() = default;

    // 方块访问 (使用状态ID)
    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const;
    void setBlockStateId(i32 x, i32 y, i32 z, u32 stateId);

    // 方块访问 (使用 BlockState 指针)
    [[nodiscard]] const BlockState* getBlock(i32 x, i32 y, i32 z) const;
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state);

    // 快速访问 (无边界检查)
    [[nodiscard]] u32 getBlockStateIdFast(i32 index) const {
        if (index < 0 || index >= static_cast<i32>(m_blockStates.size())) {
            return 0;
        }
        return m_blockStates[static_cast<size_t>(index)];
    }
    void setBlockStateIdFast(i32 index, u32 stateId) {
        if (index < 0 || index >= static_cast<i32>(m_blockStates.size())) {
            return;
        }
        m_blockStates[static_cast<size_t>(index)] = stateId;
    }

    // 索引计算
    [[nodiscard]] static i32 blockIndex(i32 x, i32 y, i32 z) {
        return y * SIZE * SIZE + z * SIZE + x;
    }

    // 段信息
    [[nodiscard]] bool isEmpty() const { return m_blockCount == 0; }
    [[nodiscard]] bool hasOnlyAir() const { return m_blockCount == 0; } // 与 isEmpty 等价，但语义更明确
    [[nodiscard]] u16 getBlockCount() const { return m_blockCount; }
    void setBlockCount(u16 count) { m_blockCount = count; }
    [[nodiscard]] bool needsRecalculate() const { return m_needsRecalculate; }
    void setNeedsRecalculate(bool value) { m_needsRecalculate = value; }

    // 随机刻计数器 (MC 1.16.5 用于性能优化)
    [[nodiscard]] bool needsRandomTickAny() const { return m_blockTickRefCount > 0 || m_fluidRefCount > 0; }
    [[nodiscard]] bool needsRandomTick() const { return m_blockTickRefCount > 0; }
    [[nodiscard]] bool needsRandomTickFluid() const { return m_fluidRefCount > 0; }
    [[nodiscard]] u16 blockTickRefCount() const { return m_blockTickRefCount; }
    [[nodiscard]] u16 fluidRefCount() const { return m_fluidRefCount; }

    // 光照
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;
    void setSkyLight(i32 x, i32 y, i32 z, u8 light);
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;
    void setBlockLight(i32 x, i32 y, i32 z, u8 light);

    // 光照访问器
    /**
     * @brief 获取天空光照数组（只读）
     */
    [[nodiscard]] const NibbleArray& skyLightNibble() const { return m_skyLight; }
    /**
     * @brief 获取天空光照数组（可变）
     */
    [[nodiscard]] NibbleArray& skyLightNibble() { return m_skyLight; }
    /**
     * @brief 获取方块光照数组（只读）
     */
    [[nodiscard]] const NibbleArray& blockLightNibble() const { return m_blockLight; }
    /**
     * @brief 获取方块光照数组（可变）
     */
    [[nodiscard]] NibbleArray& blockLightNibble() { return m_blockLight; }

    // 序列化
    [[nodiscard]] std::vector<u8> serialize() const;
    [[nodiscard]] static Result<std::unique_ptr<ChunkSection>> deserialize(const u8* data, size_t size);

    // 填充
    void fill(u32 stateId);

    // 填充光照
    /**
     * @brief 填充天空光照
     * @param light 光照值 (0-15)
     */
    void fillSkyLight(u8 light) { m_skyLight.fill(light); }

    /**
     * @brief 填充方块光照
     * @param light 光照值 (0-15)
     */
    void fillBlockLight(u8 light) { m_blockLight.fill(light); }

private:
    // 使用状态ID存储 (紧凑格式，后续可改为调色板)
    std::vector<u32> m_blockStates;  // BlockState::stateId()
    NibbleArray m_skyLight;          // 天空光照 (4位/方块)
    NibbleArray m_blockLight;        // 方块光照 (4位/方块)
    u16 m_blockCount = 0;            // 非空气方块数量
    bool m_needsRecalculate = false;

    // 随机刻计数器 (MC 1.16.5 用于性能优化)
    // 参考: net.minecraft.world.chunk.ChunkSection.blockTickRefCount 和 fluidRefCount
    u16 m_blockTickRefCount = 0;     // ticksRandomly 方块数量
    u16 m_fluidRefCount = 0;         // 流体方块数量
};

// ============================================================================
// 区块数据
// ============================================================================

class ChunkData : public IChunk {
public:

    ChunkData();
    ChunkData(ChunkCoord x, ChunkCoord z);
    ~ChunkData() = default;

    // 禁止拷贝
    ChunkData(const ChunkData&) = delete;
    ChunkData& operator=(const ChunkData&) = delete;

    // 允许移动
    ChunkData(ChunkData&&) = default;
    ChunkData& operator=(ChunkData&&) = default;

    // 位置 (IChunk 接口)
    [[nodiscard]] ChunkCoord x() const override { return m_x; }
    [[nodiscard]] ChunkCoord z() const override { return m_z; }
    [[nodiscard]] ChunkPos pos() const override { return ChunkPos(m_x, m_z); }

    // 方块访问 (IChunk 接口 - 使用 BlockState 指针)
    [[nodiscard]] const BlockState* getBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override;
    void setBlock(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

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

    // 删除段
    void removeSection(i32 index);

    // 高度图 (IChunk 接口)
    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override;
    void updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override;

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
    static constexpr i32 LIGHT_SECTIONS = world::CHUNK_SECTIONS + 2;  // -1 到 16 段

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
     */
    [[nodiscard]] bool isLightCorrect() const override { return m_lightCorrect; }

    /**
     * @brief 设置区块光照正确状态
     */
    void setLightCorrect(bool correct) override { m_lightCorrect = correct; }

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

private:
    ChunkCoord m_x = 0;
    ChunkCoord m_z = 0;

    // 区块段 (可以为空)
    std::array<std::unique_ptr<ChunkSection>, world::CHUNK_SECTIONS> m_sections;

    // 区块段指针数组（用于 getSections() 接口，mutable 允许 const 方法更新）
    mutable std::array<const ChunkSection*, world::CHUNK_SECTIONS> m_sectionPtrs{};

    // 高度图 (最高方块Y坐标)
    std::array<BlockCoord, world::CHUNK_WIDTH * world::CHUNK_WIDTH> m_heightMap;

    // 高度图 (IChunk 接口)
    std::unordered_map<HeightmapType, Heightmap> m_heightmaps;

    // 生物群系采样数据
    BiomeContainer m_biomes;

    // 状态
    ChunkLoadStatus m_status = ChunkLoadStatus::Empty;
    bool m_fullyGenerated = false;
    bool m_dirty = false;
    bool m_loaded = false;
    bool m_lightCorrect = false;

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

    /**
     * @brief 初始化 Nibble 指针数组
     */
    void ensureNibblePtrs() const;
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

// ============================================================================
// 区块唯一标识 TODO URGENT 迁移到单独文件
// ============================================================================

struct ChunkId {
    ChunkCoord x;
    ChunkCoord z;
    i32 dimension;  // 0=主世界, 1=下界, 2=末地

    ChunkId() : x(0), z(0), dimension(0) {}
    ChunkId(ChunkCoord x, ChunkCoord z, i32 dim = 0)
        : x(x), z(z), dimension(dim) {}

    [[nodiscard]] u64 toId() const {
        // 编码：高16位=维度，中间24位=X，低24位=Z
        // 这样可以支持 -8388608 到 8388607 的坐标范围
        u64 dim = static_cast<u64>(static_cast<u32>(dimension) & 0xFFFF);
        u64 dx = static_cast<u64>(static_cast<u32>(x) & 0xFFFFFF);
        u64 dz = static_cast<u64>(static_cast<u32>(z) & 0xFFFFFF);
        return (dim << 48) | (dx << 24) | dz;
    }

    [[nodiscard]] static ChunkId fromId(u64 id) {
        ChunkId cid;
        cid.dimension = static_cast<i32>(static_cast<u16>(id >> 48));
        // 处理24位有符号数
        u32 ux = static_cast<u32>((id >> 24) & 0xFFFFFF);
        u32 uz = static_cast<u32>(id & 0xFFFFFF);
        // 如果最高位为1，扩展为负数
        cid.x = (ux & 0x800000) ? static_cast<ChunkCoord>(ux | 0xFF000000) : static_cast<ChunkCoord>(ux);
        cid.z = (uz & 0x800000) ? static_cast<ChunkCoord>(uz | 0xFF000000) : static_cast<ChunkCoord>(uz);
        return cid;
    }

    bool operator==(const ChunkId& other) const {
        return x == other.x && z == other.z && dimension == other.dimension;
    }

    bool operator!=(const ChunkId& other) const {
        return !(*this == other);
    }

    bool operator<(const ChunkId& other) const {
        if (dimension != other.dimension) return dimension < other.dimension;
        if (x != other.x) return x < other.x;
        return z < other.z;
    }
};

} // namespace mc

// 哈希支持
namespace std {
template<>
struct hash<mc::ChunkId> {
    size_t operator()(const mc::ChunkId& id) const {
        return static_cast<size_t>(id.toId());
    }
};
}
