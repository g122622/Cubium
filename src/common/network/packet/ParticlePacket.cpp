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

namespace mc::network {

ParticlePacket::ParticlePacket(client::renderer::trident::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
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

    // 写入粒子类型ID
    serializer.writeVarInt(static_cast<i32>(m_particleType));

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
    m_particleType = static_cast<client::renderer::trident::particle::ParticleTypeId>(typeResult.value());

    // 验证粒子类型
    if (!client::renderer::trident::particle::isValidParticleType(m_particleType)) {
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
ParticlePacket ParticlePacket::create(client::renderer::trident::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    return ParticlePacket(type, pos, velocity, offset, count);
}

// static
ParticlePacket ParticlePacket::createSingle(
    client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
{
    return ParticlePacket(type, pos, velocity, Vector3(0.0f, 0.0f, 0.0f), 1);
}

} // namespace mc::network
