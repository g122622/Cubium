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

#include "DirtyTracker.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <utility>

namespace mc::world::storage {

class SingleLevelStorageManager;

/**
 * @brief 自动保存配置
 */
struct AutoSaveConfig {
    /// 自动保存间隔（毫秒），默认5分钟
    u64 saveIntervalMs = 5 * 60 * 1000;

    /// 每次保存的Section数量上限
    size_t maxSectionsPerSave = 100;

    /// 脏Section阈值（超过此数量立即保存）
    size_t dirtyThreshold = 1000;

    /// 是否在保存前创建快照
    bool createSnapshotBeforeSave = false;

    /// 快照名称前缀
    std::string snapshotPrefix = "auto_";

    /// 自动快照保留数量
    size_t maxAutoSnapshots = 5;
};

/**
 * @brief 自动保存管理器
 *
 * 定时检查脏Section数量并自动保存。
 * 支持阈值触发和定时触发两种模式。
 *
 * 使用示例：
 * @code
 * AutoSave autoSave(storage);
 * autoSave.setConfig(config);
 * autoSave.start();
 *
 * // 在游戏tick中调用
 * autoSave.tick(tickCount);
 *
 * // 停止
 * autoSave.stop();
 * @endcode
 */
class AutoSave {
public:
    /**
     * @brief 保存回调类型
     */
    using SaveCallback = std::function<void(size_t savedCount)>;

    /**
     * @brief 构造函数
     * @param storage 存储服务引用
     */
    explicit AutoSave(SingleLevelStorageManager& storage);

    /**
     * @brief 析构函数
     */
    ~AutoSave();

    // 禁止拷贝
    AutoSave(const AutoSave&) = delete;
    AutoSave& operator=(const AutoSave&) = delete;

    // 禁止移动（有引用成员和 atomic）
    AutoSave(AutoSave&&) noexcept = delete;
    AutoSave& operator=(AutoSave&&) noexcept = delete;

    // ========== 生命周期 ==========

    /**
     * @brief 启动自动保存
     */
    void start();

    /**
     * @brief 停止自动保存
     */
    void stop();

    /**
     * @brief 检查是否正在运行
     */
    [[nodiscard]] bool isRunning() const { return m_running; }

    // ========== 配置 ==========

    /**
     * @brief 设置配置
     */
    void setConfig(const AutoSaveConfig& config);

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const AutoSaveConfig& config() const { return m_config; }

    // ========== 主循环 ==========

    /**
     * @brief 每tick调用
     *
     * 检查是否需要执行自动保存。
     *
     * @param tickCount 当前tick计数
     */
    void tick(u64 tickCount);

    // ========== 手动操作 ==========

    /**
     * @brief 手动触发保存
     *
     * 忽略定时器，立即保存所有脏Section。
     *
     * @return 保存的Section数量
     */
    Result<size_t> saveNow();

    /**
     * @brief 手动触发保存（带快照）
     *
     * 先创建快照，再保存。
     *
     * @param snapshotName 快照名称
     * @return 保存的Section数量
     */
    Result<size_t> saveNowWithSnapshot(const std::string& snapshotName);

    // ========== 回调设置 ==========

    /**
     * @brief 设置保存完成回调
     */
    void setSaveCallback(SaveCallback callback) { m_saveCallback = std::move(callback); }

    // ========== 统计 ==========

    /**
     * @brief 获取上次保存时间
     */
    [[nodiscard]] u64 lastSaveTick() const { return m_lastSaveTick; }

    /**
     * @brief 获取总保存次数
     */
    [[nodiscard]] u64 totalSaveCount() const { return m_totalSaveCount; }

    /**
     * @brief 获取总保存的Section数量
     */
    [[nodiscard]] u64 totalSectionsSaved() const { return m_totalSectionsSaved; }

private:
    /**
     * @brief 检查是否应该保存
     */
    [[nodiscard]] bool _shouldSave(u64 tickCount) const;

    /**
     * @brief 执行保存
     */
    Result<size_t> _doSave(bool createSnapshot, const std::string& snapshotName = "");

    /**
     * @brief 清理旧快照
     */
    void _pruneOldSnapshots();

private:
    SingleLevelStorageManager& m_storage;
    AutoSaveConfig m_config;
    std::atomic<bool> m_running{false};
    mutable std::mutex m_mutex;

    // 统计
    u64 m_lastSaveTick = 0;
    u64 m_totalSaveCount = 0;
    u64 m_totalSectionsSaved = 0;

    // 回调
    SaveCallback m_saveCallback;
};

} // namespace mc::world::storage
