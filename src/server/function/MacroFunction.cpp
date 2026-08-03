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

#include "MacroFunction.hpp"
#include "FunctionManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/function/IFunction.hpp"
#include <cstdio>
#include <exception>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace function {

// ========== MacroFunctionEntry ==========

std::string MacroFunctionEntry::instantiate(const std::vector<std::string>& values) const
{
    if (std::holds_alternative<Macro>(data)) {
        const auto& macro = std::get<Macro>(data);
        // 按 parameterIndices 从 values 中取出实参值，构造与 template_.variables() 一一对应的局部 values
        std::vector<std::string> localValues;
        localValues.reserve(macro.parameterIndices.size());
        for (Size idx : macro.parameterIndices) {
            localValues.push_back(values.at(idx));
        }
        return macro.template_.substitute(localValues);
    }
    return std::get<PlainText>(data).command;
}

// ========== MacroFunction ==========

MacroFunction::MacroFunction(ResourceLocation id, std::vector<Entry> entries, std::vector<std::string> parameters)
    : m_id(std::move(id))
    , m_entries(std::move(entries))
    , m_parameters(std::move(parameters))
{}

std::string MacroFunction::stringify(const nbt::tags::tag& tag)
{
    using namespace mc::nbt;
    switch (tag.id()) {
        case TagId::Float: {
            const auto& f = static_cast<const tags::float_tag&>(tag);
            // 使用 %.15g 格式化（与 MC DecimalFormat("#", maxFractionDigits=15) 一致）
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.15g", static_cast<double>(f.value));
            return std::string(buf);
        }
        case TagId::Double: {
            const auto& d = static_cast<const tags::double_tag&>(tag);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.15g", d.value);
            return std::string(buf);
        }
        case TagId::Byte: {
            const auto& b = static_cast<const tags::byte_tag&>(tag);
            // MC: String.valueOf((int)b0)
            return std::to_string(static_cast<int>(b.value));
        }
        case TagId::Short: {
            const auto& s = static_cast<const tags::short_tag&>(tag);
            // MC: String.valueOf((int)short1)
            return std::to_string(static_cast<int>(s.value));
        }
        case TagId::Long: {
            const auto& l = static_cast<const tags::long_tag&>(tag);
            // MC: String.valueOf(i)（无 L 后缀）
            return std::to_string(l.value);
        }
        case TagId::String: {
            const auto& s = static_cast<const tags::string_tag&>(tag);
            // MC: 原始字符串值（不加引号）
            return s.value;
        }
        default:
            // 其他类型（Int/Compound/List/ByteArray/IntArray/LongArray/End）
            // MC: tag.toString() 即 SNBT 文本
            return std::to_string(tag);
    }
}

std::vector<std::string> MacroFunction::instantiate(const nbt::tags::compound_tag* arguments) const
{
    if (arguments == nullptr) {
        throw FunctionInstantiationException("Missing arguments for function '" + m_id.toString() + "'");
    }

    // 按形参顺序从 arguments 中取出 NBT tag 并 stringify
    std::vector<std::string> values;
    values.reserve(m_parameters.size());
    for (const auto& paramName : m_parameters) {
        auto it = arguments->value.find(paramName);
        if (it == arguments->value.end()) {
            throw FunctionInstantiationException(
                "Missing argument '" + paramName + "' for function '" + m_id.toString() + "'");
        }
        values.push_back(stringify(*it->second));
    }

    // 查 LRU 缓存
    if (const auto* cached = _cacheLookup(values); cached != nullptr) {
        return *cached;
    }

    // 未命中：实例化每个 entry
    std::vector<std::string> commands;
    commands.reserve(m_entries.size());
    for (const auto& entry : m_entries) {
        try {
            commands.push_back(entry.instantiate(values));
        }
        catch (const std::exception& e) {
            throw FunctionInstantiationException(
                "Failed to instantiate function '" + m_id.toString() + "': " + e.what());
        }
    }

    _cacheInsert(std::move(values), commands);
    return commands;
}

FunctionExecuteResult MacroFunction::execute(
    FunctionManager& manager, command::ServerCommandSource& source, const nbt::tags::compound_tag* arguments) const
{
    FunctionExecuteResult result{0, 0};

    if (isEmpty()) {
        return result;
    }

    // 实例化（可能抛 FunctionInstantiationException）
    std::vector<std::string> commands;
    try {
        commands = instantiate(arguments);
    }
    catch (const FunctionInstantiationException& e) {
        spdlog::warn("MacroFunction: Failed to instantiate '{}': {}", m_id.toString(), e.message());
        result.failureCount = static_cast<i32>(m_entries.size());
        return result;
    }

    spdlog::info("MacroFunction: Executing '{}' with {} entries", m_id.toString(), m_entries.size());

    auto& registry = command::CommandRegistry::getGlobal();

    for (const auto& command : commands) {
        if (command.empty()) {
            continue;
        }

        // 确保命令以 / 开头（CommandRegistry::execute 期望带 / 前缀）
        std::string fullCommand = command;
        if (fullCommand[0] != '/') {
            fullCommand = "/" + fullCommand;
        }

        auto execResult = registry.execute(fullCommand, source);
        if (execResult.success()) {
            ++result.successCount;
        } else {
            ++result.failureCount;
            spdlog::info("MacroFunction: Command '{}' in function '{}' failed: {}",
                command,
                m_id.toString(),
                execResult.error().toString());
        }
    }

    return result;
}

const std::vector<std::string>* MacroFunction::_cacheLookup(const std::vector<std::string>& key) const
{
    const std::string keyStr = _cacheKeyToString(key);
    auto it = m_cacheMap.find(keyStr);
    if (it == m_cacheMap.end()) {
        return nullptr;
    }
    // 命中：移到链表尾部（最新）
    m_cacheList.splice(m_cacheList.end(), m_cacheList, it->second);
    // 注意：splice 不失效迭代器，it->second 仍指向同一节点，但更新 map 中的位置无关紧要
    // （map 存的就是迭代器，splice 后迭代器仍有效）
    return &it->second->commands;
}

void MacroFunction::_cacheInsert(std::vector<std::string> key, std::vector<std::string> commands) const
{
    if (m_cacheList.size() >= MAX_CACHE_ENTRIES) {
        // 淘汰最旧（链表头）
        const auto& oldest = m_cacheList.front();
        m_cacheMap.erase(_cacheKeyToString(oldest.key));
        m_cacheList.pop_front();
    }
    m_cacheList.push_back(CacheEntry{key, std::move(commands)});
    auto it = std::prev(m_cacheList.end());
    m_cacheMap[_cacheKeyToString(key)] = it;
}

std::string MacroFunction::_cacheKeyToString(const std::vector<std::string>& key)
{
    // 用 \1 作为分隔符（变量值中不太可能出现的字符）
    std::string result;
    for (const auto& s : key) {
        result += s;
        result += '\1';
    }
    return result;
}

} // namespace function
} // namespace mc
