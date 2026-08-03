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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/core/Capabilities.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/core/Privilege.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"
#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptEngine;
class IScriptContext;

/**
 * @brief 上下文配置
 *
 * 创建脚本上下文时传入的配置参数。
 * 所有字段必须由调用方显式赋值，不使用默认值。
 */
struct ContextConfig {
    Capabilities capabilities;
    std::optional<Privilege> privilege;
    u32 maxMemoryBytes;    ///< 内存限制（字节），典型值 64MB
    u32 maxStackSizeBytes; ///< 栈大小限制（字节），典型值 4MB
};

/// 上下文内存限制默认值：64MB
inline constexpr u32 DEFAULT_CONTEXT_MEMORY_BYTES = 64 * 1024 * 1024;
/// 上下文栈大小默认值：4MB
inline constexpr u32 DEFAULT_CONTEXT_STACK_SIZE_BYTES = 4 * 1024 * 1024;

/**
 * @brief 运行时统计信息
 */
struct RuntimeStats {
    u64 heapSize = 0;         // 堆大小（字节）
    u64 heapLimit = 0;        // 堆限制（字节）
    u32 contextCount = 0;     // 活跃上下文数量
    u32 pendingJobsCount = 0; // 待处理任务数量
};

/**
 * @brief 脚本运行时抽象接口
 *
 * 提供JS运行时的底层操作。当前实现为QuickJS，
 * 未来可替换为V8等其他引擎。
 *
 * @note 一个IScriptRuntime实例对应一个JS运行时，
 *       可以创建多个IScriptContext（每个插件一个）。
 */
class IScriptRuntime {
public:
    virtual ~IScriptRuntime() = default;

    /**
     * @brief 创建新的脚本上下文
     *
     * @param config 上下文配置
     * @return 新创建的上下文，失败时返回nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<IScriptContext> createContext(const ContextConfig& config) = 0;

    /**
     * @brief 销毁脚本上下文
     *
     * @param context 要销毁的上下文
     */
    virtual void destroyContext(IScriptContext* context) = 0;

    /**
     * @brief 执行待处理的异步任务
     *
     * 在每个tick中调用，处理Promise回调、定时器等异步操作。
     */
    virtual void executePendingJobs() = 0;

    /**
     * @brief 检查是否有待处理的异步任务
     */
    [[nodiscard]] virtual bool hasPendingJobs() const = 0;

    /**
     * @brief 获取运行时统计信息
     */
    [[nodiscard]] virtual RuntimeStats computeStats() const = 0;

    /**
     * @brief 设置内存限制
     *
     * @param limitBytes 内存限制（字节），0表示无限制
     */
    virtual void setMemoryLimit(u64 limitBytes) = 0;
};

} // namespace mc::mod::bedrock::addon
