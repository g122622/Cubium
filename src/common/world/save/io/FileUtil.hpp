#pragma once

#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include <filesystem>
#include <vector>
#include <string>

namespace mc::world::save::io {

/**
 * @brief 文件工具类
 *
 * 提供静态方法用于文件操作，包括原子写入、目录操作等。
 * 所有方法都是线程安全的。
 *
 * ## 使用示例
 * ```cpp
 * // 原子写入
 * auto result = FileUtil::atomicWrite("level.dat", data, size);
 * if (result.failed()) {
 *     // 处理错误
 * }
 *
 * // 确保目录存在
 * FileUtil::ensureDirectory("saves/MyWorld/region");
 * ```
 */
class FileUtil {
public:
    // ========== 文件读写 ==========

    /**
     * @brief 读取文件内容
     *
     * @param path 文件路径
     * @return 成功返回文件内容，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    readFile(const std::filesystem::path& path);

    /**
     * @brief 写入文件内容
     *
     * @param path 文件路径
     * @param data 数据指针
     * @param size 数据大小
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    writeFile(const std::filesystem::path& path, const void* data, size_t size);

    /**
     * @brief 写入文件内容（vector 版本）
     *
     * @param path 文件路径
     * @param data 数据
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    writeFile(const std::filesystem::path& path, const std::vector<u8>& data);

    // ========== 原子操作 ==========

    /**
     * @brief 原子写入文件
     *
     * 写入流程：
     * 1. 写入临时文件（path + ".tmp"）
     * 2. 同步到磁盘
     * 3. 原子重命名为目标文件
     *
     * 这确保在崩溃时不会损坏原有文件。
     *
     * @param path 目标文件路径
     * @param data 数据指针
     * @param size 数据大小
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    atomicWrite(const std::filesystem::path& path, const void* data, size_t size);

    /**
     * @brief 原子写入文件（vector 版本）
     *
     * @param path 目标文件路径
     * @param data 数据
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    atomicWrite(const std::filesystem::path& path, const std::vector<u8>& data);

    /**
     * @brief 备份并更新文件
     *
     * 流程：
     * 1. 如果目标文件存在，重命名为备份文件
     * 2. 将临时文件重命名为目标文件
     *
     * 参考 MC 的 Util.backupThenUpdate()
     *
     * @param current 当前文件
     * @param updated 新文件（临时文件）
     * @param backup 备份文件
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    backupAndUpdate(const std::filesystem::path& current,
                    const std::filesystem::path& updated,
                    const std::filesystem::path& backup);

    // ========== 目录操作 ==========

    /**
     * @brief 确保目录存在
     *
     * 如果目录不存在则创建，包括所有父目录。
     *
     * @param path 目录路径
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    ensureDirectory(const std::filesystem::path& path);

    /**
     * @brief 删除目录及其内容
     *
     * @param path 目录路径
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    removeDirectory(const std::filesystem::path& path);

    /**
     * @brief 列出目录内容
     *
     * @param path 目录路径
     * @return 成功返回文件名列表，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<std::string>>
    listDirectory(const std::filesystem::path& path);

    // ========== 文件检查 ==========

    /**
     * @brief 检查文件是否存在
     *
     * @param path 文件路径
     * @return 如果文件存在返回 true
     */
    [[nodiscard]] static bool fileExists(const std::filesystem::path& path);

    /**
     * @brief 检查目录是否存在
     *
     * @param path 目录路径
     * @return 如果目录存在返回 true
     */
    [[nodiscard]] static bool directoryExists(const std::filesystem::path& path);

    /**
     * @brief 获取文件大小
     *
     * @param path 文件路径
     * @return 成功返回文件大小（字节），失败返回错误
     */
    [[nodiscard]] static Result<size_t>
    getFileSize(const std::filesystem::path& path);

    /**
     * @brief 获取文件最后修改时间
     *
     * @param path 文件路径
     * @return 成功返回 Unix 时间戳，失败返回错误
     */
    [[nodiscard]] static Result<i64>
    getLastModifiedTime(const std::filesystem::path& path);

    // ========== 同步操作 ==========

    /**
     * @brief 同步文件到磁盘
     *
     * 确保文件数据写入物理存储设备。
     *
     * @param path 文件路径
     * @return 成功返回 void，失败返回错误
     *
     * @note 在 Windows 上使用 FlushFileBuffers，在 Linux/macOS 上使用 fsync
     */
    [[nodiscard]] static Result<void>
    syncFile(const std::filesystem::path& path);

    /**
     * @brief 同步目录到磁盘
     *
     * 确保目录的元数据（如新创建的文件）写入物理存储设备。
     *
     * @param path 目录路径
     * @return 成功返回 void，失败返回错误
     */
    [[nodiscard]] static Result<void>
    syncDirectory(const std::filesystem::path& path);

private:
    FileUtil() = delete;  // 禁止实例化
};

} // namespace mc::world::save::io
