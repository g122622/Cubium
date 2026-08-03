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
#include "MacroFunction.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/function/CommandFunction.hpp"
#include "server/function/IFunction.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
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

void FunctionManager::registerMacroFunction(const ResourceLocation& id, std::unique_ptr<MacroFunction> function)
{
    m_functions[id] = std::move(function);
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

const IFunction* FunctionManager::getFunctionAny(const ResourceLocation& id) const
{
    auto it = m_functions.find(id);
    if (it != m_functions.end()) {
        return it->second.get();
    }
    return nullptr;
}

const CommandFunction* FunctionManager::getFunction(const ResourceLocation& id) const
{
    auto it = m_functions.find(id);
    if (it == m_functions.end()) {
        return nullptr;
    }
    // 仅当不是宏函数时返回 CommandFunction 指针
    if (it->second->isMacro()) {
        return nullptr;
    }
    return static_cast<const CommandFunction*>(it->second.get());
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

bool FunctionManager::hasTag(const ResourceLocation& tagId) const
{
    return m_tags.find(tagId) != m_tags.end();
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
    return execute(id, source, nullptr);
}

FunctionManager::ExecuteResult FunctionManager::execute(
    const ResourceLocation& id, command::ServerCommandSource& source, const nbt::tags::compound_tag* arguments)
{
    auto* func = getFunctionAny(id);
    if (func == nullptr) {
        spdlog::warn("FunctionManager: Unknown function '{}'", id.toString());
        return ExecuteResult{0, 0};
    }
    return func->execute(*this, source, arguments);
}

FunctionManager::ExecuteResult FunctionManager::execute(
    const CommandFunction& function, command::ServerCommandSource& source)
{
    // 历史接口：复用 IFunction::execute，arguments 忽略
    return function.execute(*this, source, nullptr);
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
        auto* func = getFunctionAny(funcId);
        if (func != nullptr) {
            // tick / load 标签中的函数无 arguments
            (void)func->execute(*this, source, nullptr);
        } else {
            spdlog::warn(
                "FunctionManager: Function '{}' referenced in tag '{}' not found", funcId.toString(), tagId.toString());
        }
    }
}

} // namespace function
} // namespace mc
