#include "RegionFile.hpp"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace mc::world::save::region {

// ========== 静态方法 ==========

Result<std::unique_ptr<RegionFile>>
RegionFile::open(const std::filesystem::path& path, bool sync) {
    auto regionFile = std::unique_ptr<RegionFile>(new RegionFile(path, sync));

    auto result = regionFile->initialize();
    if (result.failed()) {
        return result.error();
    }

    return regionFile;
}

// ========== 构造函数/析构函数 ==========

RegionFile::RegionFile(const std::filesystem::path& path, bool sync)
    : m_path(path)
    , m_sync(sync)
    , m_offsets(CHUNKS_PER_REGION, 0)
    , m_timestamps(CHUNKS_PER_REGION, 0)
{
    // 解析 Region 坐标
    parseRegionCoords(path, m_regionX, m_regionZ);
}

RegionFile::~RegionFile() {
    close();
}

RegionFile::RegionFile(RegionFile&& other) noexcept
    : m_path(std::move(other.m_path))
    , m_regionX(other.m_regionX)
    , m_regionZ(other.m_regionZ)
    , m_sync(other.m_sync)
    , m_closed(other.m_closed)
    , m_file(other.m_file)
    , m_offsets(std::move(other.m_offsets))
    , m_timestamps(std::move(other.m_timestamps))
    , m_bitmap(std::move(other.m_bitmap))
    , m_usedChunks(other.m_usedChunks)
{
    other.m_file = nullptr;
    other.m_closed = true;
}

RegionFile& RegionFile::operator=(RegionFile&& other) noexcept {
    if (this != &other) {
        close();
        m_path = std::move(other.m_path);
        m_regionX = other.m_regionX;
        m_regionZ = other.m_regionZ;
        m_sync = other.m_sync;
        m_closed = other.m_closed;
        m_file = other.m_file;
        m_offsets = std::move(other.m_offsets);
        m_timestamps = std::move(other.m_timestamps);
        m_bitmap = std::move(other.m_bitmap);
        m_usedChunks = other.m_usedChunks;
        other.m_file = nullptr;
        other.m_closed = true;
    }
    return *this;
}

// ========== 初始化 ==========

Result<void> RegionFile::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查文件是否存在
    bool exists = std::filesystem::exists(m_path);

    // 打开文件（读写二进制模式）
#ifdef _WIN32
    errno_t err = fopen_s(&m_file, m_path.string().c_str(), "r+b");
    if (err != 0 || m_file == nullptr) {
        // 文件不存在，创建新文件
        err = fopen_s(&m_file, m_path.string().c_str(), "w+b");
        if (err != 0 || m_file == nullptr) {
            return Error(ErrorCode::FileOpenFailed,
                         std::string("Failed to create region file: ") + m_path.string());
        }
    }
#else
    m_file = fopen(m_path.string().c_str(), "r+b");
    if (m_file == nullptr) {
        // 文件不存在，创建新文件
        m_file = fopen(m_path.string().c_str(), "w+b");
        if (m_file == nullptr) {
            return Error(ErrorCode::FileOpenFailed,
                         std::string("Failed to create region file: ") + m_path.string());
        }
    }
#endif

    if (exists) {
        // 读取现有头部
        auto result = readHeader();
        if (result.failed()) {
            fclose(m_file);
            m_file = nullptr;
            return result.error();
        }

        // 初始化位图
        m_bitmap = RegionBitmap(std::filesystem::file_size(m_path));
        for (u32 i = 0; i < CHUNKS_PER_REGION; ++i) {
            u32 entry = m_offsets[i];
            if (entry != 0) {
                u32 sectorOffset = getSectorOffset(entry);
                u32 sectorCount = getSectorCount(entry);
                if (sectorOffset >= 2 && sectorCount > 0) {
                    m_bitmap.markUsed(sectorOffset, sectorCount);
                    ++m_usedChunks;
                }
            }
        }
    } else {
        // 创建新文件的头部
        std::vector<u8> header(HEADER_SIZE, 0);
        if (fwrite(header.data(), 1, HEADER_SIZE, m_file) != HEADER_SIZE) {
            fclose(m_file);
            m_file = nullptr;
            return Error(ErrorCode::FileWriteFailed,
                         std::string("Failed to write region header: ") + m_path.string());
        }

        // 初始化位图（只有头部）
        m_bitmap = RegionBitmap(HEADER_SIZE);

        // 确保目录存在
        std::filesystem::path dir = m_path.parent_path();
        if (!dir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
        }
    }

    return {};
}

Result<void> RegionFile::readHeader() {
    // 读取偏移表（前 4096 字节）
    std::vector<u8> offsetData(SECTOR_SIZE);
    if (fread(offsetData.data(), 1, SECTOR_SIZE, m_file) != SECTOR_SIZE) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to read offset table: ") + m_path.string());
    }

    // 读取时间戳表（后 4096 字节）
    std::vector<u8> timestampData(SECTOR_SIZE);
    if (fread(timestampData.data(), 1, SECTOR_SIZE, m_file) != SECTOR_SIZE) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to read timestamp table: ") + m_path.string());
    }

    // 解析偏移表（大端序）
    for (u32 i = 0; i < CHUNKS_PER_REGION; ++i) {
        u32 offset = (static_cast<u32>(offsetData[i * 4]) << 24) |
                     (static_cast<u32>(offsetData[i * 4 + 1]) << 16) |
                     (static_cast<u32>(offsetData[i * 4 + 2]) << 8) |
                     static_cast<u32>(offsetData[i * 4 + 3]);
        m_offsets[i] = offset;
    }

    // 解析时间戳表（大端序）
    for (u32 i = 0; i < CHUNKS_PER_REGION; ++i) {
        u32 timestamp = (static_cast<u32>(timestampData[i * 4]) << 24) |
                        (static_cast<u32>(timestampData[i * 4 + 1]) << 16) |
                        (static_cast<u32>(timestampData[i * 4 + 2]) << 8) |
                        static_cast<u32>(timestampData[i * 4 + 3]);
        m_timestamps[i] = timestamp;
    }

    return {};
}

Result<void> RegionFile::writeHeader() {
    // 定位到文件开头
    if (fseek(m_file, 0, SEEK_SET) != 0) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to seek to header: ") + m_path.string());
    }

    // 构建偏移表（大端序）
    std::vector<u8> offsetData(SECTOR_SIZE);
    for (u32 i = 0; i < CHUNKS_PER_REGION; ++i) {
        u32 offset = m_offsets[i];
        offsetData[i * 4] = static_cast<u8>((offset >> 24) & 0xFF);
        offsetData[i * 4 + 1] = static_cast<u8>((offset >> 16) & 0xFF);
        offsetData[i * 4 + 2] = static_cast<u8>((offset >> 8) & 0xFF);
        offsetData[i * 4 + 3] = static_cast<u8>(offset & 0xFF);
    }

    // 构建时间戳表（大端序）
    std::vector<u8> timestampData(SECTOR_SIZE);
    for (u32 i = 0; i < CHUNKS_PER_REGION; ++i) {
        u32 timestamp = m_timestamps[i];
        timestampData[i * 4] = static_cast<u8>((timestamp >> 24) & 0xFF);
        timestampData[i * 4 + 1] = static_cast<u8>((timestamp >> 16) & 0xFF);
        timestampData[i * 4 + 2] = static_cast<u8>((timestamp >> 8) & 0xFF);
        timestampData[i * 4 + 3] = static_cast<u8>(timestamp & 0xFF);
    }

    // 写入偏移表
    if (fwrite(offsetData.data(), 1, SECTOR_SIZE, m_file) != SECTOR_SIZE) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to write offset table: ") + m_path.string());
    }

    // 写入时间戳表
    if (fwrite(timestampData.data(), 1, SECTOR_SIZE, m_file) != SECTOR_SIZE) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to write timestamp table: ") + m_path.string());
    }

    // 同步到磁盘
    if (m_sync) {
        fflush(m_file);
    }

    return {};
}

// ========== 区块操作 ==========

Result<std::optional<nbt::CompoundTag>>
RegionFile::readChunk(u32 localX, u32 localZ) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed) {
        return Error(ErrorCode::InvalidState, "Region file is closed");
    }

    // 验证坐标
    if (localX >= CHUNKS_PER_SIDE || localZ >= CHUNKS_PER_SIDE) {
        return Error(ErrorCode::OutOfBounds, "Chunk coordinates out of range");
    }

    u32 index = getChunkIndex(localX, localZ);
    u32 entry = m_offsets[index];

    // 检查区块是否存在
    if (entry == 0) {
        return std::nullopt;
    }

    // 读取原始数据
    auto rawResult = readRawChunk(index);
    if (rawResult.failed()) {
        return rawResult.error();
    }

    auto& rawData = rawResult.value();
    if (!rawData.has_value()) {
        return std::nullopt;
    }

    // 解析压缩类型和数据
    if (rawData->size() < 5) {
        return Error(ErrorCode::ResourceParseError, "Chunk data too small");
    }

    u32 length = (static_cast<u32>((*rawData)[0]) << 24) |
                 (static_cast<u32>((*rawData)[1]) << 16) |
                 (static_cast<u32>((*rawData)[2]) << 8) |
                 static_cast<u32>((*rawData)[3]);
    u8 compressionByte = (*rawData)[4];

    CompressionType compression = CompressionTypeUtils::fromByte(compressionByte);

    // 检查是否为外部文件
    bool isExternal = CompressionTypeUtils::isExternal(compression);
    CompressionType baseCompression = CompressionTypeUtils::getBaseType(compression);

    const u8* data = rawData->data() + 5;
    size_t dataSize = rawData->size() - 5;

    std::vector<u8> externalData;
    if (isExternal) {
        // 读取外部文件
        std::filesystem::path externalPath = m_path;
        externalPath.replace_extension(
            ".mcc"  // TODO: 应该是 c.x.z.mcc
        );

        auto externalResult = io::FileUtil::readFile(externalPath);
        if (externalResult.failed()) {
            return Error(ErrorCode::FileNotFound,
                         std::string("External chunk file not found: ") + externalPath.string());
        }
        externalData = std::move(externalResult.value());
        data = externalData.data();
        dataSize = externalData.size();
    }

    // 解压缩
    auto decompressResult = io::CompressionUtil::decompress(baseCompression, data, dataSize);
    if (decompressResult.failed()) {
        return Error(ErrorCode::DecompressionFailed,
                     std::string("Failed to decompress chunk: ") + decompressResult.error().message());
    }

    // 解析 NBT
    std::istringstream stream(std::string(
        reinterpret_cast<const char*>(decompressResult.value().data()),
        decompressResult.value().size()
    ));
    stream >> nbt::contexts::java;

    try {
        auto nbt = nbt::CompoundTag::read(stream);
        if (nbt == nullptr) {
            return Error(ErrorCode::ResourceParseError, "Failed to parse chunk NBT");
        }
        return nbt;
    } catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError,
                     std::string("Failed to parse chunk NBT: ") + e.what());
    }
}

Result<void>
RegionFile::writeChunk(u32 localX, u32 localZ, const nbt::CompoundTag& nbt) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed) {
        return Error(ErrorCode::InvalidState, "Region file is closed");
    }

    // 验证坐标
    if (localX >= CHUNKS_PER_SIDE || localZ >= CHUNKS_PER_SIDE) {
        return Error(ErrorCode::OutOfBounds, "Chunk coordinates out of range");
    }

    // 序列化 NBT
    std::ostringstream stream;
    stream << nbt::contexts::java << nbt;
    std::string nbtStr = stream.str();

    // 压缩（默认使用 Zlib）
    auto compressResult = io::CompressionUtil::zlibCompress(
        nbtStr.data(), nbtStr.size()
    );
    if (compressResult.failed()) {
        return Error(ErrorCode::CompressionFailed,
                     std::string("Failed to compress chunk: ") + compressResult.error().message());
    }

    const auto& compressedData = compressResult.value();

    // 构建原始数据：长度(4B) + 压缩类型(1B) + 压缩数据
    std::vector<u8> rawData(5 + compressedData.size());
    u32 length = static_cast<u32>(compressedData.size() + 1);  // 包含压缩类型
    rawData[0] = static_cast<u8>((length >> 24) & 0xFF);
    rawData[1] = static_cast<u8>((length >> 16) & 0xFF);
    rawData[2] = static_cast<u8>((length >> 8) & 0xFF);
    rawData[3] = static_cast<u8>(length & 0xFF);
    rawData[4] = CompressionTypeUtils::toByte(CompressionType::Zlib);
    std::copy(compressedData.begin(), compressedData.end(), rawData.begin() + 5);

    // 写入原始数据
    u32 index = getChunkIndex(localX, localZ);
    return writeRawChunk(index, rawData, CompressionType::Zlib);
}

bool RegionFile::hasChunk(u32 localX, u32 localZ) const {
    if (localX >= CHUNKS_PER_SIDE || localZ >= CHUNKS_PER_SIDE) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    u32 index = getChunkIndex(localX, localZ);
    return m_offsets[index] != 0;
}

Result<void> RegionFile::deleteChunk(u32 localX, u32 localZ) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed) {
        return Error(ErrorCode::InvalidState, "Region file is closed");
    }

    if (localX >= CHUNKS_PER_SIDE || localZ >= CHUNKS_PER_SIDE) {
        return Error(ErrorCode::OutOfBounds, "Chunk coordinates out of range");
    }

    u32 index = getChunkIndex(localX, localZ);
    u32 entry = m_offsets[index];

    if (entry != 0) {
        // 释放扇区
        u32 sectorOffset = getSectorOffset(entry);
        u32 sectorCount = getSectorCount(entry);
        m_bitmap.free(sectorOffset, sectorCount);

        // 清除偏移和时间戳
        m_offsets[index] = 0;
        m_timestamps[index] = 0;
        --m_usedChunks;

        // 更新头部
        return writeHeader();
    }

    return {};
}

// ========== 时间戳 ==========

u32 RegionFile::getTimestamp(u32 localX, u32 localZ) const {
    if (localX >= CHUNKS_PER_SIDE || localZ >= CHUNKS_PER_SIDE) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    u32 index = getChunkIndex(localX, localZ);
    return m_timestamps[index];
}

// ========== 同步 ==========

Result<void> RegionFile::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed || m_file == nullptr) {
        return {};
    }

    // 刷新文件缓冲区
    if (fflush(m_file) != 0) {
        return Error(ErrorCode::FileWriteFailed,
                     std::string("Failed to flush region file: ") + m_path.string());
    }

    return {};
}

void RegionFile::close() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed) {
        return;
    }

    if (m_file != nullptr) {
        fflush(m_file);
        fclose(m_file);
        m_file = nullptr;
    }

    m_closed = true;
}

// ========== 辅助方法 ==========

Result<std::optional<std::vector<u8>>>
RegionFile::readRawChunk(u32 index) {
    u32 entry = m_offsets[index];

    if (entry == 0) {
        return std::nullopt;
    }

    u32 sectorOffset = getSectorOffset(entry);
    u32 sectorCount = getSectorCount(entry);

    if (sectorOffset < 2) {
        return Error(ErrorCode::ResourceParseError, "Invalid sector offset in region file");
    }

    if (sectorCount == 0) {
        return std::nullopt;
    }

    // 定位到区块数据
    u64 fileOffset = static_cast<u64>(sectorOffset) * SECTOR_SIZE;
    if (fseek(m_file, static_cast<long>(fileOffset), SEEK_SET) != 0) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to seek to chunk data: ") + m_path.string());
    }

    // 读取数据
    size_t dataSize = static_cast<size_t>(sectorCount) * SECTOR_SIZE;
    std::vector<u8> data(dataSize);
    if (fread(data.data(), 1, dataSize, m_file) != dataSize) {
        return Error(ErrorCode::FileReadFailed,
                     std::string("Failed to read chunk data: ") + m_path.string());
    }

    return data;
}

Result<void>
RegionFile::writeRawChunk(u32 index, const std::vector<u8>& data, CompressionType compression) {
    // 计算需要的扇区数
    size_t dataSize = (data.size() + SECTOR_SIZE - 1) / SECTOR_SIZE * SECTOR_SIZE;
    u32 sectorsNeeded = static_cast<u32>(dataSize / SECTOR_SIZE);

    if (sectorsNeeded > 255) {
        // 区块太大，需要使用外部文件
        // TODO: 实现外部文件支持
        return Error(ErrorCode::CapacityExceeded, "Chunk data too large for region file");
    }

    // 获取旧的偏移
    u32 oldEntry = m_offsets[index];
    u32 oldSectorOffset = 0;
    u32 oldSectorCount = 0;
    if (oldEntry != 0) {
        oldSectorOffset = getSectorOffset(oldEntry);
        oldSectorCount = getSectorCount(oldEntry);
    }

    // 分配新扇区
    u32 newSectorOffset = m_bitmap.allocate(sectorsNeeded);
    if (newSectorOffset == 0) {
        return Error(ErrorCode::CapacityExceeded, "Failed to allocate sectors in region file");
    }

    // 扩展文件大小
    u64 newFileSize = static_cast<u64>(newSectorOffset + sectorsNeeded) * SECTOR_SIZE;
    if (fseek(m_file, 0, SEEK_END) != 0) {
        m_bitmap.free(newSectorOffset, sectorsNeeded);
        return Error(ErrorCode::FileWriteFailed, "Failed to extend region file");
    }

    // 写入数据
    u64 fileOffset = static_cast<u64>(newSectorOffset) * SECTOR_SIZE;
    if (fseek(m_file, static_cast<long>(fileOffset), SEEK_SET) != 0) {
        m_bitmap.free(newSectorOffset, sectorsNeeded);
        return Error(ErrorCode::FileWriteFailed, "Failed to seek to write position");
    }

    // 写入数据（填充到扇区大小）
    if (fwrite(data.data(), 1, data.size(), m_file) != data.size()) {
        m_bitmap.free(newSectorOffset, sectorsNeeded);
        return Error(ErrorCode::FileWriteFailed, "Failed to write chunk data");
    }

    // 填充剩余空间
    size_t padding = dataSize - data.size();
    if (padding > 0) {
        std::vector<u8> zero(padding, 0);
        if (fwrite(zero.data(), 1, padding, m_file) != padding) {
            m_bitmap.free(newSectorOffset, sectorsNeeded);
            return Error(ErrorCode::FileWriteFailed, "Failed to write padding");
        }
    }

    // 更新偏移表和时间戳
    m_offsets[index] = makeOffsetEntry(newSectorOffset, sectorsNeeded);
    m_timestamps[index] = static_cast<u32>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    // 更新使用计数
    if (oldEntry == 0) {
        ++m_usedChunks;
    }

    // 释放旧扇区
    if (oldEntry != 0 && oldSectorOffset >= 2) {
        m_bitmap.free(oldSectorOffset, oldSectorCount);
    }

    // 更新头部
    auto headerResult = writeHeader();
    if (headerResult.failed()) {
        return headerResult;
    }

    // 同步到磁盘
    if (m_sync) {
        fflush(m_file);
    }

    return {};
}

bool RegionFile::parseRegionCoords(const std::filesystem::path& path, i32& x, i32& z) {
    // 解析文件名: r.<x>.<z>.mca
    std::string filename = path.filename().string();

    // 检查前缀和后缀
    if (filename.substr(0, 2) != "r." ||
        filename.substr(filename.size() - 4) != ".mca") {
        x = 0;
        z = 0;
        return false;
    }

    // 解析中间部分
    std::string middle = filename.substr(2, filename.size() - 6);
    size_t dotPos = middle.find('.');
    if (dotPos == std::string::npos) {
        x = 0;
        z = 0;
        return false;
    }

    try {
        x = std::stoi(middle.substr(0, dotPos));
        z = std::stoi(middle.substr(dotPos + 1));
        return true;
    } catch (...) {
        x = 0;
        z = 0;
        return false;
    }
}

} // namespace mc::world::save::region
