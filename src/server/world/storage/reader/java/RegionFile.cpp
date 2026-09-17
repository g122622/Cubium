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

#include "RegionFile.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/CompressionUtils.hpp"
#include <filesystem>
#include <ios>
#include <iosfwd>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage::reader::java {

RegionFile::RegionFile(const std::filesystem::path& path)
    : m_path(path)
{
    // 从文件名解析区域坐标：r.X.Z.mca
    std::string filename = path.filename().string();
    if (filename.starts_with("r.") && filename.ends_with(".mca")) {
        // 去掉 "r." 前缀和 ".mca" 后缀
        std::string coords = filename.substr(2, filename.size() - 6);
        auto dotPos = coords.find('.');
        if (dotPos != std::string::npos) {
            try {
                m_regionX = std::stoi(coords.substr(0, dotPos));
                m_regionZ = std::stoi(coords.substr(dotPos + 1));
            }
            catch (...) {
                spdlog::warn("RegionFile: Failed to parse region coords from {}", filename);
            }
        }
    }
}

RegionFile::~RegionFile()
{
    close();
}

RegionFile::RegionFile(RegionFile&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_stream(std::move(other.m_stream))
    , m_locations(other.m_locations)
    , m_timestamps(other.m_timestamps)
    , m_isOpen(other.m_isOpen)
    , m_regionX(other.m_regionX)
    , m_regionZ(other.m_regionZ)
{
    other.m_isOpen = false;
}

RegionFile& RegionFile::operator=(RegionFile&& other) noexcept
{
    if (this != &other) {
        close();
        m_path = std::move(other.m_path);
        m_stream = std::move(other.m_stream);
        m_locations = other.m_locations;
        m_timestamps = other.m_timestamps;
        m_isOpen = other.m_isOpen;
        m_regionX = other.m_regionX;
        m_regionZ = other.m_regionZ;
        other.m_isOpen = false;
    }
    return *this;
}

Result<void> RegionFile::open()
{
    if (m_isOpen) {
        return {};
    }

    std::error_code ec;
    if (!std::filesystem::exists(m_path, ec)) {
        return Error(ErrorCode::FileNotFound, fmt::format("Region file not found: {}", m_path.string()));
    }

    m_stream.open(m_path, std::ios::binary);
    if (!m_stream.is_open()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Failed to open region file: {}", m_path.string()));
    }

    auto headerResult = _readHeader();
    if (headerResult.failed()) {
        m_stream.close();
        return headerResult.error();
    }

    m_isOpen = true;
    return {};
}

void RegionFile::close()
{
    if (m_stream.is_open()) {
        m_stream.close();
    }
    m_isOpen = false;
}

bool RegionFile::hasChunk(i32 localX, i32 localZ) const
{
    if (localX < 0 || localX >= REGION_SIZE || localZ < 0 || localZ >= REGION_SIZE) {
        return false;
    }
    i32 index = localZ * REGION_SIZE + localX;
    return m_locations[index].offset != 0 && m_locations[index].sectorCount != 0;
}

Result<std::vector<u8>> RegionFile::readChunkData(i32 localX, i32 localZ)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Region file not open");
    }

    if (localX < 0 || localX >= REGION_SIZE || localZ < 0 || localZ >= REGION_SIZE) {
        return Error(ErrorCode::ChunkNotFound, fmt::format("Chunk ({}, {}) out of region bounds", localX, localZ));
    }

    i32 index = localZ * REGION_SIZE + localX;
    const auto& loc = m_locations[index];

    if (loc.offset == 0 || loc.sectorCount == 0) {
        return Error(ErrorCode::ChunkNotFound, fmt::format("Chunk ({}, {}) not present in region", localX, localZ));
    }

    // 定位到区块数据
    std::streampos dataOffset = static_cast<std::streampos>(loc.offset) * SECTOR_SIZE;
    m_stream.seekg(dataOffset);
    if (!m_stream.good()) {
        return Error(ErrorCode::ChunkCorrupted, fmt::format("Failed to seek to chunk data at offset {}", loc.offset));
    }

    // 读取长度（4 字节大端序）
    u8 lengthBytes[4] = {};
    m_stream.read(reinterpret_cast<char*>(lengthBytes), 4);
    if (!m_stream.good()) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to read chunk length");
    }
    u32 length = (static_cast<u32>(lengthBytes[0]) << 24) | (static_cast<u32>(lengthBytes[1]) << 16) |
        (static_cast<u32>(lengthBytes[2]) << 8) | static_cast<u32>(lengthBytes[3]);

    if (length == 0 || length > static_cast<u32>(loc.sectorCount) * SECTOR_SIZE) {
        return Error(ErrorCode::ChunkCorrupted,
            fmt::format("Invalid chunk length {} (max sector bytes: {})",
                length,
                static_cast<u32>(loc.sectorCount) * SECTOR_SIZE));
    }

    // 读取压缩类型（1 字节）
    u8 compressionByte = 0;
    m_stream.read(reinterpret_cast<char*>(&compressionByte), 1);
    if (!m_stream.good()) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to read compression type");
    }

    auto compressionType = static_cast<CompressionType>(compressionByte);

    // 读取压缩数据（length - 1 字节，减去压缩类型占用的 1 字节）
    u32 dataLength = length - 1;
    if (dataLength == 0) {
        return Error(ErrorCode::ChunkCorrupted, "Chunk data is empty");
    }

    std::vector<u8> compressedData(dataLength);
    m_stream.read(reinterpret_cast<char*>(compressedData.data()), static_cast<std::streamsize>(dataLength));
    if (!m_stream.good()) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to read chunk compressed data");
    }

    return _decompress(compressionType, compressedData);
}

std::vector<std::pair<i32, i32>> RegionFile::listChunks() const
{
    std::vector<std::pair<i32, i32>> chunks;
    for (i32 z = 0; z < REGION_SIZE; ++z) {
        for (i32 x = 0; x < REGION_SIZE; ++x) {
            i32 index = z * REGION_SIZE + x;
            if (m_locations[index].offset != 0 && m_locations[index].sectorCount != 0) {
                chunks.emplace_back(x, z);
            }
        }
    }
    return chunks;
}

Result<void> RegionFile::_readHeader()
{
    // 读取偏移表（1024 个 4 字节条目）
    for (i32 i = 0; i < CHUNKS_PER_REGION; ++i) {
        u8 bytes[4] = {};
        m_stream.read(reinterpret_cast<char*>(bytes), 4);
        if (!m_stream.good()) {
            return Error(ErrorCode::ChunkCorrupted, "Failed to read region offset table");
        }

        // 偏移：高 3 字节 = 扇区偏移，低 1 字节 = 扇区数
        u32 raw = (static_cast<u32>(bytes[0]) << 24) | (static_cast<u32>(bytes[1]) << 16) |
            (static_cast<u32>(bytes[2]) << 8) | static_cast<u32>(bytes[3]);
        m_locations[i].offset = (raw >> 8) & 0x00FFFFFF;
        m_locations[i].sectorCount = raw & 0xFF;
    }

    // 读取时间戳表（1024 个 4 字节条目）
    for (i32 i = 0; i < CHUNKS_PER_REGION; ++i) {
        u8 bytes[4] = {};
        m_stream.read(reinterpret_cast<char*>(bytes), 4);
        if (!m_stream.good()) {
            return Error(ErrorCode::ChunkCorrupted, "Failed to read region timestamp table");
        }
        m_timestamps[i] = (static_cast<u32>(bytes[0]) << 24) | (static_cast<u32>(bytes[1]) << 16) |
            (static_cast<u32>(bytes[2]) << 8) | static_cast<u32>(bytes[3]);
    }

    return {};
}

Result<std::vector<u8>> RegionFile::_decompress(CompressionType type, const std::vector<u8>& data)
{
    const u8 rawType = static_cast<u8>(type);
    if ((rawType & static_cast<u8>(CompressionType::External)) != 0) {
        return Error(ErrorCode::Unsupported, "External .mcc chunk storage is not supported yet");
    }

    switch (type) {
        case CompressionType::GZip:
            return mc::util::decompressGzip(data);
        case CompressionType::ZLib:
            return mc::util::decompressZlib(data);
        case CompressionType::Uncompressed:
        case CompressionType::None:
            return data;
        case CompressionType::LZ4:
            return Error(ErrorCode::DecompressionFailed, "LZ4 compression not supported for Java region files");
        default:
            return Error(
                ErrorCode::DecompressionFailed, fmt::format("Unknown compression type: {}", static_cast<i32>(type)));
    }
}

} // namespace mc::world::storage::reader::java
