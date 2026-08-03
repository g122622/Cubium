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

#include "CommandFunction.hpp"
#include "FunctionManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/function/IFunction.hpp"
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace function {

CommandFunction::CommandFunction(ResourceLocation id, std::vector<std::string> commands)
    : m_id(std::move(id))
    , m_commands(std::move(commands))
{}

FunctionExecuteResult CommandFunction::execute(FunctionManager& /*manager*/,
    command::ServerCommandSource& source,
    const nbt::tags::compound_tag* /*arguments*/) const
{
    FunctionExecuteResult result{0, 0};

    if (m_commands.empty()) {
        return result;
    }

    spdlog::info("CommandFunction: Executing '{}' with {} commands", m_id.toString(), m_commands.size());

    auto& registry = command::CommandRegistry::getGlobal();

    for (const auto& command : m_commands) {
        if (command.empty()) {
            continue;
        }

        // 确保命令以 / 开头（CommandRegistry::execute 期望带 / 前缀的命令）
        std::string fullCommand = command;
        if (fullCommand[0] != '/') {
            fullCommand = "/" + fullCommand;
        }

        auto execResult = registry.execute(fullCommand, source);
        if (execResult.success()) {
            ++result.successCount;
        } else {
            ++result.failureCount;
            spdlog::info("CommandFunction: Command '{}' in function '{}' failed: {}",
                command,
                m_id.toString(),
                execResult.error().toString());
        }
    }

    return result;
}

} // namespace function
} // namespace mc
