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

#include "GameProfile.hpp"
#include "SkinTypes.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc::skin {

// ============================================================================
// GameProfile 实现
// ============================================================================

GameProfile::GameProfile(const std::array<u8, 16>& uuid, const std::string& name)
    : m_uuid(uuid)
    , m_name(name)
{}

void GameProfile::addProperty(const GameProfileProperty& property)
{
    // 检查是否已存在同名属性
    auto it = std::find_if(m_properties.begin(), m_properties.end(), [&property](const GameProfileProperty& p) {
        return p.name == property.name;
    });

    if (it != m_properties.end()) {
        // 替换现有属性
        *it = property;
    } else {
        // 添加新属性
        m_properties.push_back(property);
    }
}

void GameProfile::addProperty(GameProfileProperty&& property)
{
    // 检查是否已存在同名属性
    auto it = std::find_if(m_properties.begin(), m_properties.end(), [&property](const GameProfileProperty& p) {
        return p.name == property.name;
    });

    if (it != m_properties.end()) {
        // 替换现有属性
        *it = std::move(property);
    } else {
        // 添加新属性
        m_properties.push_back(std::move(property));
    }
}

const GameProfileProperty* GameProfile::getProperty(const std::string& name) const
{
    auto it = std::find_if(
        m_properties.begin(), m_properties.end(), [&name](const GameProfileProperty& p) { return p.name == name; });

    return it != m_properties.end() ? &(*it) : nullptr;
}

bool GameProfile::hasTextures() const noexcept
{
    return getProperty("textures") != nullptr;
}

const GameProfileProperty* GameProfile::getTexturesProperty() const
{
    return getProperty("textures");
}

std::string GameProfile::uuidToString() const
{
    // 格式: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    // 第1段: 8字节 (0-7)
    for (size_t i = 0; i < 4; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }
    oss << '-';

    // 第2段: 4字节 (4-5)
    for (size_t i = 4; i < 6; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }
    oss << '-';

    // 第3段: 4字节 (6-7)
    for (size_t i = 6; i < 8; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }
    oss << '-';

    // 第4段: 4字节 (8-9)
    for (size_t i = 8; i < 10; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }
    oss << '-';

    // 第5段: 12字节 (10-15)
    for (size_t i = 10; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }

    return oss.str();
}

std::string GameProfile::uuidToStringNoDashes() const
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }

    return oss.str();
}

std::array<u8, 16> GameProfile::parseUUID(const std::string& str)
{
    std::array<u8, 16> uuid = {};

    // 移除连字符
    std::string cleanStr;
    cleanStr.reserve(32);
    for (char c : str) {
        if (c != '-') {
            cleanStr.push_back(c);
        }
    }

    // 检查长度
    if (cleanStr.length() != 32) {
        spdlog::warn("GameProfile::parseUUID: Invalid UUID length: {}", cleanStr.length());
        return uuid; // 返回全零
    }

    // 解析十六进制
    for (size_t i = 0; i < 16; ++i) {
        std::string byteStr = cleanStr.substr(i * 2, 2);

        try {
            unsigned int byte = std::stoul(byteStr, nullptr, 16);
            uuid[i] = static_cast<u8>(byte);
        }
        catch (const std::exception& e) {
            spdlog::warn("GameProfile::parseUUID: Failed to parse byte at position {}: {}", i, byteStr);
            return std::array<u8, 16>{}; // 返回全零
        }
    }

    return uuid;
}

i32 GameProfile::uuidHashCode() const noexcept
{
    return calculateUUIDHashCode(m_uuid);
}

bool GameProfile::hasValidUUID() const noexcept
{
    return std::any_of(m_uuid.begin(), m_uuid.end(), [](u8 byte) { return byte != 0; });
}

// ============================================================================
// JSON 序列化
// ============================================================================

nlohmann::json GameProfile::toJson() const
{
    nlohmann::json json = nlohmann::json::object();

    // 写入玩家名称
    if (!m_name.empty()) {
        json["Name"] = m_name;
    }

    // 写入 UUID（带连字符格式）
    if (hasValidUUID()) {
        json["Id"] = uuidToString();
    }

    // 写入属性（如皮肤纹理）
    if (!m_properties.empty()) {
        nlohmann::json propertiesJson = nlohmann::json::object();

        for (const auto& prop : m_properties) {
            nlohmann::json propArray = nlohmann::json::array();

            nlohmann::json propEntry = nlohmann::json::object();
            propEntry["Value"] = prop.value;
            if (prop.signature.has_value()) {
                propEntry["Signature"] = *prop.signature;
            }
            propArray.push_back(std::move(propEntry));

            propertiesJson[prop.name] = std::move(propArray);
        }

        json["Properties"] = std::move(propertiesJson);
    }

    return json;
}

Result<GameProfile> GameProfile::fromJson(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "GameProfile JSON must be an object");
    }

    GameProfile profile;

    // 读取名称
    if (json.contains("Name") && json["Name"].is_string()) {
        profile.m_name = json["Name"].get<std::string>();
    }

    // 读取 UUID
    if (json.contains("Id")) {
        if (json["Id"].is_string()) {
            // 字符串格式 UUID（带或不带连字符）
            std::string uuidStr = json["Id"].get<std::string>();
            profile.m_uuid = parseUUID(uuidStr);
        } else if (json["Id"].is_array()) {
            // IntArray 格式 UUID（MC NBT 格式：4 个 int）
            auto arr = json["Id"];
            if (arr.size() == 4) {
                // 将 4 个 int 转换为 16 字节 UUID（大端序）
                for (size_t i = 0; i < 4; ++i) {
                    i32 val = arr[i].get<i32>();
                    // 大端序：高位在前
                    profile.m_uuid[i * 4] = static_cast<u8>((val >> 24) & 0xFF);
                    profile.m_uuid[i * 4 + 1] = static_cast<u8>((val >> 16) & 0xFF);
                    profile.m_uuid[i * 4 + 2] = static_cast<u8>((val >> 8) & 0xFF);
                    profile.m_uuid[i * 4 + 3] = static_cast<u8>(val & 0xFF);
                }
            }
        }
    }

    // 读取属性
    if (json.contains("Properties") && json["Properties"].is_object()) {
        const auto& props = json["Properties"];
        for (auto it = props.begin(); it != props.end(); ++it) {
            const std::string& propName = it.key();
            const auto& propArray = it.value();

            if (propArray.is_array()) {
                for (const auto& propEntry : propArray) {
                    if (propEntry.is_object()) {
                        GameProfileProperty prop;
                        prop.name = propName;

                        if (propEntry.contains("Value") && propEntry["Value"].is_string()) {
                            prop.value = propEntry["Value"].get<std::string>();
                        }

                        if (propEntry.contains("Signature") && propEntry["Signature"].is_string()) {
                            prop.signature = propEntry["Signature"].get<std::string>();
                        }

                        profile.m_properties.push_back(std::move(prop));
                    }
                }
            }
        }
    }

    return profile;
}

} // namespace mc::skin
