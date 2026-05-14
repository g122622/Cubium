#include "SleepPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

SleepPacket::SleepPacket(u32 entityId, const BlockPos& bedPos)
    : Packet(PacketType::Sleep)
    , m_entityId(entityId)
    , m_bedPos(bedPos)
{}

SleepPacket SleepPacket::createWakeUp(u32 entityId)
{
    SleepPacket packet;
    packet.setEntityId(entityId);
    // bedPos 默认为空，表示离开睡眠
    return packet;
}

Result<std::vector<u8>> SleepPacket::serialize() const
{
    PacketSerializer ser;

    // 写入实体ID
    ser.writeU32(m_entityId);

    // 写入是否有床位位置
    ser.writeBool(m_bedPos.has_value());

    // 如果有床位位置，写入坐标
    if (m_bedPos.has_value()) {
        ser.writeI32(m_bedPos->x);
        ser.writeI32(m_bedPos->y);
        ser.writeI32(m_bedPos->z);
    }

    return ser.buffer();
}

Result<void> SleepPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deser(data, size);

    // 读取实体ID
    auto entityIdResult = deser.readU32();
    if (entityIdResult.failed()) {
        return entityIdResult.error();
    }
    m_entityId = entityIdResult.value();

    // 读取是否有床位位置
    auto hasBedPosResult = deser.readBool();
    if (hasBedPosResult.failed()) {
        return hasBedPosResult.error();
    }

    if (hasBedPosResult.value()) {
        // 读取床位位置
        auto xResult = deser.readI32();
        if (xResult.failed()) {
            return xResult.error();
        }

        auto yResult = deser.readI32();
        if (yResult.failed()) {
            return yResult.error();
        }

        auto zResult = deser.readI32();
        if (zResult.failed()) {
            return zResult.error();
        }

        m_bedPos = BlockPos(xResult.value(), yResult.value(), zResult.value());
    } else {
        m_bedPos = std::nullopt;
    }

    return Result<void>::ok();
}

} // namespace mc::network
