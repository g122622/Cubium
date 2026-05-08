#include "GameProfile.hpp"
#include "SkinTypes.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace mc::skin {

// ============================================================================
// GameProfileProperty 实现
// ============================================================================

void GameProfileProperty::serialize(network::PacketSerializer& ser) const {
    ser.writeString(name);
    ser.writeString(value);

    if (signature.has_value()) {
        ser.writeBool(true);
        ser.writeString(*signature);
    } else {
        ser.writeBool(false);
    }
}

Result<GameProfileProperty> GameProfileProperty::deserialize(network::PacketDeserializer& deser) {
    GameProfileProperty prop;

    auto nameResult = deser.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    prop.name = nameResult.value();

    auto valueResult = deser.readString();
    if (valueResult.failed()) {
        return valueResult.error();
    }
    prop.value = valueResult.value();

    auto hasSigResult = deser.readBool();
    if (hasSigResult.failed()) {
        return hasSigResult.error();
    }

    if (hasSigResult.value()) {
        auto sigResult = deser.readString();
        if (sigResult.failed()) {
            return sigResult.error();
        }
        prop.signature = sigResult.value();
    }

    return prop;
}

// ============================================================================
// GameProfile 实现
// ============================================================================

GameProfile::GameProfile(const std::array<u8, 16>& uuid, const std::string& name)
    : m_uuid(uuid), m_name(name) {
}

void GameProfile::addProperty(const GameProfileProperty& property) {
    // 检查是否已存在同名属性
    auto it = std::find_if(m_properties.begin(), m_properties.end(),
        [&property](const GameProfileProperty& p) {
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

void GameProfile::addProperty(GameProfileProperty&& property) {
    // 检查是否已存在同名属性
    auto it = std::find_if(m_properties.begin(), m_properties.end(),
        [&property](const GameProfileProperty& p) {
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

const GameProfileProperty* GameProfile::getProperty(const std::string& name) const {
    auto it = std::find_if(m_properties.begin(), m_properties.end(),
        [&name](const GameProfileProperty& p) {
            return p.name == name;
        });

    return it != m_properties.end() ? &(*it) : nullptr;
}

bool GameProfile::hasTextures() const {
    return getProperty("textures") != nullptr;
}

const GameProfileProperty* GameProfile::getTexturesProperty() const {
    return getProperty("textures");
}

std::string GameProfile::uuidToString() const {
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

std::string GameProfile::uuidToStringNoDashes() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(m_uuid[i]);
    }

    return oss.str();
}

std::array<u8, 16> GameProfile::parseUUID(const std::string& str) {
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
        return uuid;  // 返回全零
    }

    // 解析十六进制
    for (size_t i = 0; i < 16; ++i) {
        std::string byteStr = cleanStr.substr(i * 2, 2);

        try {
            unsigned int byte = std::stoul(byteStr, nullptr, 16);
            uuid[i] = static_cast<u8>(byte);
        } catch (const std::exception& e) {
            spdlog::warn("GameProfile::parseUUID: Failed to parse byte at position {}: {}",
                         i, byteStr);
            return std::array<u8, 16>{};  // 返回全零
        }
    }

    return uuid;
}

i32 GameProfile::uuidHashCode() const {
    return calculateUUIDHashCode(m_uuid);
}

bool GameProfile::hasValidUUID() const {
    for (size_t i = 0; i < 16; ++i) {
        if (m_uuid[i] != 0) {
            return true;
        }
    }
    return false;
}

void GameProfile::serialize(network::PacketSerializer& ser) const {
    // UUID: 16字节
    for (size_t i = 0; i < 16; ++i) {
        ser.writeU8(m_uuid[i]);
    }

    // 名称: VarInt长度前缀字符串
    ser.writeString(m_name);

    // 属性数量: VarInt
    ser.writeVarInt(static_cast<i32>(m_properties.size()));

    // 属性列表
    for (const auto& prop : m_properties) {
        prop.serialize(ser);
    }
}

Result<GameProfile> GameProfile::deserialize(network::PacketDeserializer& deser) {
    GameProfile profile;

    // UUID: 16字节
    for (size_t i = 0; i < 16; ++i) {
        auto byteResult = deser.readU8();
        if (byteResult.failed()) {
            return byteResult.error();
        }
        profile.m_uuid[i] = byteResult.value();
    }

    // 名称: VarInt长度前缀字符串
    auto nameResult = deser.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    profile.m_name = nameResult.value();

    // 属性数量: VarInt
    auto countResult = deser.readVarInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 propCount = countResult.value();

    // 属性列表
    for (i32 i = 0; i < propCount; ++i) {
        auto propResult = GameProfileProperty::deserialize(deser);
        if (propResult.failed()) {
            return propResult.error();
        }
        profile.m_properties.push_back(propResult.value());
    }

    return profile;
}

} // namespace mc::skin
