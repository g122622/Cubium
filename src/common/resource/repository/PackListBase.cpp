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

#include "PackListBase.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/ZipResourcePack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace mc::resource {

namespace {

std::string _normalizePathSeparators(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string _toLowerAscii(std::string value)
{
    std::transform(
        value.begin(), value.end(), value.begin(), [](u8 ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

} // namespace

PackListBase::PackListBase(PackType defaultType)
    : m_defaultType(defaultType)
{}

Result<size_t> PackListBase::scanDirectory(const std::filesystem::path& dir)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "PackListBase::scanDirectory");

    if (!std::filesystem::exists(dir)) {
        return static_cast<size_t>(0);
    }

    if (!std::filesystem::is_directory(dir)) {
        return Error(ErrorCode::InvalidArgument, "Path is not a directory: " + dir.string());
    }

    const char* packTypeName = m_defaultType == PackType::ClientResources ? "resource" : "data";

    size_t addedCount = 0;
    i32 nextPriority = static_cast<i32>(packCount());

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const auto& path = entry.path();

        std::string normalizedPath = _normalizePathKey(path);
        if (containsPack(normalizedPath)) {
            continue;
        }

        bool isZip = _isZipFile(path);
        bool isPackDir = !isZip && _isPackDir(path);

        if (!isZip && !isPackDir) {
            continue;
        }

        auto result = addPack(path, true, nextPriority++);
        if (result.success()) {
            ++addedCount;
            if (result.value().initialized) {
                spdlog::info(
                    "Found {} pack: {} ({})", packTypeName, result.value().pack->name(), isZip ? "ZIP" : "folder");
            } else {
                spdlog::warn("{} pack failed to initialize: {} - {}",
                    packTypeName,
                    path.filename().string(),
                    result.value().error);
            }
        } else {
            spdlog::warn(
                "Failed to add {} pack {}: {}", packTypeName, path.filename().string(), result.error().toString());
        }
    }

    _notifyChange();
    return addedCount;
}

Result<PackListBase::PackInfo> PackListBase::addPack(const std::filesystem::path& path, bool enabled, i32 priority)
{
    if (!std::filesystem::exists(path)) {
        return Error(ErrorCode::FileNotFound, "Pack not found: " + path.string());
    }

    std::string normalizedPath = _normalizePathKey(path);

    {
        bool foundExisting = false;
        bool changed = false;
        PackInfo existing;
        {
            std::unique_lock lock(m_mutex);
            auto existingIt = std::find_if(
                m_packs.begin(), m_packs.end(), [&](const PackInfo& pack) { return pack.path == normalizedPath; });

            if (existingIt != m_packs.end()) {
                foundExisting = true;
                changed = (existingIt->enabled != enabled) || (existingIt->priority != priority);
                existingIt->enabled = enabled;
                existingIt->priority = priority;
                existing = *existingIt;
            }
        }

        if (foundExisting) {
            if (changed) {
                _notifyChange();
            }
            return existing;
        }
    }

    PackInfo info;
    info.path = normalizedPath;
    info.enabled = enabled;
    info.priority = priority;
    info.isZip = _isZipFile(path);

    if (info.isZip) {
        auto result = ZipResourcePack::create(path);
        if (result.failed()) {
            info.initialized = false;
            info.error = result.error().toString();
        } else {
            info.pack = std::shared_ptr<IResourcePack>(result.value().release());
        }
    } else {
        info.pack = std::make_shared<FolderResourcePack>(path.string());
    }

    if (info.pack) {
        auto initResult = info.pack->initialize();
        if (initResult.failed()) {
            info.initialized = false;
            info.error = initResult.error().toString();
            spdlog::warn("Failed to initialize pack {}: {}", path.filename().string(), info.error);
        } else {
            info.initialized = true;
        }
    }

    bool shouldNotify = false;
    PackInfo resultInfo;
    {
        std::unique_lock lock(m_mutex);

        auto existingIt = std::find_if(
            m_packs.begin(), m_packs.end(), [&](const PackInfo& pack) { return pack.path == normalizedPath; });

        if (existingIt != m_packs.end()) {
            bool changed = (existingIt->enabled != enabled) || (existingIt->priority != priority);
            existingIt->enabled = enabled;
            existingIt->priority = priority;
            shouldNotify = changed;
            resultInfo = *existingIt;
        } else {
            m_packs.push_back(info);
            shouldNotify = true;
            resultInfo = info;
        }
    }

    if (shouldNotify) {
        _notifyChange();
    }

    return resultInfo;
}

bool PackListBase::removePack(const std::string& path)
{
    std::string normalizedPath = _normalizePathKey(std::filesystem::path(path));

    bool removed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(
            m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == normalizedPath; });

        if (it != m_packs.end()) {
            m_packs.erase(it);
            removed = true;
        }
    }

    if (removed) {
        _notifyChange();
    }

    return removed;
}

void PackListBase::clear()
{
    {
        std::unique_lock lock(m_mutex);
        m_packs.clear();
    }
    _notifyChange();
}

bool PackListBase::setEnabled(const std::string& path, bool enabled)
{
    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });

        if (it != m_packs.end() && it->enabled != enabled) {
            it->enabled = enabled;
            changed = true;
        }
    }

    if (changed) {
        _notifyChange();
    }

    return changed;
}

bool PackListBase::setPriority(const std::string& path, i32 priority)
{
    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });

        if (it != m_packs.end() && it->priority != priority) {
            it->priority = priority;
            changed = true;
        }
    }

    if (changed) {
        _notifyChange();
    }

    return changed;
}

bool PackListBase::moveUp(const std::string& path)
{
    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });

        if (it == m_packs.end()) {
            return false;
        }

        i32 currentPriority = it->priority;
        i32 maxPriority = currentPriority;

        for (auto& pack : m_packs) {
            if (pack.priority > currentPriority) {
                maxPriority = std::max(maxPriority, pack.priority);
            }
        }

        if (maxPriority == currentPriority) {
            return false;
        }

        for (auto& pack : m_packs) {
            if (pack.priority == maxPriority) {
                pack.priority = currentPriority;
                break;
            }
        }
        it->priority = maxPriority;
        changed = true;
    }

    if (changed) {
        _notifyChange();
    }

    return changed;
}

bool PackListBase::moveDown(const std::string& path)
{
    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });

        if (it == m_packs.end()) {
            return false;
        }

        i32 currentPriority = it->priority;
        i32 minPriority = currentPriority;

        for (auto& pack : m_packs) {
            if (pack.priority < currentPriority) {
                minPriority = std::min(minPriority, pack.priority);
            }
        }

        if (minPriority == currentPriority) {
            return false;
        }

        for (auto& pack : m_packs) {
            if (pack.priority == minPriority) {
                pack.priority = currentPriority;
                break;
            }
        }
        it->priority = minPriority;
        changed = true;
    }

    if (changed) {
        _notifyChange();
    }

    return changed;
}

std::vector<ResourcePackPtr> PackListBase::getEnabledPacks() const
{
    const auto infos = getEnabledPackInfos();
    std::vector<ResourcePackPtr> result;
    result.reserve(infos.size());

    for (const auto& info : infos) {
        if (info.pack) {
            result.push_back(info.pack);
        }
    }

    return result;
}

std::vector<PackListBase::PackInfo> PackListBase::getAllPacks() const
{
    std::shared_lock lock(m_mutex);
    return m_packs;
}

std::vector<PackListBase::PackInfo> PackListBase::getEnabledPackInfos() const
{
    // 缓存命中：shared_lock 直接返回拷贝，省掉遍历 m_packs 过滤 + stable_sort。
    {
        std::shared_lock lock(m_mutex);
        if (m_enabledPackInfosCache) {
            return *m_enabledPackInfosCache;
        }
    }

    // 缓存 miss：unique_lock 构建+排序+写缓存。double-check 避免并发重复构建。
    // 模式对齐 ZipResourcePack::readResource（shared_lock 读 / unique_lock 写）。
    std::unique_lock lock(m_mutex);
    if (m_enabledPackInfosCache) {
        return *m_enabledPackInfosCache;
    }

    std::vector<PackInfo> result;
    for (const auto& info : m_packs) {
        if (info.enabled && info.initialized) {
            result.push_back(info);
        }
    }

    std::stable_sort(
        result.begin(), result.end(), [](const PackInfo& a, const PackInfo& b) { return a.priority > b.priority; });

    m_enabledPackInfosCache = result;
    return result;
}

std::optional<PackListBase::PackInfo> PackListBase::getPackInfo(const std::string& path) const
{
    std::shared_lock lock(m_mutex);
    auto it = std::find_if(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });

    if (it == m_packs.end()) {
        return std::nullopt;
    }

    return *it;
}

bool PackListBase::containsPack(const std::string& path) const
{
    std::shared_lock lock(m_mutex);
    return std::any_of(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });
}

size_t PackListBase::packCount() const
{
    std::shared_lock lock(m_mutex);
    return m_packs.size();
}

size_t PackListBase::enabledPackCount() const
{
    std::shared_lock lock(m_mutex);
    return std::count_if(
        m_packs.begin(), m_packs.end(), [](const PackInfo& info) { return info.enabled && info.initialized; });
}

bool PackListBase::hasResource(PackType type, std::string_view resourcePath) const
{
    for (const auto& pack : getEnabledPacks()) {
        if (pack->hasResource(type, resourcePath)) {
            return true;
        }
    }

    return false;
}

Result<std::vector<u8>> PackListBase::readResource(PackType type, std::string_view resourcePath) const
{
    for (const auto& pack : getEnabledPacks()) {
        if (!pack->hasResource(type, resourcePath)) {
            continue;
        }

        auto result = pack->readResource(type, resourcePath);
        if (result.success()) {
            return result;
        }
    }

    return Error(ErrorCode::ResourceNotFound, "Resource not found in any enabled pack: " + std::string(resourcePath));
}

Result<std::string> PackListBase::readTextResource(PackType type, std::string_view resourcePath) const
{
    auto dataResult = readResource(type, resourcePath);
    if (dataResult.failed()) {
        return dataResult.error();
    }

    const auto& data = dataResult.value();
    return std::string(data.begin(), data.end());
}

Result<std::vector<std::string>> PackListBase::listResources(
    PackType type, std::string_view directory, std::string_view extension) const
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.IO.Resource, "PackListBase::listResources", "directory", directory, "extension", extension);

    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const auto& pack : getEnabledPacks()) {
        auto listResult = pack->listResources(type, directory, extension);
        if (!listResult.success()) {
            continue;
        }

        for (const auto& path : listResult.value()) {
            if (seen.insert(path).second) {
                result.push_back(path);
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

Result<std::vector<std::string>> PackListBase::getResourceNamespaces(PackType type) const
{
    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const auto& pack : getEnabledPacks()) {
        auto nsResult = pack->getResourceNamespaces(type);
        if (!nsResult.success()) {
            continue;
        }

        for (const auto& ns : nsResult.value()) {
            if (seen.insert(ns).second) {
                result.push_back(ns);
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

Result<std::vector<PackListBase::ResourceVersion>> PackListBase::readAllResourceVersions(
    PackType type, std::string_view resourcePath) const
{
    std::vector<ResourceVersion> versions;
    const auto packs = getEnabledPackInfos();

    for (const auto& info : packs) {
        if (!info.pack || !info.pack->hasResource(type, resourcePath)) {
            continue;
        }

        auto result = info.pack->readTextResource(type, resourcePath);
        if (result.success()) {
            versions.push_back({info.pack->name(), std::move(result.value())});
        }
    }

    if (versions.empty()) {
        return Error(
            ErrorCode::ResourceNotFound, "Resource not found in any enabled pack: " + std::string(resourcePath));
    }

    return versions;
}

Result<std::map<std::string, std::vector<PackListBase::ResourceVersion>>> PackListBase::listResourceStacks(
    PackType type, std::string_view directory, std::string_view extension) const
{
    std::map<std::string, std::vector<ResourceVersion>> resourceStacks;
    const auto packs = getEnabledPackInfos();

    for (const auto& info : packs) {
        if (!info.pack) {
            continue;
        }

        auto listResult = info.pack->listResources(type, directory, extension);
        if (!listResult.success()) {
            continue;
        }

        for (const auto& path : listResult.value()) {
            auto textResult = info.pack->readTextResource(type, path);
            if (textResult.success()) {
                resourceStacks[path].push_back({info.pack->name(), std::move(textResult.value())});
            }
        }
    }

    return resourceStacks;
}

void PackListBase::onChange(std::function<void()> callback)
{
    std::unique_lock lock(m_mutex);
    MC_ASSERT_RELEASE(!m_callback);
    m_callback = std::move(callback);
}

void PackListBase::onPackListChanged() {}

void PackListBase::_notifyChange()
{
    // 失效已启用 pack 排序缓存。变更方法已先改 m_packs（unique_lock 下），故 reset 时
    // m_packs 已是最新，下次 getEnabledPackInfos 从最新 m_packs 重建。失效早于 callback
    // 触发的重载，保证重载内读到的缓存是新构建的。
    {
        std::unique_lock lock(m_mutex);
        m_enabledPackInfosCache.reset();
    }

    onPackListChanged();

    std::function<void()> callback;
    {
        std::shared_lock lock(m_mutex);
        callback = m_callback;
    }

    if (callback) {
        callback();
    }
}

std::string PackListBase::_normalizePathKey(const std::filesystem::path& path)
{
    std::filesystem::path normalizedPath = path;
    std::error_code ec;
    auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        normalizedPath = canonicalPath;
    } else {
        normalizedPath = path.lexically_normal();
    }

    return _toLowerAscii(_normalizePathSeparators(normalizedPath.string()));
}

bool PackListBase::_isZipFile(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    return _toLowerAscii(path.extension().string()) == ".zip";
}

bool PackListBase::_isPackDir(const std::filesystem::path& path)
{
    if (!std::filesystem::is_directory(path)) {
        return false;
    }

    std::filesystem::path mcmetaPath = path / "pack.mcmeta";
    return std::filesystem::exists(mcmetaPath);
}

} // namespace mc::resource
