/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "BedrockLDBBackend.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include "common/world/storage/player/PlayerSaveData.hpp"
#include "common/world/storage/reader/bedrock/BedrockBiomeMapper.hpp"
#include "common/world/storage/reader/bedrock/BedrockChunkReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockColumnReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDatReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDb.hpp"
#include "common/world/storage/reader/bedrock/BedrockWorldReader.hpp"
#include "common/world/storage/reader/bedrock/LevelDBKey.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

using namespace reader::bedrock;
using namespace mc::nbt;
using namespace mc::nbt::tags;

namespace {

const compound_tag* tryGetCompound(const compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const compound_tag*>(it->second.get());
}

const list_tag* tryGetList(const compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const list_tag*>(it->second.get());
}

std::optional<std::string> tryGetString(const compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }
    auto* value = dynamic_cast<const string_tag*>(it->second.get());
    if (!value) {
        return std::nullopt;
    }
    return value->value;
}

std::optional<i32> tryGetInt(const compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }
    if (auto* value = dynamic_cast<const int_tag*>(it->second.get())) {
        return value->value;
    }
    if (auto* value = dynamic_cast<const short_tag*>(it->second.get())) {
        return static_cast<i32>(value->value);
    }
    if (auto* value = dynamic_cast<const byte_tag*>(it->second.get())) {
        return static_cast<i32>(value->value);
    }
    return std::nullopt;
}

std::optional<f32> tryGetFloat(const compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }
    if (auto* value = dynamic_cast<const float_tag*>(it->second.get())) {
        return value->value;
    }
    if (auto* value = dynamic_cast<const double_tag*>(it->second.get())) {
        return static_cast<f32>(value->value);
    }
    return std::nullopt;
}

std::optional<bool> tryGetBool(const compound_tag& tag, const std::string& key)
{
    auto value = tryGetInt(tag, key);
    if (!value.has_value()) {
        return std::nullopt;
    }
    return *value != 0;
}

Result<PlayerSaveData> parseBedrockPlayerData(const compound_tag& root, const std::string& playerId)
{
    PlayerSaveData data;
    data.uuid = playerId;
    data.username = tryGetString(root, "NameTag").value_or(playerId);

    if (const auto* posList = tryGetList(root, "Pos")) {
        if (posList->element_id() == TagId::Float) {
            const auto& values = dynamic_cast<const float_list_tag&>(*posList).value;
            if (values.size() >= 3) {
                data.posX = values[0];
                data.posY = values[1];
                data.posZ = values[2];
            }
        } else if (posList->element_id() == TagId::Double) {
            const auto& values = dynamic_cast<const double_list_tag&>(*posList).value;
            if (values.size() >= 3) {
                data.posX = values[0];
                data.posY = values[1];
                data.posZ = values[2];
            }
        }
    }

    if (const auto* rotationList = tryGetList(root, "Rotation")) {
        if (rotationList->element_id() == TagId::Float) {
            const auto& values = dynamic_cast<const float_list_tag&>(*rotationList).value;
            if (values.size() >= 2) {
                data.yaw = values[0];
                data.pitch = values[1];
            }
        }
    }

    data.dimension = tryGetInt(root, "DimensionId").value_or(0);
    data.gameMode = static_cast<GameMode>(tryGetInt(root, "PlayerGameMode").value_or(0));
    data.health = tryGetFloat(root, "Health").value_or(data.health);
    data.foodLevel = tryGetInt(root, "foodLevel").value_or(data.foodLevel);
    data.experienceLevel = tryGetInt(root, "XpLevel").value_or(data.experienceLevel);
    data.totalExperience = tryGetInt(root, "XpTotal").value_or(data.totalExperience);
    data.experienceProgress = tryGetFloat(root, "XpP").value_or(data.experienceProgress);
    data.onGround = tryGetBool(root, "OnGround").value_or(data.onGround);

    if (const auto* abilities = tryGetCompound(root, "abilities")) {
        data.invulnerable = tryGetBool(*abilities, "invulnerable").value_or(data.invulnerable);
        data.canFly = tryGetBool(*abilities, "mayfly").value_or(data.canFly);
        data.flying = tryGetBool(*abilities, "flying").value_or(data.flying);
        data.flySpeed = tryGetFloat(*abilities, "flySpeed").value_or(data.flySpeed);
        data.walkSpeed = tryGetFloat(*abilities, "walkSpeed").value_or(data.walkSpeed);
    }

    if (const auto* invList = tryGetList(root, "Inventory")) {
        if (invList->element_id() == TagId::Compound) {
            const auto& values = dynamic_cast<const compound_list_tag&>(*invList).value;
            data.inventoryItems.resize(41);
            for (const auto& itemTag : values) {
                const auto slotOpt = tryGetInt(itemTag, "Slot");
                if (!slotOpt.has_value()) {
                    continue;
                }
                const i32 slot = *slotOpt;
                if (slot < 0 || slot >= 41) {
                    continue;
                }

                auto stackResult = ItemStack::fromNbt(itemTag);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    data.inventoryItems[slot] = std::move(stackResult.value());
                }
            }
        }
    }

    if (const auto* armorList = tryGetList(root, "Armor")) {
        if (armorList->element_id() == TagId::Compound) {
            const auto& values = dynamic_cast<const compound_list_tag&>(*armorList).value;
            if (data.inventoryItems.size() < 41) {
                data.inventoryItems.resize(41);
            }
            for (const auto& itemTag : values) {
                const i32 rawSlot = tryGetInt(itemTag, "Slot").value_or(0);
                const i32 slot = 36 + (3 - rawSlot);
                if (slot < 36 || slot >= 40) {
                    continue;
                }

                auto stackResult = ItemStack::fromNbt(itemTag);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    data.inventoryItems[slot] = std::move(stackResult.value());
                }
            }
        }
    }

    if (const auto* offhandList = tryGetList(root, "Offhand")) {
        if (offhandList->element_id() == TagId::Compound) {
            const auto& values = dynamic_cast<const compound_list_tag&>(*offhandList).value;
            if (data.inventoryItems.size() < 41) {
                data.inventoryItems.resize(41);
            }
            for (const auto& itemTag : values) {
                auto stackResult = ItemStack::fromNbt(itemTag);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    data.inventoryItems[40] = std::move(stackResult.value());
                    break;
                }
            }
        }
    }

    if (const auto spawnX = tryGetInt(root, "SpawnX");
        spawnX.has_value() && tryGetInt(root, "SpawnY").has_value() && tryGetInt(root, "SpawnZ").has_value()) {
        data.spawnPoint =
            GlobalPos(data.dimension, BlockPos(*spawnX, *tryGetInt(root, "SpawnY"), *tryGetInt(root, "SpawnZ")));
    }

    return data;
}

} // namespace

BedrockLDBBackend::BedrockLDBBackend()
    : m_db(std::make_unique<BedrockLevelDb>())
    , m_biomeMapper(std::make_unique<BedrockBiomeMapper>())
    , m_chunkReader(std::make_unique<BedrockChunkReader>(*m_biomeMapper))
    , m_columnReader(std::make_unique<BedrockColumnReader>(*m_chunkReader))
    , m_worldReader(std::make_unique<BedrockWorldReader>(*m_columnReader))
{}

BedrockLDBBackend::~BedrockLDBBackend()
{
    close();
}

Result<void> BedrockLDBBackend::open(const std::filesystem::path& worldPath, const SaveFormatInfo& formatInfo)
{
    m_worldPath = worldPath;
    m_formatInfo = formatInfo;

    if (m_formatInfo.format != SaveFormat::BedrockLDB) {
        return Error(ErrorCode::InvalidState, "BedrockLDBBackend can only open Bedrock LevelDB format worlds");
    }

    // 打开 LevelDB 数据库
    std::filesystem::path dbPath = worldPath / "db";
    auto dbResult = m_db->open(dbPath);
    if (dbResult.failed()) {
        return dbResult.error();
    }

    spdlog::info("BedrockLDBBackend: Opened Bedrock world at {} ({})", worldPath.string(), m_formatInfo.formatName);
    m_isOpen = true;
    return {};
}

void BedrockLDBBackend::close()
{
    if (m_db) {
        m_db->close();
    }
    m_isOpen = false;
    spdlog::info("BedrockLDBBackend: Closed");
}

bool BedrockLDBBackend::isOpen() const
{
    return m_isOpen;
}

Result<std::optional<ChunkData>> BedrockLDBBackend::loadChunk(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    return m_worldReader->readChunk(x, z, dimension, *m_db);
}

Result<std::vector<ChunkPos>> BedrockLDBBackend::listChunks(DimensionId dimension)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    return m_worldReader->listChunks(dimension, *m_db);
}

Result<std::optional<PlayerSaveData>> BedrockLDBBackend::loadPlayer(const std::string& uuid)
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    std::vector<u8> key;
    if (uuid == "~local_player" || uuid.empty()) {
        key = LevelDBKey::localPlayer();
    } else if (uuid.starts_with("actorprefix")) {
        key.assign(uuid.begin(), uuid.end());
    } else {
        key = LevelDBKey::actorPrefix();
        key.insert(key.end(), uuid.begin(), uuid.end());
    }

    const auto playerResult = m_db->get(key);
    if (playerResult.failed()) {
        return playerResult.error();
    }
    if (!playerResult.value().has_value()) {
        return std::optional<PlayerSaveData>{};
    }

    const auto& bytes = playerResult.value().value();
    std::istringstream stream(std::string(bytes.begin(), bytes.end()));
    stream >> contexts::bedrock_disk;
    auto root = compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::FileCorrupted, "Failed to parse Bedrock player NBT");
    }

    auto parseResult = parseBedrockPlayerData(*root, key.empty() ? uuid : std::string(key.begin(), key.end()));
    if (parseResult.failed()) {
        return parseResult.error();
    }

    return std::optional<PlayerSaveData>(std::move(parseResult.value()));
}

Result<std::vector<std::string>> BedrockLDBBackend::listPlayerUuids()
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }

    std::vector<std::string> ids;
    ids.emplace_back("~local_player");

    auto iterateResult = m_db->iteratePrefix(
        LevelDBKey::actorPrefix(), [&ids](const std::vector<u8>& key, const std::vector<u8>& value) -> bool {
            MC_UNUSED(value);
            const auto& prefix = LevelDBKey::actorPrefix();
            if (key.size() >= prefix.size()) {
                ids.emplace_back(key.begin() + static_cast<std::ptrdiff_t>(prefix.size()), key.end());
            }
            return true;
        });
    if (iterateResult.failed()) {
        return iterateResult.error();
    }

    return ids;
}

Result<LevelRuntimeData> BedrockLDBBackend::loadLevelData()
{
    if (!m_isOpen) {
        return Error(ErrorCode::InvalidState, "Backend not open");
    }
    return BedrockLevelDatReader::readRuntimeData(m_worldPath);
}

} // namespace mc::world::storage
