#include "CompressionUtil.hpp"
#include <zlib.h>
#include <cstring>

namespace mc::world::save::io {

// ========== GZIP 压缩/解压 ==========

Result<std::vector<u8>>
CompressionUtil::gzipCompress(const void* data, size_t size, i32 level) {
    if (data == nullptr && size > 0) {
        return Error(ErrorCode::InvalidArgument, "Data pointer is null");
    }

    if (size == 0) {
        return std::vector<u8>();
    }

    // 验证压缩级别
    if (level < 0 || level > 9) {
        level = 6;  // 使用默认级别
    }

    // 估算输出缓冲区大小
    size_t bound = compressBound(size) + 32;  // 额外空间用于 GZIP 头尾
    std::vector<u8> output(bound);

    z_stream stream = {};
    stream.next_in = static_cast<const Bytef*>(data);
    stream.avail_in = static_cast<uInt>(size);
    stream.next_out = output.data();
    stream.avail_out = static_cast<uInt>(bound);

    // 初始化 GZIP 压缩
    // windowBits = 15 | 16 表示 GZIP 格式（带头部和尾部）
    i32 ret = deflateInit2(&stream, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        return Error(ErrorCode::CompressionFailed,
                     std::string("Failed to initialize GZIP compression: ") +
                     (stream.msg ? stream.msg : "unknown error"));
    }

    // 执行压缩
    ret = deflate(&stream, Z_FINISH);
    deflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        return Error(ErrorCode::CompressionFailed,
                     std::string("GZIP compression failed: ") +
                     (stream.msg ? stream.msg : "unknown error"));
    }

    // 调整输出大小
    output.resize(stream.total_out);
    return output;
}

Result<std::vector<u8>>
CompressionUtil::gzipCompress(const std::vector<u8>& data, i32 level) {
    return gzipCompress(data.data(), data.size(), level);
}

Result<std::vector<u8>>
CompressionUtil::gzipDecompress(const void* data, size_t size, size_t maxOutputSize) {
    if (data == nullptr && size > 0) {
        return Error(ErrorCode::InvalidArgument, "Data pointer is null");
    }

    if (size == 0) {
        return std::vector<u8>();
    }

    // 分配输出缓冲区，初始大小为输入的 4 倍
    size_t bufferSize = std::min(size * 4, maxOutputSize);
    if (bufferSize < 4096) bufferSize = 4096;
    std::vector<u8> output(bufferSize);

    z_stream stream = {};
    stream.next_in = static_cast<const Bytef*>(data);
    stream.avail_in = static_cast<uInt>(size);
    stream.next_out = output.data();
    stream.avail_out = static_cast<uInt>(bufferSize);

    // 初始化 GZIP 解压
    // windowBits = 15 | 16 表示自动检测 GZIP 格式
    i32 ret = inflateInit2(&stream, 15 | 16);
    if (ret != Z_OK) {
        return Error(ErrorCode::DecompressionFailed,
                     std::string("Failed to initialize GZIP decompression: ") +
                     (stream.msg ? stream.msg : "unknown error"));
    }

    // 执行解压
    while (true) {
        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_STREAM_END) {
            break;  // 解压完成
        }

        if (ret != Z_OK) {
            inflateEnd(&stream);
            return Error(ErrorCode::DecompressionFailed,
                         std::string("GZIP decompression failed: ") +
                         (stream.msg ? stream.msg : "unknown error"));
        }

        // 检查输出缓冲区是否已满
        if (stream.avail_out == 0) {
            size_t newSize = output.size() * 2;
            if (newSize > maxOutputSize) {
                newSize = maxOutputSize;
                if (output.size() >= maxOutputSize) {
                    inflateEnd(&stream);
                    return Error(ErrorCode::DecompressionFailed,
                                 "Decompressed data exceeds maximum size limit");
                }
            }
            output.resize(newSize);
            stream.next_out = output.data() + stream.total_out;
            stream.avail_out = static_cast<uInt>(newSize - stream.total_out);
        }
    }

    inflateEnd(&stream);

    // 调整输出大小
    output.resize(stream.total_out);
    return output;
}

Result<std::vector<u8>>
CompressionUtil::gzipDecompress(const std::vector<u8>& data, size_t maxOutputSize) {
    return gzipDecompress(data.data(), data.size(), maxOutputSize);
}

// ========== Zlib 压缩/解压 ==========

Result<std::vector<u8>>
CompressionUtil::zlibCompress(const void* data, size_t size, i32 level) {
    if (data == nullptr && size > 0) {
        return Error(ErrorCode::InvalidArgument, "Data pointer is null");
    }

    if (size == 0) {
        return std::vector<u8>();
    }

    // 验证压缩级别
    if (level < 0 || level > 9) {
        level = 6;  // 使用默认级别
    }

    // 估算输出缓冲区大小
    size_t bound = compressBound(size);
    std::vector<u8> output(bound);

    uLongf destLen = static_cast<uLongf>(bound);
    i32 ret = compress2(output.data(), &destLen,
                        static_cast<const Bytef*>(data), static_cast<uLong>(size),
                        level);

    if (ret != Z_OK) {
        const char* errMsg = "unknown error";
        switch (ret) {
            case Z_MEM_ERROR: errMsg = "memory error"; break;
            case Z_BUF_ERROR: errMsg = "buffer error"; break;
            case Z_STREAM_ERROR: errMsg = "stream error"; break;
        }
        return Error(ErrorCode::CompressionFailed,
                     std::string("Zlib compression failed: ") + errMsg);
    }

    // 调整输出大小
    output.resize(destLen);
    return output;
}

Result<std::vector<u8>>
CompressionUtil::zlibCompress(const std::vector<u8>& data, i32 level) {
    return zlibCompress(data.data(), data.size(), level);
}

Result<std::vector<u8>>
CompressionUtil::zlibDecompress(const void* data, size_t size, size_t maxOutputSize) {
    if (data == nullptr && size > 0) {
        return Error(ErrorCode::InvalidArgument, "Data pointer is null");
    }

    if (size == 0) {
        return std::vector<u8>();
    }

    // 分配输出缓冲区，初始大小为输入的 4 倍
    size_t bufferSize = std::min(size * 4, maxOutputSize);
    if (bufferSize < 4096) bufferSize = 4096;
    std::vector<u8> output(bufferSize);

    z_stream stream = {};
    stream.next_in = static_cast<const Bytef*>(data);
    stream.avail_in = static_cast<uInt>(size);
    stream.next_out = output.data();
    stream.avail_out = static_cast<uInt>(bufferSize);

    // 初始化 Zlib 解压
    i32 ret = inflateInit(&stream);
    if (ret != Z_OK) {
        return Error(ErrorCode::DecompressionFailed,
                     std::string("Failed to initialize Zlib decompression: ") +
                     (stream.msg ? stream.msg : "unknown error"));
    }

    // 执行解压
    while (true) {
        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_STREAM_END) {
            break;  // 解压完成
        }

        if (ret != Z_OK) {
            inflateEnd(&stream);
            return Error(ErrorCode::DecompressionFailed,
                         std::string("Zlib decompression failed: ") +
                         (stream.msg ? stream.msg : "unknown error"));
        }

        // 检查输出缓冲区是否已满
        if (stream.avail_out == 0) {
            size_t newSize = output.size() * 2;
            if (newSize > maxOutputSize) {
                newSize = maxOutputSize;
                if (output.size() >= maxOutputSize) {
                    inflateEnd(&stream);
                    return Error(ErrorCode::DecompressionFailed,
                                 "Decompressed data exceeds maximum size limit");
                }
            }
            output.resize(newSize);
            stream.next_out = output.data() + stream.total_out;
            stream.avail_out = static_cast<uInt>(newSize - stream.total_out);
        }
    }

    inflateEnd(&stream);

    // 调整输出大小
    output.resize(stream.total_out);
    return output;
}

Result<std::vector<u8>>
CompressionUtil::zlibDecompress(const std::vector<u8>& data, size_t maxOutputSize) {
    return zlibDecompress(data.data(), data.size(), maxOutputSize);
}

// ========== 通用压缩接口 ==========

Result<std::vector<u8>>
CompressionUtil::compress(CompressionType type, const void* data, size_t size, i32 level) {
    switch (type) {
        case CompressionType::Gzip:
            return gzipCompress(data, size, level);

        case CompressionType::Zlib:
            return zlibCompress(data, size, level);

        case CompressionType::Uncompressed:
            // 无压缩，直接复制数据
            if (data == nullptr && size > 0) {
                return Error(ErrorCode::InvalidArgument, "Data pointer is null");
            }
            return std::vector<u8>(static_cast<const u8*>(data),
                                   static_cast<const u8*>(data) + size);

        default:
            return Error(ErrorCode::InvalidArgument, "Unsupported compression type");
    }
}

Result<std::vector<u8>>
CompressionUtil::decompress(CompressionType type, const void* data, size_t size,
                            size_t maxOutputSize) {
    switch (type) {
        case CompressionType::Gzip:
            return gzipDecompress(data, size, maxOutputSize);

        case CompressionType::Zlib:
            return zlibDecompress(data, size, maxOutputSize);

        case CompressionType::Uncompressed:
            // 无压缩，直接复制数据
            if (data == nullptr && size > 0) {
                return Error(ErrorCode::InvalidArgument, "Data pointer is null");
            }
            return std::vector<u8>(static_cast<const u8*>(data),
                                   static_cast<const u8*>(data) + size);

        default:
            return Error(ErrorCode::InvalidArgument, "Unsupported compression type");
    }
}

// ========== 工具方法 ==========

bool CompressionUtil::isGzip(const void* data, size_t size) {
    if (data == nullptr || size < 2) {
        return false;
    }

    const u8* bytes = static_cast<const u8*>(data);
    // GZIP 魔术字节: 0x1F 0x8B
    return bytes[0] == 0x1F && bytes[1] == 0x8B;
}

size_t CompressionUtil::estimateCompressedSize(size_t size) {
    // zlib 文档建议的缓冲区大小
    return size + (size >> 12) + (size >> 14) + (size >> 25) + 13;
}

} // namespace mc::world::save::io
