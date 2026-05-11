#include "BannedPlayerList.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace mc::server::core {

// 日期格式：yyyy-MM-dd HH:mm:ss Z
static const char* DATE_FORMAT = "%Y-%m-%d %H:%M:%S %z";

BannedPlayerList::BannedPlayerList() = default;

// ========== 条目管理 ==========

bool BannedPlayerList::addEntry(const BannedPlayerEntry& entry) {
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

bool BannedPlayerList::removeEntry(const std::string& uuid) {
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

bool BannedPlayerList::removeEntryByName(const std::string& name) {
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

bool BannedPlayerList::isBanned(const std::string& uuid) const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.contains(uuid);
}

bool BannedPlayerList::isNameBanned(const std::string& name) const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto nameIt = m_nameToUuid.find(lowerName);
    if (nameIt == m_nameToUuid.end()) {
        return false;
    }

    // 检查条目是否过期
    auto entryIt = m_entriesByUuid.find(nameIt->second);
    if (entryIt == m_entriesByUuid.end()) {
        return false;
    }

    return !entryIt->second.hasExpired();
}

std::optional<BannedPlayerEntry> BannedPlayerList::getEntry(const std::string& uuid) const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entriesByUuid.find(uuid);
    if (it == m_entriesByUuid.end()) {
        return std::nullopt;
    }

    // 检查是否过期
    if (it->second.hasExpired()) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<BannedPlayerEntry> BannedPlayerList::getEntryByName(const std::string& name) const {
    // 先清理过期条目
    removeExpired();

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

    // 检查是否过期
    if (entryIt->second.hasExpired()) {
        return std::nullopt;
    }

    return entryIt->second;
}

std::vector<BannedPlayerEntry> BannedPlayerList::getAllEntries() const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<BannedPlayerEntry> entries;
    entries.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        if (!entry.hasExpired()) {
            entries.push_back(entry);
        }
    }

    return entries;
}

std::vector<std::string> BannedPlayerList::getAllBannedNames() const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> names;
    names.reserve(m_entriesByUuid.size());

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        if (!entry.hasExpired()) {
            names.push_back(entry.name);
        }
    }

    return names;
}

size_t BannedPlayerList::size() const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.size();
}

bool BannedPlayerList::empty() const {
    // 先清理过期条目
    removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entriesByUuid.empty();
}

void BannedPlayerList::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entriesByUuid.clear();
    m_nameToUuid.clear();
}

// ========== 文件操作 ==========

Result<void> BannedPlayerList::load(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_filePath = path;

    // 清空现有数据
    m_entriesByUuid.clear();
    m_nameToUuid.clear();

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        spdlog::info("Banned players file not found, creating empty list: {}", path.string());
        // 创建空文件
        try {
            std::ofstream file(path);
            file << "[]";
        } catch (const std::exception& e) {
            return Error(ErrorCode::FileWriteFailed,
                        fmt::format("Failed to create banned players file: {}", e.what()));
        }
        return {};
    }

    // 读取文件
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed,
                        fmt::format("Failed to open banned players file: {}", path.string()));
        }

        nlohmann::json json;
        file >> json;

        if (!json.is_array()) {
            return Error(ErrorCode::FileCorrupted,
                        "Banned players file must be a JSON array");
        }

        for (const auto& item : json) {
            if (!item.is_object()) {
                spdlog::warn("Skipping invalid ban entry: not an object");
                continue;
            }

            BannedPlayerEntry entry;

            // 解析 UUID
            if (item.contains("uuid")) {
                entry.uuid = item["uuid"].get<std::string>();
            } else {
                spdlog::warn("Skipping ban entry: missing uuid");
                continue;
            }

            // 解析名称
            if (item.contains("name")) {
                entry.name = item["name"].get<std::string>();
            } else {
                spdlog::warn("Skipping ban entry: missing name");
                continue;
            }

            // 解析创建时间
            if (item.contains("created")) {
                entry.created = item["created"].get<std::string>();
            } else {
                entry.created = getCurrentTimeString();
            }

            // 解析封禁来源
            if (item.contains("source")) {
                entry.source = item["source"].get<std::string>();
            } else {
                entry.source = "(Unknown)";
            }

            // 解析过期时间
            if (item.contains("expires")) {
                entry.expires = item["expires"].get<std::string>();
            } else {
                entry.expires = "forever";
            }

            // 解析封禁原因
            if (item.contains("reason")) {
                entry.reason = item["reason"].get<std::string>();
            } else {
                entry.reason = "Banned by an operator.";
            }

            // 验证条目
            if (!entry.isValid()) {
                spdlog::warn("Skipping invalid ban entry: uuid={}, name={}",
                            entry.uuid, entry.name);
                continue;
            }

            // 检查重复 UUID
            if (m_entriesByUuid.contains(entry.uuid)) {
                spdlog::warn("Skipping duplicate ban entry with uuid: {}", entry.uuid);
                continue;
            }

            // 跳过已过期的条目
            if (entry.hasExpired()) {
                spdlog::debug("Skipping expired ban entry: uuid={}, name={}",
                             entry.uuid, entry.name);
                continue;
            }

            // 添加条目
            m_entriesByUuid[entry.uuid] = entry;

            // 添加名称映射
            std::string lowerName = entry.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            m_nameToUuid[lowerName] = entry.uuid;
        }

        spdlog::info("Loaded {} banned player entries from {}", m_entriesByUuid.size(), path.string());
        return {};

    } catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileCorrupted,
                    fmt::format("Failed to parse banned players JSON: {}", e.what()));
    } catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed,
                    fmt::format("Failed to read banned players file: {}", e.what()));
    }
}

Result<void> BannedPlayerList::save(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::filesystem::path savePath = path.empty() ? m_filePath : path;
    if (savePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path specified for saving banned players");
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
            // 跳过已过期的条目
            if (entry.hasExpired()) {
                continue;
            }

            nlohmann::json item;
            item["uuid"] = entry.uuid;
            item["name"] = entry.name;
            item["created"] = entry.created;
            item["source"] = entry.source;
            item["expires"] = entry.expires;
            item["reason"] = entry.reason;
            json.push_back(item);
        }

        // 写入文件
        std::ofstream file(savePath);
        if (!file.is_open()) {
            return Error(ErrorCode::FileWriteFailed,
                        fmt::format("Failed to open banned players file for writing: {}", savePath.string()));
        }

        file << json.dump(2);

        spdlog::info("Saved {} banned player entries to {}", json.size(), savePath.string());
        return {};

    } catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileWriteFailed,
                    fmt::format("Failed to serialize banned players JSON: {}", e.what()));
    } catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed,
                    fmt::format("Failed to save banned players file: {}", e.what()));
    }
}

Result<void> BannedPlayerList::reload() {
    if (m_filePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path to reload banned players from");
    }

    return load(m_filePath);
}

// ========== 私有方法 ==========

void BannedPlayerList::removeExpired() const {
    // 注意：此方法在持锁状态下由其他公共方法调用
    // 所以这里不需要再加锁

    std::vector<std::string> expiredUuids;

    for (const auto& [uuid, entry] : m_entriesByUuid) {
        if (entry.hasExpired()) {
            expiredUuids.push_back(uuid);
        }
    }

    for (const auto& uuid : expiredUuids) {
        auto it = m_entriesByUuid.find(uuid);
        if (it != m_entriesByUuid.end()) {
            // 移除名称映射
            std::string lowerName = it->second.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            m_nameToUuid.erase(lowerName);
            m_entriesByUuid.erase(it);
        }
    }
}

std::string BannedPlayerList::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now_time);
#else
    localtime_r(&now_time, &tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm, DATE_FORMAT);
    return ss.str();
}

// ========== BannedPlayerEntry 方法 ==========

bool BannedPlayerEntry::hasExpired() const {
    if (expires.empty() || expires == "forever") {
        return false;
    }

    // 解析过期时间
    std::tm tm = {};
    std::istringstream ss(expires);
    ss >> std::get_time(&tm, DATE_FORMAT);

    if (ss.fail()) {
        // 解析失败，假设永不过期
        return false;
    }

    // 转换为时间点
    auto expireTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto now = std::chrono::system_clock::now();

    return now >= expireTime;
}

} // namespace mc::server::core
