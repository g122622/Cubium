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

#include "common/network/crypto/ZlibCodec.hpp"

#include <zlib.h>

namespace mc::network::crypto {

namespace {

/**
 * @brief 内联 VarInt 写入（避免依赖 buffer::ByteBuf 造成反向依赖）
 *
 * 与 ByteBuf::writeVarUInt 同算法：每字节低 7 位 + 续位 0x80。
 */
void writeVarInt(std::vector<u8>& out, u32 value)
{
    while (true) {
        if ((value & ~static_cast<u32>(0x7F)) == 0) {
            out.push_back(static_cast<u8>(value));
            return;
        }
        out.push_back(static_cast<u8>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
}

/**
 * @brief 内联 VarInt 读取（最多 5 字节）
 *
 * @return {value, consumed}；失败返回 nullopt。
 */
struct VarIntRead {
    u32 value;
    usize consumed;
};
bool readVarInt(const u8* data, usize size, VarIntRead& out)
{
    u32 value = 0;
    for (usize i = 0; i < 5; ++i) {
        if (i >= size) {
            return false;
        }
        const u8 byte = data[i];
        value |= static_cast<u32>(byte & 0x7Fu) << (7 * i);
        if ((byte & 0x80u) == 0) {
            out.value = value;
            out.consumed = i + 1;
            return true;
        }
    }
    return false;
}

} // namespace

Result<std::vector<u8>> ZlibCodec::deflateBytes(const u8* data, usize size)
{
    z_stream stream = {};
    // windowBits=15 标准 zlib 格式（raw deflate + zlib header），对齐 Java Deflater/Inflater 默认。
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return Error(ErrorCode::Unknown, "deflateInit2 failed", "ZlibCodec::deflateBytes");
    }

    stream.next_in = const_cast<Bytef*>(data);
    stream.avail_in = static_cast<uInt>(size);

    std::vector<u8> out;
    constexpr usize bufSize = 8192;
    std::vector<u8> buf(bufSize);
    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = buf.data();
        stream.avail_out = static_cast<uInt>(buf.size());
        result = deflate(&stream, Z_FINISH);
        const usize have = buf.size() - stream.avail_out;
        out.insert(out.end(), buf.data(), buf.data() + have);
    }
    deflateEnd(&stream);
    if (result != Z_STREAM_END) {
        return Error(ErrorCode::Unknown, "zlib deflate did not finish", "ZlibCodec::deflateBytes");
    }
    return out;
}

Result<std::vector<u8>> ZlibCodec::inflateBytes(const u8* data, usize size, u32 maxOut)
{
    z_stream stream = {};
    if (inflateInit2(&stream, 15) != Z_OK) {
        return Error(ErrorCode::Unknown, "inflateInit2 failed", "ZlibCodec::inflateBytes");
    }

    stream.next_in = const_cast<Bytef*>(data);
    stream.avail_in = static_cast<uInt>(size);

    std::vector<u8> out;
    constexpr usize bufSize = 8192;
    std::vector<u8> buf(bufSize);
    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = buf.data();
        stream.avail_out = static_cast<uInt>(buf.size());
        result = inflate(&stream, Z_NO_FLUSH);
        if (result == Z_OK || result == Z_STREAM_END) {
            const usize have = buf.size() - stream.avail_out;
            out.insert(out.end(), buf.data(), buf.data() + have);
            if (out.size() > maxOut) {
                inflateEnd(&stream);
                return Error(ErrorCode::InvalidData, "Decompressed data exceeds limit", "ZlibCodec::inflateBytes");
            }
        }
    }
    inflateEnd(&stream);
    if (result != Z_STREAM_END) {
        return Error(ErrorCode::InvalidData, "zlib inflate failed", "ZlibCodec::inflateBytes");
    }
    return out;
}

Result<void> ZlibCodec::encode(i32 threshold, const std::vector<u8>& input, std::vector<u8>& output)
{
    if (input.size() > kMaxUncompressed) {
        return Error(ErrorCode::InvalidData, "Uncompressed data exceeds 8MB limit", "ZlibCodec::encode");
    }

    // 阈值 < 0 或数据长度 < threshold：不压缩，写 0 + 原文。
    if (threshold < 0 || static_cast<u32>(input.size()) < static_cast<u32>(threshold)) {
        writeVarInt(output, 0);
        output.insert(output.end(), input.begin(), input.end());
        return Result<void>::ok();
    }

    // 压缩：写解压后长度 + zlib 流。
    writeVarInt(output, static_cast<u32>(input.size()));
    auto deflated = deflateBytes(input.data(), input.size());
    if (!deflated.success()) {
        return deflated.error();
    }
    if (deflated.value().size() > kMaxCompressed) {
        return Error(ErrorCode::InvalidData, "Compressed data exceeds 2MB limit", "ZlibCodec::encode");
    }
    output.insert(output.end(), deflated.value().begin(), deflated.value().end());
    return Result<void>::ok();
}

Result<void> ZlibCodec::decode(
    const u8* input, usize inputSize, i32 threshold, i32& dataLengthOut, std::vector<u8>& output, usize& consumedOut)
{
    VarIntRead vlen{};
    if (!readVarInt(input, inputSize, vlen)) {
        return Error(ErrorCode::InvalidData, "Failed to read compressed data length VarInt", "ZlibCodec::decode");
    }
    const u32 dataLength = vlen.value;
    dataLengthOut = static_cast<i32>(dataLength);
    const u8* payload = input + vlen.consumed;
    const usize payloadSize = (inputSize > vlen.consumed) ? inputSize - vlen.consumed : 0;

    if (dataLength == 0) {
        // 未压缩：payload 即原 packetID+payload。
        output.insert(output.end(), payload, payload + payloadSize);
        consumedOut = vlen.consumed + payloadSize;
        return Result<void>::ok();
    }

    // 压缩：校验声明长度，再 inflate。
    if (dataLength > kMaxUncompressed) {
        return Error(ErrorCode::InvalidData, "Decompressed data exceeds 8MB limit", "ZlibCodec::decode");
    }
    // Java CompressionDecoder 校验：压缩包声明长度不应小于阈值（否则视为非法）。
    if (threshold >= 0 && dataLength < static_cast<u32>(threshold)) {
        return Error(ErrorCode::InvalidData, "Compressed packet declared length below threshold", "ZlibCodec::decode");
    }

    auto inflated = inflateBytes(payload, payloadSize, kMaxUncompressed);
    if (!inflated.success()) {
        return inflated.error();
    }
    if (inflated.value().size() != dataLength) {
        return Error(ErrorCode::InvalidData, "Decompressed length does not match declared length", "ZlibCodec::decode");
    }
    output = std::move(inflated).value();
    consumedOut = vlen.consumed + payloadSize;
    return Result<void>::ok();
}

} // namespace mc::network::crypto
