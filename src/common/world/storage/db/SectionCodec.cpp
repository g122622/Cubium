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

#include "SectionCodec.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>
#include <zstd.h>

using namespace mc::trace;

namespace mc::world::storage {

namespace {

[[nodiscard]] Result<void> validateSectionDataLayout(const SectionData& data, const char* context)
{
    if (data.blockStates.size() != SectionData::VOLUME) {
        spdlog::error("[{}] blockStates size mismatch: expected {}, got {}",
            context,
            SectionData::VOLUME,
            data.blockStates.size());
        return Error(ErrorCode::InvalidData,
            fmt::format("{}: blockStates size mismatch (expected {}, got {})",
                context,
                SectionData::VOLUME,
                data.blockStates.size()));
    }

    if (data.biomes.size() != SectionData::BIOME_COUNT) {
        spdlog::error(
            "[{}] biomes size mismatch: expected {}, got {}", context, SectionData::BIOME_COUNT, data.biomes.size());
        return Error(ErrorCode::InvalidData,
            fmt::format("{}: biomes size mismatch (expected {}, got {})",
                context,
                SectionData::BIOME_COUNT,
                data.biomes.size()));
    }

    if (data.nonEmptyBlockCount > SectionData::VOLUME) {
        spdlog::error("[{}] nonEmptyBlockCount out of range: expected <= {}, got {}",
            context,
            SectionData::VOLUME,
            data.nonEmptyBlockCount);
        return Error(ErrorCode::InvalidData,
            fmt::format("{}: nonEmptyBlockCount out of range ({} > {})",
                context,
                data.nonEmptyBlockCount,
                SectionData::VOLUME));
    }

    if (data.skyLight.has_value() && data.skyLight->size() != SectionCodec::LIGHT_DATA_SIZE) {
        spdlog::error("[{}] skyLight size mismatch: expected {}, got {}",
            context,
            SectionCodec::LIGHT_DATA_SIZE,
            data.skyLight->size());
        return Error(ErrorCode::InvalidData,
            fmt::format("{}: skyLight size mismatch (expected {}, got {})",
                context,
                SectionCodec::LIGHT_DATA_SIZE,
                data.skyLight->size()));
    }

    if (data.blockLight.has_value() && data.blockLight->size() != SectionCodec::LIGHT_DATA_SIZE) {
        spdlog::error("[{}] blockLight size mismatch: expected {}, got {}",
            context,
            SectionCodec::LIGHT_DATA_SIZE,
            data.blockLight->size());
        return Error(ErrorCode::InvalidData,
            fmt::format("{}: blockLight size mismatch (expected {}, got {})",
                context,
                SectionCodec::LIGHT_DATA_SIZE,
                data.blockLight->size()));
    }

    return {};
}

// 反序列化解压暂存缓冲区（thread_local 复用）。
// decompressInto 仅在 SectionData::deserialize 内部调用，结果在同一函数内立即 memcpy 到
// result.blockStates 后即弃用，不会跨另一次 decompress 调用存活，故 thread_local 安全。
// 复用避免每区块 24 次 16KB 堆分配（ServerCompute 反序列化热路径）。
thread_local std::vector<u8> g_decompressScratch;

} // namespace

// ============================================================================
// SectionData 实现
// ============================================================================

SectionData::SectionData()
    : m_memTrack(this)
{
    initializeDefaults();
}

SectionData::SectionData(const SectionKey& key)
    : m_memTrack(this)
    , key(key)
{
    initializeDefaults();
}

SectionData::SectionData(i32 chunkX, i32 chunkZ, i8 sectionY, DimensionId dimension)
    : m_memTrack(this)
    , key(chunkX, chunkZ, sectionY, dimension)
{
    initializeDefaults();
}

SectionData::SectionData(const SectionData& other)
    : m_memTrack(this) // 新对象新地址，bind this 发 alloc（守卫不可拷贝，故显式构造+绑定）
    , key(other.key)
    , blockStates(other.blockStates)
    , nonEmptyBlockCount(other.nonEmptyBlockCount)
    , biomes(other.biomes)
    , skyLight(other.skyLight)
    , blockLight(other.blockLight)
    , dataVersion(other.dataVersion)
    , contentHash(other.contentHash)
{}

SectionData::SectionData(SectionData&& other) noexcept
    : m_memTrack() // 默认构造为非活跃，body 中重绑定
    , key(other.key)
    , blockStates(std::move(other.blockStates))
    , nonEmptyBlockCount(other.nonEmptyBlockCount)
    , biomes(std::move(other.biomes))
    , skyLight(std::move(other.skyLight))
    , blockLight(std::move(other.blockLight))
    , dataVersion(other.dataVersion)
    , contentHash(other.contentHash)
{
    // 对象级追踪重绑定：释放源地址、分配目标地址（守卫不可移动，故在 body 处理，
    // 初始化列表中默认构造为非活跃）。若不重绑定，move 后源地址仍留在 Tracy 活跃集，
    // 堆复用该地址时触发 MemAllocTwice 硬失败。
    other.m_memTrack.unbind();
    m_memTrack.bind(this);
}

SectionData& SectionData::operator=(SectionData&& other) noexcept
{
    if (this != &other) {
        key = other.key;
        blockStates = std::move(other.blockStates);
        nonEmptyBlockCount = other.nonEmptyBlockCount;
        biomes = std::move(other.biomes);
        skyLight = std::move(other.skyLight);
        blockLight = std::move(other.blockLight);
        dataVersion = other.dataVersion;
        contentHash = other.contentHash;

        // 对象级追踪重绑定（同 move ctor 语义）：释放双方旧地址、目标重新绑定新地址
        m_memTrack.unbind();
        other.m_memTrack.unbind();
        m_memTrack.bind(this);
    }
    return *this;
}

void SectionData::initializeDefaults()
{
    // 初始化方块状态数组（全为空气，stateId=0）
    blockStates.resize(VOLUME, 0);
    nonEmptyBlockCount = 0;

    // 初始化生物群系数组（默认为平原 biome_id=1）
    biomes.resize(BIOME_COUNT, 1);

    // 光照数据不初始化（可选）
    skyLight.reset();
    blockLight.reset();

    // 数据版本
    dataVersion = CURRENT_DATA_VERSION;
    contentHash = 0;
}

void SectionData::clear()
{
    std::fill(blockStates.begin(), blockStates.end(), 0);
    nonEmptyBlockCount = 0;
    std::fill(biomes.begin(), biomes.end(), 1);
    skyLight.reset();
    blockLight.reset();
    contentHash = 0;
}

// ============================================================================
// 数据访问
// ============================================================================

u32 SectionData::getBlockStateId(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return 0;
    }
    return blockStates[static_cast<size_t>(_blockIndex(x, y, z))];
}

void SectionData::setBlockStateId(i32 x, i32 y, i32 z, u32 stateId)
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return;
    }

    i32 idx = _blockIndex(x, y, z);
    u32 oldStateId = blockStates[static_cast<size_t>(idx)];

    // 更新非空方块计数
    if (oldStateId == 0 && stateId != 0) {
        ++nonEmptyBlockCount;
    } else if (oldStateId != 0 && stateId == 0) {
        --nonEmptyBlockCount;
    }

    blockStates[static_cast<size_t>(idx)] = stateId;
}

BiomeId SectionData::getBiome(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return 1; // 默认平原
    }
    return biomes[static_cast<size_t>(_biomeIndex(x, y, z))];
}

void SectionData::setBiome(i32 x, i32 y, i32 z, BiomeId biome)
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return;
    }
    biomes[static_cast<size_t>(_biomeIndex(x, y, z))] = biome;
}

// ============================================================================
// 序列化
// ============================================================================

Result<std::vector<u8>> SectionData::serialize() const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionData::serialize", "sectionY", static_cast<i32>(key.sectionY));

    auto validationResult = validateSectionDataLayout(*this, "SectionData::serialize");
    if (validationResult.failed()) {
        return validationResult.error();
    }

    std::vector<u8> output;

    // 计算标志位
    SectionFlags flags = SectionFlags::None;
    if (!biomes.empty()) {
        flags |= SectionFlags::HasBiomes;
    }
    if (skyLight.has_value()) {
        flags |= SectionFlags::HasSkyLight;
    }
    if (blockLight.has_value()) {
        flags |= SectionFlags::HasBlockLight;
    }
    if (isEmpty()) {
        flags |= SectionFlags::IsEmpty;
    }

    // Header (12字节)
    // - version (u16)
    // - flags (u16)
    // - blockCount (u16)
    // - reserved (u16)
    // - contentHash (u32)
    output.resize(12);

    // 版本
    u16 version = static_cast<u16>(SectionFormatVersion::Current);
    output[0] = static_cast<u8>(version >> 8);
    output[1] = static_cast<u8>(version & 0xFF);

    // 标志位
    u16 flagsValue = static_cast<u16>(flags);
    output[2] = static_cast<u8>(flagsValue >> 8);
    output[3] = static_cast<u8>(flagsValue & 0xFF);

    // 非空方块数
    output[4] = static_cast<u8>(nonEmptyBlockCount >> 8);
    output[5] = static_cast<u8>(nonEmptyBlockCount & 0xFF);

    // 保留
    output[6] = 0;
    output[7] = 0;

    // 内容哈希低32位
    u32 hashLow = static_cast<u32>(contentHash & 0xFFFFFFFF);
    output[8] = static_cast<u8>(hashLow >> 24);
    output[9] = static_cast<u8>(hashLow >> 16);
    output[10] = static_cast<u8>(hashLow >> 8);
    output[11] = static_cast<u8>(hashLow & 0xFF);

    // 方块状态数据
    if (!isEmpty()) {
        // 压缩方块状态
        auto compressResult =
            SectionCodec::compress(reinterpret_cast<const u8*>(blockStates.data()), blockStates.size() * sizeof(u32));

        if (!compressResult.success()) {
            return compressResult.error();
        }

        const auto& compressed = compressResult.value();

        // 写入压缩大小（u32）
        u32 compressedSize = static_cast<u32>(compressed.size());
        output.push_back(static_cast<u8>(compressedSize >> 24));
        output.push_back(static_cast<u8>(compressedSize >> 16));
        output.push_back(static_cast<u8>(compressedSize >> 8));
        output.push_back(static_cast<u8>(compressedSize & 0xFF));

        // 写入压缩数据
        output.insert(output.end(), compressed.begin(), compressed.end());
    }

    // 生物群系数据（每个BiomeId是u16，需要逐字节序列化）
    if (hasFlag(flags, SectionFlags::HasBiomes) && !biomes.empty()) {
        // 按大端序序列化每个 BiomeId (u16)
        for (BiomeId biome : biomes) {
            output.push_back(static_cast<u8>(biome >> 8));   // 高字节
            output.push_back(static_cast<u8>(biome & 0xFF)); // 低字节
        }
    }

    // 天空光照
    if (hasFlag(flags, SectionFlags::HasSkyLight) && skyLight.has_value()) {
        output.insert(output.end(), skyLight->begin(), skyLight->end());
    }

    // 方块光照
    if (hasFlag(flags, SectionFlags::HasBlockLight) && blockLight.has_value()) {
        output.insert(output.end(), blockLight->begin(), blockLight->end());
    }

    return output;
}

Result<SectionData> SectionData::deserialize(const u8* data, size_t size)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionData::deserialize", "size", size);

    if (size < 12) {
        return Error(ErrorCode::InvalidData, "Section data too small for header");
    }

    SectionData result;

    // 解析Header
    u16 version = (static_cast<u16>(data[0]) << 8) | data[1];
    if (version > static_cast<u16>(SectionFormatVersion::Current)) {
        return Error(ErrorCode::InvalidData, fmt::format("Unsupported section format version: {}", version));
    }

    SectionFlags flags = static_cast<SectionFlags>((static_cast<u16>(data[2]) << 8) | data[3]);
    result.nonEmptyBlockCount = (static_cast<u16>(data[4]) << 8) | data[5];
    // data[6], data[7] reserved
    result.contentHash = (static_cast<u64>(data[8]) << 24) | (static_cast<u64>(data[9]) << 16) |
        (static_cast<u64>(data[10]) << 8) | static_cast<u64>(data[11]);

    if (result.nonEmptyBlockCount > VOLUME) {
        return Error(ErrorCode::InvalidData,
            fmt::format("Section data has invalid block count: {} > {}", result.nonEmptyBlockCount, VOLUME));
    }

    size_t offset = 12;

    // 方块状态数据
    if (!hasFlag(flags, SectionFlags::IsEmpty)) {
        if (offset + 4 > size) {
            return Error(ErrorCode::InvalidData, "Section data truncated at compressed size");
        }

        // 读取压缩大小
        u32 compressedSize = (static_cast<u32>(data[offset]) << 24) | (static_cast<u32>(data[offset + 1]) << 16) |
            (static_cast<u32>(data[offset + 2]) << 8) | static_cast<u32>(data[offset + 3]);
        offset += 4;

        if (offset + compressedSize > size) {
            return Error(ErrorCode::InvalidData, "Section data truncated at compressed data");
        }

        // 解压缩到 thread_local 暂存缓冲区（复用，避免 16KB 堆分配）
        if (g_decompressScratch.size() < SectionCodec::UNCOMPRESSED_BLOCK_STATES_SIZE) {
            g_decompressScratch.resize(SectionCodec::UNCOMPRESSED_BLOCK_STATES_SIZE);
        }
        auto decompressResult = SectionCodec::decompressInto(
            data + offset, compressedSize, g_decompressScratch.data(), SectionCodec::UNCOMPRESSED_BLOCK_STATES_SIZE);

        if (!decompressResult.success()) {
            return decompressResult.error();
        }

        // 复制方块状态
        result.blockStates.resize(VOLUME);
        std::memcpy(result.blockStates.data(), g_decompressScratch.data(), VOLUME * sizeof(u32));

        offset += compressedSize;
    } else {
        // 空Section，全为空气
        result.blockStates.assign(VOLUME, 0);
    }

    // 生物群系数据（每个BiomeId是u16，需要逐字节反序列化）
    if (hasFlag(flags, SectionFlags::HasBiomes)) {
        if (offset + SectionCodec::BIOME_DATA_SIZE > size) {
            return Error(ErrorCode::InvalidData, "Section data truncated at biomes");
        }
        result.biomes.resize(SectionData::BIOME_COUNT);
        // 按大端序读取每个 BiomeId (u16)
        for (size_t i = 0; i < SectionData::BIOME_COUNT; ++i) {
            u8 high = data[offset++];
            u8 low = data[offset++];
            result.biomes[i] = static_cast<BiomeId>((static_cast<u16>(high) << 8) | low);
        }
    } else {
        // 默认平原生物群系
        result.biomes.assign(SectionData::BIOME_COUNT, 1);
    }

    // 天空光照
    if (hasFlag(flags, SectionFlags::HasSkyLight)) {
        if (offset + SectionCodec::LIGHT_DATA_SIZE > size) {
            return Error(ErrorCode::InvalidData, "Section data truncated at sky light");
        }
        result.skyLight = std::vector<u8>(data + offset, data + offset + SectionCodec::LIGHT_DATA_SIZE);
        offset += SectionCodec::LIGHT_DATA_SIZE;
    }

    // 方块光照
    if (hasFlag(flags, SectionFlags::HasBlockLight)) {
        if (offset + SectionCodec::LIGHT_DATA_SIZE > size) {
            return Error(ErrorCode::InvalidData, "Section data truncated at block light");
        }
        result.blockLight = std::vector<u8>(data + offset, data + offset + SectionCodec::LIGHT_DATA_SIZE);
        offset += SectionCodec::LIGHT_DATA_SIZE;
    }

    auto validationResult = validateSectionDataLayout(result, "SectionData::deserialize");
    if (validationResult.failed()) {
        return validationResult.error();
    }

    return result;
}

// ============================================================================
// 哈希计算
// ============================================================================

void SectionData::computeHash()
{
    // 使用简单的FNV-1a哈希
    // 注意：这是一个简化的哈希实现，生产环境可能需要更强的哈希如xxHash或SipHash

    constexpr u64 FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr u64 FNV_PRIME = 1099511628211ULL;

    u64 hash = FNV_OFFSET_BASIS;

    // 哈希方块状态
    for (u32 state : blockStates) {
        hash ^= static_cast<u64>(state) & 0xFF;
        hash *= FNV_PRIME;
        hash ^= (static_cast<u64>(state) >> 8) & 0xFF;
        hash *= FNV_PRIME;
        hash ^= (static_cast<u64>(state) >> 16) & 0xFF;
        hash *= FNV_PRIME;
        hash ^= (static_cast<u64>(state) >> 24) & 0xFF;
        hash *= FNV_PRIME;
    }

    // 哈希生物群系
    for (BiomeId biome : biomes) {
        hash ^= static_cast<u64>(biome);
        hash *= FNV_PRIME;
    }

    contentHash = hash;
}

// ============================================================================
// SectionCodec 实现
// ============================================================================

Result<SectionData> SectionCodec::fromChunkSection(
    const ChunkSection& section, const SectionKey& key, const std::vector<BiomeId>& biomes)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionCodec::fromChunkSection");

    SectionData data(key);

    // 复制方块状态
    data.blockStates.resize(SectionData::VOLUME);
    for (i32 i = 0; i < SectionData::VOLUME; ++i) {
        data.blockStates[static_cast<size_t>(i)] = section.getBlockStateIdFast(i);
    }

    // 复制非空方块计数
    data.nonEmptyBlockCount = section.getBlockCount();

    // 复制生物群系：由调用方提供 4x4x4 采样数据，未提供时退回默认平原
    if (!biomes.empty() && biomes.size() == SectionData::BIOME_COUNT) {
        data.biomes = biomes;
    } else {
        data.biomes.assign(SectionData::BIOME_COUNT, 1);
    }

    // 复制光照数据（仅当非默认值时）
    // 天空光照：默认全15（日光），只有当存在非15值时才存储
    // 方块光照：默认全0（无光），只有当存在非0值时才存储
    const auto& skyLight = section.skyLightNibble();
    const auto& blockLight = section.blockLightNibble();

    // 检查天空光照是否有非默认值（非15）
    if (!skyLight.isEmpty()) {
        bool hasNonDefaultSkyLight = false;
        for (size_t i = 0; i < LIGHT_DATA_SIZE; ++i) {
            // 每个 nibble 都是 15 才是默认值
            u8 byte = skyLight.rawData()[i];
            if (byte != 0xFF) { // 0xFF = 两个 nibble 都是 15
                hasNonDefaultSkyLight = true;
                break;
            }
        }
        if (hasNonDefaultSkyLight) {
            data.skyLight = std::vector<u8>(skyLight.rawData(), skyLight.rawData() + LIGHT_DATA_SIZE);
        }
    }

    // 检查方块光照是否有非默认值（非0）
    if (!blockLight.isEmpty()) {
        bool hasNonDefaultBlockLight = false;
        for (size_t i = 0; i < LIGHT_DATA_SIZE; ++i) {
            if (blockLight.rawData()[i] != 0) {
                hasNonDefaultBlockLight = true;
                break;
            }
        }
        if (hasNonDefaultBlockLight) {
            data.blockLight = std::vector<u8>(blockLight.rawData(), blockLight.rawData() + LIGHT_DATA_SIZE);
        }
    }

    // 计算哈希
    data.computeHash();

    return data;
}

Result<void> SectionCodec::toChunkSection(const SectionData& data, ChunkSection& section)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionCodec::toChunkSection");

    auto validationResult = validateSectionDataLayout(data, "SectionCodec::toChunkSection");
    if (validationResult.failed()) {
        return validationResult.error();
    }

    // 设置方块状态
    for (i32 i = 0; i < SectionData::VOLUME; ++i) {
        section.setBlockStateIdFast(i, data.blockStates[static_cast<size_t>(i)]);
    }

    // 设置非空方块计数
    section.setBlockCount(data.nonEmptyBlockCount);

    // 设置光照数据
    if (data.skyLight.has_value() && data.skyLight->size() == SectionCodec::LIGHT_DATA_SIZE) {
        auto& skyLight = section.skyLightNibble();
        // 确保目标 NibbleArray 已分配，避免向空指针 memcpy。
        auto& skyLightData = skyLight.data();
        std::memcpy(skyLightData.data(), data.skyLight->data(), SectionCodec::LIGHT_DATA_SIZE);
    }

    if (data.blockLight.has_value() && data.blockLight->size() == SectionCodec::LIGHT_DATA_SIZE) {
        auto& blockLight = section.blockLightNibble();
        // 默认 ChunkSection 的方块光数组可能为空，这里需要先显式分配。
        auto& blockLightData = blockLight.data();
        std::memcpy(blockLightData.data(), data.blockLight->data(), SectionCodec::LIGHT_DATA_SIZE);
    }

    // 标记需要重新计算
    section.setNeedsRecalculate(true);

    return {};
}

// ============================================================================
// 压缩工具
// ============================================================================

Result<std::vector<u8>> SectionCodec::compress(const u8* data, size_t size, i32 compressionLevel)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionCodec::compress", "size", size, "level", compressionLevel);

    // 获取压缩后最大大小
    size_t bound = ZSTD_compressBound(size);
    std::vector<u8> compressed(bound);

    // 执行压缩
    size_t result = ZSTD_compress(compressed.data(), compressed.size(), data, size, compressionLevel);

    if (ZSTD_isError(result)) {
        return Error(
            ErrorCode::CompressionFailed, fmt::format("ZSTD compression failed: {}", ZSTD_getErrorName(result)));
    }

    // 调整到实际大小
    compressed.resize(result);

    return compressed;
}

Result<void> SectionCodec::decompressInto(const u8* compressedData, size_t compressedSize, u8* out, size_t expectedSize)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db,
        "SectionCodec::decompressInto",
        "compressedSize",
        compressedSize,
        "expectedSize",
        expectedSize);

    size_t result = ZSTD_decompress(out, expectedSize, compressedData, compressedSize);

    if (ZSTD_isError(result)) {
        return Error(
            ErrorCode::DecompressionFailed, fmt::format("ZSTD decompression failed: {}", ZSTD_getErrorName(result)));
    }

    if (result != expectedSize) {
        return Error(ErrorCode::DecompressionFailed,
            fmt::format("Decompressed size mismatch: expected {}, got {}", expectedSize, result));
    }

    return Result<void>::ok();
}

} // namespace mc::world::storage
