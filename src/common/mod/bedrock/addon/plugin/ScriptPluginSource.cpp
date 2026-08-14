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

#include "common/mod/bedrock/addon/plugin/ScriptPluginSource.hpp"
#include "common/mod/bedrock/addon/core/ScriptData.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptPluginSource::ScriptPluginSource(const BehaviorPack& pack)
    : m_pack(pack)
{}

std::optional<ScriptData> ScriptPluginSource::loadScript(const std::string& moduleName)
{
    // 模块名称到文件路径的映射
    // 1. 如果是相对路径（如"./utils"），解析为行为包内的脚本路径
    // 2. 如果是@minecraft/*模块，由模块系统处理，不从这里加载

    if (moduleName.starts_with("@minecraft/")) {
        // 原生模块由引擎内部处理，不从文件系统加载
        return std::nullopt;
    }

    // QuickJS 的 moduleNormalize 会把"导入者文件路径 + import specifier"拼接成模块名，
    // 因此到达此处的 moduleName 可能是以下任一形态：
    //   (a) 纯 specifier："./utils/entity/assert.js"（仅 entry 首次 import 可能）
    //   (b) 含导入者目录前缀 + ./ 段："tests/integrated\mob_behavior/scripts/./tests/.../XxxTests.js"
    //   (c) 含 .. 段的回退路径。
    // readScriptFile 会用 m_path / relativePath 拼接，故这里必须把 moduleName 规整成
    // **相对 pack 内部**的路径（不含 pack 前缀、不含 . / .. 段），否则双重前缀或残留 . 段
    // 会导致文件找不到（多文件包 entry import 相对路径模块时必现）。
    namespace fs = std::filesystem;
    fs::path normalized = fs::path(moduleName).lexically_normal();
    std::string relPathStr = normalized.generic_string();

    // 剥离 pack 路径前缀：若 moduleName 形态 (b) 带 pack 前缀，lexically_normal 不会去掉它，
    // 需手动剥离。pack 路径分隔符可能含 '\'（Windows），统一比较 generic 形式。
    const std::string packPathGeneric = fs::path(m_pack.path()).lexically_normal().generic_string();
    if (!packPathGeneric.empty() && relPathStr.rfind(packPathGeneric + "/", 0) == 0) {
        relPathStr = relPathStr.substr(packPathGeneric.size() + 1);
    }

    // 处理残余的相对前缀（剥离 pack 前缀后可能暴露出开头的 "./"）
    while (relPathStr.starts_with("./")) {
        relPathStr = relPathStr.substr(2);
    }

    // 尝试添加.js扩展名
    if (!relPathStr.ends_with(".js")) {
        relPathStr += ".js";
    }

    auto result = m_pack.readScriptFile(relPathStr);
    if (result.failed()) {
        // 模块未找到，返回空值让调用者处理
        return std::nullopt;
    }

    ScriptData data;
    data.name = moduleName;
    data.source = std::move(result.value());
    data.filePath = m_pack.path() + "/" + relPathStr;
    data.isModule = true;
    return data;
}

std::optional<ScriptData> ScriptPluginSource::loadEntryPoint() const
{
    auto modules = m_pack.manifest().getScriptModules();
    if (modules.empty()) {
        spdlog::warn("[BedrockAddon] No script modules found in pack: {}", m_pack.name());
        return std::nullopt;
    }

    const auto& entryModule = modules[0];
    if (entryModule.entry.empty()) {
        spdlog::warn("[BedrockAddon] Script module has no entry point in pack: {}", m_pack.name());
        return std::nullopt;
    }

    auto result = m_pack.readScriptFile(entryModule.entry);
    if (result.failed()) {
        spdlog::error("[BedrockAddon] Failed to load entry point '{}' from pack '{}': {}",
            entryModule.entry,
            m_pack.name(),
            result.error().message());
        return std::nullopt;
    }

    ScriptData data;
    data.name = entryModule.name.empty() ? m_pack.name() : entryModule.name;
    data.source = std::move(result.value());
    data.filePath = m_pack.path() + "/" + entryModule.entry;
    data.isModule = true;
    return data;
}

const BehaviorPack& ScriptPluginSource::pack() const
{
    return m_pack;
}

} // namespace mc::mod::bedrock::addon
