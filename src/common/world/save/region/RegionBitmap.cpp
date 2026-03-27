#include "RegionBitmap.hpp"

namespace mc::world::save::region {

RegionBitmap::RegionBitmap()
    : m_used(MAX_SECTORS, false)
    , m_totalSectors(2)  // 前两个扇区保留给文件头
    , m_usedSectors(2)   // 头部标记为已使用
{
    // 标记头部扇区为已使用
    m_used[0] = true;
    m_used[1] = true;
}

RegionBitmap::RegionBitmap(u64 fileSize)
    : RegionBitmap()
{
    // 计算文件包含的扇区数
    u32 sectors = static_cast<u32>((fileSize + SECTOR_SIZE - 1) / SECTOR_SIZE);
    if (sectors > MAX_SECTORS) {
        sectors = MAX_SECTORS;
    }
    resize(sectors);
}

u32 RegionBitmap::allocate(u32 count) {
    if (count == 0) {
        return 0;
    }

    // 从扇区 2 开始搜索（前两个扇区是头部）
    u32 freeStart = 0;
    u32 freeCount = 0;

    for (u32 i = 2; i < m_totalSectors; ++i) {
        if (!m_used[i]) {
            if (freeCount == 0) {
                freeStart = i;
            }
            ++freeCount;

            // 找到足够的连续空闲扇区
            if (freeCount >= count) {
                // 标记为已使用
                markUsed(freeStart, count);
                return freeStart;
            }
        } else {
            // 重置搜索
            freeCount = 0;
        }
    }

    // 没有找到足够的连续空间，扩展文件
    u32 newSector = m_totalSectors;
    u32 newTotalSectors = m_totalSectors + count;

    if (newTotalSectors > MAX_SECTORS) {
        return 0;  // 超出最大限制
    }

    resize(newTotalSectors);
    markUsed(newSector, count);
    return newSector;
}

void RegionBitmap::free(u32 sector, u32 count) {
    if (sector < 2) {
        return;  // 不能释放头部扇区
    }

    for (u32 i = 0; i < count && (sector + i) < m_totalSectors; ++i) {
        if (m_used[sector + i]) {
            m_used[sector + i] = false;
            --m_usedSectors;
        }
    }
}

void RegionBitmap::markUsed(u32 sector, u32 count) {
    for (u32 i = 0; i < count && (sector + i) < m_totalSectors; ++i) {
        if (!m_used[sector + i]) {
            m_used[sector + i] = true;
            ++m_usedSectors;
        }
    }
}

bool RegionBitmap::isUsed(u32 sector) const {
    if (sector >= m_totalSectors) {
        return false;
    }
    return m_used[sector];
}

u32 RegionBitmap::usedSectorCount() const {
    return m_usedSectors;
}

u32 RegionBitmap::totalSectorCount() const {
    return m_totalSectors;
}

u32 RegionBitmap::freeSectorCount() const {
    return m_totalSectors - m_usedSectors;
}

void RegionBitmap::clear() {
    std::fill(m_used.begin(), m_used.end(), false);
    m_used[0] = true;  // 头部始终标记为已使用
    m_used[1] = true;
    m_totalSectors = 2;
    m_usedSectors = 2;
}

void RegionBitmap::resize(u32 newSectorCount) {
    if (newSectorCount > MAX_SECTORS) {
        newSectorCount = MAX_SECTORS;
    }

    if (newSectorCount > m_totalSectors) {
        // 扩展：新扇区标记为空闲
        m_used.resize(newSectorCount, false);
    } else if (newSectorCount < m_totalSectors) {
        // 收缩：计算被移除的已使用扇区数
        for (u32 i = newSectorCount; i < m_totalSectors; ++i) {
            if (m_used[i]) {
                --m_usedSectors;
            }
        }
        m_used.resize(newSectorCount);
    }

    m_totalSectors = newSectorCount;
}

} // namespace mc::world::save::region
