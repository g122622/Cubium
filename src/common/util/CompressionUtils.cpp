#include "CompressionUtils.hpp"
#include <zlib.h>

namespace mc::util {

std::vector<u8> decompressGzip(const std::vector<u8>& compressed)
{
    std::vector<u8> decompressed;
    constexpr size_t bufferSize = 8192;
    std::vector<u8> buffer(bufferSize);

    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(compressed.data());
    stream.avail_in = static_cast<uInt>(compressed.size());

    // windowBits = 15 | 16 表示 gzip 格式
    if (inflateInit2(&stream, 15 | 16) != Z_OK) {
        return decompressed;
    }

    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<uInt>(buffer.size());
        result = inflate(&stream, Z_NO_FLUSH);
        if (result == Z_OK || result == Z_STREAM_END) {
            size_t have = buffer.size() - stream.avail_out;
            decompressed.insert(decompressed.end(), buffer.begin(), buffer.begin() + have);
        }
    }

    inflateEnd(&stream);
    return decompressed;
}

std::vector<u8> compressGzip(const std::vector<u8>& data)
{
    std::vector<u8> compressed;
    constexpr size_t bufferSize = 8192;
    std::vector<u8> buffer(bufferSize);

    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(data.data());
    stream.avail_in = static_cast<uInt>(data.size());

    // windowBits = 15 | 16 表示 gzip 格式
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return compressed;
    }

    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<uInt>(buffer.size());
        result = deflate(&stream, Z_FINISH);
        if (result == Z_OK || result == Z_STREAM_END) {
            size_t have = buffer.size() - stream.avail_out;
            compressed.insert(compressed.end(), buffer.begin(), buffer.begin() + have);
        }
    }

    deflateEnd(&stream);
    return compressed;
}

} // namespace mc::util
