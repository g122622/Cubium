#include "WhitelistManager.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc::server::core {

WhitelistManager::WhitelistManager() = default;

// ========== 白名单开关 ==========

bool WhitelistManager::isEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enabled;
}

void WhitelistManager::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = enabled;
}

// ========== 条目管理 ==========

bool WhitelistManager::addEntry(const WhitelistEntry& entry) {
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
    std::string lowerName = entry.name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    m_nameToUuid[lowerName] = entry.uuid;

    return true;
}

bool WhitelistManager::removeEntry(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return false;
    }

    // 移除名称映射
    std::string lowerName = it->second.name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    m_nameToUuid.erase(lowerName);

    // 移除条目
    m_entriesByUuid.erase(it);

    return true;
}

bool WhitelistManager::removeEntryByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 查找名称映射
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto nameIt = m_nameToUuid.find(lowerName);
    if (nameIt == m_nameToUuid.end()) {
        return false;
    }

    std::string uuid = nameIt->second;

    // 移除条目
    m_entriesByUuid.erase(uuid);
    m_nameToUuid.erase(nameIt);

    return true;
}

bool WhitelistManager::isWhitelisted(const std::string& uuid) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.contains(uuid);
}

bool WhitelistManager::isNameWhitelisted(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    return m_nameToUuid.contains(lowerName);
}

std::optional<WhitelistEntry> WhitelistManager::getEntry(const std::string& uuid) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<WhitelistEntry> WhitelistManager::getEntryByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto nameIt = m_nameToUuid.find(lowerName);
    if (nameIt == m_nameToUuid.end()) {
        return std::nullopt;
    }

    auto entryIt = m_entriesByUuid.find(nameIt->second);
    if (entryIt == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    return entryIt->second;
}

std::vector<WhitelistEntry> WhitelistManager::getAllEntries() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<WhitelistEntry> entries;
    entries.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        entries.push_back(entry);
    }

    return entries;
}

std::vector<std::string> WhitelistManager::getAllNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> names;
    names.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        names.push_back(entry.name);
    }

    return names;
}

size_t WhitelistManager::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.size();
}

bool WhitelistManager::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.empty();
}

void WhitelistManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entriesByUuid.clear();
    m_nameToUuid.clear();
}

// ========== 文件操作 ==========

Result<void> WhitelistManager::load(const std::filesystem::path& path) {
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
        } catch (const std::exception& e) {
            return Error(ErrorCode::FileWriteFailed,
                        fmt::format("Failed to create whitelist file: {}", e.what()));
        }
        return {};
    }

    // 读取文件
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed,
                        fmt::format("Failed to open whitelist file: {}", path.string()));
        }

        nlohmann::json json;
        file >> json;

        if (!json.is_array()) {
            return Error(ErrorCode::FileCorrupted,
                        "Whitelist file must be a JSON array");
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
                spdlog::warn("Skipping invalid whitelist entry: uuid={}, name={}",
                            entry.uuid, entry.name);
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
            std::string lowerName = entry.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            m_nameToUuid[lowerName] = entry.uuid;
        }

        spdlog::info("Loaded {} whitelist entries from {}", m_entriesByUuid.size(), path.string());
        return {};

    } catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileCorrupted,
                    fmt::format("Failed to parse whitelist JSON: {}", e.what()));
    } catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed,
                    fmt::format("Failed to read whitelist file: {}", e.what()));
    }
}

Result<void> WhitelistManager::save(const std::filesystem::path& path) {
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

    } catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileWriteFailed,
                    fmt::format("Failed to serialize whitelist JSON: {}", e.what()));
    } catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed,
                    fmt::format("Failed to save whitelist file: {}", e.what()));
    }
}

Result<void> WhitelistManager::reload() {
    if (m_filePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path to reload whitelist from");
    }

    return load(m_filePath);
}

} // namespace mc::server::core
