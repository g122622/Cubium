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

#include "../../Types.hpp"
#include <functional>
#include <memory>
#include <set>
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
        High = 0,   ///< 高优先级（立即更新）
        Normal = 1, ///< 普通优先级（下一帧更新）
        Low = 2     ///< 低优先级（批量更新）
    };

    /**
     * @brief 更新任务
     */
    struct UpdateTask {
        std::string path;       ///< 状态路径
        Priority priority;      ///< 优先级
        u64 timestamp;          ///< 创建时间戳
        bool cancelled = false; ///< 是否取消

        UpdateTask(std::string p, Priority pri, u64 ts)
            : path(std::move(p))
            , priority(pri)
            , timestamp(ts)
        {}
    };

    /**
     * @brief 构造函数
     */
    UpdateScheduler();

    /**
     * @brief 析构函数
     */
    ~UpdateScheduler();

    // 禁止拷贝
    UpdateScheduler(const UpdateScheduler&) = delete;
    UpdateScheduler& operator=(const UpdateScheduler&) = delete;

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
    u64 schedule(const std::string& path, Priority priority = Priority::Normal);

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
     * @return 执行的任务数量
     */
    u32 executePending();

    /**
     * @brief 执行高优先级任务
     */
    u32 executeHighPriority();

    /**
     * @brief 执行普通优先级任务
     */
    u32 executeNormalPriority();

    /**
     * @brief 执行低优先级任务
     */
    u32 executeLowPriority();

    /**
     * @brief 执行批量更新（合并相同路径的更新）
     */
    u32 executeBatch();

    // ========== 配置 ==========

    /**
     * @brief 设置批量更新延迟（毫秒）
     */
    void setBatchDelay(u32 delayMs) { m_batchDelayMs = delayMs; }

    /**
     * @brief 设置最大批量大小
     */
    void setMaxBatchSize(u32 size) { m_maxBatchSize = size; }

    /**
     * @brief 设置是否启用延迟更新
     */
    void setDeferredUpdate(bool enabled) { m_deferredUpdate = enabled; }

    // ========== 状态查询 ==========

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] u32 pendingCount() const;

    /**
     * @brief 检查是否有待处理任务
     */
    [[nodiscard]] bool hasPending() const { return pendingCount() > 0; }

    /**
     * @brief 获取指定优先级的待处理任务数量
     */
    [[nodiscard]] u32 pendingCount(Priority priority) const;

    /**
     * @brief 获取当前时间戳
     */
    [[nodiscard]] u64 currentTimestamp() const;

private:
    /**
     * @brief 执行指定优先级的任务
     */
    u32 executePriority(Priority priority);

    /**
     * @brief 去重路径
     */
    void deduplicatePaths();

private:
    UpdateCallback m_updateCallback;
    std::vector<std::unique_ptr<UpdateTask>> m_tasks;
    std::unordered_map<std::string, std::vector<u64>> m_pathToTasks;
    u64 m_nextTaskId = 1;
    u64 m_nextTimestamp = 0;

    // 配置
    u32 m_batchDelayMs = 16;      ///< 批量更新延迟（默认16ms）
    u32 m_maxBatchSize = 100;     ///< 最大批量大小
    bool m_deferredUpdate = true; ///< 是否启用延迟更新
};

} // namespace mc::client::ui::kagero::tpl::runtime
