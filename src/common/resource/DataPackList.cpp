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

#include "DataPackList.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <set>
#include <shared_mutex>

namespace fs = std::filesystem;

namespace mc::resource {

// ============================================================================
// 数据包管理
// ============================================================================

Result<size_t> DataPackList::scanDirectory(const std::filesystem::path& dir)
{
    if (!fs::exists(dir)) {
        return static_cast<size_t>(0);
    }

    if (!fs::is_directory(dir)) {
        return Error(ErrorCode::InvalidArgument, "Path is not a directory: " + dir.string());
    }

    size_t addedCount = 0;
    i32 nextPriority = static_cast<i32>(packCount());

    for (const auto& entry : fs::directory_iterator(dir)) {
        const auto& path = entry.path();

        std::string normalizedPath = normalizePath(path);
        if (containsPack(normalizedPath)) {
            continue;
        }

        bool isZip = isZipFile(path);
        bool isPackDir = !isZip && isDataPackDir(path);

        if (!isZip && !isPackDir) {
            continue;
        }

        auto result = addPack(path, true, nextPriority++);
        if (result.success()) {
            ++addedCount;
            if (result.value().initialized) {
                spdlog::info("Found data pack: {} ({})", result.value().pack->name(), isZip ? "ZIP" : "folder");
            } else {
                spdlog::warn("Data pack failed to initialize: {} - {}", path.filename().string(), result.value().error);
            }
        } else {
            spdlog::warn("Failed to add data pack {}: {}", path.filename().string(), result.error().toString());
        }
    }

    notifyChange();
    return addedCount;
}

Result<DataPackList::PackInfo> DataPackList::addPack(const std::filesystem::path& path, bool enabled, i32 priority)
{
    if (!fs::exists(path)) {
        return Error(ErrorCode::FileNotFound, "Data pack not found: " + path.string());
    }

    std::string normalizedPath = normalizePath(path);

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
                notifyChange();
            }
            return existing;
        }
    }

    PackInfo info;
    info.path = normalizedPath;
    info.enabled = enabled;
    info.priority = priority;
    info.isZip = isZipFile(path);

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
            spdlog::warn("Failed to initialize data pack {}: {}", path.filename().string(), info.error);
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
        notifyChange();
    }

    return resultInfo;
}

bool DataPackList::removePack(const std::string& path)
{
    std::string normalizedPath = path;
    if (normalizedPath.find('\\') != std::string::npos) {
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    }

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
        notifyChange();
    }

    return removed;
}

void DataPackList::clear()
{
    {
        std::unique_lock lock(m_mutex);
        m_packs.clear();
    }
    notifyChange();
}

// ============================================================================
// 启用/禁用和优先级
// ============================================================================

bool DataPackList::setEnabled(const std::string& path, bool enabled)
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
        notifyChange();
    }

    return changed;
}

bool DataPackList::setPriority(const std::string& path, i32 priority)
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
        notifyChange();
    }

    return changed;
}

bool DataPackList::moveUp(const std::string& path)
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
        notifyChange();
    }

    return changed;
}

bool DataPackList::moveDown(const std::string& path)
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
        notifyChange();
    }

    return changed;
}

// ============================================================================
// 查询方法
// ============================================================================

std::vector<ResourcePackPtr> DataPackList::getEnabledPacks() const
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

std::vector<DataPackList::PackInfo> DataPackList::getAllPacks() const
{
    std::shared_lock lock(m_mutex);
    return m_packs;
}

std::vector<DataPackList::PackInfo> DataPackList::getEnabledPackInfos() const
{
    std::vector<PackInfo> result;
    {
        std::shared_lock lock(m_mutex);
        for (const auto& info : m_packs) {
            if (info.enabled && info.initialized) {
                result.push_back(info);
            }
        }
    }

    std::stable_sort(
        result.begin(), result.end(), [](const PackInfo& a, const PackInfo& b) { return a.priority > b.priority; });

    return result;
}

std::optional<DataPackList::PackInfo> DataPackList::getPackInfo(const std::string& path) const
{
    std::shared_lock lock(m_mutex);
    auto it = std::find_if(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });

    if (it == m_packs.end()) {
        return std::nullopt;
    }

    return *it;
}

bool DataPackList::containsPack(const std::string& path) const
{
    std::shared_lock lock(m_mutex);
    return std::any_of(m_packs.begin(), m_packs.end(), [&](const PackInfo& info) { return info.path == path; });
}

size_t DataPackList::packCount() const
{
    std::shared_lock lock(m_mutex);
    return m_packs.size();
}

size_t DataPackList::enabledPackCount() const
{
    std::shared_lock lock(m_mutex);
    return std::count_if(
        m_packs.begin(), m_packs.end(), [](const PackInfo& info) { return info.enabled && info.initialized; });
}

// ============================================================================
// 资源访问（限定 PackType::ServerData）
// ============================================================================

bool DataPackList::hasResource(std::string_view resourcePath) const
{
    for (const auto& pack : getEnabledPacks()) {
        if (pack->hasResource(PackType::ServerData, resourcePath)) {
            return true;
        }
    }

    return false;
}

Result<std::vector<u8>> DataPackList::readResource(std::string_view resourcePath) const
{
    for (const auto& pack : getEnabledPacks()) {
        if (!pack->hasResource(PackType::ServerData, resourcePath)) {
            continue;
        }

        auto result = pack->readResource(PackType::ServerData, resourcePath);
        if (result.success()) {
            return result;
        }
    }

    return Error(
        ErrorCode::ResourceNotFound, "Resource not found in any enabled data pack: " + std::string(resourcePath));
}

Result<std::string> DataPackList::readTextResource(std::string_view resourcePath) const
{
    auto dataResult = readResource(resourcePath);
    if (dataResult.failed()) {
        return dataResult.error();
    }

    const auto& data = dataResult.value();
    return std::string(data.begin(), data.end());
}

Result<std::vector<std::string>> DataPackList::listResources(
    std::string_view directory, std::string_view extension) const
{
    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const auto& pack : getEnabledPacks()) {
        auto listResult = pack->listResources(PackType::ServerData, directory, extension);
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

Result<std::vector<std::string>> DataPackList::getResourceNamespaces() const
{
    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const auto& pack : getEnabledPacks()) {
        auto nsResult = pack->getResourceNamespaces(PackType::ServerData);
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

// ============================================================================
// 变更通知
// ============================================================================

void DataPackList::onChange(std::function<void()> callback)
{
    std::unique_lock lock(m_mutex);
    MC_ASSERT_RELEASE(!m_callback);
    m_callback = std::move(callback);
}

void DataPackList::notifyChange()
{
    std::function<void()> callback;
    {
        std::shared_lock lock(m_mutex);
        callback = m_callback;
    }

    if (callback) {
        callback();
    }
}

// ============================================================================
// 私有方法
// ============================================================================

std::string DataPackList::normalizePath(const std::filesystem::path& path) noexcept
{
    std::string result = path.string();
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool DataPackList::isZipFile(const std::filesystem::path& path) noexcept
{
    if (!fs::is_regular_file(path)) {
        return false;
    }

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".zip";
}

bool DataPackList::isDataPackDir(const std::filesystem::path& path) noexcept
{
    if (!fs::is_directory(path)) {
        return false;
    }

    // 数据包必须包含 pack.mcmeta
    fs::path mcmetaPath = path / "pack.mcmeta";
    return fs::exists(mcmetaPath);
}

} // namespace mc::resource
