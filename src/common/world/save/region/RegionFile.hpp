#pragma once

#include "CompressionType.hpp"
#include "RegionBitmap.hpp"
#include "../io/CompressionUtil.hpp"
#include "../io/FileUtil.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include "../../../world/chunk/ChunkPos.hpp"
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace mc::world::save::region {

/**
 * @brief Region 文件操作 (.mca)
 *
 * 每个 Region 文件存储 32x32 = 1024 个区块。
 * 文件结构：
 * - 头部 8KB：1024 个偏移条目 + 1024 个时间戳
 * - 数据区：压缩的区块 NBT 数据
 *
 * 参考 MC 1.16.5 RegionFile.java
 *
 * ## 使用示例
 * ```cpp
 * // 打开或创建 Region 文件
 * auto result = RegionFile::open("region/r.0.0.mca");
 * if (result.success()) {
 *     auto& regionFile = result.value();
 *
 *     // 读取区块
 *     auto chunkResult = regionFile->readChunk(0, 0);
 *     if (chunkResult.success() && chunkResult.value().has_value()) {
 *         // 处理区块数据
 *         auto& nbt = chunkResult.value().value();
 *     }
 *
 *     // 写入区块
 *     regionFile->writeChunk(0, 0, *chunkNbt);
 * }
 * ```
 *
 * ## 线程安全
 *
 * 所有公共方法都是线程安全的。
 */
class RegionFile {
public:
    /// 扇区大小（字节）
    static constexpr u32 SECTOR_SIZE = 4096;

    /// 头部大小（字节）
    static constexpr u32 HEADER_SIZE = 8192;

    /// 每个 Region 的区块数
    static constexpr u32 CHUNKS_PER_SIDE = 32;
    static constexpr u32 CHUNKS_PER_REGION = 1024;

    /// 最大扇区数（文件最大约 256MB）
    static constexpr u32 MAX_SECTORS = 65536;

    /// 大区块阈值（超过此大小使用外部文件）
    static constexpr u32 LARGE_CHUNK_THRESHOLD = SECTOR_SIZE * 256;  // 1MB

    /**
     * @brief 打开或创建 Region 文件
     *
     * @param path 文件路径
     * @param sync 是否同步写入（默认 false）
     * @return 成功返回 RegionFile，失败返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<RegionFile>>
    open(const std::filesystem::path& path, bool sync = false);

    ~RegionFile();

    // 禁止拷贝
    RegionFile(const RegionFile&) = delete;
    RegionFile& operator=(const RegionFile&) = delete;

    // 允许移动
    RegionFile(RegionFile&& other) noexcept;
    RegionFile& operator=(RegionFile&& other) noexcept;

    // ========== 区块操作 ==========

    /**
     * @brief 读取区块数据
     *
     * @param localX Region 内区块 X (0-31)
     * @param localZ Region 内区块 Z (0-31)
     * @return 成功返回 NBT 数据（区块不存在返回 nullopt），失败返回错误
     */
    [[nodiscard]] Result<std::optional<nbt::CompoundTag>>
    readChunk(u32 localX, u32 localZ);

    /**
     * @brief 写入区块数据
     *
     * @param localX Region 内区块 X (0-31)
     * @param localZ Region 内区块 Z (0-31)
     * @param nbt 区块 NBT 数据
     * @return 成功返回 void，失败返回错误
     */
    Result<void> writeChunk(u32 localX, u32 localZ, const nbt::CompoundTag& nbt);

    /**
     * @brief 检查区块是否存在
     *
     * @param localX Region 内区块 X (0-31)
     * @param localZ Region 内区块 Z (0-31)
     * @return 如果区块存在返回 true
     */
    [[nodiscard]] bool hasChunk(u32 localX, u32 localZ) const;

    /**
     * @brief 删除区块（标记为空）
     *
     * @param localX Region 内区块 X (0-31)
     * @param localZ Region 内区块 Z (0-31)
     * @return 成功返回 void，失败返回错误
     */
    Result<void> deleteChunk(u32 localX, u32 localZ);

    // ========== 时间戳 ==========

    /**
     * @brief 获取区块最后修改时间
     *
     * @param localX Region 内区块 X (0-31)
     * @param localZ Region 内区块 Z (0-31)
     * @return Unix 时间戳（秒）
     */
    [[nodiscard]] u32 getTimestamp(u32 localX, u32 localZ) const;

    // ========== 同步 ==========

    /**
     * @brief 强制同步到磁盘
     *
     * @return 成功返回 void，失败返回错误
     */
    Result<void> flush();

    /**
     * @brief 关闭文件
     */
    void close();

    // ========== 文件信息 ==========

    /**
     * @brief 获取文件路径
     */
    [[nodiscard]] std::filesystem::path path() const { return m_path; }

    /**
     * @brief 获取 Region 坐标
     */
    [[nodiscard]] i32 regionX() const { return m_regionX; }
    [[nodiscard]] i32 regionZ() const { return m_regionZ; }

    /**
     * @brief 获取已使用的区块数
     */
    [[nodiscard]] u32 usedChunkCount() const { return m_usedChunks; }

private:
    /**
     * @brief 私有构造函数
     */
    explicit RegionFile(const std::filesystem::path& path, bool sync);

    /**
     * @brief 初始化文件（创建或加载）
     */
    Result<void> initialize();

    /**
     * @brief 读取头部
     */
    Result<void> readHeader();

    /**
     * @brief 写入头部
     */
    Result<void> writeHeader();

    /**
     * @brief 计算区块在头部中的索引
     */
    [[nodiscard]] static u32 getChunkIndex(u32 localX, u32 localZ) {
        return (localX & 31) + (localZ & 31) * CHUNKS_PER_SIDE;
    }

    /**
     * @brief 从偏移条目获取扇区偏移
     */
    [[nodiscard]] static u32 getSectorOffset(u32 entry) {
        return (entry >> 8) & 0xFFFFFF;
    }

    /**
     * @brief 从偏移条目获取扇区数量
     */
    [[nodiscard]] static u32 getSectorCount(u32 entry) {
        return entry & 0xFF;
    }

    /**
     * @brief 创建偏移条目
     */
    [[nodiscard]] static u32 makeOffsetEntry(u32 sectorOffset, u32 sectorCount) {
        return (sectorOffset << 8) | (sectorCount & 0xFF);
    }

    /**
     * @brief 读取原始区块数据
     */
    Result<std::optional<std::vector<u8>>>
    readRawChunk(u32 index);

    /**
     * @brief 写入原始区块数据
     */
    Result<void> writeRawChunk(u32 index, const std::vector<u8>& data, CompressionType compression);

    /**
     * @brief 从文件路径解析 Region 坐标
     */
    static bool parseRegionCoords(const std::filesystem::path& path, i32& x, i32& z);

    // ========== 成员变量 ==========

    std::filesystem::path m_path;       ///< 文件路径
    i32 m_regionX = 0;                  ///< Region X 坐标
    i32 m_regionZ = 0;                  ///< Region Z 坐标
    bool m_sync = false;                ///< 是否同步写入
    bool m_closed = false;              ///< 是否已关闭

    // 文件句柄（使用 FILE* 以简化跨平台）
    FILE* m_file = nullptr;

    // 头部数据
    std::vector<u32> m_offsets;         ///< 偏移表（1024 条）
    std::vector<u32> m_timestamps;      ///< 时间戳表（1024 条）

    // 扇区位图
    RegionBitmap m_bitmap;

    // 已使用区块数
    u32 m_usedChunks = 0;

    // 线程安全锁
    mutable std::mutex m_mutex;
};

} // namespace mc::world::save::region
