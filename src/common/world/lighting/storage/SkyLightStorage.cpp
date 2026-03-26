#include "SkyLightStorage.hpp"
#include "../engine/LightEngineUtils.hpp"
#include <climits>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// SkyLightStorage 实现
// ============================================================================

SkyLightStorage::SkyLightStorage(IChunkLightProvider* provider)
    : SectionLightStorage<SkyLightDataMap>(LightType::SKY, provider, SkyLightDataMap()) {
}

u8 SkyLightStorage::getLightOrDefault(i64 worldPos) const {
    i64 sectionPos = worldToSectionPos(worldPos);
    const SWMRNibbleArray* array = getArray(sectionPos, true);

    // 如果数组不存在或为空，返回15（天空光照默认值）
    if (array == nullptr || array->isNullUpdating() || array->isUninitializedUpdating()) {
        return 15;
    }

    i32 x, localY, z;
    LightEngineUtils::extractNibbleIndices(worldPos, x, localY, z);

    return array->getUpdating(x, localY, z);
}

u8 SkyLightStorage::getLight(i64 worldPos) const {
    i64 sectionPos = worldToSectionPos(worldPos);
    const SWMRNibbleArray* array = getArray(sectionPos, true);

    if (array == nullptr || array->isNullUpdating()) {
        return 0;
    }

    i32 x, localY, z;
    LightEngineUtils::extractNibbleIndices(worldPos, x, localY, z);

    return array->getUpdating(x, localY, z);
}

void SkyLightStorage::setLight(i64 worldPos, u8 light) {
    i64 sectionPos = worldToSectionPos(worldPos);

    SWMRNibbleArray* array = m_cachedLightData.getArray(sectionPos);
    if (array == nullptr) {
        array = &m_cachedLightData.getOrCreateArray(sectionPos);
        array->setZero();
    }

    m_dirtyCachedSections.insert(sectionPos);

    i32 x, localY, z;
    LightEngineUtils::extractNibbleIndices(worldPos, x, localY, z);

    array->set(x, localY, z, light);

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                SectionPos pos = SectionPos::fromLong(sectionPos);
                i64 neighborPos = SectionPos(pos.x + dx, pos.y + dy, pos.z + dz).toLong();
                m_changedLightPositions.insert(neighborPos);
            }
        }
    }
}

void SkyLightStorage::setColumnEnabled(i64 columnPos, bool enabled) {
    processAllLevelUpdates();

    if (enabled && m_enabledColumns.insert(columnPos).second) {
        // 新启用的列
    } else if (!enabled) {
        m_enabledColumns.erase(columnPos);
    }
}

bool SkyLightStorage::isSectionEnabled(i64 sectionPos) const {
    i64 columnPos = SectionPos::fromLong(sectionPos).toColumnLong();
    return m_enabledColumns.count(columnPos) > 0;
}

bool SkyLightStorage::isAboveWorld(i64 sectionPos) const {
    SectionPos pos = SectionPos::fromLong(sectionPos);
    i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(pos.toColumnLong());
    return pos.y >= surfaceHeight;
}

bool SkyLightStorage::isAtSurfaceTop(i64 worldPos) const {
    i32 y = static_cast<i32>(worldPos & 0xFFF);
    if ((y & 0xF) != 15) {
        return false;
    }

    i64 sectionPos = worldToSectionPos(worldPos);
    i64 columnPos = SectionPos::fromLong(sectionPos).toColumnLong();

    if (m_enabledColumns.count(columnPos) == 0) {
        return false;
    }

    i32 surfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);
    return surfaceHeight == (y + 16) / 16;
}

bool SkyLightStorage::hasSectionsToUpdate() const {
    return !m_noLightSections.empty() || m_newArrays.size() > 0 || m_hasPendingUpdates;
}

bool SkyLightStorage::isAboveBottom(i32 sectionY) const {
    return sectionY >= 0;
}

void SkyLightStorage::addSection(i64 sectionPos) {
    SectionPos pos = SectionPos::fromLong(sectionPos);
    i64 columnPos = pos.toColumnLong();

    // 更新表面高度
    i32 currentSurfaceHeight = m_cachedLightData.getSurfaceHeight(columnPos);
    if (pos.y + 1 > currentSurfaceHeight) {
        m_cachedLightData.setSurfaceHeight(columnPos, pos.y + 1);
    }

    m_pendingAdditions.insert(sectionPos);
    m_pendingRemovals.erase(sectionPos);
    m_hasPendingUpdates = true;
}

void SkyLightStorage::removeSection(i64 sectionPos) {
    m_pendingRemovals.insert(sectionPos);
    m_pendingAdditions.erase(sectionPos);
    m_hasPendingUpdates = true;
}

void SkyLightStorage::scheduleFullUpdate(i64 sectionPos) {
    m_pendingAdditions.insert(sectionPos);
    m_pendingRemovals.erase(sectionPos);
}

void SkyLightStorage::scheduleSurfaceUpdate(i64 sectionPos) {
    m_pendingRemovals.insert(sectionPos);
    m_pendingAdditions.erase(sectionPos);
}

void SkyLightStorage::updateHasPendingUpdates() {
    m_hasPendingUpdates = !m_pendingAdditions.empty() || !m_pendingRemovals.empty();
}

constexpr Direction SkyLightStorage::HORIZONTAL_DIRECTIONS[4];

} // namespace mc
