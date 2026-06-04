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

#include "SkinPackets.hpp"
#include "util/text/ITextComponent.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc::skin {

// ============================================================================
// PlayerListEntry 实现
// ============================================================================

PlayerListEntry PlayerListEntry::createAdd(const GameProfile& profile, GameMode gameMode, i32 ping) noexcept
{
    PlayerListEntry entry;
    entry.uuid = profile.uuid();
    entry.name = profile.name();
    entry.properties = profile.properties();
    entry.gameMode = gameMode;
    entry.ping = ping;
    return entry;
}

PlayerListEntry PlayerListEntry::createRemove(const std::array<u8, 16>& uuid) noexcept
{
    PlayerListEntry entry;
    entry.uuid = uuid;
    return entry;
}

PlayerListEntry PlayerListEntry::createUpdateLatency(const std::array<u8, 16>& uuid, i32 ping) noexcept
{
    PlayerListEntry entry;
    entry.uuid = uuid;
    entry.ping = ping;
    return entry;
}

PlayerListEntry PlayerListEntry::createUpdateGameMode(const std::array<u8, 16>& uuid, GameMode gameMode) noexcept
{
    PlayerListEntry entry;
    entry.uuid = uuid;
    entry.gameMode = gameMode;
    return entry;
}

PlayerListEntry PlayerListEntry::createUpdateDisplayName(
    const std::array<u8, 16>& uuid, const std::optional<std::string>& displayName) noexcept
{
    PlayerListEntry entry;
    entry.uuid = uuid;
    entry.displayName = displayName;
    return entry;
}

std::string PlayerListEntry::serializeText(const text::ITextComponent& text)
{
    return text.toJson().dump();
}

void PlayerListEntry::setDisplayName(const text::ITextComponent& text)
{
    displayName = serializeText(text);
}

std::unique_ptr<text::ITextComponent> PlayerListEntry::getDisplayNameAsText() const
{
    if (!displayName.has_value() || displayName->empty()) {
        return nullptr;
    }

    try {
        nlohmann::json json = nlohmann::json::parse(*displayName);
        return text::ITextComponent::fromJson(json);
    }
    catch (const nlohmann::json::exception& e) {
        spdlog::warn("PlayerListEntry: Failed to parse displayName JSON: {}", e.what());
        return nullptr;
    }
}

void PlayerListEntry::serialize(network::PacketSerializer& ser, PlayerListAction action) const
{
    // UUID: 16 bytes
    for (size_t i = 0; i < 16; ++i) {
        ser.writeU8(uuid[i]);
    }

    switch (action) {
        case PlayerListAction::AddPlayer: {
            // Name: std::string (max 16)
            ser.writeString(name);

            // Properties count: VarInt
            ser.writeVarInt(static_cast<i32>(properties.size()));

            // Properties
            for (const auto& prop : properties) {
                prop.serialize(ser);
            }

            // GameMode: VarInt
            ser.writeVarInt(static_cast<i32>(gameMode));

            // Ping: VarInt
            ser.writeVarInt(ping);

            // HasDisplayName: Boolean
            ser.writeBool(displayName.has_value());

            // DisplayName?: Chat (optional)
            // 格式：JSON序列化的ITextComponent，例如 {"text":"PlayerName","color":"red"}
            if (displayName.has_value()) {
                ser.writeString(*displayName);
            }
            break;
        }

        case PlayerListAction::UpdateGameMode: {
            // GameMode: VarInt
            ser.writeVarInt(static_cast<i32>(gameMode));
            break;
        }

        case PlayerListAction::UpdateLatency: {
            // Ping: VarInt
            ser.writeVarInt(ping);
            break;
        }

        case PlayerListAction::UpdateDisplayName: {
            // HasDisplayName: Boolean
            ser.writeBool(displayName.has_value());

            // DisplayName?: Chat (optional)
            // 格式：JSON序列化的ITextComponent
            if (displayName.has_value()) {
                ser.writeString(*displayName);
            }
            break;
        }

        case PlayerListAction::RemovePlayer: {
            // No additional data
            break;
        }
    }
}

Result<PlayerListEntry> PlayerListEntry::deserialize(network::PacketDeserializer& deser, PlayerListAction action)
{
    PlayerListEntry entry;

    // UUID: 16 bytes
    for (size_t i = 0; i < 16; ++i) {
        auto byteResult = deser.readU8();
        if (byteResult.failed()) {
            return byteResult.error();
        }
        entry.uuid[i] = byteResult.value();
    }

    switch (action) {
        case PlayerListAction::AddPlayer: {
            // Name: std::string (max 16)
            auto nameResult = deser.readString();
            if (nameResult.failed()) {
                return nameResult.error();
            }
            entry.name = nameResult.value();

            // Properties count: VarInt
            auto countResult = deser.readVarInt();
            if (countResult.failed()) {
                return countResult.error();
            }
            i32 propCount = countResult.value();

            // Properties
            for (i32 i = 0; i < propCount; ++i) {
                auto propResult = GameProfileProperty::deserialize(deser);
                if (propResult.failed()) {
                    return propResult.error();
                }
                entry.properties.push_back(propResult.value());
            }

            // GameMode: VarInt
            auto gameModeResult = deser.readVarInt();
            if (gameModeResult.failed()) {
                return gameModeResult.error();
            }
            entry.gameMode = static_cast<GameMode>(gameModeResult.value());

            // Ping: VarInt
            auto pingResult = deser.readVarInt();
            if (pingResult.failed()) {
                return pingResult.error();
            }
            entry.ping = pingResult.value();

            // HasDisplayName: Boolean
            auto hasNameResult = deser.readBool();
            if (hasNameResult.failed()) {
                return hasNameResult.error();
            }

            if (hasNameResult.value()) {
                // DisplayName: Chat
                // 格式：JSON序列化的ITextComponent
                auto displayNameResult = deser.readString();
                if (displayNameResult.failed()) {
                    return displayNameResult.error();
                }
                entry.displayName = displayNameResult.value();
            }
            break;
        }

        case PlayerListAction::UpdateGameMode: {
            // GameMode: VarInt
            auto gameModeResult = deser.readVarInt();
            if (gameModeResult.failed()) {
                return gameModeResult.error();
            }
            entry.gameMode = static_cast<GameMode>(gameModeResult.value());
            break;
        }

        case PlayerListAction::UpdateLatency: {
            // Ping: VarInt
            auto pingResult = deser.readVarInt();
            if (pingResult.failed()) {
                return pingResult.error();
            }
            entry.ping = pingResult.value();
            break;
        }

        case PlayerListAction::UpdateDisplayName: {
            // HasDisplayName: Boolean
            auto hasNameResult = deser.readBool();
            if (hasNameResult.failed()) {
                return hasNameResult.error();
            }

            if (hasNameResult.value()) {
                // DisplayName: Chat
                // 格式：JSON序列化的ITextComponent
                auto displayNameResult = deser.readString();
                if (displayNameResult.failed()) {
                    return displayNameResult.error();
                }
                entry.displayName = displayNameResult.value();
            }
            break;
        }

        case PlayerListAction::RemovePlayer: {
            // No additional data
            break;
        }
    }

    return entry;
}

// ============================================================================
// PlayerListItemPacket 实现
// ============================================================================

PlayerListItemPacket::PlayerListItemPacket() noexcept
    : Packet(network::PacketType::PlayerListItem)
{}

PlayerListItemPacket::PlayerListItemPacket(PlayerListAction action) noexcept
    : Packet(network::PacketType::PlayerListItem)
    , m_action(action)
{}

Result<std::vector<u8>> PlayerListItemPacket::serialize() const
{
    network::PacketSerializer ser;

    // Action: VarInt
    ser.writeVarInt(static_cast<i32>(m_action));

    // Count: VarInt
    ser.writeVarInt(static_cast<i32>(m_entries.size()));

    // Entries
    for (const auto& entry : m_entries) {
        entry.serialize(ser, m_action);
    }

    return ser.buffer();
}

Result<void> PlayerListItemPacket::deserialize(const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);

    // Action: VarInt
    auto actionResult = deser.readVarInt();
    if (actionResult.failed()) {
        return actionResult.error();
    }
    m_action = static_cast<PlayerListAction>(actionResult.value());

    // Count: VarInt
    auto countResult = deser.readVarInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 entryCount = countResult.value();

    // Entries
    m_entries.clear();
    m_entries.reserve(static_cast<size_t>(entryCount));

    for (i32 i = 0; i < entryCount; ++i) {
        auto entryResult = PlayerListEntry::deserialize(deser, m_action);
        if (entryResult.failed()) {
            return entryResult.error();
        }
        m_entries.push_back(entryResult.value());
    }

    return {};
}

} // namespace mc::skin
