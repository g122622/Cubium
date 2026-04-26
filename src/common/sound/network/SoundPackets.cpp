#include "common/sound/network/SoundPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"

namespace mc::sound {

// ============================================================================
// PlaySoundPacket
// ============================================================================

PlaySoundPacket::PlaySoundPacket()
    : network::Packet(network::PacketType::PlaySound)
{
}

PlaySoundPacket::PlaySoundPacket(const ResourceLocation& soundEventId,
                                 SoundCategory category,
                                 const glm::vec3& position,
                                 f32 volume,
                                 f32 pitch)
    : network::Packet(network::PacketType::PlaySound)
    , m_soundEventId(soundEventId)
    , m_category(category)
    , m_x(static_cast<i32>(position.x * 8.0f))
    , m_y(static_cast<i32>(position.y * 8.0f))
    , m_z(static_cast<i32>(position.z * 8.0f))
    , m_volume(volume)
    , m_pitch(pitch)
{
}

glm::vec3 PlaySoundPacket::getPosition() const noexcept {
    return glm::vec3(
        static_cast<f32>(m_x) / 8.0f,
        static_cast<f32>(m_y) / 8.0f,
        static_cast<f32>(m_z) / 8.0f
    );
}

size_t PlaySoundPacket::expectedSize() const {
    // 包体大小预估：
    // 声音事件ID字符串 + category(varint) + x/y/z(i32*3) + volume/pitch(f32*2)
    return 64;
}

Result<std::vector<u8>> PlaySoundPacket::serialize() const {
    network::PacketSerializer serializer;
    serializer.reserve(expectedSize());

    // 写入声音事件ID（字符串格式：namespace:path）
    serializer.writeString(m_soundEventId.toString());

    // 写入类别（VarInt）
    serializer.writeVarInt(static_cast<i32>(m_category));

    // 写入位置（定点整数）
    serializer.writeI32(m_x);
    serializer.writeI32(m_y);
    serializer.writeI32(m_z);

    // 写入音量和音调
    serializer.writeF32(m_volume);
    serializer.writeF32(m_pitch);

    // 仅返回包体。外层网络头由 ConnectionManager::encapsulatePacket 写入。
    return serializer.buffer();
}

Result<void> PlaySoundPacket::deserialize(const u8* data, size_t size) {
    network::PacketDeserializer deserializer(data, size);

    // 读取声音事件ID
    auto idResult = deserializer.readString();
    if (!idResult.success()) {
        return idResult.error();
    }
    m_soundEventId = ResourceLocation::parse(idResult.value());

    // 读取类别
    auto categoryResult = deserializer.readVarInt();
    if (!categoryResult.success()) {
        return categoryResult.error();
    }
    m_category = static_cast<SoundCategory>(categoryResult.value());
    if (!isValidSoundCategory(m_category)) {
        m_category = SoundCategory::Master;
    }

    // 读取位置
    auto xResult = deserializer.readI32();
    if (!xResult.success()) {
        return xResult.error();
    }
    m_x = xResult.value();

    auto yResult = deserializer.readI32();
    if (!yResult.success()) {
        return yResult.error();
    }
    m_y = yResult.value();

    auto zResult = deserializer.readI32();
    if (!zResult.success()) {
        return zResult.error();
    }
    m_z = zResult.value();

    // 读取音量和音调
    auto volumeResult = deserializer.readF32();
    if (!volumeResult.success()) {
        return volumeResult.error();
    }
    m_volume = volumeResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) {
        return pitchResult.error();
    }
    m_pitch = pitchResult.value();

    return {};
}

// ============================================================================
// StopSoundPacket
// ============================================================================

StopSoundPacket::StopSoundPacket()
    : network::Packet(network::PacketType::StopSound)
    , m_soundEventId(std::nullopt)
    , m_category(std::nullopt)
{
}

StopSoundPacket::StopSoundPacket(const std::optional<ResourceLocation>& soundEventId,
                                  const std::optional<SoundCategory>& category)
    : network::Packet(network::PacketType::StopSound)
    , m_soundEventId(soundEventId)
    , m_category(category)
{
}

StopSoundPacket::StopSoundPacket(const ResourceLocation& soundEventId)
    : network::Packet(network::PacketType::StopSound)
    , m_soundEventId(soundEventId)
    , m_category(std::nullopt)
{
}

StopSoundPacket::StopSoundPacket(SoundCategory category)
    : network::Packet(network::PacketType::StopSound)
    , m_soundEventId(std::nullopt)
    , m_category(category)
{
}

size_t StopSoundPacket::expectedSize() const {
    // 标志字节 + 可选的声音事件ID + 可选的类别
    return 64;
}

Result<std::vector<u8>> StopSoundPacket::serialize() const {
    network::PacketSerializer serializer;
    serializer.reserve(expectedSize());

    // 标志字节：
    // bit 0: 有声音事件ID
    // bit 1: 有类别
    u8 flags = 0;
    if (m_soundEventId.has_value()) {
        flags |= 0x01;
    }
    if (m_category.has_value()) {
        flags |= 0x02;
    }
    serializer.writeU8(flags);

    // 可选：声音事件ID
    if (m_soundEventId.has_value()) {
        serializer.writeString(m_soundEventId->toString());
    }

    // 可选：类别
    if (m_category.has_value()) {
        serializer.writeVarInt(static_cast<i32>(m_category.value()));
    }

    // 仅返回包体。外层网络头由 ConnectionManager::encapsulatePacket 写入。
    return serializer.buffer();
}

Result<void> StopSoundPacket::deserialize(const u8* data, size_t size) {
    network::PacketDeserializer deserializer(data, size);

    // 读取标志字节
    auto flagsResult = deserializer.readU8();
    if (!flagsResult.success()) {
        return flagsResult.error();
    }
    u8 flags = flagsResult.value();

    // 根据标志读取声音事件ID
    if (flags & 0x01) {
        auto idResult = deserializer.readString();
        if (!idResult.success()) {
            return idResult.error();
        }
        m_soundEventId = ResourceLocation::parse(idResult.value());
    } else {
        m_soundEventId = std::nullopt;
    }

    // 根据标志读取类别
    if (flags & 0x02) {
        auto categoryResult = deserializer.readVarInt();
        if (!categoryResult.success()) {
            return categoryResult.error();
        }
        SoundCategory category = static_cast<SoundCategory>(categoryResult.value());
        if (isValidSoundCategory(category)) {
            m_category = category;
        } else {
            m_category = std::nullopt;
        }
    } else {
        m_category = std::nullopt;
    }

    return {};
}

// ============================================================================
// PlaySoundEffectPacket
// ============================================================================

PlaySoundEffectPacket::PlaySoundEffectPacket()
    : network::Packet(network::PacketType::PlaySoundEffect)
{
}

PlaySoundEffectPacket::PlaySoundEffectPacket(const ResourceLocation& soundEventId,
                                             SoundCategory category,
                                             const glm::vec3& position,
                                             f32 volume,
                                             f32 pitch)
    : network::Packet(network::PacketType::PlaySoundEffect)
    , m_soundEventId(soundEventId)
    , m_category(category)
    , m_x(static_cast<i32>(position.x * 8.0f))
    , m_y(static_cast<i32>(position.y * 8.0f))
    , m_z(static_cast<i32>(position.z * 8.0f))
    , m_volume(volume)
    , m_pitch(pitch)
{
}

glm::vec3 PlaySoundEffectPacket::getPosition() const noexcept {
    return glm::vec3(
        static_cast<f32>(m_x) / 8.0f,
        static_cast<f32>(m_y) / 8.0f,
        static_cast<f32>(m_z) / 8.0f
    );
}

size_t PlaySoundEffectPacket::expectedSize() const {
    return 64;
}

Result<std::vector<u8>> PlaySoundEffectPacket::serialize() const {
    network::PacketSerializer serializer;
    serializer.reserve(expectedSize());

    // 与 PlaySoundPacket 格式相同
    serializer.writeString(m_soundEventId.toString());
    serializer.writeVarInt(static_cast<i32>(m_category));
    serializer.writeI32(m_x);
    serializer.writeI32(m_y);
    serializer.writeI32(m_z);
    serializer.writeF32(m_volume);
    serializer.writeF32(m_pitch);

    // 仅返回包体。外层网络头由 ConnectionManager::encapsulatePacket 写入。
    return serializer.buffer();
}

Result<void> PlaySoundEffectPacket::deserialize(const u8* data, size_t size) {
    network::PacketDeserializer deserializer(data, size);

    auto idResult = deserializer.readString();
    if (!idResult.success()) {
        return idResult.error();
    }
    m_soundEventId = ResourceLocation::parse(idResult.value());

    auto categoryResult = deserializer.readVarInt();
    if (!categoryResult.success()) {
        return categoryResult.error();
    }
    m_category = static_cast<SoundCategory>(categoryResult.value());
    if (!isValidSoundCategory(m_category)) {
        m_category = SoundCategory::Master;
    }

    auto xResult = deserializer.readI32();
    if (!xResult.success()) {
        return xResult.error();
    }
    m_x = xResult.value();

    auto yResult = deserializer.readI32();
    if (!yResult.success()) {
        return yResult.error();
    }
    m_y = yResult.value();

    auto zResult = deserializer.readI32();
    if (!zResult.success()) {
        return zResult.error();
    }
    m_z = zResult.value();

    auto volumeResult = deserializer.readF32();
    if (!volumeResult.success()) {
        return volumeResult.error();
    }
    m_volume = volumeResult.value();

    auto pitchResult = deserializer.readF32();
    if (!pitchResult.success()) {
        return pitchResult.error();
    }
    m_pitch = pitchResult.value();

    return {};
}

} // namespace mc::sound
