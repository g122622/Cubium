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
#include "common/resource/ResourceLocation.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace mc {

namespace command {
class ServerCommandSource;
}

namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

namespace function {

// 前向声明，避免循环 include
class FunctionManager;

/**
 * @brief 函数执行结果
 *
 * 由 IFunction::execute 返回，FunctionManager 也使用此类型。
 * 对应 MC 1.21.11 中 ExecutionContext 累加的成功/失败计数。
 */
struct FunctionExecuteResult {
    i32 successCount = 0; ///< 成功执行的命令数
    i32 failureCount = 0; ///< 失败的命令数
};

/**
 * @brief 函数实例化异常
 *
 * 对应 MC 1.21.11 的 net.minecraft.commands.FunctionInstantiationException。
 *
 * 在宏函数实例化（CompoundTag 参数替换）过程中发生错误时抛出：
 * - 缺少参数（commands.function.error.missing_arguments / missing_argument）
 * - 替换后命令解析失败（commands.function.error.parse）
 */
class FunctionInstantiationException : public std::runtime_error {
public:
    explicit FunctionInstantiationException(std::string message)
        : std::runtime_error(message)
        , m_message(std::move(message))
    {}

    [[nodiscard]] const std::string& message() const noexcept { return m_message; }

private:
    std::string m_message;
};

/**
 * @brief 函数统一接口
 *
 * 抽象基类，CommandFunction（普通函数）和 MacroFunction（宏函数）都继承它。
 * 对应 MC 1.21.11 的 net.minecraft.commands.functions.CommandFunction<T> 接口。
 *
 * 职责：
 * - 提供函数 ID、命令数量、空判断等元信息
 * - 提供执行接口，由子类实现具体逻辑：
 *   - CommandFunction：逐行通过 CommandRegistry 执行，忽略 arguments
 *   - MacroFunction：先用 CompoundTag 实例化 $(var) 占位符，再逐行执行
 *
 * FunctionManager 通过此接口统一管理所有函数对象，外部调用方无需区分子类类型。
 */
class IFunction {
public:
    virtual ~IFunction() = default;

    /** @brief 获取函数 ID */
    [[nodiscard]] virtual const ResourceLocation& id() const noexcept = 0;

    /**
     * @brief 获取命令数量
     *
     * 普通函数返回 commands().size()；宏函数返回 entries 数量（含宏行和纯文本行）。
     */
    [[nodiscard]] virtual Size commandCount() const noexcept = 0;

    /** @brief 检查函数是否为空（无命令） */
    [[nodiscard]] virtual bool isEmpty() const noexcept = 0;

    /**
     * @brief 是否为宏函数
     *
     * 普通函数返回 false，宏函数返回 true。
     * 用于 FunctionManager 区分实例化路径和错误消息。
     */
    [[nodiscard]] virtual bool isMacro() const noexcept = 0;

    /**
     * @brief 执行函数
     *
     * 普通函数：逐行通过 CommandRegistry 执行，忽略 arguments。
     * 宏函数：先用 arguments 实例化 $(var) 占位符，再逐行执行。
     *        arguments 为 nullptr 时，宏函数抛 FunctionInstantiationException。
     *
     * @param manager 函数管理器（用于获取 CommandRegistry 等依赖）
     * @param source 命令源（权限等级、反馈等）
     * @param arguments 实参 CompoundTag（可为 nullptr，仅宏函数使用）
     * @return 执行结果（成功/失败命令数）
     */
    [[nodiscard]] virtual FunctionExecuteResult execute(FunctionManager& manager,
        command::ServerCommandSource& source,
        const nbt::tags::compound_tag* arguments) const = 0;
};

} // namespace function
} // namespace mc
