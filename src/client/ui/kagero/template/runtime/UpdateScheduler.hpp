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

#include "client/ui/kagero/Types.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::client::ui::kagero::tpl::runtime {

/**
 * @brief 更新调度器
 *
 * 管理模板实例的增量更新，避免频繁刷新整个模板。
 * 支持批量更新、延迟更新和优先级调度。
 *
 * 使用回调函数而非直接依赖 TemplateInstance，以降低耦合。
 *
 * 时间模型：
 * - 外部通过 tick(currentMs) 推进调度器时间（毫秒级时间戳，通常来自 steady_clock）
 * - schedule() 时根据 m_deferredUpdate 与 m_batchDelayMs 计算任务到期时间 dueTimeMs
 * - executePending() / tick() 只执行 dueTimeMs <= 当前时间 的任务
 * - flush() 无视延迟立即执行所有待处理任务（用于屏幕关闭、强制刷新等场景）
 *
 * 批量延迟语义（trailing-edge debounce）：
 * - 同一路径的多次 schedule 会被 _deduplicatePaths() 合并，只保留最新任务
 * - 启用 deferredUpdate 时，每次 schedule 会把该路径的到期时间推迟 now + batchDelayMs
 * - 这意味着连续高频更新只会被处理一次（最后一次），避免重复刷新
 */
class UpdateScheduler {
public:
    /**
     * @brief 更新回调类型
     *
     * 参数为状态路径，返回是否更新成功
     */
    using UpdateCallback = std::function<bool(const std::string& path)>;

    /**
     * @brief 更新优先级
     */
    enum class Priority : u8 {
        High = 0,   ///< 高优先级（立即更新，但仍受延迟限制）
        Normal = 1, ///< 普通优先级（下一帧更新）
        Low = 2     ///< 低优先级（批量更新）
    };

    /**
     * @brief 更新任务
     */
    struct UpdateTask {
        std::string path;       ///< 状态路径
        Priority priority;      ///< 优先级
        u64 timestamp;          ///< 创建时间戳（逻辑序号，用于去重时取最新）
        u64 dueTimeMs;          ///< 到期时间（毫秒时间戳，到期后才能执行）
        bool cancelled = false; ///< 是否取消

        UpdateTask(std::string p, Priority pri, u64 ts, u64 dueMs)
            : path(std::move(p))
            , priority(pri)
            , timestamp(ts)
            , dueTimeMs(dueMs)
        {}
    };

    UpdateScheduler() = default;
    ~UpdateScheduler();

    // 禁止拷贝
    UpdateScheduler(const UpdateScheduler&) = delete;
    UpdateScheduler& operator=(const UpdateScheduler&) = delete;

    // 允许移动（注意：移动后回调仍捕获原 this，需由调用方重新绑定）
    UpdateScheduler(UpdateScheduler&&) noexcept = default;
    UpdateScheduler& operator=(UpdateScheduler&&) noexcept = default;

    // ========== 回调设置 ==========

    /**
     * @brief 设置更新回调
     *
     * @param callback 更新回调函数
     */
    void setUpdateCallback(UpdateCallback callback) { m_updateCallback = std::move(callback); }

    // ========== 任务调度 ==========

    /**
     * @brief 调度更新任务
     *
     * @param path 状态路径
     * @param priority 优先级
     * @return 任务ID
     */
    u64 schedule(const std::string& path, Priority priority);

    /**
     * @brief 取消更新任务
     */
    void cancel(u64 taskId);

    /**
     * @brief 取消所有指定路径的任务
     */
    void cancelByPath(const std::string& path);

    /**
     * @brief 取消所有任务
     */
    void cancelAll();

    // ========== 更新执行 ==========

    /**
     * @brief 执行所有待处理任务
     *
     * 若启用延迟更新（m_deferredUpdate=true），仅执行到期任务；
     * 若禁用延迟更新，执行所有未取消任务。
     *
     * @return 执行的任务数量
     */
    u32 executePending();

    /**
     * @brief 执行高优先级任务
     *
     * 受延迟更新限制，仅执行到期的高优先级任务。
     */
    u32 executeHighPriority();

    /**
     * @brief 执行普通优先级任务
     *
     * 受延迟更新限制，仅执行到期的普通优先级任务。
     */
    u32 executeNormalPriority();

    /**
     * @brief 执行低优先级任务
     *
     * 受延迟更新限制，仅执行到期的低优先级任务。
     */
    u32 executeLowPriority();

    /**
     * @brief 执行批量更新（合并相同路径的更新）
     *
     * 按 m_maxBatchSize 限制单次批量执行的最大路径数。
     * 受延迟更新限制，仅执行到期任务。
     *
     * @return 执行的任务数量
     */
    u32 executeBatch();

    /**
     * @brief 立即执行所有待处理任务（无视延迟）
     *
     * 用于屏幕关闭、强制刷新等需要立即同步的场景。
     * 执行顺序：High -> Normal -> Low，同路径去重。
     *
     * @return 执行的任务数量
     */
    u32 flush();

    /**
     * @brief 推进调度器时间并执行到期任务
     *
     * 由外部主循环每帧调用，传入当前毫秒时间戳。
     * 内部按 High -> Normal -> Low 顺序执行到期任务，并清理已取消任务。
     *
     * @param currentMs 当前毫秒时间戳（通常来自 TimeUtils::getCurrentTimeMs()）
     * @return 执行的任务数量
     */
    u32 tick(u64 currentMs);

    // ========== 配置 ==========

    /**
     * @brief 设置批量更新延迟（毫秒）
     *
     * 启用延迟更新后，每次 schedule 会把任务到期时间设为 now + delayMs。
     * 连续高频更新会被推迟到延迟结束后才执行，实现 trailing-edge debounce。
     */
    void setBatchDelay(u32 delayMs) { m_batchDelayMs = delayMs; }

    /**
     * @brief 获取批量更新延迟（毫秒）
     */
    [[nodiscard]] u32 batchDelay() const { return m_batchDelayMs; }

    /**
     * @brief 设置最大批量大小
     */
    void setMaxBatchSize(u32 size) { m_maxBatchSize = size; }

    /**
     * @brief 获取最大批量大小
     */
    [[nodiscard]] u32 maxBatchSize() const { return m_maxBatchSize; }

    /**
     * @brief 设置是否启用延迟更新
     *
     * 启用后，schedule() 会根据 batchDelayMs 计算到期时间，
     * executePending() / tick() 只执行到期任务。
     * 禁用后，所有任务立即执行（dueTimeMs 设为 0）。
     */
    void setDeferredUpdate(bool enabled) { m_deferredUpdate = enabled; }

    /**
     * @brief 是否启用延迟更新
     */
    [[nodiscard]] bool deferredUpdate() const { return m_deferredUpdate; }

    // ========== 状态查询 ==========

    /**
     * @brief 获取待处理任务数量（含未到期）
     */
    [[nodiscard]] u32 pendingCount() const;

    /**
     * @brief 检查是否有待处理任务
     */
    [[nodiscard]] bool hasPending() const { return pendingCount() > 0; }

    /**
     * @brief 获取指定优先级的待处理任务数量（含未到期）
     */
    [[nodiscard]] u32 pendingCount(Priority priority) const;

    /**
     * @brief 获取已到期但未执行的待处理任务数量
     *
     * 若禁用延迟更新，所有未取消任务都视为已到期。
     *
     * @param currentMs 当前毫秒时间戳
     */
    [[nodiscard]] u32 dueCount(u64 currentMs) const;

    /**
     * @brief 获取当前逻辑时间戳（用于 schedule 排序的序号）
     *
     * 注意：这是单调递增的逻辑序号，不是真实时间。
     * 真实时间通过 tick(currentMs) 由外部传入。
     */
    [[nodiscard]] u64 currentTimestamp() const;

    /**
     * @brief 获取调度器内部时钟（毫秒时间戳）
     *
     * 返回最近一次 tick(currentMs) 传入的时间戳。
     * 若从未调用过 tick()，返回 0。
     */
    [[nodiscard]] u64 nowMs() const { return m_nowMs; }

private:
    /**
     * @brief 执行指定优先级的到期任务
     *
     * @param priority 优先级
     * @param forceDue true 表示无视延迟立即执行（用于 flush）
     */
    u32 _executePriority(Priority priority, bool forceDue);

    /**
     * @brief 检查任务是否到期
     */
    [[nodiscard]] bool _isDue(const UpdateTask& task, bool forceDue) const;

    /**
     * @brief 去重路径（对每个路径只保留最新任务）
     */
    void _deduplicatePaths();

    /**
     * @brief 清理已取消的任务
     */
    void _cleanupCancelled();

    /**
     * @brief 计算新任务的到期时间
     */
    [[nodiscard]] u64 _computeDueTime() const;

private:
    UpdateCallback m_updateCallback;
    std::vector<std::unique_ptr<UpdateTask>> m_tasks;
    std::unordered_map<std::string, std::vector<u64>> m_pathToTasks;
    u64 m_nextTimestamp = 0; ///< 逻辑序号，用于去重时取最新

    // 配置项
    u32 m_batchDelayMs = 16;      ///< 批量更新延迟（默认16ms）
    u32 m_maxBatchSize = 100;     ///< 最大批量大小
    bool m_deferredUpdate = true; ///< 是否启用延迟更新

    // 调度器内部时钟（毫秒级真实时间戳，由外部 tick 推进）
    u64 m_nowMs = 0;
};

} // namespace mc::client::ui::kagero::tpl::runtime
