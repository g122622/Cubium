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

#include "WorldSessionLock.hpp"
#include "common/core/Result.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace mc::world::storage {

// ============================================================================
// 常量定义
// ============================================================================

/// 锁文件名
inline constexpr std::string_view LOCK_FILE_NAME = "session.lock";

/// 锁文件标识（雪人字符，与原版一致）
inline constexpr std::string_view LOCK_FILE_IDENTIFIER = "☃";

WorldSessionLock::WorldSessionLock(std::filesystem::path worldDir, bool readonly)
    : m_readonly(readonly)
#ifdef _WIN32
    , m_fileHandle(INVALID_HANDLE_VALUE)
#else
    , m_fd(-1)
#endif
    , m_worldDir(std::move(worldDir))
    , m_lockPath()
    , m_valid(false)
{}

Result<WorldSessionLock> WorldSessionLock::acquire(const std::filesystem::path& worldDir)
{
    std::error_code ec;

    // 确保世界目录存在
    if (!std::filesystem::exists(worldDir, ec)) {
        std::filesystem::create_directories(worldDir, ec);
    }

    std::filesystem::path lockPath = worldDir / LOCK_FILE_NAME;

    WorldSessionLock lock(worldDir);
    lock.m_lockPath = lockPath;

#ifdef _WIN32
    // Windows: 使用 LockFileEx 实现跨进程锁
    HANDLE hFile = CreateFileW(lockPath.wstring().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // 不共享
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_SHARING_VIOLATION) {
            spdlog::warn("WorldSessionLock: {} is already locked by another process", worldDir.string());
            return Error(ErrorCode::WorldLocked, "World is locked by another process");
        }
        spdlog::error("WorldSessionLock: Failed to create lock file at {} (error: {})", lockPath.string(), err);
        return Error(ErrorCode::PermissionDenied, "Failed to create session lock");
    }

    // 尝试获取独占锁
    OVERLAPPED overlapped = {};
    overlapped.Offset = 0;
    overlapped.OffsetHigh = 0;
    overlapped.hEvent = nullptr;

    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &overlapped)) {
        DWORD err = GetLastError();
        CloseHandle(hFile);
        if (err == ERROR_LOCK_VIOLATION) {
            spdlog::warn("WorldSessionLock: {} is already locked by another process", worldDir.string());
            return Error(ErrorCode::WorldLocked, "World is locked by another process");
        }
        spdlog::error("WorldSessionLock: Failed to acquire lock (error: {})", err);
        return Error(ErrorCode::PermissionDenied, "Failed to acquire file lock");
    }

    // 写入标识
    DWORD bytesWritten = 0;
    WriteFile(
        hFile, LOCK_FILE_IDENTIFIER.data(), static_cast<DWORD>(LOCK_FILE_IDENTIFIER.size()), &bytesWritten, nullptr);

    lock.m_fileHandle = hFile;
    lock.m_valid = true;

#else
    // Unix: 使用 flock 实现跨进程锁
    int fd = open(lockPath.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        spdlog::error("WorldSessionLock: Failed to create lock file at {} (errno: {})", lockPath.string(), errno);
        return Error(ErrorCode::PermissionDenied, "Failed to create session lock");
    }

    // 尝试获取独占锁（非阻塞）
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        int err = errno;
        close(fd);
        if (err == EWOULDBLOCK || err == EAGAIN) {
            spdlog::warn("WorldSessionLock: {} is already locked by another process", worldDir.string());
            return Error(ErrorCode::WorldLocked, "World is locked by another process");
        }
        spdlog::error("WorldSessionLock: Failed to acquire lock (errno: {})", err);
        return Error(ErrorCode::PermissionDenied, "Failed to acquire file lock");
    }

    // 写入标识
    (void)write(fd, LOCK_FILE_IDENTIFIER.data(), static_cast<ssize_t>(LOCK_FILE_IDENTIFIER.size()));

    lock.m_fd = fd;
    lock.m_valid = true;
#endif

    spdlog::info("WorldSessionLock: Acquired lock for {}", worldDir.string());
    return lock;
}

Result<WorldSessionLock> WorldSessionLock::acquireReadOnly(const std::filesystem::path& worldDir)
{
    std::error_code ec;

    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::WorldNotFound, fmt::format("World directory does not exist: {}", worldDir.string()));
    }

    // 检查是否有其他进程持有独占锁
    if (isLocked(worldDir)) {
        spdlog::warn("WorldSessionLock: {} is locked by another process, cannot open readonly", worldDir.string());
        return Error(ErrorCode::WorldLocked, "World is locked by another process");
    }

    WorldSessionLock lock(worldDir, true);
    lock.m_valid = true;

    spdlog::info("WorldSessionLock: Opened {} in readonly mode (no exclusive lock)", worldDir.string());
    return lock;
}

bool WorldSessionLock::isLocked(const std::filesystem::path& worldDir)
{
    std::error_code ec;
    std::filesystem::path lockPath = worldDir / LOCK_FILE_NAME;

    if (!std::filesystem::exists(lockPath, ec)) {
        return false;
    }

#ifdef _WIN32
    // Windows: 尝试打开文件并获取锁
    HANDLE hFile = CreateFileW(lockPath.wstring().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return true; // 无法打开，可能被锁定
    }

    OVERLAPPED overlapped = {};
    overlapped.Offset = 0;
    overlapped.OffsetHigh = 0;
    overlapped.hEvent = nullptr;

    BOOL locked =
        LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &overlapped);
    if (locked) {
        UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
    }
    CloseHandle(hFile);

    return !locked; // 如果无法获取锁，则已锁定
#else
    // Unix: 尝试获取锁
    int fd = open(lockPath.c_str(), O_RDWR);
    if (fd < 0) {
        return true; // 无法打开，可能被锁定
    }

    bool locked = (flock(fd, LOCK_EX | LOCK_NB) < 0);
    if (!locked) {
        flock(fd, LOCK_UN); // 释放刚获取的锁
    }
    close(fd);

    return locked;
#endif
}

WorldSessionLock::WorldSessionLock(WorldSessionLock&& other) noexcept
    : m_readonly(other.m_readonly)
#ifdef _WIN32
    , m_fileHandle(other.m_fileHandle)
#else
    , m_fd(other.m_fd)
#endif
    , m_worldDir(std::move(other.m_worldDir))
    , m_lockPath(std::move(other.m_lockPath))
    , m_valid(other.m_valid)
{
#ifdef _WIN32
    other.m_fileHandle = INVALID_HANDLE_VALUE;
#else
    other.m_fd = -1;
#endif
    other.m_valid = false;
}

WorldSessionLock& WorldSessionLock::operator=(WorldSessionLock&& other) noexcept
{
    if (this != &other) {
        release();

        m_readonly = other.m_readonly;
#ifdef _WIN32
        m_fileHandle = other.m_fileHandle;
        other.m_fileHandle = INVALID_HANDLE_VALUE;
#else
        m_fd = other.m_fd;
        other.m_fd = -1;
#endif
        m_worldDir = std::move(other.m_worldDir);
        m_lockPath = std::move(other.m_lockPath);
        m_valid = other.m_valid;

        other.m_valid = false;
    }
    return *this;
}

WorldSessionLock::~WorldSessionLock()
{
    release();
}

bool WorldSessionLock::isValid() const noexcept
{
    return m_valid;
}

const std::filesystem::path& WorldSessionLock::worldDir() const noexcept
{
    return m_worldDir;
}

void WorldSessionLock::release()
{
    if (!m_valid) {
        return;
    }

    if (m_readonly) {
        spdlog::info("WorldSessionLock: Released readonly access for {}", m_worldDir.string());
        m_valid = false;
        return;
    }

#ifdef _WIN32
    if (m_fileHandle != INVALID_HANDLE_VALUE) {
        OVERLAPPED overlapped = {};
        overlapped.Offset = 0;
        overlapped.OffsetHigh = 0;
        overlapped.hEvent = nullptr;

        UnlockFileEx(m_fileHandle, 0, MAXDWORD, MAXDWORD, &overlapped);
        CloseHandle(m_fileHandle);
        m_fileHandle = INVALID_HANDLE_VALUE;
    }
#else
    if (m_fd >= 0) {
        flock(m_fd, LOCK_UN);
        close(m_fd);
        m_fd = -1;
    }
#endif

    // 删除锁文件
    std::error_code ec;
    if (std::filesystem::exists(m_lockPath, ec)) {
        std::filesystem::remove(m_lockPath, ec);
        if (ec) {
            spdlog::warn("WorldSessionLock: Failed to remove lock file: {}", ec.message());
        }
    }

    spdlog::info("WorldSessionLock: Released lock for {}", m_worldDir.string());
    m_valid = false;
}

} // namespace mc::world::storage
