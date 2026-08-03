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

#include "WhitelistManager.hpp"
#include "common/core/Result.hpp"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::server::core {

WhitelistManager::WhitelistManager() = default;

// ========== 白名单开关 ==========

bool WhitelistManager::isEnabled() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enabled;
}

void WhitelistManager::setEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = enabled;
}

// ========== 条目管理 ==========

bool WhitelistManager::addEntry(const WhitelistEntry& entry)
{
    if (!entry.isValid()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    if (m_entriesByUuid.contains(entry.uuid)) {
        return false;
    }

    // 添加条目
    m_entriesByUuid[entry.uuid] = entry;

    // 添加名称映射（小写）
    m_nameToUuid[_toLower(entry.name)] = entry.uuid;

    return true;
}

bool WhitelistManager::removeEntry(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return false;
    }

    // 移除名称映射
    m_nameToUuid.erase(_toLower(it->second.name));

    // 移除条目
    m_entriesByUuid.erase(it);

    return true;
}

bool WhitelistManager::removeEntryByName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 查找名称映射
    auto nameIt = m_nameToUuid.find(_toLower(name));
    if (nameIt == m_nameToUuid.end()) {
        return false;
    }

    std::string uuid = nameIt->second;

    // 移除条目
    m_entriesByUuid.erase(uuid);
    m_nameToUuid.erase(nameIt);

    return true;
}

bool WhitelistManager::isWhitelisted(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.contains(uuid);
}

bool WhitelistManager::isNameWhitelisted(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nameToUuid.contains(_toLower(name));
}

std::optional<WhitelistEntry> WhitelistManager::getEntry(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<WhitelistEntry> WhitelistManager::getEntryByName(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto nameIt = m_nameToUuid.find(_toLower(name));
    if (nameIt == m_nameToUuid.end()) {
        return std::nullopt;
    }

    auto entryIt = m_entriesByUuid.find(nameIt->second);
    if (entryIt == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    return entryIt->second;
}

std::vector<WhitelistEntry> WhitelistManager::getAllEntries() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<WhitelistEntry> entries;
    entries.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        entries.push_back(entry);
    }

    return entries;
}

std::vector<std::string> WhitelistManager::getAllNames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> names;
    names.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        names.push_back(entry.name);
    }

    return names;
}

size_t WhitelistManager::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.size();
}

bool WhitelistManager::empty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.empty();
}

void WhitelistManager::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entriesByUuid.clear();
    m_nameToUuid.clear();
}

// ========== 文件操作 ==========

Result<void> WhitelistManager::load(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_filePath = path;

    // 清空现有数据
    m_entriesByUuid.clear();
    m_nameToUuid.clear();

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        spdlog::info("Whitelist file not found, creating empty whitelist: {}", path.string());
        // 创建空文件
        try {
            std::ofstream file(path);
            file << "[]";
        }
        catch (const std::exception& e) {
            return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create whitelist file: {}", e.what()));
        }
        return {};
    }

    // 读取文件
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open whitelist file: {}", path.string()));
        }

        nlohmann::json json;
        file >> json;

        if (!json.is_array()) {
            return Error(ErrorCode::FileCorrupted, "Whitelist file must be a JSON array");
        }

        for (const auto& item : json) {
            if (!item.is_object()) {
                spdlog::warn("Skipping invalid whitelist entry: not an object");
                continue;
            }

            WhitelistEntry entry;

            // 解析 UUID
            if (item.contains("uuid")) {
                entry.uuid = item["uuid"].get<std::string>();
            } else {
                spdlog::warn("Skipping whitelist entry: missing uuid");
                continue;
            }

            // 解析名称
            if (item.contains("name")) {
                entry.name = item["name"].get<std::string>();
            } else {
                spdlog::warn("Skipping whitelist entry: missing name");
                continue;
            }

            // 验证条目
            if (!entry.isValid()) {
                spdlog::warn("Skipping invalid whitelist entry: uuid={}, name={}", entry.uuid, entry.name);
                continue;
            }

            // 检查重复 UUID
            if (m_entriesByUuid.contains(entry.uuid)) {
                spdlog::warn("Skipping duplicate whitelist entry with uuid: {}", entry.uuid);
                continue;
            }

            // 添加条目
            m_entriesByUuid[entry.uuid] = entry;

            // 添加名称映射
            m_nameToUuid[_toLower(entry.name)] = entry.uuid;
        }

        spdlog::info("Loaded {} whitelist entries from {}", m_entriesByUuid.size(), path.string());
        return {};
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to parse whitelist JSON: {}", e.what()));
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed, fmt::format("Failed to read whitelist file: {}", e.what()));
    }
}

Result<void> WhitelistManager::save(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::filesystem::path savePath = path.empty() ? m_filePath : path;
    if (savePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path specified for saving whitelist");
    }

    m_filePath = savePath;

    try {
        // 确保目录存在
        std::filesystem::path parentDir = savePath.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }

        // 构建 JSON 数组
        nlohmann::json json = nlohmann::json::array();

        for (const auto& [uuid, entry] : m_entriesByUuid) {
            nlohmann::json item;
            item["uuid"] = entry.uuid;
            item["name"] = entry.name;
            json.push_back(item);
        }

        // 写入文件
        std::ofstream file(savePath);
        if (!file.is_open()) {
            return Error(ErrorCode::FileWriteFailed,
                fmt::format("Failed to open whitelist file for writing: {}", savePath.string()));
        }

        file << json.dump(2);

        spdlog::info("Saved {} whitelist entries to {}", m_entriesByUuid.size(), savePath.string());
        return {};
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to serialize whitelist JSON: {}", e.what()));
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to save whitelist file: {}", e.what()));
    }
}

Result<void> WhitelistManager::reload()
{
    std::filesystem::path path;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_filePath.empty()) {
            return Error(ErrorCode::InvalidArgument, "No file path to reload whitelist from");
        }
        path = m_filePath;
    }
    return load(path);
}

} // namespace mc::server::core
