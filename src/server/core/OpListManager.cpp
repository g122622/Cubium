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

#include "OpListManager.hpp"

#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::core {

OpListManager::OpListManager() = default;

// ========== 私有方法 ==========

std::string OpListManager::_toLowerName(const std::string& name)
{
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    return lowerName;
}

// ========== 条目管理 ==========

bool OpListManager::setEntry(const OpEntry& entry)
{
    if (!entry.isValid()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 添加或更新条目
    m_entriesByUuid[entry.uuid] = entry;

    // 更新名称映射（小写）
    m_nameToUuid[_toLowerName(entry.name)] = entry.uuid;

    return true;
}

bool OpListManager::removeEntry(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return false;
    }

    // 移除名称映射
    m_nameToUuid.erase(_toLowerName(it->second.name));

    // 移除条目
    m_entriesByUuid.erase(it);

    return true;
}

bool OpListManager::removeEntryByName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 查找名称映射
    auto nameIt = m_nameToUuid.find(_toLowerName(name));
    if (nameIt == m_nameToUuid.end()) {
        return false;
    }

    std::string uuid = nameIt->second;

    // 移除条目
    m_entriesByUuid.erase(uuid);
    m_nameToUuid.erase(nameIt);

    return true;
}

bool OpListManager::isOp(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.contains(uuid);
}

bool OpListManager::isNameOp(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nameToUuid.contains(_toLowerName(name));
}

OpLevel OpListManager::getLevel(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return OpLevel::Normal;
    }

    return it->second.level;
}

OpLevel OpListManager::getLevelByName(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto nameIt = m_nameToUuid.find(_toLowerName(name));
    if (nameIt == m_nameToUuid.end()) {
        return OpLevel::Normal;
    }

    auto entryIt = m_entriesByUuid.find(nameIt->second);
    if (entryIt == m_entriesByUuid.end()) {
        return OpLevel::Normal;
    }

    return entryIt->second.level;
}

bool OpListManager::bypassesPlayerLimit(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return false;
    }

    return it->second.bypassesPlayerLimit;
}

std::optional<OpEntry> OpListManager::getEntry(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<OpEntry> OpListManager::getEntryByName(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto nameIt = m_nameToUuid.find(_toLowerName(name));
    if (nameIt == m_nameToUuid.end()) {
        return std::nullopt;
    }

    auto entryIt = m_entriesByUuid.find(nameIt->second);
    if (entryIt == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    return entryIt->second;
}

std::vector<OpEntry> OpListManager::getAllEntries() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<OpEntry> entries;
    entries.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        entries.push_back(entry);
    }

    return entries;
}

std::vector<std::string> OpListManager::getAllNames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> names;
    names.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        names.push_back(entry.name);
    }

    return names;
}

size_t OpListManager::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.size();
}

bool OpListManager::empty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.empty();
}

void OpListManager::clear() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entriesByUuid.clear();
    m_nameToUuid.clear();
}

// ========== 文件操作 ==========

Result<void> OpListManager::load(const std::filesystem::path& path)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "OpListManager::load", "path", path.string());

    std::lock_guard<std::mutex> lock(m_mutex);

    m_filePath = path;

    // 清空现有数据
    m_entriesByUuid.clear();
    m_nameToUuid.clear();

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        spdlog::info("Ops file not found, creating empty list: {}", path.string());
        // 创建空文件
        try {
            std::ofstream file(path);
            file << "[]";
        }
        catch (const std::exception& e) {
            return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create ops file: {}", e.what()));
        }
        return {};
    }

    // 读取文件
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open ops file: {}", path.string()));
        }

        nlohmann::json json;
        file >> json;

        if (!json.is_array()) {
            return Error(ErrorCode::FileCorrupted, "Ops file must be a JSON array");
        }

        for (const auto& item : json) {
            if (!item.is_object()) {
                spdlog::warn("Skipping invalid op entry: not an object");
                continue;
            }

            OpEntry entry;

            // 解析 UUID
            if (item.contains("uuid")) {
                entry.uuid = item["uuid"].get<std::string>();
            } else {
                spdlog::warn("Skipping op entry: missing uuid");
                continue;
            }

            // 解析名称
            if (item.contains("name")) {
                entry.name = item["name"].get<std::string>();
            } else {
                spdlog::warn("Skipping op entry: missing name");
                continue;
            }

            // 解析权限等级
            if (item.contains("level")) {
                i32 level = item["level"].get<i32>();
                // 限制等级范围 0-4
                if (level < 0) level = 0;
                if (level > 4) level = 4;
                entry.level = static_cast<OpLevel>(level);
            } else {
                // MC 1.16.5 默认等级为 2
                entry.level = OpLevel::GameMaster;
            }

            // 解析 bypassesPlayerLimit
            if (item.contains("bypassesPlayerLimit")) {
                entry.bypassesPlayerLimit = item["bypassesPlayerLimit"].get<bool>();
            } else {
                entry.bypassesPlayerLimit = false;
            }

            // 验证条目
            if (!entry.isValid()) {
                spdlog::warn("Skipping invalid op entry: uuid={}, name={}", entry.uuid, entry.name);
                continue;
            }

            // 检查重复 UUID
            if (m_entriesByUuid.contains(entry.uuid)) {
                spdlog::warn("Skipping duplicate op entry with uuid: {}", entry.uuid);
                continue;
            }

            // 添加条目
            m_entriesByUuid[entry.uuid] = entry;

            // 添加名称映射
            m_nameToUuid[_toLowerName(entry.name)] = entry.uuid;
        }

        spdlog::info("Loaded {} op entries from {}", m_entriesByUuid.size(), path.string());
        return {};
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to parse ops JSON: {}", e.what()));
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed, fmt::format("Failed to read ops file: {}", e.what()));
    }
}

Result<void> OpListManager::save(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::filesystem::path savePath = path.empty() ? m_filePath : path;
    if (savePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path specified for saving ops");
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
            item["level"] = entry.getLevelValue();
            item["bypassesPlayerLimit"] = entry.bypassesPlayerLimit;
            json.push_back(item);
        }

        // 写入文件
        std::ofstream file(savePath);
        if (!file.is_open()) {
            return Error(
                ErrorCode::FileWriteFailed, fmt::format("Failed to open ops file for writing: {}", savePath.string()));
        }

        file << json.dump(2);

        spdlog::info("Saved {} op entries to {}", json.size(), savePath.string());
        return {};
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to serialize ops JSON: {}", e.what()));
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to save ops file: {}", e.what()));
    }
}

Result<void> OpListManager::reload()
{
    if (m_filePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path to reload ops from");
    }

    return load(m_filePath);
}

} // namespace mc::server::core
