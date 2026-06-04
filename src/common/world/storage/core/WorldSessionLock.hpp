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

#pragma once

#include "common/core/Result.hpp"
#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 世界会话锁
 *
 * RAII 包装器，用于检测和获取世界目录的独占访问权。
 * 使用平台特定的文件锁实现跨进程互斥：
 * - Unix: flock(LOCK_EX | LOCK_NB)
 * - Windows: LockFileEx
 *
 * 参考原版 SessionLockManager 实现：
 * - 锁文件为 session.lock
 * - 创建时写入固定标识
 * - 析构时释放锁
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
     * @brief 以只读模式获取世界目录的锁
     *
     * 只读模式下不获取独占锁，仅检查存档是否被其他进程锁定。
     * 如果其他进程已持有独占锁，则返回错误。
     *
     * @param worldDir 世界目录路径
     * @return 成功返回锁对象，失败返回错误
     */
    static Result<WorldSessionLock> acquireReadOnly(const std::filesystem::path& worldDir);

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
    WorldSessionLock(std::filesystem::path worldDir, bool readonly = false);

    bool m_readonly;

    // 平台特定的文件句柄
#ifdef _WIN32
    void* m_fileHandle; // HANDLE
#else
    int m_fd; // file descriptor
#endif

    std::filesystem::path m_worldDir;
    std::filesystem::path m_lockPath;
    bool m_valid;
};

} // namespace mc::world::storage
