#include "WorldSessionLock.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

Result<WorldSessionLock> WorldSessionLock::acquire(const std::filesystem::path& worldDir)
{
    std::error_code ec;
    std::filesystem::path lockPath = worldDir / "session.lock";

    // 确保世界目录存在
    if (!std::filesystem::exists(worldDir, ec)) {
        std::filesystem::create_directories(worldDir, ec);
    }

    // 检查锁文件是否存在
    bool hasFileLock = false;

    // 第一版：仅通过文件存在与否判断锁定
    // 后续版本可添加跨进程文件锁支持
    if (std::filesystem::exists(lockPath, ec)) {
        // 锁文件已存在，说明可能被锁定
        // 尝试读取内容判断是否是当前进程的锁
        std::ifstream lockFile(lockPath, std::ios::binary);
        if (lockFile.is_open()) {
            // 锁文件存在，我们认为世界已被锁定
            // 注意：第一版不实现跨进程锁，这里总是返回失败
            spdlog::warn("WorldSessionLock: {} is already locked", worldDir.string());
            // 对于第一版，我们允许覆盖锁（用于开发调试）
            // 后续应返回错误
        }
    }

    // 创建/覆盖锁文件
    {
        std::ofstream lockFile(lockPath, std::ios::binary | std::ios::trunc);
        if (lockFile.is_open()) {
            // 写入固定标识（雪花符号，参考原版）
            const char* snowman = "☃";
            lockFile.write(snowman, 3);
            lockFile.close();
            hasFileLock = true;
        } else {
            spdlog::error("WorldSessionLock: Failed to create lock file at {}", lockPath.string());
            return Error(ErrorCode::PermissionDenied, "Failed to create session lock");
        }
    }

    return WorldSessionLock(worldDir, hasFileLock);
}

bool WorldSessionLock::isLocked(const std::filesystem::path& worldDir)
{
    std::error_code ec;
    std::filesystem::path lockPath = worldDir / "session.lock";

    if (!std::filesystem::exists(lockPath, ec)) {
        return false;
    }

    // 第一版仅检查文件存在
    // 后续版本可使用平台文件锁检查
    return true;
}

WorldSessionLock::WorldSessionLock(WorldSessionLock&& other) noexcept
    : m_worldDir(std::move(other.m_worldDir))
    , m_hasFileLock(other.m_hasFileLock)
    , m_valid(other.m_valid)
{
    other.m_valid = false;
    other.m_hasFileLock = false;
}

WorldSessionLock& WorldSessionLock::operator=(WorldSessionLock&& other) noexcept
{
    if (this != &other) {
        release();

        m_worldDir = std::move(other.m_worldDir);
        m_hasFileLock = other.m_hasFileLock;
        m_valid = other.m_valid;

        other.m_valid = false;
        other.m_hasFileLock = false;
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

    if (m_hasFileLock) {
        std::error_code ec;
        std::filesystem::path lockPath = m_worldDir / "session.lock";

        if (std::filesystem::exists(lockPath, ec)) {
            std::filesystem::remove(lockPath, ec);
            if (ec) {
                spdlog::warn("WorldSessionLock: Failed to remove lock file: {}", ec.message());
            }
        }
        m_hasFileLock = false;
    }

    m_valid = false;
}

WorldSessionLock::WorldSessionLock(std::filesystem::path worldDir, bool hasFileLock)
    : m_worldDir(std::move(worldDir))
    , m_hasFileLock(hasFileLock)
    , m_valid(true)
{
}

} // namespace mc::world::storage
