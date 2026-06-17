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
#include "common/perfetto/TraceEvents.hpp"
#include <filesystem>
#include <sstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
    MC_TRACE_EVENT("io.resource", "FunctionLoader::loadFromDataPackRepository");

    LoadResult result{};

    if (m_clearBeforeLoad) {
        m_manager.clear();
    }

    // ========== 第一阶段：加载 .mcfunction 文件 ==========
    auto listResult = dataPacks.listResources("", ".mcfunction");
    if (!listResult.success()) {
        return listResult.error();
    }

    // 过滤出包含 "/functions/" 的路径
    std::vector<std::string> functionResources;
    for (const auto& path : listResult.value()) {
        if (path.find("/functions/") == std::string::npos && path.find("functions/") == std::string::npos) {
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
            if (!parsed.commands.empty()) {
                ResourceLocation loc = ResourceLocation::parse(id);
                m_manager.registerFunction(loc, std::move(parsed.commands));
                ++result.successCount;
            } else {
                // 空函数仍然注册
                ResourceLocation loc = ResourceLocation::parse(id);
                m_manager.registerFunction(loc, std::vector<std::string>{});
                ++result.successCount;
            }
            result.skippedCount += parsed.skippedMacroCount;
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
    MC_TRACE_EVENT("io.resource", "FunctionLoader::loadFunctionTags");

    // 获取所有命名空间
    auto namespacesResult = dataPacks.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return 0;
    }

    // 收集所有标签文件及其来源数据包
    // key: 标签 ResourceLocation, value: 按数据包优先级从高到低排列的 (JSON内容, 数据包来源) 列表
    // 由于 DataPackRepository::readTextResource 返回最高优先级数据包的内容，
    // 而 listResources 合并所有数据包的文件列表（去重），
    // 我们需要另一种方式来处理多数据包标签合并。
    //
    // MC Java 的标签合并语义：
    // - 多个数据包对同一标签的值默认追加
    // - replace=true 时，先清空之前（高优先级）数据包添加的条目，再添加当前数据包的条目
    // - 数据包按优先级从高到低遍历
    //
    // 由于 DataPackRepository 的 listResources 只返回去重后的路径列表，
    // 而 readTextResource 只返回最高优先级数据包的内容，
    // 我们无法获取低优先级数据包中同一标签文件的内容。
    // 因此，当前实现只处理最高优先级数据包中的标签文件，
    // 这是项目中其他标签加载器（如 BiomeTagLoader）的同样做法。
    // 如果未来需要完整的多数据包标签合并，需要在 DataPackRepository 层面
    // 提供读取所有数据包中同一资源的方法。

    // 用于存储已解析的标签数据，合并同 ID 标签
    // key: 标签 ResourceLocation, value: 合并后的函数 ID 列表
    std::unordered_map<ResourceLocation, std::vector<ResourceLocation>> mergedTags;

    // 用于存储标签引用关系，后续统一解析
    // key: 标签 ResourceLocation, value: (replace标志, 直接引用的函数ID, 引用的标签ID)
    struct TagParseData {
        bool replace = false;
        std::vector<ResourceLocation> functionIds;
        std::vector<ResourceLocation> tagReferences;
    };
    std::unordered_map<ResourceLocation, TagParseData> parsedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 <namespace>/tags/functions/ 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/tags/functions";
        auto listResult = dataPacks.listResources(directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取标签名称
            // 路径格式: namespace/tags/functions/xxx.json 或 namespace/tags/functions/sub/xxx.json
            std::string tagIdStr = pathToTagId(resourcePath);
            ResourceLocation tagLoc = ResourceLocation::parse(tagIdStr);

            // 读取 JSON 内容
            auto readResult = dataPacks.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("FunctionLoader: 无法读取标签文件: {}", resourcePath);
                ++result.failedCount;
                result.errors.push_back(resourcePath + ": " + readResult.error().toString());
                continue;
            }

            // 解析 JSON
            auto parseResult = parseTagJson(tagLoc, readResult.value());
            if (!parseResult.success()) {
                spdlog::warn("FunctionLoader: 无法解析标签 {}: {}", tagIdStr, parseResult.error().message());
                ++result.failedCount;
                result.errors.push_back(tagIdStr + ": " + parseResult.error().message());
                continue;
            }

            auto& tagData = parseResult.value();

            // 存储解析结果
            auto& existing = parsedTags[tagLoc];
            if (tagData.replace) {
                // replace=true：清空已有条目，使用当前数据包的内容
                existing.replace = true;
                existing.functionIds = std::move(tagData.functionIds);
                existing.tagReferences = std::move(tagData.tagReferences);
            } else {
                // 默认追加模式
                existing.replace = false; // 只在第一个数据包设置 replace 时才为 true
                for (auto& id : tagData.functionIds) {
                    existing.functionIds.push_back(std::move(id));
                }
                for (auto& ref : tagData.tagReferences) {
                    existing.tagReferences.push_back(std::move(ref));
                }
            }
        }
    }

    // 解析标签引用（# 前缀），将引用的标签内容展开到函数列表
    // 使用迭代方式解析，支持前向引用（被引用的标签可能尚未注册）
    // 最多迭代若干次以处理间接引用
    for (auto& [tagLoc, tagData] : parsedTags) {
        // 先将直接引用的函数 ID 加入合并列表
        auto& mergedIds = mergedTags[tagLoc];
        for (auto& funcId : tagData.functionIds) {
            mergedIds.push_back(std::move(funcId));
        }
    }

    // 解析标签引用（# 前缀引用）
    // 使用集合防止循环引用和重复
    Size tagCount = 0;
    for (auto& [tagLoc, tagData] : parsedTags) {
        auto& mergedIds = mergedTags[tagLoc];
        std::unordered_set<ResourceLocation> visitedTags;
        visitedTags.insert(tagLoc); // 防止自引用

        for (const auto& refTagLoc : tagData.tagReferences) {
            // 防止循环引用
            if (visitedTags.count(refTagLoc) > 0) {
                spdlog::warn(
                    "FunctionLoader: 循环标签引用 '{}' in tag '{}', 跳过", refTagLoc.toString(), tagLoc.toString());
                continue;
            }
            visitedTags.insert(refTagLoc);

            // 在已解析的标签中查找引用
            auto refIt = mergedTags.find(refTagLoc);
            if (refIt != mergedTags.end()) {
                for (const auto& funcId : refIt->second) {
                    mergedIds.push_back(funcId);
                }
            } else {
                spdlog::warn("FunctionLoader: 引用的标签 '{}' 未找到, 跳过", refTagLoc.toString());
            }
        }

        ++tagCount;
    }

    // 将合并后的标签注册到 FunctionManager
    for (auto& [tagLoc, functionIds] : mergedTags) {
        m_manager.registerTag(tagLoc, std::move(functionIds));
    }

    if (tagCount > 0) {
        spdlog::info("FunctionLoader: 从数据包加载了 {} 个函数标签", tagCount);
    }

    return tagCount;
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
            return Error(ErrorCode::InvalidData, "函数标签 '" + tagId.toString() + "' 缺少 'values' 数组");
        }

        for (const auto& value : jsonObj["values"]) {
            if (!value.is_string()) {
                spdlog::warn("FunctionLoader: 标签 '{}' 中的值不是字符串, 跳过", tagId.toString());
                continue;
            }

            std::string entry = value.get<std::string>();
            resolveTagEntry(entry, tagData.functionIds, tagData.tagReferences);
        }

        return tagData;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON 解析错误: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("解析 JSON 失败: ") + e.what());
    }
}

void FunctionLoader::resolveTagEntry(
    const std::string& entry, std::vector<ResourceLocation>& functionIds, std::vector<ResourceLocation>& tagReferences)
{
    if (entry.empty()) {
        return;
    }

    if (entry[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = entry.substr(1);
        ResourceLocation tagLoc = ResourceLocation::parse(tagRef);
        tagReferences.push_back(std::move(tagLoc));
    } else {
        // 直接函数引用: namespace:path
        ResourceLocation funcLoc = ResourceLocation::parse(entry);
        functionIds.push_back(std::move(funcLoc));
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

        // 宏行：以 $ 开头 - 当前版本跳过
        if (processedLine[0] == '$') {
            spdlog::warn("Macro function line skipped in function '{}' on line {}: {}", id, i + 1, processedLine);
            ++result.skippedMacroCount;
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
            result.commands.push_back(processedLine);
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

    // 查找 "functions/" 的位置
    Size functionsPos = genericPath.find("/functions/");
    Size functionsTokenSize = std::string("/functions/").size();
    if (functionsPos == std::string::npos) {
        functionsPos = genericPath.find("functions/");
        functionsTokenSize = std::string("functions/").size();
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
