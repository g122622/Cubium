#pragma once

#include "../../../core/Types.hpp"

namespace mc::world::save::region {

/**
 * @brief Region 文件压缩类型
 *
 * Region 文件中的区块数据可以使用不同的压缩方式。
 * 参考 MC 1.16.5 RegionFile.java
 *
 * | 值   | 类型         | 说明                    |
 * |------|--------------|------------------------|
 * | 1    | GZIP         | GZIP 压缩              |
 * | 2    | Zlib         | Zlib (Deflate) 压缩     |
 * | 3    | Uncompressed | 无压缩                 |
 * | 128+ | External     | 外部文件 (.mcc)        |
 */
enum class CompressionType : u8 {
    Gzip = 1,          ///< GZIP 压缩（Java 版较少使用）
    Zlib = 2,          ///< Zlib (Deflate) 压缩（最常用）
    Uncompressed = 3,  ///< 无压缩
    ExternalGzip = 129,    ///< 外部文件，GZIP 压缩
    ExternalZlib = 130,    ///< 外部文件，Zlib 压缩
    ExternalUncompressed = 131  ///< 外部文件，无压缩
};

/**
 * @brief 压缩类型工具函数
 */
struct CompressionTypeUtils {
    /**
     * @brief 检查是否为外部文件
     *
     * @param type 压缩类型
     * @return 如果是外部文件返回 true
     */
    [[nodiscard]] static bool isExternal(CompressionType type) {
        return static_cast<u8>(type) >= 128;
    }

    /**
     * @brief 获取基础压缩类型（去除外部文件标记）
     *
     * @param type 压缩类型
     * @return 基础压缩类型
     */
    [[nodiscard]] static CompressionType getBaseType(CompressionType type) {
        u8 value = static_cast<u8>(type);
        if (value >= 128) {
            return static_cast<CompressionType>(value - 128);
        }
        return type;
    }

    /**
     * @brief 创建外部文件类型
     *
     * @param type 基础压缩类型
     * @return 外部文件类型
     */
    [[nodiscard]] static CompressionType makeExternal(CompressionType type) {
        return static_cast<CompressionType>(static_cast<u8>(type) + 128);
    }

    /**
     * @brief 检查是否为有效的压缩类型
     *
     * @param type 压缩类型
     * @return 如果有效返回 true
     */
    [[nodiscard]] static bool isValid(CompressionType type) {
        u8 value = static_cast<u8>(type);
        return value == 1 || value == 2 || value == 3 ||
               value == 129 || value == 130 || value == 131;
    }

    /**
     * @brief 从字节值创建压缩类型
     *
     * @param value 字节值
     * @return 压缩类型，如果无效则返回 Zlib（默认）
     */
    [[nodiscard]] static CompressionType fromByte(u8 value) {
        if (isValid(static_cast<CompressionType>(value))) {
            return static_cast<CompressionType>(value);
        }
        return CompressionType::Zlib;  // 默认使用 Zlib
    }

    /**
     * @brief 转换为字节值
     *
     * @param type 压缩类型
     * @return 字节值
     */
    [[nodiscard]] static u8 toByte(CompressionType type) {
        return static_cast<u8>(type);
    }
};

} // namespace mc::world::save::region
