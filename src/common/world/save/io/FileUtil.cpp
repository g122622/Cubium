#include "FileUtil.hpp"
#include <fstream>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace mc::world::save::io {

// ========== 文件读写 ==========

Result<std::vector<u8>>
FileUtil::readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return Error(ErrorCode::FileNotFound,
                     std::string("Failed to open file: ") + path.string());
    }

    // 获取文件大小
    auto size = file.tellg();
    if (size == -1) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to get file size: ") + path.string());
    }
    file.seekg(0, std::ios::beg);

    // 读取文件内容
    std::vector<u8> buffer(static_cast<size_t>(size));
    if (size > 0) {
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return Error(ErrorCode::FileReadFailed,
                         std::string("Failed to read file: ") + path.string());
        }
    }

    return buffer;
}

Result<void>
FileUtil::writeFile(const std::filesystem::path& path, const void* data, size_t size) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to create file: ") + path.string());
    }

    if (size > 0 && data != nullptr) {
        if (!file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size))) {
            return Error(ErrorCode::FileWriteFailed,
                         std::string("Failed to write file: ") + path.string());
        }
    }

    return {};
}

Result<void>
FileUtil::writeFile(const std::filesystem::path& path, const std::vector<u8>& data) {
    return writeFile(path, data.data(), data.size());
}

// ========== 原子操作 ==========

Result<void>
FileUtil::atomicWrite(const std::filesystem::path& path, const void* data, size_t size) {
    // 生成临时文件路径
    std::filesystem::path tempPath = path;
    tempPath += ".tmp";

    // 写入临时文件
    auto writeResult = writeFile(tempPath, data, size);
    if (writeResult.failed()) {
        return writeResult.error();
    }

    // 同步到磁盘
    auto syncResult = syncFile(tempPath);
    if (syncResult.failed()) {
        // 同步失败，删除临时文件
        std::filesystem::remove(tempPath);
        return syncResult.error();
    }

    // 原子重命名
    std::error_code ec;
    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        // 重命名失败，删除临时文件
        std::filesystem::remove(tempPath);
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to rename temporary file: ") + ec.message());
    }

    return {};
}

Result<void>
FileUtil::atomicWrite(const std::filesystem::path& path, const std::vector<u8>& data) {
    return atomicWrite(path, data.data(), data.size());
}

Result<void>
FileUtil::backupAndUpdate(const std::filesystem::path& current,
                          const std::filesystem::path& updated,
                          const std::filesystem::path& backup) {
    std::error_code ec;

    // 如果备份文件存在，先删除
    if (std::filesystem::exists(backup)) {
        std::filesystem::remove(backup, ec);
        if (ec) {
            return Error(ErrorCode::FileWriteFailed,
                         std::string("Failed to remove old backup: ") + ec.message());
        }
    }

    // 如果当前文件存在，重命名为备份
    if (std::filesystem::exists(current)) {
        std::filesystem::rename(current, backup, ec);
        if (ec) {
            return Error(ErrorCode::FileWriteFailed,
                         std::string("Failed to create backup: ") + ec.message());
        }
    }

    // 将新文件重命名为当前文件
    std::filesystem::rename(updated, current, ec);
    if (ec) {
        // 尝试恢复备份
        if (std::filesystem::exists(backup)) {
            std::filesystem::rename(backup, current, ec);
        }
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to update file: ") + ec.message());
    }

    return {};
}

// ========== 目录操作 ==========

Result<void>
FileUtil::ensureDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (!std::filesystem::create_directories(path, ec) && ec) {
            return Error(ErrorCode::FileWriteFailed,
                         std::string("Failed to create directory: ") + ec.message());
        }
    }
    return {};
}

Result<void>
FileUtil::removeDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        if (!std::filesystem::remove_all(path, ec) && ec) {
            return Error(ErrorCode::FileWriteFailed,
                         std::string("Failed to remove directory: ") + ec.message());
        }
    }
    return {};
}

Result<std::vector<std::string>>
FileUtil::listDirectory(const std::filesystem::path& path) {
    std::vector<std::string> entries;
    std::error_code ec;

    if (!std::filesystem::exists(path, ec)) {
        return Error(ErrorCode::FileNotFound,
                     std::string("Directory not found: ") + path.string());
    }

    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        entries.push_back(entry.path().filename().string());
    }

    if (ec) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to list directory: ") + ec.message());
    }

    return entries;
}

// ========== 文件检查 ==========

bool FileUtil::fileExists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool FileUtil::directoryExists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

Result<size_t>
FileUtil::getFileSize(const std::filesystem::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to get file size: ") + ec.message());
    }
    return static_cast<size_t>(size);
}

Result<i64>
FileUtil::getLastModifiedTime(const std::filesystem::path& path) {
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to get last modified time: ") + ec.message());
    }

    // 转换为 Unix 时间戳
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        sctp.time_since_epoch()
    ).count();

    return static_cast<i64>(timestamp);
}

// ========== 同步操作 ==========

Result<void>
FileUtil::syncFile(const std::filesystem::path& path) {
#ifdef _WIN32
    // Windows: 使用 FlushFileBuffers
    HANDLE handle = CreateFileW(
        path.wstring().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle == INVALID_HANDLE_VALUE) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to open file for sync: ") + path.string());
    }

    BOOL success = FlushFileBuffers(handle);
    CloseHandle(handle);

    if (!success) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to flush file buffers: ") + path.string());
    }
#else
    // Linux/macOS: 使用 fsync
    int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to open file for sync: ") + path.string());
    }

    if (fsync(fd) < 0) {
        close(fd);
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to sync file: ") + path.string());
    }

    close(fd);
#endif

    return {};
}

Result<void>
FileUtil::syncDirectory(const std::filesystem::path& path) {
#ifdef _WIN32
    // Windows: 目录同步通过打开目录并刷新
    HANDLE handle = CreateFileW(
        path.wstring().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (handle == INVALID_HANDLE_VALUE) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to open directory for sync: ") + path.string());
    }

    BOOL success = FlushFileBuffers(handle);
    CloseHandle(handle);

    if (!success) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to flush directory buffers: ") + path.string());
    }
#else
    // Linux/macOS: 使用 fsync on directory
    int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to open directory for sync: ") + path.string());
    }

    if (fsync(fd) < 0) {
        close(fd);
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to sync directory: ") + path.string());
    }

    close(fd);
#endif

    return {};
}

} // namespace mc::world::save::io
