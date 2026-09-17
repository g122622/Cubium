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

#include "SectionKey.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mc::world::storage {

/**
 * @brief Section数据格式版本
 */
enum class SectionFormatVersion : u16 {
    /// 初始版本
    V1 = 1,

    /// 当前版本
    Current = V1
};

/**
 * @brief Section数据标志位
 */
enum class SectionFlags : u16 {
    /// 无标志
    None = 0,

    /// 包含生物群系数据
    HasBiomes = 1 << 0,

    /// 包含天空光照
    HasSkyLight = 1 << 1,

    /// 包含方块光照
    HasBlockLight = 1 << 2,

    /// 空Section（无方块）
    IsEmpty = 1 << 3,

    /// 压缩数据
    Compressed = 1 << 4
};

/**
 * @brief Section标志位操作
 */
inline SectionFlags operator|(SectionFlags a, SectionFlags b)
{
    return static_cast<SectionFlags>(static_cast<u16>(a) | static_cast<u16>(b));
}

inline SectionFlags operator&(SectionFlags a, SectionFlags b)
{
    return static_cast<SectionFlags>(static_cast<u16>(a) & static_cast<u16>(b));
}

inline SectionFlags& operator|=(SectionFlags& a, SectionFlags b)
{
    a = a | b;
    return a;
}

inline bool hasFlag(SectionFlags flags, SectionFlags flag)
{
    return (flags & flag) == flag;
}

/**
 * @brief Section数据结构
 *
 * 存储单个Section（16x16x16方块块）的所有数据。
 * 用于RocksDB存储和ChunkSection之间的转换。
 */
struct SectionData {
    // ========================================================================
    // 静态常量
    // ========================================================================

    /// Section尺寸（16）
    static constexpr i32 SIZE = 16;

    /// Section体积（4096）
    static constexpr i32 VOLUME = SIZE * SIZE * SIZE;

    /// 生物群系采样尺寸（4x4x4）
    static constexpr i32 BIOME_SIZE = 4;

    /// 生物群系数量（64）
    static constexpr i32 BIOME_COUNT = BIOME_SIZE * BIOME_SIZE * BIOME_SIZE;

    /// 当前数据版本
    static constexpr u32 CURRENT_DATA_VERSION = 1;

    // ========================================================================
    // 对象级内存追踪
    // ========================================================================
private:
    // 绑定本对象地址，ctor 发 alloc、dtor 发 free。move 时由 move ctor/assign 显式
    // 「释放旧地址 + 分配新地址」重绑定（守卫不可移动）。仅 MC_ENABLE_MEMORY &&
    // MC_ENABLE_TRACY 时发事件。sizeof(SectionData) 只含外层结构体，绝对值偏低但
    // 能正确反映 SectionCache LRU 中 section 的驻留数量波动。
    // 声明于数据成员之前，使 ctor 初始化列表可将其置于首位（满足 -Wreorder-ctor）。
    ::mc::profiler::TracyObjectTracker<"SectionCache"> m_memTrack;

    // ========================================================================
    // 数据成员
    // ========================================================================
public:
    /// Section标识
    SectionKey key;

    /// 方块状态ID数组（4096个）
    /// 使用BlockState::stateId()的值
    std::vector<u32> blockStates;

    /// 非空方块数量
    u16 nonEmptyBlockCount = 0;

    /// 生物群系数据（4x4x4采样，64个）
    std::vector<BiomeId> biomes;

    /// 天空光照（可选，2048字节NibbleArray）
    std::optional<std::vector<u8>> skyLight;

    /// 方块光照（可选，2048字节NibbleArray）
    std::optional<std::vector<u8>> blockLight;

    /// 数据版本
    u32 dataVersion = CURRENT_DATA_VERSION;

    /// 内容哈希（用于快照去重）
    u64 contentHash = 0;

    // ========================================================================
    // 构造函数
    // ========================================================================

    SectionData();

    /**
     * @brief 从SectionKey构造
     */
    explicit SectionData(const SectionKey& key);

    /**
     * @brief 从坐标构造
     */
    SectionData(i32 chunkX, i32 chunkZ, i8 sectionY, DimensionId dimension = 0);

    // 不可拷贝赋值（含对象追踪守卫）；提供显式拷贝构造（产生新对象，bind 新地址）；
    // 显式移动（守卫不可移动，须在 body 重绑定）
    SectionData(const SectionData& other);
    SectionData& operator=(const SectionData&) = delete;
    SectionData(SectionData&& other) noexcept;
    SectionData& operator=(SectionData&& other) noexcept;

    // ========================================================================
    // 数据访问
    // ========================================================================

    /**
     * @brief 获取方块状态ID
     * @param x 局部X坐标 (0-15)
     * @param y 局部Y坐标 (0-15)
     * @param z 局部Z坐标 (0-15)
     * @return 方块状态ID
     */
    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const;

    /**
     * @brief 设置方块状态ID
     */
    void setBlockStateId(i32 x, i32 y, i32 z, u32 stateId);

    /**
     * @brief 获取生物群系ID
     * @param x 局部X坐标 (0-15)
     * @param y 局部Y坐标 (0-15)
     * @param z 局部Z坐标 (0-15)
     * @return 生物群系ID
     */
    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const;

    /**
     * @brief 设置生物群系ID
     */
    void setBiome(i32 x, i32 y, i32 z, BiomeId biome);

    /**
     * @brief 检查是否为空Section
     */
    [[nodiscard]] bool isEmpty() const { return nonEmptyBlockCount == 0; }

    // ========================================================================
    // 序列化
    // ========================================================================

    /**
     * @brief 序列化为二进制
     *
     * 二进制格式：
     * - Header (12字节):
     *   - version (u16): 格式版本
     *   - flags (u16): 标志位
     *   - blockCount (u16): 非空方块数
     *   - reserved (u16): 保留
     *   - contentHash (u32): 内容哈希低32位
     * - Block States (压缩):
     *   - 如果 isEmpty: 无数据
     *   - 否则: ZSTD压缩的4096个u32
     * - Biomes (可选, 64字节):
     *   - 如果 HasBiomes: 64个BiomeId
     * - Sky Light (可选, 2048字节):
     *   - 如果 HasSkyLight: NibbleArray数据
     * - Block Light (可选, 2048字节):
     *   - 如果 HasBlockLight: NibbleArray数据
     *
     * @return 序列化后的数据
     */
    [[nodiscard]] Result<std::vector<u8>> serialize() const;

    /**
     * @brief 从二进制反序列化
     * @param data 二进制数据
     * @param size 数据大小
     * @return Section数据
     */
    [[nodiscard]] static Result<SectionData> deserialize(const u8* data, size_t size);

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 计算内容哈希
     *
     * 基于方块状态和生物群系数据计算64位哈希，
     * 用于快照去重。
     */
    void computeHash();

    /**
     * @brief 初始化默认数据
     */
    void initializeDefaults();

    /**
     * @brief 清空所有数据
     */
    void clear();

private:
    /**
     * @brief 计算方块索引
     */
    [[nodiscard]] static i32 _blockIndex(i32 x, i32 y, i32 z) { return y * SIZE * SIZE + z * SIZE + x; }

    /**
     * @brief 计算生物群系索引
     */
    [[nodiscard]] static i32 _biomeIndex(i32 x, i32 y, i32 z)
    {
        i32 bx = x / 4;
        i32 by = y / 4;
        i32 bz = z / 4;
        return by * BIOME_SIZE * BIOME_SIZE + bz * BIOME_SIZE + bx;
    }
};

/**
 * @brief Section序列化/反序列化工具
 *
 * 提供SectionData与ChunkSection之间的转换，
 * 以及压缩/解压缩功能。
 */
class SectionCodec {
public:
    // ========================================================================
    // ChunkSection转换
    // ========================================================================

    /**
     * @brief 从ChunkSection创建SectionData
     *
     * @param section ChunkSection对象
     * @param key Section标识
     * @param biomes 生物群系数据（可选，64个）
     * @return SectionData
     */
    [[nodiscard]] static Result<SectionData> fromChunkSection(
        const ChunkSection& section, const SectionKey& key, const std::vector<BiomeId>& biomes = {});

    /**
     * @brief 将SectionData应用到ChunkSection
     *
     * @param data Section数据
     * @param section 目标ChunkSection
     * @return 成功或错误
     */
    [[nodiscard]] static Result<void> toChunkSection(const SectionData& data, ChunkSection& section);

    // ========================================================================
    // 压缩工具
    // ========================================================================

    /**
     * @brief 压缩数据
     *
     * 使用ZSTD算法压缩数据。
     *
     * @param data 原始数据
     * @param size 数据大小
     * @param compressionLevel 压缩级别（1-22，默认3）
     * @return 压缩后的数据
     */
    [[nodiscard]] static Result<std::vector<u8>> compress(const u8* data, size_t size, i32 compressionLevel = 3);

    /**
     * @brief 解压缩数据到调用方提供的缓冲区
     *
     * 不分配新内存：直接写入 out（调用方保证 out 容量 ≥ expectedSize）。
     * 反序列化热路径用 thread_local 暂存缓冲区复用，避免每区块 24 次 16KB 堆分配。
     *
     * @param compressedData 压缩数据
     * @param compressedSize 压缩数据大小
     * @param out 输出缓冲区（容量必须 ≥ expectedSize）
     * @param expectedSize 预期解压缩大小
     * @return 成功返回 ok；失败返回 DecompressionFailed
     */
    [[nodiscard]] static Result<void> decompressInto(
        const u8* compressedData, size_t compressedSize, u8* out, size_t expectedSize);

    // ========================================================================
    // 常量
    // ========================================================================

    /// 未压缩的方块状态数据大小（4096 * 4字节）
    static constexpr size_t UNCOMPRESSED_BLOCK_STATES_SIZE = SectionData::VOLUME * sizeof(u32);

    /// 生物群系数据大小（64字节）
    static constexpr size_t BIOME_DATA_SIZE = SectionData::BIOME_COUNT * sizeof(BiomeId);

    /// 光照数据大小（2048字节）
    static constexpr size_t LIGHT_DATA_SIZE = SectionData::VOLUME / 2;

    /// 最大压缩数据大小
    static constexpr size_t MAX_COMPRESSED_SIZE = UNCOMPRESSED_BLOCK_STATES_SIZE * 2;
};

} // namespace mc::world::storage
