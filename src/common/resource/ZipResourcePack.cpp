#include "ZipResourcePack.hpp"

#include <spdlog/spdlog.h>
#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <cstring>

namespace mc {

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

ZipResourcePack::ZipResourcePack(std::filesystem::path zipPath)
    : m_zipPath(std::move(zipPath))
    , m_name(m_zipPath.stem().string())
{
}

ZipResourcePack::~ZipResourcePack() = default;

// ============================================================================
// 静态工厂方法
// ============================================================================

Result<std::unique_ptr<ZipResourcePack>> ZipResourcePack::create(const std::filesystem::path& zipPath)
{
    if (!std::filesystem::exists(zipPath)) {
        return Error(ErrorCode::FileNotFound,
                     "ZIP file not found: " + zipPath.string());
    }

    if (!std::filesystem::is_regular_file(zipPath)) {
        return Error(ErrorCode::FileNotFound,
                     "Path is not a regular file: " + zipPath.string());
    }

    auto pack = std::unique_ptr<ZipResourcePack>(new ZipResourcePack(zipPath));
    return pack;
}

// ============================================================================
// IResourcePack 接口实现
// ============================================================================

Result<void> ZipResourcePack::initialize()
{
    // 使用 libarchive 打开 ZIP 文件
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

#ifdef _WIN32
    int r = archive_read_open_filename_w(a, m_zipPath.wstring().c_str(), 10240);
#else
    int r = archive_read_open_filename(a, m_zipPath.string().c_str(), 10240);
#endif

    if (r != ARCHIVE_OK) {
        spdlog::error("Failed to open ZIP file: {}", archive_error_string(a));
        archive_read_free(a);
        return Error(ErrorCode::FileOpenFailed,
                     "Failed to open ZIP file: " + m_zipPath.string() +
                     " - " + String(archive_error_string(a)));
    }

    // 读取所有条目，构建索引
    m_entries.clear();
    struct archive_entry* entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (pathname && *pathname) {
            String normalizedPath = normalizePath(pathname);
            // 跳过目录条目
            if (!normalizedPath.empty() && normalizedPath.back() != '/') {
                m_entries.insert(std::move(normalizedPath));
            }
        }
        // 跳过文件数据
        archive_read_data_skip(a);
    }

    archive_read_free(a);

    // 读取 pack.mcmeta
    const String mcmetaPath = "pack.mcmeta";
    if (hasResource(mcmetaPath)) {
        auto dataResult = readResource(mcmetaPath);
        if (dataResult.success()) {
            const auto& data = dataResult.value();
            String jsonStr(data.begin(), data.end());
            auto metadataResult = PackMetadata::parse(jsonStr);
            if (metadataResult.success()) {
                m_metadata = std::move(metadataResult.value());
            } else {
                spdlog::error("Failed to parse pack.mcmeta for {}: {}",
                             m_name, metadataResult.error().toString());
                m_metadata = PackMetadata();
            }
        }
    } else {
        m_metadata = PackMetadata();
    }

    spdlog::info("ZIP resource pack '{}' loaded: {} entries, format {}",
                 m_name, m_entries.size(), m_metadata.packFormat());
    return Result<void>::ok();
}

bool ZipResourcePack::hasResource(StringView resourcePath) const
{
    String normalized = normalizePath(resourcePath);
    return m_entries.find(normalized) != m_entries.end();
}

Result<std::vector<u8>> ZipResourcePack::readResource(StringView resourcePath) const
{
    String normalized = normalizePath(resourcePath);

    // 检查缓存
    auto cacheIt = m_cache.find(normalized);
    if (cacheIt != m_cache.end()) {
        return cacheIt->second;
    }

    // 检查条目是否存在
    if (m_entries.find(normalized) == m_entries.end()) {
        return Error(ErrorCode::ResourceNotFound,
                     "Resource not found in ZIP: " + normalized);
    }

    // 使用 libarchive 读取文件
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

#ifdef _WIN32
    int r = archive_read_open_filename_w(a, m_zipPath.wstring().c_str(), 10240);
#else
    int r = archive_read_open_filename(a, m_zipPath.string().c_str(), 10240);
#endif

    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return Error(ErrorCode::FileOpenFailed,
                     "Failed to open ZIP file: " + m_zipPath.string());
    }

    // 查找目标条目
    struct archive_entry* entry;
    std::vector<u8> data;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (pathname && normalizePath(pathname) == normalized) {
            // 找到目标条目，读取数据
            la_int64_t size = archive_entry_size(entry);
            if (size > 0) {
                data.resize(static_cast<size_t>(size));
                la_ssize_t read = archive_read_data(a, data.data(), data.size());
                if (read < 0) {
                    spdlog::error("Failed to read ZIP entry {}: {}", normalized, archive_error_string(a));
                    archive_read_free(a);
                    return Error(ErrorCode::FileReadFailed,
                                 "Failed to read ZIP entry: " + normalized);
                }
                data.resize(static_cast<size_t>(read));
            }
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_free(a);

    if (data.empty() && m_entries.find(normalized) == m_entries.end()) {
        return Error(ErrorCode::ResourceNotFound,
                     "Resource not found in ZIP: " + normalized);
    }

    // 缓存结果
    m_cache[normalized] = data;
    return data;
}

Result<std::vector<String>> ZipResourcePack::listResources(
    StringView directory,
    StringView extension) const
{
    std::vector<String> resources;
    String normalizedDir = normalizePath(directory);

    // 确保目录以斜杠结尾
    if (!normalizedDir.empty() && normalizedDir.back() != '/') {
        normalizedDir += '/';
    }

    for (const auto& path : m_entries) {
        // 检查是否在指定目录下
        if (path.size() > normalizedDir.size() &&
            path.substr(0, normalizedDir.size()) == normalizedDir) {

            // 检查扩展名
            if (extension.empty()) {
                resources.push_back(path);
            } else {
                if (path.size() >= extension.size() &&
                    path.substr(path.size() - extension.size()) == extension) {
                    resources.push_back(path);
                }
            }
        }
    }

    // 排序
    std::sort(resources.begin(), resources.end());

    return resources;
}

// ============================================================================
// 额外方法
// ============================================================================

void ZipResourcePack::clearCache()
{
    m_cache.clear();
}

// ============================================================================
// 私有方法
// ============================================================================

String ZipResourcePack::normalizePath(StringView path)
{
    String result(path);

    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');

    // 移除前导斜杠
    while (!result.empty() && result.front() == '/') {
        result.erase(0, 1);
    }

    return result;
}

} // namespace mc
