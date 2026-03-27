#pragma once

#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include <filesystem>
#include <memory>

namespace mc::world::save {

/**
 * @brief 会话锁 - 防止多进程同时访问存档
 *
 * 使用文件锁实现，在进程退出时自动释放。
 * 参考 MC 1.16.5 SessionLockManager.java
 *
 * ## 工作原理
 *
 * 当进程打开世界存档时，会在世界目录下创建 `session.lock` 文件，
 * 并获取该文件的独占锁。如果另一个进程已经持有锁，则打开失败。
 *
 * ## 使用示例
 * ```cpp
 * // 尝试获取会话锁
 * auto lockResult = SessionLock::create("saves/MyWorld");
 * if (lockResult.success()) {
 *     auto lock = std::move(lockResult.value());
 *     // 锁已获取，可以安全访问存档
 *     // ...
 *     // 锁在析构时自动释放
 * } else {
 *     // 存档已被其他进程占用
 *     std::cerr << lockResult.error().message() << std::endl;
 * }
 * ```
 *
 * ## 跨平台支持
 *
 * - Windows: 使用 `LockFileEx` / `UnlockFileEx`
 * - Linux/macOS: 使用 `flock` 或 `fcntl`
 *
 * @note 会话锁是不可拷贝的，但可以移动
 */
class SessionLock {
public:
    /**
     * @brief 尝试创建会话锁
     *
     * 在指定目录下创建 `session.lock` 文件并获取独占锁。
     *
     * @param worldDir 世界目录路径
     * @return 成功返回锁对象，失败（已被锁定）返回错误
     */
    [[nodiscard]] static Result<std::unique_ptr<SessionLock>>
    create(const std::filesystem::path& worldDir);

    ~SessionLock();

    // 禁止拷贝
    SessionLock(const SessionLock&) = delete;
    SessionLock& operator=(const SessionLock&) = delete;

    // 允许移动
    SessionLock(SessionLock&& other) noexcept;
    SessionLock& operator=(SessionLock&& other) noexcept;

    /**
     * @brief 检查锁是否有效
     *
     * @return 如果锁仍然持有返回 true
     */
    [[nodiscard]] bool isValid() const;

    /**
     * @brief 获取锁文件路径
     */
    [[nodiscard]] std::filesystem::path lockFilePath() const;

    /**
     * @brief 手动释放锁
     *
     * 析构时会自动释放，此方法用于提前释放。
     */
    void release();

private:
    /**
     * @brief 私有构造函数
     *
     * @param lockPath 锁文件路径
     * @param handle 平台相关的文件句柄
     */
    explicit SessionLock(std::filesystem::path lockPath, void* handle);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace mc::world::save
