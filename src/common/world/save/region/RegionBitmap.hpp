#pragma once

#include "../../../core/Types.hpp"
#include <vector>
#include <cstdint>

namespace mc::world::save::region {

/**
 * @brief Region 文件扇区位图
 *
 * 用于跟踪 Region 文件中的扇区使用情况。
 * 每个 Region 文件最多有 1024 个区块，每个扇区 4096 字节。
 *
 * 参考 MC 1.16.5 RegionBitmap.java
 *
 * ## 使用示例
 * ```cpp
 * RegionBitmap bitmap;
 *
 * // 分配 3 个连续扇区
 * u32 sector = bitmap.allocate(3);
 * if (sector != 0) {
 *     // 成功分配，sector 是起始扇区号
 * }
 *
 * // 释放扇区
 * bitmap.free(sector, 3);
 * ```
 */
class RegionBitmap {
public:
    /// 扇区大小（字节）
    static constexpr u32 SECTOR_SIZE = 4096;

    /// 最大扇区数（2GB / 4KB = 524288，但实际受限于文件大小）
    static constexpr u32 MAX_SECTORS = 65536;

    /**
     * @brief 构造空的位图
     */
    RegionBitmap();

    /**
     * @brief 从文件大小构造位图
     *
     * @param fileSize 文件大小（字节）
     */
    explicit RegionBitmap(u64 fileSize);

    /**
     * @brief 分配连续扇区
     *
     * @param count 需要的扇区数量
     * @return 起始扇区号，如果无法分配返回 0
     *
     * @note 返回 0 表示分配失败，因为扇区 0-1 保留给文件头
     */
    [[nodiscard]] u32 allocate(u32 count);

    /**
     * @brief 释放扇区
     *
     * @param sector 起始扇区号
     * @param count 扇区数量
     */
    void free(u32 sector, u32 count);

    /**
     * @brief 标记扇区为已使用
     *
     * @param sector 起始扇区号
     * @param count 扇区数量
     */
    void markUsed(u32 sector, u32 count);

    /**
     * @brief 检查扇区是否已使用
     *
     * @param sector 扇区号
     * @return 如果已使用返回 true
     */
    [[nodiscard]] bool isUsed(u32 sector) const;

    /**
     * @brief 获取已使用的扇区数量
     */
    [[nodiscard]] u32 usedSectorCount() const;

    /**
     * @brief 获取总扇区数量
     */
    [[nodiscard]] u32 totalSectorCount() const;

    /**
     * @brief 获取空闲扇区数量
     */
    [[nodiscard]] u32 freeSectorCount() const;

    /**
     * @brief 清空位图
     */
    void clear();

    /**
     * @brief 调整位图大小
     *
     * @param newSectorCount 新的扇区数量
     */
    void resize(u32 newSectorCount);

private:
    /// 扇区使用位图（true = 已使用）
    std::vector<bool> m_used;

    /// 总扇区数
    u32 m_totalSectors = 0;

    /// 已使用扇区数
    u32 m_usedSectors = 0;
};

} // namespace mc::world::save::region
