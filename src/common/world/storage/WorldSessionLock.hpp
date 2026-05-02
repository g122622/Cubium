#pragma once

#include "../../core/Result.hpp"
#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 世界会话锁
 *
 * RAII 包装器，用于检测和获取世界目录的独占访问权。
 * 参考原版 SessionLockManager 实现：
 * - 锁文件为 session.lock
 * - 创建时写入固定标识
 * - 平台支持时使用文件锁
 * - 析构时释放锁
 *
 * 注意：第一版只实现同进程可检测的 session.lock 文件语义，
 * 跨进程文件锁可分阶段增强。
 */
class WorldSessionLock {
public:
    /**
     * @brief 尝试获取世界目录的独占锁
     *
     * @param worldDir 世界目录路径
     * @return 成功返回锁对象，失败返回错误
     */
    static Result<WorldSessionLock> acquire(const std::filesystem::path& worldDir);

    /**
     * @brief 检查世界目录是否被锁定
     *
     * 此检查仅用于 UI 显示，不能替代实际获取锁。
     * 实际操作前必须重新 acquire。
     *
     * @param worldDir 世界目录路径
     * @return 锁定返回 true，未锁定或错误返回 false
     */
    static bool isLocked(const std::filesystem::path& worldDir);

    // 禁止拷贝
    WorldSessionLock(const WorldSessionLock&) = delete;
    WorldSessionLock& operator=(const WorldSessionLock&) = delete;

    // 允许移动
    WorldSessionLock(WorldSessionLock&& other) noexcept;
    WorldSessionLock& operator=(WorldSessionLock&& other) noexcept;

    /**
     * @brief 析构时释放锁
     */
    ~WorldSessionLock();

    /**
     * @brief 检查锁是否仍然有效
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief 获取世界目录路径
     */
    [[nodiscard]] const std::filesystem::path& worldDir() const noexcept;

    /**
     * @brief 手动释放锁
     */
    void release();

private:
    WorldSessionLock(std::filesystem::path worldDir, bool hasFileLock);

    std::filesystem::path m_worldDir;
    bool m_hasFileLock;
    bool m_valid;
};

} // namespace mc::world::storage
