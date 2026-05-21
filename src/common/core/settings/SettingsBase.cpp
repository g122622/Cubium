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

#include "SettingsBase.hpp"

#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // 禁用 getenv 不安全警告
#endif

namespace mc {

Result<void> SettingsBase::loadOrGenerate(const std::filesystem::path& path)
{
    spdlog::debug("Loading or generating settings at: {}", path.string());

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        spdlog::info("Settings file not found, generating default: {}", path.string());

        // 先重置为默认值
        resetToDefaults();

        // 生成默认配置文件
        auto genResult = generateDefaultConfig(path);
        if (genResult.failed()) {
            spdlog::warn("Failed to generate default settings file: {}", genResult.error().toString());
            // 生成失败不影响程序运行，使用内存中的默认值
            return Result<void>::ok();
        }

        spdlog::info("Default settings file generated: {}", path.string());
        return Result<void>::ok();
    }

    // 文件存在，正常加载
    return load(path);
}

Result<void> SettingsBase::load(const std::filesystem::path& path)
{
    spdlog::debug("Loading settings from: {}", path.string());

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        spdlog::info("Settings file not found, using defaults: {}", path.string());
        return Result<void>::ok();
    }

    // 读取文件内容
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open settings file: " + path.string());
    }

    try {
        // 解析 JSON
        nlohmann::json j;
        file >> j;
        file.close();

        // 加载设置
        loadFromJson(j);

        spdlog::info("Settings loaded successfully from: {}", path.string());
        return Result<void>::ok();
    }
    catch (const nlohmann::json::parse_error& e) {
        file.close();
        return Error(ErrorCode::FileCorrupted, "Failed to parse settings file: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        file.close();
        return Error(ErrorCode::FileReadFailed, "Failed to read settings file: " + std::string(e.what()));
    }
}

Result<void> SettingsBase::save(const std::filesystem::path& path) const
{
    spdlog::debug("Saving settings to: {}", path.string());

    // 确保目录存在
    std::filesystem::path dir = path.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(dir, ec)) {
            return Error(ErrorCode::FileWriteFailed, "Failed to create settings directory: " + dir.string());
        }
    }

    try {
        // 构建 JSON 对象
        nlohmann::json j;
        saveToJson(j);

        // 写入文件
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, "Failed to create settings file: " + path.string());
        }

        file << j.dump(4); // 美化输出，缩进 4 空格
        file.close();

        m_dirty = false;
        spdlog::info("Settings saved successfully to: {}", path.string());
        return Result<void>::ok();
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, "Failed to write settings file: " + std::string(e.what()));
    }
}

void SettingsBase::loadFromJson(const nlohmann::json& j)
{
    // 加载版本号
    if (j.contains("version") && j["version"].is_number_integer()) {
        m_version = j["version"].get<i32>();
    }

    // 加载各分组
    for (auto& [group, options] : m_options) {
        if (j.contains(group) && j[group].is_object()) {
            const auto& groupJson = j[group];
            for (auto* option : options) {
                option->deserialize(groupJson);
            }
        }
    }
}

void SettingsBase::saveToJson(nlohmann::json& j) const
{
    // 保存版本号
    j["version"] = m_version;

    // 保存各分组
    for (const auto& [group, options] : m_options) {
        nlohmann::json groupJson;
        for (const auto* option : options) {
            option->serialize(groupJson);
        }
        j[group] = groupJson;
    }
}

void SettingsBase::registerOption(const std::string& group, IOption* option)
{
    if (option == nullptr) {
        spdlog::warn("Attempted to register null option in group: {}", group);
        return;
    }

    m_options[group].push_back(option);
    spdlog::trace("Registered option '{}' in group '{}'", option->getKey(), group);
}

void SettingsBase::resetToDefaults()
{
    for (auto& [group, options] : m_options) {
        for (auto* option : options) {
            option->reset();
        }
    }
    spdlog::debug("All settings reset to defaults");
}

void SettingsBase::resetGroupToDefaults(const std::string& group)
{
    auto it = m_options.find(group);
    if (it != m_options.end()) {
        for (auto* option : it->second) {
            option->reset();
        }
        spdlog::debug("Settings group '{}' reset to defaults", group);
    }
}

void SettingsBase::enableAutoSave(std::filesystem::path path)
{
    m_autoSave = true;
    m_autoSavePath = std::move(path);
    spdlog::debug("Auto-save enabled for settings: {}", m_autoSavePath.string());
}

void SettingsBase::disableAutoSave()
{
    m_autoSave = false;
    spdlog::debug("Auto-save disabled for settings");
}

void SettingsBase::triggerAutoSave() const
{
    if (m_autoSave && m_dirty) {
        // 忽略错误，自动保存失败不应影响程序运行
        auto result = save(m_autoSavePath);
        if (result.failed()) {
            spdlog::warn("Auto-save failed: {}", result.error().toString());
        }
    }
}

void SettingsBase::onSettingChanged()
{
    m_dirty = true;
    triggerAutoSave();
}

std::filesystem::path SettingsBase::getSettingsPath(const std::string& appName)
{
    std::filesystem::path basePath;

#if defined(_WIN32)
    // Windows: %APPDATA%/appName/options.json
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        basePath = std::filesystem::path(appData);
    } else {
        // 回退到用户主目录
        const char* userProfile = std::getenv("USERPROFILE");
        if (userProfile) {
            basePath = std::filesystem::path(userProfile) / "AppData" / "Roaming";
        } else {
            basePath = std::filesystem::current_path();
        }
    }
#elif defined(__APPLE__)
    // macOS: ~/Library/Application Support/appName/options.json
    const char* home = std::getenv("HOME");
    if (home) {
        basePath = std::filesystem::path(home) / "Library" / "Application Support";
    } else {
        basePath = std::filesystem::current_path();
    }
#else
    // Linux: ~/.config/appName/options.json
    const char* home = std::getenv("HOME");
    if (home) {
        basePath = std::filesystem::path(home) / ".config";
    } else {
        basePath = std::filesystem::current_path();
    }
#endif

    return basePath / appName / "options.json";
}

std::filesystem::path SettingsBase::ensureSettingsDir(const std::string& appName)
{
    auto path = getSettingsPath(appName);
    auto dir = path.parent_path();

    if (!std::filesystem::exists(dir)) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            spdlog::warn("Failed to create settings directory: {}", dir.string());
        }
    }

    return dir;
}

} // namespace mc

#ifdef _MSC_VER
#pragma warning(pop)
#endif
