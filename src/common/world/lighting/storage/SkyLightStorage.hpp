#pragma once

#include "SectionLightStorage.hpp"
#include "LightDataMap.hpp"
#include <unordered_map>
#include <unordered_set>

namespace mc {

/**
 * @brief 天空光照数据映射
 *
 * 管理天空光照数据，包括表面区块段追踪。
 * 参考: net.minecraft.world.lighting.SkyLightStorage.StorageMap
 */
class SkyLightDataMap : public LightDataMap<SkyLightDataMap> {
public:
    SkyLightDataMap();
    explicit SkyLightDataMap(
        std::unordered_map<i64, NibbleArray> arrays,
        std::unordered_map<i64, i32> surfaceSections,
        i32 minY);

    /**
     * @brief 复制数据映射
     */
    [[nodiscard]] SkyLightDataMap copy() const;

    /**
     * @brief 获取表面区块段映射
     */
    [[nodiscard]] std::unordered_map<i64, i32>& surfaceSections();
    [[nodiscard]] const std::unordered_map<i64, i32>& surfaceSections() const;

    /**
     * @brief 获取最小Y值
     */
    [[nodiscard]] i32 minY() const;
    void setMinY(i32 minY);

    /**
     * @brief 获取表面区块段高度（默认值）
     */
    [[nodiscard]] i32 getSurfaceHeight(i64 columnPos) const;
    void setSurfaceHeight(i64 columnPos, i32 height);
    void removeSurfaceHeight(i64 columnPos);

private:
    i32 m_minY;
    std::unordered_map<i64, i32> m_surfaceSections;
};

/**
 * @brief 天空光照存储
 *
 * 管理天空光照的数据存储，包括表面区块段追踪。
 * 天空光照从天空向下传播，需要追踪每个区块列的最高非空区块段。
 *
 * 参考: net.minecraft.world.lighting.SkyLightStorage
 */
class SkyLightStorage : public SectionLightStorage<SkyLightDataMap> {
public:
    /**
     * @brief 构造函数
     * @param provider 区块光照提供者
     */
    explicit SkyLightStorage(IChunkLightProvider* provider);

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

    /**
     * @brief 启用/禁用区块列
     *
     * @param columnPos 区块列位置（不含Y坐标）
     * @param enabled 是否启用
     */
    void setColumnEnabled(i64 columnPos, bool enabled) override;

    /**
     * @brief 检查区块段是否启用
     */
    [[nodiscard]] bool isSectionEnabled(i64 sectionPos) const;

    /**
     * @brief 检查区块段是否在世界之上（天空可见）
     *
     * @param sectionPos 区块段位置编码
     * @return 如果区块段在表面之上返回true
     */
    [[nodiscard]] bool isAboveWorld(i64 sectionPos) const;

    /**
     * @brief 检查位置是否在表面区块段顶部
     *
     * 用于判断天空光照是否可以直接到达该位置。
     */
    [[nodiscard]] bool isAtSurfaceTop(i64 worldPos) const;

    /**
     * @brief 检查是否有待处理的更新
     */
    [[nodiscard]] bool hasSectionsToUpdate() const override;

    /**
     * @brief 检查Y坐标是否在底部之上
     */
    [[nodiscard]] bool isAboveBottom(i32 sectionY) const;

    /**
     * @brief 处理区块段更新
     *
     * @param engine 光照引擎
     * @param updateSkyLight 是否更新天空光照
     * @param updateBlockLight 是否更新方块光照
     * @return 剩余更新配额
     */
    template<typename E>
    i32 updateSections(E* engine, i32 remainingUpdates, bool updateSkyLight, bool updateBlockLight);

protected:
    /**
     * @brief 添加区块段
     */
    void addSection(i64 sectionPos) override;

    /**
     * @brief 移除区块段
     */
    void removeSection(i64 sectionPos) override;

    /**
     * @brief 获取或创建光照数组
     */
    [[nodiscard]] NibbleArray getOrCreateArray(i64 sectionPos);

private:
    /**
     * @brief 调度完整更新
     */
    void scheduleFullUpdate(i64 sectionPos);

    /**
     * @brief 调度表面更新
     */
    void scheduleSurfaceUpdate(i64 sectionPos);

    /**
     * @brief 更新待处理标志
     */
    void updateHasPendingUpdates();

    /**
     * @brief 世界位置编码
     */
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z) {
        u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFLL);
        u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFLL);
        u64 uy = static_cast<u64>(y) & 0xFFF;
        return (ux << 38) | (uz << 12) | uy;
    }

    // 有光照的区块段
    std::unordered_set<i64> m_sectionsWithLight;

    // 待添加的区块段
    std::unordered_set<i64> m_pendingAdditions;

    // 待移除的区块段
    std::unordered_set<i64> m_pendingRemovals;

    // 启用的区块列
    std::unordered_set<i64> m_enabledColumns;

    // 是否有待处理更新
    bool m_hasPendingUpdates = false;

    // 水平方向（NORTH, SOUTH, WEST, EAST）
    static constexpr Direction HORIZONTAL_DIRECTIONS[4] = {
        Direction::North, Direction::South, Direction::West, Direction::East
    };
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename E>
i32 SkyLightStorage::updateSections(E* engine, i32 remainingUpdates, bool updateSkyLight, bool updateBlockLight) {
    // 先调用父类处理
    (void)updateBlockLight;  // 天空光照存储不处理方块光照

    if (!updateSkyLight) {
        return remainingUpdates;
    }

    // 处理待添加的区块段
    if (!m_pendingAdditions.empty()) {
        for (i64 sectionPos : m_pendingAdditions) {
            if (!m_pendingRemovals.count(sectionPos) &&
                m_sectionsWithLight.insert(sectionPos).second) {

                // 标记为脏
                m_dirtyCachedSections.insert(sectionPos);

                // 填充为全亮
                NibbleArray* array = getArray(sectionPos, true);
                if (array != nullptr) {
                    array->fill(15);
                }

                // 新区块段，从顶部开始调度光照更新
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
    }

    m_pendingAdditions.clear();

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
    }

    m_pendingRemovals.clear();
    m_hasPendingUpdates = false;

    // 仅处理区块段状态与调度，不直接消耗传播配额
    return remainingUpdates;
}

} // namespace mc
