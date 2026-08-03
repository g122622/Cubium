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

#include "AutoSave.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "world/storage/SingleLevelStorageManager.hpp"
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

AutoSave::AutoSave(SingleLevelStorageManager& storage)
    : m_storage(storage)
{}

AutoSave::~AutoSave()
{
    stop();
}

// ============================================================================
// 生命周期
// ============================================================================

void AutoSave::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running) {
        return;
    }
    m_running = true;
    spdlog::info("AutoSave started with interval {}ms, threshold {}", m_config.saveIntervalMs, m_config.dirtyThreshold);
}

void AutoSave::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) {
        return;
    }
    m_running = false;
    spdlog::info("AutoSave stopped");
}

// ============================================================================
// 配置
// ============================================================================

void AutoSave::setConfig(const AutoSaveConfig& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

// ============================================================================
// 主循环
// ============================================================================

void AutoSave::tick(u64 tickCount)
{
    AutoSaveConfig config;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_running) {
            return;
        }
        config = m_config;
    }

    if (!_shouldSave(tickCount)) {
        return;
    }

    // 执行保存
    auto result = _doSave(config.createSnapshotBeforeSave);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastSaveTick = tickCount;
        ++m_totalSaveCount;
        m_totalSectionsSaved += result.value();

        if (m_saveCallback) {
            m_saveCallback(result.value());
        }
    } else {
        spdlog::error("AutoSave failed: {}", result.error().message());
    }
}

// ============================================================================
// 手动操作
// ============================================================================

Result<size_t> AutoSave::saveNow()
{
    return _doSave(false);
}

Result<size_t> AutoSave::saveNowWithSnapshot(const std::string& snapshotName)
{
    return _doSave(true, snapshotName);
}

// ============================================================================
// 私有方法
// ============================================================================

bool AutoSave::_shouldSave(u64 tickCount) const
{
    AutoSaveConfig config;
    u64 lastSaveTick = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        config = m_config;
        lastSaveTick = m_lastSaveTick;
    }

    // 检查脏Section数量
    size_t dirtyCount = m_storage.getTotalDirtyCount();

    // 阈值触发
    if (dirtyCount >= config.dirtyThreshold) {
        spdlog::info("AutoSave triggered by threshold: {} >= {}", dirtyCount, config.dirtyThreshold);
        return true;
    }

    // 定时触发（假设 20 ticks/秒）
    // 1秒 = 20 ticks, 所以 saveIntervalMs / 1000 * 20 = saveIntervalMs / 50 ticks
    u64 ticksPerInterval = config.saveIntervalMs / 50;
    if (ticksPerInterval == 0) {
        ticksPerInterval = 1;
    }

    if (tickCount - lastSaveTick >= ticksPerInterval && dirtyCount > 0) {
        spdlog::info("AutoSave triggered by timer: {} ticks elapsed", tickCount - lastSaveTick);
        return true;
    }

    return false;
}

Result<size_t> AutoSave::_doSave(bool createSnapshot, const std::string& snapshotName)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Task, "AutoSave::doSave");

    AutoSaveConfig config;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        config = m_config;
    }

    // 可选：创建快照
    if (createSnapshot) {
        std::string name = snapshotName.empty() ? fmt::format("{}{}",
                                                      config.snapshotPrefix,
                                                      std::chrono::duration_cast<std::chrono::milliseconds>(
                                                          std::chrono::system_clock::now().time_since_epoch())
                                                          .count())
                                                : snapshotName;

        auto backupResult = m_storage.createBackup(name);
        if (backupResult.failed()) {
            spdlog::warn("Failed to create auto-save snapshot: {}", backupResult.error().message());
        } else {
            spdlog::info("Created auto-save snapshot: {}", name);
            _pruneOldSnapshots();
        }
    }

    // 保存所有脏数据
    auto result = m_storage.flushAllDirty();
    if (result.failed()) {
        return result.error();
    }

    return result;
}

void AutoSave::_pruneOldSnapshots()
{
    AutoSaveConfig config;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        config = m_config;
    }

    auto result = m_storage.pruneOldBackups(config.maxAutoSnapshots);
    if (result.success() && result.value() > 0) {
        spdlog::info("Pruned {} old snapshots", result.value());
    }
}

} // namespace mc::world::storage
