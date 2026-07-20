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

#include "ParticlePacket.hpp"
#include "PacketDeserializer.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc::network {

ParticlePacket::ParticlePacket(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count)
    : Packet(PacketType::Particle)
    , m_particleType(type)
    , m_x(pos.x)
    , m_y(pos.y)
    , m_z(pos.z)
    , m_velocityX(velocity.x)
    , m_velocityY(velocity.y)
    , m_velocityZ(velocity.z)
    , m_offsetX(offset.x)
    , m_offsetY(offset.y)
    , m_offsetZ(offset.z)
    , m_count(count)
{}

size_t ParticlePacket::expectedSize() const noexcept
{
    // 基础大小：包头 + VarInt(粒子类型) + 3*f64(位置) + 3*f32(速度) + 3*f32(偏移) + VarInt(数量) + VarInt(数据长度)
    // 保守估计：12 (header) + 5 + 24 + 12 + 12 + 5 + 5 = 75 bytes
    // 加上可选数据
    return sizeof(PacketHeader) + 64 + m_optionalData.size();
}

Result<std::vector<u8>> ParticlePacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    // 写入粒子类型ID（转换为 MC 协议 ID）
    // 内部扩展粒子（115~123）不在 MC 协议中，映射为最接近的协议粒子类型
    serializer.writeVarInt(particle::toProtocolId(m_particleType));

    // 写入位置 (f64)
    serializer.writeF64(m_x);
    serializer.writeF64(m_y);
    serializer.writeF64(m_z);

    // 写入偏移 (f32) - 协议顺序：先偏移后速度
    serializer.writeF32(m_offsetX);
    serializer.writeF32(m_offsetY);
    serializer.writeF32(m_offsetZ);

    // 写入速度 (f32)
    serializer.writeF32(m_velocityX);
    serializer.writeF32(m_velocityY);
    serializer.writeF32(m_velocityZ);

    // 写入粒子数量
    serializer.writeVarInt(static_cast<i32>(m_count));

    // 写入可选数据长度和数据
    serializer.writeVarInt(static_cast<i32>(m_optionalData.size()));
    if (!m_optionalData.empty()) {
        serializer.writeBytes(m_optionalData.data(), m_optionalData.size());
    }

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> ParticlePacket::deserialize(const u8* data, size_t size)
{
    if (size < sizeof(PacketHeader)) {
        return Error(ErrorCode::InvalidData, "ParticlePacket: insufficient data for header");
    }

    PacketDeserializer deserializer(data, size);

    // 读取粒子类型ID
    auto typeResult = deserializer.readVarInt();
    if (!typeResult.success()) {
        return typeResult.error();
    }
    // 读取粒子类型ID（从 MC 协议 ID 转换为内部枚举）
    m_particleType = particle::fromProtocolId(typeResult.value());

    // 验证粒子类型
    if (!particle::isValidParticleType(m_particleType)) {
        return Error(ErrorCode::InvalidData, "ParticlePacket: invalid particle type");
    }

    // 读取位置 (f64)
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

    // 读取偏移 (f32)
    auto oxResult = deserializer.readF32();
    if (!oxResult.success()) {
        return oxResult.error();
    }
    m_offsetX = oxResult.value();

    auto oyResult = deserializer.readF32();
    if (!oyResult.success()) {
        return oyResult.error();
    }
    m_offsetY = oyResult.value();

    auto ozResult = deserializer.readF32();
    if (!ozResult.success()) {
        return ozResult.error();
    }
    m_offsetZ = ozResult.value();

    // 读取速度 (f32)
    auto vxResult = deserializer.readF32();
    if (!vxResult.success()) {
        return vxResult.error();
    }
    m_velocityX = vxResult.value();

    auto vyResult = deserializer.readF32();
    if (!vyResult.success()) {
        return vyResult.error();
    }
    m_velocityY = vyResult.value();

    auto vzResult = deserializer.readF32();
    if (!vzResult.success()) {
        return vzResult.error();
    }
    m_velocityZ = vzResult.value();

    // 读取粒子数量
    auto countResult = deserializer.readVarInt();
    if (!countResult.success()) {
        return countResult.error();
    }
    m_count = static_cast<u32>(countResult.value());

    // 读取可选数据
    auto dataLenResult = deserializer.readVarInt();
    if (!dataLenResult.success()) {
        return dataLenResult.error();
    }
    auto dataLen = static_cast<size_t>(dataLenResult.value());

    if (dataLen > 0) {
        if (!deserializer.hasRemaining(dataLen)) {
            return Error(ErrorCode::InvalidData, "ParticlePacket: insufficient data for optional data");
        }
        m_optionalData.resize(dataLen);
        auto readResult = deserializer.readBytesInto(m_optionalData.data(), dataLen);
        if (!readResult.success()) {
            return readResult.error();
        }
    } else {
        m_optionalData.clear();
    }

    return {};
}

// static
ParticlePacket ParticlePacket::create(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count)
{
    return ParticlePacket(type, pos, velocity, offset, count);
}

// static
ParticlePacket ParticlePacket::createSingle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
{
    return ParticlePacket(type, pos, velocity, Vector3(0.0f, 0.0f, 0.0f), 1);
}

// static
ParticlePacket ParticlePacket::createVibration(const Vector3& pos, const BlockPos& targetBlockPos, i32 arrivalInTicks)
{
    // 构建振动粒子包：粒子类型为 Vibration，无偏移，数量为 1
    ParticlePacket packet(particle::ParticleTypeId::Vibration,
        pos,
        Vector3(0.0f, 0.0f, 0.0f), // 无速度
        Vector3(0.0f, 0.0f, 0.0f), // 无偏移
        1);                        // 单个粒子

    // 编码可选数据（与 MC Java 1.21.11 VibrationParticleOption.STREAM_CODEC 一致）：
    //   VarInt positionSourceTypeId(0=Block)
    //   i64 packedBlockPos (BlockPos.asLong)
    //   VarInt arrivalInTicks
    PacketSerializer serializer(16);
    serializer.writeVarInt(0); // BLOCK = 0
    serializer.writeI64(targetBlockPos.asLong());
    serializer.writeVarInt(arrivalInTicks);

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

// static
ParticlePacket ParticlePacket::createVibration(
    const Vector3& pos, EntityInstanceId targetEntityId, f32 yOffset, i32 arrivalInTicks)
{
    // 构建振动粒子包：粒子类型为 Vibration，无偏移，数量为 1
    ParticlePacket packet(particle::ParticleTypeId::Vibration,
        pos,
        Vector3(0.0f, 0.0f, 0.0f), // 无速度
        Vector3(0.0f, 0.0f, 0.0f), // 无偏移
        1);                        // 单个粒子

    // 编码可选数据（与 MC Java 1.21.11 VibrationParticleOption.STREAM_CODEC 一致）：
    //   VarInt positionSourceTypeId(1=Entity)
    //   VarInt entityId
    //   f32 yOffset
    //   VarInt arrivalInTicks
    PacketSerializer serializer(16);
    serializer.writeVarInt(1); // ENTITY = 1
    serializer.writeVarInt(static_cast<i32>(targetEntityId));
    serializer.writeF32(yOffset);
    serializer.writeVarInt(arrivalInTicks);

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

bool ParticlePacket::isVibrationParticle() const noexcept
{
    return m_particleType == particle::ParticleTypeId::Vibration && !m_optionalData.empty();
}

std::optional<ParticlePacket::VibrationTarget> ParticlePacket::decodeVibrationTarget() const
{
    if (!isVibrationParticle()) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 读取 PositionSource 类型 ID (VarInt)
    auto typeResult = deserializer.readVarInt();
    if (!typeResult.success()) {
        return std::nullopt;
    }

    VibrationTarget target;
    switch (typeResult.value()) {
        case 0: { // BLOCK
            target.kind = VibrationTarget::Kind::Block;
            auto packedResult = deserializer.readI64();
            if (!packedResult.success()) {
                return std::nullopt;
            }
            target.blockPos = BlockPos::fromLong(packedResult.value());
            break;
        }
        case 1: { // ENTITY
            target.kind = VibrationTarget::Kind::Entity;
            auto entityIdResult = deserializer.readVarInt();
            if (!entityIdResult.success()) {
                return std::nullopt;
            }
            auto yOffsetResult = deserializer.readF32();
            if (!yOffsetResult.success()) {
                return std::nullopt;
            }
            target.entityId = static_cast<EntityInstanceId>(entityIdResult.value());
            target.yOffset = yOffsetResult.value();
            break;
        }
        default:
            // 未知的位置源类型
            return std::nullopt;
    }

    return target;
}

std::optional<i32> ParticlePacket::decodeVibrationArrivalInTicks() const
{
    if (!isVibrationParticle()) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 跳过 PositionSource 部分：先读类型 ID，再按类型跳过对应字段
    auto typeResult = deserializer.readVarInt();
    if (!typeResult.success()) {
        return std::nullopt;
    }

    switch (typeResult.value()) {
        case 0: { // BLOCK: 跳过 i64 packedBlockPos
            auto packedResult = deserializer.readI64();
            if (!packedResult.success()) {
                return std::nullopt;
            }
            break;
        }
        case 1: { // ENTITY: 跳过 VarInt entityId + f32 yOffset
            auto entityIdResult = deserializer.readVarInt();
            if (!entityIdResult.success()) {
                return std::nullopt;
            }
            auto yOffsetResult = deserializer.readF32();
            if (!yOffsetResult.success()) {
                return std::nullopt;
            }
            break;
        }
        default:
            return std::nullopt;
    }

    // 读取 arrivalInTicks
    auto tickResult = deserializer.readVarInt();
    if (!tickResult.success()) {
        return std::nullopt;
    }

    return tickResult.value();
}

// static
ParticlePacket ParticlePacket::createTrail(
    const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks)
{
    // 构建轨迹粒子包：粒子类型为 Trail，无偏移，数量为 1
    ParticlePacket packet(particle::ParticleTypeId::Trail,
        pos,
        Vector3(0.0f, 0.0f, 0.0f), // 无速度
        Vector3(0.0f, 0.0f, 0.0f), // 无偏移
        1);                        // 单个粒子

    // 编码可选数据：f64 targetX, f64 targetY, f64 targetZ, i32 color(ARGB), VarInt durationInTicks
    PacketSerializer serializer(36);
    serializer.writeF64(targetPosition.x);
    serializer.writeF64(targetPosition.y);
    serializer.writeF64(targetPosition.z);
    serializer.writeI32(static_cast<i32>(color));
    serializer.writeVarInt(durationInTicks);

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

bool ParticlePacket::isTrailParticle() const noexcept
{
    return m_particleType == particle::ParticleTypeId::Trail && !m_optionalData.empty();
}

std::optional<Vector3d> ParticlePacket::decodeTrailTarget() const
{
    if (!isTrailParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：f64 targetX, f64 targetY, f64 targetZ, i32 color(ARGB), VarInt durationInTicks
    if (m_optionalData.size() < 3 * sizeof(f64)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    auto xResult = deserializer.readF64();
    if (!xResult.success()) {
        return std::nullopt;
    }

    auto yResult = deserializer.readF64();
    if (!yResult.success()) {
        return std::nullopt;
    }

    auto zResult = deserializer.readF64();
    if (!zResult.success()) {
        return std::nullopt;
    }

    return Vector3d(xResult.value(), yResult.value(), zResult.value());
}

std::optional<u32> ParticlePacket::decodeTrailColor() const
{
    if (!isTrailParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：f64 targetX, f64 targetY, f64 targetZ, i32 color(ARGB), VarInt durationInTicks
    // 需要至少 3*f64 + i32 = 28 字节
    if (m_optionalData.size() < 3 * sizeof(f64) + sizeof(i32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 跳过 3 个 f64（targetX, targetY, targetZ）
    auto xResult = deserializer.readF64();
    if (!xResult.success()) {
        return std::nullopt;
    }

    auto yResult = deserializer.readF64();
    if (!yResult.success()) {
        return std::nullopt;
    }

    auto zResult = deserializer.readF64();
    if (!zResult.success()) {
        return std::nullopt;
    }

    auto colorResult = deserializer.readI32();
    if (!colorResult.success()) {
        return std::nullopt;
    }

    return static_cast<u32>(colorResult.value());
}

std::optional<i32> ParticlePacket::decodeTrailDuration() const
{
    if (!isTrailParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：f64 targetX, f64 targetY, f64 targetZ, i32 color(ARGB), VarInt durationInTicks
    // 需要至少 3*f64 + i32 = 28 字节才有 VarInt 数据
    if (m_optionalData.size() < 3 * sizeof(f64) + sizeof(i32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 跳过 3 个 f64（targetX, targetY, targetZ）
    auto xResult = deserializer.readF64();
    if (!xResult.success()) {
        return std::nullopt;
    }

    auto yResult = deserializer.readF64();
    if (!yResult.success()) {
        return std::nullopt;
    }

    auto zResult = deserializer.readF64();
    if (!zResult.success()) {
        return std::nullopt;
    }

    // 跳过 i32 color
    auto colorResult = deserializer.readI32();
    if (!colorResult.success()) {
        return std::nullopt;
    }

    auto durationResult = deserializer.readVarInt();
    if (!durationResult.success()) {
        return std::nullopt;
    }

    return durationResult.value();
}

// static
ParticlePacket ParticlePacket::createDust(particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    u32 color,
    f32 scale)
{
    ParticlePacket packet(type, pos, velocity, offset, count);

    // 编码可选数据：i32 color(ARGB), f32 scale
    PacketSerializer serializer(8);
    serializer.writeI32(static_cast<i32>(color));
    serializer.writeF32(scale);

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

// static
ParticlePacket ParticlePacket::createDustColorTransition(const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    u32 fromColor,
    u32 toColor,
    f32 scale)
{
    ParticlePacket packet(particle::ParticleTypeId::DustColorTransition, pos, velocity, offset, count);

    // 编码可选数据：i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
    PacketSerializer serializer(12);
    serializer.writeI32(static_cast<i32>(fromColor));
    serializer.writeI32(static_cast<i32>(toColor));
    serializer.writeF32(scale);

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

bool ParticlePacket::isDustParticle() const noexcept
{
    return (m_particleType == particle::ParticleTypeId::Dust || m_particleType == particle::ParticleTypeId::Redstone) &&
        !m_optionalData.empty();
}

std::optional<u32> ParticlePacket::decodeDustColor() const
{
    if (!isDustParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：i32 color(ARGB), f32 scale
    if (m_optionalData.size() < sizeof(i32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());
    auto colorResult = deserializer.readI32();
    if (!colorResult.success()) {
        return std::nullopt;
    }

    return static_cast<u32>(colorResult.value());
}

std::optional<f32> ParticlePacket::decodeDustScale() const
{
    if (!isDustParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：i32 color(ARGB), f32 scale
    if (m_optionalData.size() < sizeof(i32) + sizeof(f32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 跳过 i32 color
    auto colorResult = deserializer.readI32();
    if (!colorResult.success()) {
        return std::nullopt;
    }

    auto scaleResult = deserializer.readF32();
    if (!scaleResult.success()) {
        return std::nullopt;
    }

    return scaleResult.value();
}

bool ParticlePacket::isDustColorTransitionParticle() const noexcept
{
    return m_particleType == particle::ParticleTypeId::DustColorTransition && !m_optionalData.empty();
}

std::optional<u32> ParticlePacket::decodeDustColorTransitionFromColor() const
{
    if (!isDustColorTransitionParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
    if (m_optionalData.size() < sizeof(i32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());
    auto colorResult = deserializer.readI32();
    if (!colorResult.success()) {
        return std::nullopt;
    }

    return static_cast<u32>(colorResult.value());
}

std::optional<u32> ParticlePacket::decodeDustColorTransitionToColor() const
{
    if (!isDustColorTransitionParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
    if (m_optionalData.size() < 2 * sizeof(i32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 跳过 i32 fromColor
    auto fromResult = deserializer.readI32();
    if (!fromResult.success()) {
        return std::nullopt;
    }

    auto toResult = deserializer.readI32();
    if (!toResult.success()) {
        return std::nullopt;
    }

    return static_cast<u32>(toResult.value());
}

std::optional<f32> ParticlePacket::decodeDustColorTransitionScale() const
{
    if (!isDustColorTransitionParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：i32 fromColor(ARGB), i32 toColor(ARGB), f32 scale
    if (m_optionalData.size() < 2 * sizeof(i32) + sizeof(f32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());

    // 跳过 i32 fromColor, i32 toColor
    auto fromResult = deserializer.readI32();
    if (!fromResult.success()) {
        return std::nullopt;
    }

    auto toResult = deserializer.readI32();
    if (!toResult.success()) {
        return std::nullopt;
    }

    auto scaleResult = deserializer.readF32();
    if (!scaleResult.success()) {
        return std::nullopt;
    }

    return scaleResult.value();
}

// static
ParticlePacket ParticlePacket::createEntityEffect(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color)
{
    ParticlePacket packet(particle::ParticleTypeId::EntityEffect, pos, velocity, offset, count);

    // 编码可选数据：i32 color(ARGB)
    PacketSerializer serializer(4);
    serializer.writeI32(static_cast<i32>(color));

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

bool ParticlePacket::isEntityEffectParticle() const noexcept
{
    return m_particleType == particle::ParticleTypeId::EntityEffect && !m_optionalData.empty();
}

std::optional<u32> ParticlePacket::decodeEntityEffectColor() const
{
    if (!isEntityEffectParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：i32 color(ARGB)
    if (m_optionalData.size() < sizeof(i32)) {
        return std::nullopt;
    }

    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());
    auto colorResult = deserializer.readI32();
    if (!colorResult.success()) {
        return std::nullopt;
    }

    return static_cast<u32>(colorResult.value());
}

// static
ParticlePacket ParticlePacket::createBlock(particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    u32 blockStateId)
{
    ParticlePacket packet(type, pos, velocity, offset, count);

    // 编码可选数据：VarInt blockStateId
    PacketSerializer serializer(5);
    serializer.writeVarInt(static_cast<i32>(blockStateId));

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

bool ParticlePacket::isBlockParticle() const noexcept
{
    return particle::requiresBlockState(m_particleType) && !m_optionalData.empty();
}

std::optional<u32> ParticlePacket::decodeBlockStateId() const
{
    if (!isBlockParticle()) {
        return std::nullopt;
    }

    // 可选数据格式：VarInt blockStateId
    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());
    auto stateIdResult = deserializer.readVarInt();
    if (!stateIdResult.success()) {
        return std::nullopt;
    }

    return static_cast<u32>(stateIdResult.value());
}

// static
ParticlePacket ParticlePacket::createItem(particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    const ::mc::ItemStack& itemStack)
{
    ParticlePacket packet(type, pos, velocity, offset, count);

    // 将 ItemStack 序列化到 PacketSerializer，再将字节流复制到 optionalData
    // ItemStack::serialize 写入：bool(present) + u16(itemId) + i32(count) + 可选字段
    // 整个字节流即为 optionalData 内容，无需额外长度前缀
    PacketSerializer serializer(32);
    itemStack.serialize(serializer);

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());
    packet.setOptionalData(std::move(data));

    return packet;
}

bool ParticlePacket::isItemParticle() const noexcept
{
    return particle::requiresItemData(m_particleType) && !m_optionalData.empty();
}

std::optional<::mc::ItemStack> ParticlePacket::decodeItemStack() const
{
    if (!isItemParticle()) {
        return std::nullopt;
    }

    // 用 optionalData 构造 PacketDeserializer，调用 ItemStack::deserialize 解析
    PacketDeserializer deserializer(m_optionalData.data(), m_optionalData.size());
    auto itemResult = ::mc::ItemStack::deserialize(deserializer);
    if (!itemResult.success()) {
        return std::nullopt;
    }

    return std::move(itemResult.value());
}

} // namespace mc::network
