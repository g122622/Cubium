#include "WorldBorderPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

WorldBorderPacket::WorldBorderPacket()
    : Packet(PacketType::WorldBorder)
{}

WorldBorderPacket::WorldBorderPacket(WorldBorderAction action)
    : Packet(PacketType::WorldBorder)
    , m_action(action)
{}

// ============================================================================
// 静态工厂方法
// ============================================================================

WorldBorderPacket WorldBorderPacket::setSize(double size)
{
    WorldBorderPacket packet(WorldBorderAction::SetSize);
    packet.m_size = size;
    return packet;
}

WorldBorderPacket WorldBorderPacket::lerpSize(double oldSize, double newSize, u64 timeMs)
{
    WorldBorderPacket packet(WorldBorderAction::LerpSize);
    packet.m_oldSize = oldSize;
    packet.m_newSize = newSize;
    packet.m_timeMs = timeMs;
    return packet;
}

WorldBorderPacket WorldBorderPacket::setCenter(double x, double z)
{
    WorldBorderPacket packet(WorldBorderAction::SetCenter);
    packet.m_centerX = x;
    packet.m_centerZ = z;
    return packet;
}

WorldBorderPacket WorldBorderPacket::initialize(const world::border::WorldBorder& border)
{
    WorldBorderPacket packet(WorldBorderAction::Initialize);
    packet.m_centerX = border.getCenterX();
    packet.m_centerZ = border.getCenterZ();
    packet.m_size = border.getSize();
    packet.m_newSize = border.getTargetSize();
    packet.m_timeMs = border.getTimeUntilTarget();
    packet.m_damagePerBlock = border.getDamagePerBlock();
    packet.m_damageBuffer = border.getDamageBuffer();
    packet.m_warningTime = border.getWarningTime();
    packet.m_warningDistance = border.getWarningDistance();
    return packet;
}

WorldBorderPacket WorldBorderPacket::setWarningTime(i32 warningTime)
{
    WorldBorderPacket packet(WorldBorderAction::SetWarningTime);
    packet.m_warningTime = warningTime;
    return packet;
}

WorldBorderPacket WorldBorderPacket::setWarningDistance(i32 warningDistance)
{
    WorldBorderPacket packet(WorldBorderAction::SetWarningDistance);
    packet.m_warningDistance = warningDistance;
    return packet;
}

WorldBorderPacket WorldBorderPacket::setDamageBuffer(double damageBuffer)
{
    WorldBorderPacket packet(WorldBorderAction::SetDamageBuffer);
    packet.m_damageBuffer = damageBuffer;
    return packet;
}

WorldBorderPacket WorldBorderPacket::setDamagePerBlock(double damagePerBlock)
{
    WorldBorderPacket packet(WorldBorderAction::SetDamagePerBlock);
    packet.m_damagePerBlock = damagePerBlock;
    return packet;
}

// ============================================================================
// 序列化
// ============================================================================

Result<std::vector<u8>> WorldBorderPacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU8(static_cast<u8>(m_action));

    switch (m_action) {
        case WorldBorderAction::SetSize:
            serializer.writeF64(m_size);
            break;

        case WorldBorderAction::LerpSize:
            serializer.writeF64(m_oldSize);
            serializer.writeF64(m_newSize);
            serializer.writeVarLong(static_cast<i64>(m_timeMs));
            break;

        case WorldBorderAction::SetCenter:
            serializer.writeF64(m_centerX);
            serializer.writeF64(m_centerZ);
            break;

        case WorldBorderAction::Initialize:
            serializer.writeF64(m_centerX);
            serializer.writeF64(m_centerZ);
            serializer.writeF64(m_oldSize); // 当前大小
            serializer.writeF64(m_newSize); // 目标大小
            serializer.writeVarLong(static_cast<i64>(m_timeMs));
            serializer.writeVarLong(static_cast<i64>(m_warningTime));
            serializer.writeVarLong(static_cast<i64>(m_warningDistance));
            serializer.writeF64(m_damageBuffer);
            serializer.writeF64(m_damagePerBlock);
            break;

        case WorldBorderAction::SetWarningTime:
            serializer.writeVarLong(static_cast<i64>(m_warningTime));
            break;

        case WorldBorderAction::SetWarningDistance:
            serializer.writeVarLong(static_cast<i64>(m_warningDistance));
            break;

        case WorldBorderAction::SetDamageBuffer:
            serializer.writeF64(m_damageBuffer);
            break;

        case WorldBorderAction::SetDamagePerBlock:
            serializer.writeF64(m_damagePerBlock);
            break;
    }

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> WorldBorderPacket::deserialize(const u8* data, size_t size)
{
    if (size < 1) {
        return Error(ErrorCode::InvalidPacket, "WorldBorderPacket: data too short");
    }

    PacketDeserializer deserializer(data, size);

    auto actionResult = deserializer.readU8();
    if (!actionResult.success()) {
        return actionResult.error();
    }
    m_action = static_cast<WorldBorderAction>(actionResult.value());

    switch (m_action) {
        case WorldBorderAction::SetSize: {
            auto sizeResult = deserializer.readF64();
            if (!sizeResult.success()) return sizeResult.error();
            m_size = sizeResult.value();
            break;
        }

        case WorldBorderAction::LerpSize: {
            auto oldSizeResult = deserializer.readF64();
            if (!oldSizeResult.success()) return oldSizeResult.error();
            m_oldSize = oldSizeResult.value();

            auto newSizeResult = deserializer.readF64();
            if (!newSizeResult.success()) return newSizeResult.error();
            m_newSize = newSizeResult.value();

            auto timeResult = deserializer.readVarLong();
            if (!timeResult.success()) return timeResult.error();
            m_timeMs = static_cast<u64>(timeResult.value());
            break;
        }

        case WorldBorderAction::SetCenter: {
            auto xResult = deserializer.readF64();
            if (!xResult.success()) return xResult.error();
            m_centerX = xResult.value();

            auto zResult = deserializer.readF64();
            if (!zResult.success()) return zResult.error();
            m_centerZ = zResult.value();
            break;
        }

        case WorldBorderAction::Initialize: {
            auto centerXResult = deserializer.readF64();
            if (!centerXResult.success()) return centerXResult.error();
            m_centerX = centerXResult.value();

            auto centerZResult = deserializer.readF64();
            if (!centerZResult.success()) return centerZResult.error();
            m_centerZ = centerZResult.value();

            auto sizeResult = deserializer.readF64();
            if (!sizeResult.success()) return sizeResult.error();
            m_size = sizeResult.value();

            auto targetSizeResult = deserializer.readF64();
            if (!targetSizeResult.success()) return targetSizeResult.error();
            m_newSize = targetSizeResult.value();

            auto timeResult = deserializer.readVarLong();
            if (!timeResult.success()) return timeResult.error();
            m_timeMs = static_cast<u64>(timeResult.value());

            auto warningTimeResult = deserializer.readVarLong();
            if (!warningTimeResult.success()) return warningTimeResult.error();
            m_warningTime = static_cast<i32>(warningTimeResult.value());

            auto warningDistResult = deserializer.readVarLong();
            if (!warningDistResult.success()) return warningDistResult.error();
            m_warningDistance = static_cast<i32>(warningDistResult.value());

            auto bufferResult = deserializer.readF64();
            if (!bufferResult.success()) return bufferResult.error();
            m_damageBuffer = bufferResult.value();

            auto damageResult = deserializer.readF64();
            if (!damageResult.success()) return damageResult.error();
            m_damagePerBlock = damageResult.value();
            break;
        }

        case WorldBorderAction::SetWarningTime: {
            auto timeResult = deserializer.readVarLong();
            if (!timeResult.success()) return timeResult.error();
            m_warningTime = static_cast<i32>(timeResult.value());
            break;
        }

        case WorldBorderAction::SetWarningDistance: {
            auto distResult = deserializer.readVarLong();
            if (!distResult.success()) return distResult.error();
            m_warningDistance = static_cast<i32>(distResult.value());
            break;
        }

        case WorldBorderAction::SetDamageBuffer: {
            auto bufferResult = deserializer.readF64();
            if (!bufferResult.success()) return bufferResult.error();
            m_damageBuffer = bufferResult.value();
            break;
        }

        case WorldBorderAction::SetDamagePerBlock: {
            auto damageResult = deserializer.readF64();
            if (!damageResult.success()) return damageResult.error();
            m_damagePerBlock = damageResult.value();
            break;
        }
    }

    return Result<void>();
}

size_t WorldBorderPacket::expectedSize() const
{
    // 动作类型 (1) + 数据
    size_t base = sizeof(PacketHeader) + 1;

    switch (m_action) {
        case WorldBorderAction::SetSize:
            return base + 8; // f64

        case WorldBorderAction::LerpSize:
            return base + 8 + 8 + 10; // f64 + f64 + varlong (最多10字节)

        case WorldBorderAction::SetCenter:
            return base + 8 + 8; // f64 + f64

        case WorldBorderAction::Initialize:
            return base + 8 + 8 + 8 + 8 + 10 + 10 + 10 + 8 + 8; // 所有字段

        case WorldBorderAction::SetWarningTime:
        case WorldBorderAction::SetWarningDistance:
            return base + 10; // varlong (最多10字节)

        case WorldBorderAction::SetDamageBuffer:
        case WorldBorderAction::SetDamagePerBlock:
            return base + 8; // f64
    }

    return base;
}

} // namespace mc::network
