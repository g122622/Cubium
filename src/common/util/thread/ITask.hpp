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

#include "common/core/Types.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace mc::util {

/**
 * @brief 任务优先级
 *
 * 数值越小优先级越高。
 */
enum class TaskPriority : i8 {
    Critical = -3, ///< 紧急任务（退出游戏、崩溃恢复）
    High = -2,     ///< 高优先级（玩家附近区块、关键IO）
    Normal = 0,    ///< 普通任务
    Low = 2,       ///< 低优先级（后台任务）
    Background = 3 ///< 最低优先级（导入、压缩）
};

/**
 * @brief 任务类型枚举
 *
 * 用于追踪和统计。
 */
enum class TaskType : u8 {
    ChunkGenerate,   ///< 区块生成
    ChunkSave,       ///< 区块保存
    ChunkLoad,       ///< 区块加载
    WorldImport,     ///< 世界导入
    SnapshotCreate,  ///< 快照创建
    SnapshotRestore, ///< 快照恢复
    DBWrite,         ///< 数据库写入
    DBRead,          ///< 数据库读取
    Custom           ///< 自定义任务
};

/**
 * @brief 任务基类
 *
 * 所有提交到 UniversalWorkerPool 的任务必须继承此类。
 *
 * 使用方法：
 * @code
 * class MyTask : public ITask {
 * public:
 *     bool execute(const std::atomic<bool>& abortSignal) override {
 *         // 检查取消信号
 *         if (abortSignal.load()) {
 *             return false;
 *         }
 *         // 执行任务...
 *         return true;
 *     }
 *
 *     TaskType type() const override { return TaskType::Custom; }
 *     std::string description() const override { return "MyTask"; }
 * };
 * @endcode
 */
class ITask {
public:
    virtual ~ITask() = default;

    /**
     * @brief 执行任务
     *
     * @param abortSignal 取消信号，执行器应定期检查
     * @return true 表示成功，false 表示失败或取消
     */
    virtual bool execute(const std::atomic<bool>& abortSignal) = 0;

    /**
     * @brief 任务被取消时的回调
     *
     * 当任务在执行前被取消时调用，用于清理资源。
     */
    virtual void onCancel() {}

    /**
     * @brief 获取任务类型
     *
     * 用于追踪和统计。
     */
    virtual TaskType type() const = 0;

    /**
     * @brief 获取任务描述
     *
     * 用于日志和追踪。
     */
    virtual std::string description() const = 0;

    /**
     * @brief 获取追踪类别
     *
     * 用于 Perfetto 追踪。
     */
    virtual const char* traceCategory() const { return "worker_pool"; }
};

/**
 * @brief 任务完成回调
 *
 * @param success true 表示任务成功完成，false 表示失败或取消
 * @param task 任务指针（用于获取结果）
 */
using TaskCallback = std::function<void(bool success, ITask* task)>;

/**
 * @brief 通用 lambda 任务包装器
 *
 * 把任意 `bool(const std::atomic<bool>&)` 可调用对象包装为 ITask，
 * 用于向 UniversalWorkerPool 提交一次性计算任务（如反序列化、组装）。
 * 与 StorageTask 不同，FunctionTask 不绑定存储键、不限定 traceCategory，
 * 适用于任意线程池（ServerCompute 等）。
 *
 * execute 返回 executor 的返回值；onCancel 为空（协作取消由 abortSignal 完成）。
 */
class FunctionTask : public ITask {
public:
    using Executor = std::function<bool(const std::atomic<bool>&)>;

    /**
     * @param type 任务类型（用于追踪）
     * @param description 任务描述（用于日志/追踪）
     * @param executor 任务体，接收 abortSignal，返回 true=成功 false=失败/取消
     * @param traceCategory Perfetto 追踪类别（默认 "worker_pool"）
     */
    FunctionTask(TaskType type, std::string description, Executor executor, const char* traceCategory = "worker_pool")
        : m_type(type)
        , m_description(std::move(description))
        , m_traceCategory(traceCategory)
        , m_executor(std::move(executor))
    {}

    bool execute(const std::atomic<bool>& abortSignal) override
    {
        if (!m_executor) {
            return false;
        }
        return m_executor(abortSignal);
    }

    TaskType type() const override { return m_type; }
    std::string description() const override { return m_description; }
    const char* traceCategory() const override { return m_traceCategory; }

private:
    TaskType m_type;
    std::string m_description;
    const char* m_traceCategory;
    Executor m_executor;
};

} // namespace mc::util
