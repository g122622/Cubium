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

#include "IFunction.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace command {
class CommandRegistry;
}

namespace function {

/**
 * @brief 命令函数 - 表示从 .mcfunction 文件解析出的纯命令序列
 *
 * 对应 MC 1.21.11 的 net.minecraft.commands.functions.PlainTextFunction。
 *
 * 每个 CommandFunction 包含一个函数 ID 和一组按行解析的命令字符串。
 * 命令在执行时通过 CommandRegistry 逐行解析和执行。
 *
 * 与 MacroFunction 不同，CommandFunction 不含 $(var) 占位符，无需参数实例化。
 *
 * @see IFunction
 */
class CommandFunction : public IFunction {
public:
    /**
     * @brief 构造命令函数
     * @param id 函数的资源位置 ID（如 minecraft:foo/bar）
     * @param commands 按行解析的命令字符串列表（不含 / 前缀和注释行）
     */
    CommandFunction(ResourceLocation id, std::vector<std::string> commands);

    ~CommandFunction() override = default;

    CommandFunction(const CommandFunction&) = default;
    CommandFunction& operator=(const CommandFunction&) = default;
    CommandFunction(CommandFunction&&) = default;
    CommandFunction& operator=(CommandFunction&&) = default;

    // ========== IFunction 接口实现 ==========

    [[nodiscard]] const ResourceLocation& id() const noexcept override { return m_id; }

    [[nodiscard]] Size commandCount() const noexcept override { return m_commands.size(); }

    [[nodiscard]] bool isEmpty() const noexcept override { return m_commands.empty(); }

    [[nodiscard]] bool isMacro() const noexcept override { return false; }

    /**
     * @brief 执行函数
     *
     * 逐行通过 CommandRegistry 执行，忽略 arguments。
     */
    [[nodiscard]] FunctionExecuteResult execute(FunctionManager& manager,
        command::ServerCommandSource& source,
        const nbt::tags::compound_tag* arguments) const override;

    // ========== 兼容旧接口 ==========

    /**
     * @brief 获取命令列表
     *
     * 仅对 CommandFunction 有效；MacroFunction 不暴露此接口。
     * 保留是为了 FunctionManager::execute(CommandFunction&) 等历史接口。
     */
    [[nodiscard]] const std::vector<std::string>& commands() const noexcept { return m_commands; }

private:
    ResourceLocation m_id;
    std::vector<std::string> m_commands;
};

} // namespace function
} // namespace mc
