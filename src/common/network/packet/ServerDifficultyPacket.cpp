#include "ServerDifficultyPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

ServerDifficultyPacket::ServerDifficultyPacket()
    : Packet(PacketType::ServerDifficulty)
{}

ServerDifficultyPacket::ServerDifficultyPacket(Difficulty difficulty, bool locked)
    : Packet(PacketType::ServerDifficulty)
    , m_difficulty(difficulty)
    , m_locked(locked)
{}

Result<std::vector<u8>> ServerDifficultyPacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    serializer.writeU8(static_cast<u8>(m_difficulty));
    serializer.writeBool(m_locked);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> ServerDifficultyPacket::deserialize(const u8* data, size_t size)
{
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidPacket, "ServerDifficultyPacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    // 跳过包头（由上层处理）
    // 这里直接读取数据

    auto difficultyResult = deserializer.readU8();
    if (difficultyResult.failed()) {
        return difficultyResult.error();
    }
    m_difficulty = static_cast<Difficulty>(difficultyResult.value());

    auto lockedResult = deserializer.readBool();
    if (lockedResult.failed()) {
        return lockedResult.error();
    }
    m_locked = lockedResult.value();

    return Result<void>::ok();
}

size_t ServerDifficultyPacket::expectedSize() const
{
    // difficulty (1 byte) + locked (1 byte)
    return 2;
}

} // namespace mc::network
