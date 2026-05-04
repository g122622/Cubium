#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "ChunkPos.hpp"
#include <array>
#include <vector>

namespace mc {

// 前向声明
class BlockState;
class ChunkSection;
class SWMRNibbleArray;

// ============================================================================
// 区块状态枚举 (简化版)
// ============================================================================

enum class ChunkLoadStatus : u8 {
    Empty,          // 空区块，刚创建
    Generating,     // 正在生成
    Generated,      // 已生成，完整
    Loaded,         // 已加载到内存
    Unloaded        // 已卸载
};

// ============================================================================
// 区块高度图类型
// ============================================================================

enum class HeightmapType : u8 {
    WorldSurface,           // 最高非空气方块
    OceanFloor,             // 最高固体方块
    MotionBlocking,         // 最高阻挡运动方块
    MotionBlockingNoLeaves, // 最高阻挡运动方块（不含树叶）
    WorldSurfaceWG,         // 世界表面（生成时）
    OceanFloorWG,           // 海底（生成时）
    LightBlocking           // 最高阻挡光照方块
};

// ============================================================================
// 区块接口
// ============================================================================

/**
 * @brief 区块接口
 *
 * 参考 MC IChunk，定义区块的基本操作接口。
 * 支持 ChunkPrimer（中间状态）和 ChunkData（最终状态）。
 */
class IChunk {
public:
    virtual ~IChunk() = default;

    // === 位置信息 ===
    [[nodiscard]] virtual ChunkCoord x() const = 0;
    [[nodiscard]] virtual ChunkCoord z() const = 0;
    [[nodiscard]] virtual ChunkPos pos() const = 0;

    // === 方块访问 ===
    [[nodiscard]] virtual const BlockState* getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const = 0;
    virtual void setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) = 0;
    [[nodiscard]] virtual u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const = 0;
    virtual void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) = 0;

    // === 区块段访问 ===
    [[nodiscard]] virtual ChunkSection* getSection(i32 index) = 0;
    [[nodiscard]] virtual const ChunkSection* getSection(i32 index) const = 0;
    [[nodiscard]] virtual bool hasSection(i32 index) const = 0;
    virtual ChunkSection* createSection(i32 index) = 0;

    /**
     * @brief 获取所有区块段数组
     * @return 指向区块段指针数组的指针，数组大小为 16
     */
    [[nodiscard]] virtual const ChunkSection* const* getSections() const = 0;

    // === 生物群系 ===
    [[nodiscard]] virtual BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const = 0;

    // === 高度图 ===
    [[nodiscard]] virtual BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const = 0;
    virtual void updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) = 0;

    // === 状态 ===
    [[nodiscard]] virtual ChunkLoadStatus getStatus() const = 0;
    virtual void setStatus(ChunkLoadStatus status) = 0;

    // === 标记 ===
    [[nodiscard]] virtual bool isModified() const = 0;
    virtual void setModified(bool modified) = 0;

    // ========================================================================
    // Starlight 光照数据接口 (参考 Moonrise StarlightChunk)
    // ========================================================================

    /**
     * @brief 获取天空光照空映射
     * @return 空映射数组，每个元素对应一个区块段是否为空
     */
    [[nodiscard]] virtual const bool* getSkyEmptinessMap() const { return nullptr; }

    /**
     * @brief 设置天空光照空映射
     */
    virtual void setSkyEmptinessMap(const bool* map) { (void)map; }

    /**
     * @brief 获取方块光照空映射
     */
    [[nodiscard]] virtual const bool* getBlockEmptinessMap() const { return nullptr; }

    /**
     * @brief 设置方块光照空映射
     */
    virtual void setBlockEmptinessMap(const bool* map) { (void)map; }

    /**
     * @brief 获取天空光照 Nibble 数组
     * @return Nibble 数组指针数组，索引为 [y - minLightSection]
     */
    [[nodiscard]] virtual SWMRNibbleArray* const* getSkyNibbles() const { return nullptr; }

    /**
     * @brief 设置天空光照 Nibble 数组
     */
    virtual void setSkyNibbles(SWMRNibbleArray* const* nibbles) { (void)nibbles; }

    /**
     * @brief 获取方块光照 Nibble 数组
     */
    [[nodiscard]] virtual SWMRNibbleArray* const* getBlockNibbles() const { return nullptr; }

    /**
     * @brief 设置方块光照 Nibble 数组
     */
    virtual void setBlockNibbles(SWMRNibbleArray* const* nibbles) { (void)nibbles; }

    /**
     * @brief 检查区块是否光照正确
     * 用于判断区块是否可以用于光照计算
     */
    [[nodiscard]] virtual bool isLightCorrect() const { return true; }

    /**
     * @brief 设置区块光照正确状态
     */
    virtual void setLightCorrect(bool correct) { (void)correct; }
};

// ============================================================================
// 生物群系容器
// ============================================================================

/**
 * @brief 生物群系容器
 *
 * 存储区块内的生物群系信息。每个区块有 4x4x4 的生物群系采样点。
 */
class BiomeContainer {
public:
    // 生物群系采样尺寸（每个区块段的生物群系采样点数量）
    static constexpr i32 BIOME_WIDTH = 4;
    static constexpr i32 BIOME_HEIGHT = 4;
    static constexpr i32 BIOME_DEPTH = 4;
    static constexpr i32 BIOME_SIZE = BIOME_WIDTH * BIOME_HEIGHT * BIOME_DEPTH;

    BiomeContainer() = default;

    /**
     * @brief 设置生物群系
     * @param x X 采样位置 (0-3)
     * @param y Y 采样位置 (0-3 对应 16 段区块中的某一段)
     * @param z Z 采样位置 (0-3)
     * @param biome 生物群系 ID
     */
    void setBiome(i32 x, i32 y, i32 z, BiomeId biome);

    /**
     * @brief 获取生物群系
     */
    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取方块位置的生物群系（插值）
     * @param x 方块 X 坐标 (0-15)
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标 (0-15)
     */
    [[nodiscard]] BiomeId getBiomeAtBlock(i32 x, i32 y, i32 z) const;

    /**
     * @brief 序列化
     */
    [[nodiscard]] std::vector<u8> serialize() const;
    static Result<BiomeContainer> deserialize(const u8* data, size_t size);

private:
    // 存储生物群系 ID，初始化为 0 (void/placeholder)
    std::array<BiomeId, BIOME_SIZE> m_biomes{};
};

// ============================================================================
// 高度图
// ============================================================================

/**
 * @brief 高度图
 *
 * 存储每个 XZ 位置的最高方块 Y 坐标。
 */
class Heightmap {
public:
    static constexpr i32 SIZE = 16 * 16;

    explicit Heightmap(HeightmapType type = HeightmapType::WorldSurface);

    /**
     * @brief 更新高度图
     * @param x 区块内 X 坐标 (0-15)
     * @param y 方块 Y 坐标
     * @param z 区块内 Z 坐标 (0-15)
     * @param state 方块状态
     * @return true 如果高度更新
     */
    bool update(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state);

    /**
     * @brief 获取高度
     */
    [[nodiscard]] BlockCoord getHeight(BlockCoord x, BlockCoord z) const;

    /**
     * @brief 设置高度数据（从存档加载）
     */
    void setData(const std::array<BlockCoord, SIZE>& data);

    /**
     * @brief 获取高度数据
     */
    [[nodiscard]] const std::array<BlockCoord, SIZE>& getData() const { return m_heights; }

    [[nodiscard]] HeightmapType getType() const { return m_type; }

private:
    HeightmapType m_type;
    std::array<BlockCoord, SIZE> m_heights;

    /**
     * @brief 检查方块是否影响此高度图
     */
    [[nodiscard]] bool isOpaque(const BlockState* state) const;
};

} // namespace mc
