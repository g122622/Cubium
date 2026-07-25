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

} // namespace mc::skin
