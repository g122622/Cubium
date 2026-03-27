#pragma once

#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include <vector>
#include <cstddef>

namespace mc::world::save::io {

/**
 * @brief 压缩类型枚举
 *
 * 用于 Region 文件中标识区块数据的压缩方式。
 * 参考 MC 1.16.5 RegionFile.java
 */
enum class CompressionType : u8 {
    Gzip = 1,          ///< GZIP 压缩
    Zlib = 2,          ///< Zlib (Deflate) 压缩，最常用
    Uncompressed = 3,  ///< 无压缩
    External = 128     ///< 外部文件标记（大于1MB的区块）
};

/**
 * @brief 压缩工具类
 *
 * 提供静态方法用于 gzip 和 zlib 压缩/解压缩。
 * 内部使用 zlib 库实现。
 *
 * @note 所有方法都是线程安全的
 *
 * ## 使用示例
 * ```cpp
 * // GZIP 压缩
 * auto compressed = CompressionUtil::gzipCompress(data, size);
 * if (compressed.success()) {
 *     // 使用 compressed.value()
 * }
 *
 * // GZIP 解压
 * auto decompressed = CompressionUtil::gzipDecompress(compressed.data(), compressed.size());
 * ```
 */
class CompressionUtil {
public:
    // ========== GZIP 压缩/解压 ==========

    /**
     * @brief GZIP 压缩数据
     *
     * 使用 GZIP 格式压缩数据，生成的数据包含 GZIP 头部和尾部。
     *
     * @param data 原始数据指针
     * @param size 原始数据大小
     * @param level 压缩级别 (0-9)，默认 6
     * @return 成功返回压缩后的数据，失败返回错误
     *
     * @note 压缩级别越高，压缩率越大但速度越慢
     */
    [[nodiscard]] static Result<std::vector<u8>>
    gzipCompress(const void* data, size_t size, i32 level = 6);

    /**
     * @brief GZIP 压缩数据（vector 版本）
     *
     * @param data 原始数据
     * @param level 压缩级别
     * @return 成功返回压缩后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    gzipCompress(const std::vector<u8>& data, i32 level = 6);

    /**
     * @brief GZIP 解压数据
     *
     * @param data 压缩数据指针
     * @param size 压缩数据大小
     * @param maxOutputSize 最大输出大小限制（防止解压炸弹）
     * @return 成功返回解压后的数据，失败返回错误
     *
     * @note 默认最大输出大小为 16MB，足够容纳任何区块数据
     */
    [[nodiscard]] static Result<std::vector<u8>>
    gzipDecompress(const void* data, size_t size, size_t maxOutputSize = 16 * 1024 * 1024);

    /**
     * @brief GZIP 解压数据（vector 版本）
     *
     * @param data 压缩数据
     * @param maxOutputSize 最大输出大小限制
     * @return 成功返回解压后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    gzipDecompress(const std::vector<u8>& data, size_t maxOutputSize = 16 * 1024 * 1024);

    // ========== Zlib 压缩/解压 ==========

    /**
     * @brief Zlib 压缩数据
     *
     * 使用 Zlib (Deflate) 格式压缩数据，不含 GZIP 头部。
     * 这是 Minecraft Region 文件中最常用的压缩格式。
     *
     * @param data 原始数据指针
     * @param size 原始数据大小
     * @param level 压缩级别 (0-9)，默认 6
     * @return 成功返回压缩后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    zlibCompress(const void* data, size_t size, i32 level = 6);

    /**
     * @brief Zlib 压缩数据（vector 版本）
     *
     * @param data 原始数据
     * @param level 压缩级别
     * @return 成功返回压缩后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    zlibCompress(const std::vector<u8>& data, i32 level = 6);

    /**
     * @brief Zlib 解压数据
     *
     * @param data 压缩数据指针
     * @param size 压缩数据大小
     * @param maxOutputSize 最大输出大小限制
     * @return 成功返回解压后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    zlibDecompress(const void* data, size_t size, size_t maxOutputSize = 16 * 1024 * 1024);

    /**
     * @brief Zlib 解压数据（vector 版本）
     *
     * @param data 压缩数据
     * @param maxOutputSize 最大输出大小限制
     * @return 成功返回解压后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    zlibDecompress(const std::vector<u8>& data, size_t maxOutputSize = 16 * 1024 * 1024);

    // ========== 通用压缩接口 ==========

    /**
     * @brief 根据类型压缩数据
     *
     * @param type 压缩类型
     * @param data 原始数据
     * @param size 原始数据大小
     * @param level 压缩级别
     * @return 成功返回压缩后的数据，失败返回错误
     *
     * @note Uncompressed 类型直接返回原始数据的副本
     */
    [[nodiscard]] static Result<std::vector<u8>>
    compress(CompressionType type, const void* data, size_t size, i32 level = 6);

    /**
     * @brief 根据类型解压数据
     *
     * @param type 压缩类型
     * @param data 压缩数据
     * @param size 压缩数据大小
     * @param maxOutputSize 最大输出大小限制
     * @return 成功返回解压后的数据，失败返回错误
     */
    [[nodiscard]] static Result<std::vector<u8>>
    decompress(CompressionType type, const void* data, size_t size,
               size_t maxOutputSize = 16 * 1024 * 1024);

    // ========== 工具方法 ==========

    /**
     * @brief 检查数据是否为 GZIP 格式
     *
     * GZIP 文件以魔术字节 0x1F 0x8B 开头
     *
     * @param data 数据指针
     * @param size 数据大小
     * @return 如果是 GZIP 格式返回 true
     */
    [[nodiscard]] static bool isGzip(const void* data, size_t size);

    /**
     * @brief 估算压缩后大小
     *
     * @param size 原始数据大小
     * @return 估算的压缩后最大大小
     */
    [[nodiscard]] static size_t estimateCompressedSize(size_t size);

private:
    CompressionUtil() = delete;  // 禁止实例化
};

} // namespace mc::world::save::io
