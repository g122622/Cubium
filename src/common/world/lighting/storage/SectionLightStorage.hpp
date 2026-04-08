#pragma once

#include "SWMRLightDataMap.hpp"
#include "../LightType.hpp"
#include "../IChunkLightProvider.hpp"
#include "../engine/LightEngineUtils.hpp"
#include "../../chunk/ChunkPos.hpp"
#include "common/perfetto/TraceEvents.hpp"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <climits>

namespace mc {

/**
 * @brief 方块光照数据映射（使用 SWMR）
 */
class BlockLightDataMap : public SWMRLightDataMap<BlockLightDataMap> {
public:
    BlockLightDataMap() = default;
};

/**
 * @brief 天空光照数据映射（使用 SWMR）
 */
class SkyLightDataMap : public SWMRLightDataMap<SkyLightDataMap> {
public:
    SkyLightDataMap() = default;

    void setSurfaceHeight(i64 columnPos, i32 height) {
        m_surfaceHeights[columnPos] = height;
    }

    [[nodiscard]] i32 getSurfaceHeight(i64 columnPos) const {
        auto it = m_surfaceHeights.find(columnPos);
        return it != m_surfaceHeights.end() ? it->second : 0;
    }

    [[nodiscard]] bool hasSurfaceHeight(i64 columnPos) const {
        return m_surfaceHeights.find(columnPos) != m_surfaceHeights.end();
    }

    void removeSurfaceHeight(i64 columnPos) {
        m_surfaceHeights.erase(columnPos);
    }

private:
    std::unordered_map<i64, i32> m_surfaceHeights;
};

/**
 * @brief 区块段光照存储基类（使用 SWMR）
 *
 * 使用 SWMRNibbleArray 实现单写多读的光照数据存储。
 */
template<typename M>
class SectionLightStorage {
public:
    SectionLightStorage(LightType type, StarLightLightingProvider* provider, M&& dataMap)
        : m_type(type)
        , m_chunkProvider(provider)
        , m_cachedLightData(std::move(dataMap)) {
    }

    virtual ~SectionLightStorage() = default;

    // ========================================================================
    // 区块段管理
    // ========================================================================

    [[nodiscard]] bool hasSection(i64 sectionPos) const {
        return m_cachedLightData.hasArray(sectionPos);
    }

    [[nodiscard]] SWMRNibbleArray* getArray(i64 sectionPos, bool useCache) {
        (void)useCache;
        return m_cachedLightData.getArray(sectionPos);
    }

    [[nodiscard]] const SWMRNibbleArray* getArray(i64 sectionPos, bool useCache) const {
        (void)useCache;
        return m_cachedLightData.getArray(sectionPos);
    }

    [[nodiscard]] SWMRNibbleArray* getArray(i64 sectionPos) {
        SWMRNibbleArray* cachedArray = m_cachedLightData.getArray(sectionPos);
        if (cachedArray != nullptr) {
            return cachedArray;
        }
        return m_newArrays.getArray(sectionPos);
    }

    void setData(i64 sectionPos, SWMRNibbleArray&& array, bool retain) {
        m_newArrays.setArray(sectionPos, std::move(array));
        if (!retain) {
            m_dirtyNewArrays.insert(sectionPos);
        }
    }

    void setData(i64 sectionPos, const NibbleArray& array, bool retain) {
        SWMRNibbleArray swmrArray = SWMRNibbleArray::fromData(array.data());
        m_newArrays.setArray(sectionPos, std::move(swmrArray));
        if (!retain) {
            m_dirtyNewArrays.insert(sectionPos);
        }
    }

    void updateSectionStatus(i64 sectionPos, bool isEmpty) {
        bool isActive = m_activeLightSections.count(sectionPos) > 0;

        if (!isActive && !isEmpty) {
            m_addedActiveSections.insert(sectionPos);
        }

        if (isActive && isEmpty) {
            m_addedEmptySections.insert(sectionPos);
        }
    }

    virtual void setColumnEnabled(i64 columnPos, bool enabled) {
        if (enabled) {
            m_enabledColumns.insert(columnPos);
        } else {
            m_enabledColumns.erase(columnPos);
        }
    }

    // ========================================================================
    // 光照访问
    // ========================================================================

    [[nodiscard]] virtual u8 getLightOrDefault(i64 worldPos) const = 0;

    // ========================================================================
    // 状态查询
    // ========================================================================

    [[nodiscard]] virtual bool hasSectionsToUpdate() const {
        return !m_noLightSections.empty() || m_newArrays.size() > 0;
    }

    void processAllLevelUpdates() {
        MC_TRACE_EVENT("server.lighting", "SectionLightStorage::processAllLevelUpdates",
                         "type", (m_type == LightType::SKY) ? "Sky" : "Block",
                         "newArrays", m_newArrays.size(),
                         "noLightSections", m_noLightSections.size(),
                         "addedActiveSections", m_addedActiveSections.size(),
                         "addedEmptySections", m_addedEmptySections.size());

        // 处理区块段状态变化
        if (!m_addedActiveSections.empty()) {
            for (i64 sectionPos : m_addedActiveSections) {
                if (m_activeLightSections.insert(sectionPos).second) {
                    addSection(sectionPos);
                }
            }
            m_addedActiveSections.clear();
        }

        if (!m_addedEmptySections.empty()) {
            for (i64 sectionPos : m_addedEmptySections) {
                if (m_activeLightSections.erase(sectionPos) > 0) {
                    removeSection(sectionPos);
                }
            }
            m_addedEmptySections.clear();
        }

        // 提交待写入的光照数组
        if (m_newArrays.size() > 0) {
            std::vector<i64> newSectionPositions;
            newSectionPositions.reserve(m_newArrays.size());

            for (auto it = m_newArrays.begin(); it != m_newArrays.end(); ++it) {
                m_cachedLightData.setArray(it->first, std::move(it->second));
                m_dirtyCachedSections.insert(it->first);
                m_changedLightPositions.insert(it->first);
                newSectionPositions.push_back(it->first);
            }

            for (i64 sectionPos : newSectionPositions) {
                m_newArrays.removeArray(sectionPos);
            }
        }

        // 处理待移除的无光照区块段
        if (!m_noLightSections.empty()) {
            for (i64 sectionPos : m_noLightSections) {
                m_cachedLightData.removeArray(sectionPos);
                m_dirtyCachedSections.insert(sectionPos);
                m_changedLightPositions.insert(sectionPos);
            }
            m_noLightSections.clear();
        }

        m_dirtyNewArrays.clear();
    }

    void updateAndNotify() {
        MC_TRACE_EVENT("server.lighting", "SectionLightStorage::updateAndNotify",
                         "type", (m_type == LightType::SKY) ? "Sky" : "Block");

        // 同步所有更新到可见侧
        m_cachedLightData.updateVisible();

        if (!m_dirtyCachedSections.empty()) {
            m_dirtyCachedSections.clear();
        }

        if (!m_changedLightPositions.empty()) {
            for (i64 pos : m_changedLightPositions) {
                m_chunkProvider->markLightChanged(m_type, SectionPos::fromLong(pos));
            }
            m_changedLightPositions.clear();
        }
    }

    [[nodiscard]] size_t getChangedPositionsCount() const {
        return m_changedLightPositions.size();
    }

protected:
    LightType m_type;
    StarLightLightingProvider* m_chunkProvider;
    M m_cachedLightData;
    M m_newArrays;

    std::unordered_set<i64> m_activeLightSections;
    std::unordered_set<i64> m_addedEmptySections;
    std::unordered_set<i64> m_addedActiveSections;
    std::unordered_set<i64> m_noLightSections;
    std::unordered_set<i64> m_enabledColumns;
    std::unordered_set<i64> m_dirtyCachedSections;
    std::unordered_set<i64> m_changedLightPositions;
    std::unordered_set<i64> m_dirtyNewArrays;
    std::unordered_set<i64> m_chunksToRetain;

    [[nodiscard]] static i64 worldToSectionPos(i64 worldPos) {
        return LightEngineUtils::worldToSectionPos(worldPos);
    }

    virtual void addSection(i64 sectionPos) {
        (void)sectionPos;
    }

    virtual void removeSection(i64 sectionPos) {
        (void)sectionPos;
    }
};

} // namespace mc
