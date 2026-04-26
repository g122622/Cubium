#include "ResourcePackList.hpp"

#include <spdlog/spdlog.h>
#include "common/perfetto/TraceEvents.hpp"

#include <algorithm>
#include <set>
#include <shared_mutex>

namespace mc {

// ============================================================================
// 线程安全说明
//
// ResourcePackList 会在客户端主线程与音频线程之间共享，属于“读多写少”的场景。
// - 读操作（查询/遍历/读取资源）使用 std::shared_lock
// - 写操作（增删/启用/优先级/回调注册）使用 std::unique_lock
// - 对外返回 PackInfo 一律按值拷贝，避免暴露 vector 元素地址导致悬垂指针
// ============================================================================

// ============================================================================
// 资源包管理
// ============================================================================

Result<size_t> ResourcePackList::scanDirectory(const std::filesystem::path& dir)
{
    MC_TRACE_EVENT("client.initialization", "ResourcePackList::scanDirectory", "dir", dir.string());

    // 注意：音频线程会并发读取 ResourcePackList，因此这里的“已存在检查”
    // 不能再返回内部元素指针给外部长期持有。我们只做布尔查询。
    if (!std::filesystem::exists(dir)) {
        spdlog::debug("Resource pack directory does not exist: {}", dir.string());
        return static_cast<size_t>(0);
    }

    if (!std::filesystem::is_directory(dir)) {
        return Error(ErrorCode::InvalidArgument, "Path is not a directory: " + dir.string());
    }

    size_t addedCount = 0;
    i32 nextPriority = static_cast<i32>(packCount());

    // 遍历目录
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const auto& path = entry.path();

        // 跳过已存在的资源包
        String normalizedPath = normalizePath(path);
        if (containsPack(normalizedPath)) {
            continue;
        }

        // 检查是否是 ZIP 文件或资源包目录
        bool isZip = isZipFile(path);
        bool isPackDir = !isZip && isResourcePackDir(path);

        if (!isZip && !isPackDir) {
            continue;
        }

        // 添加资源包
        auto result = addPack(path, true, nextPriority++);
        if (result.success()) {
            ++addedCount;
            if (result.value().initialized) {
                spdlog::info("Found resource pack: {} ({})",
                             result.value().pack->name(),
                             isZip ? "ZIP" : "folder");
            } else {
                spdlog::warn("Resource pack failed to initialize: {} - {}",
                             path.filename().string(),
                             result.value().error);
            }
        } else {
            spdlog::warn("Failed to add resource pack {}: {}",
                         path.filename().string(),
                         result.error().toString());
        }
    }

    notifyChange();
    return addedCount;
}

Result<ResourcePackList::PackInfo> ResourcePackList::addPack(
    const std::filesystem::path& path,
    bool enabled,
    i32 priority)
{
    MC_TRACE_EVENT("client.initialization", "ResourcePackList::addPack", "path", path.string(), "enabled", enabled, "priority", priority);

    if (!std::filesystem::exists(path)) {
        return Error(ErrorCode::FileNotFound, "Resource pack not found: " + path.string());
    }

    String normalizedPath = normalizePath(path);

    // 先在锁内做一次“已存在”检查：如果只是更新开关/优先级，应该快速返回，
    // 不要做昂贵的 ZIP 打开与初始化。
    {
        bool foundExisting = false;
        bool changed = false;
        PackInfo existing;
        {
            std::unique_lock lock(m_mutex);
            auto existingIt = std::find_if(m_packs.begin(), m_packs.end(),
                [&](const PackInfo& pack) { return pack.path == normalizedPath; });

            if (existingIt != m_packs.end()) {
                foundExisting = true;
                changed = (existingIt->enabled != enabled) || (existingIt->priority != priority);
                existingIt->enabled = enabled;
                existingIt->priority = priority;
                existing = *existingIt; // 返回拷贝，避免对外暴露内部地址
            }
        }

        if (foundExisting) {
            if (changed) {
                notifyChange();
            }
            return existing;
        }
    }

    // 注意：资源包初始化可能涉及 IO/解压/解析，为避免阻塞并发读，初始化放到锁外。
    PackInfo info;
    info.path = normalizedPath;
    info.enabled = enabled;
    info.priority = priority;
    info.isZip = isZipFile(path);

    // 创建资源包实例
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

    // 初始化资源包
    if (info.pack) {
        auto initResult = info.pack->initialize();
        if (initResult.failed()) {
            info.initialized = false;
            info.error = initResult.error().toString();
            spdlog::warn("Failed to initialize resource pack {}: {}",
                         path.filename().string(),
                         info.error);
        } else {
            info.initialized = true;
        }
    }

    // 二次检查并插入：避免并发 addPack 时重复插入同一路径。
    bool shouldNotify = false;
    PackInfo resultInfo;
    {
        std::unique_lock lock(m_mutex);

        auto existingIt = std::find_if(m_packs.begin(), m_packs.end(),
            [&](const PackInfo& pack) { return pack.path == normalizedPath; });

        if (existingIt != m_packs.end()) {
            // 有人抢先插入了，则我们仅更新 enabled/priority 保持语义一致。
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

bool ResourcePackList::removePack(const String& path)
{
    String normalizedPath = path;
    // 如果路径不是规范的，尝试规范化
    if (normalizedPath.find('\\') != String::npos) {
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    }

    bool removed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(),
            [&](const PackInfo& info) { return info.path == normalizedPath; });

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

void ResourcePackList::clear()
{
    // 清空列表后统一发出变更通知，避免重复触发重载
    {
        std::unique_lock lock(m_mutex);
        m_packs.clear();
    }
    notifyChange();
}

// ============================================================================
// 启用/禁用和优先级
// ============================================================================

bool ResourcePackList::setEnabled(const String& path, bool enabled)
{
    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(),
            [&](const PackInfo& info) { return info.path == path; });

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

bool ResourcePackList::setPriority(const String& path, i32 priority)
{
    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(),
            [&](const PackInfo& info) { return info.path == path; });

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

bool ResourcePackList::moveUp(const String& path)
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::moveUp", "path", path);

    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(),
            [&](const PackInfo& info) { return info.path == path; });

        if (it == m_packs.end()) {
            return false;
        }

        // 找到比当前优先级高的下一个资源包
        i32 currentPriority = it->priority;
        i32 maxPriority = currentPriority;

        for (auto& pack : m_packs) {
            if (pack.priority > currentPriority) {
                maxPriority = std::max(maxPriority, pack.priority);
            }
        }

        if (maxPriority == currentPriority) {
            // 已经是最高优先级
            return false;
        }

        // 交换优先级
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

bool ResourcePackList::moveDown(const String& path)
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::moveDown", "path", path);

    bool changed = false;
    {
        std::unique_lock lock(m_mutex);
        auto it = std::find_if(m_packs.begin(), m_packs.end(),
            [&](const PackInfo& info) { return info.path == path; });

        if (it == m_packs.end()) {
            return false;
        }

        // 找到比当前优先级低的下一个资源包
        i32 currentPriority = it->priority;
        i32 minPriority = currentPriority;

        for (auto& pack : m_packs) {
            if (pack.priority < currentPriority) {
                minPriority = std::min(minPriority, pack.priority);
            }
        }

        if (minPriority == currentPriority) {
            // 已经是最低优先级
            return false;
        }

        // 交换优先级
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

std::vector<ResourcePackPtr> ResourcePackList::getEnabledPacks() const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::getEnabledPacks");

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

std::vector<ResourcePackList::PackInfo> ResourcePackList::getAllPacks() const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::getAllPacks");

    std::shared_lock lock(m_mutex);
    return m_packs;
}

std::vector<ResourcePackList::PackInfo> ResourcePackList::getEnabledPackInfos() const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::getEnabledPackInfos");

    std::vector<PackInfo> result;
    {
        // 收集启用的资源包（锁内复制，锁外排序）
        std::shared_lock lock(m_mutex);
        for (const auto& info : m_packs) {
            if (info.enabled && info.initialized) {
                result.push_back(info);
            }
        }
    }

    // 按优先级降序排序
    std::stable_sort(result.begin(), result.end(),
        [](const PackInfo& a, const PackInfo& b) {
            return a.priority > b.priority;
        });

    return result;
}

std::optional<ResourcePackList::PackInfo> ResourcePackList::getPackInfo(const String& path) const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::getPackInfo", "path", path);

    std::shared_lock lock(m_mutex);
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    if (it == m_packs.end()) {
        return std::nullopt;
    }

    return *it;
}

bool ResourcePackList::containsPack(const String& path) const
{
    std::shared_lock lock(m_mutex);
    return std::any_of(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });
}

size_t ResourcePackList::packCount() const
{
    std::shared_lock lock(m_mutex);
    return m_packs.size();
}

size_t ResourcePackList::enabledPackCount() const
{
    std::shared_lock lock(m_mutex);
    return std::count_if(m_packs.begin(), m_packs.end(),
        [](const PackInfo& info) { return info.enabled && info.initialized; });
}

// ============================================================================
// 资源访问
// ============================================================================

bool ResourcePackList::hasResource(StringView resourcePath) const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::hasResource", "resourcePath", resourcePath);

    for (const auto& pack : getEnabledPacks()) {
        if (pack->hasResource(resourcePath)) {
            return true;
        }
    }

    return false;
}

Result<std::vector<u8>> ResourcePackList::readResource(StringView resourcePath) const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::readResource", "resourcePath", resourcePath);

    for (const auto& pack : getEnabledPacks()) {
        if (!pack->hasResource(resourcePath)) {
            continue;
        }

        auto result = pack->readResource(resourcePath);
        if (result.success()) {
            return result;
        }

        // 如果读取失败，继续尝试下一个资源包
        spdlog::debug("Failed to read resource {} from pack {}: {}",
                      resourcePath,
                      pack->name(),
                      result.error().toString());
    }

    return Error(ErrorCode::ResourceNotFound,
                 "Resource not found in any enabled pack: " + String(resourcePath));
}

Result<String> ResourcePackList::readTextResource(StringView resourcePath) const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::readTextResource", "resourcePath", resourcePath);

    auto dataResult = readResource(resourcePath);
    if (dataResult.failed()) {
        return dataResult.error();
    }

    const auto& data = dataResult.value();
    return String(data.begin(), data.end());
}

Result<std::vector<String>> ResourcePackList::listResources(StringView directory, StringView extension) const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::listResources", "directory", directory, "extension", extension);

    std::vector<String> result;
    std::set<String> seen;

    for (const auto& pack : getEnabledPacks()) {
        auto listResult = pack->listResources(directory, extension);
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

// ============================================================================
// 设置同步
// ============================================================================

void ResourcePackList::loadFromSettings(const ResourcePackListOption& settings)
{
    MC_TRACE_EVENT("client.initialization", "ResourcePackList::loadFromSettings");
    
    for (const auto& entry : settings.getSortedEnabledEntries()) {
        auto result = addPack(std::filesystem::path(entry.path), entry.enabled, entry.priority);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack from settings {}: {}",
                         entry.path,
                         result.error().toString());
        }
    }
}

void ResourcePackList::saveToSettings(ResourcePackListOption& settings) const
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::saveToSettings");

    std::vector<ResourcePackEntry> entries;
    {
        std::shared_lock lock(m_mutex);
        entries.reserve(m_packs.size());
        for (const auto& info : m_packs) {
            entries.emplace_back(info.path, info.enabled, info.priority);
        }
    }

    settings.setEntries(std::move(entries));
}

// ============================================================================
// 变更通知
// ============================================================================

void ResourcePackList::onChange(std::function<void()> callback)
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::onChange");

    std::unique_lock lock(m_mutex);
    MC_ASSERT_RELEASE(!m_callback);
    m_callback = std::move(callback);
}

void ResourcePackList::notifyChange()
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::notifyChange");

    // 拷贝回调到局部变量，避免回调内部再次访问 ResourcePackList 时造成锁重入
    std::function<void()> callback;
    {
        std::shared_lock lock(m_mutex);
        callback = m_callback;
    }

    if (callback) {
        MC_TRACE_EVENT("client.resource", "ResourcePackList::notifyChange::callback");
        callback();
    }
}

// ============================================================================
// 私有方法
// ============================================================================

String ResourcePackList::normalizePath(const std::filesystem::path& path)
{
    MC_TRACE_EVENT("client.resource", "ResourcePackList::normalizePath");

    String result = path.string();
    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool ResourcePackList::isZipFile(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    String ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".zip";
}

bool ResourcePackList::isResourcePackDir(const std::filesystem::path& path)
{
    if (!std::filesystem::is_directory(path)) {
        return false;
    }

    // 检查是否包含 pack.mcmeta
    std::filesystem::path mcmetaPath = path / "pack.mcmeta";
    return std::filesystem::exists(mcmetaPath);
}

} // namespace mc
