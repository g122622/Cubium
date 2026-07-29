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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EITHER OR CONSEQUENTIAL
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "FunctionLoader.hpp"
#include "FunctionManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <filesystem>
#include <sstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {
namespace function {

namespace fs = std::filesystem;

/// 命令行最大字符数限制
static constexpr Size MAX_COMMAND_LINE_LENGTH = 2000000;

FunctionLoader::FunctionLoader(FunctionManager& manager)
    : m_manager(manager)
{}

Result<FunctionLoader::LoadResult> FunctionLoader::loadFromDataPackRepository(
    const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "FunctionLoader::loadFromDataPackRepository");

    LoadResult result{};

    if (m_clearBeforeLoad) {
        m_manager.clear();
    }

    // ========== 第一阶段：加载 .mcfunction 文件 ==========
    auto listResult = dataPacks.listResources("", ".mcfunction");
    if (!listResult.success()) {
        return listResult.error();
    }

    // 过滤出包含 "/functions/" 或 "/function/" 的路径（MC 1.21+ 使用单数形式）
    std::vector<std::string> functionResources;
    for (const auto& path : listResult.value()) {
        if (path.find("/functions/") == std::string::npos && path.find("functions/") == std::string::npos &&
            path.find("/function/") == std::string::npos && path.find("function/") == std::string::npos) {
            continue;
        }
        functionResources.push_back(path);
    }

    Size current = 0;
    const Size total = functionResources.size();

    for (const auto& resourcePath : functionResources) {
        const std::string id = pathToFunctionId(resourcePath);
        if (callback) {
            callback(current, total, id);
        }

        auto readResult = dataPacks.readTextResource(resourcePath);
        if (!readResult.success()) {
            ++result.failedCount;
            result.errors.push_back(resourcePath + ": " + readResult.error().toString());
            ++current;
            continue;
        }

        auto parseResult = parseFunctionContent(id, readResult.value());
        if (parseResult.success()) {
            auto& parsed = parseResult.value();
            ResourceLocation loc = ResourceLocation::parse(id);

            if (parsed.isMacro()) {
                // 含 $ 宏行：注册为 MacroFunction
                auto macroFunc = std::make_unique<MacroFunction>(
                    loc, std::move(parsed.macroEntries), std::move(parsed.macroParameters));
                m_manager.registerMacroFunction(loc, std::move(macroFunc));
                ++result.successCount;
                ++result.macroFunctionCount;
            } else {
                // 普通函数：注册为 CommandFunction
                m_manager.registerFunction(loc, std::move(parsed.commands));
                ++result.successCount;
            }
        } else {
            ++result.failedCount;
            result.errors.push_back(id + ": " + parseResult.error().toString());
        }

        ++current;
    }

    if (callback) {
        callback(total, total, "");
    }

    // ========== 第二阶段：加载函数标签 ==========
    result.tagCount = loadFunctionTags(dataPacks, result);

    return result;
}

Size FunctionLoader::loadFunctionTags(const mc::resource::DataPackRepository& dataPacks, LoadResult& result)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "FunctionLoader::loadFunctionTags");

    // 获取所有命名空间
    auto namespacesResult = dataPacks.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return 0;
    }

    // 多数据包标签合并：使用 listResourceStacks 获取同一资源路径在所有数据包中的内容。
    // MC Java 的标签加载语义：按数据包优先级从低到高遍历同名标签文件，
    // 默认追加，replace=true 时清空已有条目后追加。
    // listResourceStacks 返回的每个路径对应的 ResourceVersion 向量按数据包优先级从高到低排序，
    // 因此需要逆序遍历以匹配 MC Java 的从低到高遍历顺序。

    // 用于存储已解析的标签数据
    // key: 标签 ResourceLocation, value: (replace标志, 条目列表)
    struct TagParseData {
        bool replace = false;
        std::vector<TagEntry> entries;
    };
    std::unordered_map<ResourceLocation, TagParseData> parsedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 <namespace>/tags/functions/ 目录下所有数据包中的 JSON 文件及其内容栈
        std::string directory = namespace_ + "/tags/functions";
        auto stacksResult = dataPacks.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            // 从路径提取标签名称
            std::string tagIdStr = pathToTagId(resourcePath);
            ResourceLocation tagLoc = ResourceLocation::parse(tagIdStr);

            // 遍历同一资源路径在所有数据包中的版本（按优先级从低到高）
            // listResourceStacks 返回的版本按优先级从高到低排序，
            // 但 MC Java 的 TagLoader.load() 语义是先处理低优先级数据包，后处理高优先级数据包，
            // replace=true 时清空已有条目后追加，默认追加。
            // 因此需要逆序遍历，以匹配 MC Java 的行为。
            for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
                auto& version = *it;
                // 解析 JSON
                auto parseResult = parseTagJson(tagLoc, version.content);
                if (!parseResult.success()) {
                    spdlog::warn("FunctionLoader: Failed to resolve tag {} (from data pack {}): {}",
                        tagIdStr,
                        version.packName,
                        parseResult.error().message());
                    ++result.failedCount;
                    result.errors.push_back(tagIdStr + " [" + version.packName + "]: " + parseResult.error().message());
                    continue;
                }

                auto& tagData = parseResult.value();

                // 合并到 parsedTags：replace=true 清空已有条目后追加，默认追加
                auto& existing = parsedTags[tagLoc];
                if (tagData.replace) {
                    // replace=true：清空已有条目，使用当前数据包的内容
                    existing.replace = true;
                    existing.entries = std::move(tagData.entries);
                } else {
                    // 默认追加模式
                    for (auto& entry : tagData.entries) {
                        existing.entries.push_back(std::move(entry));
                    }
                }
            }
        }
    }

    // 构建标签：采用三阶段策略，与 MC Java 的 TagLoader.build() 行为一致：
    // 1. 收集所有条目到合并列表（直接函数 + 标签引用）
    // 2. 验证 required 条目并确定哪些标签应被丢弃（含级联丢弃）
    // 3. 仅从有效（未被丢弃的）标签解析引用，构建最终的函数 ID 列表并注册

    // 用于存储标签的原始条目
    // key: 标签 ResourceLocation
    struct MergedTag {
        std::vector<ResourceLocation> directFunctionIds; // 标签直接声明的函数条目
        std::vector<ResourceLocation> tagReferences;     // 需要展开的标签引用（# 前缀）
    };
    std::unordered_map<ResourceLocation, MergedTag> mergedTags;

    // 建立条目ID到required的映射，用于后续验证
    struct RequiredMaps {
        std::unordered_map<ResourceLocation, bool> functionRequired;
        std::unordered_map<ResourceLocation, bool> tagRequired;
    };
    std::unordered_map<ResourceLocation, RequiredMaps> requiredMaps;

    // 第一步：将条目分类收集（直接函数引用 + 标签引用）
    // 注意：必须先使用 entry.id 进行 map 操作，再移动 entry.id，避免使用 moved-from 对象
    for (auto& [tagLoc, tagData] : parsedTags) {
        auto& merged = mergedTags[tagLoc];
        auto& reqMaps = requiredMaps[tagLoc];
        for (auto& entry : tagData.entries) {
            if (entry.type == TagEntryType::Function) {
                // 先更新 required 映射（在移动 entry.id 之前）
                auto it = reqMaps.functionRequired.find(entry.id);
                if (it == reqMaps.functionRequired.end() || entry.required) {
                    reqMaps.functionRequired[entry.id] = entry.required;
                }
                merged.directFunctionIds.push_back(std::move(entry.id));
            } else {
                // 先更新 required 映射（在移动 entry.id 之前）
                auto it = reqMaps.tagRequired.find(entry.id);
                if (it == reqMaps.tagRequired.end() || entry.required) {
                    reqMaps.tagRequired[entry.id] = entry.required;
                }
                merged.tagReferences.push_back(std::move(entry.id));
            }
        }
    }

    const Size tagCount = mergedTags.size();

    // 第二步：验证 required 条目并确定哪些标签应被丢弃
    // 与 MC Java 的 TagLoader.build() 行为一致：
    // - required=true 的函数条目缺失 → 整个标签被丢弃
    // - required=true 的标签引用缺失 → 整个标签被丢弃
    // - required=false 的条目缺失 → 静默跳过
    // 级联效果：被丢弃的标签不会出现在 lookup map 中，导致引用它的标签也无法解析
    std::unordered_set<ResourceLocation> discardedTags;

    // 2a: 验证 required=true 的函数条目是否在 FunctionManager 中已注册
    // 由于函数在第一阶段已全部加载到 FunctionManager 中，此时可以安全地验证
    for (auto& [tagLoc, merged] : mergedTags) {
        auto& reqMaps = requiredMaps[tagLoc];
        std::vector<std::string> missingEntries;

        for (const auto& [funcId, isRequired] : reqMaps.functionRequired) {
            if (isRequired && !m_manager.hasFunction(funcId)) {
                missingEntries.push_back(funcId.toString());
            }
        }

        if (!missingEntries.empty()) {
            // 拼接缺失条目列表（与 MC Java 错误消息格式一致）
            std::string missingStr;
            for (Size i = 0; i < missingEntries.size(); ++i) {
                if (i > 0) {
                    missingStr += ", ";
                }
                missingStr += missingEntries[i];
            }
            spdlog::error("FunctionLoader: Couldn't load tag {} as it is missing following references: {}",
                tagLoc.toString(),
                missingStr);
            discardedTags.insert(tagLoc);
        }
    }

    // 2b: 验证 required=true 的标签引用 + 级联丢弃
    // 迭代直到没有新的标签被丢弃（处理多级级联）
    bool newDiscards = true;
    while (newDiscards) {
        newDiscards = false;
        for (auto& [tagLoc, merged] : mergedTags) {
            if (discardedTags.count(tagLoc) > 0) {
                continue;
            }

            auto& reqMaps = requiredMaps[tagLoc];
            for (const auto& [refTagLoc, isRequired] : reqMaps.tagRequired) {
                if (!isRequired) {
                    continue;
                }
                // 引用的标签不存在于 mergedTags 中（数据包中没有定义）
                if (mergedTags.find(refTagLoc) == mergedTags.end()) {
                    if (discardedTags.insert(tagLoc).second) {
                        spdlog::error("FunctionLoader: Referenced tag '{}' not found (required), tag '{}' will be dropped",
                            refTagLoc.toString(),
                            tagLoc.toString());
                        newDiscards = true;
                    }
                }
                // 引用的标签存在但已被丢弃
                else if (discardedTags.count(refTagLoc) > 0) {
                    if (discardedTags.insert(tagLoc).second) {
                        spdlog::error("FunctionLoader: Referenced tag '{}' was dropped (required), tag '{}' will also be dropped",
                            refTagLoc.toString(),
                            tagLoc.toString());
                        newDiscards = true;
                    }
                }
                if (discardedTags.count(tagLoc) > 0) {
                    break; // 此标签已标记为丢弃，无需继续检查其他引用
                }
            }
        }
    }

    // 第三步：仅从有效（未被丢弃的）标签构建并注册
    // 这与 MC Java 的 TagLoader.build() 一致：只有成功构建的标签才会出现在 lookup map 中，
    // 被丢弃的标签不会参与引用解析，其函数不会传播到引用它的其他标签。
    //
    // 多层标签引用的展开策略：
    // MC Java 使用 DependencySorter 按拓扑顺序构建标签，确保被引用标签在引用它的标签之前构建，
    // 因此被引用标签的函数列表已包含其自身引用的展开结果。
    // 当前实现采用两阶段注册：先注册只含直接函数的标签，再迭代展开标签引用直到稳定，
    // 这等效于 MC Java 的拓扑排序构建。

    // 3a: 先注册所有有效标签的直接函数（不含标签引用展开）
    Size registeredTagCount = 0;
    for (auto& [tagLoc, merged] : mergedTags) {
        if (discardedTags.count(tagLoc) > 0) {
            continue;
        }

        // 构建只含直接函数的列表
        std::vector<ResourceLocation> directOnlyIds;
        for (const auto& funcId : merged.directFunctionIds) {
            // 过滤掉 required=false 且不存在的函数（required=true 的缺失函数已在第二步导致标签丢弃）
            auto reqIt = requiredMaps[tagLoc].functionRequired.find(funcId);
            if (reqIt != requiredMaps[tagLoc].functionRequired.end() && !reqIt->second) {
                if (!m_manager.hasFunction(funcId)) {
                    continue;
                }
            }
            directOnlyIds.push_back(funcId);
        }

        m_manager.registerTag(tagLoc, std::move(directOnlyIds));
        ++registeredTagCount;
    }

    // 3b: 迭代展开标签引用并更新已注册的标签
    // 由于标签可能多层引用（A → B → C），需要迭代展开直到所有标签引用都被解析。
    // 每轮迭代中，从头构建每个标签的函数列表（直接函数 + 标签引用展开），
    // 通过 FunctionManager.getTag() 获取被引用标签的函数列表（可能已包含前一轮展开的结果），
    // 直到没有任何标签的函数列表发生变化（达到不动点）。
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [tagLoc, merged] : mergedTags) {
            if (discardedTags.count(tagLoc) > 0) {
                continue;
            }

            // 如果此标签没有标签引用，无需展开
            if (merged.tagReferences.empty()) {
                continue;
            }

            // 从头构建函数列表：直接函数 + 标签引用展开
            std::vector<ResourceLocation> expandedIds;

            // 添加直接声明的函数条目
            for (const auto& funcId : merged.directFunctionIds) {
                auto reqIt = requiredMaps[tagLoc].functionRequired.find(funcId);
                if (reqIt != requiredMaps[tagLoc].functionRequired.end() && !reqIt->second) {
                    if (!m_manager.hasFunction(funcId)) {
                        continue;
                    }
                }
                expandedIds.push_back(funcId);
            }

            // 展开标签引用
            std::unordered_set<ResourceLocation> visitedTags;
            visitedTags.insert(tagLoc); // 防止自引用

            for (const auto& refTagLoc : merged.tagReferences) {
                if (visitedTags.count(refTagLoc) > 0) {
                    // 循环引用：required=true 已在第二步处理，required=false 静默跳过
                    continue;
                }
                visitedTags.insert(refTagLoc);

                // 从 FunctionManager 获取被引用标签的函数列表（可能已包含前一轮展开的结果）
                const auto& refFuncIds = m_manager.getTag(refTagLoc);
                if (!refFuncIds.empty()) {
                    for (const auto& funcId : refFuncIds) {
                        expandedIds.push_back(funcId);
                    }
                }
                // 空列表表示标签不存在或已被丢弃：required=true 已在第二步处理，
                // required=false 静默跳过
            }

            // 与当前注册的函数列表比较，仅在发生变化时更新
            const auto& currentIds = m_manager.getTag(tagLoc);
            if (expandedIds != currentIds) {
                m_manager.registerTag(tagLoc, std::move(expandedIds));
                changed = true;
            }
        }
    }

    if (!discardedTags.empty()) {
        spdlog::warn("FunctionLoader: {} tags dropped due to missing required entries ({} tags parsed in total)",
            discardedTags.size(),
            tagCount);
    }

    if (registeredTagCount > 0) {
        spdlog::info("FunctionLoader: Loaded {} function tags from data packs", registeredTagCount);
    }

    return registeredTagCount;
}

Result<FunctionLoader::TagData> FunctionLoader::parseTagJson(
    const ResourceLocation& tagId, const std::string& jsonContent)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(jsonContent);

        TagData tagData;
        tagData.id = tagId;

        // 解析 replace 字段（可选，默认 false）
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            tagData.replace = jsonObj["replace"].get<bool>();
        }

        // 解析 values 数组（必需）
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "Function tag '" + tagId.toString() + "' missing 'values' array");
        }

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "namespace:path" 或 "#namespace:tag"
                // 字符串格式默认 required=true
                std::string entry = value.get<std::string>();
                resolveTagEntry(entry, true, tagData.entries);
            } else if (value.is_object()) {
                // 对象格式: {"id":"namespace:path","required":false}
                // 对应 MC Java 的 TagEntry 对象格式，支持 required 语义
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn("FunctionLoader: Object-format entry in tag '{}' missing 'id' field, skipped", tagId.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn("FunctionLoader: Object-format entry in tag '{}' has empty 'id', skipped", tagId.toString());
                    continue;
                }

                // 解析 required 字段（可选，默认 true）
                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                resolveTagEntry(id, required, tagData.entries);
            } else {
                spdlog::warn("FunctionLoader: Value in tag '{}' is not a string or object, skipped", tagId.toString());
            }
        }

        return tagData;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

void FunctionLoader::resolveTagEntry(const std::string& entry, bool required, std::vector<TagEntry>& entries)
{
    if (entry.empty()) {
        return;
    }

    if (entry[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = entry.substr(1);
        ResourceLocation tagLoc = ResourceLocation::parse(tagRef);
        entries.push_back(TagEntry::tagEntry(std::move(tagLoc), required));
    } else {
        // 直接函数引用: namespace:path
        ResourceLocation funcLoc = ResourceLocation::parse(entry);
        entries.push_back(TagEntry::functionEntry(std::move(funcLoc), required));
    }
}

Result<FunctionLoader::ParseResult> FunctionLoader::parseFunctionContent(
    const std::string& id, const std::string& content)
{
    ParseResult result;
    std::istringstream stream(content);
    std::string line;
    std::vector<std::string> lines;

    // 先将所有行读入列表，处理行连接
    while (std::getline(stream, line)) {
        // 去除行尾的 \r（Windows 换行符）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    // 形参名 -> 索引（用于 MacroFunctionEntry::Macro::parameterIndices）
    // 与 MC 1.21.11 FunctionBuilder#getArgumentIndex 行为一致：首次出现时分配索引
    std::unordered_map<std::string, Size> parameterIndexMap;

    /// 内部辅助：将普通命令加入 result（按当前是否已切换到 macro 模式分别处理）
    auto addPlainCommand = [&](std::string command) {
        if (result.isMacro()) {
            // 已切换到 macro 模式：转为 PlainTextEntry
            result.macroEntries.push_back(MacroFunctionEntry::plainText(std::move(command)));
        } else {
            // 仍在纯命令模式：累积到 commands
            result.commands.push_back(std::move(command));
        }
    };

    /// 内部辅助：将 $ 宏行加入 result，并在需要时切换到 macro 模式
    /// 对应 MC 1.21.11 FunctionBuilder#addMacro
    auto addMacroLine = [&](const std::string& macroBody, Size lineNumber) -> Result<void> {
        // 解析 $(var) 语法
        StringTemplate tmpl;
        try {
            tmpl = StringTemplate::fromString(macroBody);
        }
        catch (const std::exception& e) {
            return Error(ErrorCode::ResourceParseError,
                "Can't parse function line " + std::to_string(lineNumber) + ": '" + macroBody + "' (" + e.what() + ")");
        }

        // 首次遇到 $ 宏行时切换到 macro 模式：把已累积的 commands 转为 PlainTextEntry
        if (!result.isMacro()) {
            result.macroEntries.reserve(result.commands.size() + 1);
            for (auto& cmd : result.commands) {
                result.macroEntries.push_back(MacroFunctionEntry::plainText(std::move(cmd)));
            }
            result.commands.clear();
        }

        // 为模板中的每个变量分配或复用形参索引
        std::vector<Size> indices;
        indices.reserve(tmpl.variables().size());
        for (const auto& varName : tmpl.variables()) {
            auto it = parameterIndexMap.find(varName);
            if (it == parameterIndexMap.end()) {
                Size idx = result.macroParameters.size();
                result.macroParameters.push_back(varName);
                parameterIndexMap[varName] = idx;
                indices.push_back(idx);
            } else {
                indices.push_back(it->second);
            }
        }

        result.macroEntries.push_back(MacroFunctionEntry::macro(std::move(tmpl), std::move(indices)));
        return {};
    };

    // 处理行连接和命令解析
    for (Size i = 0; i < lines.size(); ++i) {
        std::string processedLine = lines[i];

        // 处理行连接（\ 结尾的行与下一行连接）
        while (!processedLine.empty() && processedLine.back() == '\\' && (i + 1) < lines.size()) {
            processedLine.pop_back(); // 移除末尾的反斜杠
            ++i;
            std::string nextLine = lines[i];
            // 去除下一行前导空白
            Size startPos = nextLine.find_first_not_of(" \t");
            if (startPos != std::string::npos) {
                nextLine = nextLine.substr(startPos);
            }
            processedLine += nextLine;
        }

        // 检查命令长度限制
        if (processedLine.length() > MAX_COMMAND_LINE_LENGTH) {
            std::string preview = processedLine.substr(0, std::min(processedLine.length(), static_cast<size_t>(512)));
            return Error(ErrorCode::ResourceParseError,
                "Command too long in function '" + id + "' on line " + std::to_string(i + 1) + ": " +
                    std::to_string(processedLine.length()) + " characters, contents: " + preview + "...");
        }

        // 去除前导空白
        Size startPos = processedLine.find_first_not_of(" \t");
        if (startPos == std::string::npos) {
            continue; // 空行
        }
        processedLine = processedLine.substr(startPos);

        // 注释行：以 # 开头
        if (processedLine.empty() || processedLine[0] == '#') {
            continue;
        }

        // 宏行：以 $ 开头 - 解析为 MacroFunctionEntry::Macro
        if (processedLine[0] == '$') {
            // 去除 $ 前缀，剩余部分作为宏体（对应 MC 1.21.11 FunctionBuilder#addMacro 的 s1.substring(1)）
            std::string macroBody = processedLine.substr(1);
            auto addResult = addMacroLine(macroBody, i + 1);
            if (!addResult.success()) {
                return addResult.error();
            }
            continue;
        }

        // 去除 / 前缀（MC 不允许在函数文件中使用 / 前缀，但我们做容错处理）
        if (processedLine[0] == '/') {
            // 检查是否是 // 注释（非法，需报错）
            if (processedLine.size() > 1 && processedLine[1] == '/') {
                return Error(ErrorCode::ResourceParseError,
                    "Invalid command in function '" + id + "' on line " + std::to_string(i + 1) +
                        ": use '#' for comments, not '//'. If you intended to run a command, remove the leading '/'.");
            }
            // 单 / 前缀：去除
            processedLine = processedLine.substr(1);
        }

        if (!processedLine.empty()) {
            addPlainCommand(std::move(processedLine));
        }
    }

    return result;
}

std::string FunctionLoader::pathToFunctionId(const std::string& filePath) const
{
    fs::path path(filePath);

    std::string namespaceStr = "minecraft";
    std::string relativePath;

    // 将路径转为通用格式（正斜杠）
    std::string genericPath = path.generic_string();

    // 查找 "functions/" 或 "function/" 的位置（MC 1.21+ 使用单数形式）
    Size functionsPos = genericPath.find("/functions/");
    Size functionsTokenSize = std::string("/functions/").size();
    if (functionsPos == std::string::npos) {
        functionsPos = genericPath.find("functions/");
        functionsTokenSize = std::string("functions/").size();
    }
    if (functionsPos == std::string::npos) {
        functionsPos = genericPath.find("/function/");
        functionsTokenSize = std::string("/function/").size();
    }
    if (functionsPos == std::string::npos) {
        functionsPos = genericPath.find("function/");
        functionsTokenSize = std::string("function/").size();
    }

    if (functionsPos != std::string::npos) {
        // 从 functions/ 前面提取 namespace
        auto dataPos = genericPath.rfind("/data/", functionsPos);
        if (dataPos == std::string::npos && genericPath.rfind("data/", 0) == 0) {
            dataPos = 0;
        }
        if (dataPos != std::string::npos) {
            Size namespaceStart = dataPos == 0 ? 5 : dataPos + 6; // 跳过 "data/" 或 "/data/"
            Size namespaceEnd = functionsPos;
            namespaceStr = genericPath.substr(namespaceStart, namespaceEnd - namespaceStart);
        } else if (functionsPos > 0 && genericPath[functionsPos] == '/') {
            namespaceStr = genericPath.substr(0, functionsPos);
        }

        // 从 functions/ 后面提取路径（不含 .mcfunction 扩展名）
        Size pathStart = functionsPos + functionsTokenSize;
        relativePath = genericPath.substr(pathStart);

        // 去掉 .mcfunction 扩展名
        const std::string ext = ".mcfunction";
        if (relativePath.size() > ext.size() && relativePath.substr(relativePath.size() - ext.size()) == ext) {
            relativePath = relativePath.substr(0, relativePath.size() - ext.size());
        }
    } else {
        relativePath = path.stem().string();
    }

    // 组合为 ResourceLocation 格式的字符串
    ResourceLocation location(namespaceStr, relativePath);
    return location.toString();
}

std::string FunctionLoader::pathToTagId(const std::string& filePath) const
{
    fs::path path(filePath);

    std::string namespaceStr = "minecraft";
    std::string relativePath;

    // 将路径转为通用格式（正斜杠）
    std::string genericPath = path.generic_string();

    // 查找 "tags/functions/" 的位置
    static const std::string tagsFunctionsDir = "/tags/functions/";
    static const std::string tagsFunctionsDirNoSlash = "tags/functions/";

    Size tagsPos = genericPath.find(tagsFunctionsDir);
    Size tagsTokenSize = tagsFunctionsDir.size();
    if (tagsPos == std::string::npos) {
        tagsPos = genericPath.find(tagsFunctionsDirNoSlash);
        tagsTokenSize = tagsFunctionsDirNoSlash.size();
    }

    if (tagsPos != std::string::npos) {
        // 从 tags/functions/ 前面提取 namespace
        auto dataPos = genericPath.rfind("/data/", tagsPos);
        if (dataPos == std::string::npos && genericPath.rfind("data/", 0) == 0) {
            dataPos = 0;
        }
        if (dataPos != std::string::npos) {
            Size namespaceStart = dataPos == 0 ? 5 : dataPos + 6; // 跳过 "data/" 或 "/data/"
            Size namespaceEnd = tagsPos;
            namespaceStr = genericPath.substr(namespaceStart, namespaceEnd - namespaceStart);
        } else if (tagsPos > 0) {
            // 路径格式: namespace/tags/functions/xxx.json（不含 /data/ 前缀）
            // 此时 tagsPos 指向 "tags/" 或 "/tags/" 的位置
            // 需要提取 tags/ 之前的路径部分作为命名空间
            Size nsEnd = tagsPos;
            // 如果前面有 '/'，跳过它
            if (nsEnd > 0 && genericPath[nsEnd - 1] == '/') {
                --nsEnd;
            }
            // 找到命名空间的起始位置（上一个 '/' 之后）
            Size nsStart = genericPath.rfind('/', nsEnd - 1);
            if (nsStart == std::string::npos) {
                nsStart = 0;
            } else {
                ++nsStart; // 跳过 '/'
            }
            namespaceStr = genericPath.substr(nsStart, nsEnd - nsStart);
        }

        // 从 tags/functions/ 后面提取路径（不含 .json 扩展名）
        Size pathStart = tagsPos + tagsTokenSize;
        relativePath = genericPath.substr(pathStart);

        // 去掉 .json 扩展名
        const std::string ext = ".json";
        if (relativePath.size() > ext.size() && relativePath.substr(relativePath.size() - ext.size()) == ext) {
            relativePath = relativePath.substr(0, relativePath.size() - ext.size());
        }
    } else {
        relativePath = path.stem().string();
    }

    // 组合为 ResourceLocation 格式的字符串
    ResourceLocation location(namespaceStr, relativePath);
    return location.toString();
}

} // namespace function
} // namespace mc
