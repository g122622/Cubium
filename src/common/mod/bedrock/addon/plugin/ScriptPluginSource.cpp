#include "common/mod/bedrock/addon/plugin/ScriptPluginSource.hpp"

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

    // 尝试解析为行为包内的脚本文件
    std::string relativePath = moduleName;

    // 处理相对路径前缀
    if (relativePath.starts_with("./")) {
        relativePath = relativePath.substr(2);
    }

    // 尝试添加.js扩展名
    if (!relativePath.ends_with(".js")) {
        relativePath += ".js";
    }

    auto result = m_pack.readScriptFile(relativePath);
    if (result.failed()) {
        spdlog::debug("[BedrockAddon] Script module not found: {} in pack {}", moduleName, m_pack.name());
        return std::nullopt;
    }

    ScriptData data;
    data.name = moduleName;
    data.source = std::move(result.value());
    data.filePath = m_pack.path() + "/" + relativePath;
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
