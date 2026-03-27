#include "SessionLock.hpp"
#include "FileUtil.hpp"
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#endif

namespace mc::world::save {

/**
 * @brief 平台相关的实现细节
 */
struct SessionLock::Impl {
    std::filesystem::path lockPath;  ///< 锁文件路径
    void* handle = nullptr;           ///< 平台相关的文件句柄
    bool valid = false;               ///< 锁是否有效

#ifdef _WIN32
    // Windows: 使用 HANDLE
    ~Impl() {
        if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
            // 释放锁并关闭句柄
            HANDLE h = static_cast<HANDLE>(handle);
            UnlockFile(h, 0, 0, MAXDWORD, MAXDWORD);
            CloseHandle(h);
        }
    }
#else
    // Linux/macOS: 使用 int 文件描述符
    ~Impl() {
        if (handle != nullptr) {
            int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle));
            // 释放锁并关闭文件
            flock(fd, LOCK_UN);
            close(fd);
        }
    }
#endif
};

SessionLock::SessionLock(std::filesystem::path lockPath, void* handle)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->lockPath = std::move(lockPath);
    m_impl->handle = handle;
    m_impl->valid = true;
}

SessionLock::~SessionLock() {
    release();
}

SessionLock::SessionLock(SessionLock&& other) noexcept
    : m_impl(std::move(other.m_impl)) {
    other.m_impl = std::make_unique<Impl>();
}

SessionLock& SessionLock::operator=(SessionLock&& other) noexcept {
    if (this != &other) {
        release();
        m_impl = std::move(other.m_impl);
        other.m_impl = std::make_unique<Impl>();
    }
    return *this;
}

Result<std::unique_ptr<SessionLock>>
SessionLock::create(const std::filesystem::path& worldDir) {
    // 确保目录存在
    auto dirResult = io::FileUtil::ensureDirectory(worldDir);
    if (dirResult.failed()) {
        return dirResult.error();
    }

    // 锁文件路径
    std::filesystem::path lockPath = worldDir / "session.lock";

#ifdef _WIN32
    // Windows: 使用 LockFileEx
    HANDLE handle = CreateFileW(
        lockPath.wstring().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,  // 允许其他进程读取（但不能写入）
        nullptr,
        OPEN_ALWAYS,  // 如果不存在则创建
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION) {
            return Error(ErrorCode::PermissionDenied,
                         "World is already in use by another process");
        }
        return Error(ErrorCode::FileOpenFailed,
                     std::string("Failed to create session lock: error code ") +
                     std::to_string(error));
    }

    // 尝试获取独占锁
    OVERLAPPED overlapped = {};
    if (!LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                    0, MAXDWORD, MAXDWORD, &overlapped)) {
        DWORD error = GetLastError();
        CloseHandle(handle);

        if (error == ERROR_LOCK_VIOLATION) {
            return Error(ErrorCode::PermissionDenied,
                         "World is already in use by another process");
        }
        return Error(ErrorCode::PermissionDenied,
                     std::string("Failed to acquire session lock: error code ") +
                     std::to_string(error));
    }

    // 写入锁定信息（可选，用于调试）
    std::string lockInfo = "Locked by Minecraft Reborn\n";
    DWORD written;
    WriteFile(handle, lockInfo.c_str(), static_cast<DWORD>(lockInfo.size()), &written, nullptr);
    FlushFileBuffers(handle);

    return std::unique_ptr<SessionLock>(new SessionLock(lockPath, handle));

#else
    // Linux/macOS: 使用 flock
    int fd = open(lockPath.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            return Error(ErrorCode::PermissionDenied,
                         "World is already in use by another process");
        }
        return Error(ErrorCode::FileOpenFailed,
                     std::string("Failed to create session lock: ") + strerror(errno));
    }

    // 尝试获取独占锁（非阻塞）
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        int err = errno;
        close(fd);

        if (err == EWOULDBLOCK || err == EAGAIN) {
            return Error(ErrorCode::PermissionDenied,
                         "World is already in use by another process");
        }
        return Error(ErrorCode::PermissionDenied,
                     std::string("Failed to acquire session lock: ") + strerror(err));
    }

    // 写入锁定信息（可选，用于调试）
    std::string lockInfo = "Locked by Minecraft Reborn\n";
    write(fd, lockInfo.c_str(), lockInfo.size());
    fsync(fd);

    // 将 int 文件描述符存储为 void*
    void* handle = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    return std::unique_ptr<SessionLock>(new SessionLock(lockPath, handle));
#endif
}

bool SessionLock::isValid() const {
    return m_impl && m_impl->valid;
}

std::filesystem::path SessionLock::lockFilePath() const {
    return m_impl ? m_impl->lockPath : std::filesystem::path();
}

void SessionLock::release() {
    if (!m_impl || !m_impl->valid) {
        return;
    }

#ifdef _WIN32
    if (m_impl->handle != nullptr && m_impl->handle != INVALID_HANDLE_VALUE) {
        HANDLE h = static_cast<HANDLE>(m_impl->handle);
        UnlockFile(h, 0, 0, MAXDWORD, MAXDWORD);
        CloseHandle(h);
        m_impl->handle = nullptr;
    }
#else
    if (m_impl->handle != nullptr) {
        int fd = static_cast<int>(reinterpret_cast<intptr_t>(m_impl->handle));
        flock(fd, LOCK_UN);
        close(fd);
        m_impl->handle = nullptr;
    }
#endif

    m_impl->valid = false;
}

} // namespace mc::world::save
