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

#include "JavaAnvilBackend.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/CompressionUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include "common/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "common/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include "common/world/storage/reader/java/JavaChunkReader.hpp"
#include "common/world/storage/reader/java/JavaColumnReader.hpp"
#include "common/world/storage/reader/java/JavaLevelDatReader.hpp"
#include "common/world/storage/reader/java/JavaWorldReader.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

using namespace reader::java;

JavaAnvilBackend::JavaAnvilBackend()
    : m_blockMapper(std::make_unique<JavaBlockStateMapper>())
    , m_biomeMapper(std::make_unique<JavaBiomeMapper>())
    , m_chunkReader(std::make_unique<JavaChunkReader>(*m_blockMapper, *m_biomeMapper))
    , m_columnReader(std::make_unique<JavaColumnReader>(*m_chunkReader))
    , m_worldReader(std::make_unique<JavaWorldReader>(*m_columnReader))
{}

JavaAnvilBackend::~JavaAnvilBackend()
{
    close();
}

JavaAnvilBackend::JavaAnvilBackend(JavaAnvilBackend&& other) noexcept
    : m_worldPath(std::move(other.m_worldPath))
    , m_formatInfo(std::move(other.m_formatInfo))
    , m_isOpen(other.m_isOpen)
    , m_blockMapper(std::move(other.m_blockMapper))
    , m_biomeMapper(std::move(other.m_biomeMapper))
    , m_chunkReader(std::move(other.m_chunkReader))
    , m_columnReader(std::move(other.m_columnReader))
    , m_worldReader(std::move(other.m_worldReader))
{
    other.m_isOpen = false;
}

JavaAnvilBackend& JavaAnvilBackend::operator=(JavaAnvilBackend&& other) noexcept
{
    if (this != &other) {
        close();
        m_worldPath = std::move(other.m_worldPath);
        m_formatInfo = std::move(other.m_formatInfo);
        m_isOpen = other.m_isOpen;
        m_blockMapper = std::move(other.m_blockMapper);
        m_biomeMapper = std::move(other.m_biomeMapper);
        m_chunkReader = std::move(other.m_chunkReader);
        m_columnReader = std::move(other.m_columnReader);
        m_worldReader = std::move(other.m_worldReader);
        other.m_isOpen = false;
    }
    return *this;
}

Result<void> JavaAnvilBackend::open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo)
{
    m_worldPath = worldPath;
    m_formatInfo = formatInfo;

    if (m_formatInfo.format != SaveFormat::JavaAnvil) {
        return Error(ErrorCode::InvalidState, "JavaAnvilBackend can only open Java Anvil format worlds");
    }

    auto openResult = m_worldReader->open(worldPath, formatInfo);
    if (openResult.failed()) {
        return openResult.error();
    }
    spdlog::info("JavaAnvilBackend: Opened Java world at {} ({})", worldPath.string(), m_formatInfo.formatName);
    m_isOpen = true;
    return {};
}

void JavaAnvilBackend::close()
{
    m_worldReader->close();
    m_isOpen = false;
    spdlog::info("JavaAnvilBackend: Closed");
}

bool JavaAnvilBackend::isOpen() const
{
    return m_isOpen;
}

Result<std::optional<ChunkData>> JavaAnvilBackend::loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    return m_worldReader->readChunk(x, z, dimension);
}

Result<std::vector<ChunkPos>> JavaAnvilBackend::listChunks(DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    return m_worldReader->listChunks(dimension);
}

Result<std::optional<PlayerSaveData>> JavaAnvilBackend::loadPlayer(const std::string& uuid)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    if (uuid == "~local_player" || uuid.empty()) {
        return JavaLevelDatReader::readLocalPlayer(m_worldPath);
    }

    const std::filesystem::path playerPath = m_worldPath / "playerdata" / fmt::format("{}.dat", uuid);
    std::error_code ec;
    if (!std::filesystem::exists(playerPath, ec)) {
        return std::optional<PlayerSaveData>{};
    }

    std::ifstream file(playerPath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Cannot open player data file: {}", playerPath.string()));
    }

    std::vector<u8> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (compressed.empty()) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Player data file is empty: {}", playerPath.string()));
    }

    auto decompressed = mc::util::decompressGzip(compressed);
    if (decompressed.empty()) {
        return Error(
            ErrorCode::DecompressionFailed, fmt::format("Failed to decompress player data: {}", playerPath.string()));
    }

    std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
    stream >> mc::nbt::contexts::java;
    auto root = mc::nbt::tags::compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to parse player NBT: {}", playerPath.string()));
    }

    auto playerResult = PlayerSaveData::fromNbt(*root);
    if (playerResult.failed()) {
        return playerResult.error();
    }

    auto playerData = playerResult.value();
    if (playerData.uuid.empty()) {
        playerData.uuid = uuid;
    }
    return std::optional<PlayerSaveData>(std::move(playerData));
}

Result<std::vector<std::string>> JavaAnvilBackend::listPlayerUuids()
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    std::vector<std::string> uuids;
    auto localPlayerResult = JavaLevelDatReader::readLocalPlayer(m_worldPath);
    if (localPlayerResult.failed()) {
        return localPlayerResult.error();
    }
    if (localPlayerResult.value().has_value()) {
        uuids.emplace_back("~local_player");
    }

    const std::filesystem::path playerDir = m_worldPath / "playerdata";
    std::error_code ec;
    if (!std::filesystem::exists(playerDir, ec)) {
        return uuids;
    }

    for (const auto& entry : std::filesystem::directory_iterator(playerDir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".dat") {
            continue;
        }
        uuids.push_back(entry.path().stem().string());
    }

    return uuids;
}

Result<LevelRuntimeData> JavaAnvilBackend::loadLevelData()
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }
    return JavaLevelDatReader::readRuntimeData(m_worldPath);
}

} // namespace mc::world::storage
