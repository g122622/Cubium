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

#include "BannedIpList.hpp"

#include "common/core/Result.hpp"
#include "common/util/DateTimeUtils.hpp"
#include <chrono>
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

BannedIpList::BannedIpList() = default;

// ========== 条目管理 ==========

bool BannedIpList::addEntry(const BannedIpEntry& entry)
{
    if (!entry.isValid()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    if (m_entries.contains(entry.ip)) {
        return false;
    }

    // 添加条目
    m_entries[entry.ip] = entry;

    return true;
}

bool BannedIpList::removeEntry(const std::string& ip)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(ip);
    if (it == m_entries.end()) {
        return false;
    }

    m_entries.erase(it);
    return true;
}

bool BannedIpList::isBanned(const std::string& ip) const
{
    // 先清理过期条目
    _removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(ip);
    if (it == m_entries.end()) {
        return false;
    }

    return !it->second.hasExpired();
}

std::optional<BannedIpEntry> BannedIpList::getEntry(const std::string& ip) const
{
    // 先清理过期条目
    _removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(ip);
    if (it == m_entries.end()) {
        return std::nullopt;
    }

    // 检查是否过期
    if (it->second.hasExpired()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<BannedIpEntry> BannedIpList::getAllEntries() const
{
    // 先清理过期条目
    _removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<BannedIpEntry> entries;
    entries.reserve(m_entries.size());

    for (const auto& [ip, entry] : m_entries) {
        if (!entry.hasExpired()) {
            entries.push_back(entry);
        }
    }

    return entries;
}

std::vector<std::string> BannedIpList::getAllBannedIps() const
{
    // 先清理过期条目
    _removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> ips;
    ips.reserve(m_entries.size());

    for (const auto& [ip, entry] : m_entries) {
        if (!entry.hasExpired()) {
            ips.push_back(entry.ip);
        }
    }

    return ips;
}

size_t BannedIpList::size() const
{
    // 先清理过期条目
    _removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries.size();
}

bool BannedIpList::empty() const
{
    // 先清理过期条目
    _removeExpired();

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries.empty();
}

void BannedIpList::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

// ========== 文件操作 ==========

Result<void> BannedIpList::load(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_filePath = path;

    // 清空现有数据
    m_entries.clear();

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        spdlog::info("Banned IPs file not found, creating empty list: {}", path.string());
        // 创建空文件
        try {
            std::ofstream file(path);
            file << "[]";
        }
        catch (const std::exception& e) {
            return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to create banned IPs file: {}", e.what()));
        }
        return {};
    }

    // 读取文件
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open banned IPs file: {}", path.string()));
        }

        nlohmann::json json;
        file >> json;

        if (!json.is_array()) {
            return Error(ErrorCode::FileCorrupted, "Banned IPs file must be a JSON array");
        }

        for (const auto& item : json) {
            if (!item.is_object()) {
                spdlog::warn("Skipping invalid IP ban entry: not an object");
                continue;
            }

            BannedIpEntry entry;

            // 解析 IP
            if (item.contains("ip")) {
                entry.ip = item["ip"].get<std::string>();
            } else {
                spdlog::warn("Skipping IP ban entry: missing ip");
                continue;
            }

            // 解析创建时间
            if (item.contains("created")) {
                entry.created = item["created"].get<std::string>();
            } else {
                entry.created = _getCurrentTimeString();
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
                spdlog::warn("Skipping invalid IP ban entry: ip={}", entry.ip);
                continue;
            }

            // 检查重复 IP
            if (m_entries.contains(entry.ip)) {
                spdlog::warn("Skipping duplicate IP ban entry with ip: {}", entry.ip);
                continue;
            }

            // 跳过已过期的条目
            if (entry.hasExpired()) {
                continue;
            }

            // 添加条目
            m_entries[entry.ip] = entry;
        }

        spdlog::info("Loaded {} banned IP entries from {}", m_entries.size(), path.string());
        return {};
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to parse banned IPs JSON: {}", e.what()));
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileReadFailed, fmt::format("Failed to read banned IPs file: {}", e.what()));
    }
}

Result<void> BannedIpList::save(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::filesystem::path savePath = path.empty() ? m_filePath : path;
    if (savePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path specified for saving banned IPs");
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

        for (const auto& [ip, entry] : m_entries) {
            // 跳过已过期的条目
            if (entry.hasExpired()) {
                continue;
            }

            nlohmann::json item;
            item["ip"] = entry.ip;
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
                fmt::format("Failed to open banned IPs file for writing: {}", savePath.string()));
        }

        file << json.dump(2);

        spdlog::info("Saved {} banned IP entries to {}", json.size(), savePath.string());
        return {};
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to serialize banned IPs JSON: {}", e.what()));
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileWriteFailed, fmt::format("Failed to save banned IPs file: {}", e.what()));
    }
}

Result<void> BannedIpList::reload()
{
    if (m_filePath.empty()) {
        return Error(ErrorCode::InvalidArgument, "No file path to reload banned IPs from");
    }

    return load(m_filePath);
}

// ========== 私有方法 ==========

void BannedIpList::_removeExpired() const
{
    // 注意：此方法在持锁状态下由其他公共方法调用
    // 所以这里不需要再加锁

    std::vector<std::string> expiredIps;

    for (const auto& [ip, entry] : m_entries) {
        if (entry.hasExpired()) {
            expiredIps.push_back(ip);
        }
    }

    for (const auto& ip : expiredIps) {
        m_entries.erase(ip);
    }
}

std::string BannedIpList::_getCurrentTimeString()
{
    return util::DateTimeUtils::getCurrentDateTimeString();
}

// ========== BannedIpEntry 方法 ==========

bool BannedIpEntry::hasExpired() const
{
    if (expires.empty() || expires == "forever") {
        return false;
    }

    // 解析过期时间（使用与 MC Java 版兼容的日期时间格式）
    auto expireMillis = util::DateTimeUtils::parseDateTimeToMillis(expires);
    if (!expireMillis.has_value()) {
        // 解析失败，假设永不过期
        return false;
    }

    auto expireTime = util::DateTimeUtils::millisToTimePoint(expireMillis.value());
    auto now = std::chrono::system_clock::now();

    return now >= expireTime;
}

} // namespace mc::server::core
