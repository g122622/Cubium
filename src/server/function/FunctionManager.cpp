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

#include "FunctionManager.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace function {

const ResourceLocation FunctionManager::TICK_TAG = ResourceLocation::parse("minecraft:tick");
const ResourceLocation FunctionManager::LOAD_TAG = ResourceLocation::parse("minecraft:load");
const std::vector<ResourceLocation> FunctionManager::EMPTY_TAG;

void FunctionManager::registerFunction(const ResourceLocation& id, std::vector<std::string> commands)
{
    auto func = std::make_unique<CommandFunction>(id, std::move(commands));
    m_functions[id] = std::move(func);
}

void FunctionManager::registerTag(const ResourceLocation& tagId, std::vector<ResourceLocation> functionIds)
{
    m_tags[tagId] = std::move(functionIds);
}

void FunctionManager::clear()
{
    m_functions.clear();
    m_tags.clear();
}

const CommandFunction* FunctionManager::getFunction(const ResourceLocation& id) const
{
    auto it = m_functions.find(id);
    if (it != m_functions.end()) {
        return it->second.get();
    }
    return nullptr;
}

const std::vector<ResourceLocation>& FunctionManager::getTag(const ResourceLocation& tagId) const
{
    auto it = m_tags.find(tagId);
    if (it != m_tags.end()) {
        return it->second;
    }
    return EMPTY_TAG;
}

bool FunctionManager::hasFunction(const ResourceLocation& id) const
{
    return m_functions.find(id) != m_functions.end();
}

std::vector<ResourceLocation> FunctionManager::getAllFunctionIds() const
{
    std::vector<ResourceLocation> ids;
    ids.reserve(m_functions.size());
    for (const auto& [id, _] : m_functions) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<ResourceLocation> FunctionManager::getAllTagIds() const
{
    std::vector<ResourceLocation> ids;
    ids.reserve(m_tags.size());
    for (const auto& [id, _] : m_tags) {
        ids.push_back(id);
    }
    return ids;
}

FunctionManager::ExecuteResult FunctionManager::execute(
    const ResourceLocation& id, command::ServerCommandSource& source)
{
    auto* func = getFunction(id);
    if (func == nullptr) {
        spdlog::warn("FunctionManager: Unknown function '{}'", id.toString());
        return ExecuteResult{0, 0};
    }
    return execute(*func, source);
}

FunctionManager::ExecuteResult FunctionManager::execute(
    const CommandFunction& function, command::ServerCommandSource& source)
{
    ExecuteResult result{0, 0};

    if (function.isEmpty()) {
        return result;
    }

    spdlog::info(
        "FunctionManager: Executing function '{}' with {} commands", function.id().toString(), function.commandCount());

    auto& registry = command::CommandRegistry::getGlobal();

    for (const auto& command : function.commands()) {
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
            spdlog::info("FunctionManager: Command '{}' in function '{}' failed: {}",
                command,
                function.id().toString(),
                execResult.error().toString());
        }
    }

    return result;
}

void FunctionManager::tick(command::ServerCommandSource& source)
{
    // 首次重载后执行 minecraft:load 标签
    if (m_postReload) {
        m_postReload = false;
        executeTagFunctions(LOAD_TAG, source);
    }

    // 每 tick 执行 minecraft:tick 标签
    executeTagFunctions(TICK_TAG, source);
}

void FunctionManager::notifyReload()
{
    m_postReload = true;
}

void FunctionManager::executeTagFunctions(const ResourceLocation& tagId, command::ServerCommandSource& source)
{
    const auto& functionIds = getTag(tagId);
    for (const auto& funcId : functionIds) {
        auto* func = getFunction(funcId);
        if (func != nullptr) {
            execute(*func, source);
        } else {
            spdlog::warn(
                "FunctionManager: Function '{}' referenced in tag '{}' not found", funcId.toString(), tagId.toString());
        }
    }
}

} // namespace function
} // namespace mc
