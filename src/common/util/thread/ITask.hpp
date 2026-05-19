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
 * 所有提交到 ServerWorkerPool 的任务必须继承此类。
 *
 * 使用方法：
 * @code
 * class MyTask : public ITask {
 * public:
 *     bool execute(const std::atomic<bool>& cancelSignal) override {
 *         // 检查取消信号
 *         if (cancelSignal.load()) {
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
     * @param cancelSignal 取消信号，执行器应定期检查
     * @return true 表示成功，false 表示失败或取消
     */
    virtual bool execute(const std::atomic<bool>& cancelSignal) = 0;

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

} // namespace mc::util
