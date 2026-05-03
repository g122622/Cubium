#include "SaveManager.hpp"
#include "perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

SaveManager::SaveManager(WorldStorageService& storage)
    : m_storage(storage)
    , m_autoSave(std::make_unique<AutoSave>(storage))
{
}

SaveManager::~SaveManager()
{
    shutdown();
}

// ============================================================================
// 生命周期
// ============================================================================

void SaveManager::initialize(const AutoSaveConfig& config)
{
    if (m_initialized) {
        return;
    }

    m_autoSave = std::make_unique<AutoSave>(m_storage);
    m_autoSave->setConfig(config);

    m_initialized = true;
    spdlog::info("SaveManager initialized");
}

void SaveManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    // 停止自动保存
    if (m_autoSave) {
        m_autoSave->stop();
    }

    // 保存所有脏数据
    auto result = saveNow();
    if (result.failed()) {
        spdlog::error("Failed to save dirty data during shutdown: {}",
                      result.error().message());
    }

    m_initialized = false;
    spdlog::info("SaveManager shutdown complete");
}

void SaveManager::startAutoSave()
{
    if (m_autoSave) {
        m_autoSave->start();
    }
}

void SaveManager::stopAutoSave()
{
    if (m_autoSave) {
        m_autoSave->stop();
    }
}

bool SaveManager::isAutoSaveRunning() const
{
    return m_autoSave && m_autoSave->isRunning();
}

// ============================================================================
// 脏追踪
// ============================================================================

void SaveManager::markDirty(const SectionKey& key)
{
    m_dirtyTracker.markDirty(key);
}

void SaveManager::markDirtyBatch(const std::vector<SectionKey>& keys)
{
    m_dirtyTracker.markDirtyBatch(keys);
}

size_t SaveManager::dirtyCount() const
{
    return m_dirtyTracker.dirtyCount();
}

std::vector<SectionKey> SaveManager::getDirtyKeys() const
{
    return m_dirtyTracker.getDirtyKeys();
}

// ============================================================================
// 保存操作
// ============================================================================

Result<size_t> SaveManager::saveNow()
{
    MC_TRACE_EVENT("storage.save", "SaveManager::saveNow");

    if (!m_storage.isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    // 刷新所有维度的脏Section
    auto result = m_storage.flushAllDirty();
    if (result.success()) {
        // 清除脏标记
        m_dirtyTracker.clearAll();
    }
    return result;
}

Result<size_t> SaveManager::saveNowWithSnapshot(const std::string& snapshotName)
{
    MC_TRACE_EVENT("storage.save", "SaveManager::saveNowWithSnapshot");

    if (!m_storage.isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    if (!m_autoSave) {
        return Error(ErrorCode::InvalidState, "AutoSave not initialized");
    }

    auto result = m_autoSave->saveNowWithSnapshot(snapshotName);
    if (result.success()) {
        m_dirtyTracker.clearAll();
    }
    return result;
}

Result<size_t> SaveManager::saveAll()
{
    MC_TRACE_EVENT("storage.save", "SaveManager::saveAll");

    if (!m_storage.isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    // 强制保存所有缓存中的Section
    // 这需要遍历所有SectionManager并保存所有Section
    size_t totalSaved = 0;

    for (auto dim : m_storage.getOpenDimensions()) {
        auto& sectionMgr = m_storage.sectionManager(dim);
        auto result = sectionMgr.saveAll();
        if (result.failed()) {
            return result.error();
        }
        totalSaved += result.value();
    }

    // 清除脏标记
    m_dirtyTracker.clearAll();

    spdlog::info("SaveManager saved {} sections", totalSaved);
    return totalSaved;
}

// ============================================================================
// 主循环
// ============================================================================

void SaveManager::tick(u64 tickCount)
{
    if (m_autoSave && m_autoSave->isRunning()) {
        m_autoSave->tick(tickCount);
    }
}

} // namespace mc::world::storage
