#include "ExperiencePackets.hpp"
#include "PacketSerializer.hpp"
#include "../../entity/entities/player/Player.hpp"

namespace mc {
namespace network {

// ============================================================================
// SetExperiencePacket
// ============================================================================

SetExperiencePacket SetExperiencePacket::fromPlayer(const Player& player) {
    SetExperiencePacket packet;
    packet.m_level = player.experienceLevel();
    packet.m_progress = player.experienceProgress();
    packet.m_totalXp = player.totalExperience();
    return packet;
}

Result<std::vector<u8>> SetExperiencePacket::serialize() const {
    PacketSerializer serializer(expectedSize());

    serializer.writeF32(m_progress);
    serializer.writeVarInt(m_totalXp);
    serializer.writeVarInt(m_level);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> SetExperiencePacket::deserialize(const u8* data, size_t size) {
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidData, "SetExperiencePacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    // 读取进度
    auto progressResult = deserializer.readF32();
    if (!progressResult.success()) {
        return progressResult.error();
    }
    m_progress = progressResult.value();

    // 读取总经验
    auto totalXpResult = deserializer.readVarInt();
    if (!totalXpResult.success()) {
        return totalXpResult.error();
    }
    m_totalXp = totalXpResult.value();

    // 读取等级
    auto levelResult = deserializer.readVarInt();
    if (!levelResult.success()) {
        return levelResult.error();
    }
    m_level = levelResult.value();

    // 验证
    if (m_progress < 0.0f || m_progress > 1.0f) {
        return Error(ErrorCode::InvalidData, "Experience progress out of range");
    }
    if (m_level < 0) {
        return Error(ErrorCode::InvalidData, "Experience level cannot be negative");
    }
    if (m_totalXp < 0) {
        return Error(ErrorCode::InvalidData, "Total experience cannot be negative");
    }

    return {};
}

// ============================================================================
// SpawnExperienceOrbPacket
// ============================================================================

Result<std::vector<u8>> SpawnExperienceOrbPacket::serialize() const {
    PacketSerializer serializer(expectedSize());

    serializer.writeVarInt(m_entityId);
    serializer.writeF64(m_x);
    serializer.writeF64(m_y);
    serializer.writeF64(m_z);
    serializer.writeI16(m_xpValue);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> SpawnExperienceOrbPacket::deserialize(const u8* data, size_t size) {
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidData, "SpawnExperienceOrbPacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    // 读取实体ID
    auto idResult = deserializer.readVarInt();
    if (!idResult.success()) {
        return idResult.error();
    }
    m_entityId = idResult.value();

    // 读取位置
    auto xResult = deserializer.readF64();
    if (!xResult.success()) {
        return xResult.error();
    }
    m_x = xResult.value();

    auto yResult = deserializer.readF64();
    if (!yResult.success()) {
        return yResult.error();
    }
    m_y = yResult.value();

    auto zResult = deserializer.readF64();
    if (!zResult.success()) {
        return zResult.error();
    }
    m_z = zResult.value();

    // 读取经验值
    auto xpResult = deserializer.readI16();
    if (!xpResult.success()) {
        return xpResult.error();
    }
    m_xpValue = xpResult.value();

    // 验证
    if (m_xpValue < 1) {
        return Error(ErrorCode::InvalidData, "Experience orb value must be at least 1");
    }
    if (m_xpValue > 2477) {
        return Error(ErrorCode::InvalidData, "Experience orb value cannot exceed 2477");
    }

    return {};
}

} // namespace network
} // namespace mc
