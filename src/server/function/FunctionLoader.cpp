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
 * LIABILITY, WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FunctionLoader.hpp"
#include "FunctionManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <filesystem>
#include <sstream>
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

    // TODO: 加载函数标签（data/<namespace>/tags/functions/*.json）
    // 函数标签用于将多个函数分组，例如 minecraft:tick 标签中的函数每 tick 自动执行，
    // minecraft:load 标签中的函数在重载后首次 tick 执行。
    // 标签文件的 JSON 格式：{ "values": ["namespace:path", "#namespace:tag"] }
    // 需要实现标签加载、合并（数据包覆盖/追加）和注册到 FunctionManager.registerTag()。

    return result;
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

} // namespace function
} // namespace mc
