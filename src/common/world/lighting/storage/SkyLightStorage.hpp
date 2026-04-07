#pragma once

#include "SectionLightStorage.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {

/**
 * @brief 天空光照存储（使用 SWMR）
 *
 * 管理天空光照的数据存储，包括表面区块段追踪。
 * 天空光照从天空向下传播，需要追踪每个区块列的最高非空区块段。
 */
class SkyLightStorage : public SectionLightStorage<SkyLightDataMap> {
public:
    explicit SkyLightStorage(StarLightLightingProvider* provider);

    // ========================================================================
    // 光照访问
    // ========================================================================

    /**
     * @brief 获取指定位置的光照等级，如果不存在返回15（天空光照默认值）
     */
    [[nodiscard]] u8 getLightOrDefault(i64 worldPos) const override;

    /**
     * @brief 获取指定位置的实际光照等级
     */
    [[nodiscard]] u8 getLight(i64 worldPos) const;

    /**
     * @brief 设置指定位置的光照等级
     */
    void setLight(i64 worldPos, u8 light);

    // ========================================================================
    // 区块段管理
    // ========================================================================

    void setColumnEnabled(i64 columnPos, bool enabled) override;

    [[nodiscard]] bool isSectionEnabled(i64 sectionPos) const;

    [[nodiscard]] bool isAboveWorld(i64 sectionPos) const;

    [[nodiscard]] bool isAtSurfaceTop(i64 worldPos) const;

    [[nodiscard]] bool hasSectionsToUpdate() const override;

    [[nodiscard]] bool isAboveBottom(i32 sectionY) const;

    template<typename E>
    i32 updateSections(E* engine, i32 remainingUpdates, bool updateSkyLight, bool updateBlockLight);

protected:
    void addSection(i64 sectionPos) override;
    void removeSection(i64 sectionPos) override;

private:
    void scheduleFullUpdate(i64 sectionPos);
    void scheduleSurfaceUpdate(i64 sectionPos);
    void updateHasPendingUpdates();

    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z) {
        u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFLL);
        u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFLL);
        u64 uy = static_cast<u64>(y) & 0xFFF;
        return (ux << 38) | (uz << 12) | uy;
    }

    std::unordered_set<i64> m_sectionsWithLight;
    std::unordered_set<i64> m_pendingAdditions;
    std::unordered_set<i64> m_pendingRemovals;
    std::unordered_set<i64> m_enabledColumns;
    bool m_hasPendingUpdates = false;

    static constexpr Direction HORIZONTAL_DIRECTIONS[4] = {
        Direction::North, Direction::South, Direction::West, Direction::East
    };
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename E>
i32 SkyLightStorage::updateSections(E* engine, i32 remainingUpdates, bool updateSkyLight, bool updateBlockLight) {
    (void)updateBlockLight;

    if (!updateSkyLight) {
        return remainingUpdates;
    }

    // 处理待添加的区块段
    if (!m_pendingAdditions.empty()) {
        for (i64 sectionPos : m_pendingAdditions) {
            if (!m_pendingRemovals.count(sectionPos) &&
                m_sectionsWithLight.insert(sectionPos).second) {

                m_dirtyCachedSections.insert(sectionPos);

                SWMRNibbleArray* array = getArray(sectionPos, true);
                if (array != nullptr) {
                    array->setFull();
                }

                SectionPos pos = SectionPos::fromLong(sectionPos);
                i32 worldX = pos.x * 16;
                i32 worldY = pos.y * 16;
                i32 worldZ = pos.z * 16;

                for (i32 localX = 0; localX < 16; ++localX) {
                    for (i32 localZ = 0; localZ < 16; ++localZ) {
                        i64 blockPos = packPos(worldX + localX, worldY + 15, worldZ + localZ);
                        engine->scheduleUpdate(blockPos);
                    }
                }
            }
        }
        m_pendingAdditions.clear();
    }

    // 处理待移除的区块段
    if (!m_pendingRemovals.empty()) {
        for (i64 sectionPos : m_pendingRemovals) {
            if (m_sectionsWithLight.erase(sectionPos) > 0 && hasSection(sectionPos)) {
                SectionPos pos = SectionPos::fromLong(sectionPos);
                i32 worldX = pos.x * 16;
                i32 worldY = pos.y * 16;
                i32 worldZ = pos.z * 16;

                for (i32 localX = 0; localX < 16; ++localX) {
                    for (i32 localZ = 0; localZ < 16; ++localZ) {
                        i64 blockPos = packPos(worldX + localX, worldY + 15, worldZ + localZ);
                        engine->scheduleUpdate(blockPos);
                    }
                }
            }
        }
        m_pendingRemovals.clear();
    }

    m_hasPendingUpdates = false;
    return remainingUpdates;
}

} // namespace mc
