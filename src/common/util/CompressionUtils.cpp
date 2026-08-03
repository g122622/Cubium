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

#include "CompressionUtils.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <vector>
#include <zconf.h>
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

std::vector<u8> decompressZlib(const std::vector<u8>& compressed)
{
    std::vector<u8> decompressed;
    constexpr size_t bufferSize = 8192;
    std::vector<u8> buffer(bufferSize);

    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(compressed.data());
    stream.avail_in = static_cast<uInt>(compressed.size());

    // windowBits = 15 表示 zlib 格式（raw deflate + zlib header）
    if (inflateInit2(&stream, 15) != Z_OK) {
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

std::vector<u8> compressZlib(const std::vector<u8>& data)
{
    std::vector<u8> compressed;
    constexpr size_t bufferSize = 8192;
    std::vector<u8> buffer(bufferSize);

    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(data.data());
    stream.avail_in = static_cast<uInt>(data.size());

    // windowBits = 15 表示 zlib 格式
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
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
